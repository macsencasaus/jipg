FLAGS := -Wall -Wextra -Wpedantic -Wconversion

shapes: shapes.cpp shapes_parser.h
	c++ $(FLAGS) shapes.cpp -o shapes -O3

shapes_parser.h: shapes_parser_generator
	./shapes_parser_generator

shapes_parser_generator: shapeschema.c ../../jipg.h
	cc $(FLAGS) shapeschema.c -o shapes_parser_generator

clean:
	rm shapes shapes_parser*
.PHONY: clean
