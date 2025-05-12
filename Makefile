CC := cc

build:
	$(CC) main.c -o bin/dvm -lSDL3 -lm

dvmc:
	$(CC) compiler/assembler.c -o bin/dvmc

examples:
	./bin/dvmc ./scripts/hello-world.dvm ./bin/hello-world

all: build dvmc dvmc examples