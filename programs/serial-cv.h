/* serial-cv.h - Serial-CV converter board interface library.
 * Written in 2012 by Nick Ames (nick@fetchmodus.org).
 * Placed in the Public Domain. */

/* The serial-cv converter board is a project I made to interface
 * a Korg Monotron (and other analog synthesizers) to a computer.
 * You can read about it at http://www.fetchmodus.org/projects/serialcv/
 * This library (serial-cv.h and serial-cv.c) provides a simple interface
 * for setting up the serial port and setting the output voltage.
 * It should work on all POSIX-compliant systems.
 */

/* Open the serial port and configure it.
 * path is the path to the serial port.
 * Vref is the reference voltage of the DAC, in volts.
 * Vref is usually 5V.
 * Returns 0 on success, -1 on error. */
int serialcv_init(const char *path, float Vref);

/* Set the output voltage.
 * voltage is the desired output voltage, in volts.
 * Returns 0 on success, -1 on error. */
int serialcv_voltage(float voltage);

/* Close the serial port. */
void serialcv_shutdown(void);