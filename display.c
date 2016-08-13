#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <inttypes.h>
#include <libconfig.h>

// TODO Make everything into functions

int main(int argc, char **argv) {
	Display* myDisp;
	Window myWin;
	XRRMonitorInfo* myMon;
	XRRScreenResources* myScreen;
	XRROutputInfo* myOutput;
	XRRPropertyInfo* myProp;
	XEvent event;
	Atom edid_atom;
	char* display_name = 0;
	int i,j,k,l,z;
	int save = 0;
	int load = 0;
	int numMon,nprop;
	int quiet = 0;
	int i2cbus = -1;
	int cfg_idx;
	Bool get_active = 0;
	char edid_name[] = "EDID";
	unsigned char* edid,*edid_string;
	Bool only_if_exists = 1;
	unsigned char* prop;
	int actual_format;
	unsigned long nitems,bytes_after;
	Atom actual_type;
	XRRCrtcInfo* myCrtc;
	char config_file[] = "umon.conf";
	char* profile_name;
	config_setting_t *edid_setting,*resolution_setting,*pos_x_setting,*pos_y_setting,*pos_group,*root,*group,*list;
	config_t config;

	if (argc == 3) {
		if (!strcmp("--save", argv[1])) {
			save = 1;
		}
		else if (!strcmp("--load", argv[1])) {
			load = 1;
		}

		profile_name = argv[2];

		config_init(&config);

		if (config_read_file(&config, config_file)) {
			printf("Detected existing configuration file\n");
			// Existing config file to load setting values
			if (load) {
				/*
				group = config_lookup(&config,profile_name);
				if (group != NULL) {
					edid_setting = config_setting_get_string(group,"EDID",CONFIG_TYPE_STRING);
					resolution_setting = config_setting_add(group,"resolution",CONFIG_TYPE_STRING);
					pos_group = config_setting_add(group,"pos",CONFIG_TYPE_GROUP);
					pos_x_setting = config_setting_add(pos_group,"x",CONFIG_TYPE_INT);
					pos_y_setting = config_setting_add(pos_group,"y",CONFIG_TYPE_INT);

				}
				else {
					// No profile with the specified name
					exit(2);
				}
				*/
			}
			// Overwrite existing profile
			if (save) {

				list = config_lookup(&config,profile_name);
				if (list != NULL) {
					printf("Existing profile found, overwriting\n");
					cfg_idx = config_setting_index(list);
					root = config_setting_parent(list);
					printf("Configuration index: %d\n",cfg_idx);
					config_setting_remove_elem(root,cfg_idx);
				}
				// Make new profile 
				// TODO: Make into function
				root = config_root_setting(&config);
				list = config_setting_add(root,profile_name,CONFIG_TYPE_LIST);
				group = config_setting_add(list,NULL,CONFIG_TYPE_GROUP);
				edid_setting = config_setting_add(group,"EDID",CONFIG_TYPE_STRING);
				resolution_setting = config_setting_add(group,"resolution",CONFIG_TYPE_STRING);
				pos_group = config_setting_add(group,"pos",CONFIG_TYPE_GROUP);
				pos_x_setting = config_setting_add(pos_group,"x",CONFIG_TYPE_INT);
				pos_y_setting = config_setting_add(pos_group,"y",CONFIG_TYPE_INT);
			}
		}
		else {
			if (load) exit(1); // No file to load
			if (save) {
				root = config_root_setting(&config);
				list = config_setting_add(root,profile_name,CONFIG_TYPE_LIST);
				group = config_setting_add(list,NULL,CONFIG_TYPE_GROUP);
				edid_setting = config_setting_add(group,"EDID",CONFIG_TYPE_STRING);
				resolution_setting = config_setting_add(group,"resolution",CONFIG_TYPE_STRING);
				pos_group = config_setting_add(group,"pos",CONFIG_TYPE_GROUP);
				pos_x_setting = config_setting_add(pos_group,"x",CONFIG_TYPE_INT);
				pos_y_setting = config_setting_add(pos_group,"y",CONFIG_TYPE_INT);
			}
		}
		if (save) {
			// TODO Fix the logic to make multiple groups for multiple outputs
			// Fetch current configuration info
			myDisp = XOpenDisplay(display_name);
			myWin = DefaultRootWindow(myDisp);
			edid_atom = XInternAtom(myDisp,edid_name,only_if_exists);

			// Get screen configuration
			// TODO Assume 1 screen?
			myScreen = XRRGetScreenResources(myDisp,myWin);
			// XRRSelectInput(myDisp,myWin,RROutputChangeNotifyMask);
			
			myCrtc = (XRRCrtcInfo*) malloc(myScreen->ncrtc * sizeof(XRRCrtcInfo));
			for(k=0;k<myScreen->ncrtc;++k) {
				myCrtc[k] = *XRRGetCrtcInfo(myDisp,myScreen,myScreen->crtcs[k]);
			}

			// printf("Number of outputs: %d\n", myScreen->noutput);
			for (i=0;i<myScreen->noutput;++i) {
				myOutput = XRRGetOutputInfo(myDisp,myScreen,myScreen->outputs[i]);
				// printf("Name: %s Connection %d\n",myOutput->name,myOutput->connection);
				if (!myOutput->connection) {
					XRRGetOutputProperty(myDisp,myScreen->outputs[i],edid_atom,0,100,False,False,AnyPropertyType,&actual_type,&actual_format,&nitems,&bytes_after,&edid);
					// printf("%d",nitems);
					if (nitems) {
						// printf("%s: ",edid_name);
						// Make edid into string
						edid_string = (unsigned char *) malloc((nitems+1) * sizeof(char));
						for (z=0;z<nitems;++z) {
							edid_string[z] = edid[z];
							printf("%c",edid[z]);
						}
						printf("\n");
						edid_string[nitems] = 0;
						printf("%s",edid_string);
						// Get current mode
						for (j=0;j<myOutput->nmode;++j) {
							// printf("Mode: %d\n",myOutput->modes[j]);
							// Get crct configuration
							for(k=0;k<myScreen->ncrtc;++k) {
								// printf("Crtc mode id: %d\n",myCrtc[k].mode);
								if (myOutput->modes[j] == myCrtc[k].mode) {
									// Save current output and mode id

									config_setting_set_string(edid_setting,edid_string);
									free(edid_string);

									// Have mode number, must find name of mode
									for (l=0;l<myScreen->nmode;++l) {
										if (myScreen->modes[l].id == myOutput->modes[j]) {
											config_setting_set_string(resolution_setting,myScreen->modes[l].name);
										}
									}

									config_setting_set_int(pos_x_setting,myCrtc[k].x);
									config_setting_set_int(pos_y_setting,myCrtc[k].y);
									// printf("Match!");
								}
							}
						}
					}
				}
			}
			config_write_file(&config,config_file);
			config_destroy(&config);
			free(myCrtc);
		}
	}
	

// 	while (1){
// 		XNextEvent(myDisp, (XEvent *) &event);
// 		printf ("Event received, type = %d\n", event.type);
// 	}

}
