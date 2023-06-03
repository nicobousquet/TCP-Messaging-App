all:
	cc -Wall -Wextra -g -std=gnu99 src/header.c src/packet.c src/client.c -o bin/client
	cc -Wall -Wextra -g -std=gnu99 src/header.c src/packet.c src/user_node.c src/chatroom.c src/server.c -o bin/server

clean:
	rm -rf bin/*


