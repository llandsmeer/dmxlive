all:
	g++ ./main.c ./libe131/e131.c -o dmxlive

RunRelease: all
	./dmxlive

