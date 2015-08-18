BIN = .
INCLUDE = include

# Entrada
LIBS = lib/ieee_32m.lib lib/gpib-32.obj -lkernel32 -lm
SOURCES = src/main.c
# Nombre del programa compilado
TARGET = programa

$(BIN)/$(TARGET): $(SOURCES)
	gcc -pipe -O2 -Wall -Wextra -I$(INCLUDE) -o $@ $^ $(LIBS)
