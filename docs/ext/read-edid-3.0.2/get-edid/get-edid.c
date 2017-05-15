/* This file mostly just does some ui and calls the two main functions (in other files)
 *
 *
 * (C)opyright 2008-2014 Matthew Kern
 * Full license terms in file LICENSE
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int quiet=0;
int i2conly=0; //0=both, 1=i2conly, 2=classiconly
int i2cbus=-1;
int classmon=0;

#define display(...) if (quiet == 0) { fprintf(stderr, __VA_ARGS__); }
#define version "3.0.2"

int stuped(char *str) {
	int i;
	char temp[128];
	for (i=0;i<strlen(str);i++) {
		if (str[i] >= '0' && str[i] <= '9') {
			temp[i] = str[i];
			display("%c\n", temp[i]);
		}
		else {
			if (i==0) {
				display("-b and -m require numerical arguments.\n");
				exit(3);
			}
			temp[i] = '\0';
			break;
		}
	}
	return atoi(temp);

}

#if defined I2CBUILD && defined CLASSICBUILD
#define BOTHBUILD
#endif

#ifdef CLASSICBUILD
int classicmain( unsigned contr, int qit );
#endif
#ifdef I2CBUILD
int i2cmain( int bus, int qit );
#endif

void help() {
	display("get-edid, from read-edid %s. Licensed under the GPL.\n", version);
	display("Current version by Matthew Kern <pyrophobicman@gmail.com>\n");
	display("Previous work by John Fremlin <vii@users.sourceforge.net>\n");
	display("and others (See AUTHORS).\n\n");
	display("Usage:\n");
	#ifdef I2CBUILD
	display(" -b BUS, --bus BUS	Only scan the i2c bus BUS.\n");
	#endif
	#ifdef BOTHBUILD
	display(" -c, --classiconly	Do not check the i2c bus for an EDID\n");
	#endif
	display(" -h, --help		Display this help\n");
	#ifdef BOTHBUILD
	display(" -i, --i2conly		Do not check for an EDID over VBE\n");
	#endif
	#ifdef CLASSICBUILD
	display(" -m NUM, --monitor NUM	For VBE only - some lame attempt at selecting monitors.\n");
	#endif
	display(" -q, --quiet		Do not output anything over stderr (messages, essentially)\n\n");
	display("For help, go to <http://polypux.org/projects/read-edid/> or\n");
	display("email <pyrophobicman@gmail.com>.\n");
	exit(0);
}

int main(int argc, char *argv[]) {
	int i;
	
//	display("%s\n", argv[2]);

	for (i=1;i<argc;i++) {
		//display("%i: %s\n", i, argv[i]);
		if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0)
			quiet = 1;
		#ifdef BOTHBUILD
		else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--i2conly") == 0)
			i2conly = 1;
		else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--classiconly") == 0)
			i2conly = 2;
		#endif
		else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
			help();
		#ifdef I2CBUILD
		else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--bus") == 0){
			i++;
			if (argc <= i) {
				display("-b requires a numerical argument... did you put it too close to a pipe?\n");
				exit(2);
			}
			i2cbus = stuped(argv[i]);
		}
		#endif
		#ifdef CLASSICBUILD
		else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--monitor") == 0){
			i++;
			if (argc <= i) {
				display("-m requires a numerical argument... did you put it too close to a pipe?\n");
				exit(2);
			}
			classmon = stuped(argv[i]);
		}
		#endif
		else
			help();
	}

	display("This is read-edid version %s. Prepare for some fun.\n", version);
	
	#ifdef I2CBUILD
	if (i2conly < 2) {
		display("Attempting to use i2c interface\n");
		if (i2cmain(i2cbus, quiet) == 0) {
			display("Looks like i2c was successful. Have a good day.\n");
			exit(0);
		}
	}
	#endif
	#ifdef CLASSICBUILD
	if (i2conly != 1) {
		display("Attempting to use the classical VBE interface\n");
		if (classicmain(classmon, quiet) == 0) {
			display("Looks like VBE was successful. Have a good day.\n");
			exit(0);
		}
	}
	#endif

	display("I'm sorry nothing was successful. Maybe try some other arguments\nif you played with them, or send an email to Matthew Kern <pyrophobicman@gmail.com>.\n");

	return 1;
}
