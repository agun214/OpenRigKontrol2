#include "rigkontrol2_alsa_midi_functions.h"

struct libevdev* kr2_connect(int vendor_id, int product_id, int* fd_ptr, int* rc_ptr) {
    
    //int vendor_id = 0x045e;
    //int product_id = 0x028e;

    struct libevdev* dev = NULL;
    int fd = -1;
    int rc = -1;

    for (int i = 0; i < 300; i++) {
        char path[128];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);

        fd = open(path, O_RDONLY|O_NONBLOCK);
        if (fd < 0) {continue;}

        rc = libevdev_new_from_fd(fd, &dev);
        if (rc < 0) {close(fd); continue;}

        if (libevdev_get_id_vendor(dev) == vendor_id &&
            libevdev_get_id_product(dev) == product_id) 
            {break;}
    }
    printf("Device found: %s\n", libevdev_get_name(dev));

    rc = libevdev_grab(dev, LIBEVDEV_GRAB);
    if (rc != 0) {
        fprintf(stderr, "Failed to grab device: %s\n", strerror(-rc));
        libevdev_free(dev);
        close(fd);
    }

    // Update the pointers with the obtained values
    *fd_ptr = fd;
    *rc_ptr = rc;

    return dev; // Return the dev variable to main
}

// create_alsamidi_port
int setup_midi_port(snd_seq_t** midi_ptr, int* port_ptr) {
    // Open a MIDI device
	snd_seq_t* midi = NULL;
	snd_seq_open(&midi, "default", SND_SEQ_OPEN_OUTPUT, 0);
	snd_seq_set_client_name(midi, "KontrolRig2 MIDI");

	// Create a MIDI port
	int port = snd_seq_create_simple_port(midi, "KontrolRig2", SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ, SND_SEQ_PORT_TYPE_MIDI_GENERIC);

    // Update the pointer with the obtained values
	*midi_ptr = midi;
	*port_ptr = port;

    printf("port: open\n");
}

//   MIDI_note(midi, controlStates[controlIndex].MIDIevcode, ev.value, channel, port, noteoffset);
void MIDI_note(snd_seq_t *midi, int note, int velocity, int channel, int port, int noteoffset) {
	snd_seq_event_t midi_event;
	snd_seq_ev_clear(&midi_event);
	snd_seq_ev_set_source(&midi_event, port);
	snd_seq_ev_set_subs(&midi_event);
	snd_seq_ev_set_direct(&midi_event);

	if (velocity != 0) {
		midi_event.type = SND_SEQ_EVENT_NOTEON;
		midi_event.data.note.channel = 0;
		midi_event.data.note.note = note + noteoffset; //controlStates[controlIndex].MIDIevcode 
		midi_event.data.note.velocity = 127;
	} else {
		midi_event.type = SND_SEQ_EVENT_NOTEOFF;
		midi_event.data.note.channel = channel;
		midi_event.data.note.note = note + noteoffset;
		midi_event.data.note.velocity = 0;
	}
    snd_seq_event_output(midi, &midi_event);
    snd_seq_drain_output(midi);
}

void MIDI_controller(struct libevdev *dev, struct input_event ev, snd_seq_t *midi, int channel, int port, int invert_axis) {
	snd_seq_event_t midi_event;
	snd_seq_ev_clear(&midi_event);
	snd_seq_ev_set_source(&midi_event, port);
	snd_seq_ev_set_subs(&midi_event);
	snd_seq_ev_set_direct(&midi_event);
	// Calculate the pitch bend value based on the joystick position
	int modwheel_val = (int)(ev.value / 32.0f);

	// Set the MIDI event to a pitchbend event
	midi_event.type = SND_SEQ_EVENT_CONTROLLER;
	midi_event.data.control.channel = channel;  // set MIDI channel to 1
	midi_event.data.control.param = 1;  // set controller to modulation wheel
	midi_event.data.control.value = modwheel_val;  // use joystick position as controller value	
    // Send the MIDI event
    snd_seq_event_output(midi, &midi_event);
    snd_seq_drain_output(midi);
	printf("CTRL ev.value: %d\n", ev.value);
}

// Function to handle pitch bend events
//   MIDI_pitchbend(dev, ev, midi, port, channel, 1);
void MIDI_pitchbend(struct libevdev *dev, struct input_event ev, snd_seq_t *midi, int port, int channel, int invert_axis) {
    snd_seq_event_t midi_event;
    snd_seq_ev_clear(&midi_event);
    snd_seq_ev_set_source(&midi_event, port);
    snd_seq_ev_set_subs(&midi_event);
    snd_seq_ev_set_direct(&midi_event);
	// Calculate the pitch bend value based on the joystick position
	int pitchbend_val = (int)(ev.value * 2.0f);

    // Set the MIDI event to a pitchbend event
    midi_event.type = SND_SEQ_EVENT_PITCHBEND;
    midi_event.data.control.channel = channel;
    midi_event.data.control.value = pitchbend_val;
    // Send the MIDI event
    snd_seq_event_output(midi, &midi_event);
    snd_seq_drain_output(midi);
	printf("PBEND ev.value: %d\n", ev.value);
}

void dev_midi_event_loop(struct libevdev *dev, snd_seq_t *midi, int port) {
	int rc;	
	int controlIndex;
	int noteoffset = 60;
	int channel = 0;

	// Array to store the state of each button : ev.code, ev.value, MIDI 
	struct ControlState controlStates[NUM_CONTROLS] = {
		{1, 2, 0, 0},	      	// 1
		{1, 3, 0, 2},	      	// 2
		{1, 4, 0, 4},	      	// 3
		{1, 5, 0, 7},	      	// 4
		{1, 6, 0, 9},	 		// 5
		{1, 7, 0, 12},	 		// 6
		{1, 8, 0, 1},	    	// 7
		{3, 0, 0, 3},       	// onboard expression pedal 
		{3, 1, 0, 6},        	// pedal input #1 		
		{3, 2, 0, 8}, 			// pedal input #2
	};

	// Process events
	while (1) {
		struct input_event ev;
		rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
		if (rc == 0) {
		    // Find the corresponding button state
		    controlIndex = -1;

		    for (int i = 0; i < NUM_CONTROLS; i++) {
		        if (ev.code == controlStates[i].evcode) {
		            controlIndex = i;
		            break;
		        }
		    }
//		    if (controlIndex != -1 && (ev.type == EV_ABS || ev.type == EV_KEY)) {
			switch (ev.type) {
				case 1:
					switch (ev.code) {
						// Buttons
						case 2 ... 8:  // Button pressed
							printf("BTN ev: %d, val: %d, MIDI: %d\n", ev.code, ev.value, controlStates[controlIndex].MIDIevcode);
							MIDI_note(midi, controlStates[controlIndex].MIDIevcode, ev.value, channel, port, noteoffset);
							break;
					}
					break;
				case 3:
					switch (ev.code) {
						// Joystick Axes
						case 0: 						
							// struct libevdev *dev, struct input_event ev, snd_seq_t *midi, int port, int channel, int invert_axis
							MIDI_pitchbend(dev, ev, midi, port, channel, 1); // invert_axis = 1;
		                    break;
						case 1:
							// struct libevdev *dev, struct input_event ev, snd_seq_t *midi, int channel, int port, int invert_axis
							MIDI_controller(dev, ev, midi, port, channel, 1);
		                    break;
						case 3:
							printf("JOY ev: %d, val: %d, MIDI: %d\n", ev.code, ev.value, controlStates[controlIndex].MIDIevcode);
		                    break;
					}
					break;
				default:
					break;
		    }
		} else if (rc == -EAGAIN) {
		    // No events available, try again
		    continue;
		} else {
		    // Error reading event, exit loop
		    break;
		}
	}
}