gcc -o cdht cdht.c -std=c99 -lpthread
xterm -hold -title "Peer 1" -e "./cdht 1 4 7" &
xterm -hold -title "Peer 4" -e "./cdht 4 7 13" &
xterm -hold -title "Peer 7" -e "./cdht 7 13 15" &
xterm -hold -title "Peer 13" -e "./cdht 13 15 28" &
xterm -hold -title "Peer 15" -e "./cdht 15 28 127" &
xterm -hold -title "Peer 28" -e "./cdht 28 127 255" &
xterm -hold -title "Peer 127" -e "./cdht 127 255 1" &
xterm -hold -title "Peer 255" -e "./cdht 255 1 4" &
