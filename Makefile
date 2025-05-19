TARGET = bin/dbview
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))
CFLAGS ?= -std=c90

run: clean default
	./$(TARGET) -f ./mynewdb.db -n
	./$(TARGET) -f ./mynewdb.db -a "Timmy,123 Sheshire Ln.,120"
	./$(TARGET) -f ./mynewdb.db -a "Mats,46 Hallandsgatan,200"
	./$(TARGET) -f ./mynewdb.db -a "Tomten,Jukkasjärvi,400"
	./$(TARGET) -f ./mynewdb.db -l
	./$(TARGET) -f ./mynewdb.db -d "Mats"
	./$(TARGET) -f ./mynewdb.db -e "Tomten,Jukkasjärvi,4000"
	./$(TARGET) -f ./mynewdb.db -l

default: setup $(TARGET)

setup:
	mkdir -p bin obj

clean:
	rm -f obj/*.o
	rm -f bin/*
	rm -f *.db

$(TARGET): $(OBJ)
	gcc $(CFLAGS) -o $@ $?

obj/%.o : src/%.c
	gcc $(CFLAGS) -c $< -o $@ -Iinclude