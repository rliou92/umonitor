#include "umonitor.h"

/*! \mainpage Main Page
 *
 * \section intro_sec Introduction
 *
 * This program is intended to manage monitor hotplugging.

 	 \section readme README
	 Usage:
	 				1. Setup your configuration using xrandr or related tools
	 				2. Run \code umonitor --save <profile_name> \endcode
					3. Run \code umonitor --listen \endcode to begin automatically detecting and changing monitor configuration
 *
 */

/*! \file
		\brief Main file

		Contains the main function plus some helper functions that are shared by the classes
*/

/*! Logic for parsing options here*/

int main(int argc, char **argv) {
	int save = 0;
	int load = 0;
	int delete = 0;
	int listen = 0;
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
		else if (!strcmp("--listen", argv[i])){
			listen = 1;
		}
		else {
			printf("Unrecognized argument: %s\n",argv[i]);
		}
	}

	if (save + load + listen >= 2) exit(10);
	// if (save+load+listen == 0) sleep(5);
	screen_class_constructor(&screen_o);
	// printf("Connected to screen\n");
	// printf("Proof screen connection: %d\n",screen_o.c);

	char *home_directory = getenv("HOME");
	printf("Home directory: %s\n",home_directory);
	const char *conf_file = "/.config/umon2.conf";
	char *path = malloc((strlen(home_directory)+strlen(conf_file)));
	strcpy(path,home_directory);
	strcat(path,conf_file);
	printf("Path: %s\n",path);
	CONF_FP = fopen(path,"rw");
	if (!CONF_FP) error("Cannot find configuration file");
	printf("File pointer: %d\n",CONF_FP);

	if (config_read(&config, CONF_FP)) {
		if (VERBOSE) printf("Detected existing configuration file\n");
		// Existing config file to load setting values
		if (load) {
			if (VERBOSE) printf("Loading profile: %s\n", profile_name);
			// Load profile
			profile_group = config_lookup(&config,profile_name);

			if (profile_group != NULL) {
				load_class_constructor(&load_o,&screen_o);
				load_o->load_profile(load_o,profile_group);
				load_class_destructor(load_o);
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

		if (listen) {
			// TODO Will not use new configuration file if it is changed
			autoload_constructor(&autoload_o,&screen_o,&config);
			//autoload_o->find_profile_and_load(autoload_o);
			autoload_o->wait_for_event(autoload_o);
			if (VERBOSE) printf("Autoloading\n");
			autoload_destructor(autoload_o);

		}
	}

	else {
		if (load || delete) {
			printf("No file to load or delete\n");
			exit(1);
		}
		if (listen){
			printf("No file to load when event is triggered\n");
			exit(3);
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

	// Free things
	// Screen destructor
	screen_class_destructor(&screen_o);

	config_destroy(&config);
	fclose(CONF_FP);
	free(path);

}

/*! \brief Loop over each output

	Calls the callback function for each output
 */
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

/*! \brief Loop over each output mode

Calls the callback function for each output mode

 */
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

/*! \brief Converts the edid that is returned from the X11 server into a string
 *
 * @param[in]		output_p		the output whose edid is desired
 * @param[in]		screen_t_p	the connection information
 * @param[out]	edid_string	the edid in string form
 */

void fetch_edid(xcb_randr_output_t *output_p,	screen_class *screen_t_p,
	 char **edid_string){



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

	free(output_property_reply);
	if (VERBOSE) printf("Finished edid_to_string\n");
}
