# WyldChess
### Overview
A free chess engine in C. It does not provide a GUI (Graphical User Interface)
but can be linked to one that supports either the UCI or the Xboard/Winboard protocol.

### Files
* `src`: A subdirectory containing the source code of the program.
* `LICENSE`: A file containing a copy of the GNU Public License.
* `README.md`: The file you're currently reading.
* `makefile`: The file outlining the automated build process.

### Binaries
There are 4 types of binaries available for Windows and GNU/Linux each:
* `fast_tc`: Appropriate for less than 1-minute long games(Busy waits the engine thread).
* `popcnt`: Compiled with the -mpopcnt option.
* `popcnt_fast_tc`: Includes both of the above.
* `No extension`: Includes none of the above.
