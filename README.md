# Lazy Wyld

### Overview
The branch dedicated to the development of the Lazy SMP algorithm in WyldChess.

### Features

#### Evaluation
* Phased/Tapered evaluation
* Piece values
* Piece square tables (custom)
* King attack:
	* Glaurung 1.2 attack table
	* Queen next to king
	* King shelter

* Pawn structure:
	* Passed pawns
	* Doubled pawns
	* Isolated pawns

* Piece mobility
* Miscellaneous terms:
	* Dual bishops
	* Rook on open file
	* Rook on semi-open file
	* Rook on relative 7th rank
	* Knight and bishop outposts

* Basic endgame knowledge
    * "Can win?" scenarios(incomplete)

#### Search
* Lazy SMP
* Aspiration window search
* Pruning losing captures (negative SEE) in quiescence search if not in check
* Mate distance pruning
* Transposition table
* Syzygy EGTB (Adapted from the [Fathom tool](https://github.com/basil00/Fathom))
* Futility pruning
* Null move pruning
* Internal iterative deepening
* Move ordering:
    * Hash move
    * Good captures (positive SEE)
    * Killer moves
    * Counter moves
    * Other captures (negative and zero SEE)
    * History heuristic
* Check extension at frontier nodes and SEE >= 0 moves
* Late move reduction
* Principal variation search

### Files
* `src`      : A subdirectory containing the source code of the program and the makefile.
* `LICENSE`  : A file containing a copy of the GNU Public License.
* `README.md`: The file you're currently reading.
* `build.sh` : The file outlining the automated build process of the release.

### Binaries
All binaries are 64-bit

MacOS release is provided by Michael B

WyldChess 1.5 and beyond now have an Android/Rpi release!

#### WyldChess 1.5 and beyond

There are 3 types of binaries available for Windows and GNU/Linux each:

* `bmi`           : Compiled with the -mbmi -mbmi2 and -mpopcnt options.
* `popcnt`        : Compiled with the -mpopcnt option.
* `no extension`  : Basic compile.

#### WyldChess 1.3 and beyond

There are 2 types of binaries available for Windows and GNU/Linux each:

* `popcnt`        : Compiled with the -mpopcnt option.
* `no extension`  : Basic compile.

#### WyldChess 1.2 and before

There are 4 types of binaries available for Windows and GNU/Linux each:

* `fast_tc`       : Appropriate for less than 1-minute long games(Busy waits the engine thread).
* `popcnt`        : Compiled with the -mpopcnt option.
* `fast_tc_popcnt`: Includes both of the above.
* `no extension`  : Basic option, includes none of the above.
