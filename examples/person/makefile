person: person_parser.c person_parser.h person.c
	cc person_parser.c person.c -o person -O3

person_parser.c person_parser.h: person_parser_generator
	./person_parser_generator --source=person_parser.c --header=person_parser.h

person_parser_generator: personschema.c ../../jipg.h
	cc -o person_parser_generator personschema.c

clean:
	rm -f person person_parser*
.PHONY: clean
