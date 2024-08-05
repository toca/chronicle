#include <Windows.h>
#include <locale.h>

#include <format>
#include <memory>
#include <cstdio>
#include <iostream>
#include <thread>
#include <fstream>
#include <array>
#include <stdexcept>
#include <tuple>

#include "history.h"
#include "controller.h"
#include "searchcontroller.h"
#include "inputbuffer.h"
#include "mode.h"

#include "result.h"
#include "defer.h"



bool stop = false;
HANDLE stdinHandle = ::GetStdHandle(STD_INPUT_HANDLE);

Result<std::vector<INPUT_RECORD>> Read();

Result<std::wifstream> OpenHistoryFile();
Result<std::wofstream> GetHistoryFile();




// M A I N ////////////////
int main()
{
    // TODO
    // move history to ctrl and view
    // "/C" option
    try {
        // locale
        setlocale(LC_ALL, "");

        //DWORD handleCountBefore = 0;
        //::GetProcessHandleCount(::GetCurrentProcess(), &handleCountBefore);

        
        // history
        std::shared_ptr<History> history = std::make_shared<History>();
        auto [ihistoryFile, iresult] = OpenHistoryFile();
        if (ihistoryFile) {
            history->Load(*ihistoryFile);
            ihistoryFile->close();
        }
        // input buffer
        auto inputBuffer = std::make_shared<InputBuffer>();

        // main controller
        auto [mcRes, mcErr] = Controller::Create(inputBuffer, history);
        if (mcErr) {
            fwprintf(stderr, L"%s\n", mcErr->message.c_str());
            return mcErr->code;
        }
        std::unique_ptr<Controller> controller(*mcRes);

        // search controller
        auto [scRes, scErr] = SearchController::Create(inputBuffer, history);
        if (scErr) {
            fwprintf(stderr, L"%s\n", scErr->message.c_str());
            return scErr->code;
        }
        std::unique_ptr<SearchController> searchController(*scRes);


        // Mode changing event
        SetOnModeChanged([&searchController, &controller](Mode mode) 
            {
                controller->OnModeChanged(mode);
                searchController->OnModeChanged(mode);
            }
        );
        // Default mode
        SetMode(Mode::Main);


        // Handle CTRL + C
        ::SetConsoleCtrlHandler([](DWORD event) -> BOOL {
            //::OutputDebugStringA(std::format("e: %d", event).c_str());
            if (event == CTRL_C_EVENT)
            {
                stop = true;
                printf("\nCtrl+C\n");
                return TRUE;
            }
            else if (event == CTRL_CLOSE_EVENT) {
                printf("close");
            }
            return FALSE;
            }, TRUE
        );


        // Main loop ////////////////
        while (!stop) {
            // read input
            auto [inputs, readErr] = Read();
            if (readErr) {
                fwprintf(stderr, L"%s : %d\n", readErr->message.c_str(), readErr->code);
                return readErr->code;
            }
            switch (GetMode())
            {
            case Mode::Main:
                controller->Input(*inputs);
                controller->Render();
                break;
            case Mode::Search:
                searchController->Input(*inputs);
                searchController->Render();
                break;
            }

            Sleep(25);
            continue;
        }

        // TODO save histories immediately
        // save histories
        auto [ohistoryFile, oresult] = GetHistoryFile();
        if (ohistoryFile) {
            history->Dump(*ohistoryFile);
            ohistoryFile->close();
        }

        printf("Bye.\n");

    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }
    catch (...) {
        return 1;
    }

    return 0;
}


Result<std::wifstream> OpenHistoryFile() 
{
    std::string buf(4096, '\0');
    ::ExpandEnvironmentStringsA("%USERPROFILE%\\.cmd_history", buf.data(), buf.size());

    // open file
    std::wifstream fileStream(buf);
    if (!fileStream.is_open()) {
        return { std::nullopt, Error(2, L"Failed to Open history file") };
    }
    return { std::move(fileStream), std::nullopt };
}


Result<std::wofstream> GetHistoryFile()
{
    std::string buf(4096, '\0');
    ::ExpandEnvironmentStringsA("%USERPROFILE%\\.cmd_history", buf.data(), buf.size());

    // open or create file
    std::wofstream fileStream(buf);
    if (!fileStream.is_open()) {
        return { std::nullopt, Error(2, L"Failed to Open history file") };
    }
    return { std::move(fileStream), std::nullopt };
}


Result<std::vector<INPUT_RECORD>> Read()
{
    DWORD count = 0;
    if (!::GetNumberOfConsoleInputEvents(stdinHandle, &count)) {
        return { {}, Error(::GetLastError(), L"Failed to ::GetNumberOfConsoleInputEvents") };
    }
    if (!count) {
        return { std::vector<INPUT_RECORD>{}, std::nullopt };
    }
    std::vector<INPUT_RECORD> inputs(count);
    DWORD len = DWORD(inputs.size());
    DWORD numOfEvents = 0;

    if (!::ReadConsoleInputW(stdinHandle, inputs.data(), len, &numOfEvents)) {
        return { {}, Error(::GetLastError(), L"Failed to ::ReadConsoleInput") };
    }
    return { inputs, std::nullopt };
}
