# P2P_DHT
This is a P2P Distributed Hash Table written with Linux C.<br>
This project is a peer-to-peer systems, also a circular DHT, containing multi-threading, ping, hash function and socket programming for both UDP and TCP protocols.<br>
Every peer in the circle has a unique number, and it has a successor and a precessor. At the start of the program, every peer will ping its successor and precessor, and it will print the message whether it sends a ping request or receives a ping result.<br>
Every peer contains files which is calculated by a hash function. A peer will send a message to its successor if it need a file, and the message will transfer in the circle if its successor doesnot have the file until the file is found or the message reaches its origin peer.<br>
If a peer quit from the system, it will send messages to its successor and precessor to update the DHT.<br>
How to run the program?<br>
run the command: sh setup.sh<br>
The sh file contians the peer and its two successors, and it will know its precessors after the ping function. Every peer is activated in a single xwindow, and you can input commands such as:<br>
1. "request 2016" (find the file 2016)<br>
2. "quit" (the peer will quit from the DHT)
