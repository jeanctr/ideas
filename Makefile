CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -lsqlite3
TARGET = ideas
SOURCES = main.c db.c commands.c colors.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SOURCES) db.h commands.h colors.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
	rm -f $(TARGET) ideas_export.*

test: all
	bash tests/test.sh
