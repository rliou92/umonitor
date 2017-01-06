#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <inttypes.h>
#include <libconfig.h>
#include <unistd.h>

/* This program is intended to manage monitor hotplugging. It is able to save
 * the current display settings and load them automatically when monitors are
 * added/removed. In the future might be able run a script as well when
 * monitors are hotplugged to extend this program's functionality. Should there
 * be a Wayland version of this?
 *
 * README
 * 1. Set up the display configuration however you want. 
 * 2. Save the settings into a profile called profile_name:
 * 	display --save profile_name
 * Configuration settings are saved in the same location as the program in a file called "umon.conf". This will be changed to follow the XDG config directory in the future.
 * 3. In order to automatically detect and manage display settings, you can run:
 * 	display --test-event
 * 4. Deleting a profile from the configuration file can be done by
 * 	display --delete profile_name
 * 5. Manually loading a profile:
 * 	display --load profile_name
 *
 * Ricky
 */

/* TODO:	README
 * 		test duplicate displays
 * 		XDG config file storage
 */

/* Structure for saving the list of connected outputs
 */
struct conOutputs {
	XRROutputInfo *outputInfo;
	int outputNum;
	struct conOutputs *next;
	char *edid_string;
};

/* Structure for loading and saving the configuration file
 */
struct conf_sett_struct {
	const char **edid_val,**resolution_str;
	int *pos_val,*disp_val;
};

/* Structure for saving the current display settings
 */
struct disp_info {
	Display *myDisp;
	Window myWin;
	XRRScreenResources *myScreen;
	Atom edid_atom;
};

char config_file[] = "umon.conf";
int verbose = 0; 

void load_profile(struct disp_info *myDisp_info, struct conOutputs *head, struct conf_sett_struct *mySett, int num_out_pp, config_setting_t *profile_group);
void fetch_display_status(struct disp_info *myDisp_info);
void free_display_status(struct disp_info *myDisp_info);
void construct_output_list(struct disp_info *, struct conOutputs **head,int *);
void free_output_list(struct conOutputs *);
void load_val_from_config(config_setting_t *list, struct conf_sett_struct *mySett, int *num_out_pp);
void free_val_from_config(struct conf_sett_struct *mySett);
void edid_to_string(unsigned char *edid, unsigned long nitems, unsigned char *edid_string);
void save_profile(config_t *config,config_setting_t *,struct disp_info *, struct conOutputs *);
void listen_for_event(config_t *config_p,struct disp_info *myDisp_info,int,struct conOutputs *head,struct conf_sett_struct *mySett);

int main(int argc, char **argv) {
	int save = 0;
	int load = 0;
	int delete = 0;
	int test_event = 0;
	int cfg_idx,i,num_out_pp,num_conn_outputs;
	config_setting_t *root, *profile_group;
	struct disp_info myDisp_info;
	struct conOutputs *head;
	struct conf_sett_struct mySett;
	config_t config; 
	char* profile_name;

	config_init(&config);
	// printf("save: %d\n",save);

	for(i=1;i<argc;++i) {
		if (!strcmp("--save", argv[i])) {
			profile_name = argv[++i];
			save = 1;
		}
		else if (!strcmp("--load", argv[i])) {
			profile_name = argv[++i];
			load = 1;
		}
		else if (!strcmp("--delete", argv[i])){
			profile_name = argv[++i];
			delete = 1;
		}
		else if (!strcmp("--verbose", argv[i])){
			verbose = 1;
		}
		else if (!strcmp("--test-event", argv[i])){
			test_event = 1;
		}
		else {
			printf("Unrecognized arguments");
			exit(5);
		}
	}

	fetch_display_status(&myDisp_info);
	construct_output_list(&myDisp_info,&head,&num_conn_outputs); // Free


	if (config_read_file(&config, config_file)) {
		if (verbose) printf("Detected existing configuration file\n");
		// Existing config file to load setting values
		if (load) {
			if (verbose) printf("Loading profile: %s\n", profile_name);
			// Load profile
			profile_group = config_lookup(&config,profile_name);

			if (profile_group != NULL) {
				load_val_from_config(profile_group,&mySett,&num_out_pp);
				load_profile(&myDisp_info,head,&mySett,num_out_pp,profile_group);
				free_val_from_config(&mySett);
			}
			else {
				printf("No profile found\n");
				exit(2);
			}
		}

		if (save || delete) {
			profile_group = config_lookup(&config,profile_name);
			if (profile_group != NULL) {
				// Overwrite existing profile
				cfg_idx = config_setting_index(profile_group);
				root = config_root_setting(&config);
				//printf("Configuration index: %d\n",cfg_idx);
				config_setting_remove_elem(root,cfg_idx);
				if (verbose) printf("Deleted profile %s\n", profile_name);
			}
		}
	}

	else {
		if (load || delete) {
			printf("No file to load or delete\n");
			exit(1); // No file to load or delete
		}
	}

	if (save) {
		if (verbose) printf("Saving current settings into profile: %s\n", profile_name);
		// Always create the new profile group because above code has already deleted it if it existed before
		root = config_root_setting(&config);
		profile_group = config_setting_add(root,profile_name,CONFIG_TYPE_GROUP);
		save_profile(&config,profile_group,&myDisp_info,head);
	}

	if (test_event){
		if (config_read_file(&config, config_file)) {
			listen_for_event(&config,&myDisp_info,num_conn_outputs,head,&mySett); // Will not use new configuration file if it is changed 
		}
		else {
			printf("No file to load when event is triggered\n");
			exit(3);
		}
	}

	free_output_list(head);
	config_destroy(&config);
	if (verbose) printf("Done freeing config\n");
	free_display_status(&myDisp_info);

}

void listen_for_event(config_t *config_p,struct disp_info *myDisp_info,int num_conn_outputs,struct conOutputs *head,struct conf_sett_struct *mySett){

	/* Listens for a screen change event. When the event occurs, find the profile that matches the current configuration. When the matching profile is found, call load_profile to apply the setting
	 * Inputs;	config_p	pointer to the configuration file that is being read
	 * Outputs:	none
	 */	

	int i,k,matches,event_num,event_base,ignore,num_out_pp,num_profiles;
	XEvent event;
	config_setting_t *root, *profile_group;
	struct conOutputs *cur_output;
	char *profile_match;
	// struct disp_info myDisp_info;
	// struct conf_sett_struct mySett;

	//fetch_display_status(&myDisp_info); // No need to free?
	XRRQueryExtension(myDisp_info->myDisp,&event_base,&ignore);
	XRRSelectInput(myDisp_info->myDisp,myDisp_info->myWin,RRScreenChangeNotifyMask);

	while (1){
		if (verbose) printf("Listening for event\n");
		XNextEvent(myDisp_info->myDisp, (XEvent *) &event);

		if (verbose) printf ("Event received, type = %d\n", event.type);
		XRRUpdateConfiguration(&event);
		event_num = event.type - event_base;
		if (event_num == RRScreenChangeNotify){
			if (verbose) printf("Screen event change received\n");
			// Wait a bit?
			sleep(1);
			// Need to find out which profile to load
			// Get list of connected outputs
			// Get list of available profiles
			// Cannot free config
			root = config_root_setting(config_p);
			num_profiles = config_setting_length(root);
			if (verbose) printf("Number of profiles in configuration file: %d\n", num_profiles);
			for (i=0;i<num_profiles;++i){
				// For each profile
				profile_group = config_setting_get_elem(root,i); // I believe not freeing these config_setting_t won't become a memory leak
				profile_match = config_setting_name(profile_group); // Is assuming that these functions are not allocating additional memory
				// Get profile group of profile outputs
				load_val_from_config(profile_group,mySett,&num_out_pp);
				// Config data is now stored in mySett (edid_val, resolution_str, pos_val)

				// See if the list of connected outputs match the list of profile outputs
				// Assuming there is only one edid per profile in configuration
				matches = 0;
				if (verbose) printf("num_conn_outputs: %d\n",num_conn_outputs);
				if (verbose) printf("num_out_pp: %d\n",num_out_pp);
				if (num_conn_outputs == num_out_pp) {
					for (k=0;k<num_out_pp;++k) {
						for (cur_output=head;cur_output;cur_output=cur_output->next) {
							// Fetch edid and turn it into a string
							// XRRGetOutputProperty(myDisp_info.myDisp,myDisp_info.myScreen->outputs[cur_output->outputNum],myDisp_info.edid_atom,0,100,False,False,AnyPropertyType,&actual_type,&actual_format,&nitems,&bytes_after,&edid);
							// Convert edid to how it is stored
							// Assuming edid exists!!
							// if (nitems) {
							// Make edid into string
							// edid_to_string(edid,nitems,&edid_string);
							// If match - load!
							if (!strcmp(cur_output->edid_string,*(mySett->edid_val+k))){
								++matches;
								// printf("Found a match! Match: %d\n",matches);
							}
							// cur_output = cur_output->next;
						}
					}

					if (matches == num_conn_outputs) {
						if (verbose) printf("Profile %s matches current configuration\n", profile_match);
						load_profile(myDisp_info,head,mySett,num_out_pp,profile_group);
					}
					else {
						if (verbose) printf("Profile %s does not match current configuration\n", profile_match);
					}
				}

				else {
					if (verbose) printf("Profile %s does not match current configuration\n", profile_match);
				}

				free_val_from_config(mySett);
			}

		}

		free_display_status(myDisp_info);
		free_output_list(head);
		// Prepare to listen for another event
		fetch_display_status(myDisp_info);
		construct_output_list(myDisp_info,&head,&num_conn_outputs); // Free

		XRRSelectInput(myDisp_info->myDisp,myDisp_info->myWin,RRScreenChangeNotifyMask);

	}
		if (verbose) printf("Done listening for event\n");
}

void load_profile(struct disp_info *myDisp_info, struct conOutputs *head, struct conf_sett_struct *mySett, int num_out_pp, config_setting_t *profile_group){

	// Output list, configuration file values
	/* Applies profile settings to given display given the profile
	 * Inputs: 	myDisp_info	Display structure information
	 * 		profile_group	Profile containing the settings that should be loaded
	 * Outputs: 	none
	 */

	int j, z, xrrset_status, screen;
	struct conOutputs *cur_output;
	// struct conf_sett_struct mySett;

	// list = config_lookup(&config,profile_name);
	// Construct list of current EDIDS
	// Fetch current configuration info
	// Assume display info is already fetched

	// construct_output_list(myDisp_info,&head,&num_conn_outputs); // Free
	// cur_output = head;

	// load_val_from_config(profile_group,&mySett,&num_out_pp);

	// Now I have both lists, so can do a double loops around list of connected monitors and the saved monitors
	// num_conn_outputs is the number of connected outputs
	// l is the number of loaded monitors
	if (verbose) printf("Trying to find matching monitor...\n");
	for (cur_output=head;cur_output;cur_output=cur_output->next) {
		// Loop around connected outputs
		// Get edid
		 //printf("%d\n",cur_output->outputNum);
		// XRRGetOutputProperty(myDisp_info.myDisp,myDisp_info.myScreen->outputs[cur_output->outputNum],myDisp_info.edid_atom,0,100,False,False,AnyPropertyType,&actual_type,&actual_format,&nitems,&bytes_after,&edid);
		// Convert edid to how it is stored
		if (cur_output->edid_string) {
			 //printf("%s\n",cur_output->edid_string);

			for (j=0;j<num_out_pp;++j) {
				// Now loop around loaded outputs
				//printf("Current output edid: %s\n", cur_output->edid_string);
				// printf("Matching with loaded output %s\n", *(mySett->edid_val+j));
				if (!strcmp(cur_output->edid_string,*(mySett->edid_val+j))){
					// printf("Match found!!!\n");
					// Now I need to find the current output mode and then change them
					// Really I only need the display mode (resolution) and ? (position)
					// So I'm going to temporarily find them out in the following just to find the name and then remove the code
					// Screen has a list of modes, and so does the output
					// Must find screen name first
					for (z=0;z<myDisp_info->myScreen->nmode;++z) {
						if (!strcmp(myDisp_info->myScreen->modes[z].name,*(mySett->resolution_str+j))) {
							// printf("I know the mode!: %s \n",*(mySett.resolution_str+j));
							// printf("Position %dx%d\n",*(mySett.pos_val+2*j),*(mySett.pos_val+1+2*j));
							// Set
							screen = DefaultScreen(myDisp_info->myDisp);
							// printf("Screen: %d\n", screen);
							// printf("DisplayWidth %d\n", *(mySett.disp_val));
							// printf("DisplayHeight %d\n", *(mySett.disp_val+1));
							// printf("DisplayWidthMM %d\n", *(mySett.disp_val+2));
							// printf("DisplayHeightMM %d\n", *(mySett.disp_val+3));
							// Looks like the way xrandr does it is to disable crtcs first
							XRRSetCrtcConfig (myDisp_info->myDisp, myDisp_info->myScreen, cur_output->outputInfo->crtc, CurrentTime, 0, 0, None, RR_Rotate_0, NULL, 0);
 							XRRSetScreenSize (myDisp_info->myDisp, myDisp_info->myWin,*(mySett->disp_val),*(mySett->disp_val+1),*(mySett->disp_val+2),*(mySett->disp_val+3));
							// TODO Assuming only one output per crtc
							// For duplicating displays I think this will work by creating two crtcs on top of each other
							// Probably not ideal, but my implementation of this program has been flawed from the beginning in how I store the configuration file, my understanding at that time of the X11 protocol was not so clear
							// printf("number of outputs per crtc: %d\n",cur_output->outputInfo->crtc.crtc_info->noutput);
							xrrset_status = XRRSetCrtcConfig(myDisp_info->myDisp,myDisp_info->myScreen,cur_output->outputInfo->crtc,CurrentTime,*(mySett->pos_val+2*j),*(mySett->pos_val+1+2*j),myDisp_info->myScreen->modes[z].id,RR_Rotate_0,&(myDisp_info->myScreen->outputs[cur_output->outputNum]),1);
							if (verbose) printf("Setting monitor %s\n",cur_output->outputInfo->name);

						}
					}
				}
			}
		}
		// cur_output = cur_output->next;
	}

 	// free_output_list(head);
	// free_val_from_config(&mySett);
	if (verbose) printf("Done loading profile\n");

}

void fetch_display_status(struct disp_info *myDisp_info){

	/* Loads current display status into myDisp_info struct
	 * Inputs: 	none
	 * Outputs:	myDisp_info	structure containing display information
	 */

	char *display_name = 0; // Is this correct?
	Bool only_if_exists = 1;
	char *edid_name = "EDID";

	myDisp_info->myDisp = XOpenDisplay(display_name);
	myDisp_info->myWin = DefaultRootWindow(myDisp_info->myDisp);
	// Assuming only one screen
	myDisp_info->myScreen = XRRGetScreenResources(myDisp_info->myDisp,myDisp_info->myWin);
	myDisp_info->edid_atom = XInternAtom(myDisp_info->myDisp,edid_name,only_if_exists);
	if (verbose) printf("Done fetching display status\n");
}

void free_display_status(struct disp_info *myDisp_info){
	XFree(myDisp_info->myDisp);
	XFree(myDisp_info->myScreen);
	//free(myDisp_info);
	if (verbose) printf("Done freeing display status\n");
}

void  construct_output_list(struct disp_info *myDisp_info, struct conOutputs **head,int *num_conn_outputs){

	/* Constructs a linked list containing output information
	 * Inputs:	myDisp_info			current display 
	 * 		myScreen		current screen
	 * Outputs:	head			pointer to head of linked list
	 */

	int i;
	struct conOutputs *new_output;
	struct conOutputs *cur_output;
	XRROutputInfo *myOutput;
	unsigned char *edid, *edid_string;
	int actual_format;
	unsigned long nitems,bytes_after;
	Atom actual_type;

	*head = NULL; cur_output = NULL;

	*num_conn_outputs = 0;
	for (i=0;i<myDisp_info->myScreen->noutput;++i) {
		myOutput = XRRGetOutputInfo(myDisp_info->myDisp,myDisp_info->myScreen,myDisp_info->myScreen->outputs[i]);
		// printf("Name: %s Connection %d\n",myOutput->name,myOutput->connection);
		if (!myOutput->connection) {
			new_output = (struct conOutputs*) malloc(sizeof(struct conOutputs));
			// printf("%d\n",i);
			new_output->outputInfo = myOutput;
			new_output->next = *head;
			// printf("new_output->next: %d\n",new_output->next);
			new_output->outputNum = i;
			*head = new_output;
			XRRGetOutputProperty(myDisp_info->myDisp,myDisp_info->myScreen->outputs[i],myDisp_info->edid_atom,0,100,False,False,AnyPropertyType,&actual_type,&actual_format,&nitems,&bytes_after,&edid);
			if (nitems) {
				edid_string = (char *) malloc((nitems+1)*sizeof(char));
				edid_to_string(edid,nitems,edid_string);
				new_output->edid_string = edid_string;
			}
			else {
				new_output->edid_string = NULL;
			}
			//printf("Oh where oh where\n");
			*num_conn_outputs = *num_conn_outputs + 1; 
		}
	}
	if (verbose) printf("Number of outputs: %d\n", *num_conn_outputs);
	if (verbose) printf("Done constructing linked list of outputs\n");
}

void free_output_list(struct conOutputs *cur_output){

	/* Frees the connected output list
	 * Inputs:	cur_output	pointer to the head of the connected output linked list
	 * Outputs:	none
	 */
	
	struct conOutputs *temp;

	while(cur_output){
		temp = cur_output->next;
		XFree(cur_output->outputInfo); // Umm 
		free(cur_output->edid_string);
		free(cur_output);
		cur_output = temp;
	}

	if (verbose) 	printf("Done freeing linked list of outputs\n");
}

void load_val_from_config(config_setting_t *profile_group, struct conf_sett_struct *mySett, int *num_out_pp){

	/* Loads all configuration settings from one profile into mySett
	 * Inputs:	profile_group	configuration group that is to be loaded
	 * Outputs: 	mySett		structure containing the values of the profile
	 * 		num_out_pp	number of outputs per profile
	 */
	
	int i;
	config_setting_t *pos_group, *group, *mon_group;
	
	// printf("am i here\n");
	mon_group = config_setting_lookup(profile_group,"Monitors");
	// printf("Checking group %d\n",mon_group);
	*num_out_pp = config_setting_length(mon_group);
	mySett->edid_val = (const char **) malloc(*num_out_pp * sizeof(const char *));
	mySett->resolution_str = (const char **) malloc(*num_out_pp * sizeof(const char *));
	mySett->pos_val = (int *) malloc(2*(*num_out_pp) * sizeof(int));
	mySett->disp_val = (int *) malloc(4*sizeof(int));

	for(i=0;i<*num_out_pp;++i) {
		group = config_setting_get_elem(mon_group,i);
		// printf("Checking group %d\n",group);
		pos_group = config_setting_lookup(group,"pos");
		config_setting_lookup_string(group,"EDID",mySett->edid_val+i);
		config_setting_lookup_string(group,"resolution",mySett->resolution_str+i);
		config_setting_lookup_int(pos_group,"x",mySett->pos_val+2*i);
		config_setting_lookup_int(pos_group,"y",mySett->pos_val+2*i+1);
		// printf("Loaded values: \n");
		// printf("EDID: %s\n",*(mySett->edid_val+i));
		// printf("Resolution: %s\n",*(mySett->resolution_str+i));
		// printf("Pos: x=%d y=%d\n",*(mySett->pos_val+2*i),*(mySett->pos_val+2*i+1));
	}

	group = config_setting_lookup(profile_group,"Screen");
	config_setting_lookup_int(group,"width",mySett->disp_val);
	config_setting_lookup_int(group,"height",mySett->disp_val+1);
	config_setting_lookup_int(group,"widthMM",mySett->disp_val+2);
	config_setting_lookup_int(group,"heightMM",mySett->disp_val+3);

	if (verbose) printf("Done loading values from configuration file\n");
}

void free_val_from_config(struct conf_sett_struct *mySett){
	free(mySett->edid_val);
	free(mySett->resolution_str);
	free(mySett->pos_val);
	free(mySett->disp_val);

	if (verbose) 	printf("Done freeing values from configuration file\n");
}

void edid_to_string(unsigned char *edid, unsigned long nitems, unsigned char *edid_string){

	/* Converts the edid that is returned from the X11 server into a string
	 * Inputs: 	edid	 	the bits return from X11 server
	 * Outputs: 	edid_string	 edid in string form
	 */

	int z;
	
	for (z=0;z<nitems;++z) {
		if (edid[z] == '\0') {
			edid_string[z] = '0';
		}
		else {
			edid_string[z] = edid[z];
		}
		//printf("\n");
		//printf("%c",*((*edid_string)+z));
	}
	edid_string[z] = '\0';

	if (verbose) printf("Finished edid_to_string\n");
}


void save_profile(config_t *config_p, config_setting_t *profile_group, struct disp_info *myDisp_info, struct conOutputs *head){

	/* Saves the current display settings into the configuration file
	 * Inputs:	config_p	pointer to the configuration file
	 * 		profile_group	the profile to which the current display settings are saved
	 */

	int i,j,k,l,screen,num_conn_outputs;
	XRRCrtcInfo **myCrtc; 
	config_setting_t *pos_group, *group;
	// struct disp_info myDisp_info;
	struct conOutputs *cur_output;
	config_setting_t *edid_setting,*resolution_setting,*pos_x_setting,*pos_y_setting,*disp_group,*disp_width_setting,*disp_height_setting,*disp_widthMM_setting,*disp_heightMM_setting;
	config_setting_t *mon_group,*output_group;

	// Fetch current configuration info
	// fetch_display_status();
	// fetch_display_status(&myDisp_info);
	screen = DefaultScreen(myDisp_info->myDisp);
	//printf("Fetched display status\n");

	myCrtc = (XRRCrtcInfo**) malloc(myDisp_info->myScreen->ncrtc * sizeof(XRRCrtcInfo *));
	for(k=0;k<myDisp_info->myScreen->ncrtc;++k) {
		myCrtc[k] = XRRGetCrtcInfo(myDisp_info->myDisp,myDisp_info->myScreen,(myDisp_info->myScreen)->crtcs[k]);
	}

	// construct_output_list(myDisp_info, &head, &num_conn_outputs);
	// cur_output = head;


	disp_group = config_setting_add(profile_group,"Screen",CONFIG_TYPE_GROUP);
	disp_width_setting = config_setting_add(disp_group,"width",CONFIG_TYPE_INT);
	disp_height_setting = config_setting_add(disp_group,"height",CONFIG_TYPE_INT);
	disp_widthMM_setting = config_setting_add(disp_group,"widthMM",CONFIG_TYPE_INT);
	disp_heightMM_setting = config_setting_add(disp_group,"heightMM",CONFIG_TYPE_INT);

	config_setting_set_int(disp_width_setting,DisplayWidth(myDisp_info->myDisp,screen));
	config_setting_set_int(disp_height_setting,DisplayHeight(myDisp_info->myDisp,screen));
	config_setting_set_int(disp_widthMM_setting,DisplayWidthMM(myDisp_info->myDisp,screen));
	config_setting_set_int(disp_heightMM_setting,DisplayHeightMM(myDisp_info->myDisp,screen));

	// crtc_group = config_setting_add(profile_group,"Crtc",CONFIG_TYPE_GROUP);
	mon_group = config_setting_add(profile_group,"Monitors",CONFIG_TYPE_GROUP);
	// printf("Done with setting up config\n");
	for (cur_output=head;cur_output;cur_output=cur_output->next) {
		output_group = config_setting_add(mon_group,cur_output->outputInfo->name,CONFIG_TYPE_GROUP);
		edid_setting = config_setting_add(output_group,"EDID",CONFIG_TYPE_STRING);
		resolution_setting = config_setting_add(output_group,"resolution",CONFIG_TYPE_STRING);
		pos_group = config_setting_add(output_group,"pos",CONFIG_TYPE_GROUP);
		pos_x_setting = config_setting_add(pos_group,"x",CONFIG_TYPE_INT);
		pos_y_setting = config_setting_add(pos_group,"y",CONFIG_TYPE_INT);	 
		// printf("cur_output %s\n", cur_output->edid_string);

		if (cur_output->edid_string) {
			// Make edid into string
			// edid_to_string(edid,nitems,&edid_string);
			// printf("edid %s\n",cur_output->edid_string);
			// Get current mode
			for (j=0;j<cur_output->outputInfo->nmode;++j) {
				// printf("Mode: %d\n",myOutput->modes[j]);
				// Get crct configuration
				for(k=0;k<myDisp_info->myScreen->ncrtc;++k) {
					// printf("Crtc mode id: %d\n",myCrtc[k].mode);
					if (cur_output->outputInfo->modes[j] == myCrtc[k]->mode) {
						// Save current output and mode id
						config_setting_set_string(edid_setting,cur_output->edid_string);
						// free(edid_string);

						// Have mode number, must find name of mode
						for (l=0;l<myDisp_info->myScreen->nmode;++l) {
							if (myDisp_info->myScreen->modes[l].id == cur_output->outputInfo->modes[j]) {
								config_setting_set_string(resolution_setting,myDisp_info->myScreen->modes[l].name);
							}
						}

						config_setting_set_int(pos_x_setting,myCrtc[k]->x);
						config_setting_set_int(pos_y_setting,myCrtc[k]->y);
					}
				}
			}
		}
	//	cur_output = cur_output->next;
	}
	// Need to free myCrtc
	for(k=0;k<myDisp_info->myScreen->ncrtc;++k) {
		XFree(myCrtc[k]);
	}
	free(myCrtc);
	config_write_file(config_p,config_file);

	if (verbose) printf("Done saving settings to profile\n");
}


