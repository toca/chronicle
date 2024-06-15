// readandwrite.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <Windows.h>
#include <cstdio>
#include <iostream>
#include <vector>


bool stop = false;

int main()
{
    setlocale(LC_ALL, "");

    ::SetConsoleCtrlHandler([](DWORD event) -> BOOL {
        if (event == CTRL_C_EVENT)
        {
            stop = true;
            printf("\nBreak.\n");
            return TRUE;
        }
        return FALSE;
        }, TRUE
    );

    while (!stop) {
        DWORD count = 0;
        HANDLE handle = ::GetStdHandle(STD_INPUT_HANDLE);
        ::GetNumberOfConsoleInputEvents(handle, &count);
        if (!count)
        {
            continue;
        }

        std::vector<INPUT_RECORD> inputs(4);
        DWORD len = DWORD(inputs.size());
        DWORD numOfEvents = 0;

        ::ReadConsoleInput(handle, inputs.data(), len, &numOfEvents);

        for (size_t i = 0; i < numOfEvents; i++)
        {
            if (inputs[i].EventType == KEY_EVENT)
            {
                 if (inputs[i].Event.KeyEvent.bKeyDown) {
                     if (inputs[i].Event.KeyEvent.uChar.UnicodeChar == '\0') {
                         printf("0x%x ", inputs[i].Event.KeyEvent.wVirtualKeyCode);
                     }
                     else {
                         wprintf(L"%lc ", inputs[i].Event.KeyEvent.uChar.UnicodeChar);
                         //printf("%c ", inputs[i].Event.KeyEvent.uChar.AsciiChar);
                     }

                }
            }

            //if (inputs[i].EventType == WINDOW_BUFFER_SIZE_EVENT)
            //{
            //	auto newSize = inputs[i].Event.WindowBufferSizeEvent.dwSize;
            //	::OutputDebugString(format(L"newSize: %d, %d\n", newSize.X, newSize.Y).c_str());
            //	if (this->windowSize.X != newSize.X || this->windowSize.Y != newSize.Y)
            //	{
            //		//this->OnSizeChanged(newSize);
            //		//result.HasSizeEvent = true;
            //		//result.NewSize = newSize;
            //	}
            //}
        }
    }
    std::cout << "Hello World!\n";
    // read input て全部来るのバックスペースとかカーソルとか
}

