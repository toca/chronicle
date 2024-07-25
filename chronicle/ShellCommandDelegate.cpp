#include "shellcommanddelegate.h"
#include <Windows.h>

ShellCommandDelegate::~ShellCommandDelegate()
{
    this->waitExitThread.join();
    this->Close();
}

Result<ShellCommandDelegate*> ShellCommandDelegate::Create()
{
    // Security Attributes to use handle by child process
    SECURITY_ATTRIBUTES security{};
    security.nLength = sizeof(SECURITY_ATTRIBUTES);
    security.bInheritHandle = TRUE;

    ShellCommandDelegate *self = new ShellCommandDelegate();
    if (!::CreatePipe(&self->stdinRead, &self->stdinWrite, &security, 0)) {
        return { std::nullopt, Error(::GetLastError(), L"Failed to ::CreatePipe") };
    }

    ::SetHandleInformation(self->stdinWrite, HANDLE_FLAG_INHERIT, 0);
    
    
    // launch cmd.exe
    STARTUPINFOA startupInfo{};
    startupInfo.cb = sizeof(startupInfo);

    startupInfo.dwFlags |= STARTF_USESTDHANDLES;
    startupInfo.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);
    startupInfo.hStdOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
    startupInfo.hStdInput = self->stdinRead;

    //startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
    //startupInfo.wShowWindow = SW_SHOW;


    // launch cmd.exe
    char commandLine[] = "cmd.exe /Q";
    auto res = ::CreateProcessA(
        nullptr,
        commandLine,
        nullptr,
        nullptr,
        TRUE,
        //CREATE_NEW_CONSOLE,
        //CREATE_NEW_PROCESS_GROUP,
        0, // if 0 Ctrl + C send to each process
        nullptr,
        nullptr,
        &startupInfo,
        &self->processInfo
    );
    if (!res) {
        return { std::nullopt, Error(::GetLastError(), L"Failed to ::CreateProcess")};
    }
   
    //SetConsoleOutputCP(CP_UTF8);
    //DWORD consoleMode;
    //consoleMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    //consoleMode |= ENABLE_PROCESSED_INPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    //SetConsoleMode(self->stdinWrite, consoleMode);
    self->StartWaitingForExit();
    return { self, std::nullopt };
}


std::optional<Error> ShellCommandDelegate::Input(const std::string& line)
{
    if (!line.size()) {
        return std::nullopt;
    }
    DWORD written = 0;
    if (!::WriteFile(this->stdinWrite, line.data(), line.size(), &written, nullptr)) {
        return Error(::GetLastError(), L"Failed to WriteFile");
    }
    else {
        return std::nullopt;
    }
}

void ShellCommandDelegate::Exit()
{
    this->Input("exit\r\n");
}

void ShellCommandDelegate::OnExit(std::function<void()> callback)
{
    this->exitListener = callback;
}

void ShellCommandDelegate::Break()
{
    ::GenerateConsoleCtrlEvent(CTRL_C_EVENT, this->processInfo.dwProcessId);
}

void ShellCommandDelegate::Wait()
{
    ::WaitForSingleObject(this->processInfo.hProcess, INFINITE);
}


ShellCommandDelegate::ShellCommandDelegate()
{
}

void ShellCommandDelegate::StartWaitingForExit()
{
    this->waitExitThread = std::thread([this]()
        {
            this->Wait();
            if (this->exitListener) {
                this->exitListener();
            }
        }
    );
}

void ShellCommandDelegate::Close()
{
    ::CloseHandle(this->stdinRead);
    ::CloseHandle(this->stdinWrite);
    ::CloseHandle(this->processInfo.hProcess);
    ::CloseHandle(this->processInfo.hThread);

}