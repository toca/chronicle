#include <Windows.h>
#include <locale.h>

#include <format>
#include <memory>
#include <cstdio>
#include <iostream>
#include <thread>
#include <fstream>
#include <array>

#include <tuple>
#include "shellcommanddelegate.h"
#include "keyhooker.h"
#include "history.h"
#include "taskqueue.h"
#include "searchview.h"
#include "result.h"
#include "defer.h"


std::unique_ptr<ShellCommandDelegate> cmd;
std::unique_ptr<KeyHooker> hooker;
std::unique_ptr<History> history;
std::unique_ptr<TaskQueue<std::function<void()>>> queue;
std::unique_ptr<SearchView> searchView;

bool stop = false;
bool searching = false;
HANDLE stdinHandle = ::GetStdHandle(STD_INPUT_HANDLE);
HANDLE consoleIn{};
OVERLAPPED overLapped{};

std::thread hookThread;
std::thread queueThread;

void Prev();
void Next();
bool AmIActive();
HANDLE OpenConsoleIn();
OptionalError Clear();
OptionalError Send(const std::string& message);

bool Redirectable(const std::string& command);
void ShowPrompt();
COORD PromptPosition();
Result<std::string> ReadUncommitedInputs(COORD from);
void WaitOutputDone();


Result<std::ifstream> OpenHistoryFile();
Result<std::ofstream> GetHistoryFile();


// M A I N ////////////////
int main()
{
    // TODO
    //   * transfer uncommited input to searh mode
    //   * remove uncommited input when going to search mode (memorize cursor pos before input)
    try {
        // locale
        setlocale(LC_ALL, "");
        
        // history
        history.reset(new History());
        auto [ihistoryFile, iresult] = OpenHistoryFile();
        if (ihistoryFile) {
            history->Load(*ihistoryFile);
            ihistoryFile->close();
        }


        // Open CONIN$ to async read
        consoleIn = OpenConsoleIn();
        if (!consoleIn) {
            auto err = ::GetLastError();
            fprintf(stderr, "Failed to OpenConsoleIn %d", err);
            return err;
        }

        // OVERLAPPED 
        overLapped.hEvent = ::CreateEventA(nullptr, TRUE, FALSE, nullptr);
        if (!overLapped.hEvent) {
            auto err = ::GetLastError();
            fprintf(stderr, "Failed to ::CreateEvent %d", err);
            return err;
        }
        DEFER([]() { ::CloseHandle(overLapped.hEvent); });


        // CMD
        auto [value, error] = ShellCommandDelegate::Create();
        if (error) {
            printf("%s %d\n", error->message.c_str(), error->code);
            return error->code;
        }
        cmd.reset(*value);
        // on internal cmd.exe exited
        cmd->OnExit([]()
            {
                stop = true;
                ::CancelIoEx(consoleIn, &overLapped);
            }
        );
        DEFER([]() { cmd->Wait(); });


        // queue
        queue.reset(new TaskQueue<std::function<void()>>());
        queueThread = std::thread([&]() {
            queue->processTasks();
        });
        DEFER([]() { queue->Stop(); queueThread.join(); });
         
        
        // key hook
        hooker.reset(new KeyHooker([](KBDLLHOOKSTRUCT k) {
            // if not active nothing to do
            if (!AmIActive() || searching) return false;
            // to return true means input has been processed here.
            switch (k.vkCode) {
            case VK_UP:
                queue->enqueue([&]() {
                    //OutputDebugStringA("UP!!\n");
                    Prev();
                });
                return true;
            case VK_DOWN:
                queue->enqueue([&]() {
                    //OutputDebugStringA("DOWN!!\n");
                    Next();
                });
                return true;
            case 'R': // search mode
                if (::GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                    queue->enqueue([&]() {
                        OutputDebugStringA("SearchMode!!\n");
                        searching = true;
                        ::CancelIoEx(consoleIn, &overLapped);
                        });
                    return true;
                } else {
                    return false;
                }
            default:
                return false;
            }
        }));

        hookThread = std::thread([&]() {
            auto err = hooker->Start();
            if (err) {
                fprintf(stderr, err->message.c_str());
            }
        });

        DEFER([]() { hooker->Stop(); hookThread.join(); });


        // search view
        searchView.reset(new SearchView());

        // Handle CTRL + C
        ::SetConsoleCtrlHandler([](DWORD event) -> BOOL {
            //::OutputDebugStringA(std::format("e: %d", event).c_str());
            if (event == CTRL_C_EVENT)
            {
                if (stop) return FALSE;
                stop = true;
                cmd->Exit();
                searchView->Stop();
                printf("\nCtrl+C\n");
                ::CancelIoEx(consoleIn, &overLapped);
                return TRUE;
            }
            else if (event == CTRL_CLOSE_EVENT) {
                printf("close");
            }
            return FALSE;
            }, TRUE
        );

        // wait for cmd's output done
        WaitOutputDone();

        // reading input loop ////////////////
        // TODO test when over length
        std::string buf(4096, '\0');
        while (!stop) {
            DWORD read = 0;
            buf.assign(buf.size(), '\0');

            auto promptPos = PromptPosition();

            // read input here -------------------------------------------------------
            if (!::ReadFile(consoleIn, buf.data(), buf.size(), &read, &overLapped)) {
                auto err = GetLastError();
                if (err != ERROR_IO_PENDING) {
                    fprintf(stderr, "Failed to ::ReadFile %d\n", err);
                    return err;
                }
                ::WaitForSingleObject(overLapped.hEvent, INFINITE);
                DWORD transferred = 0;
                if (!::GetOverlappedResult(consoleIn, &overLapped, &transferred, TRUE)) {
                    auto err = ::GetLastError();
                    if (err != ERROR_OPERATION_ABORTED) {
                        fprintf(stderr, "Failed to ::GetOverlappedResult %d\n", err);
                        return err;
                    }
                }
            }
            // read done or canceled
            if (stop) break;

            // search mode ----
            if (searching) {
                // read uncommited buffer and remove it
                auto[ uncommitedInputs, readErr ] = ReadUncommitedInputs(promptPos);
                if (readErr) {
                    fprintf(stderr, "%s %d", readErr->message.c_str(), readErr->code);
                    return readErr->code;
                }
                Clear();

                ::CloseHandle(consoleIn);
                auto [ result, err] = searchView->Show(history->GetAll());
                if (err) {
                    fprintf(stderr, "%s %d", err->message.c_str(), err->code);
                    return err->code;
                }
                if (result) {
                    Clear();
                    Send(*result);
                }
                searching = false;
                consoleIn = OpenConsoleIn();
                if (!consoleIn) {
                    auto err = ::GetLastError();
                    fprintf(stderr, "Failed to OpenConsoleIn %d", err);
                    return err;
                }
                continue;
            }

            // process command
            std::string line;
            auto pos = buf.find('\r');
            if (pos != std::string::npos) {
                line.assign(buf.begin(), buf.begin() + pos);
            }
            if (Redirectable(line)) {
                cmd->Input(line + '\n');
            }
            else {
                system(line.c_str());
                ShowPrompt();
            }
            if (line.size()) {
                history->Add(line);
            }
            WaitOutputDone();
        }
        //////////////////////////////////////


        // save histories
        auto [ohistoryFile, oresult] = GetHistoryFile();
        if (ohistoryFile) {
            history->Dump(*ohistoryFile);
            ohistoryFile->close();
        }

        printf("Bye.\n");
       
    }
    catch (...) {
        return 1;
    }

    return 0;
}


void Prev()
{
    auto s = history->Older();
    if (s) {
        ::OutputDebugStringA(std::format("{}\n", *s).c_str());
        Clear();
        Send(*s);
    }
}


void Next() 
{
    auto s = history->Newer();

    if (s) {
        ::OutputDebugStringA(std::format("{}\n", *s).c_str());
        Clear();
        Send(*s);
    }
}


bool AmIActive()
{
    std::string consoleTitle(256, '\0');
    ::GetConsoleTitleA(consoleTitle.data(), consoleTitle.size());
    //::OutputDebugStringA(consoleTitle.c_str());
    std::string foregroundTitle(256, '\0');
    ::GetWindowTextA(::GetForegroundWindow(), foregroundTitle.data(), foregroundTitle.size());
    //::OutputDebugStringA(foregroundTitle.c_str());

    return consoleTitle == foregroundTitle;

    //This code does not work on Windows Terminal
    //HWND consoleWindow = ::GetConsoleWindow();
    //HWND foregroundWindow = ::GetForegroundWindow();
    //if (consoleWindow == foregroundWindow) {
    //    ::OutputDebugStringA("Active\n");
    //}
    //else {
    //    ::OutputDebugStringA("Inactive\n");
    //}
    //return consoleWindow == foregroundWindow;
}


OptionalError Clear() 
{

    INPUT_RECORD records[2] = {};
    // Write Console 'ESC'

    records[0].EventType = KEY_EVENT;
    records[0].Event.KeyEvent = {};
    records[0].Event.KeyEvent.bKeyDown = TRUE;
    records[0].Event.KeyEvent.dwControlKeyState = 0;
    records[0].Event.KeyEvent.uChar.UnicodeChar = L'\0';
    records[0].Event.KeyEvent.wRepeatCount = 1;
    records[0].Event.KeyEvent.wVirtualKeyCode = VK_ESCAPE;
    records[0].Event.KeyEvent.wVirtualScanCode = 0;

    records[1].EventType = KEY_EVENT;
    records[1].Event.KeyEvent = {};
    records[1].Event.KeyEvent.bKeyDown = FALSE;
    records[1].Event.KeyEvent.dwControlKeyState = 0;
    records[1].Event.KeyEvent.uChar.UnicodeChar = L'\0';
    records[1].Event.KeyEvent.wRepeatCount = 1;
    records[1].Event.KeyEvent.wVirtualKeyCode = VK_ESCAPE;
    records[1].Event.KeyEvent.wVirtualScanCode = 0;

    
    DWORD written = 0;
    if (!::WriteConsoleInputA(stdinHandle, records, 2, &written)) {
        return Error(::GetLastError(), "Failed to WriteConsoleIput");
    }

    return std::nullopt;
}


OptionalError Send(const std::string& message)
{
    std::wstring wideMessage(message.size(), L'\0');
    if (0 == ::MultiByteToWideChar(CP_ACP, MB_COMPOSITE, message.data(), message.size(), wideMessage.data(), wideMessage.size())) {
        return Error(::GetLastError(), "Failed to MultiByteToWideChar");
    }

    std::vector<INPUT_RECORD> buf;
    buf.reserve(wideMessage.size());
    for (auto& each : wideMessage) {
        INPUT_RECORD record{};
        KEY_EVENT_RECORD keyEvent{};
        record.EventType = KEY_EVENT;
        record.Event.KeyEvent.bKeyDown = TRUE;
        record.Event.KeyEvent.dwControlKeyState = 0;
        record.Event.KeyEvent.uChar.UnicodeChar = each;
        record.Event.KeyEvent.wRepeatCount = 1;
        record.Event.KeyEvent.wVirtualKeyCode = 0;
        record.Event.KeyEvent.wVirtualScanCode = 0;

        buf.push_back(record);
        
    }
    DWORD written = 0;
    if (!::WriteConsoleInputA(stdinHandle, buf.data(), buf.size(), &written)) {
        return Error(::GetLastError(), "Failed to WriteConsoleIput");
    }
    return std::nullopt;
}



Result<std::ifstream> OpenHistoryFile() 
{
    std::string buf(4096, '\0');
    ::ExpandEnvironmentStringsA("%USERPROFILE%\\.cmd_history", buf.data(), buf.size());

    // open file
    std::ifstream fileStream(buf);
    if (!fileStream.is_open()) {
        return { std::nullopt, Error(2, "Failed to Open history file") };
    }
    return { std::move(fileStream), std::nullopt };
}


Result<std::ofstream> GetHistoryFile()
{
    std::string buf(4096, '\0');
    ::ExpandEnvironmentStringsA("%USERPROFILE%\\.cmd_history", buf.data(), buf.size());

    // open or create file
    std::ofstream fileStream(buf);
    if (!fileStream.is_open()) {
        return { std::nullopt, Error(2, "Failed to Open history file") };
    }
    return { std::move(fileStream), std::nullopt };
}


HANDLE OpenConsoleIn() 
{
    return ::CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
}


std::array<std::string, 2> NonRedirectableCommands{ "pause", "timeout" };
bool Redirectable(const std::string& command)
{
    bool quote = false;
    std::string verb;
    for (size_t i = 0; i < command.size(); i++) {
        if (command[i] == '\\') i++;
        if (command[i] == '\"') quote = !quote;
        if (command[i] == ' ' && !quote) break;
        verb += command[i];
    }
    for (auto& each : NonRedirectableCommands) {
        if (_stricmp(verb.c_str(), each.c_str()) == 0) {
            return false;
        }
    }
    return true;
}


void ShowPrompt()
{
    size_t size = ::GetCurrentDirectoryA(0, nullptr);
    std::string buf(size + 1, ' ');
    ::GetCurrentDirectoryA(buf.size(), buf.data());
    printf("%s>", buf.c_str());
}


COORD PromptPosition()
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!::GetConsoleScreenBufferInfo(::GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
        auto err = ::GetLastError();
        fprintf(stderr, "Failed to ::GetConsoleScreenBuffer: %d\n", err);
        return { 0, 0 };
    }
    return info.dwCursorPosition;
}


Result<std::string> ReadUncommitedInputs(COORD from)
{
    HANDLE handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!::GetConsoleScreenBufferInfo(handle, &info)) {
        auto err = ::GetLastError();
        return { std::nullopt, Error(err, "Failed to ::GetConsoleScreenBuffer") };
    }
    auto dx = info.dwCursorPosition.X - from.X;
    auto dy = info.dwCursorPosition.Y - from.Y;
    auto windowWidth = info.srWindow.Right - info.srWindow.Left + 1;
    size_t lengthToRead = dx + dy * windowWidth;
    std::string buf(lengthToRead, '\0');
    DWORD read = 0;
    if (!::ReadConsoleOutputCharacterA(handle, buf.data(), buf.size(), from, &read)) {
        auto err = ::GetLastError();
        return { std::nullopt, Error(err, "Failed to ::ReadConsoleOutputCharacterA") };
    }
    return { buf, std::nullopt };
}


void WaitOutputDone()
{
    COORD last{ 0, 0 };
    for (int i = 0; i < 10; i++){
        COORD current = PromptPosition();
        if (last.X == current.X && last.Y == current.Y)
        {
            break;
        }
        Sleep(25);
    }
}