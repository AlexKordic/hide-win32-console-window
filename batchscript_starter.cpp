
#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>

#include <string>
#include <time.h>

// buffer size for chunked read/write operation
#define BUFSIZE 4096
#define SLEEP_MS 500

// \\\\ \\\\\\\\
// Two pipes created in total
// main program uses this handles. Marked as non-inheritable
HANDLE g_pipe_main_stdin               = NULL;
HANDLE g_pipe_main_stderr_stdout       = NULL;
// subprocess uses this handles.
HANDLE g_pipe_subprocess_stderr_stdout = NULL;
HANDLE g_pipe_subprocess_stdin         = NULL;
// //// ////////

PROCESS_INFORMATION g_process_information;
// HANDLE g_hInputFile = NULL;
TCHAR               current_dirpath[MAX_PATH];
std::wstring        output_filename;
HANDLE              output_file_handle        = INVALID_HANDLE_VALUE;

// Creates subprocess with connected pipes 
//     child(STDOUT, STDERR) >> g_pipe_main_stderr_stdout
//     g_pipe_main_stdin     >> child(STDIN)
void CreateChildProcess(LPWSTR commandline_from_main);

// Reads from g_pipe_main_stderr_stdout pipe and writes it to log file (output_file_handle)
void ReadFromPipe(void);

// Show MessageBox and exit
void ErrorExit(const wchar_t *);

// Check if subprocess ended and if so closes pipe handles unblocking readfile operation.
// DWORD WINAPI WaitForSubprocessEnd(LPVOID parameter);



// Used for debugging pipe transfer
// int print_log(const char* format, ...) {
// 	static char s_printf_buf[1024];
// 	va_list args;
// 	va_start(args, format);
// 	_vsnprintf_s(s_printf_buf, sizeof(s_printf_buf), format, args);
// 	va_end(args);
// 	OutputDebugStringA(s_printf_buf);
// 	return 0;
// }


void compose_output_filename(LPWSTR * argv) {
	std::wstring target = argv[0];
	WCHAR    file_name[100];
	errno_t  split_status = _wsplitpath_s(
		argv[0],
		NULL, 0,
		NULL, 0,
		file_name, 100,
		NULL, 0
	);
	DWORD  status = GetCurrentDirectory(MAX_PATH, current_dirpath);
	time_t time_now = time(NULL);
	struct tm tm;
	errno_t localtime_status = localtime_s(&tm, &time_now);
	if(localtime_status != 0) {
		ErrorExit(TEXT("localtime_s error"));
	}
	WCHAR  timestamp_buffer[30];
	int timestamp_charcount = swprintf_s(
		(wchar_t *) timestamp_buffer,
		30,
		L"%02d-%02d-%02d-%02d-%02d-%02d",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec
	);
	output_filename += current_dirpath;
	output_filename += TEXT("/");
	output_filename += file_name;
	output_filename += TEXT(".");
	output_filename += timestamp_buffer;
	output_filename += TEXT(".log");
}



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow) 
{
	int      argc         = 0;
	LPWSTR * argv         = CommandLineToArgvW(lpCmdLine, &argc);
	compose_output_filename(argv);
	output_file_handle    = CreateFile(
		output_filename.c_str(), 
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, 
		NULL, 
		OPEN_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL
	);
	if(INVALID_HANDLE_VALUE == output_file_handle) {
		ErrorExit(TEXT("Failed to open log file"));
	}


	if(argc == 1) ErrorExit(TEXT("Please specify executable target as first argument.\n"));

	SECURITY_ATTRIBUTES security_attributes;

	// Set the bInheritHandle flag so pipe handles are inherited. 
	security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	security_attributes.bInheritHandle = TRUE;
	security_attributes.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT. 
	if(!CreatePipe(
		&g_pipe_main_stderr_stdout,        // handle to read inlet of this pipe
		&g_pipe_subprocess_stderr_stdout,  // handle to write inlet of this pipe
		&security_attributes, 
		0)) ErrorExit(TEXT("StdoutRd CreatePipe"));

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if(!SetHandleInformation(g_pipe_main_stderr_stdout, HANDLE_FLAG_INHERIT, 0)) ErrorExit(TEXT("Stdout SetHandleInformation"));

	// Create a pipe for the child process's STDIN. 
	if(!CreatePipe(
		&g_pipe_subprocess_stdin,          // handle to read inlet of this pipe
		&g_pipe_main_stdin,                // handle to write inlet of this pipe
		&security_attributes, 
		0)) ErrorExit(TEXT("Stdin CreatePipe"));

	// Ensure the write handle to the pipe for STDIN is not inherited. 
	if(!SetHandleInformation(g_pipe_main_stdin, HANDLE_FLAG_INHERIT, 0)) ErrorExit(TEXT("Stdin SetHandleInformation"));

	CreateChildProcess(lpCmdLine);

	// Read from pipe that is the standard output for child process. 
	ReadFromPipe();

	CloseHandle(output_file_handle);
	CloseHandle(g_process_information.hProcess);
	CloseHandle(g_process_information.hThread);
	CloseHandle(g_pipe_main_stdin);
	CloseHandle(g_pipe_main_stderr_stdout);
	CloseHandle(g_pipe_subprocess_stderr_stdout);
	CloseHandle(g_pipe_subprocess_stdin);

	// The remaining open handles are cleaned up when this process terminates. 
	// To avoid resource leaks in a larger application, close handles explicitly. 
	return 0;
}

// Create a child process that uses the previously created pipes for STDIN and STDOUT.
void CreateChildProcess(LPWSTR commandline_from_main)
{
	STARTUPINFO         startup_information;
	BOOL                operation_ok = FALSE;

	// Set up members of the PROCESS_INFORMATION structure. 
	ZeroMemory(&g_process_information, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.
	ZeroMemory(&startup_information, sizeof(STARTUPINFO));
	startup_information.cb         = sizeof(STARTUPINFO);
	startup_information.hStdError  = g_pipe_subprocess_stderr_stdout;
	startup_information.hStdOutput = g_pipe_subprocess_stderr_stdout;
	startup_information.hStdInput  = g_pipe_subprocess_stdin;
	startup_information.dwFlags   |= STARTF_USESTDHANDLES;

	// Create the child process. 
	operation_ok = CreateProcessW(
		NULL,
		commandline_from_main, // command line 
		NULL,                  // process security attributes 
		NULL,                  // primary thread security attributes 
		TRUE,                  // handles are inherited 
		CREATE_NO_WINDOW,      // creation flags 
		NULL,                  // use parent's environment 
		NULL,                  // use parent's current directory 
		&startup_information,  // STARTUPINFO pointer 
		&g_process_information); // receives PROCESS_INFORMATION 

	 // If an error occurs, exit the application. 
	if(!operation_ok) {
		ErrorExit(TEXT("CreateProcess"));
	}
}

void ReadFromPipe(void)
{
	DWORD  bytes_read, bytes_written;
	CHAR   chBuf[BUFSIZE];
	BOOL   operation_ok  = FALSE;
	HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	for(;;) {
		DWORD bytes_available = 0;
		BOOL ok_if_not_zero = PeekNamedPipe(g_pipe_main_stderr_stdout, NULL, 0, NULL, &bytes_available, NULL);
		if(ok_if_not_zero != 0 && bytes_available > 0) {
			DWORD size = min(bytes_available, BUFSIZE);
			operation_ok = ReadFile(g_pipe_main_stderr_stdout, chBuf, size, &bytes_read, NULL);
			if(!operation_ok || bytes_read == 0) return;

			operation_ok = WriteFile(output_file_handle, chBuf, bytes_read, &bytes_written, NULL);
			if(!operation_ok) return;
		} else {
			DWORD exit_code = -1;
			BOOL  zero_on_failure = GetExitCodeProcess(g_process_information.hProcess, &exit_code);
			if(zero_on_failure == 0) {
				ErrorExit(TEXT("GetExitCodeProcess failed"));
			}
			if(exit_code != STILL_ACTIVE) {
				return;
			}
			Sleep(SLEEP_MS);
		}
	}
}

// Format a readable error message, display a message box, 
// and exit from the application.
void ErrorExit(const wchar_t * message_to_display)
{
	LPVOID message_buffer;
	LPVOID display_buffer;
	DWORD  error_code = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error_code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &message_buffer,
		0, NULL);

	display_buffer = (LPVOID) LocalAlloc(
		LMEM_ZEROINIT,
		(lstrlen((LPCTSTR) message_buffer) + lstrlen((LPCTSTR) message_to_display) + 40) * sizeof(TCHAR)
	);
	StringCchPrintf(
		(LPTSTR) display_buffer,
		LocalSize(display_buffer) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		message_to_display, 
		error_code, 
		message_buffer
	);
	MessageBox(NULL, (LPCTSTR) display_buffer, TEXT("Error"), MB_OK);

	LocalFree(message_buffer);
	LocalFree(display_buffer);
	ExitProcess(1);
}
