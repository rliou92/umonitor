#include "umonitor.h"

/*
 * Prototypes
 */
//void connect_to_server(void);
//void load_val_from_config(void);
//void free_val_from_config(void);
//void load_profile(void);
//void fetch_display_status(void);
//void free_display_status(void);
//void construct_output_list(void;
//void free_output_list(void);

//void listen_for_event(void);


//static xcb_timestamp_t timestamp;





// int num_out_pp;




int main(int argc, char **argv) {
	int save = 0;
	int load = 0;
	int delete = 0;
	int test_event = 0;
	save_profile_class *save_profile_o;

	config_t config;
	config_setting_t *root, *profile_group;
	char* profile_name;

	int i, cfg_idx;

	screen_class screen_o;

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
			VERBOSE = 1;
		}
		else if (!strcmp("--test-event", argv[i])){
			test_event = 1;
		}
		else {
			printf("Unrecognized argument: %s\n",argv[i]);
		}
	}

	screen_class_constructor(&screen_o);


	if (config_read_file(&config, CONFIG_FILE)) {
		if (VERBOSE) printf("Detected existing configuration file\n");
		// Existing config file to load setting values
		if (load) {
			if (VERBOSE) printf("Loading profile: %s\n", profile_name);
			// Load profile
			profile_group = config_lookup(&config,profile_name);

			if (profile_group != NULL) {
				// load_val_from_config();
				//load_profile(profile_group);
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
				if (VERBOSE) printf("Deleted profile %s\n", profile_name);
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
		if (VERBOSE) printf("Saving current settings into profile: %s\n", profile_name);
		/*
		 * Always create the new profile group because above code has already
		 * deleted it if it existed before
		*/
		root = config_root_setting(&config);
		profile_group = config_setting_add(root,profile_name,CONFIG_TYPE_GROUP);
		save_profile_o = (save_profile_class *) malloc(sizeof(save_profile_class));
		save_profile_class_constructor(save_profile_o,&screen_o,&config);
		save_profile_o->save_profile(save_profile_o,profile_group);
	}

	if (test_event){
		if (config_read_file(&config, CONFIG_FILE)) {
			//listen_for_event(); // Will not use new configuration file if it is changed
		}
		else {
			printf("No file to load when event is triggered\n");
			exit(3);
		}
	}
}

void for_each_output(
	void *self,
	xcb_randr_get_screen_resources_reply_t *screen_resources_reply,
	void (*callback)(void *,xcb_randr_output_t *)){

		int i;

		int outputs_length;

		xcb_randr_output_t *output_p;

		output_p = xcb_randr_get_screen_resources_outputs(screen_resources_reply);
		outputs_length =
			xcb_randr_get_screen_resources_outputs_length(screen_resources_reply);

		for (i=0; i<outputs_length; ++i){
			callback(self,output_p);
			++output_p;
		}
}

void for_each_output_mode(
  void *self,
  xcb_randr_get_output_info_reply_t *output_info_reply,
	void (*callback)(void *,xcb_randr_mode_t *)){

	int j,num_output_modes;

	xcb_randr_mode_t *mode_id_p;

	num_output_modes =
		xcb_randr_get_output_info_modes_length(output_info_reply);
	if (VERBOSE) printf("number of modes %d\n",num_output_modes);
	mode_id_p = xcb_randr_get_output_info_modes(output_info_reply);

	for (j=0;j<num_output_modes;++j){
		callback(self,mode_id_p);
		++mode_id_p;
	}

}

void edid_to_string(uint8_t *edid, int length, char **edid_string){

	/*
	 * Converts the edid that is returned from the X11 server into a string
	 * Inputs: 	edid	 	the bits return from X11 server
	 * Outputs: 	edid_string	 edid in string form
	 */

	int z;

	if (VERBOSE) printf("Starting edid_to_string\n");
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

	if (VERBOSE) printf("Finished edid_to_string\n");
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






/*
void load_profile(config_setting_t *profile_group){

	int i;

	struct conf_sett_struct mySett;
	config_setting_t *pos_group,*group,*mon_group,*res_group;
	xcb_randr_crtc_t *crtcs_p;
	xcb_randr_set_crtc_config_cookie_t *crtc_config_p;
	xcb_randr_set_crtc_config_cookie_reply_t **crtc_config_reply_pp;

	mon_group = config_setting_lookup(profile_group,"Monitors");
	// printf("Checking group %d\n",mon_group);
	num_out_pp = config_setting_length(mon_group);
	mySett.edid_val = (const char **) malloc(num_out_pp * sizeof(const char *));
	mySett.res_x = (int *) malloc(num_out_pp * sizeof(int);
	mySett.res_y = (int *) malloc(num_out_pp * sizeof(int);
	mySett.pos_x = (int *) malloc(num_out_pp * sizeof(int));
	mySett.pos_y = (int *) malloc(num_out_pp * sizeof(int));
	mySett.width = (int *) malloc(num_out_pp * sizeof(int));
	mySett.height = (int *) malloc(num_out_pp * sizeof(int));
	mySett.widthMM = (int *) malloc(num_out_pp * sizeof(int));
	mySett.heightMM = (int *) malloc(num_out_pp * sizeof(int));

	for(i=0;i<num_out_pp;++i) {
		group = config_setting_get_elem(mon_group,i);
		// printf("Checking group %d\n",group);
		res_group = config_setting_lookup(group,"resolution")
		pos_group = config_setting_lookup(group,"pos");
		config_setting_lookup_string(group,"EDID",mySett.edid_val+i);
		config_setting_lookup_int(res_group,"x",mySett.res_x+i);
		config_setting_lookup_int(res_group,"y",mySett.res_y+i);
		config_setting_lookup_int(pos_group,"x",mySett.pos_x+i);
		config_setting_lookup_int(pos_group,"y",mySett.pos_y+i);
		// printf("Loaded values: \n");
		// printf("EDID: %s\n",*(mySett->edid_val+i));
		// printf("Resolution: %s\n",*(mySett->resolution_str+i));
		// printf("Pos: x=%d y=%d\n",*(mySett->pos_val+2*i),*(mySett->pos_val+2*i+1));
	}

	group = config_setting_lookup(profile_group,"Screen");
	config_setting_lookup_int(group,"width",mySett.width);
	config_setting_lookup_int(group,"height",mySett.height);
	config_setting_lookup_int(group,"widthMM",mySett.widthMM);
	config_setting_lookup_int(group,"heightMM",mySett.heightMM);

	if (VERBOSE) printf("Done loading values from configuration file\n");


	 * Plan of attack
	 * 1. Disable all crtcs
	 * 2. Resize the screen
	 * 3. Enabled desired crtcs


  screen_resources_cookie = xcb_randr_get_screen_resources(c,screen->root);

 	screen_resources_reply =
 		xcb_randr_get_screen_resources_reply(c,screen_resources_cookie,e);

	crtcs_p = xcb_randr_get_screen_resources_crtcs(screen_resources_reply);
	crtc_config_p = (xcb_randr_set_crtc_config_cookie_t *) malloc(num_crtcs*sizeof(xcb_randr_set_crtc_config_cookie_t));

	for(i=0;i<screen_resources_reply->num_crtcs;++i){
		crtc_config_p[i] = xcb_randr_set_crtc_config(c,crtcs_p[i],XCB_CURRENT_TIME, screen_resources_reply->config_timestamp,0,0,XCB_NONE,  XCB_RANDR_ROTATION_ROTATE_0,NULL,0);
	}

	for(i=0;i<screen_resources_reply->num_crtcs;++i){
		crtc_config_reply_pp[i] = xcb_randr_set_crtc_config_reply(c,crtc_config_p[i],e);
	}

	xcb_randr_set_screen_size(c,screen->root,mySett.width,mySett.height, mySett.widthMM,mySett.heightMM);



	output_with_conf_match = 0;
	for_each_output(&match_with_config,&match_with_config,&do_nothing);
}

void match_with_config(xcb_randr_output_t *output_p){

	int i;

	xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;

	char *edid_string;
	uint8_t *output_property_data;
	int output_property_length;
	uint8_t delete = 0;
	uint8_t pending = 0;

	output_info_cookie =
	xcb_randr_get_output_info(c, output_p, XCB_CURRENT_TIME);

	output_property_cookie = xcb_randr_get_output_property(c,
		*output_p, atom_reply->atom, AnyPropertyType, 0, 100, delete,
		pending);
	output_info_reply =
		xcb_randr_get_output_info_reply (c, output_info_cookie, e);
	output_property_reply = xcb_randr_get_output_property_reply(
		c,output_property_cookie,e);
	output_property_data = xcb_randr_get_output_property_data(
		output_property_reply);
	output_property_length = xcb_randr_get_output_property_data_length(
		output_property_reply);

	edid_to_string(output_property_data,output_property_length,
		&edid_string);

	for(i=0;i<num_out_pp;++i){
		if (!strcmp(mySett.edid_val[i],edid_string)){
			// Found a match between the configuration file edid and currently connected edid. Now have to load correct settings.
			// Need to find the proper crtc to assign the output
			// Which crtc has the same resolution?
			for_each_output_mode(&find_res_match,output_info_reply,crtc_info_reply);
			find_crtc_by_res(mySett.res_x,mySett.res_y);
			// Connect correct crtc to correct output

			crtc_config_p = xcb_randr_set_crtc_config(c,crtcs_p[i],XCB_CURRENT_TIME, screen_resources_reply->config_timestamp,0,0,XCB_NONE,  XCB_RANDR_ROTATION_ROTATE_0,NULL,0);



			crtc_config_reply_pp[i] = xcb_randr_set_crtc_config_reply(c,crtc_config_p[i],e);


		}

	}



}

void find_crtc_by_res(int res_x, int res_y){


	xcb_randr_crtc_t *crtc_p;
	int num_crtcs;



	num_crtcs = xcb_randr_get_output_info_crtcs_length(output_info_reply)
	crtc_p = xcb_randr_get_output_info_crtcs(output_info_reply);

	for(i=0;i<num_crtcs,++i){
		for_each_output_mode()

		++crtc_p;
	}

}
*/
