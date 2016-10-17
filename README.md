# WyldChess
### Overview
A free chess engine in C. It does not provide a GUI (Graphical User Interface)
but can be linked to one that supports either the UCI or the Xboard/Winboard protocol.
### Usage
#### GNU/Linux
WyldChess should easily compile from source with a simple `$ make`.
#### Windows
The 32-bit and 64-bit executables are available in the `binaries` folder. Alternatively, WyldChess
can be compiled using Cygwin or MinGW (Samples available in `makefile`).
### Files
* `binaries`: A subdirectory containing the Windows 32-bit and 64-bit executables.
* `src`: A subdirectory containing the source code of the program.
* `LICENSE`: A file containing a copy of the GNU Public License.
* `README.md`: The file you're currently reading.
* `makefile`: The file outlining the automated build process.
### Release System
A single(the latest) binary release for Windows will be available in the `binaries` subdirectory until the engine has achieved a strength of approximately 2400 ELO after which I will make the first official release and start recording version numbers for the following releases.
