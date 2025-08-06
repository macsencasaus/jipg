jsonparser.h: example
	./example

example: example.c jipg.h
	gcc -ggdb -o example example.c
