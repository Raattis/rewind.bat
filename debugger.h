
enum { DEBUGGER_TRACE = 0&&TRACE };
#define dbg_printf(...) do { if (DEBUGGER_TRACE) printf(__VA_ARGS__); } while(0)

typedef int(*DebugSprintf)(int indent, char*, const void*, const void*);

int sprintfIndent(int indent, char* buffer)
{
	return sprintf(buffer, "%s", "                      " + 20 - indent * 2);
}

int sprintfSingle(int indent, char* buffer, const void* fmt, const void* ptr)
{
	return sprintf(buffer, (const char*)fmt, *(long long*)ptr);
}

int sprintfFloat(int indent, char* buffer, const void* fmt, const void* ptr)
{
	return sprintf(buffer, (const char*)fmt, *(float*)ptr);
}

int sprintfDouble(int indent, char* buffer, const void* fmt, const void* ptr)
{
	return sprintf(buffer, (const char*)fmt, *(double*)ptr);
}

#define COMMA ,
#define PRINTF_GENERIC(VarType, Name, Fmt) VarType: sprintf ## Name
#define USER_PTR(VarType, Name, Fmt) Name*: Fmt

#define _GET_PRINTF_FUNC(x) _Generic((x), \
	DEBUG_USER_TYPES(PRINTF_GENERIC, COMMA), \
	float: sprintfFloat, \
	double: sprintfDouble, \
	default: sprintfSingle \
)

#define _GET_USER_PTR(x) _Generic((x), \
	DEBUG_USER_TYPES(USER_PTR, COMMA), \
	default: _DEBUG_STR_FMT(x))

#define _DEBUG_STR_FMT(x) (_Generic((x), \
	char: "%c", \
	unsigned char: "%hhu", \
	short: "%hd", \
	unsigned short: "%hu", \
	unsigned: "%u", \
	int: "%d", \
	float: "%f", \
	double: "%f", \
	char*: "%s", \
	const char*: "%s", \
	void*: "%p", \
	const void*: "%p", \
	default: "%p"))

#define _DEBUG_VAR(var) \
	do { \
		DebugSprintf f = _GET_PRINTF_FUNC(owner->var); \
		void* user_ptr = _GET_USER_PTR(owner->var); \
		buffer += sprintfIndent(indent + 1, buffer); \
		buffer += sprintf(buffer, #var " = "); \
		buffer += f(indent + 1, buffer, user_ptr, &owner->var); \
		buffer += sprintf(buffer, ",\n"); \
	} while(0)

#define _DEBUG_STRUCT(Struct, VARS) \
int sprintf ## Struct(int indent, char *buffer, const void* user_ptr, const void* ptr) \
{ \
	const Struct* owner = (const Struct*)ptr; \
	const char* original = buffer; \
	buffer += sprintf(buffer, #Struct " [0x%llX] {\n", ptr); \
	VARS(_DEBUG_VAR); \
	buffer += sprintfIndent(indent, buffer); \
	buffer += sprintf(buffer, "}"); \
	return buffer - original; \
}

typedef struct _t_Variable
{
	char buffer[64];
	const void* address;
	DebugSprintf sprintf_func;
	const void* user_ptr;
	int assignment_line;
	int is_arg;
} Variable;

typedef struct _t_Scope
{
	Variable variables[64];
	int count;
	const char* function;
	const char* file;
	int line;
	int depth;
} Scope;

typedef struct _t_Debug_Local_Scope
{
	int depth;
} _Debug_Local_Scope;

typedef struct _t_Location
{
	const char* content;
	const char* file;
	int line;
} Location;

typedef struct _t_Debugger
{
	int depth;
	Scope scope[64];

	Location breakpoints[64];

	Location current_location;

	int ignore_counter;
	int break_depth_or_below;
} Debugger;


_Debug_Local_Scope push_scope(Debugger* debugger, const char* function, const char* file, int line)
{
	int depth = ++debugger->depth;

	Scope* scope = &debugger->scope[depth];
	memset(scope, 0, sizeof(*scope));
	scope->function = function;
	scope->file = file;
	scope->line = line;
	scope->depth = depth;
	_Debug_Local_Scope local_scope;
	local_scope.depth = depth;
	return local_scope;
}

void pop_scope(Debugger* debugger, _Debug_Local_Scope local_scope)
{
	int depth = local_scope.depth - 1;
	FATAL(depth >= 0, "%d -> %d depth", debugger->depth, depth);
	debugger->depth = depth;
}

Scope* get_scope(Debugger* debugger)
{
	FATAL(debugger->depth > 0, "%d depth", debugger->depth);
	return &debugger->scope[debugger->depth];
}

void push_variable(Scope* scope, const char* name, const void* address, DebugSprintf sprintf_func, const void* user_ptr, int line, int is_arg)
{
	Variable* variable = 0;
	for (int i = 0; i < scope->count; ++i)
	{
		if (strncmp(scope->variables[i].buffer, name, sizeof(scope->variables[i].buffer)) != 0)
			continue;

		variable = &scope->variables[i];
		break;
	}
	if (variable == 0)
	{
		variable = &scope->variables[scope->count++];
	}
	strcpy(variable->buffer, name);
	variable->address = address;
	variable->sprintf_func = sprintf_func;
	variable->user_ptr = user_ptr;
	variable->assignment_line = line;
	variable->is_arg = is_arg;
}

int sprint_lines(char* output, int buffer_len, const char* filename, int start, int count, int highlight)
{
	char* end = output + buffer_len;

	FILE* file = fopen(filename, "r");
	FATAL(file, "'%s' doesn't exist", filename);

	int lineno = 1;
	while(start-- > 1 && fgets(output, buffer_len, file)) ++lineno;

	while(count-- > 0)
	{
		char* line_start = output;
		if (lineno != highlight)
			output += sprintf(output, "%5d ", lineno);
		else
			output += sprintf(output, "   -->");
		FATAL(output < end, "Buffer overrun");

		if (!fgets(output, end - output, file))
		{
			output = line_start;
			*output = 0;
			break;
		}

		output += strlen(output);
		FATAL(output < end, "Buffer overrun");

		++lineno;
	}

	fclose(file);
}

char g_temp_buffer[16 * 1024];
int print_lines(const char* filename, int start, int count, int highlight)
{
	sprint_lines(g_temp_buffer, sizeof(g_temp_buffer), filename, start, count, highlight);
	printf("%s", g_temp_buffer);
}

int sprintf_value_impl(char* buffer, DebugSprintf sprintf_func, const void* user_ptr, const void* address)
{
	char* output = buffer;
	output += sprintf_func(0, output, user_ptr, address);
	return output - buffer;
}

const char* get_line(const char* filename, int line)
{
	FILE* file = fopen(filename, "r");
	if (!file)
	{
		fprintf(stderr, "'%s' doesn't exist", filename);
		return "<FILE_NOT_FOUND>";
	}

	while(line-- > 1 && fgets(g_temp_buffer, sizeof(g_temp_buffer), file));
	int success = !!fgets(g_temp_buffer, sizeof(g_temp_buffer), file);
	fclose(file);

	if (!success)
		return "EOF";

	g_temp_buffer[strlen(g_temp_buffer) - 1] = 0;
	return g_temp_buffer;
}

void print_variable(const char* name, DebugSprintf sprintf_func, const void* user_ptr, const void* address)
{
	char buffer[512] = {0};
	sprintf_value_impl(buffer, sprintf_func, user_ptr, address);
	printf("%s = %s\n", name, buffer);
}

#define PRINT_VARIABLE(var) do { print_variable(#var, _GET_PRINTF_FUNC(var), _GET_USER_PTR(var), &(var)); } while(0)
#define PRINT_VARIABLE_POINTER(var) do { print_variable(#var, _GET_PRINTF_FUNC(var), _GET_USER_PTR(var), (var)); } while(0)

void print_variable_from_scope(Scope* scope, const char* name)
{
	char buffer[512] = {0};
	char* output = buffer;
	for (int i = 0; i < scope->count; ++i)
	{
		if (strcmp(scope->variables[i].buffer, name) != 0)
			continue;
		Variable v = scope->variables[i];
		output += sprintf_value_impl(buffer, v.sprintf_func, v.user_ptr, v.address);
		printf("%s = %s\n", name, buffer);
		return;
	}
	printf("'%s' variable doesn't exist in scope", name);
}

void print_variables(Scope* scope, int args)
{
	char buffer[1024] = {0};
	for (int i = 0; i < scope->count; ++i)
	{
		char* output = buffer;
		Variable v = scope->variables[i];
		if (v.is_arg != args)
			continue;
		output += sprintf_value_impl(output, v.sprintf_func, v.user_ptr, v.address);
		printf("%s = %s\n", v.buffer, buffer);
	}
}

void print_stack(Debugger* debugger)
{
	for (int i = 1; i <= debugger->depth; ++i)
	{
		Scope* scope = &debugger->scope[i];
		printf("[%d] %s:%d scope: %s {\n", scope->depth, scope->file, scope->line, scope->function);
		print_variables(scope, 1);
		print_variables(scope, 0);
		printf("}\n");
	}
}

void print_location(Location location)
{
	if (location.file)
		printf("%s:%d '%s'\n", location.file, location.line, get_line(location.file, location.line));
	else
		printf("<null>:%d 'n/a'\n", location.line);
}

void do_command_ll(Debugger* debugger)
{
	print_lines(debugger->current_location.file, debugger->current_location.line - 10, 20, debugger->current_location.line);
}

void debug_interactively(Debugger* debugger)
{
	print_location(debugger->current_location);
	do_command_ll(debugger);

	fflush(stdin);
	char input[1024] = {0};
	int input_length = 0;

	while (1)
	{
		memset(input, 0, sizeof(input));
		input_length = 0;

		printf("(debugger) ");
		while (input_length < sizeof(input) && !strchr(input, '\n'))
		{
			if (fgets(input + input_length, sizeof(input) - input_length, stdin))
				input_length += strlen(input + input_length);
			Sleep(10);
			input[input_length] = 0;
			dbg_printf("%s\n", input);
		}

#define ERROR_AND_RETRY(p_error, ...) { fprintf(stderr, "error: " p_error "\n", ##__VA_ARGS__); continue; }
#define COMMAND(p_command) (strcmp(command, p_command) == 0)

		char command[20] = {0};
		char arg1[260] = {0};
		char arg2[260] = {0};
		char arg3[20] = {0};
		int arg1_int = 0;
		int arg2_int = 0;
		int arg3_int = 0;
		int arg_count = sscanf(input, "%19s %s %s %s", command, arg1, arg2, arg3) - 1;
		if (arg_count < 0)
			ERROR_AND_RETRY("Invalid input '%s'\n", input);

		if (arg_count > 0)
			sscanf(arg1, "%d", &arg1_int);

		if (arg_count > 1)
			sscanf(arg2, "%d", &arg2_int);

		if (arg_count > 2)
			sscanf(arg3, "%d", &arg3_int);

		dbg_printf("RAW_INPUT:'%s', COMMAND:'%s', [%d] 1:'%s'(%d), 2:'%s'(%d), 3:'%s'(%d)\n", input, command, arg_count, arg1, arg1_int, arg2, arg2_int, arg3, arg3_int);

		if (COMMAND("c") || COMMAND("continue"))
		{
			if (arg_count > 0)
				ERROR_AND_RETRY("continue doesn't take arguments");
			debugger->break_depth_or_below = -1;
			return;
		}
		else if (COMMAND("ignore"))
		{
			int number = -1;
			scanf(arg1, "%d", number);
			if (number < 0)
				ERROR_AND_RETRY("Invalid ignore count %d", number);
			debugger->ignore_counter = number;
			continue;
		}
		else if (COMMAND("exit"))
		{
			if (arg_count > 0)
				ERROR_AND_RETRY("exit doesn't take arguments");
			void exit(int);
			exit(1);
			continue;
		}
		else if (COMMAND("b") || COMMAND("break"))
		{
			int bp_count = sizeof(debugger->breakpoints) / sizeof(debugger->breakpoints[0]);
			if (arg_count == 0)
			{
				for (int i = 0; i < bp_count; ++i)
				{
					if (debugger->breakpoints[i].file == 0)
						continue;

					printf("Breakpoint %d at '%s:%d'\n", i, debugger->breakpoints[i].file, debugger->breakpoints[i].line);
				}
				continue;
			}

			int line_number = arg_count == 1 ? arg1_int : arg_count == 2 ? arg2_int : arg3_int;
			if (line_number < 0)
					ERROR_AND_RETRY("'%s' is not valid line number", arg_count == 1 ? arg1 : arg_count == 2 ? arg2 : arg3);

			const char* filepath = arg_count == 1 ? debugger->current_location.file : arg_count == 2 ? arg1 : arg2;
			int needs_to_allocate_string = arg_count > 1;

			int breakpoint_index = arg_count == 3 ? arg1_int : -1;
			if (breakpoint_index < 0 && arg_count == 3)
				ERROR_AND_RETRY("'%s' is not valid breakpoint index", arg1);

			if (breakpoint_index < 0)
			{
				breakpoint_index = 0;
				for (int i = 0; i < bp_count; ++i)
				{
					if (debugger->breakpoints[i].file != 0)
						continue;

					breakpoint_index = i;
					break;
				}
			}

			int i = breakpoint_index;
			if (needs_to_allocate_string)
			{
				void* malloc(size_t);
				char* new_string = malloc(strlen(filepath) + 1); // @LEAK;
				strcpy(new_string, filepath);
				debugger->breakpoints[i].file = new_string;
			}
			else
			{
				debugger->breakpoints[i].file = filepath;
			}
			debugger->breakpoints[i].line = line_number;
			printf("Breakpoint %d set to '%s:%d' '%s'\n", i, debugger->breakpoints[i].file, debugger->breakpoints[i].line);
			continue;
		}
		else if (COMMAND("s") || COMMAND("step") || COMMAND("d") || COMMAND("down"))
		{
			debugger->break_depth_or_below = 999;
			do_command_ll(debugger);
			return;
		}
		else if (COMMAND("n") || COMMAND("next"))
		{
			debugger->break_depth_or_below = debugger->depth;
			do_command_ll(debugger);
			return;
		}
		else if (COMMAND("u") || COMMAND("up"))
		{
			debugger->break_depth_or_below = debugger->depth - 1;
			do_command_ll(debugger);
			return;
		}
		else if (COMMAND("t") || COMMAND("trace"))
		{
			if (debugger->depth <= 0)
				ERROR_AND_RETRY("no scopes");
			printf("%s:%d\n", debugger->current_location.file, debugger->current_location.line);
			print_stack(debugger);
		}
		else if (COMMAND("ls") || COMMAND("locals") || COMMAND("lsa"))
		{
			if (debugger->depth <= 0)
				ERROR_AND_RETRY("no current scopes");
			print_variables(&debugger->scope[debugger->depth], 0);

			if (COMMAND("lsa"))
				print_variables(&debugger->scope[debugger->depth], 1);
		}
		else if (COMMAND("a") || COMMAND("args"))
		{
			if (debugger->depth <= 0)
				ERROR_AND_RETRY("no current scopes");
			print_variables(&debugger->scope[debugger->depth], 1);
		}
		else if (COMMAND("w") || COMMAND("where"))
		{
			if (arg_count > 0)
				ERROR_AND_RETRY("where doesn't take arguments");
			print_location(debugger->current_location);
		}
		else if (COMMAND("l") || COMMAND("ll"))
		{
			do_command_ll(debugger);
		}
		else
		{
			printf("Unknown command: '%s'\n", command);
		}
	}
}

void debug_break(Debugger* debugger, const char* file, int line)
{
	debugger->current_location.file = file;
	debugger->current_location.line = line;
	debug_interactively(debugger);
}

#define APPLY_0(F)
#define APPLY_1(F, X) F X
#define APPLY_2(F, X, ...) (F X, APPLY_1(F, __VA_ARGS__))
#define APPLY_3(F, X, ...) (F X, APPLY_2(F, __VA_ARGS__))
#define APPLY_4(F, X, ...) (F X, APPLY_3(F, __VA_ARGS__))
#define APPLY_5(F, X, ...) (F X, APPLY_4(F, __VA_ARGS__))
#define APPLY_6(F, X, ...) (F X, APPLY_5(F, __VA_ARGS__))
#define APPLY_7(F, X, ...) (F X, APPLY_6(F, __VA_ARGS__))
#define APPLY_8(F, X, ...) (F X, APPLY_7(F, __VA_ARGS__))
#define APPLY_9(F, X, ...) (F X, APPLY_8(F, __VA_ARGS__))

void debug_location(Debugger* debugger, _Debug_Local_Scope local_scope, int line)
{
	debugger->depth = local_scope.depth;

	const char* file =  get_scope(debugger)->file;

	if (local_scope.depth < debugger->break_depth_or_below)
	{
		debug_break(debugger, file, line);
		return;
	}

	int bp_count = sizeof(debugger->breakpoints) / sizeof(debugger->breakpoints[0]);
	for (int i = 0; i < bp_count; ++i)
	{
		if (debugger->breakpoints[i].line != line)
			continue;

		if (strcmp(debugger->breakpoints[i].file, file) != 0)
			continue;

		debug_break(debugger, file, line);
	}
}

#define DEBUG_VARIABLE_IMPL(deref, var, is_arg) push_variable(get_scope(&g_debugger), #var, deref (var), _DEBUG_STR_FMT(var), _GET_USER_PTR(var), __LINE__, is_arg)
#define DEBUG_ARG_IMPL(deref, var) DEBUG_VARIABLE_IMPL(deref, var, 1)
#define DEBUG_VARIABLE(var) DEBUG_VARIABLE_IMPL(&, var, 0)
#define DEBUG_POINTER(var) DEBUG_VARIABLE_IMPL(, var, 0)

#define DEBUG_SCOPE_IMPL(content) _Debug_Local_Scope _debug_local_scope = push_scope(&g_debugger, content, __FILE__, __LINE__)
#define DEBUG_SCOPE DEBUG_SCOPE_IMPL(__FUNCTION__);
#define DEBUG_SCOPE_END pop_scope(&g_debugger, _debug_local_scope);

#define DEBUG_ARGS(N, ...) APPLY_##N(DEBUG_ARG_IMPL, ##__VA_ARGS__)

#define DEBUG_BREAK() debug_break(&g_debugger, get_scope(&g_debugger)->file, __LINE__)
#define DEBUG_LOCATION debug_location(&g_debugger, _debug_local_scope, __LINE__);

#define dbg_loc DEBUG_LOCATION
#define dbg_scp DEBUG_SCOPE
#define dbg_scp_end DEBUG_SCOPE_END
#define dbg_args DEBUG_ARGS
#define dbg_var DEBUG_VARIABLE
#define dbg_struct _DEBUG_STRUCT

Debugger g_debugger = {0};
