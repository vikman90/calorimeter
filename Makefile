BIN = .
SRC = src
INC = include

# Input

LIBS = lib/ieee_32m.lib lib/gpib-32.obj -lkernel32 -lm
SOURCES = src/main.c

# Target programs

TARGET = $(BIN)/calorimeter $(BIN)/calorimeter_nc

.PHONY: all clean

all: $(TARGET)

$(BIN)/calorimeter: $(SRC)/main.c $(SRC)/calorimeter.c
	gcc -pipe -O2 -Wall -Wextra -I$(INC) -o $@ $^ $(LIBS)

$(BIN)/calorimeter_nc: $(SRC)/main_nc.c $(SRC)/calorimeter.c
	gcc -pipe -O2 -Wall -Wextra -I$(INC) -o $@ $^ $(LIBS)

clean:
	del $(TARGET)

$(SRC)/main.c: $(INC)/calorimeter.h
$(SRC)/main_nc.c: $(INC)/calorimeter.h
$(SRC)/calorimeter.c: $(INC)/calorimeter.h
