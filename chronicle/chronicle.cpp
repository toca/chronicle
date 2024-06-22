#include <Windows.h>
#include <locale.h>

#include <format>
#include <memory>
#include <cstdio>
#include <iostream>
#include <thread>
#include <fstream>

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
std::optional<Error> Clear();
std::optional<Error> Send(const std::string& message);


Result<std::ifstream> OpenHistoryFile();
Result<std::ofstream> GetHistoryFile();

// TODO pause, timeout ... use system()

// M A I N
int main()
{
    // TODO
    // search histories
    // Send CTRL-C to child process
    // どうやって 子 cmd を終了するか exitじゃあだめ

    // SearchMode
    // なるほど。fish 検索中は選択とか出来ない
    // 現在の情報を取ってサイズを取得して
    // stop, search とか  enum でもいいか?

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
            case 'R':
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

        // こっちの CTRL-C をどうやって子cmdへ送るのか?
        ::SetConsoleCtrlHandler([](DWORD event) -> BOOL {
            if (event == CTRL_C_EVENT)
            {
                // TODO confirm?
                stop = true;
                cmd->Exit();
                searchView->Stop();
                printf("\nBreak.\n");
                ::CancelIoEx(consoleIn, &overLapped);
                return TRUE;
            }
            return FALSE;
            }, TRUE
        );


        // reading input loop ////////////////
        // TODO test when over length
        std::string buf(4096, '\0');
        while (!stop) {
            DWORD read = 0;
            buf.assign(buf.size(), '\0');
            // read input here
            // TODO func
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

            std::string line;
            auto pos = buf.find('\r');
            if (pos != std::string::npos) {
                line.assign(buf.begin(), buf.begin() + pos);
            }
            if (line.size()) {
                history->Add(line);
                cmd->Input(line + '\n');
            }
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
    auto s = history->Prev();
    if (s) {
        ::OutputDebugStringA(std::format("{}\n", *s).c_str());
        Clear();
        Send(*s);
    }
}


void Next() 
{
    auto s = history->Next();

    if (s) {
        ::OutputDebugStringA(std::format("{}\n", *s).c_str());
        Clear();
        Send(*s);
    }
}


bool AmIActive() 
{
    HWND consoleWindow = ::GetConsoleWindow();
    HWND foregroundWindow = ::GetForegroundWindow();
    return consoleWindow == foregroundWindow;
}


std::optional<Error> Clear() 
{
    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;
    input.ki.wVk = VK_ESCAPE;
    input.ki.dwFlags = 0;
    SendInput(1, &input, sizeof(INPUT));
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
    Sleep(1); // DO NOT RMOVE THIS!

    //printf("\x1b[0K");
    //printf("\x1b[2K"); // 行をクリア
    //printf("\x1b[0G"); // カーソルを行の先頭に移動

    //printf("\x1b\x1b");
    //printf("\x1b[31mHello\x1b[0m \x1b[32mworld\x1b[0m\n");
    //printf("\x1b[C");



    //if (!FlushConsoleInputBuffer(stdinHandle)) {
    //    return Error(::GetLastError(), "Failed to FlushConsoleInputBuffer");
    //}
    //return std::nullopt;
    //DWORD count = 0;
    //::GetNumberOfConsoleInputEvents(stdinHandle, &count);
    //if (count == 0) {
    //    OutputDebugStringA("clear on empty\n");
    //    return std::nullopt;
    //}

    //std::vector<BYTE> buf(1024);
    //DWORD read = 0;
    //if (!::ReadConsoleA(stdinHandle, buf.data(), buf.size(), &read, nullptr)) {
    //    return Error(::GetLastError(), "Failed to ReadConsole");
    //}
    return std::nullopt;
}


std::optional<Error> Send(const std::string& message)
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