
CC=g++
CFLAGS+=-g -std=c++11
LDFLAGS+= -llua5.1

BIN=./
OBJ=./obj
SRC=./src
INC=./inc

target=a
depend= $(OBJ)/terror.o $(OBJ)/matrix.o #$(OBJ)/sps.o $(OBJ)/pps.o $(OBJ)/nal.o $(OBJ)/slice.o $(OBJ)/picture.o $(OBJ)/pixmap.o

$(BIN)/$(target):main.cpp $(depend)
	$(CC) -I $(INC)  $(CFLAGS) $^ -o $@ $(LDFLAGS) 
$(OBJ)/%.o:$(SRC)/%.cpp
	$(CC) -c $(CFLAGS) -I $(INC) $^    -o  $@
clean:
	@if [ -d $(OBJ) ]; then rm $(OBJ)/*.o; fi;
	@rm $(BIN)/$(target)
