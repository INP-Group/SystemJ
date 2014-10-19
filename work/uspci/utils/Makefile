#

all:		uspci_test

uspci_test:	uspci_test.o
		gcc -o $@ $<
		
uspci_test.o:	uspci_test.c ../include/uspci.h
		gcc -c -W -Wall -I../include -o $@ $<

clean:
		rm -f *.o uspci_test *~
