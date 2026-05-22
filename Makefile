# MMUKO OS Makefile
# github.com/obinexus/mmuko-os

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -lm -DMMUKO_SCHEDULER_TEST
TARGET = mmuko-os

SRCS = mmuko.c mmuko_scheduler.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c mmuko.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

test: $(TARGET)
	./$(TARGET)
