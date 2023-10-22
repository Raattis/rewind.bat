
enum { DEBUGGER_TRACE = 0&&TRACE };
#define dbg_printf(...) do { if (DEBUGGER_TRACE) printf(__VA_ARGS__); } while(0)

#define user_printf printf

typedef int(*DebugSnprintf)(int indent, char*, unsigned long long, const void*, const void*);

int snprintf_indent(int indent, char* buffer, unsigned long long size)
{
	return snprintf(buffer, size, "%s", "                      " + 20 - indent * 2);
}

int snprintf_single(int indent, char* buffer, unsigned long long size, const void* fmt, const void* ptr)
{
	dbg_printf("\n%s(indent:%d, buffer:%p, fmt:%s, ptr:%p), ", __FUNCTION__, indent, buffer, fmt?fmt:"null", ptr);
	return snprintf(buffer, size, (const char*)fmt, *(long long*)ptr);
}

int snprintf_float(int indent, char* buffer, unsigned long long size, const void* fmt, const void* ptr)
{
	dbg_printf("\n%s(indent:%d, buffer:%p, fmt:%s, dbl:%f), ", __FUNCTION__, indent, buffer, fmt?fmt:"null", *(float*)ptr);
	return snprintf(buffer, size, (const char*)fmt, *(float*)ptr);
}

int snprintf_double(int indent, char* buffer, unsigned long long size, const void* fmt, const void* ptr)
{
	dbg_printf("\n%s(indent:%d, buffer:%p, fmt:%s, dbl:%f), ", __FUNCTION__, indent, buffer, fmt?fmt:"null", *(double*)ptr);
	return snprintf(buffer, size, (const char*)fmt, *(double*)ptr);
}

#define COMMA ,
#define GENERIC_SNPRINTF(Type, Ptr, Name, Fmt) Type: snprintf_ ## Name
#define GENERIC_FMT_ARGUMENT(Type, Ptr, Name, Fmt) Type: Fmt
#define GENERIC_TYPE_NAME(Type, Ptr, Name, Fmt) Type: #Name
#define GENERIC_TYPE_STR(Type, Ptr, Name, Fmt) Type: #Type
#define GENERIC_REF(Type, Ptr, Name, Fmt, x) Type: &(x)
#define GENERIC_DEREF(Type, Ptr, Name, Fmt, x) Type: (x)

#define _DEBUG_PRINTF_FUNC(x) _Generic((x), \
	DEBUG_USER_TYPES(GENERIC_SNPRINTF, COMMA), \
	float: snprintf_float, \
	double: snprintf_double, \
	default: snprintf_single \
)

#define DEBUG_DEFAULT_TYPES(X, SEP, ...) \
	X(char, char*, char, "%c", ##__VA_ARGS__) SEP \
	X(unsigned char, unsigned char*, u8, "%hhu", ##__VA_ARGS__) SEP \
	X(short, short*, i16, "%hd", ##__VA_ARGS__) SEP \
	X(unsigned short, unsigned short*, u16, "%hu", ##__VA_ARGS__) SEP \
	X(unsigned, unsigned*, u32, "%u", ##__VA_ARGS__) SEP \
	X(int, int*, i32, "%d", ##__VA_ARGS__) SEP \
	X(float, float*, f32, "%f", ##__VA_ARGS__) SEP \
	X(double, double*, f64, "%f", ##__VA_ARGS__) SEP \
	X(char*, char**, char_ptr, "'%s'", ##__VA_ARGS__) SEP \
	X(const char*, const char**, const_char_ptr, "'%s'", ##__VA_ARGS__) SEP \
	X(void*, void**, void_ptr, "%p", ##__VA_ARGS__) SEP \
	X(const void*, const void**, const_void_ptr, "%p", ##__VA_ARGS__)

#define _DEBUG_FMT_ARGUMENT_PTR(x) _Generic((x), \
	DEBUG_USER_TYPES(GENERIC_FMT_ARGUMENT, COMMA), \
	DEBUG_DEFAULT_TYPES(GENERIC_FMT_ARGUMENT, COMMA), \
	default: _DEBUG_TYPE_NAME(x))

#define _DEBUG_TYPE_NAME(x) _Generic((x), \
	DEBUG_USER_TYPES(GENERIC_TYPE_NAME, COMMA), \
	DEBUG_DEFAULT_TYPES(GENERIC_TYPE_NAME, COMMA), \
	default: "'" #x "' has unknown type")

#define _DEBUG_TYPE_STR(x) _Generic((x), \
	DEBUG_USER_TYPES(GENERIC_TYPE_STR, COMMA), \
	DEBUG_DEFAULT_TYPES(GENERIC_TYPE_STR, COMMA), \
	default: "'" #x "' has unknown type")

#define _DEBUG_TYPE_REF(x) _Generic((x), \
	DEBUG_USER_TYPES(GENERIC_REF, COMMA, x), \
	DEBUG_DEFAULT_TYPES(GENERIC_REF, COMMA, x), \
	default: (x) )

#define _DEBUG_TYPE_DEREF(x) _Generic((x), \
	DEBUG_USER_TYPES(GENERIC_DEREF, COMMA, x), \
	DEBUG_DEFAULT_TYPES(GENERIC_DEREF, COMMA, x), \
	default: *(x) )


#define _DEBUG_VAR(var) \
	do { \
		DebugSnprintf f = _DEBUG_PRINTF_FUNC(owner->var); \
		void* user_ptr = _DEBUG_FMT_ARGUMENT_PTR(owner->var); \
		buffer += snprintf_indent(indent + 1, buffer, end - buffer); \
		buffer += snprintf(buffer, end - buffer, #var " = "); \
		buffer += f(indent + 1, buffer, end - buffer, user_ptr, &owner->var); \
		buffer += snprintf(buffer, end - buffer, ",\n"); \
	} while(0)

#define _DEBUG_STRUCT(Name, VARS) \
int snprintf_ ## Name(int indent, char* buffer, unsigned long long size, const void* user_ptr, const void* ptr) \
{ \
	dbg_printf("%s(indent:%d, buffer:%p, user_ptr:%s, ptr:%p), ", __FUNCTION__, indent, buffer, user_ptr?user_ptr:"null", ptr); \
	const Name* owner = (const Name*)ptr; \
	const char* original = buffer; \
	const char* end = buffer + size; \
	buffer += snprintf(buffer, end - buffer, "[0x%llX] {\n", ptr); \
	VARS(_DEBUG_VAR); \
	buffer += snprintf_indent(indent, buffer, end - buffer); \
	buffer += snprintf(buffer, end - buffer, "}"); \
	return buffer - original; \
}

typedef struct _t_Type
{
	const char* readable_name;
	const char* as_string;
	DebugSnprintf snprintf_func;
	const void* user_ptr;
} Type;

typedef struct _t_Variable
{
	char buffer[64];
	const void* address;
	int assignment_line;
	int is_arg;

	Type type;
} Variable;

typedef struct _t_Location
{
	const char* name;
	const char* file;
	int line;
} Location;

typedef struct _t_Scope
{
	Location start_location;
	Variable variables[64];
	int count;
	int depth;
	int current_line;
} Scope;

typedef struct _t_Debug_Local_Scope
{
	int depth;
} _Debug_Local_Scope;

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
	scope->start_location.name = function;
	scope->start_location.file = file;
	scope->start_location.line = line;
	scope->current_line = line;
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

void push_variable(Scope* scope, const char* name, const void* address, int line, int is_arg, DebugSnprintf snprintf_func, const void* user_ptr, const char* type, const char* readable_name)
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
	variable->assignment_line = line;
	variable->is_arg = is_arg;

	variable->type.snprintf_func = snprintf_func;
	variable->type.user_ptr = user_ptr;
	variable->type.readable_name = readable_name;
	variable->type.as_string = type;
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
			output += snprintf(output, end - output, "%5d  ", lineno);
		else
			output += snprintf(output, end - output, "   --> ");
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
	user_printf("%s", g_temp_buffer);
}

int snprintf_value_impl(char* buffer, unsigned long long size, DebugSnprintf snprintf_func, const void* user_ptr, const void* address)
{
	dbg_printf("%s(buffer:%p,func:%p,user_ptr:%p,address:%p), ", __FUNCTION__, buffer, snprintf_func, user_ptr, address);
	char* output = buffer;
	dbg_printf("1, ");
	output += snprintf_func(0, output, size, user_ptr, address);
	dbg_printf("2, ");
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

void print_variable(const char* name, DebugSnprintf snprintf_func, const void* user_ptr, const void* address)
{
	char buffer[1024] = {0};
	snprintf_value_impl(buffer, sizeof(buffer), snprintf_func, user_ptr, address);
	user_printf("%s = %s\n", name, buffer);
}

#define PRINT_VARIABLE(var) do { print_variable(#var, _DEBUG_PRINTF_FUNC(var), _DEBUG_FMT_ARGUMENT_PTR(var), &(var)); } while(0)
#define PRINT_VARIABLE_POINTER(var) do { print_variable(#var, _DEBUG_PRINTF_FUNC(var), _DEBUG_FMT_ARGUMENT_PTR(var), (var)); } while(0)

void print_variable_from_scope(Scope* scope, const char* name)
{
	char buffer[1024] = {0};
	for (int i = 0; i < scope->count; ++i)
	{
		if (strcmp(scope->variables[i].buffer, name) != 0)
			continue;
		Variable v = scope->variables[i];
		snprintf_value_impl(buffer, sizeof(buffer), v.type.snprintf_func, v.type.user_ptr, v.address);
		user_printf("%s = %s\n", name, buffer);
		return;
	}
	user_printf("'%s' variable doesn't exist in scope", name);
}

void print_variables(Scope* scope, int args)
{
	dbg_printf("%s(scope:%p, args:%d), ", __FUNCTION__, scope, args);

	char buffer[1024] = {0};
	for (int i = 0; i < scope->count; ++i)
	{
		Variable v = scope->variables[i];
		dbg_printf("%d:%s,%sarg,'%s' ", i, v.buffer, v.is_arg ? "" : "not ", v.type.user_ptr);
		if (v.is_arg != args)
			continue;
		snprintf_value_impl(buffer, sizeof(buffer), v.type.snprintf_func, v.type.user_ptr, v.address);
		user_printf("%s = %s\n", v.buffer, buffer);
	}
}

Location get_current_location(Scope* scope)
{
	Location result = scope->start_location;
	result.line = scope->current_line;
	return result;
}

void fit_long_string_in_args(char* buffer)
{
	int deleted = 0;
	int max_length = 32;
	for (int i = 0; i - deleted < max_length && i < max_length * 4; ++i)
	{
		char c = buffer[i];
		switch (c)
		{
			case '\r':
			case '\n':
			case '\t':
			case ' ':
				++deleted;
				continue;
			default:
				break;
		}

		buffer[i - deleted] = c;
		if (c == 0)
			return;
	}
	strcpy(buffer + max_length - 3, "...");
}

void print_args(Scope* scope)
{
	char buffer[1024];
	for (int i = 0; i < scope->count; ++i)
	{
		if (i > 0)
			user_printf(", ");
		Variable v = scope->variables[i];
		if (!v.is_arg)
			continue;

		snprintf_value_impl(buffer, sizeof(buffer), v.type.snprintf_func, v.type.user_ptr, v.address);
		fit_long_string_in_args(buffer);
		user_printf("%s:=%s", scope->variables[i].buffer, buffer);
	}
}

void print_stack(Debugger* debugger, int current_depth, int show_variables)
{
	for (int i = 1; i <= debugger->depth; ++i)
	{
		dbg_printf("print_stack for %d, ", i);
		Scope* scope = &debugger->scope[i];
		Location location = get_current_location(scope);
		if (current_depth == i)
			user_printf("->[%d] %s:%d %s", scope->depth, location.file, location.line, location.name);
		else
			user_printf("  [%d] %s:%d %s", scope->depth, location.file, location.line, location.name);
		user_printf("(");
		print_args(scope);
		user_printf(")");
		if (show_variables)
		{
			user_printf(" {\n");
			print_variables(scope, 1);
			print_variables(scope, 0);
			user_printf("  }");
		}
		user_printf("\n");
	}
}

void print_location(Location location)
{
	if (location.file)
		user_printf("%s:%d '%s'\n", location.file, location.line, get_line(location.file, location.line));
	else
		user_printf("<null>:%d 'n/a'\n", location.line);
}

void do_command_ll(Location location)
{
	print_lines(location.file, location.line - 10, 20, location.line);
}

void debug_interactively(Debugger* debugger)
{
	Location location = debugger->current_location;
	int depth = debugger->depth;

	print_location(location);
	do_command_ll(location);

	fflush(stdin);
	char input[1024] = {0};
	int input_length = 0;

	while (1)
	{
		memset(input, 0, sizeof(input));
		input_length = 0;

		user_printf("---------------------------------------------\n");
		user_printf("(debugger) ");
		while (input_length < sizeof(input) && !strchr(input, '\n'))
		{
			if (fgets(input + input_length, sizeof(input) - input_length, stdin))
				input_length += strlen(input + input_length);
			Sleep(10);
			input[input_length] = 0;
			dbg_printf("INPUT: %s\n", input);
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

		dbg_printf("RAW_INPUT:'%s', COMMAND:'%s', [%d] 1:'%s'(%d), 2:'%s'(%d), 3:'%s'(%d), ", input, command, arg_count, arg1, arg1_int, arg2, arg2_int, arg3, arg3_int);

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
				number = 1000000000;
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

					user_printf("Breakpoint %d at '%s:%d'\n", i, debugger->breakpoints[i].file, debugger->breakpoints[i].line);
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
			user_printf("Breakpoint %d set to '%s:%d' '%s'\n", i, debugger->breakpoints[i].file, debugger->breakpoints[i].line);
			continue;
		}
		else if (COMMAND("s") || COMMAND("step"))
		{
			debugger->break_depth_or_below = 999;
			return;
		}
		else if (COMMAND("n") || COMMAND("next"))
		{
			debugger->break_depth_or_below = debugger->depth;
			return;
		}
		else if (COMMAND("d") || COMMAND("down"))
		{
			if (depth >= debugger->depth)
				ERROR_AND_RETRY("can't go lower. valid depth range: [1..%d]", depth, debugger->depth);
			depth += 1;
			location = get_current_location(&debugger->scope[depth]);
			do_command_ll(location);
		}
		else if (COMMAND("u") || COMMAND("up"))
		{
			if (depth <= 1)
				ERROR_AND_RETRY("can't go lower. valid depth range: [1..%d]", depth, debugger->depth);
			depth -= 1;
			location = get_current_location(&debugger->scope[depth]);
			do_command_ll(location);
		}
		else if (COMMAND("r") || COMMAND("return"))
		{
			debugger->break_depth_or_below = debugger->depth - 1;
			user_printf("Attempting to return from '%s'.", debugger->scope[debugger->depth].start_location.name);
			return;
		}
		else if (COMMAND("t") || COMMAND("trace"))
		{
			user_printf("%s:%d\n", debugger->current_location.file, debugger->current_location.line);
			print_stack(debugger, depth, 0);
		}
		else if (COMMAND("all") || COMMAND("deep"))
		{
			user_printf("%s:%d\n", debugger->current_location.file, debugger->current_location.line);
			print_stack(debugger, depth, 1);
		}
		else if (COMMAND("ls") || COMMAND("locals") || COMMAND("lsa"))
		{
			if (depth < 0 || depth > debugger->depth)
				ERROR_AND_RETRY("no scopes at depth %d, valid range: [0..%d)", depth, debugger->depth);
			print_variables(&debugger->scope[depth], 0);

			if (COMMAND("lsa"))
				print_variables(&debugger->scope[depth], 1);
		}
		else if (COMMAND("a") || COMMAND("args"))
		{
			if (debugger->depth <= 0)
				ERROR_AND_RETRY("no current scopes");
			print_variables(&debugger->scope[depth], 1);
		}
		else if (COMMAND("w") || COMMAND("where"))
		{
			if (arg_count > 0)
				ERROR_AND_RETRY("where doesn't take arguments");
			print_location(location);
		}
		else if (COMMAND("l") || COMMAND("ll"))
		{
			do_command_ll(location);
		}
		else if (COMMAND("help"))
		{
			const char* commands[] = {
				"unpause and continue execution\0c\0continue\0",
				"print, insert or modify breakpoint 'b[ [[breakpoint_number ]file ]line]'\0b\0break\0",
				"ignore a breakpoint once or near permanently 'ignore [breakpoint_number[ count]]'\0ignore\0",
				"move the smallest possible distance and stop again\0s\0step\0",
				"move to the next row in same or parent function\0n\0next\0",
				"move to the parent function\0r\0return\0",
				"navigate up/down the callstack\0u\0up\0d\0down\0",
				"print source lines around current location\0l\0ll\0",
				"print current location\0w\0where\0",
				"print local variables\0ls\0locals\0",
				"print function arguments\0a\0args\0",
				"print function arguments and local variables\0lsa\0",
				"print callstack\0t\0trace\0",
				"print callstack with all variables\0deep\0all\0",
				"exit application\0exit\0",
				"print this message or more details on any one of the commands 'help [command]'\0help\0"};

			int print_all = arg_count == 0;
			if (arg_count > 1)
				ERROR_AND_RETRY("help only expects 1 or two ")
			int count = sizeof(commands) / sizeof(commands[0]);

			int match = 0;
			for (int i = 0; i < count; ++i)
			{
				const char* current = commands[i];
				const char* desc = current;
				int alias_count = 0;
				const char* aliases[5] = {0};
				while (*++current != 0); // Skip desc
				while (*++current != 0) // Skip terminator
				{
					aliases[alias_count++] = current;
					while (*++current != 0); // Skip until next alias
				}

				if (arg_count == 1)
				{
					for (int j = 0; j < alias_count; ++j)
					{
						if (strcmp(arg1, aliases[j]) != 0)
							continue;
						match = 1;
						break;
					}
				}

				if (match || arg_count == 0)
				{
					for (int j = 0; j < alias_count; ++j)
					{
						if (j > 0)
							user_printf(", ", aliases[j]);
						user_printf("%s", aliases[j]);
					}
					user_printf("\n\t%s\n", desc);
					if (match)
						break;
				}
			}
			if (!match && arg_count == 1)
				ERROR_AND_RETRY("No such command '%s'", arg1);
		}
		else
		{
			user_printf("Unknown command: '%s'\n", command);
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
#define APPLY_1(F, X) F(X)
#define APPLY_2(F, X, ...) (F(X), APPLY_1(F, __VA_ARGS__))
#define APPLY_3(F, X, ...) (F(X), APPLY_2(F, __VA_ARGS__))
#define APPLY_4(F, X, ...) (F(X), APPLY_3(F, __VA_ARGS__))
#define APPLY_5(F, X, ...) (F(X), APPLY_4(F, __VA_ARGS__))
#define APPLY_6(F, X, ...) (F(X), APPLY_5(F, __VA_ARGS__))
#define APPLY_7(F, X, ...) (F(X), APPLY_6(F, __VA_ARGS__))
#define APPLY_8(F, X, ...) (F(X), APPLY_7(F, __VA_ARGS__))
#define APPLY_9(F, X, ...) (F(X), APPLY_8(F, __VA_ARGS__))

void debug_location(Debugger* debugger, _Debug_Local_Scope local_scope, int line)
{
	debugger->depth = local_scope.depth;
	debugger->scope[local_scope.depth].current_line = line;

	const char* file = debugger->scope[local_scope.depth].start_location.file;
	if (local_scope.depth <= debugger->break_depth_or_below)
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

int next_line(const char* input)
{
	int result = 0;
	while(!*input && *input++ != '\n')
		++result;
	return  result;
}
const char* stripl(const char* input)
{
	for (char c = *input; c; c = *++input)
	{
		switch(c)
		{
		case '\t':
		case ' ':
		case '\r':
		case '\n': continue;
		default: return input;
		}
	}
	return  input;
}

int copy(char* buffer, const char* buffer_end, const char* input, const char* input_end)
{
	int result = 0;
	for (;!input_end || input < input_end;)
	{
		char c = *input;
		FATAL(buffer < buffer_end, "Buffer overrun");
		FATAL(c != '\r', "No carriage returns allowed!");

		*buffer = c;
		if (!c)
			break;
		 ++buffer; ++input; ++result;
	}
	return result;
}

int is_alpha(char c)
{
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

int is_token_char(char c)
{
	return is_alpha(c) || ('0' <= c && c <= '9') || c == '_';
}

const char* keyword(const char* input, const char* tok)
{
	const char* at = strstr(input, tok);
	if (!at)
		return 0;
	const char* before = input < at ? at - 1 : "\0";
	const char* after = at + strlen(tok);
	if (is_alpha(*before) || is_alpha(*after))
		return 0;
	return at;
}

const char* token_before(const char** token_end_ptr, const char* line_start)
{
	const char* token_end = *token_end_ptr;
	const char* token_start = token_end;
	while (token_start > line_start && !is_token_char(*token_start))
		--token_start;
	if (token_start == line_start)
		return 0;
	while (token_start > line_start && is_token_char(*(token_start - 1)))
		--token_start;
	return token_start;
}

void auto_instrument(char* buffer, size_t buffer_size, const char* filename)
{
	char line[16 * 1024] = {0};
	char function_arguments[1024] = {0};
	const char* buffer_start = buffer + buffer_size;
	const char* buffer_end = buffer + buffer_size;
	FILE* file = fopen(filename, "r");
	if (!file)
	{
		fprintf(stderr, "'%s' doesn't exist", filename);
		FATAL(0, "'%s' doesn't exist", filename);
		return;
	}
	printf("Hello: '%s'\n", filename);

	int func_started = 0;
	int inline_scope_started = 0;
	while (fgets(line, sizeof(line), file))
	{
		const char* buffer_line_start = buffer;
		const char* a = line;
more:
		a = stripl(a);
		char c = *a;
		#define KEYWORD(w) const char* w ## _at = keyword(a, #w)
		const char* assign_at = strstr(a, "=") != strstr(a, "==") ? strstr(a, "=") : 0;
		KEYWORD(return);
		KEYWORD(if);
		KEYWORD(for);
		KEYWORD(while);
		KEYWORD(do);
		KEYWORD(switch);
		KEYWORD(case);
		//printf("Start: %c, a:'%s', line:'%s'\n", c, a, line);
		if (c =='{')
		{
			if (func_started)
			{
				func_started = 0;
				buffer += copy(buffer, buffer_end, "{ dbg_scope", 0); ++a;
				buffer += copy(buffer, buffer_end, function_arguments, 0);
				goto more;
			}
			else
			{
				buffer += copy(buffer, buffer_end, line, 0);
			}
		}
		else if(c == '}')
		{
			func_started = 0;
			buffer += copy(buffer, buffer_end, "dbg_scope", 0);
			buffer += copy(buffer, buffer_end, function_arguments, 0);
			buffer += copy(buffer, buffer_end, a, 0);
		}
		else if(c == '/')
		{
			buffer += copy(buffer, buffer_end, a, 0);
		}
		else if(c == '#')
		{
			buffer += copy(buffer, buffer_end, line, 0);
		}
		else if(a == return_at)
		{
			func_started = 0;
			buffer += copy(buffer, buffer_end, "dbg_loc dbg_scp_end ", 0);
			buffer += copy(buffer, buffer_end, a, 0);
		}
		else if(is_alpha(c))
		{
			if (assign_at)
			{
				const char* eol = strchr(a, '\n');
				buffer += copy(buffer, buffer_end, "dbg_loc ", 0);
				buffer += copy(buffer, buffer_end, a, eol);

				const char* token_end = assign_at;
				const char* tok = token_before(&token_end, a);
				if (!tok)
				{
					buffer += copy(buffer, buffer_end, "NO TOKEN FOUND", 0);
				}
				else
				{
					buffer += copy(buffer, buffer_end, " dbg_var(", 0);
					buffer += copy(buffer, buffer_end, tok, token_end);
					buffer += copy(buffer, buffer_end, ")", 0);
				}
				if (eol)
					buffer += copy(buffer, buffer_end, eol, 0);
			}
			else
			{
				buffer += copy(buffer, buffer_end, "dbg_loc ", 0);
				buffer += copy(buffer, buffer_end, a, 0);
			}
		}
		if (strlen(buffer_line_start) > 0)
			printf("out: '%c', %s", c, buffer_line_start);
		else if (strlen(a) > 0) 
			printf("EMPTY OUTPUT FOR: '%c', '%*s'\n", c, strlen(a) - 1, a);
		else
			printf("EMPTY INPUT\n");
	}
}

#define DEBUG_VARIABLE_IMPL(var, is_arg) push_variable(get_scope(&g_debugger), #var, _DEBUG_TYPE_REF(var), __LINE__, is_arg, _DEBUG_PRINTF_FUNC(_DEBUG_TYPE_DEREF(var)), _DEBUG_FMT_ARGUMENT_PTR(_DEBUG_TYPE_DEREF(var)), _DEBUG_TYPE_NAME(_DEBUG_TYPE_DEREF(var)), _DEBUG_TYPE_STR(_DEBUG_TYPE_DEREF(var)))
#define DEBUG_ARG_IMPL(var) DEBUG_VARIABLE_IMPL(var, 1)
#define DEBUG_VARIABLE(var) DEBUG_VARIABLE_IMPL(var, 0)

#define DEBUG_SCOPE_IMPL(content) _Debug_Local_Scope _debug_local_scope = push_scope(&g_debugger, content, __FILE__, __LINE__)
#define DEBUG_SCOPE DEBUG_SCOPE_IMPL(__FUNCTION__);
#define DEBUG_SCOPE_END pop_scope(&g_debugger, _debug_local_scope);

#define DEBUG_ARGS(N, ...) APPLY_##N(DEBUG_ARG_IMPL, ##__VA_ARGS__)

#define DEBUG_BREAK() debug_break(&g_debugger, get_scope(&g_debugger)->start_location.file, __LINE__)
#define DEBUG_LOCATION debug_location(&g_debugger, _debug_local_scope, __LINE__);

#define dbg_loc DEBUG_LOCATION
#define dbg_scp DEBUG_SCOPE
#define dbg_scp_end DEBUG_SCOPE_END
#define dbg_args DEBUG_ARGS
#define dbg_var DEBUG_VARIABLE
#define dbg_struct _DEBUG_STRUCT

Debugger g_debugger = {0};
