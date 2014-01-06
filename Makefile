SRC = $(wildcard *.c)
OBJ = $(SRC:%.c=%.o)
OUT = witplayer
CC = gcc

CFLAGS = -I. -Wall `pkg-config gstreamer-0.10 --cflags`
LDFLAGS = -lpthread -lm -lpng -lasound `pkg-config gstreamer-0.10 --libs`

all: $(OUT)

$(OUT): $(SRC)
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	@rm -rfv *.o $(OUT)
