#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <inttypes.h>
#include <libconfig.h>

// TODO: document functions, which variables are input
//
struct conOutputs {
	XRROutputInfo *outputInfo;
	int outputNum;
	struct conOutputs *next;
};

Display* myDisp;
Window myWin;
XRRScreenResources* myScreen;
XRROutputInfo* myOutput;
Atom edid_atom;
int num_out_pp,num_conn_outputs,num_profiles;
// I think the following initialization syntax works
int save = 0;
int load = 0;
int delete = 0;
int test_event = 0;
int quiet = 0;
unsigned char* edid,*edid_string;
int actual_format;
unsigned long nitems,bytes_after;
Atom actual_type;
char config_file[] = "umon.conf";
char* profile_name;
config_setting_t *edid_setting,*resolution_setting,*pos_x_setting,*pos_y_setting,*list;
config_t config; 
const char **edid_val,**resolution_str;
int *pos_val;
char* edid_name = "EDID";

void load_profile(void);
void fetch_display_status(void);
struct conOutputs *construct_output_list(void);
void free_output_list(struct conOutputs *);
void load_val_from_config(void);
void edid_to_string(void);
void save_profile(void);
void listen_for_event(void);

int main(int argc, char **argv) {
	int cfg_idx;
	config_setting_t *root;

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

		profile_name = argv[2];
		printf("Profile name: %s\n", profile_name);


		if (config_read_file(&config, config_file)) {
			printf("Detected existing configuration file\n");
			// Existing config file to load setting values
			if (load) {
				// Load profile
				list = config_lookup(&config,profile_name);
				load_profile();
			}

			if (save || delete) {
				list = config_lookup(&config,profile_name);
				if (list != NULL) {
					// Overwrite existing profile
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

		// Doesn't need to be in the above conditions because will create the config file if doesn't exist
		if (save) {
			save_profile();
		}
	}

	if (!strcmp("--test-event", argv[1])){
		if (config_read_file(&config, config_file)) {
			listen_for_event();
		}
		else {
			printf("No file to load when event is triggered\n");
			exit(3);
		}
	}

}

void listen_for_event(){
	// Inputs: num_out_pp
	int i,k,j,matches,event_num,event_base,ignore;
	XEvent event;
	config_setting_t *root;
	struct conOutputs *cur_output, *head;
	char *profile_match;

	while (1){
		// Need to find out which profile to load
		
		fetch_display_status();
		XRRQueryExtension(myDisp,&event_base,&ignore);
		XRRSelectInput(myDisp,myWin,RRScreenChangeNotifyMask);
		printf("Listening for event\n");
		XNextEvent(myDisp, (XEvent *) &event);

		printf ("Event received, type = %d\n", event.type);
		XRRUpdateConfiguration(&event);
		event_num = event.type - event_base;
		if (event_num == RRScreenChangeNotify){
			printf("Screen event change received\n");
			// Get list of connected outputs
			//fetch_display_status();	

			head = construct_output_list();
			cur_output = head;
			// Get list of available profiles
			root = config_root_setting(&config);
			num_profiles = config_setting_length(root);
			printf("Num profiles: %d\n", num_profiles);
			for (i=0;i<num_profiles;++i){
				// For each profile
				printf("i=%d\n",i);
				list = config_setting_get_elem(root,i);
				profile_match = config_setting_name(list);
				// Get list of profile outputs
				load_val_from_config();
				// Config data is now stored in edid_val, resolution_str, pos_val
				printf("Profile number %d\n",i);

				// See if the list of connected outputs match the list of profile outputs
				// Assuming there is only one edid per profile in configuration
				matches = 0;
				for (k=0;k<num_out_pp;++k){
					for (j=0;j<num_conn_outputs;++j){
						// Fetch edid and turn it into a string
						XRRGetOutputProperty(myDisp,myScreen->outputs[cur_output->outputNum],edid_atom,0,100,False,False,AnyPropertyType,&actual_type,&actual_format,&nitems,&bytes_after,&edid);
						// Convert edid to how it is stored
						// Assuming edid exists!!
						// if (nitems) {
						// Make edid into string
						edid_to_string();
						// If match - load!
						if (!strcmp(edid_string,*(edid_val+k))){
							++matches;
							printf("Found a match! Match: %d\n",matches);
						}
						cur_output = cur_output->next;
					}
					cur_output = head;
					}

					if (matches == num_conn_outputs) {
						printf("Profile %s matches!!\n", profile_match );
						load_profile();
					}
				}
				free_output_list(head);
			}
		}
	}

void load_profile(){
	// Inputs: list, num_out_pp
	int i,j,z;  
	struct conOutputs *cur_output, *head;

	printf("Loading profile\n");
	// list = config_lookup(&config,profile_name);
	printf("Profile loaded\n");
	if (list != NULL) {
		printf("Fetching display status\n");
		// Construct list of current EDIDS
		// Fetch current configuration info
		fetch_display_status();

		head = construct_output_list();
		cur_output = head;

		load_val_from_config();

		// Now I have both lists, so can do a double loops arounnd list of connected monitors and the saved monitors
		// num_conn_outputs is the number of connected outputs
		// l is the number of loaded monitors
		printf("Trying to find matching monitor...\n");
		for (i=0;i<num_conn_outputs;++i) {
			// Loop around connected outputs
			// Get edid
			printf("%d\n",cur_output->outputNum);
			XRRGetOutputProperty(myDisp,myScreen->outputs[cur_output->outputNum],edid_atom,0,100,False,False,AnyPropertyType,&actual_type,&actual_format,&nitems,&bytes_after,&edid);
			// Convert edid to how it is stored
			if (nitems) {
				// printf("%s: ",edid_name);
				// Make edid into string
				edid_to_string();

				for (j=0;j<num_out_pp;++j) {
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
								XRRSetCrtcConfig(myDisp,myScreen,cur_output->outputInfo->crtc,CurrentTime,*(pos_val+2*j),*(pos_val+1+2*j),myScreen->modes[z].id,RR_Rotate_0,&myScreen->outputs[cur_output->outputNum],1);
								printf("XRRSetCrtcConfig success\n");
							}
						}
					}
				}
				free(edid_string);
			}
			cur_output = cur_output->next;
		}

		config_destroy(&config);
		free(edid_val);
		free(resolution_str);
		free(pos_val);
		free_output_list(head);
	}

	else {
		printf("No profile found\n");
		exit(2); // No profile with the specified name
	}
}

void fetch_display_status(){
	// Loads current display status
	// Loads myDisp, myScreen, and edid_atom
	// TODO Free myDisp,myWin, myScreen, atom??
	char* display_name = 0; // TODO is this correct?
	Bool only_if_exists = 1;

	myDisp = XOpenDisplay(display_name);
	myWin = DefaultRootWindow(myDisp);
	// TODO Assume 1 screen?
	// TODO Gotta free myScreen probably myCrtc as well XRRFree-something
	myScreen = XRRGetScreenResources(myDisp,myWin);
	edid_atom = XInternAtom(myDisp,edid_name,only_if_exists);
}

struct conOutputs* construct_output_list(){
	// Constructs a linked list containing output information
	// TODO: Should probably optmize usage of global variables here
	// Outputs:	cur_output:		pointer to head of linked list
	// 		num_conn_outputs:	length of linked list	
	int i;
	struct conOutputs *new_output;
	struct conOutputs *head, *cur_output;

	head = NULL; cur_output = NULL;

	num_conn_outputs = 0;
	for (i=0;i<myScreen->noutput;++i) {
		myOutput = XRRGetOutputInfo(myDisp,myScreen,myScreen->outputs[i]);
		// printf("Name: %s Connection %d\n",myOutput->name,myOutput->connection);
		if (!myOutput->connection) {
			// TODO Free the list and int
			new_output = (struct conOutputs*) malloc(sizeof(struct conOutputs));
			printf("%d\n",i);
			new_output->outputInfo = myOutput;
			new_output->next = head;
			printf("new_output->next: %d\n",new_output->next);
			new_output->outputNum = i;
			head = new_output;
			++num_conn_outputs;
			printf("Oh where oh where\n");
		}
	}
	cur_output = head;
	return cur_output;
	printf("Done constructing linked list of outputs\n");
}

void free_output_list(struct conOutputs *cur_output){
	struct conOutputs *temp;

	while(cur_output){
		temp = cur_output->next;
		free(cur_output);
		cur_output = temp;
	}

	return;
}

void load_val_from_config(){
	// Loads all settings from one profile
	// Inputs: list
	// Outputs: edid_val, resolution_str, pos_val
	// 	num_out_pp: how many saved outputs per profile there are
	int i;
	config_setting_t *pos_group, *group;
	
	num_out_pp = config_setting_length(list);
	edid_val = (const char **) malloc(num_out_pp * sizeof(const char *));
	resolution_str = (const char **) malloc(num_out_pp * sizeof(const char *));
	pos_val = (int *) malloc(2*num_out_pp * sizeof(int));
	for(i=0;i<num_out_pp;++i) {
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
}

void edid_to_string(){
	//Inputs: edid: the bits return from X11 server
	//Outputs: edid_string: edid in string form
	int z;
	
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
}


void save_profile(){
	int i,j,k,l;
	XRRCrtcInfo* myCrtc; 
	config_setting_t *pos_group, *group, *root;

	root = config_root_setting(&config);
	list = config_setting_add(root,profile_name,CONFIG_TYPE_LIST);

	// Fetch current configuration info
	fetch_display_status();

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
				edid_to_string();
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

