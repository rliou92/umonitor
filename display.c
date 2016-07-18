#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <inttypes.h>
#include <libconfig.h>

int main(int argc, char **argv) {
	Display* myDisp;
	Window myWin;
	char* display_name = 0;
	XEvent event;
	int i,j,k;
	int save = 0;
	int load = 0;
	int numMon,nprop;
	unsigned char edid[256];
	int quiet = 0;
	int i2cbus = -1;
	XRRMonitorInfo* myMon;
	Bool get_active = 0;
	XRRScreenResources* myScreen;
	XRROutputInfo* myOutput;
	XRRPropertyInfo* myProp;
	Atom edid_atom;
	char edid_name[] = "EDID";
	char* edid_check;
	Bool only_if_exists = 1;
	unsigned char* prop;
	int actual_format;
	unsigned long nitems,bytes_after;
	Atom actual_type;
	XRRCrtcInfo* myCrtc;
	char config_file[] = "umon.conf";
	char* profile_name;
	config_setting_t *edid_setting,*edid_array,*resolution_setting,*resolution_array,*pos_setting,*pos_array,*root,*group;
	config_t config;

	if (argc == 3) {
		if (!strcmp("--save", argv[2])) {
			save = 1;
		}
		else if (!strcmp("--load", argv[2])) {
			load = 1;
		}

		profile_name = argv[3];

		if (!config_read_file(&config, config_file)) {
			if (load) exit(1); // No file to load
			if (save) {
				config_init(&config);
				root = config_root_setting(&config);
				group = config_setting_add(root,profile_name,CONFIG_TYPE_GROUP);
				edid_setting = config_setting_add(group,"EDID",CONFIG_TYPE_ARRAY);
				resolution_setting = config_setting_add(group,"resolution",CONFIG_TYPE_ARRAY);
				pos_setting = config_setting_add(group,"pos",CONFIG_TYPE_ARRAY);
			}
		}
		if (save) {
			// Fetch current configuration info
			myDisp = XOpenDisplay(display_name);
			myWin = DefaultRootWindow(myDisp);
			edid_atom = XInternAtom(myDisp,edid_name,only_if_exists);

			// Get screen configuration
			// TODO Assume 1 screen?
			myScreen = XRRGetScreenResources(myDisp,myWin);
			// XRRSelectInput(myDisp,myWin,RROutputChangeNotifyMask);
			// printf("Number of outputs: %d\n", myScreen->noutput);
			for (i=0;i<myScreen->noutput;++i) {
				myOutput = XRRGetOutputInfo(myDisp,myScreen,myScreen->outputs[i]);
				// printf("Name: %s Connection %d\n",myOutput->name,myOutput->connection);
				XRRGetOutputProperty(myDisp,myScreen->outputs[i],edid_atom,0,100,False,False,AnyPropertyType,&actual_type,&actual_format,&nitems,&bytes_after,&prop);
				// printf("%d",nitems);
				if (nitems) {
					edid_sett
					// printf("%s: ",edid_name);
					// for (j=0;j<nitems;++j){
						// printf("%02" PRIx8, prop[j]);
					// }
				}
				// printf("\n");
				// Get current mode
				myCrtc = (XRRCrtcInfo*) malloc(myScreen->ncrtc * sizeof(XRRCrtcInfo));
				for(k=0;k<myScreen->ncrtc;++k) {
					myCrtc[k] = *XRRGetCrtcInfo(myDisp,myScreen,myScreen->crtcs[k]);
				}
				for (j=0;j<myOutput->nmode;++j) {
					// printf("Mode: %d\n",myOutput->modes[j]);
					// Get crct configuration
					for(k=0;k<myScreen->ncrtc;++k) {
						// printf("Crtc mode id: %d\n",myCrtc[k].mode);
						if (myOutput->modes[j] == myCrtc[k].mode) {
							// Save current output and mode id
							// printf("Match!");
						}
					}
				}
			}
			config_setting_set_string(edid_setting,edid
		}

	config_destroy(&config);

	}
	

// 	while (1){
// 		XNextEvent(myDisp, (XEvent *) &event);
// 		printf ("Event received, type = %d\n", event.type);
// 	}

}
