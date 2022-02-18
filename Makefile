build/dmxlive: main.c build/duktape.o build/e131.o
	g++ main.c build/e131.o build/duktape.o -o dmxlive -lm

build/duktape.o: duktape/duktape.c
	mkdir -p build
	g++ -c duktape/duktape.c -o build/duktape.o

build/e131.o: libe131/e131.c
	mkdir -p build
	g++ -c libe131/e131.c -o build/e131.o

RunRelease: build/dmxlive
	./dmxlive

