# Chronicle
This program extends the history of the Windows command prompt.
As a result, this program functions as a simple command-line shell. It independently handles user inputs such as text and cursor movements, as well as some internal commands.
It can also serve as a sample for implementing a custom shell on Windows.
## Usage
* Start: Launch from the command prompt with `chronicle.exe`.
* Use the ↑ key and ↓ key to display the history.
* Switch to history search mode with Ctrl + R.
* Switch back to normal mode from search mode with the ESC key.
* Exit with Ctrl + C.

## Details
* The history data is saved to `%USERPROFILE%\.cmd_history`.
* Ctrl - b ... back
* Ctrl - f ... forward
* Ctrl - a ... ahead
* Ctrl - e ... end
* Ctrl - h ... backspace 
* Ctrl - d ... delete
