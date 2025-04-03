CC ?= gcc
CFLAGS = -std=c99 -O3
LDFLAGS ?= -lxcb

SRC_FIX = main.c
OBJ_FIX = $(SRC_FIX:.c=.o)
TARGET_FIX = libnvglxfix.so

SRC_XSPAM = xspam.c
OBJ_XSPAM = $(SRC_XSPAM:.c=.o)
TARGET_XSPAM = xspam

all: $(TARGET_FIX) $(TARGET_XSPAM)

$(TARGET_FIX): $(OBJ_FIX)
	$(CC) -shared -o $(TARGET_FIX) $(OBJ_FIX) $(LDFLAGS) -lxcb-sync

$(TARGET_XSPAM): $(OBJ_XSPAM)
	$(CC) -o $(TARGET_XSPAM) $(OBJ_XSPAM) $(LDFLAGS) -lpthread

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

clean:
	rm -f $(OBJ_FIX) $(TARGET_FIX) $(OBJ_XSPAM) $(TARGET_XSPAM)
