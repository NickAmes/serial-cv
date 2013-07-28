/* serial-cv.h - Serial-CV converter board interface library.
 * http://www.fetchmodus.org/projects/serialcv/
 * Written in 2013 by Nick Ames <nick@fetchmodus.org>.
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
	cfsetospeed(&options, B9600);
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
	return 0;
}

/* Set the output voltage.
 * voltage is the desired output voltage, in volts.
 * If voltage is negative, the device will shutdown the DAC,
 * allowing it to be overridden by another controller.
 * Returns 0 on success, -1 on error. */
int serialcv_voltage(float voltage){
	char first, second; /* high and low output bytes */
	unsigned int fraction; /* Voltage fraction: 0 - 4095 */
	int r; /* Return value of write() */
	if(-1 == serialfd){
		fputs("serialcv_voltage: error: not initalized\n", stderr);
		return -1;
	}
	/* Each command consists of two bytes, containing the desired voltage word and
	 * a shutdown bit. If the shutdown bit is 1, the voltage word is ignored, and
	 * the DAC is put into a high-Z mode. When shutdown, the DAC can be overridden
	 * by another controller.
	 * Command Format (V# = voltage word bit, S = shutdown bit, X = don't care):
	 *        1st byte                 2nd byte
	 * MSB                    LSB     MSB                LSB
	 *  V11 V10 V9 V8 V7 V6 V5 1       X S V4 V3 V2 V1 V0 0              */
	if(voltage >= 0){
		if(voltage > vref)voltage = vref;
		fraction = (voltage * 4096)/vref;
		if(fraction >= 4096)fraction = 4095;
		first = 0x01 | (fraction >> 4);
		second = 0x3E & (fraction << 1);
	} else {
		/* Shutdown the DAC. */
		first = 0x01;
		second = 0x40;
	}
	if((r = write(serialfd, &first, 1)) != 1){
		if(0 == r){
			fputs("serialcv_voltage: an unknown error occured while"
			" writing to the serial port\n", stderr);
			return -1;
		} else {
			perror("serialcv_voltage");
		}
	}
	if((r = write(serialfd, &second, 1)) != 1){
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