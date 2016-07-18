#include <stdio.h>
#include <libudev.h>

int main() {

	struct udev* mon_mon;

	mon_mon = udev_new();

	printf("Start\n");
	printf("Udev context object at %d", mon_mon);

	// Gotta free mon_mon
	

	return 0;

}
