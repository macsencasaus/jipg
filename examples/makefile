example: jsonparser.c example.c
	cc jsonparser.c example.c -ggdb -o example

jsonparser.c: exampleparser
	./exampleparser

jsonparser.h: exampleparser
	./exampleparser

exampleparser: exampleparser.c ../jipg.h
	cc -ggdb -o exampleparser exampleparser.c
