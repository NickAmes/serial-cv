/* monotron.c - Receives MIDI events and uses them to control a Korg Monotron.
 * http://www.fetchmodus.org/projects/serialcv/
 * Written in 2013 by Nick Ames <nick@fetchmodus.org>.
 * Placed in the Public Domain. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <alsa/asoundlib.h>
#include "serial-cv.h"

char *usage = "Usage:\n"
              "  monotron <serial port>\n"
              "Edit the source file (monotron.c) to change MIDI channel,"
              "voltages, etc.\n";

/* Used by SIGTERM  and SIGINT signal handler to tell
 * the main loop to shutdown. */
int gotsignal = 0;

/* DAC reference voltage, in volts */
#define VREF 5.0
/* MIDI channel to listen on (0-15) */
#define MIDI_CH 0
/* Name of port in ALSA seq interface. */
#define SEQ_NAME "Korg Monotron"

/* This program assumes that the synthesizer uses an exponential control
 * voltage (resulting in a *linear* relationship
 * between MIDI note number and voltage). */

/* The lowest MIDI note that the synthesizer can produce;
 * lower notes will be octave-shifted into range. */
#define LOW_MIDI 58
/* Output voltage of the lowest note that the synthesizer can produce */
#define LOW_VOLTS 1.56
/* The highest MIDI note that the synthesizer can produce;
 * higher notes will be octave-shifted into range. */
#define HIGH_MIDI 72
/* Output voltage of the highest note that the synthesizer can produce. */
#define HIGH_VOLTS 4.29
/* Output voltage when no note is being played.
 * A negative voltage puts the DAC into a high-impedance state, which
 * allows other control sources to override it. */
#define IDLE_VOLTS -1

/* Return the required voltage for a MIDI note number.
 * If the note number is negative, IDLE_VOLTS will be returned.
 * If the note number is out of range, it will be octave shifted
 * into range. If the range of the synthesizer is less than one octave,
 * and the note is not present in that octave, it will not be played. */
float note_voltage(int midinote);

/* Return the voltage offset for a MIDI pitchbend. */
float pitchbend_voltage(int midibend);

/* Setup the ALSA seq port. Data will be copied into seqhandle.
 * Returns 0 on success, -1 on error. */
int alsa_init(snd_seq_t **seqhandle);

/* Close down the ALSA seq port. */
void alsa_shutdown(snd_seq_t *seqhandle);

/* Set gotsignal to 1, terminating the main loop.
 * Called on receipt of SIGTERM or SIGINT */
void terminate_loop(int);

int main(int argc, char *argv[]){
	int note = -1; /* MIDI note we are currently playing. */
	int note_update = 1; /* If 1, the note has changed;
	                      * re-send the output voltage. */
	int pitchbend = 8192; /* Current pitchbend value. 8192 = no bend. */
	snd_seq_t *seqhandle; /* ALSA Sequencer handle */
	snd_seq_event_t *ev; /* Sequencer event for handling MIDI messages. */
	int npfd; /* Number of pollfds */
	struct pollfd *pfd; /* Storage for array of struct pollfd, used for
	                     * polling for MIDI messages. */
	if(argc != 2){
		fputs(usage, stderr);
		return -1;
	}
	if(serialcv_init(argv[1], VREF)){
		fputs("monotron: could not open serial port\n", stderr);
		return -2;
	}
	if(alsa_init(&seqhandle)){
		fputs("monotron: coudle not open ALSA sequencer\n", stderr);
		serialcv_shutdown();
		return -3;
	}
	npfd = snd_seq_poll_descriptors_count(seqhandle, POLLIN);
	pfd = malloc(npfd * sizeof(struct pollfd));
	if(NULL == pfd){
		fputs("monotron: malloc failure\n", stderr);
		serialcv_shutdown();
		alsa_shutdown(seqhandle);
	}
	snd_seq_poll_descriptors(seqhandle, pfd, npfd, POLLIN);
	signal(SIGINT, terminate_loop);
	signal(SIGTERM, terminate_loop);
	while(!gotsignal){
		if(note_update){
			if(note >= 0){
				serialcv_voltage(note_voltage(note) +
				    pitchbend_voltage(pitchbend));
			} else {
				serialcv_voltage(note_voltage(-1));
			}
			note_update = 0;
		}
		if(poll(pfd, npfd, 1000) > 0){ /* MIDI Message received */
			do{
				snd_seq_event_input(seqhandle, &ev);
				if(MIDI_CH == ev->data.control.channel){
					switch(ev->type){
						case SND_SEQ_EVENT_PITCHBEND:
							pitchbend = ev->data.control.value;
							note_update = 1;
							break;
						case SND_SEQ_EVENT_NOTEON:
							if(0 == ev->data.note.velocity){
								if(ev->data.note.note == note){
									note = -1;
									note_update = 1;
								}
							} else {
								note = ev->data.note.note;
							/* Briefly output the idle voltage to
							 * re-trigger the synth's EG */
								serialcv_voltage(IDLE_VOLTS);
								note_update = 1;
							}
							break;
						case SND_SEQ_EVENT_NOTEOFF:
							if(ev->data.note.note == note){
								note = -1;
								note_update = 1;
							}
							break;
					}
				}
				snd_seq_free_event(ev);
			}while(snd_seq_event_input_pending(seqhandle, 0));
		}
	}
	serialcv_shutdown();
	alsa_shutdown(seqhandle);
	free(pfd);
	return 0;
}

#define M ((HIGH_VOLTS-LOW_VOLTS)/(HIGH_MIDI-LOW_MIDI))
#define B (HIGH_VOLTS - (M * HIGH_MIDI))
/* Return a number between 0 (C) and 11 (B), representing the
 * tone of a midi note regardless of the octave. This is used
 * when octave shifting to determine the note to play. */
#define TONE(midinote) (midinote % 12)

/* Return the required voltage for a MIDI note number.
 * If the note number is negative, IDLE_VOLTS will be returned.
 * If the note number is out of range, it will be octave shifted
 * into range. If the range of the synthesizer is less than one octave,
 * and the note is not present in that octave, it will not be played. */
float note_voltage(int midinote){
	if(midinote < 0)return IDLE_VOLTS;
	if(midinote <= HIGH_MIDI &&
		midinote >= LOW_MIDI){ /* Simple case - we can play the note as-is. */
		return (midinote * M) + B;
	} else { /* Complicated case - octave shifting is needed. */
#if ((HIGH_MIDI - LOW_MIDI) + 1) < 12
		/* If the synthesizer cannot play a complete octave,
		 * notes that it cannot play will be ignored. */
		if(TONE(midinote) >= TONE(LOW_MIDI) && TONE(midinote) <= TONE(HIGH_MIDI)){
			midinote = LOW_MIDI - TONE(LOW_MIDI) + TONE(midinote);
			return (midinote * M) + B;
		} else {
			return IDLE_VOLTS;
		}
#else
		/* If the synthesizer can play a complete octave (or more),
		 * notes that are too high will be shifted to the top of its
		 * range, and notes that are too low will be shifted to the bottom
		 * of its range. */
		 if(midinote > HIGH_MIDI){
			if(TONE(midinote) > TONE(HIGH_MIDI)){
				midinote = HIGH_MIDI - 12 - TONE(HIGH_MIDI) + TONE(midinote);
			} else {
				midinote = HIGH_MIDI - TONE(HIGH_MIDI) + TONE(midinote);
			}
		 } else {
			if(TONE(midinote) < TONE(LOW_MIDI)){
				midinote = LOW_MIDI + 12 - TONE(LOW_MIDI) + TONE(midinote);
			} else {
				midinote = LOW_MIDI - TONE(LOW_MIDI) + TONE(midinote);
			}
		 }
		return (midinote * M) + B;
#endif
	}
}
#undef B
#undef TONE

/* Return the voltage offset for a MIDI pitchbend. */
float pitchbend_voltage(int midibend){
	/* Total pitchbend range is +/- 2 semitones */
	midibend -= 8192; /* Convert from 8192 = no bend to 0 = no bend */
	return ((float)midibend/4096) * M;
}
#undef M

/* Setup the ALSA seq port. Data will be copied into seqhandle.
 * Returns 0 on success, -1 on error. */
int alsa_init(snd_seq_t **seqhandle){
	if(NULL == seqhandle){
		return -1;
	}
	if(snd_seq_open(seqhandle, "default", SND_SEQ_OPEN_INPUT, 0)){
		return -1;
	}
	snd_seq_set_client_name(*seqhandle, SEQ_NAME);
	if(snd_seq_create_simple_port(*seqhandle, "Input",
	              SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
	              SND_SEQ_PORT_TYPE_APPLICATION)){
		return -1;
	}
	return 0;
}

/* Close down the ALSA seq port. */
void alsa_shutdown(snd_seq_t *seqhandle){
	if(NULL != seqhandle){
		snd_seq_close(seqhandle);
	}
}

/* Set gotsignal to 1, terminating the main loop.
 * Called on receipt of SIGTERM or SIGINT */
void terminate_loop(int unused){
	gotsignal = 1;
}
