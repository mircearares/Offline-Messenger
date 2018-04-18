comp:
	g++ -pthread -o server.exe server.cpp -lsqlite3
	g++ client.cpp -o client.exe -lsqlite3
	gnome-terminal

client:
	./client.exe 127.0.0.1 2018

server:
	./server.exe