all:
	cc -Wall -Wextra -g -std=gnu99 src/common.c src/client.c -o bin/client
	cc -Wall -Wextra -g -std=gnu99 src/common.c src/server.c -o bin/server

clean:
	rm -rf bin/*

zip:
	zip -r jalon4.zip src/client.c src/server.c src/common.c include/client.h include/server.h include/common.h Makefile


