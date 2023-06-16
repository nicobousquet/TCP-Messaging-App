all:
	cc -Wall -Wextra -g -std=gnu99 src/packet/header.c src/packet/packet.c src/client/peer.c src/client/client.c src/client/main.c -o bin/client
	cc -Wall -Wextra -g -std=gnu99 src/packet/header.c src/packet/packet.c src/server/user_node.c src/server/chatroom.c src/server/server.c src/server/main.c -o bin/server

clean:
	rm -rf bin/*


