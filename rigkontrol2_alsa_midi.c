#include "rigkontrol2_alsa_midi_functions.h"

/*
gcc -c rigkontrol2_alsa_midi_functions.c -o rigkontrol2_alsa_midi_functions.o

gcc evtest_rigkontrol2.c -o evtest_rigkontrol2 -I/usr/include/libevdev-1.0 -levdev 

./evtest_rigkontrol2

*/


int main(int argc, char** argv) {
    int fd, rc, port;
	snd_seq_t* midi;
	struct libevdev* dev = kr2_connect(0x17cc, 0x1969, &fd, &rc);
	setup_midi_port(&midi, &port);
	dev_midi_event_loop(dev, midi, port);


    // Cleanup
	libevdev_grab(dev, LIBEVDEV_UNGRAB); // Ungrab the device before exiting
    libevdev_free(dev);
    close(fd);
    return 0;
}

