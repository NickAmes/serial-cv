/* serial-cv.h - Serial-CV converter board interface library.
 * Written in 2012 by Nick Ames (nick@fetchmodus.org).
 * Placed in the Public Domain. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "serial-cv.h"

/* The serial-cv converter board is a project I made to interface
 * a Korg Monotron (and other analog synthesizers) to a computer.
 * You can read about it at http://www.fetchmodus.org/projects/serialcv/
 * This library (serial-cv.h and serial-cv.c) provides a simple interface
 * for setting up the serial port and setting the output voltage.
 * It should work on all POSIX-compliant systems.
 */

/* Serial port file descriptor. */
static int serialfd = -1;

/* DAC reference voltage */
static int vref = 5;

/* Open the serial port and configure it.
 * path is the path to the serial port.
 * Vref is the reference voltage of the DAC, in volts.
 * Vref is usually 5V.
 * Returns 0 on success, -1 on error. */
int serialcv_init(const char *path, float Vref){
	struct termios options; /* Serial port options */
	int r; /* Return value of write() */
	if(Vref < 0)Vref = 0;
	vref = Vref;
	serialfd = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
	if(-1 == serialfd){
		fprintf(stderr, "serialcv_init: %s: ", path);
		perror(NULL);
		return -1;
	}
	/* Setup the serial port to transmit at 9600 baud 8N1 */
	if(tcgetattr(serialfd, &options)){
		fprintf(stderr, "serialcv_init: %s: ", path);
		perror(NULL);
		close(serialfd);
		serialfd = -1;
		return -1;
	}
	cfsetospeed(&options, 9600);
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_oflag &= ~OPOST;
	if(tcsetattr(serialfd, TCSANOW, &options)){
		fprintf(stderr, "serialcv_init: %s: ", path);
		perror(NULL);
		close(serialfd);
		serialfd = -1;
		return -1;
	}
	/* For some reason the AVR ignores the first byte sent to it.
	 * This "primes the pump" so the user's requests take effect. */
	if((r = write(serialfd, "\0\0", 2)) != 2){
		if(0 == r){
			fputs("serialcv_init: an unknown error occured while"
			      " writing to the serial port\n", stderr);
			return -1;
		} else {
			perror("serialcv_init");
		}
	}
	return 0;
}

/* Set the output voltage.
 * voltage is the desired output voltage, in volts.
 * Returns 0 on success, -1 on error. */
int serialcv_voltage(float voltage){
	char high, low; /* high and low output bytes */
	int fraction; /* Voltage fraction: 0 - 4095 */
	int r; /* Return value of write() */
	if(-1 == serialfd){
		fputs("serialcv_voltage: error: not initalized\n", stderr);
		return -1;
	}
	if(voltage < 0)voltage = 0;
	if(voltage > vref)voltage = vref;
	fraction = (voltage * 4096)/vref;
	high = 0x80 | (fraction >> 5);
	low = fraction & 0x1F;
	if((r = write(serialfd, &high, 1)) != 1){
		if(0 == r){
			fputs("serialcv_voltage: an unknown error occured while"
			      " writing to the serial port\n", stderr);
			return -1;
		} else {
			perror("serialcv_voltage");
		}
	}
	if((r = write(serialfd, &low, 1)) != 1){
		if(0 == r){
			fputs("serialcv_voltage: an unknown error occured while"
			      " writing to the serial port\n", stderr);
			return -1;
		} else {
			perror("serialcv_voltage");
		}
	}
	return 0;
}

/* Close the serial port. */
void serialcv_shutdown(void){
	close(serialfd);
	serialfd = -1;
}