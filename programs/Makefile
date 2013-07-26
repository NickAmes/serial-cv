CC=/usr/bin/gcc
#CFLAGS=-O0 -Wall -Wextra -ansi -pedantic -ggdb
CFLAGS= -O2 -ansi -ggdb
LIBS=-lasound

all: serial-cv.o setvoltage.c monotron.c
	@$(CC) setvoltage.c serial-cv.o -o setvoltage $(CFLAGS) $(LIBS) 
	@$(CC) monotron.c serial-cv.o -o monotron $(CFLAGS) $(LIBS)
	
serial-cv.o: serial-cv.h serial-cv.c
	@$(CC) -c serial-cv.c $(CFLAGS) $(LIBS)
clean:
	@rm *.o
	@rm setvoltage
	@rm monotron
