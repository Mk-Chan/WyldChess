#!/bin/bash
# Extract games in which white wins
pgn-extract -s -D -pl40 -Tr1-0 -Wepd games.pgn | sed 's/c0.*$/0 1 1/p' > epd_list.epd
# Extract games in which black wins
pgn-extract -s -D -pl40 -Tr0-1 -Wepd games.pgn | sed 's/c0.*$/0 1 0/p' >> epd_list.epd
# Extract drawn games
pgn-extract -s -D -pl40 -Tr1/2-1/2 -Wepd games.pgn | sed 's/c0.*$/0 1 0.5/p' >> epd_list.epd
# Delete first 20 lines(10 moves of first game)
sed -i '1, 20d' epd_list.epd
# Delete 20 lines(10 moves) after each empty line
sed -i '/^$/,+20d' epd_list.epd
# Delete matching lines
sed -i '1~2d' epd_list.epd
