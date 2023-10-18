
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

#define GET_PRINTF_FUNC(x) _Generic((x), \
	State*: sprintfState, \
	Tetris: sprintfTetris, \
	TetrisPiece: sprintfTetrisPiece, \
	float: sprintfFloat, \
	double: sprintfDouble, \
	default: sprintfSingle \
)

#define GET_USER_PTR(x) _Generic((x), \
	State*: 0, \
	Tetris*: 0, \
	TetrisPiece*: 0, \
	default: GET_FMT_STRING(x))

#define GET_FMT_STRING(x) (_Generic((x), \
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

#define VAR(var) \
	do { \
		DebugSprintf f = GET_PRINTF_FUNC(owner->var); \
		void* user_ptr = GET_USER_PTR(owner->var); \
		buffer += sprintfIndent(indent + 1, buffer); \
		buffer += sprintf(buffer, #var " = "); \
		buffer += f(indent + 1, buffer, user_ptr, &owner->var); \
		buffer += sprintf(buffer, ",\n"); \
	} while(0)

#define STRUCT(Struct, VARS) \
int sprintf ## Struct(int indent, char *buffer, const void* user_ptr, const void* ptr) \
{ \
	const Struct* owner = (const Struct*)ptr; \
	const char* original = buffer; \
	buffer += sprintf(buffer, #Struct " [0x%llX] {\n", ptr); \
	VARS(VAR); \
	buffer += sprintfIndent(indent, buffer); \
	buffer += sprintf(buffer, "}"); \
	return buffer - original; \
}

typedef struct
{
	char buffer[64];
	const void* address;
	DebugSprintf sprintf_func;
	const void* user_ptr;
	int assignment_line;
} Variable;

typedef struct
{
	Variable variables[64];
	int count;
	const char* function;
	const char* file;
	int line;
	int depth;
} Scope;

typedef struct
{
	Scope* scope;
} LocalScope;

typedef struct
{
	const char* content;
	const char* file;
	int line;
} Location;

typedef struct
{
	int depth;
	Scope scope[64];

	Location breakpoints[64];

	Location current_location;

	int ignore_counter;
	int break_depth_or_below;
} Debugger;


LocalScope push_scope(Debugger* debugger, const char* function, const char* file, int line)
{
	int depth = ++debugger->depth;

	LocalScope local_scope;
	local_scope.scope = &debugger->scope[depth];
	memset(local_scope.scope, 0, sizeof(*local_scope.scope));

	Scope* scope = local_scope.scope;
	scope->function = function;
	scope->file = file;
	scope->line = line;
	scope->depth = depth;
	return local_scope;
}

void pop_scope(Debugger* debugger, LocalScope local_scope)
{
	debugger->depth = local_scope.scope->depth;
	FATAL(debugger->depth > 0, "%d depth", debugger->depth);
	memset(local_scope.scope, 0, sizeof(*local_scope.scope));
}

Scope* get_scope(Debugger* debugger)
{
	FATAL(debugger->depth > 0, "%d depth", debugger->depth);
	return &debugger->scope[debugger->depth];
}

void push_variable(Scope* scope, const char* name, const void* address, DebugSprintf sprintf_func, const void* user_ptr, int line)
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
}

int sprintf_value_impl(char* buffer, DebugSprintf sprintf_func, const void* user_ptr, const void* address)
{
	char* output = buffer;
	output += sprintf_func(0, output, user_ptr, address);
	return output - buffer;
}

void print_variable(const char* name, DebugSprintf sprintf_func, const void* user_ptr, const void* address)
{
	char buffer[512] = {0};
	sprintf_value_impl(buffer, sprintf_func, user_ptr, address);
	printf("%s = %s\n", name, buffer);
}

#define PRINT_VARIABLE(var) do { print_variable(#var, GET_PRINTF_FUNC(var), GET_USER_PTR(var), &(var)); } while(0)
#define PRINT_VARIABLE_POINTER(var) do { print_variable(#var, GET_PRINTF_FUNC(var), GET_USER_PTR(var), (var)); } while(0)

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

void print_scope(Scope* scope)
{
	printf("[%d] %s:%d scope: %s {\n", scope->depth, scope->file, scope->line, scope->function);

	char buffer[1024] = {0};
	for (int i = 0; i < scope->count; ++i)
	{
		char* output = buffer;
		Variable v = scope->variables[i];
		output += sprintf_value_impl(output, v.sprintf_func, v.user_ptr, v.address);
		printf("%s = %s\n", v.buffer, buffer);
	}
	printf("}\n");
}

void print_stack(Debugger* debugger)
{
	for (int i = 1; i <= debugger->depth; ++i)
	{
		print_scope(&debugger->scope[i]);
	}
}

void print_location(Location location)
{
	if (location.file)
		printf("%s:%d '%s'\n", location.file, location.line, location.content);
	else
		printf("No location stored\n");
}

void debug_interactively(Debugger* debugger)
{
	print_location(debugger->current_location);
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
			printf("%s\n", input);
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

		printf("RAW_INPUT:'%s', COMMAND:'%s', [%d] 1:'%s'(%d), 2:'%s'(%d), 3:'%s'(%d)\n", input, command, arg_count, arg1, arg1_int, arg2, arg2_int, arg3, arg3_int);

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
			return;
		}
		else if (COMMAND("n") || COMMAND("next"))
		{
			debugger->break_depth_or_below = debugger->depth;
			return;
		}
		else if (COMMAND("u") || COMMAND("up"))
		{
			debugger->break_depth_or_below = debugger->depth - 1;
			return;
		}
		else if (COMMAND("t") || COMMAND("trace"))
		{
			if (debugger->depth <= 0)
				ERROR_AND_RETRY("no scopes");
			printf("%s:%d\n", debugger->current_location.file, debugger->current_location.line);
			print_stack(debugger);
		}
		else if (COMMAND("ls") || COMMAND("locals"))
		{
			if (debugger->depth <= 0)
				ERROR_AND_RETRY("no current scopes");
			print_scope(&debugger->scope[debugger->depth]);
		}
		else if (COMMAND("w") || COMMAND("where"))
		{
			if (arg_count > 0)
				ERROR_AND_RETRY("where doesn't take arguments");
			print_location(debugger->current_location);
		}
		else
		{
			printf("Unknown command: '%s'\n", command);
		}
	}
}

void debug_break(Debugger* debugger, const char* content, const char* file, int line)
{
	debugger->current_location.content = content;
	debugger->current_location.file = file;
	debugger->current_location.line = line;
	debug_interactively(debugger);
}

void debug_location(Debugger* debugger, const char* content, const char* file, int line)
{
	if (get_scope(debugger)->depth < debugger->break_depth_or_below)
	{
		debug_break(debugger, content, file, line);
		return;
	}

	int bp_count = sizeof(debugger->breakpoints) / sizeof(debugger->breakpoints[0]);
	for (int i = 0; i < bp_count; ++i)
	{
		if (debugger->breakpoints[i].line != line)
			continue;

		if (strcmp(debugger->breakpoints[i].file, file) != 0)
			continue;

		debug_break(debugger, content, file, line);
	}
}

#define INSTRUMENT_VARIABLE(var) do { push_variable(get_scope(&g_debugger), #var, &(var), GET_PRINTF_FUNC(var), GET_USER_PTR(var), __LINE__); } while(0)
#define INSTRUMENT_POINTER(var) do { push_variable(get_scope(&g_debugger), #var, (var), GET_PRINTF_FUNC(var), GET_USER_PTR(var), __LINE__); } while(0)
#define SCOPE_BEGIN_IMPL(func, id) LocalScope local_scope_##id = push_scope(&g_debugger, func, __FILE__, __LINE__)
#define SCOPE_END_IMPL(id) pop_scope(&g_debugger, local_scope_##id)

#define SCOPE_BEGIN() SCOPE_BEGIN_IMPL(__FUNCTION__, 0)
#define SCOPE_END() SCOPE_END_IMPL(0)

#define SCOPE_CALL_RET(x) ({ SCOPE_BEGIN_IMPL(#x, __LINE__); auto _ret = x; DEBUG_ROW(#x); SCOPE_END_IMPL(__LINE__); _ret; })
#define SCOPE_CALL(x) do { SCOPE_BEGIN_IMPL(#x, __LINE__); x; DEBUG_ROW(#x); SCOPE_END_IMPL(__LINE__); } while(0)

#define DEBUG_BREAK() debug_break(&g_debugger, __FUNCTION__, __FILE__, __LINE__)
#define DEBUG_LOCATION() debug_location(&g_debugger, __FUNCTION__, __FILE__, __LINE__)
#define DEBUG_ROW(x) debug_location(&g_debugger, #x, __FILE__, __LINE__)

Debugger g_debugger = {0};
