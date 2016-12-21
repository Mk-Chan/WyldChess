# WyldChess
## Latest Release
Get the latest release here: https://github.com/Mk-Chan/WyldChess/releases/latest

### Overview
A free chess engine in C. It does not provide a GUI (Graphical User Interface)
but can be linked to one that supports either the UCI or the Xboard/Winboard protocol.

### Features
#### Evaluation
* Phased/Tapered evaluation
* Piece values
* Piece square tables(custom)
* King attack:
	* Glaurung 1.2 attack table
	* Pawn cover

* Pawn structure:
	* Passed pawns
	* Doubled pawns
	* Isolated pawns

* Piece mobility
* Miscellaneous terms:
	* Blocked bishop
	* Dual bishops
	* Rook on open file
	* Rook on semi-open file
	* Knight and bishop outposts
	* Protected outpost

#### Search
* Aspiration window search
* Pruning losing captures (negative SEE) in quiescence search
* Mate distance pruning
* Transposition table
* Futility pruning
* Null move pruning
* Internal iterative deepening
* Move ordering:
    * Hash move
    * Good captures (positive SEE)
    * Queen promotions
    * Equal captures (~0 SEE)
    * Killer move from current ply
    * Killer moves from 2 plies ago
    * Minor promotions
    * Bad captures
    * Passed pawn push
    * Castling
    * Rest by piece square table (to - from value difference)
* Check extension at frontier nodes and positive SEE moves
* Late move reduction:
* Principal variation search

### Files
* `src`      : A subdirectory containing the source code of the program.
* `LICENSE`  : A file containing a copy of the GNU Public License.
* `README.md`: The file you're currently reading.
* `build.sh` : The file outlining the automated build process of the release.
* `makefile` : The file outlining the automated compile process.
* `notes`    : Some notes.

### Binaries
All binaries are x64 bit

There are 4 types of binaries available for Windows and GNU/Linux each:

* `fast_tc`       : Appropriate for less than 1-minute long games(Busy waits the engine thread).
* `popcnt`        : Compiled with the -mpopcnt option.
* `fast_tc_popcnt`: Includes both of the above.
* `no extension`  : Basic option, includes none of the above.
