#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <inttypes.h>
#include <libconfig.h>

// TODO Make everything into functions
// TODO Optimize for loop variable names
// TODO Fix * locations for variable declarations
//
//
struct conOutputs {
	XRROutputInfo *outputInfo;
	int outputNum;
	struct conOutputs *next;
};

Display* myDisp;
Window myWin;
XRRMonitorInfo* myMon;
XRRScreenResources* myScreen;
XRROutputInfo* myOutput;
XRRPropertyInfo* myProp;
XEvent event;
Atom edid_atom, *temp;
char* display_name = 0; // TODO is this correct?
int i,j,k,l,z;
int *m;
int save = 0;
int load = 0;
int delete = 0;
int numMon,nprop;
int quiet = 0;
int cfg_idx;
Bool get_active = 0;
char edid_name[] = "EDID"; // TODO needed?
unsigned char* edid,*edid_string;
Bool only_if_exists = 1;
unsigned char* prop;
int actual_format;
unsigned long nitems,bytes_after;
Atom actual_type;
XRRCrtcInfo* myCrtc; // TODO I am allocating the memory twice over must fix
XRRCrtcInfo **myCrtc2;
char config_file[] = "umon.conf";
char* profile_name;
config_setting_t *edid_setting,*resolution_setting,*pos_x_setting,*pos_y_setting,*pos_group,*root,*group,*list;
config_t config;
const char **edid_val,**resolution_str;
int *pos_val;
unsigned char **connected_edids;
struct conOutputs *new_output;
struct conOutputs *head, *cur_output = NULL;


int main(int argc, char **argv) {

	if (argc == 3) {
		if (!strcmp("--save", argv[1])) {
			save = 1;
		}
		else if (!strcmp("--load", argv[1])) {
			printf("Argument is to load\n");
			load = 1;
		}
		else if (!strcmp("--delete", argv[1])){
			delete = 1;
		}

		profile_name = argv[2];
		printf("Profile name: %s\n", profile_name);

		config_init(&config);

		if (config_read_file(&config, config_file)) {
			printf("Detected existing configuration file\n");
			// Existing config file to load setting values
			if (load) {
				printf("Loading profile\n");
				list = config_lookup(&config,profile_name);
				printf("Profile loaded\n");
				if (list != NULL) {
					printf("Fetching display status\n");
					// Construct list of current EDIDS
					// Fetch current configuration info
					myDisp = XOpenDisplay(display_name);
					myWin = DefaultRootWindow(myDisp);
					// TODO Assume 1 screen?
					// TODO Gotta free myScreen probably myCrtc as well XRRFree-something
					myScreen = XRRGetScreenResources(myDisp,myWin);
					edid_atom = XInternAtom(myDisp,edid_name,only_if_exists);
					myCrtc2 = (XRRCrtcInfo**) malloc(myScreen->ncrtc * sizeof(XRRCrtcInfo*));

					for(k=0;k<myScreen->ncrtc;++k) {
						myCrtc2[k] = XRRGetCrtcInfo(myDisp,myScreen,myScreen->crtcs[k]);
					}
					printf("I should be fine here\n");

					k = 0;
					// Should use linked lists
					for (i=0;i<myScreen->noutput;++i) {
						myOutput = XRRGetOutputInfo(myDisp,myScreen,myScreen->outputs[i]);
						// printf("Name: %s Connection %d\n",myOutput->name,myOutput->connection);
						if (!myOutput->connection) {
							// TODO Free the list and int
							new_output = (struct conOutputs*) malloc(sizeof(struct conOutputs));
							printf("%d\n",i);
							new_output->outputInfo = myOutput;
							new_output->next = head;
							new_output->outputNum = i;
							head = new_output;
							++k;
							printf("Oh where oh where\n");
						}
					}
					cur_output = head;
					printf("Done constructing linked list of outputs\n");

					l = config_setting_length(list);
					edid_val = (const char **) malloc(l * sizeof(const char *));
					resolution_str = (const char **) malloc(l * sizeof(const char *));
					pos_val = (int *) malloc(2*l * sizeof(int));
					for(i=0;i<l;++i) {
							group = config_setting_get_elem(list,i);
							pos_group = config_setting_lookup(group,"pos");
							config_setting_lookup_string(group,"EDID",edid_val+i);
							config_setting_lookup_string(group,"resolution",resolution_str+i);
							config_setting_lookup_int(pos_group,"x",pos_val+2*i);
							config_setting_lookup_int(pos_group,"y",pos_val+2*i+1);
							printf("Loaded values: \n");
							printf("EDID: %s\n",*(edid_val+i));
							printf("Resolution: %s\n",*(resolution_str+i));
							printf("Pos: x=%d y=%d\n",*(pos_val+2*i),*(pos_val+2*i+1));
					}

					// Now I have both lists, so can do a double loops arounnd list of connected monitors and the saved monitors
					// k is the number of connected outputs
					// l is the number of loaded monitors
					printf("Trying to find matching monitor...\n");
					for (i=0;i<k;++i) {
						// Loop around connected outputs
						// Get edid
						printf("%d\n",cur_output[i].outputNum);
						XRRGetOutputProperty(myDisp,myScreen->outputs[cur_output[i].outputNum],edid_atom,0,100,False,False,AnyPropertyType,&actual_type,&actual_format,&nitems,&bytes_after,&edid);
						// Convert edid to how it is stored
						if (nitems) {
							// printf("%s: ",edid_name);
							// Make edid into string
							edid_string = (unsigned char *) malloc((nitems+1) * sizeof(char));
							for (z=0;z<nitems;++z) {
								if (edid[z] == '\0') {
									edid_string[z] = '0';
								}
								else {
									edid_string[z] = edid[z];
								}
								//printf("%c",edid_string[z]);
							}
							edid_string[nitems] = '\0';

							for (j=0;j<l;++j) {
									// Now loop around loaded outputs
									printf("Current output edid: %s\n", edid_string);
									printf("Matching with loaded output %s\n", *(edid_val+j));
									if (!strcmp(edid_string,*(edid_val+j))){
										printf("Match found!!!\n");
										// Now I need to find the current output mode and then change them
										// Really I only need the display mode (resolution) and ? (position)
										// So I'm going to temporarily find them out in the following just to find the name and then remove the code
										// Screen has a list of modes, and so does the output
										// Must find screen name first
										for (z=0;z<myScreen->nmode;++z) {
											if (!strcmp(myScreen->modes[z].name,*(resolution_str+j))) {
												printf("I know the mode! \n");
												// Set
												// TODO Assuming only one output per crtc
												XRRSetCrtcConfig(myDisp,myScreen,cur_output[i].outputInfo->crtc,CurrentTime,*(pos_val+2*j),*(pos_val+1+2*j),myScreen->modes[z].id,RR_Rotate_0,&myScreen->outputs[cur_output[i].outputNum],1);
											}
										}
									}
								}
								free(edid_string);
							}
							cur_output = cur_output->next;
						}

						// XRRSetScreenConfigAndRate(myDisp,screen_config,draw,size_index,rotation,rate,timestamp);
						config_destroy(&config);
						free(edid_val);
						free(resolution_str);
						free(pos_val);
					}
					else {
						printf("No profile found");
						exit(2); // No profile with the specified name
					}
				}
				// Overwrite existing profile
				// printf("Am I here?\n");
				if (save || delete) {

					list = config_lookup(&config,profile_name);
					if (list != NULL) {
						printf("Existing profile found, overwriting\n");
						cfg_idx = config_setting_index(list);
						root = config_setting_parent(list);
						// printf("Configuration index: %d\n",cfg_idx);
						config_setting_remove_elem(root,cfg_idx);
					}
				}
			}
			else {
				if (load || delete) {
					printf("No file to load or delete");
					exit(1); // No file to load or delete
				}
			}

			if (save) {
				root = config_root_setting(&config);
				list = config_setting_add(root,profile_name,CONFIG_TYPE_LIST);

				// Fetch current configuration info
				myDisp = XOpenDisplay(display_name);
				myWin = DefaultRootWindow(myDisp);
				edid_atom = XInternAtom(myDisp,edid_name,only_if_exists);

				// Get screen configuration
				// TODO Assume 1 screen?
				// TODO Gotta free myScreen XRRFree-something
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

						// Setup config
						group = config_setting_add(list,NULL,CONFIG_TYPE_GROUP);
						edid_setting = config_setting_add(group,"EDID",CONFIG_TYPE_STRING);
						resolution_setting = config_setting_add(group,"resolution",CONFIG_TYPE_STRING);
						pos_group = config_setting_add(group,"pos",CONFIG_TYPE_GROUP);
						pos_x_setting = config_setting_add(pos_group,"x",CONFIG_TYPE_INT);
						pos_y_setting = config_setting_add(pos_group,"y",CONFIG_TYPE_INT);

						XRRGetOutputProperty(myDisp,myScreen->outputs[i],edid_atom,0,100,False,False,AnyPropertyType,&actual_type,&actual_format,&nitems,&bytes_after,&edid);
						// printf("%d",nitems);
						if (nitems) {
							// printf("%s: ",edid_name);
							// Make edid into string
							edid_string = (unsigned char *) malloc((nitems+1) * sizeof(char));
							for (z=0;z<nitems;++z) {
								if (edid[z] == '\0') {
									edid_string[z] = '0';
								}
								else {
									edid_string[z] = edid[z];
								}
								//printf("%c",edid_string[z]);
							}
							printf("\n");
							edid_string[nitems] = '\0';
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
				free(myCrtc);
				config_write_file(&config,config_file);
				printf("Destroying config");
				config_destroy(&config);
			}
		}


		// 	while (1){
		// 		XNextEvent(myDisp, (XEvent *) &event);
		// 		printf ("Event received, type = %d\n", event.type);
		// 		XRRUpdateConfiguration?
		// 	}

	}
