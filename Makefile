build/dmxlive.audio: main.c build/duktape.o build/e131.o
	g++ -Wall -Wextra -g main.c build/e131.o build/duktape.o -o dmxlive -lm -std=c++11 -DENABLE_AUDIO -lopenal

build/dmxlive.noaudio: main.c build/duktape.o build/e131.o
	g++ -Wall -Wextra -g main.c build/e131.o build/duktape.o -o dmxlive -lm -std=c++11

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
