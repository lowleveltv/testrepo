CC = clang
CFLAGS = -Iinclude -O2 -Wall -Wextra

TARGET = bin/dbview

SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

default: $(TARGET)

debug: CFLAGS := $(CFLAGS) -Og -g -D DEBUG
debug: default

clean:
	rm -f obj/*.o
	rm -f bin/*

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $?

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@
