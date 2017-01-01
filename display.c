#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <inttypes.h>
#include <libconfig.h>
#include <unistd.h>

// TODO: document functions, which variables are input
//
struct conOutputs {
	XRROutputInfo *outputInfo;
	int outputNum;
	struct conOutputs *next;
};

// Outputs: edid_val, resolution_str, pos_val
struct conf_sett_struct {
	//config_setting_t *edid_setting,*resolution_setting,*pos_x_setting,*pos_y_setting;
	const char **edid_val,**resolution_str;
	int *pos_val,*disp_val;
};

struct disp_info {
	Display *myDisp;
	Window myWin;
	XRRScreenResources *myScreen;
	Atom edid_atom;
};

// Display* myDisp;
// Window myWin;
// XRRScreenResources* myScreen;
// XRROutputInfo* myOutput;
// Atom edid_atom;
// int num_out_pp,num_conn_outputs,num_profiles;
// I think the following initialization syntax works
// unsigned char* edid,*edid_string;
// int actual_format;
// unsigned long nitems,bytes_after;
// Atom actual_type;
// const char **edid_val,**resolution_str;
// int *pos_val;
char config_file[] = "umon.conf";

void load_profile(struct disp_info myDisp_info, config_setting_t *list);
void fetch_display_status(struct disp_info *myDisp_info);
void construct_output_list(Display *myDisp, XRRScreenResources *myScreen, struct conOutputs **head, int *num_conn_outputs);
void free_output_list(struct conOutputs *);
void load_val_from_config(config_setting_t *list, struct conf_sett_struct *mySett, int *num_out_pp);
void edid_to_string(unsigned char *edid, unsigned long nitems, unsigned char **edid_string);
void save_profile(config_t *config,config_setting_t *list);
void listen_for_event(config_t *);

int main(int argc, char **argv) {
	int save = 0;
	int load = 0;
	int delete = 0;
	int test_event = 0;
	int quiet = 0;
	int cfg_idx;
	config_setting_t *root, *profile_group;
	struct disp_info myDisp_info;
	config_t config; 
	char* profile_name;

	config_init(&config);
	// printf("save: %d\n",save);

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
		else {
			printf("Unrecognized second argument");
			exit(5);
		}

		profile_name = argv[2];
		printf("Profile name: %s\n", profile_name);


		if (config_read_file(&config, config_file)) {
			printf("Detected existing configuration file\n");
			// Existing config file to load setting values
			if (load) {
				// Load profile
				profile_group = config_lookup(&config,profile_name);

				if (profile_group != NULL) {
					fetch_display_status(&myDisp_info);
					load_profile(myDisp_info,profile_group);
				}
				else {
					printf("No profile found");
					exit(2);
				}

				config_destroy(&config);
			}

			if (save || delete) {
				profile_group = config_lookup(&config,profile_name);
				if (profile_group != NULL) {
					// Overwrite existing profile
					printf("Existing profile found, overwriting\n");
					cfg_idx = config_setting_index(profile_group);
					root = config_setting_parent(profile_group);
					//printf("Configuration index: %d\n",cfg_idx);
					config_setting_remove_elem(root,cfg_idx);
					printf("Removed profile\n");
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
			// Always create the new profile group because above code has already deleted it if it existed before
			profile_group = config_setting_add(root,profile_name,CONFIG_TYPE_GROUP);
			printf("Checking group: %d\n", profile_group);
			save_profile(&config,profile_group);
		}
	}

	if (!strcmp("--test-event", argv[1])){
		if (config_read_file(&config, config_file)) {
			listen_for_event(&config);
		}
		else {
			printf("No file to load when event is triggered\n");
			exit(3);
		}
	}

}

void listen_for_event(config_t *config_p){
	// Inputs: num_out_pp
	int i,k,j,matches,event_num,event_base,ignore,num_conn_outputs,num_out_pp,num_profiles;
	XEvent event;
	config_setting_t *root, *list;
	struct conOutputs *cur_output, *head;
	char *profile_match;
	struct disp_info myDisp_info;
	struct conf_sett_struct mySett;
	unsigned char *edid, *edid_string;
	int actual_format;
	unsigned long nitems,bytes_after;
	Atom actual_type;

	fetch_display_status(&myDisp_info);
	XRRQueryExtension(myDisp_info.myDisp,&event_base,&ignore);
	XRRSelectInput(myDisp_info.myDisp,myDisp_info.myWin,RRScreenChangeNotifyMask);

	while (1){
		printf("Listening for event\n");
		XNextEvent(myDisp_info.myDisp, (XEvent *) &event);

		printf ("Event received, type = %d\n", event.type);
		// Is this necessary?
		XRRUpdateConfiguration(&event);
		event_num = event.type - event_base;
		if (event_num == RRScreenChangeNotify){
			printf("Screen event change received\n");
			// Wait a bit?
			sleep(1);
			// Need to find out which profile to load
			// Get list of connected outputs
			fetch_display_status(&myDisp_info);

			construct_output_list(myDisp_info.myDisp,myDisp_info.myScreen,&head,&num_conn_outputs);
			cur_output = head;
			// Get list of available profiles
			// Cannot free config
			root = config_root_setting(config_p);
			num_profiles = config_setting_length(root);
			printf("Num profiles: %d\n", num_profiles);
			for (i=0;i<num_profiles;++i){
				// For each profile
				printf("i=%d\n",i);
				list = config_setting_get_elem(root,i);
				profile_match = config_setting_name(list);
				// Get list of profile outputs
				load_val_from_config(list,&mySett,&num_out_pp);
				// Config data is now stored in mySett (edid_val, resolution_str, pos_val)
				printf("Profile number %d\n",i);

				// See if the list of connected outputs match the list of profile outputs
				// Assuming there is only one edid per profile in configuration
				matches = 0;
				for (k=0;k<num_out_pp;++k){
					for (j=0;j<num_conn_outputs;++j){
						// Fetch edid and turn it into a string
						XRRGetOutputProperty(myDisp_info.myDisp,myDisp_info.myScreen->outputs[cur_output->outputNum],myDisp_info.edid_atom,0,100,False,False,AnyPropertyType,&actual_type,&actual_format,&nitems,&bytes_after,&edid);
						// Convert edid to how it is stored
						// Assuming edid exists!!
						// if (nitems) {
						// Make edid into string
						edid_to_string(edid,nitems,&edid_string);
						// If match - load!
						if (!strcmp(edid_string,*(mySett.edid_val+k))){
							++matches;
							printf("Found a match! Match: %d\n",matches);
						}
						cur_output = cur_output->next;
					}
					cur_output = head;
					}

					if (matches == num_conn_outputs) {
						printf("Profile %s matches!!\n", profile_match );
						load_profile(myDisp_info,list);
					}
				}
				free_output_list(head);
			}
			// Prepare to listen for another event
			XRRSelectInput(myDisp_info.myDisp,myDisp_info.myWin,RRScreenChangeNotifyMask);
		}
	}

void load_profile(struct disp_info myDisp_info, config_setting_t *profile_group){
	// TODO Possible optimization, bring fetch_display_status out of this function
	// Inputs: list, num_out_pp
	int i, j, z, xrrset_status, num_conn_outputs, num_out_pp;
	struct conOutputs *cur_output, *head;
	struct conf_sett_struct mySett;
	unsigned char *edid, *edid_string;
	int actual_format;
	unsigned long nitems,bytes_after;
	Atom actual_type;
	int screen;

	printf("Loading profile\n");
	// list = config_lookup(&config,profile_name);
	printf("Fetching display status\n");
	// Construct list of current EDIDS
	// Fetch current configuration info
	// Assume display info is already fetched

	construct_output_list(myDisp_info.myDisp,myDisp_info.myScreen,&head,&num_conn_outputs);
	cur_output = head;

	load_val_from_config(profile_group,&mySett,&num_out_pp);

	// Now I have both lists, so can do a double loops arounnd list of connected monitors and the saved monitors
	// num_conn_outputs is the number of connected outputs
	// l is the number of loaded monitors
	printf("Trying to find matching monitor...\n");
	for (i=0;i<num_conn_outputs;++i) {
		// Loop around connected outputs
		// Get edid
		printf("%d\n",cur_output->outputNum);
		XRRGetOutputProperty(myDisp_info.myDisp,myDisp_info.myScreen->outputs[cur_output->outputNum],myDisp_info.edid_atom,0,100,False,False,AnyPropertyType,&actual_type,&actual_format,&nitems,&bytes_after,&edid);
		// Convert edid to how it is stored
		if (nitems) {
			// printf("%s: ",edid_name);
			// Make edid into string
			edid_to_string(edid, nitems, &edid_string);

			for (j=0;j<num_out_pp;++j) {
				// Now loop around loaded outputs
				printf("Current output edid: %s\n", edid_string);
				printf("Matching with loaded output %s\n", *(mySett.edid_val+j));
				if (!strcmp(edid_string,*(mySett.edid_val+j))){
					printf("Match found!!!\n");
					// Now I need to find the current output mode and then change them
					// Really I only need the display mode (resolution) and ? (position)
					// So I'm going to temporarily find them out in the following just to find the name and then remove the code
					// Screen has a list of modes, and so does the output
					// Must find screen name first
					for (z=0;z<myDisp_info.myScreen->nmode;++z) {
						if (!strcmp(myDisp_info.myScreen->modes[z].name,*(mySett.resolution_str+j))) {
							printf("I know the mode!: %s \n",*(mySett.resolution_str+j));
							printf("Position %dx%d\n",*(mySett.pos_val+2*j),*(mySett.pos_val+1+2*j));
							// Set
							screen = DefaultScreen(myDisp_info.myDisp);
							printf("Screen: %d\n", screen);
							printf("DisplayWidth %d\n", *(mySett.disp_val));
							printf("DisplayHeight %d\n", *(mySett.disp_val+1));
							printf("DisplayWidthMM %d\n", *(mySett.disp_val+2));
							printf("DisplayHeightMM %d\n", *(mySett.disp_val+3));
							// Check to see if current display width height is bigger than setting width height
							printf("setting res: %d\n",(*(mySett.disp_val)) * (*(mySett.disp_val+1)));
							printf("current res: %d\n",DisplayWidth(myDisp_info.myDisp,screen) * DisplayHeight(myDisp_info.myDisp,screen));
							// Looks like the way xrandr does it is to disable crtcs first
							XRRSetCrtcConfig (myDisp_info.myDisp, myDisp_info.myScreen, cur_output->outputInfo->crtc, CurrentTime, 0, 0, None, RR_Rotate_0, NULL, 0);
 							XRRSetScreenSize (myDisp_info.myDisp, myDisp_info.myWin,*(mySett.disp_val),*(mySett.disp_val+1),*(mySett.disp_val+2),*(mySett.disp_val+3));
							xrrset_status = XRRSetCrtcConfig(myDisp_info.myDisp,myDisp_info.myScreen,cur_output->outputInfo->crtc,CurrentTime,*(mySett.pos_val+2*j),*(mySett.pos_val+1+2*j),myDisp_info.myScreen->modes[z].id,RR_Rotate_0,&(myDisp_info.myScreen->outputs[cur_output->outputNum]),1);
							/*
							if ((*(mySett.disp_val)) * (*(mySett.disp_val+1)) > DisplayWidth(myDisp_info.myDisp,screen) * DisplayHeight(myDisp_info.myDisp,screen)) {
								printf("Expanding screen first\n");
 							XRRSetScreenSize (myDisp_info.myDisp, myDisp_info.myWin,*(mySett.disp_val),*(mySett.disp_val+1),*(mySett.disp_val+2),*(mySett.disp_val+3));
							xrrset_status = XRRSetCrtcConfig(myDisp_info.myDisp,myDisp_info.myScreen,cur_output->outputInfo->crtc,CurrentTime,*(mySett.pos_val+2*j),*(mySett.pos_val+1+2*j),myDisp_info.myScreen->modes[z].id,RR_Rotate_0,&(myDisp_info.myScreen->outputs[cur_output->outputNum]),1);
								
							}
							else {
								printf("Shrinking crtc first\n");
 							XRRSetScreenSize (myDisp_info.myDisp, myDisp_info.myWin,*(mySett.disp_val),*(mySett.disp_val+1),*(mySett.disp_val+2),*(mySett.disp_val+3));
							xrrset_status = XRRSetCrtcConfig(myDisp_info.myDisp,myDisp_info.myScreen,cur_output->outputInfo->crtc,CurrentTime,*(mySett.pos_val+2*j),*(mySett.pos_val+1+2*j),myDisp_info.myScreen->modes[z].id,RR_Rotate_0,&(myDisp_info.myScreen->outputs[cur_output->outputNum]),1);

							}
							*/
							// TODO Assuming only one output per crtc
							printf("XRRSetCrtcConfig success? %d\n",xrrset_status);
							// Have to change screen size now

						}
					}
				}
			}
			free(edid_string);
		}
		cur_output = cur_output->next;
	}

// 	free(edid_val);
// 	free(resolution_str);
// 	free(pos_val);
// 	free_output_list(head);

}

void fetch_display_status(struct disp_info *myDisp_info){
	// Loads current display status
	// Loads myDisp, myScreen, and edid_atom
	// TODO Free myDisp,myWin, myScreen, atom??
	char *display_name = 0; // TODO is this correct?
	Bool only_if_exists = 1;
	char *edid_name = "EDID";

	myDisp_info->myDisp = XOpenDisplay(display_name);
	myDisp_info->myWin = DefaultRootWindow(myDisp_info->myDisp);
	// TODO Assume 1 screen?
	// TODO Gotta free myScreen probably myCrtc as well XRRFree-something
	myDisp_info->myScreen = XRRGetScreenResources(myDisp_info->myDisp,myDisp_info->myWin);
	myDisp_info->edid_atom = XInternAtom(myDisp_info->myDisp,edid_name,only_if_exists);
}

void  construct_output_list(Display *myDisp, XRRScreenResources *myScreen, struct conOutputs **head, int *num_conn_outputs){
	// Constructs a linked list containing output information
	// TODO: Should probably optmize usage of global variables here
	// Outputs:	cur_output:		pointer to head of linked list
	// 		num_conn_outputs:	length of linked list	
	int i;
	struct conOutputs *new_output;
	struct conOutputs *cur_output;
	XRROutputInfo *myOutput;

	*head = NULL; cur_output = NULL;

	*num_conn_outputs = 0;
	for (i=0;i<myScreen->noutput;++i) {
		myOutput = XRRGetOutputInfo(myDisp,myScreen,myScreen->outputs[i]);
		// printf("Name: %s Connection %d\n",myOutput->name,myOutput->connection);
		if (!myOutput->connection) {
			// TODO Free the list and int
			new_output = (struct conOutputs*) malloc(sizeof(struct conOutputs));
			printf("%d\n",i);
			new_output->outputInfo = myOutput;
			new_output->next = *head;
			printf("new_output->next: %d\n",new_output->next);
			new_output->outputNum = i;
			*head = new_output;
			*num_conn_outputs = *num_conn_outputs + 1;
			printf("Oh where oh where\n");
		}
	}
	printf("Done constructing linked list of outputs\n");
}

void free_output_list(struct conOutputs *cur_output){
	//TODO Free myOutput
	struct conOutputs *temp;

	while(cur_output){
		temp = cur_output->next;
		free(cur_output);
		cur_output = temp;
	}

	return;
}

void load_val_from_config(config_setting_t *profile_group, struct conf_sett_struct *mySett, int *num_out_pp){
	// Loads all settings from one profile
	// Inputs: list
	// Outputs: edid_val, resolution_str, pos_val
	// 	num_out_pp: how many saved outputs per profile there are
	int i;
	config_setting_t *pos_group, *group, *mon_group;
	
	printf("am i here\n");
	mon_group = config_setting_lookup(profile_group,"Monitors");
	printf("Checking group %d\n",mon_group);
	*num_out_pp = config_setting_length(mon_group);
	mySett->edid_val = (const char **) malloc(*num_out_pp * sizeof(const char *));
	mySett->resolution_str = (const char **) malloc(*num_out_pp * sizeof(const char *));
	mySett->pos_val = (int *) malloc(2*(*num_out_pp) * sizeof(int));
	mySett->disp_val = (int *) malloc(4*sizeof(int));
	printf("am i here\n");

	for(i=0;i<*num_out_pp;++i) {
		group = config_setting_get_elem(mon_group,i);
		printf("Checking group %d\n",group);
		pos_group = config_setting_lookup(group,"pos");
		config_setting_lookup_string(group,"EDID",mySett->edid_val+i);
		config_setting_lookup_string(group,"resolution",mySett->resolution_str+i);
		config_setting_lookup_int(pos_group,"x",mySett->pos_val+2*i);
		config_setting_lookup_int(pos_group,"y",mySett->pos_val+2*i+1);
		printf("Loaded values: \n");
		printf("EDID: %s\n",*(mySett->edid_val+i));
		printf("Resolution: %s\n",*(mySett->resolution_str+i));
		printf("Pos: x=%d y=%d\n",*(mySett->pos_val+2*i),*(mySett->pos_val+2*i+1));
	}

	printf("am i here\n");
	group = config_setting_lookup(profile_group,"Screen");
	config_setting_lookup_int(group,"width",mySett->disp_val);
	config_setting_lookup_int(group,"height",mySett->disp_val+1);
	config_setting_lookup_int(group,"widthMM",mySett->disp_val+2);
	config_setting_lookup_int(group,"heightMM",mySett->disp_val+3);
}

void edid_to_string(unsigned char *edid, unsigned long nitems, unsigned char **edid_string){
	//Inputs: edid: the bits return from X11 server
	//Outputs: edid_string: edid in string form
	int z;
	
	*edid_string = (unsigned char *) malloc((nitems+1) * sizeof(char));
	for (z=0;z<nitems;++z) {
		if (edid[z] == '\0') {
			*((*edid_string)+z) = '0';
		}
		else {
			*((*edid_string)+z) = edid[z];
		}
		//printf("\n");
		//printf("%c",*((*edid_string)+z));
	}
	*((*edid_string)+z) = '\0';
	// printf("Finished edid_to_string\n");
}


void save_profile(config_t *config_p, config_setting_t *profile_group){
	int i,j,k,l,screen;
	XRRCrtcInfo* myCrtc; 
	config_setting_t *pos_group, *group;
	struct disp_info myDisp_info;
	unsigned char *edid, *edid_string;
	int actual_format;
	unsigned long nitems,bytes_after;
	Atom actual_type;
	XRROutputInfo *myOutput;
	config_setting_t *edid_setting,*resolution_setting,*pos_x_setting,*pos_y_setting,*disp_group,*disp_width_setting,*disp_height_setting,*disp_widthMM_setting,*disp_heightMM_setting;
	config_setting_t *mon_group,*output_group;

	// Fetch current configuration info
	// fetch_display_status();
	fetch_display_status(&myDisp_info);
	screen = DefaultScreen(myDisp_info.myDisp);
	//printf("Fetched display status\n");

	myCrtc = (XRRCrtcInfo*) malloc(myDisp_info.myScreen->ncrtc * sizeof(XRRCrtcInfo));
	for(k=0;k<myDisp_info.myScreen->ncrtc;++k) {
		myCrtc[k] = *XRRGetCrtcInfo(myDisp_info.myDisp,myDisp_info.myScreen,(myDisp_info.myScreen)->crtcs[k]);
	}

	printf("Number of outputs: %d\n", myDisp_info.myScreen->noutput);

	disp_group = config_setting_add(profile_group,"Screen",CONFIG_TYPE_GROUP);
	disp_width_setting = config_setting_add(disp_group,"width",CONFIG_TYPE_INT);
	disp_height_setting = config_setting_add(disp_group,"height",CONFIG_TYPE_INT);
	disp_widthMM_setting = config_setting_add(disp_group,"widthMM",CONFIG_TYPE_INT);
	disp_heightMM_setting = config_setting_add(disp_group,"heightMM",CONFIG_TYPE_INT);
	//printf("Done with setting up config\n");

	config_setting_set_int(disp_width_setting,DisplayWidth(myDisp_info.myDisp,screen));
	config_setting_set_int(disp_height_setting,DisplayHeight(myDisp_info.myDisp,screen));
	config_setting_set_int(disp_widthMM_setting,DisplayWidthMM(myDisp_info.myDisp,screen));
	config_setting_set_int(disp_heightMM_setting,DisplayHeightMM(myDisp_info.myDisp,screen));

	mon_group = config_setting_add(profile_group,"Monitors",CONFIG_TYPE_GROUP);
	for (i=0;i<myDisp_info.myScreen->noutput;++i) {
		myOutput = XRRGetOutputInfo(myDisp_info.myDisp,myDisp_info.myScreen,(myDisp_info.myScreen)->outputs[i]);
		printf("Name: %s Connection %d\n",myOutput->name,myOutput->connection);
		if (!myOutput->connection) {

			// Setup config
			output_group = config_setting_add(mon_group,myOutput->name,CONFIG_TYPE_GROUP);
			edid_setting = config_setting_add(output_group,"EDID",CONFIG_TYPE_STRING);
			resolution_setting = config_setting_add(output_group,"resolution",CONFIG_TYPE_STRING);
			pos_group = config_setting_add(output_group,"pos",CONFIG_TYPE_GROUP);
			pos_x_setting = config_setting_add(pos_group,"x",CONFIG_TYPE_INT);
			pos_y_setting = config_setting_add(pos_group,"y",CONFIG_TYPE_INT);

			XRRGetOutputProperty(myDisp_info.myDisp,(myDisp_info.myScreen)->outputs[i],myDisp_info.edid_atom,0,100,False,False,AnyPropertyType,&actual_type,&actual_format,&nitems,&bytes_after,&edid);
			//printf("%d\n",nitems);
			if (nitems) {
				// Make edid into string
				edid_to_string(edid,nitems,&edid_string);
				//printf("edid %s\n",edid_string);
				// Get current mode
				for (j=0;j<myOutput->nmode;++j) {
					// printf("Mode: %d\n",myOutput->modes[j]);
					// Get crct configuration
					for(k=0;k<myDisp_info.myScreen->ncrtc;++k) {
						// printf("Crtc mode id: %d\n",myCrtc[k].mode);
						if (myOutput->modes[j] == myCrtc[k].mode) {
							// Save current output and mode id

							config_setting_set_string(edid_setting,edid_string);
							free(edid_string);

							// Have mode number, must find name of mode
							for (l=0;l<myDisp_info.myScreen->nmode;++l) {
								if (myDisp_info.myScreen->modes[l].id == myOutput->modes[j]) {
									config_setting_set_string(resolution_setting,myDisp_info.myScreen->modes[l].name);
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
	config_write_file(config_p,config_file);
	printf("Destroying config\n");
	config_destroy(config_p);
}

