
CC=g++
CFLAGS+=-g -std=c++11 
LDFLAGS+= -llua5.1 -lSDL #`pkg-config --cflags --libs opencv4`

BIN=./
OBJ=./obj
SRC=./src
INC=./inc

target=a

depend= $(OBJ)/terror.o\
$(OBJ)/reader.o  \
$(OBJ)/decoder.o $(OBJ)/parser.o\
$(OBJ)/pps.o $(OBJ)/sps.o\
$(OBJ)/nal.o $(OBJ)/slice.o $(OBJ)/picture.o\
$(OBJ)/macroblock.o $(OBJ)/residual.o\
$(OBJ)/cabac.o $(OBJ)/matrix.o $(OBJ)/pixmap.o\
$(OBJ)/gfunc.o $(OBJ)/sei.o\
$(OBJ)/intra4x4.o $(OBJ)/intra8x8.o $(OBJ)/intra16x16.o $(OBJ)/inter.o\
$(OBJ)/block.o $(OBJ)/neighbour.o

$(BIN)/$(target):main.cpp $(depend)
	$(CC) -I $(INC)  $(CFLAGS) $^ -o $@ $(LDFLAGS) 
$(OBJ)/%.o:$(SRC)/%.cpp
	$(CC) -c $(CFLAGS) -I $(INC) $^    -o  $@
clean:
	@if [ -d $(OBJ) ]; then rm $(OBJ)/*.o; fi;
	@rm $(BIN)/$(target)
analy:
	$(CC) teana.c -o teana
viewr:
	$(CC) teview.cpp -o view `pkg-config --cflags --libs opencv4`