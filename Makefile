CC ?= gcc
CFLAGS = -std=c99 -O3
LDFLAGS = -lxcb -lxcb-sync
SRC = main.c
OBJ = $(SRC:.c=.o)
TARGET = libnvglxfix.so

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -shared -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
