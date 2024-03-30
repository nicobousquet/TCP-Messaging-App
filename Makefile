CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -g -std=gnu99
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
INBOX_DIR = inbox

CLIENT_SRC = $(wildcard $(SRC_DIR)/client/*.c)
SERVER_SRC = $(wildcard $(SRC_DIR)/server/*.c)
PACKET_SRC = $(wildcard $(SRC_DIR)/packet/*.c)

CLIENT_OBJ = $(patsubst $(SRC_DIR)/client/%.c,$(OBJ_DIR)/client/%.o,$(CLIENT_SRC))
SERVER_OBJ = $(patsubst $(SRC_DIR)/server/%.c,$(OBJ_DIR)/server/%.o,$(SERVER_SRC))
PACKET_OBJ = $(patsubst $(SRC_DIR)/packet/%.c,$(OBJ_DIR)/packet/%.o,$(PACKET_SRC))

all: $(BIN_DIR) $(BIN_DIR)/client $(BIN_DIR)/server

$(BIN_DIR)/client: $(CLIENT_OBJ) $(PACKET_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(BIN_DIR)/server: $(SERVER_OBJ) $(PACKET_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/client/%.o: $(SRC_DIR)/client/%.c | $(OBJ_DIR)/client
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/server/%.o: $(SRC_DIR)/server/%.c | $(OBJ_DIR)/server
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/packet/%.o: $(SRC_DIR)/packet/%.c $(SRC_DIR)/packet/%.c | $(OBJ_DIR)/packet
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/client $(OBJ_DIR)/server $(OBJ_DIR)/packet $(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(INBOX_DIR)

.PHONY: all clean

