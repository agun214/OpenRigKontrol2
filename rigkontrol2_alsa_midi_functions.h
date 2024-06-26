#ifndef RIGKONTROL2_ALSA_MIDI_FUNCTIONS_H
#define PS3_CONNECT_H
#define SETUP_MIDI_PORT_H
#define MIDI_NOTE_H
#define MIDI_CONTROLLER_H
#define MIDI_PITCHBEND_H
#define DEV_MIDI_EVENT_LOOP_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <alsa/asoundlib.h>

#define NUM_CONTROLS 10

// Structure to store the state of each button
struct ControlState {
    int evtype;  // KR2 input type
    int evcode;  // KR2 code
    int state;    // 0 for released, 1-255 for pressed
    int MIDIevcode;   // midi event associated with the control
};

struct libevdev* kr2_connect(int vendor_id, int product_id, int* fd_ptr, int* rc_ptr);
int setup_midi_port(snd_seq_t** midi_ptr, int* port_ptr);
void MIDI_note(snd_seq_t *midi, int note, int velocity, int channel, int port, int noteoffset);
void MIDI_controller(struct libevdev *dev, struct input_event ev, snd_seq_t *midi, int channel, int port, int invert_axis);
void MIDI_pitchbend(struct libevdev *dev, struct input_event ev, snd_seq_t *midi, int port, int channel, int invert_axis);
void dev_midi_event_loop(struct libevdev *dev, snd_seq_t *midi, int port);

#endif // PS3_CONNECT_H