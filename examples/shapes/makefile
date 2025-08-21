shapes: shapes.c shapes_parser.h
	cc shapes.c -o shapes -O3	

shapes_parser.h: shapes_parser_generator
	./shapes_parser_generator --single-file --header=shapes_parser.h

shapes_parser_generator: shapeschema.c ../../jipg.h
	cc shapeschema.c -o shapes_parser_generator
