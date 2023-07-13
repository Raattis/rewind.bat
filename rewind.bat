: " In sh this syntax begins a multiline comment, whereas in batch it's a valid label that gets ignored.
@goto batch_bootstrap_builder "
if false; then */
#error Remember to insert "#if 0" into the compiler input pipe or skip the first 6 lines when compiling this file.
// Notepad++ run command: cmd /c 'cd /d $(CURRENT_DIRECTORY) &amp;&amp; $(FULL_CURRENT_PATH)'
#endif // GOTO_BOOTSTRAP_BUILDER

///////////////////////////////////////////////////////////////////////////////

#ifdef BOOTSTRAP_BUILDER
/*
fi # sh_bootstrap_builder

# Did you know that hashbang doesn't have to be on the first line of a file? Wild, right!
#!/usr/bin/env sh

compiler_executable=tcc
me=`basename "$0"`
no_ext=`echo "$me" | cut -d'.' -f1`
executable="${no_ext}.exe"

echo "static const char* b_source_filename = \"$me\";
#line 1 \"$me\"
#if GOTO_BOOTSTRAP_BUILDER /*" | cat - $me | $compiler_executable -x c -o $executable -DHELLO_WORLD -

compiler_exit_status=$?
if test $compiler_exit_status -ne 0; then echo "Failed to compile $me. Exit code: $compiler_exit_status"; exit $compiler_exit_status; fi

chmod +x $executable
./$executable

execution_exit_status=$?
if test $execution_exit_status -ne 0; then echo "$executable exited with status $execution_exit_status"; exit $execution_exit_status; fi

# -run -bench -nostdlib -lmsvcrt(?) -nostdinc -Iinclude
exit 0

///////////////////////////////////////////////////////////////////////////////

:batch_bootstrap_builder
@echo off
set compiler_executable=tcc\tcc.exe
set compiler_zip_name=tcc-0.9.27-win64-bin.zip
set download_tcc=n
if not exist %compiler_executable% if not exist %compiler_zip_name% set /P download_tcc="Download Tiny C Compiler? Please, try to avoid unnecessary redownloading. [y/n] "

if not exist %compiler_executable% (
	if not exist %compiler_zip_name% (
		if %download_tcc% == y (
			powershell -Command "Invoke-WebRequest http://download.savannah.gnu.org/releases/tinycc/%compiler_zip_name% -OutFile %compiler_zip_name%"
			if exist %compiler_zip_name% (
				echo Download complete!
			) else (
				echo Failed to download %compiler_zip_name%
			)
		)

		if not exist %compiler_zip_name% (
			echo Download Tiny C Compiler manually from http://download.savannah.gnu.org/releases/tinycc/ and unzip it here.
			pause
			exit 1
		)
	)

	if not exist tcc (
		echo Unzipping %compiler_zip_name%
		powershell Expand-Archive %compiler_zip_name% -DestinationPath .

		if exist %compiler_executable% (
			echo It seems the %compiler_zip_name% contained the %compiler_executable% directly. Thats cool.
		) else if not exist tcc (
			echo Unzipping %compiler_zip_name% did not yield the expected "tcc" folder.
			echo Move the contents of the archive here manually so that tcc.exe is in the same folder as %~n0%~x0.
			pause
			exit 1
		)
	)

	echo Tiny C Compiler Acquired!
)

(
	echo static const char* b_source_filename = "%~n0%~x0";
	echo #line 0 "%~n0%~x0"
	echo #if GOTO_BOOTSTRAP_BUILDER
	type %~n0%~x0
) | %compiler_executable% -run -nostdlib -nostdinc -lmsvcrt -lkernel32 -luser32 -lgdi32 -Itcc/include -Itcc/include/winapi -Itcc/libtcc -Ltcc/libtcc -llibtcc -DSHARED_PREFIX -DSOURCE -bench - 
@rem ) | %compiler_executable% -o %~n0.exe  -nostdlib -nostdinc -lmsvcrt -lkernel32 -luser32 -Itcc/include -Itcc/include/winapi -Itcc/libtcc -Ltcc/libtcc -llibtcc -DSHARED_PREFIX -DSOURCE -bench - 

if %errorlevel% == 0 (
	echo Run finished without errors!
) else (
	echo Run finished with return value %errorlevel%
)

exit errorlevel

*/
#endif // BOOTSTRAP_BUILDER

///////////////////////////////////////////////////////////////////////////////

#ifdef HELLO_WORLD

#include <stdio.h>

int main()
{
	printf("Hello, World!\n");
	return 0;
}

#endif // HELLO_WORLD

///////////////////////////////////////////////////////////////////////////////

#ifdef SHARED_PREFIX

enum { TRACE=1 };
#define trace_printf(...) do { if (TRACE) printf(__VA_ARGS__); } while(0)
#define FATAL(x, ...) do { if (x) break; fprintf(stderr, "%s:%d: (" SEGMENT_NAME "/%s) FATAL: ", __FILE__, __LINE__, __FUNCTION__); fprintf(stderr, __VA_ARGS__ ); fprintf(stderr, "\n(%s)\n", #x); int system(const char*); system("pause"); void exit(int); exit(1); } while(0)

typedef struct
{
	int stop;
	int was_recompiled;
	int redraw_requested;
	int replay;
	unsigned long long buffer_size;
	char* buffer;
} Communication;

#endif // SHARED_PREFIX

///////////////////////////////////////////////////////////////////////////////

#ifdef SOURCE
#define SEGMENT_NAME "SOURCE"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <sys/stat.h>
#include <time.h>

#include <libtcc.h>

#include "wm_message_to_string.h"

enum { TRACE_INPUT=0&&TRACE, TRACE_TICKS=0&&TRACE };

#define input_printf(...) do { if (TRACE_INPUT) printf(__VA_ARGS__); } while(0)
#define tick_printf(...) do { if (TRACE_TICKS) printf(__VA_ARGS__); } while(0)


typedef struct stat file_timestamp;

void get_file_timestamp(file_timestamp* stamp, const char* file)
{
	stat(file, stamp);
}

int cmp_and_swap_timestamps(file_timestamp* stamp1, const char* file)
{
	file_timestamp stamp2;
	get_file_timestamp(&stamp2, file);

	if (stamp1->st_mtime == stamp2.st_mtime)
		return 0;

	if (stamp1->st_mtime < stamp2.st_mtime)
	{
		*stamp1 = stamp2;
		return -1;
	}

	return 1;
}

size_t scan_includes(const char* source_file, char** files_to_watch, size_t files_to_watch_count, size_t written)
{
	trace_printf("scan_includes('%s', %lld)\n", source_file, written);

	char buffer[1024] = {0};

	size_t first_written = written;
	FILE* infile = fopen(source_file, "r");
	while (fgets(buffer, sizeof(buffer), infile))
	{
		if (strstr(buffer, "#include \"") == 0)
			continue;

		char* begin = buffer + strlen("#include \"");
		char* end = begin;
		while(end < buffer + files_to_watch_count && *end != '"' && *end != 0)
			end += 1;

		int found = 0;
		for (size_t i = 0; i < written; i++)
		{
			if (strncmp(files_to_watch[i], begin, end - begin) == 0)
			{
				found = 1;
				break;
			}
		}

		if (found)
			continue;

		char* existing_file = files_to_watch[written];
		if (existing_file == 0 || strncmp(existing_file, begin, end - begin) != 0)
		{
			extern void free(void*);
			extern void* malloc(size_t);
			if (existing_file != 0)
				free(existing_file);

			size_t length = end - begin;
			existing_file = (char*)malloc(length);
			strncpy(existing_file, begin, length);
			existing_file[length] = 0;

			printf("Watching '%s' for changes.\n", existing_file);

			files_to_watch[written] = existing_file;
		}
		written += 1;
	}
	fclose(infile);

	for (size_t i = first_written, end = written; i < end; ++i)
	{
		written = scan_includes(files_to_watch[i], files_to_watch, files_to_watch_count, written);
	}

	return written;
}

size_t find_corresponding_source_files(const char** includes, size_t includes_count, char** sources, size_t sources_count, size_t written_sources)
{
	trace_printf("find_corresponding_source_files(%lld, %lld)\n", includes_count, written_sources);

	char buffer[1024] = {0};
	for (int i = 0; i < includes_count && written_sources < sources_count; ++i)
	{
		printf("checking '%s'\n", includes[i]);
		strcpy(buffer, includes[i]);
		char* ext = strstr(buffer, ".h");
		if (!ext)
			continue;

		ext[1] = 'c';

		char* existing_file = sources[written_sources];
		if (existing_file != 0 && strcmp(existing_file, buffer) == 0)
		{
			written_sources += 1;
			continue;
		}

		struct stat dummy;
		if (stat(buffer, &dummy) == 0)
		{
			extern void free(void*);
			extern void* malloc(size_t);
			if (existing_file != 0)
				free(existing_file);

			existing_file = (char*)malloc(strlen(buffer));
			strcpy(existing_file, buffer);

			sources[written_sources] = existing_file;
			written_sources += 1;
		}
	}

	return written_sources;
}

struct headers_and_sources {
	char* headers[256];
	char* sources[256];
	size_t sources_count;
	size_t headers_count;
};

void get_headers_and_sources(const char* main_source_file, struct headers_and_sources* headers_and_sources)
{
	size_t headers_buffer_size = sizeof(headers_and_sources->headers) / sizeof(headers_and_sources->headers[0]);
	size_t sources_buffer_size = sizeof(headers_and_sources->sources) / sizeof(headers_and_sources->sources[0]);
	void* malloc(size_t);
	char* main_source_file_copy = malloc(strlen(main_source_file) + 1);
	strcpy(main_source_file_copy, main_source_file);
	headers_and_sources->sources[0] = main_source_file_copy;
	headers_and_sources->sources_count = 1;
	headers_and_sources->headers_count = 0;
	for (size_t i = 0; i < headers_and_sources->sources_count; ++i)
	{
		const char* source = headers_and_sources->sources[i];
		trace_printf("Scanning '%s'\n", source);

		size_t prev_headers_count = headers_and_sources->headers_count;

		headers_and_sources->headers_count
			= scan_includes(
				source,
				headers_and_sources->headers,
				headers_buffer_size,
				headers_and_sources->headers_count);

		headers_and_sources->sources_count
			= find_corresponding_source_files(
				headers_and_sources->headers + prev_headers_count,
				headers_and_sources->headers_count - prev_headers_count,
				headers_and_sources->sources,
				sources_buffer_size,
				headers_and_sources->sources_count);
	}
}

int get_any_newer_file_timestamp(file_timestamp* stamp, struct headers_and_sources* headers_and_sources)
{
	int found = 0;
	for (size_t i = 0; i < headers_and_sources->sources_count; ++i)
	{
		if (cmp_and_swap_timestamps(stamp, headers_and_sources->sources[i]) < 0)
		{
			printf("Timestamp of '%s' was newer than previous timestamp\n", headers_and_sources->sources[i]);
			found = 1;
		}
	}

	for (size_t i = 0; i < headers_and_sources->headers_count; ++i)
	{
		if (cmp_and_swap_timestamps(stamp, headers_and_sources->headers[i]) < 0)
		{
			printf("Timestamp of '%s' was newer than previous timestamp\n", headers_and_sources->sources[i]);
			found = 1;
		}
	}

	return found;
}

typedef struct
{
	PAINTSTRUCT ps;
	HDC screen_device_context;
	HDC hdc;
	HBITMAP bitmap;
	HGDIOBJ previous_gdi_object;
	int screen_width;
	int screen_height;
} Drawer;

void open_drawer(HWND hWnd, Drawer* drawer)
{
	RECT screen_rect;
	GetClientRect(hWnd, &screen_rect);
	drawer->screen_width = screen_rect.right;
	drawer->screen_height = screen_rect.bottom;

	drawer->screen_device_context = BeginPaint(hWnd, &drawer->ps);
	drawer->hdc = CreateCompatibleDC(drawer->screen_device_context);
	drawer->bitmap = CreateCompatibleBitmap(drawer->screen_device_context, drawer->screen_width, drawer->screen_height);
	drawer->previous_gdi_object = SelectObject(drawer->hdc, drawer->bitmap);
}

void close_drawer(HWND hWnd, Drawer* drawer)
{
	BitBlt(drawer->screen_device_context, 0, 0, drawer->screen_width, drawer->screen_height, drawer->hdc, 0, 0, SRCCOPY);

	SelectObject(drawer->hdc, drawer->previous_gdi_object);
	DeleteObject(drawer->bitmap);
	DeleteDC(drawer->hdc);
	ReleaseDC(hWnd, drawer->screen_device_context);
	EndPaint(hWnd, &drawer->ps);
}

typedef int (*Key_Down_Func)(Communication* communication, int vk_key_code);
typedef void (*Update_Func)(Communication* communication);
typedef void (*Paint_Func)(Communication* communication, Drawer* drawer);

typedef LRESULT (*Window_Message_Handler_Func)(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
struct Tick_Data
{
	int state_offset;
	int source_offset;
	int key_down_func_offset;
	int update_func_offset;
	int paint_func_offset;

	int redraw_requested;
	int stop;
};

static const struct Tick_Data* g_tick_data = 0;
static void* g_state_buffer_start = 0;
static const void* g_program_buffer_start = 0;
static int g_window_close_requested = 0;
static int g_stopping = 0;
static void* g_user_buffer = 0;

LRESULT window_message_handler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	input_printf("\n\twindow_message_handler_impl(%s, %lld, %lld) ", wm_get_string(message), (long long)wParam, (long long)lParam);

	switch (message)
	{
		case WM_CREATE:
		{
			if (!SetWindowPos(hWnd, NULL, 1400, 70, 0, 0, SWP_NOSIZE | SWP_NOZORDER))
				FATAL(0, "Failed to position window. Error: ", GetLastError());

			//CREATESTRUCT *pCreate = (CREATESTRUCT*)lParam;
			//State* state = (State*)pCreate->lpCreateParams;
			//FATAL(state->initialized == StateInitializedMagicNumber, "State not initialized in message loop.");
			//SetLastError(0);
			//if (!SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)state) && GetLastError() != 0)
			//	printf("State set failed. Error: %d\n", GetLastError());

			return 0;
		}
		case WM_ERASEBKGND:
			//printf("WM_ERASEBKGND\n");
			break;
		case WM_SETREDRAW:
			printf("WM_SETREDRAW\n");
			break;
		case WM_PAINT:
		{
			//printf("WM_PAINT\n");
			
			Communication communication = {0};
			//communication.buffer = GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (g_tick_data)
				communication.buffer = g_state_buffer_start + g_tick_data->state_offset;

			Drawer drawer;
			open_drawer(hWnd, &drawer);
			
			Paint_Func paint = (Paint_Func)(g_program_buffer_start + g_tick_data->paint_func_offset);
			paint(&communication, &drawer);

			close_drawer(hWnd, &drawer);
			return 1;
		}
		case WM_KEYDOWN:
		{
			if (wParam == VK_ESCAPE)
			{
				printf("VK_ESCAPE\n");
				DestroyWindow(hWnd);
				g_window_close_requested = 1;
				return 0;
			}

			if (g_user_buffer)
			{
				Communication communication = {0};
				communication.buffer = g_user_buffer;

				Key_Down_Func key_down = (Key_Down_Func)(g_program_buffer_start + g_tick_data->key_down_func_offset);
				if (!key_down(&communication, wParam))
					return 0;
			}

			break;
		}
		case WM_QUIT:
			printf("WM_QUIT\n");
			break;
		case WM_DESTROY:
		{
			printf("WM_DESTROY\n");
			//PostQuitMessage(0);
			g_stopping = 1;
			// fallthrough
		}
		default:
			//printf("%x\n", message);
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND create_window(void)
{
	trace_printf("create_window\n");

	WNDCLASSEX wcex;
	memset(&wcex, 0, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = window_message_handler;
	wcex.hInstance = GetModuleHandle(NULL);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = "MyWindowClass";
	RegisterClassEx(&wcex);

	RECT rc = { 0, 0, 400, 300 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	// TODO: No hardcoded name. Get the name of the executable from commandline arguments.
	HWND hWnd = CreateWindow("MyWindowClass", GetCommandLine(), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
		NULL, NULL, GetModuleHandle(NULL), NULL);
	ShowWindow(hWnd, SW_SHOW);

	return hWnd;
}

void poll_messages(HWND hWnd, void* user_buffer)
{
	input_printf("poll_messages { ");

	if (!hWnd)
		return;

	int message_received = 0;

	g_user_buffer = user_buffer;

	MSG msg;
	while (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE))
	{
		message_received = 1;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	g_user_buffer = 0;

	if (message_received)
		input_printf("\n}\n");
	else
		input_printf("} ");

	return;
}

void rect(Drawer* drawer, int x, int y, int w, int h, int r, int g, int b)
{
	RECT rect = {x, y, x+w, y+h};
	HBRUSH brush = CreateSolidBrush(RGB(r,g,b));
	int success = FillRect(drawer->hdc, &rect, brush);
	if (success < 0)
		fprintf(stderr, "Failed to draw a rectangle. (%d, %d, %d, %d)", x,y, w,h);
	DeleteObject(brush);
}

void text(Drawer* drawer, int x, int y, char* str, int strLen)
{
	RECT rect = {x, y, x, y};
	DrawTextExA(drawer->hdc, str, strLen, &rect, DT_NOCLIP|DT_NOPREFIX|DT_SINGLELINE|DT_CENTER|DT_VCENTER, 0);
}

void get_screen_width_and_height(Drawer* drawer, int* screen_width, int* screen_height)
{
	FATAL(screen_height || screen_width, "Don't call this for no reason...");

	if (screen_height) *screen_height = drawer->screen_height;
	if (screen_width) *screen_width = drawer->screen_width;
}

void pixel(Drawer* drawer, int x, int y, int r, int g, int b)
{
	int success = SetPixel(drawer->hdc, x, y, RGB(r, g, b));
	//if (success < 0)
	//	fprintf(stderr, "Failed to set pixel to color. (%d, %d) -> (%d,%d,%d)", x,y, r,g,b);
}

void fill(Drawer* drawer, int r, int g, int b)
{
	int w = drawer->screen_width;
	int h = drawer->screen_height; //GetDeviceCaps(drawer->hdc, VERTRES);
	rect(drawer, 0,0, w,h, r,g,b);
}

void text_w(Drawer* drawer, int x, int y, wchar_t* str, int strLen)
{
	RECT rect = {x, y, x, y};
	DrawTextExW(drawer->hdc, str, strLen, &rect, DT_NOCLIP|DT_NOPREFIX|DT_SINGLELINE|DT_CENTER|DT_VCENTER, 0);
}

int is_window_open(HWND hWnd)
{
	return !!IsWindow(hWnd);
}

void wait_for_change(file_timestamp* newest_file_timestamp, struct headers_and_sources* headers_and_sources)
{
	// Timeout is around 30 seconds

	for (int i = 0; i < 300; i += 1)
	{
		Sleep(100);

		if (get_any_newer_file_timestamp(newest_file_timestamp, headers_and_sources))
			return;
	}
}

void run_recompilation_loop()
{
	void* malloc(size_t);
	struct headers_and_sources* headers_and_sources = (struct headers_and_sources*)malloc(sizeof(struct headers_and_sources));
	memset(headers_and_sources, 0, sizeof(*headers_and_sources));
	get_headers_and_sources(b_source_filename, headers_and_sources);

	file_timestamp newest_file_timestamp;
	get_file_timestamp(&newest_file_timestamp, b_source_filename);

	int user_buffer_size = 1000;
	void* user_buffer = malloc(user_buffer_size);
	int force_recompile = 1;

	TCCState *s = 0;

	int source_buffer_size = 40 * 1024 * 100; // Roughly space for 100 recompiles
	char* source_buffer = malloc(source_buffer_size);
	const char* source_buffer_start = source_buffer;
	const char* source_buffer_end = source_buffer + source_buffer_size;

	int program_buffer_size = 40 * 1024 * 100; // Roughly space for 100 recompiles
	void* program_buffer = malloc(program_buffer_size);
	const void* program_buffer_start = program_buffer;
	const void* program_buffer_end = program_buffer + program_buffer_size;
	g_program_buffer_start = program_buffer;

	int state_size = user_buffer_size;
	int state_max_count = 16 * 1024;
	void* state_buffer = malloc(state_size * state_max_count);
	const void* state_buffer_start = state_buffer;
	const void* state_buffer_end = state_buffer + state_size * state_max_count;
	g_state_buffer_start = state_buffer;

	int tick_data_buffer_count = state_max_count;
	struct Tick_Data* tick_data_buffer = (struct Tick_Data*)malloc(tick_data_buffer_count * sizeof(struct Tick_Data));
	const struct Tick_Data* tick_data_buffer_start = tick_data_buffer;
	const struct Tick_Data* tick_data_buffer_end = tick_data_buffer + tick_data_buffer_count;
	int tick_count = 0;

	HWND hWnd = create_window();

	for (;;)
	{
		tick_printf("\rTicks left: %lld, %lld, %lld, %lld ", tick_data_buffer - tick_data_buffer_start, state_buffer - state_buffer_start, program_buffer - program_buffer_start, source_buffer - source_buffer_start);

		if (tick_data_buffer >= tick_data_buffer_end)
		{
			printf("Out of ticks.");
			break;
		}

		struct Tick_Data new_tick;

		if (!g_tick_data)
			memset(&new_tick, 0, sizeof(struct Tick_Data));
		else
			memcpy(&new_tick, g_tick_data, sizeof(struct Tick_Data));

		tick_printf("tick(%d,%d,%d,%d), ", new_tick.state_offset, new_tick.update_func_offset, new_tick.paint_func_offset, new_tick.key_down_func_offset, new_tick.source_offset);

		int was_recompiled = 0;

		if (get_any_newer_file_timestamp(&newest_file_timestamp, headers_and_sources))
			force_recompile = 1;

		if (force_recompile)
		{
			clock_t c = clock();

			printf("Recompiling '%s'\n", b_source_filename);

			printf("Writing prefix\n");
			int prefix_length = sprintf(source_buffer,
				"\n" "#line 0 \"%s\""
				"\n" "#if GOTO_BOOTSTRAP_BUILDER"
				"\n"
				, b_source_filename);

			char* src_end = 0;
			printf("Copying source\n");
			{
				char* src = source_buffer + prefix_length;
				int size_left = source_buffer_end - src;

				FILE* src_file = fopen(b_source_filename, "r");
				if (!src_file)
				{
					fprintf(stderr, "Source file '%s' doesn't exist.", b_source_filename);
					wait_for_change(&newest_file_timestamp, headers_and_sources);
					continue;
				}
				size_t read_length = fread(src, sizeof(char), size_left, src_file);
				fclose(src_file);

				FATAL(read_length + 1 < size_left, "%s is too big (%d B < %d B) to runtime compile.", b_source_filename, read_length, size_left);

				src[read_length] = 0;
				src_end = src + read_length + 1;
			}

			new_tick.paint_func_offset = -1;
			new_tick.key_down_func_offset = -1;

			if (s)
				tcc_delete(s);

			s = tcc_new();
			FATAL(s, "Could not create tcc state\n");

			trace_printf("tcc_set_output_type  \n");
			tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

			trace_printf("tcc_set_options \n");
			//tcc_set_options(s, "-DRUNTIME_LOOP -DSHARED_PREFIX -nostdlib -nostdinc -vv");
			tcc_set_options(s, "-DRUNTIME_LOOP -DSHARED_PREFIX -nostdlib -nostdinc");

			trace_printf("tcc_add_include_path  \n");
			tcc_add_include_path(s, "tcc/include");
			tcc_add_include_path(s, "tcc/include/winapi");

			trace_printf("tcc_add_library_path  \n");
			tcc_add_library_path(s, "tcc/lib");

			trace_printf("tcc_add_library_err  \n");
			extern int tcc_add_library_err(TCCState *s, const char *f);
			//tcc_add_library_err(s, "gdi32");
			tcc_add_library_err(s, "msvcrt");
			tcc_add_library_err(s, "kernel32");
			tcc_add_library_err(s, "user32");

			trace_printf("tcc_add_symbol \n");
			//tcc_add_symbol(s, "get_window_message_handler", get_window_message_handler);
			tcc_add_symbol(s, "get_screen_width_and_height", get_screen_width_and_height);
			tcc_add_symbol(s, "rect", rect);
			tcc_add_symbol(s, "text", text);
			tcc_add_symbol(s, "fill", fill);
			tcc_add_symbol(s, "pixel", pixel);
			tcc_add_symbol(s, "text_w", text_w);

			trace_printf("Compiling\n");
			if (-1 == tcc_compile_string(s, source_buffer))
			{
				fprintf(stderr, "Failed to recompile '%s'.\n", b_source_filename);
				wait_for_change(&newest_file_timestamp, headers_and_sources);
				continue;
			}

			trace_printf("Checking resulting size...\n");
			int program_size = tcc_relocate(s, 0);
			if (program_size < 0)
			{
				fprintf(stderr, "Failed get size for relocate (=linking). Err: %d\n", program_size);
				wait_for_change(&newest_file_timestamp, headers_and_sources);
				continue;
			}
			else
			{
				if (program_size > program_buffer_end - program_buffer)
				{
					if (program_size > 1024 * 1024 * 1024)
					{
						fprintf(stderr, "Sanity check failed: Compilation result is %d bytes which is more than 1 GB.\n", program_size);
						wait_for_change(&newest_file_timestamp, headers_and_sources);
						continue;
					}

					FATAL(0, "Out of recompilation buffer space");
				}
			}

#ifdef _WIN32
			DWORD old;
			if (!VirtualProtect(program_buffer, program_size, PAGE_READWRITE, &old))
			{
				fprintf(stderr, "Couldn't unlock page protection. Old protection value: %d", old);
				wait_for_change(&newest_file_timestamp, headers_and_sources);
				continue;
			}
#else
			#error "TODO: Copy non-windows memory protection undoing from tccrun.c set_pages_executable()"
#endif

			printf("Linking...\n");
			int err = 0;
			if (0 > (err = tcc_relocate(s, program_buffer)))
			{
				fprintf(stderr, "Failed to relocate (=link). Err: %d\n", err);
				wait_for_change(&newest_file_timestamp, headers_and_sources);
				continue;
			}

			clock_t milliseconds = (clock() - c) * (1000ull / CLOCKS_PER_SEC);
			printf("Recompilation took %lld.%03lld seconds. Executable size in memory is %lld.%03lld KB\n", milliseconds/1000ull, milliseconds%1000ull, program_size / 1000ull, program_size % 1000ull);

			Update_Func update = tcc_get_symbol(s, "update");
			if (!update)
			{
				fprintf(stderr, "Failed to load the 'void update(Communication*)' symbol after recompilation.\n");
				wait_for_change(&newest_file_timestamp, headers_and_sources);
				continue;
			}

			Paint_Func paint  = tcc_get_symbol(s, "paint");
			if (!paint)
			{
				fprintf(stderr, "Failed to load the 'paint' symbol after recompilation.\n");
				wait_for_change(&newest_file_timestamp, headers_and_sources);
				continue;
			}

			Key_Down_Func key_down  = tcc_get_symbol(s, "key_down");
			if (!key_down)
			{
				fprintf(stderr, "Failed to load the 'key_down' symbol after recompilation.\n");
				wait_for_change(&newest_file_timestamp, headers_and_sources);
				continue;
			}

			new_tick.source_offset = src_end - source_buffer;
			source_buffer = src_end;

			new_tick.update_func_offset = ((char*)update) - program_buffer_start;
			new_tick.paint_func_offset = ((char*)paint) - program_buffer_start;
			new_tick.key_down_func_offset = ((char*)key_down) - program_buffer_start;

			program_buffer += program_size;
			get_headers_and_sources(b_source_filename, headers_and_sources);

			memcpy(tick_data_buffer, &new_tick, sizeof(struct Tick_Data));
			g_tick_data = tick_data_buffer;
			tick_data_buffer += 1;

			force_recompile = 0;
			was_recompiled = 1;

			continue;
		}

		FATAL(new_tick.update_func_offset > 0, "'update' not loaded.");
		Update_Func update = (Update_Func)(program_buffer_start + new_tick.update_func_offset);

		Communication communication = {0};
		communication.was_recompiled = was_recompiled;
		communication.buffer = user_buffer;
		communication.buffer_size = 1000;

		update(&communication);

		if (communication.redraw_requested)
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE); // Add "|RDW_ERASE" to see the flicker that is currently hidden by double buffering the draw target.

		// Paint and input
		poll_messages(hWnd, user_buffer);

		new_tick.redraw_requested = communication.redraw_requested;
		new_tick.stop = communication.stop || g_stopping || g_window_close_requested;

		new_tick.state_offset = state_buffer - state_buffer_start;
		memcpy(state_buffer, user_buffer, user_buffer_size);
		state_buffer += state_size;

		memcpy(tick_data_buffer, &new_tick, sizeof(struct Tick_Data));
		g_tick_data = tick_data_buffer;
		tick_data_buffer += 1;

		if (new_tick.stop != 0)
			break;

		continue;
	}

	if (s)
		tcc_delete(s);

	if (!is_window_open(hWnd))
		hWnd = create_window();

	for(const struct Tick_Data* tick = tick_data_buffer_start; tick <= tick_data_buffer; tick++)
	{
		tick_printf("\rTicks left: %lld, ", tick - tick_data_buffer_start);
		tick_printf("tick(%d,%d,%d,%d), ", tick->state_offset, tick->update_func_offset, tick->paint_func_offset, tick->key_down_func_offset, tick->source_offset);

		g_tick_data = tick;

		if (tick->redraw_requested)
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE); // Add "|RDW_ERASE" to see the flicker that is currently hidden by double buffering the draw target.

		poll_messages(hWnd, 0);

		if (tick->stop)
		{
			DestroyWindow(hWnd);
			break;
		}

		if (tick->redraw_requested)
			Sleep(2);
	}

	return;
}

LONG exception_handler(LPEXCEPTION_POINTERS p)
{
	FATAL(0, "Exception!!!\n");
	return EXCEPTION_EXECUTE_HANDLER;
}

void _start()
{
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)&exception_handler);
	run_recompilation_loop();
}

void _runmain() { _start(); }

#endif // SOURCE

///////////////////////////////////////////////////////////////////////////////

#ifdef RUNTIME_LOOP
#define SEGMENT_NAME "RUNTIME_LOOP"

#include <stdio.h>
#include <windows.h>
#include <time.h>

typedef signed long long i64;
i64 microseconds()
{
	clock_t c = clock();
	return ((i64)c) * (1000000ull / CLOCKS_PER_SEC);
}

#include "tetris.h"

typedef struct
{
	HWND hWnd;
	int initialized;
	int redraw_requested;
	unsigned tick;
	int x, y;
	unsigned long long old_window_proc;

	Tetris tetris;
} State;

enum { StateInitializedMagicNumber = 123456 };

typedef void Drawer;

void paint(Communication* communication, Drawer* drawer)
{
	//trace_printf("paint ");

	fill(drawer, 255, 255, 255);
	rect(drawer, 20, 20, 200, 200, 255, 255, 0);

	State* state = (State*)communication->buffer;
	if (state)
	{
		for (int x = state->x - 50; x < state->x + 50; ++x)
			pixel(drawer, x, state->y, 255, 0, 0);

		rect(drawer, state->x, state->y - 50, 1, 100, 0, 0, 255);
	}

	text(drawer, 30, 30, "Hello, World!", -1);
	text_w(drawer, 30, 60, L"Hëllö, Wärld!", -1);

	if (state)
		tetris_draw(drawer, &state->tetris);
}

int key_down(Communication* communication, int vk_key_code)
{
	State* state = (State*)communication->buffer;

	trace_printf("key_down ");
	trace_printf("INPUT tetris fall timer: %lld\n", state->tetris.fall_timer);
	trace_printf("INPUT tetris piece x,y: %d,%d\n", state->tetris.current_piece.x, state->tetris.	current_piece.y);

	switch (vk_key_code)
	{
		case VK_LEFT:
			state->tetris.input_left = 1;
			return 0;
		case VK_RIGHT:
			state->tetris.input_right = 1;
			return 0;
		case VK_DOWN:
			state->tetris.input_down = 1;
			return 0;
		case VK_UP:
			state->tetris.input_rotate = 1;
			return 0;
		case VK_SPACE:
			state->tetris.input_drop = 1;
			return 0;
	}
	
	return 1;
}

static void setup(State* state)
{
	if (state->initialized == StateInitializedMagicNumber)
		return;

	trace_printf("Clearing state...\n");
	memset(state, 0, sizeof(*state));

	trace_printf("Initializing state...\n");
	state->initialized = StateInitializedMagicNumber;
	state->tick = 0;
	state->x = 200;
	state->y = 150;

	printf("\n\nGo to the `tick` function at line %d of this source file and edit the 'state->x' and 'state->y' variables or something and see what happens. :)\n\n", __LINE__ + 3);
}

static void tick(State* state)
{
	// Modify these, save and note the cross in the window being painted to a different spot
	state->x = 200;
	state->y = 100;

	if (tetris_update(&state->tetris))
		state->redraw_requested = 2;
}

void update(Communication* communication)
{
	FATAL(sizeof(State) <= communication->buffer_size, "State is larger than the buffer. %lld <= %lld", sizeof(State), communication->buffer_size);

	i64 t = microseconds();

	State* state = (State*)communication->buffer;

	setup(state);

	state->tick += 1;
	if (state->tick % 10 == 0)
		printf("update(%5d)\n", state->tick);

	tick(state);

	communication->redraw_requested = 0 != state->redraw_requested;
	if (state->redraw_requested > 0)
		state->redraw_requested--;

	i64 d = microseconds() - t;
	trace_printf("%lldms\r", (d/1000) % 1000);

	Sleep(16);
}

#endif // RUNTIME_LOOP
