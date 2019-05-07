dep = $(shell pkg-config --libs libcap)

build:
	gcc src/misc.c src/traceroute.c $(dep) -o traceroute 
