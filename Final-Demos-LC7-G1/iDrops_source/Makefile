CFLAGS += -std=c99 -pedantic -O2
SRC = ./src/main.c
EXTRAS = ./extra_files
LIBS =  -lglfw -lGL -lm -lGLU -lGLEW -lportaudio -lsndfile

.PHONY: drummerboy

drummerboy:
	mkdir -p bin
	rm -f bin/iDrop
	cp extra_files/* bin/
	$(CC) $(SRC) $(CFLAGS) -D_DEFAULT_SOURCE -o bin/iDrop $(LIBS)
