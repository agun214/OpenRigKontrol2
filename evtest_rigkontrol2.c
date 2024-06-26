#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <libevdev/libevdev.h>

/*
gcc evtest_rigkontrol2.c -o evtest_rigkontrol2 -I/usr/include/libevdev-1.0 -levdev 

./evtest_rigkontrol2

*/


int main(int argc, char** argv) {
    // Input device vendor product ID
    int vendor_id = 0x17cc;
    int product_id = 0x1969;

    // Find the device with the matching vendor/product IDs
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

    if (fd < 0 || rc < 0) {
        fprintf(stderr, "Failed to find device with vendor/product ID %04x:%04x\n", vendor_id, product_id);
        return 1;
    }

    printf("Device found: %s\n", libevdev_get_name(dev));

    // Grab the device to prevent it from sending events to the system
    rc = libevdev_grab(dev, LIBEVDEV_GRAB);
    if (rc != 0) {
        fprintf(stderr, "Failed to grab device: %s\n", strerror(-rc));
        libevdev_free(dev);
        close(fd);
        return 1;
    }

    // Process events
    while (1) {
        struct input_event ev;
        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc == 0 && ev.type != 0) {
            printf("Event: time %ld.%06ld, type %d, code %d, value %d\n",
                   ev.time.tv_sec, ev.time.tv_usec, ev.type, ev.code, ev.value);
        }
    }

    // Cleanup
	libevdev_grab(dev, LIBEVDEV_UNGRAB); // Ungrab the device before exiting
    libevdev_free(dev);
    close(fd);
    return 0;
}

