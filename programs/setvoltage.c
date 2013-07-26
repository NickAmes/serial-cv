/* setvoltage.c - Send a voltage to a serial-cv converter board.
 * http://www.fetchmodus.org/projects/serialcv/
 * Written in 2013 by Nick Ames <nick@fetchmodus.org>.
 * Placed in the Public Domain. */
#include <stdio.h>
#include <stdlib.h>
#include "serial-cv.h"

/* Reference voltage of DAC */
#define VREF 5.0

char *usage = "Usage:\n"
              "  setvoltage <serial port> <voltage>\n";

int main(int argc, char *argv[]){
	double voltage;
	if(argc != 3){
		fputs(usage, stderr);
		return -1;
	}
	if(serialcv_init(argv[1], VREF)){
		fprintf(stderr, "setvoltage: could not open serial port\n");
		return -2;
	}
	voltage = atof(argv[2]);
	if(voltage < 0)voltage = 0;
	if(voltage > VREF)voltage = VREF;
	serialcv_voltage(voltage);
	serialcv_shutdown();
	return 0;
}

/* Print the usage message to stderr. */
void print_usage(void){
	fprintf(stderr, "%s", usage);
}
