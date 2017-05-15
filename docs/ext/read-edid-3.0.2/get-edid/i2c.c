/* The Great I2C Getter.
 *
 * (C)opyright 2008-2014 Matthew Kern
 * Full license terms in file LICENSE
 */
#ifdef I2CBUILD
#include <stdio.h>
#include "i2c-dev.h"//use ours 'cuz it's betterer.
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//Ideas (but not too much actual code) taken from i2c-tools. Thanks guys.

int quiet;

#define display(...) if (quiet == 0) { fprintf(stderr, __VA_ARGS__); }

int open_i2c_dev(int i2cbus) {
	int i2cfile;
	char filename[16];
	unsigned long funcs;

	sprintf(filename, "/dev/i2c-%d", i2cbus);
	i2cfile = open(filename, O_RDWR);

	if (i2cfile < 0 && errno == ENOENT) {
		filename[8] = '/';
		i2cfile = open(filename, O_RDWR);
	}

	if (errno == EACCES) {
		display("Permission denied opening i2c. Run as root!\n");
		i2cfile = -2;
	}
	
	if (i2cfile >=0) {
		if (ioctl(i2cfile, I2C_FUNCS, &funcs) < 0) {
			if (quiet==0) { perror("ioctl I2C_FUNCS"); }
			i2cfile=-3;
		}

		if (!(funcs & (I2C_FUNC_SMBUS_READ_BYTE_DATA))) {
			display("No byte reading on this bus...\n");
			i2cfile=-4;
		}

		if (ioctl(i2cfile, I2C_SLAVE, 0x50) < 0) {
			if (quiet==0) {perror("Problem requesting slave address");}
			i2cfile=-5;
		}
	}

	return i2cfile;
}

int i2cmain( int bus, int qit ) {
	int i, j, ret, len, numbusses=0, tryonly=-1, i2cfile, i2cbus=0;
	int goodbus[128];
	unsigned char block[256];

	quiet = qit;
	if (bus==-1) {
		for (i2cfile = open_i2c_dev(i2cbus);i2cfile >= 0 || i2cfile < -3;) {
			//read a byte. This is the official way to scan
			if (i2cfile < -3) //problem with a bus, not general enough. Skip and close.
				goto endloop;
	
			ret = i2c_smbus_read_byte_data(i2cfile, 0);
			if (ret < 0) {
				display("No EDID on bus %i\n", i2cbus);
			}
			else {
				goodbus[numbusses] = i2cbus;
				numbusses++;
			}
	
endloop:
			close(i2cfile);
			i2cbus++;
			i2cfile = open_i2c_dev(i2cbus);
		}
		if (i2cfile == -2) {
			return 1;
		}
	
		if (numbusses == 0) {
			display("Looks like no busses have an EDID. Sorry!\n");
			return 2;
		}

		display("%i potential busses found:", numbusses);
		for (i=0;i<numbusses;i++)
			display(" %i", goodbus[i]);
		display("\n");
		if (numbusses > 1) {
			if (bus == -1)
				display("Will scan through until the first EDID is found.\nPass a bus number as an option to this program to go only for that one.\n");
		}
	}
	else {
		tryonly = bus;
		numbusses=1;
		display("Only trying %i as per your request.\n", tryonly);
	}
	ret=1;
	for (i=0;i<numbusses;i++) {
		i2cbus = goodbus[i];
		if (tryonly >= 0) {
			i2cbus = tryonly;
		}
		
		i2cfile = open_i2c_dev(i2cbus);
		if (i2cfile >=0) {//no matter how many times, >=0 still looks really angry.
			for (j=0;j<256;j++)
				block[j] = i2c_smbus_read_byte_data(i2cfile, j);
		}
		close(i2cfile);
		if (block[0]==0x00&&block[7]==0x00&&block[1]==0xff&&block[2]==0xff&&block[3]==0xff&&block[4]==0xff&&block[5]==0xff&&block[6]==0xff) {
			ret = 0;
			break;
		}
		else
			display("Bus %i doesn't really have an EDID...\n", i2cbus);
	}
	if (ret==0) {
		if (block[128] == 0xff)
			len = 128;
		else
			len = 256;
		display("%i-byte EDID successfully retrieved from i2c bus %i\n", len, i2cbus);
		if (i2cbus < (numbusses-1))
			display("If this isn't the EDID you were looking for, consider the other potential busses.\n");
		for (i=0;i<len;i++) {
			printf("%c", block[i]);
		}
	}
	else {
		display("Couldn't find an accessible EDID on this %s.\n", (tryonly==-1)?"computer":"bus");
		return 3;
	}
	return 0;
}
#endif
