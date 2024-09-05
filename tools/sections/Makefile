CC := cc
CFLAGS := -std=c11 -Wall -Wextra -Wshadow -O2
OUTPUT := ./sections

.PHONY: all clean

all: $(OUTPUT)

$(OUTPUT): main.c
	$(CC) $(CFLAGS) main.c -o $(OUTPUT)

clean:
	rm -f $(OUTPUT)
