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





// int *num_out_pp;




int main(int argc, char **argv) {
	int save = 0;
	int load = 0;
	int delete = 0;
	int test_event = 0;
	save_profile_class *save_profile_o;
	load_class *load_o;
	autoload_class *autoload_o;

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
				load_o = (load_class *) malloc(sizeof(load_class));
				load_class_constructor(load_o,&screen_o,&config);
				load_o->load_profile(load_o,profile_group);
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
			autoload_o = (autoload_class *) malloc(sizeof(autoload_class));
			autoload_constructor(autoload_o,&screen_o,&config); // Will not use new configuration file if it is changed
			autoload_o->find_profile_and_load(autoload_o);
			if (VERBOSE) printf("Autoloading\n");
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

		int i,outputs_length;

		xcb_randr_output_t *output_p;

		output_p = xcb_randr_get_screen_resources_outputs(screen_resources_reply);
		outputs_length =
			xcb_randr_get_screen_resources_outputs_length(screen_resources_reply);

		for (i=0; i<outputs_length; ++i){
			callback(self,output_p++);
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
		callback(self,mode_id_p++);
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
