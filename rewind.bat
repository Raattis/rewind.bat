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
set compiler_executable=tcc.exe
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

	if not exist %compiler_executable% (
		echo Moving files from .\tcc\* to .\*
		robocopy /NJH /NJS /NS /NC /NFL /NDL /NP /MOVE /E tcc .

		if not exist %compiler_executable% (
			echo %compiler_executable% still not found.
			echo Download Tiny C Compiler manually and unzip it here.
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
) | %compiler_executable% -run -nostdlib -lmsvcrt -lkernel32 -luser32 -nostdinc -Iinclude -Iinclude/winapi -bench -Ilibtcc -Llibtcc -llibtcc -DSHARED_PREFIX -DSOURCE - %~n0%~x0
@exit ERRORLEVEL
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
#define FATAL(x, ...) do { if (x) break; fprintf(stderr, "%s:%d: (" SEGMENT_NAME "/%s) FATAL: ", __FILE__, __LINE__, __FUNCTION__); fprintf(stderr, __VA_ARGS__ ); fprintf(stderr, "\n(%s)\n", #x); void exit(int); exit(1); } while(0)

typedef struct
{
	int stop;
	int request_recompile;
	int was_recompiled;
	unsigned long long buffer_size;
	char* buffer;
} Communication;

#endif // SHARED_PREFIX

///////////////////////////////////////////////////////////////////////////////

#ifdef SOURCE

#include <stdio.h>
#include <windows.h>
#include <sys/stat.h>
#include <time.h>

#include <libtcc.h>

enum { debug_printing_verbose = 1 };

#define SEGMENT_NAME "SOURCE"

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
	if (debug_printing_verbose)
		printf("scan_includes('%s', %lld)\n", source_file, written);

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
	if (debug_printing_verbose)
		printf("find_corresponding_source_files(%lld, %lld)\n", includes_count, written_sources);

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
		if (debug_printing_verbose)
			printf("Scanning '%s'\n", source);

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


typedef LRESULT (*Window_Message_Handler_Func)(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
Window_Message_Handler_Func window_message_handler_impl = 0;

// Have this up here to prevent moving the function address due to resizing other functions when recompiling.
LRESULT window_message_handler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (window_message_handler_impl)
		return window_message_handler_impl(hWnd, message, wParam, lParam);
	else
		return DefWindowProc(hWnd, message, wParam, lParam);
}

Window_Message_Handler_Func get_window_message_handler()
{
	return &window_message_handler;
}

void run_recompilation_loop()
{
	void* malloc(size_t);
	struct headers_and_sources* headers_and_sources = (struct headers_and_sources*)malloc(sizeof(struct headers_and_sources));
	memset(headers_and_sources, 0, sizeof(*headers_and_sources));
	get_headers_and_sources(b_source_filename, headers_and_sources);

	file_timestamp newest_file_timestamp;
	get_file_timestamp(&newest_file_timestamp, b_source_filename);

	void* user_buffer = malloc(1000);
	int force_recompile = 1;

	typedef void (*UpdateFunc)(Communication* communication);
	UpdateFunc update = 0;

	TCCState *s = 0;
	int compilation_result_buffer_size = 16 * 1024 * 1024;
	void* compilation_result_buffer = malloc(compilation_result_buffer_size);

	for (;;)
	{
		int was_recompiled = 0;

		if (get_any_newer_file_timestamp(&newest_file_timestamp, headers_and_sources))
			force_recompile = 1;

		if (force_recompile)
		{
			clock_t c = clock();

			printf("Recompiling '%s'\n", b_source_filename);

			static char* source_buffer = 0;
			const int MAX_SOURCE_SIZE = 1024 * 1024 * 16; // 4 MB
			if (source_buffer == 0)
				source_buffer = malloc(MAX_SOURCE_SIZE);

			printf("Writing prefix\n");
			int prefix_length = sprintf(source_buffer,
				"\n" "#line 0 \"%s\""
				"\n" "#if GOTO_BOOTSTRAP_BUILDER"
				"\n"
				, b_source_filename);

			printf("Copying source\n");
			{
				char* src = source_buffer + prefix_length;
				int size_left = MAX_SOURCE_SIZE - prefix_length;

				FILE* src_file = fopen(b_source_filename, "r");
				size_t read_length = fread(src, sizeof(char), size_left, src_file);
				fclose(src_file);

				FATAL(read_length + 1 < size_left, "%s is too big (%d B < %d B) to runtime compile.", b_source_filename, read_length, size_left);

				src[read_length] = 0;
			}

			window_message_handler_impl = 0;

			if (s)
				tcc_delete(s);

			s = tcc_new();
			FATAL(s, "Could not create tcc state\n");

			printf("tcc_set_output_type  \n");
			tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

			//tcc_set_options(s, "-vv -nostdlib -nostdinc");
			tcc_set_options(s, "-nostdlib -nostdinc");

			tcc_add_include_path(s, "include");
			tcc_add_include_path(s, "include/winapi");

			tcc_add_library_path(s, "lib");

			extern int tcc_add_library_err(TCCState *s, const char *f);
			tcc_add_library_err(s, "gdi32");
			tcc_add_library_err(s, "msvcrt");
			tcc_add_library_err(s, "kernel32");
			tcc_add_library_err(s, "user32");

			tcc_set_options(s, "-DRUNTIME_LOOP -DSHARED_PREFIX");

			tcc_add_symbol(s, "get_window_message_handler", get_window_message_handler);

			printf("Compiling\n");
			if (-1 == tcc_compile_string(s, source_buffer))
			{
				trace_printf("Failed to recompile '%s'.\n", b_source_filename);
				Sleep(5000);
				continue;
			}

			trace_printf("Checking resulting size...\n");
			int size = tcc_relocate(s, 0);
			if (size < 0)
			{
				fprintf(stderr, "Failed get size for relocate (=linking). Err: %d\n", size);
				Sleep(5000);
				continue;
			}
			else
			{
				if (size > compilation_result_buffer_size)
				{
					if (size > 1024 * 1024 * 1024)
					{
						fprintf(stderr, "Sanity check failed: Compilation result is %d bytes which is more than 1 GB.\n", size);
						Sleep(5000);
						continue;
					}
					
					free(compilation_result_buffer);
					compilation_result_buffer_size = size;
					compilation_result_buffer = malloc(compilation_result_buffer_size);
				}
			}

#ifdef _WIN32
			DWORD old;
			if (!VirtualProtect(compilation_result_buffer, compilation_result_buffer_size, PAGE_READWRITE, &old))
			{
				fprintf(stderr, "Couldn't unlock page protection. Old protection value: %d", old);
				Sleep(5000);
				continue;
			}
#else
			#error "TODO: Copy non-windows memory protection undoing from tccrun.c set_pages_executable()"
#endif

			printf("Linking...\n");
			int err = 0;
			if (0 > (err = tcc_relocate(s, compilation_result_buffer)))
			{
				fprintf(stderr, "Failed to relocate (=link). Err: %d\n", err);
				Sleep(5000);
				continue;
			}

			clock_t milliseconds = (clock() - c) * (1000ull / CLOCKS_PER_SEC);
			printf("Recompilation took %lld.%03lld seconds. Executable size in memory is %lld.%03lld KB\n", milliseconds/1000ull, milliseconds%1000ull, size / 1000ull, size % 1000ull);

			update = tcc_get_symbol(s, "update");
			if (!update)
			{
				fprintf(stderr, "Failed to load the 'void update(Communication*)' symbol after recompilation.\n");
				Sleep(5000);
				continue;
			}

			window_message_handler_impl = tcc_get_symbol(s, "window_message_handler_impl");
			if (!window_message_handler_impl)
			{
				fprintf(stderr, "Failed to load the 'window_message_handler_impl' symbol after recompilation.\n");
				Sleep(5000);
				continue;
			}

			get_headers_and_sources(b_source_filename, headers_and_sources);

			force_recompile = 0;
			was_recompiled = 1;
		}

		if (!update)
		{
			fprintf(stderr, "'update' not loaded. Last error: 0x%X\n.", GetLastError());
			force_recompile = 1;
			Sleep(500);
			continue;
		}

		Communication communication = {0};
		communication.was_recompiled = was_recompiled;
		communication.buffer = user_buffer;
		communication.buffer_size = 1000;
		update(&communication);
		if (communication.stop != 0)
			break;

		if (communication.request_recompile != 0)
			force_recompile = 1;

		continue;
	}

	tcc_delete(s);
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

#include <stdio.h>
#include <windows.h>
#include <time.h>

#define SEGMENT_NAME "RUNTIME_LOOP"

enum { TetrisWidth = 10, TetrisHeight = 22 };
typedef enum { PieceL, PieceJ, PieceI, PieceO, PieceT, PieceS, PieceZ } PieceType;

typedef struct
{
	PieceType type;
	int rotation;
	int x, y;
} TetrisPiece;

typedef signed long long i64;
typedef struct
{
	int magic_number;
	TetrisPiece current_piece;
	unsigned char board[TetrisWidth * TetrisHeight];
	int lines_cleared;
	int score;
	i64 current_time_us;
	i64 fall_timer;
	int game_over;
	int input_left;
	int input_right;
	int input_down;
	int input_rotate;
	int input_drop;
} Tetris;

typedef struct
{
	HWND hWnd;
	int initialized;
	int redraw_requested;
	int window_closed;
	unsigned tick;
	int x, y;
	unsigned long long old_window_proc;

	Tetris tetris;
} State;
enum { StateInitializedMagicNumber = 123456 };

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

Drawer make_drawer(HWND hWnd)
{
	Drawer drawer;

	RECT screen_rect;
	GetClientRect(hWnd, &screen_rect);
	drawer.screen_width = screen_rect.right;
	drawer.screen_height = screen_rect.bottom;

	drawer.screen_device_context = BeginPaint(hWnd, &drawer.ps);
	drawer.hdc = CreateCompatibleDC(drawer.screen_device_context);
	drawer.bitmap = CreateCompatibleBitmap(drawer.screen_device_context, drawer.screen_width, drawer.screen_height);
	drawer.previous_gdi_object = SelectObject(drawer.hdc, drawer.bitmap);
	return drawer;
}

void free_drawer(HWND hWnd, Drawer drawer)
{
	BitBlt(drawer.screen_device_context, 0, 0, drawer.screen_width, drawer.screen_height, drawer.hdc, 0, 0, SRCCOPY);

	SelectObject(drawer.hdc, drawer.previous_gdi_object);
	DeleteObject(drawer.bitmap);
	DeleteDC(drawer.hdc);
	ReleaseDC(hWnd, drawer.screen_device_context);
	EndPaint(hWnd, &drawer.ps);
}

void pixel(Drawer drawer, int x, int y, int r, int g, int b)
{
	int success = SetPixel(drawer.hdc, x, y, RGB(r, g, b));
	//if (success < 0)
	//	fprintf(stderr, "Failed to set pixel to color. (%d, %d) -> (%d,%d,%d)", x,y, r,g,b);
}

void rect(Drawer drawer, int x, int y, int w, int h, int r, int g, int b)
{
	RECT rect = {x, y, x+w, y+h};
	HBRUSH brush = CreateSolidBrush(RGB(r,g,b));
	int success = FillRect(drawer.hdc, &rect, brush);
	if (success < 0)
		fprintf(stderr, "Failed to draw a rectangle. (%d, %d, %d, %d)", x,y, w,h);
	DeleteObject(brush);
}

void fill(Drawer drawer, int r, int g, int b)
{
	int w = GetDeviceCaps(drawer.hdc, HORZRES);
	int h = GetDeviceCaps(drawer.hdc, VERTRES);
	rect(drawer, 0,0, w,h, r,g,b);
}

void text(Drawer drawer, int x, int y, char* str, int strLen)
{
	RECT rect = {x, y, x, y};
	DrawTextExA(drawer.hdc, str, strLen, &rect, DT_NOCLIP|DT_NOPREFIX|DT_SINGLELINE|DT_CENTER|DT_VCENTER, 0);
}

void text_w(Drawer drawer, int x, int y, wchar_t* str, int strLen)
{
	RECT rect = {x, y, x, y};
	DrawTextExW(drawer.hdc, str, strLen, &rect, DT_NOCLIP|DT_NOPREFIX|DT_SINGLELINE|DT_CENTER|DT_VCENTER, 0);
}

i64 microseconds()
{
	clock_t c = clock();
	return ((i64)c) * (1000000ull / CLOCKS_PER_SEC);
}

void tetris_draw(Drawer drawer, Tetris* tetris);

void paint(HWND hWnd, State* state)
{
	Drawer drawer = make_drawer(hWnd);

	fill(drawer, 255, 255, 255);
	rect(drawer, 20, 20, 200, 200, 255, 255, 0);

	if (state)
	{
		for (int x = state->x - 50; x < state->x + 50; ++x)
			pixel(drawer, x, state->y, 255, 0, 0);

		rect(drawer, state->x, state->y - 50, 1, 100, 0, 0, 255);
	}

	text(drawer, 30, 30, "Hello, World!", -1);
	text_w(drawer, 30, 60, L"Hëllö, Wärld!", -1);

	tetris_draw(drawer, &state->tetris);

	free_drawer(hWnd, drawer);
}

int window_message_handler_impl(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			if (!SetWindowPos(hWnd, NULL, 1400, 70, 0, 0, SWP_NOSIZE | SWP_NOZORDER))
				FATAL(0, "Failed to position window. Error: ", GetLastError());

			CREATESTRUCT *pCreate = (CREATESTRUCT*)lParam;
			State* state = (State*)pCreate->lpCreateParams;
			FATAL(state->initialized == StateInitializedMagicNumber, "State not initialized in message loop.");
			SetLastError(0);
			if (!SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)state) && GetLastError() != 0)
				printf("State set failed. Error: %d\n", GetLastError());

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
			State* state = (State*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (!state)
				printf("No state.\n");

			paint(hWnd, state);
			return 1;
		}
		case WM_KEYDOWN:
		{
			if (wParam == VK_ESCAPE)
			{
				printf("VK_ESCAPE\n");
				DestroyWindow(hWnd);
				return 0;
			}

			State *state = (State*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			switch (wParam)
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

			break;
		}
		case WM_QUIT:
			printf("WM_QUIT\n");
			break;
		case WM_DESTROY:
		{
			printf("WM_DESTROY\n");
			//PostQuitMessage(0);
			State *state = (State*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (state)
				state->window_closed = 1;
			// fallthrough
		}
		default:
			//printf("%x\n", message);
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

typedef LRESULT (*Window_Message_Handler_Func)(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
extern Window_Message_Handler_Func get_window_message_handler();

void create_window(State* state)
{
	Window_Message_Handler_Func window_message_handler = get_window_message_handler();
	state->old_window_proc = (unsigned long long)window_message_handler;

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
	state->hWnd = CreateWindow("MyWindowClass", GetCommandLine(), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
		NULL, NULL, GetModuleHandle(NULL), state);
	ShowWindow(state->hWnd, SW_SHOW);
}

int poll_messages(State* state)
{
	if (!state->hWnd)
		return 0;

	MSG msg;
	while (PeekMessage(&msg, state->hWnd, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

void get_block_offsets(PieceType piece_type, int rotation, int* out_offsets_x, int* out_offsets_y)
{
	//                     PieceL,  PieceJ,  PieceI, PieceO, PieceT,  PieceS,  PieceZ
	int offsets_x[7*3] = {-1,-1,1,  1,-1,1, -1,1,2,  1,0,1,  1,-1,0,  1,0,-1, -1,0,1};
	int offsets_y[7*3] = { 1, 0,0,  1, 0,0,  0,0,0,  0,1,1,  0, 0,1,  0,1, 1,  0,1,1};

	//                       L, J, I, O, T, S, Z
	int rotation_counts[] = {4, 4, 2, 1, 4, 2, 2};

	out_offsets_x[0] = out_offsets_y[0] = 0;
	for (int i = 1; i < 4; ++i)
	{
		out_offsets_x[i] = offsets_x[piece_type * 3 + i-1];
		out_offsets_y[i] = offsets_y[piece_type * 3 + i-1];
	}

	rotation = rotation % rotation_counts[piece_type];
	for (int r = 0; r < rotation; ++r)
	{
		for (int i = 1; i < 4; ++i)
		{
			int temp = out_offsets_x[i];
			out_offsets_x[i] = -out_offsets_y[i];
			out_offsets_y[i] = temp;
		}
	}
}

void get_piece_blocks(TetrisPiece piece, int* out_piece_x, int* out_piece_y)
{
	int piece_offsets_x[4] = {0};
	int piece_offsets_y[4] = {0};
	get_block_offsets(piece.type, piece.rotation, piece_offsets_x, piece_offsets_y);

	for (int i = 0; i < 4; ++i)
	{
		out_piece_x[i] = piece.x + piece_offsets_x[i];
		out_piece_y[i] = piece.y + piece_offsets_y[i];
	}
}

void get_tetris_color(int tile_color, int* r, int* g, int* b)
{
	int x = 170, y = 70, z = 110;
	switch(tile_color)
	{
	case 0: *r = *g = *b = 0; return;
	case 1: *r = x; *g = *b = y; return;
	case 2: *g = x; *r = *b = y; return;
	case 3: *b = x; *r = *g = y; return;
	case 4: *r = *b = x; *g = y; return;
	case 5: *g = *r = x; *b = y; return;
	case 6: *b = *g = x; *r = y; return;
	case 7: *b = *r = *g = z; return;
	}
}

int move_piece_to(TetrisPiece* piece, Tetris* tetris, int x, int y, int r)
{
	int offsets_x[4];
	int offsets_y[4];
	get_block_offsets(piece->type, piece->rotation + r, offsets_x, offsets_y);

	for (int i = 0; i < 4; ++i)
	{
		int xx = piece->x + x + offsets_x[i];
		int yy = piece->y + y + offsets_y[i];
		if (xx < 0 || xx >= TetrisWidth)
			return 0;
		if (yy >= TetrisHeight)
			return 0;
		if (yy < 0)
			continue;

		if (tetris->board[xx + yy * TetrisWidth])
			return 0;
	}

	piece->rotation += r;
	piece->x += x;
	piece->y += y;
	return 1;
}

int move_to(Tetris* tetris, int x, int y, int r)
{
	return move_piece_to(&tetris->current_piece, tetris, x, y, r);
}

void tetris_draw(Drawer drawer, Tetris* tetris)
{
	RECT screen_rect;
	GetClientRect(WindowFromDC(drawer.screen_device_context), &screen_rect);
	int screen_height = screen_rect.bottom - screen_rect.top;
	int h = (screen_height - 20) / TetrisHeight;
	int w = h;
	int margin = w/2;
	rect(drawer, 0, 0, w * TetrisWidth + margin*2, w * TetrisHeight + margin*2, 70,10,50);

	TetrisPiece shadow_piece = tetris->current_piece;
	while (move_piece_to(&shadow_piece, tetris, 0, 1, 0)) {}

	int piece_x[4] = {0};
	int piece_y[4] = {0};
	get_block_offsets(tetris->current_piece.type, tetris->current_piece.rotation, piece_x, piece_y);
	for (int i = 0; i < 4; ++i)
	{
		piece_x[i] += tetris->current_piece.x;
		piece_y[i] += tetris->current_piece.y;
	}

	for (int y = 0; y < TetrisHeight; ++y)
	{
		for (int x = 0; x < TetrisWidth; ++x)
		{
			int tile_color = tetris->board[y * TetrisWidth + x];
			int piece_hit = 0;
			int shadow_hit = 0;
			int shadow_piece_hit = 0;

			for (int i = 0; i < 4; ++i)
			{
				if (piece_x[i] != x || piece_y[i] > y)
					continue;

				int shadow_y = piece_y[i] - tetris->current_piece.y + shadow_piece.y;
				if (piece_y[i] == y)
					piece_hit = 1;
				else if (shadow_y == y)
					shadow_piece_hit = 1;
				else if (shadow_y > y)
					shadow_hit = 1;
				else
					continue;

				tile_color = tetris->current_piece.type + 1;
			}

			int r,g,b;
			get_tetris_color(tile_color, &r,&g,&b);
			int divider = 1;
			if (piece_hit)
				divider = 1;
			else if (shadow_piece_hit)
				divider = 2;
			else if (shadow_hit)
				divider = 3;

			r/=divider; g/=divider; b/=divider;

			rect(drawer, x * w + margin, y * h + margin, w, h, r,g,b);
		}
	}

	char score_buffer[32];
	sprintf(score_buffer, "%d", tetris->score);
	text(drawer, drawer.screen_width / 2, 30, score_buffer, -1);

	if (tetris->game_over)
		text(drawer, drawer.screen_width / 2, 60, "Game Over!", -1);
}

int tetris_update(Tetris* tetris)
{
	if (tetris->magic_number != StateInitializedMagicNumber
		|| (tetris->game_over && tetris->input_drop))
	{
		memset(tetris, 0, sizeof *tetris);
		tetris->magic_number = StateInitializedMagicNumber;
		tetris->current_time_us =  microseconds();
		tetris->fall_timer = 1000 * 1000; // 1 second
		tetris->current_piece.x = 5;
		return 1;
	}

	if (tetris->game_over)
		return 0;

	{
		i64 t = microseconds();
		tetris->fall_timer -= t - tetris->current_time_us;
		//printf("fall: %lld, t: %lld, ct: %lld, -:%lld\n", tetris->fall_timer, t, tetris->current_time_us, t - tetris->current_time_us);
		tetris->current_time_us = t;
	}

	int move_left = tetris->input_left;
	int move_right = tetris->input_right;
	int move_down = tetris->input_down;
	int rotate = tetris->input_rotate;
	int drop = tetris->input_drop;
	tetris->input_left = tetris->input_right = tetris->input_down = tetris->input_rotate = tetris->input_drop = 0;

	int difficulty = 1 + tetris->lines_cleared / 10;
	i64 drop_delay = 1000 * 1000 / difficulty;
	if (drop)
	{
		tetris->fall_timer = drop_delay;
	}
	else if (move_down)
	{
		tetris->fall_timer = drop_delay;
		tetris->score += 1;
	}
	else if (tetris->fall_timer <= 0)
	{
		move_down = 1;
		tetris->fall_timer += drop_delay;
		if (tetris->fall_timer < 0)
			tetris->fall_timer = drop_delay; // No double drops if the game was paused etc.
	}

	if (rotate)
	{
		int i_piece_nudge = tetris->current_piece.type == PieceI && tetris->current_piece.x >= 8;

		move_to(tetris, 0,0,1)
		|| move_to(tetris,  1,0,1)
		|| move_to(tetris, -1,0,1)
		|| (i_piece_nudge && move_to(tetris, -2,0,1))
		|| move_to(tetris, 0,1,1);
	}

	if (move_left)
		move_to(tetris, -1,0,0);

	if (move_right)
		move_to(tetris, 1,0,0);

	if (drop)
	{
		while (move_to(tetris, 0,1,0))
			tetris->score += 1;
		move_down = 1;
	}

	if (move_down)
	{
		if (!move_to(tetris, 0,1,0))
		{
			int offsets_x[4];
			int offsets_y[4];
			get_block_offsets(tetris->current_piece.type, tetris->current_piece.rotation, offsets_x, offsets_y);

			// stick
			for (int i = 0; i < 4; ++i)
			{
				int x = tetris->current_piece.x + offsets_x[i];
				int y = tetris->current_piece.y + offsets_y[i];
				tetris->board[x + y * TetrisWidth] = tetris->current_piece.type + 1;
			}

			// destroy
			int clears = 0;
			for (int y = TetrisHeight; y-- > 0;)
			{
				int block_count = 0;
				for (int x = 0; x < TetrisWidth; ++x)
				{
					int val = tetris->board[x + y * TetrisWidth];
					if (val)
						block_count += 1;

					tetris->board[x + y * TetrisWidth] = 0;
					tetris->board[x + (y + clears) * TetrisWidth] = val;
				}

				if (block_count == 10)
					clears += 1;
			}

			// meta
			tetris->lines_cleared += clears;
			tetris->score += clears * clears * 100 * difficulty;

			// new piece
			tetris->current_piece.type = (tetris->current_piece.type + 1) % 7;
			tetris->current_piece.x = 5;
			tetris->current_piece.y = 0;
			tetris->current_piece.rotation = 0;

			if (!move_to(tetris, 0,0,0))
			{
				tetris->game_over = 1;
				printf("Game Over! Final Score: %d\n", tetris->score);
			}
		}
	}

	return move_down || move_left || move_right || move_down || rotate || drop;
}

static void setup(State* state)
{
	if (state->initialized == StateInitializedMagicNumber)
	{
		FATAL(state->old_window_proc == (unsigned long long)get_window_message_handler()
			, "Window message handler function address moved. 0x%X == 0x%X\n"
			, state->old_window_proc, (unsigned long long)get_window_message_handler());
		return;
	}

	printf("Init state.\n");
	memset(state, 0, sizeof(*state));
	state->initialized = StateInitializedMagicNumber;
	state->tick = 0;
	state->x = 200;
	state->y = 150;

	create_window(state);

	printf("\n\nGo to the `tick` function at line %d of this source file and edit the 'state->x' and 'state->y' variables or something and see what happens. :)\n\n", __LINE__ + 3);
}

void tick(State* state)
{
	// Modify these, save and note the cross in the window being painted to a different spot
	state->x = 200;
	state->y = 100;

	if (tetris_update(&state->tetris))
		state->redraw_requested = 1;
}

void update(Communication* communication)
{
	FATAL(sizeof(State) <= communication->buffer_size, "State is larger than the buffer. %lld <= %lld", sizeof(State), communication->buffer_size);

	i64 t = microseconds();

	State* state = (State*)communication->buffer;
	setup(state);

	state->tick += 1;
	if (state->tick % 100 == 0)
		printf("update(%5d)\n", state->tick);

	tick(state);

	if (state->hWnd && (communication->was_recompiled || state->redraw_requested))
	{
		state->redraw_requested = 0;
		RedrawWindow(state->hWnd, NULL, NULL, RDW_INVALIDATE); // Add "|RDW_ERASE" to see the flicker that is currently hidden by double buffering the draw target.
	}

	if (poll_messages(state) != 0)
		communication->stop = 1;

	if (state->window_closed)
		communication->stop = 1;

	i64 d = microseconds() - t;
	printf("%lldms\r", (d/1000) % 1000);

	Sleep(16);
}

__declspec(dllexport) void dll_update(Communication* communication)
{
	update(communication);
}

#endif // RUNTIME_LOOP
