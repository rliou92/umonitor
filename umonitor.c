#include "umonitor.h"

int main(int argc, char **argv) {
	int save = 0;
	int load = 0;
	int delete = 0;
	int test_event = 0;
	save_class *save_o;
	load_class *load_o;
	autoload_class *autoload_o;
	screen_class screen_o;

	config_t config;
	config_setting_t *root, *profile_group;
	char* profile_name;

	int i, cfg_idx;

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

		save_class_constructor(&save_o,&screen_o,&config);
		save_o->save_profile(save_o,profile_group);
		save_class_destructor(save_o);
		//free save_o
	}

	if (test_event){
		if (config_read_file(&config, CONFIG_FILE)) {
			autoload_o = (autoload_class *) malloc(sizeof(autoload_class));
			autoload_constructor(autoload_o,&screen_o,&config); // Will not use new configuration file if it is changed
			//autoload_o->find_profile_and_load(autoload_o);
			autoload_o->wait_for_event(autoload_o);
			if (VERBOSE) printf("Autoloading\n");
		}
		else {
			printf("No file to load when event is triggered\n");
			exit(3);
		}
	}

	// Free things

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

void fetch_edid(xcb_randr_output_t *output_p,	screen_class *screen_t_p,
	 char **edid_string){

	/*
	 * Converts the edid that is returned from the X11 server into a string
	 * Inputs: 	edid	 	the bits return from X11 server
	 * Outputs: 	edid_string	 edid in string form
	 */

	int z,length;
	uint8_t delete = 0;
	uint8_t pending = 0;
	xcb_randr_get_output_property_cookie_t output_property_cookie;
	xcb_randr_get_output_property_reply_t *output_property_reply;
	uint8_t *edid;

	output_property_cookie = xcb_randr_get_output_property(screen_t_p->c,
		*output_p,screen_t_p->edid_atom->atom,AnyPropertyType,0,100,
    delete,pending);

	output_property_reply = xcb_randr_get_output_property_reply(
		screen_t_p->c,output_property_cookie,&screen_t_p->e);

	edid = xcb_randr_get_output_property_data(
		output_property_reply);
	length = xcb_randr_get_output_property_data_length(
		output_property_reply);

	if (VERBOSE) printf("Starting edid_to_string\n");
	*edid_string = (char *) malloc((length+1)*sizeof(char));
	for (z=0;z<length;++z) {
		if ((char) edid[z] == '\0') {
			*(*edid_string+z) = '0';
		}
		else {
			*(*edid_string+z) = (char) edid[z];
		}

	}
	*(*edid_string+z) = '\0';

	if (VERBOSE) printf("Finished edid_to_string\n");
}
