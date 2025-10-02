CC = gcc
CFLAGS = -std=c11 -O2 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs`
SRC = src/game.c
BIN = pong

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $(BIN) $(SRC) $(LDFLAGS)

clean:
	rm -f $(BIN)
