/* Firmware for Serial-CV converter board.
 * http://www.fetchmodus.org/projects/serialcv/
 * Written in 2013 by Nick Ames <nick@fetchmodus.org>.
 * Placed in the Public Domain. */
#include <avr/interrupt.h>
#include <stdbool.h>
#define F_CPU 8000000UL
#include <util/delay.h>
#include <stdint.h>

/* This program receives commands over RS-232 (9600 baud, 8N1) on pin
 * PB3. (As the attiny25 lacks a UART, this is done in software.)
 * It communicates with an MCP4716/4726 DAC over I2C on pins
 * PB2 (SCL) and PB0 (SDA). This is also performed in software. */

/* Each command consists of two bytes, containing the desired voltage word and
 * a shutdown bit. If the shutdown bit is 1, the voltage word is ignored, and
 * the DAC is put into a high-Z mode. When shutdown, the DAC can be overridden
 * by another controller.
 * Command Format (V# = voltage word bit, S = shutdown bit, X = don't care):
 *        1st byte                 2nd byte
 * MSB                    LSB     MSB                LSB
 *  V11 V10 V9 V8 V7 V6 V5 1       X S V4 V3 V2 V1 V0 0              */

/* MCP4716/4726 I2C Address */
#define I2C_ADDR 0b1100000


/* Set the DAC output voltage.
 * This re-enables the DAC after shutdown. */
void voltage(uint16_t voltage_word);

/* Shutdown the DAC, putting its output in a Hi-Z mode. */
void shutdown_dac(void);

/* Receive a byte over the RS-232 connection.
 * Blocks until a byte has been received. */
uint8_t get_byte(void);

int main(void){
	/* Setup pins. */
	PORTB = 1 & ~_BV(PB0) & ~_BV(PB2);
	/* Setup timer and interrupts for RS-232 */
	GIMSK |= _BV(PCIE); /* Enable Pin Change Interrupt. */
	PCMSK =  _BV(PCINT3); /* Enable PCINT3. */
	GTCCR |= _BV(TSM) | _BV(PSR0); /* Stop timer 0. */
	TCNT0 = 0; /* Reset timer 0. */
	TCCR0B |= _BV(CS01); /* Set timer 0 clock to 1Mhz (system clock/8). */
	TIMSK |= _BV(OCIE0A); /* Enable timer interrupts. */
	sei(); /* Enable interrupts. */

	uint8_t first, second;
	uint16_t dac_word;
	shutdown_dac();
	first = get_byte();
	while(1){
		if(first & 0x01){
			second = get_byte();
			if(second & 0x01){
				first = second;
				continue;
			} else {
				if(second & 0x40){
					shutdown_dac();
				} else {
					dac_word = (second & 0x3E) >> 1;
					dac_word |= (first & 0xFE) << 4;
					voltage(dac_word);
				}
			}
		}
		first = get_byte();
	}

	return 0;
}

#define SDA_H DDRB &= ~_BV(PB0)
#define SDA_L DDRB |= _BV(PB0)
#define SCL_H DDRB &= ~_BV(PB2)
#define SCL_L DDRB |= _BV(PB2)
#define I2C_DELAY 2 /* Half of SCL period, in us. */

/* Send a byte over I2C. SCL should be low before calling this function.
 * The MSB is the first bit to be transmitted. */
void send_byte(uint8_t byte){
	uint8_t i;
	for(i=0;i<8;i++){
		_delay_us(I2C_DELAY);
		if(0x80 & byte){
			SDA_H;
		} else {
			SDA_L;
		}
		byte <<= 1;
		SCL_H;
		_delay_us(I2C_DELAY);
		SCL_L;
	}
	_delay_us(I2C_DELAY);
	SCL_H; /* ACK bit. */
	_delay_us(I2C_DELAY);
	SCL_L;
}

/* Send a two-byte command to the DAC, with the R/W bit set to write. */
void send_cmd(uint8_t first, uint8_t second){
	SCL_H;
	_delay_us(I2C_DELAY);
	SDA_H;
	_delay_us(I2C_DELAY);
	/* Start condition: */
	SDA_L;
	_delay_us(I2C_DELAY);
	SCL_L;
	send_byte(I2C_ADDR << 1);
	send_byte(first);
	send_byte(second);
	/* Stop condition: */
	SCL_H;
	_delay_us(I2C_DELAY);
	SDA_H;
	_delay_us(I2C_DELAY);
}

/* Set the DAC output voltage.
 * This re-enables the DAC after shutdown. */
void voltage(uint16_t voltage_word){
	send_cmd(voltage_word >> 8, 0xFF & voltage_word);
}

/* Shutdown the DAC, putting its output in a Hi-Z mode. */
void shutdown_dac(void){
	send_cmd(0x30, 0);
}

/* Received byte. */
volatile uint8_t rs232_byte;

/* If 1, a new byte has been received. */
volatile uint8_t rs232_byte_ready;

/* Receive a byte over the RS-232 connection.
 * Blocks until a byte has been received. */
uint8_t get_byte(void){
	while(!rs232_byte_ready){
		/* Wait for a new byte to arrive. */
	}
	rs232_byte_ready = 0;
	return rs232_byte;
}

/* Buffer for received byte. */
volatile uint8_t rs232_buffer;

/* Bit (0-7) currently being received. */
volatile uint8_t rs232_bit;

/* Adjust the bit periods (or calibrate the RC oscillator) to get good reception.
 * These period are for a clock frequency of 8.07 Mhz.
 * The CLKOUT fuse bit is helpful in determining your AVR's clock. */

/* Time between bit samples, in uS.  */
#define rs232_period 96

/* 1.5 * rs232_period. */
#define rs232_period_and_half 146

ISR(PCINT0_vect){
	if(!(PINB & _BV(PB3))){ /* Falling edge */
		OCR0A = rs232_period_and_half;
		GTCCR = 0; /* Start timer 0. */
		GIMSK &= ~_BV(PCIE); /* Disable PCINT. */
	}
}

ISR(TIM0_COMPA_vect){
	rs232_buffer |= (((PINB & _BV(PB3)) != 0)) << rs232_bit;
	GTCCR |= _BV(TSM) | _BV(PSR0); /* Stop timer 0. */
	TCNT0 = 0; /* Reset timer 0. */
	GTCCR = 0; /* Start timer 0. */
	OCR0A = rs232_period;
	rs232_bit++;
	if(9 == rs232_bit){
		rs232_byte = rs232_buffer;
		rs232_byte_ready = 1;
		rs232_buffer = 0;
		rs232_bit = 0;
		GTCCR |= _BV(TSM) | _BV(PSR0); /* Stop timer 0. */
		TCNT0 = 0; /* Reset timer 0. */
		GIMSK |= _BV(PCIE); /* Enable PCINT. */
	}
}
