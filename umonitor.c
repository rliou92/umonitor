#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <libconfig.h>
#include <unistd.h>
#include <X11/extensions/Xrandr.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <stdarg.h>


	/*
	 * Structure for loading and saving the configuration file
	 */
struct conf_sett_struct {
	const char **edid_val,**resolution_str;
	int *pos_val,*disp_val;
};

/*
 * Prototypes
 */
void connect_to_server(void);
//void load_val_from_config(void);
//void free_val_from_config(void);
//void load_profile(void);
//void fetch_display_status(void);
//void free_display_status(void);
//void construct_output_list(void;
//void free_output_list(void);
void edid_to_string(uint8_t *edid, int length, char **edid_string);
void save_profile(void);
void for_each_output(void (*con_enabled)(xcb_randr_get_output_info_reply_t *,
	xcb_randr_output_t *),
	void (*con_disabled)(xcb_randr_get_output_info_reply_t *),
	void (*discon)(void));
void output_info_to_config(xcb_randr_get_output_info_reply_t *,
	xcb_randr_output_t *);
void for_each_output_mode(
	void (*output_mode_action)(xcb_randr_get_crtc_info_reply_t *,
	xcb_randr_mode_t *),
xcb_randr_get_output_info_reply_t *output_info_reply,
xcb_randr_get_crtc_info_reply_t *crtc_info_reply);
void find_res_string_to_config(xcb_randr_get_crtc_info_reply_t *,
	xcb_randr_mode_t *);
void disabled_to_config(xcb_randr_get_output_info_reply_t *);
void do_nothing(void);
//void listen_for_event(void);


static xcb_generic_error_t **e;
static xcb_connection_t *c;
static xcb_screen_t *screen;
static xcb_randr_get_screen_resources_reply_t *screen_resources_reply;

config_t config;
config_setting_t *root, *profile_group;
char* profile_name;

static char config_file[] = "umon2.conf";
static int verbose = 0;

config_setting_t *edid_setting,*resolution_setting,*pos_x_setting,
	*pos_y_setting,*disp_group,*disp_width_setting,*disp_height_setting,
	*disp_widthMM_setting,*disp_heightMM_setting,*mon_group,*output_group,
	*pos_group,*status_setting;



int main(int argc, char **argv) {
	int save = 0;
	int load = 0;
	int delete = 0;
	int test_event = 0;

	int i, screenNum, cfg_idx;

	config_init(&config);

	for(i=1;i<argc;++i) {
		if (!strcmp("--save", argv[i])) {
			if (++i >= argc) {
								printf("Saving needs an argument!\n");
								exit(6);
						}
						profile_name = argv[i];
			save = 1;
		}
		else if (!strcmp("--load", argv[i])) {
			if (++i >= argc) {
								printf("Loading needs an argument!\n");
								exit(6);
						}
						profile_name = argv[i];
			load = 1;
		}
		else if (!strcmp("--delete", argv[i])){
						if (++i >= argc) {
								printf("Deleting needs an argument!\n");
								exit(6);
						}
			profile_name = argv[i];
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

	connect_to_server();


	if (config_read_file(&config, config_file)) {
		if (verbose) printf("Detected existing configuration file\n");
		// Existing config file to load setting values
		if (load) {
			if (verbose) printf("Loading profile: %s\n", profile_name);
			// Load profile
			profile_group = config_lookup(&config,profile_name);

			if (profile_group != NULL) {
				// load_val_from_config();
				//load_profile();
				// free_val_from_config();
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
				config_setting_remove_elem(root,cfg_idx);
				if (verbose) printf("Deleted profile %s\n", profile_name);
			}
		}
	}

	else {
		if (load || delete) {
			printf("No file to load or delete\n");
			exit(1);
		}
	}

	if (save) {
		if (verbose) printf("Saving current settings into profile: %s\n", profile_name);
		/*
		 * Always create the new profile group because above code has already
		 * deleted it if it existed before
		*/
		root = config_root_setting(&config);
		profile_group = config_setting_add(root,profile_name,CONFIG_TYPE_GROUP);
		save_profile();
	}

	if (test_event){
		if (config_read_file(&config, config_file)) {
			//listen_for_event(); // Will not use new configuration file if it is changed
		}
		else {
			printf("No file to load when event is triggered\n");
			exit(3);
		}
	}


/*
	xcb_randr_select_input(c, screen->root, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
  xcb_flush(c);

  while ((evt = xcb_wait_for_event(conn)) != NULL) {
    if (evt->response_type & XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE) {
      randr_evt = (xcb_randr_screen_change_notify_event_t*) evt;
      if (last_time != randr_evt->timestamp) {
        last_time = randr_evt->timestamp;
        monitor_connected = !monitor_connected;
        if (monitor_connected) {
          printf("Monitor connected\n");
        } else {
          printf("Monitor disconnected\n");
        }
      }
    }
    free(evt);
  }
  xcb_disconnect(conn);
}
*/
	//xcb_randr_get_screen_info_cookie_t get_screen_info_cookie = xcb_randr_get_screen_info (c, screen->root);


	/* report */

	/*printf ("\n");
	printf ("Informations of screen %"PRIu32":\n", screen->root);
	printf ("  width.........: %"PRIu16"\n", screen->width_in_pixels);
	printf ("  height........: %"PRIu16"\n", screen->height_in_pixels);
	printf ("  white pixel...: %"PRIu32"\n", screen->white_pixel);
	printf ("  black pixel...: %"PRIu32"\n", screen->black_pixel);
	printf ("\n");

	return 0;*/

}

void connect_to_server(){

	int i,screenNum;
	static const xcb_setup_t *setup;
	static xcb_screen_iterator_t iter;
	/* Open the connection to the X server. Use the DISPLAY environment variable */
	c = xcb_connect (NULL, &screenNum);


	/* Get the screen whose number is screenNum */

	setup = xcb_get_setup (c);
	iter = xcb_setup_roots_iterator (setup);

	// we want the screen at index screenNum of the iterator
	for (i = 0; i < screenNum; ++i) {
		xcb_screen_next (&iter);
	}

	screen = iter.data;
	if (verbose) printf("Connected to server\n");
	//screen_resources_cookie = xcb_randr_get_screen_resources(c,screen->root);

	//screen_resources_reply =
		//xcb_randr_get_screen_resources_reply(c,screen_resources_cookie,e);
}

void save_profile(){

	int j,k,num_output_modes,num_screen_modes;

	// xcb_randr_get_screen_info_cookie_t screen_info_cookie;
	// xcb_randr_get_screen_info_reply_t *screen_info_reply;
	// xcb_randr_screen_size_iterator_t screen_size_iterator;
	xcb_randr_screen_size_t screen_size;

	xcb_randr_get_screen_resources_cookie_t screen_resources_cookie;


	disp_group = config_setting_add(profile_group,"Screen",CONFIG_TYPE_GROUP);
	disp_width_setting = config_setting_add(disp_group,"width",CONFIG_TYPE_INT);
	disp_height_setting = config_setting_add(disp_group,"height",CONFIG_TYPE_INT);
	disp_widthMM_setting =
		config_setting_add(disp_group,"widthMM",CONFIG_TYPE_INT);
	disp_heightMM_setting =
		config_setting_add(disp_group,"heightMM",CONFIG_TYPE_INT);

	config_setting_set_int(disp_width_setting,screen->width_in_pixels);
	config_setting_set_int(disp_height_setting,screen->height_in_pixels);
	config_setting_set_int(disp_widthMM_setting,screen->width_in_millimeters);
	config_setting_set_int(disp_heightMM_setting,screen->height_in_millimeters);

	mon_group = config_setting_add(profile_group,"Monitors",CONFIG_TYPE_GROUP);
// Need to iterate over the outputs now

	screen_resources_cookie = xcb_randr_get_screen_resources(c,screen->root);

	screen_resources_reply =
		xcb_randr_get_screen_resources_reply(c,screen_resources_cookie,e);

	for_each_output(&output_info_to_config,&disabled_to_config,
		&do_nothing);

	if (verbose) printf("About to write to config file\n");
	config_write_file(&config,config_file);

	if (verbose) printf("Done saving settings to profile\n");

}

void do_nothing(){

}


void edid_to_string(uint8_t *edid, int length, char **edid_string){

	/*
	 * Converts the edid that is returned from the X11 server into a string
	 * Inputs: 	edid	 	the bits return from X11 server
	 * Outputs: 	edid_string	 edid in string form
	 */

	int z;

	if (verbose) printf("Starting edid_to_string\n");
	*edid_string = (char *) malloc((length+1)*sizeof(char));
	for (z=0;z<length;++z) {
		if ((char) edid[z] == '\0') {
			*(*edid_string+z) = '0';
		}
		else {
			*(*edid_string+z) = (char) edid[z];
		}
		//printf("\n");
		//printf("%c",*((*edid_string)+z));
	}
	*(*edid_string+z) = '\0';

	if (verbose) printf("Finished edid_to_string\n");
}

void for_each_output(void (*con_enabled)(xcb_randr_get_output_info_reply_t *,
	xcb_randr_output_t *),
void (*con_disabled)(xcb_randr_get_output_info_reply_t *),
void (*discon)(void)){

	int i;

	int outputs_length;

	xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;

	xcb_randr_output_t *output_p;

	output_p = xcb_randr_get_screen_resources_outputs(screen_resources_reply);
	outputs_length =
		xcb_randr_get_screen_resources_outputs_length(screen_resources_reply);

	for (i=0; i<outputs_length; ++i){

		output_info_cookie =
		xcb_randr_get_output_info(c, *output_p, XCB_CURRENT_TIME);
		output_info_reply =
			xcb_randr_get_output_info_reply (c, output_info_cookie, e);
		if (verbose) printf("Looping over output %s\n",xcb_randr_get_output_info_name(output_info_reply));
		if (!output_info_reply->connection){
			if (verbose) printf("Found output that is connected\n");

			if (output_info_reply->crtc){
				(*con_enabled)(output_info_reply,output_p);
			}
			else {
				(*con_disabled)(output_info_reply);
			}

		}
		else {
			(*discon)();
		}

		++output_p;
	}


}

void output_info_to_config(xcb_randr_get_output_info_reply_t *output_info_reply, xcb_randr_output_t *output_p){

	xcb_randr_get_crtc_info_cookie_t crtc_info_cookie;
	xcb_randr_get_output_property_cookie_t output_property_cookie;
	xcb_randr_get_output_property_reply_t *output_property_reply;
	xcb_intern_atom_cookie_t atom_cookie;
	xcb_intern_atom_reply_t *atom_reply;
	xcb_randr_get_crtc_info_reply_t *crtc_info_reply;

	char *edid_string;
	uint8_t *output_property_data;
	int output_property_length;
	uint8_t delete = 0;
	uint8_t pending = 0;
	uint8_t only_if_exists = 1;
	const char *edid_name = "EDID";
	uint16_t name_len = strlen(edid_name);

	atom_cookie = xcb_intern_atom(c,only_if_exists,name_len,edid_name);
	output_group =
		config_setting_add(mon_group,
											xcb_randr_get_output_info_name(output_info_reply),
											CONFIG_TYPE_GROUP);
	if (verbose) printf("Found output that is enabled\n");
	edid_setting =
	config_setting_add(output_group,"EDID",CONFIG_TYPE_STRING);
	resolution_setting =
	config_setting_add(output_group,"resolution",CONFIG_TYPE_STRING);
	pos_group = config_setting_add(output_group,"pos",CONFIG_TYPE_GROUP);
	pos_x_setting = config_setting_add(pos_group,"x",CONFIG_TYPE_INT);
	pos_y_setting = config_setting_add(pos_group,"y",CONFIG_TYPE_INT);
	if (verbose) printf("Finish setting up settings\n");
	atom_reply = xcb_intern_atom_reply(c,atom_cookie,e);
	output_property_cookie = xcb_randr_get_output_property(c,
		*output_p, atom_reply->atom, AnyPropertyType, 0, 100, delete,
		pending);

	crtc_info_cookie = xcb_randr_get_crtc_info(c,output_info_reply->crtc,
			XCB_CURRENT_TIME);

			if (verbose) printf("cookies done\n");
	crtc_info_reply = xcb_randr_get_crtc_info_reply(c,crtc_info_cookie,e);
	output_property_reply = xcb_randr_get_output_property_reply(
		c,output_property_cookie,e);

	if (verbose) printf("output_property_reply %d\n",output_property_reply);
	output_property_data = xcb_randr_get_output_property_data(
		output_property_reply);
	output_property_length = xcb_randr_get_output_property_data_length(
		output_property_reply);

	if (verbose) printf("Finish fetching info from server\n");
	edid_to_string(output_property_data,output_property_length,
		&edid_string);
	config_setting_set_string(edid_setting,edid_string);
	// Free edid_string?

	// Need to find the mode info now
	// Look at output crtc
	if (verbose) printf("finished edid_setting config\n");
	for_each_output_mode(&find_res_string_to_config,output_info_reply,
		crtc_info_reply);

	config_setting_set_int(pos_x_setting,crtc_info_reply->x);
	config_setting_set_int(pos_y_setting,crtc_info_reply->y);


}

void for_each_output_mode(
	void (*output_mode_action)(xcb_randr_get_crtc_info_reply_t *,
		xcb_randr_mode_t *),
xcb_randr_get_output_info_reply_t *output_info_reply,
xcb_randr_get_crtc_info_reply_t *crtc_info_reply){

	int j,num_output_modes;

	xcb_randr_mode_t *mode_id_p;

	num_output_modes =
		xcb_randr_get_output_info_modes_length(output_info_reply);
	if (verbose) printf("number of modes %d\n",num_output_modes);
	mode_id_p = xcb_randr_get_output_info_modes(output_info_reply);
	if (verbose) printf("First mode id %d\n",*mode_id_p);
	if (verbose) printf("current crtc mode id %d\n",crtc_info_reply->mode);
	for (j=0;j<num_output_modes;++j){
		(*output_mode_action)(crtc_info_reply,mode_id_p);
		++mode_id_p;
	}


}


void find_res_string_to_config(xcb_randr_get_crtc_info_reply_t *crtc_info_reply,
	xcb_randr_mode_t *mode_id_p){

	int num_screen_modes,k;
	char res_string[10];
	xcb_randr_mode_info_iterator_t mode_info_iterator;
	//if (verbose) printf("current mode id %d\n",*mode_id_p);
	if (*mode_id_p == crtc_info_reply->mode){
		if (verbose) printf("Found current mode id\n");
		// Get output info iterator
		 mode_info_iterator =
	xcb_randr_get_screen_resources_modes_iterator(screen_resources_reply);
		num_screen_modes = xcb_randr_get_screen_resources_modes_length(
			screen_resources_reply);
		 for (k=0;k<num_screen_modes;++k){
			 if (mode_info_iterator.data->id == *mode_id_p){
				 if (verbose) printf("Found current mode info\n");
				 sprintf(res_string,"%dx%d",mode_info_iterator.data->width,mode_info_iterator.data->height);
				 config_setting_set_string(resolution_setting,res_string);
			 }
			xcb_randr_mode_info_next(&mode_info_iterator);

		 }
	}


}

void disabled_to_config(xcb_randr_get_output_info_reply_t *output_info_reply){

	output_group =
		config_setting_add(mon_group,
											xcb_randr_get_output_info_name(output_info_reply),
											CONFIG_TYPE_GROUP);
	if (verbose) printf("Found output that is disabled\n");
	status_setting =
	config_setting_add(output_group,"Status",CONFIG_TYPE_STRING);
	config_setting_set_string(status_setting,"Disabled");


}
/*
void load_profile(){

	int num_out_pp;
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
}*/
