# Chronicle
This program extends the history of the Windows command prompt.

## Usage
* Start: Launch from the command prompt with `chronicle.exe`.
* Use the ↑ key and ↓ key to display the history.
* Switch to history search mode with Ctrl + R.
* Switch back to normal mode from search mode with the ESC key.
* Exit with Ctrl + C.

## Details
* The history data is saved to `%USERPROFILE%\.cmd_history` when the program exits.
* The program launches cmd.exe internally, and commands are primarily processed in that process.
* Commands that do not support redirection are processed using the `system` function.
