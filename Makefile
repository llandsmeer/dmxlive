all: dmxlive.noaudio dmxlive

dmxlive: $(wildcard src/*) build/duktape.o build/e131.o
	g++ -Wall -Wextra -g src/main.c build/e131.o build/duktape.o -o dmxlive -lm -std=c++11 -DENABLE_AUDIO -lopenal -I . -O2

dmxlive.noaudio: $(wildcard src/*) build/duktape.o build/e131.o
	g++ -Wall -Wextra -g src/main.c build/e131.o build/duktape.o -o dmxlive.noaudio -lm -std=c++11 -I . -O2

build/duktape.o: duktape/duktape.c
	mkdir -p build
	g++ -g -c duktape/duktape.c -o build/duktape.o

build/e131.o: libe131/e131.c
	mkdir -p build
	g++ -g -c libe131/e131.c -o build/e131.o

RunRelease: build/dmxlive.audio
	./dmxlive

clean:
	rm build -fr
	rm dmxlive
