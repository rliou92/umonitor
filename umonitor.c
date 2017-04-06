#include <stdio.h>
#include <inttypes.h>
#include <libconfig.h>
#include <unistd.h>
#include <X11/extensions/Xrandr.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>

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
void edid_to_string(void);
void save_profile(void);
//void listen_for_event(void);


static xcb_generic_error_t **e;
static xcb_connection_t *c;
static const xcb_setup_t *setup;
static xcb_screen_iterator_t iter;
static xcb_screen_t *screen;
static xcb_randr_get_screen_resources_cookie_t screen_resources_cookie;
static xcb_randr_get_screen_resources_reply_t *screen_resources_reply;


config_t config;
config_setting_t *root, *profile_group;
char* profile_name;

static char config_file[] = "umon.conf";
static int verbose = 0;

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
				load_val_from_config();
				load_profile();
				free_val_from_config();
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
			listen_for_event(); // Will not use new configuration file if it is changed
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

	int i;
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

	//screen_resources_cookie = xcb_randr_get_screen_resources(c,screen->root);

	//screen_resources_reply =
		//xcb_randr_get_screen_resources_reply(c,screen_resources_cookie,e);
}

void save_profile(){

	int i;

	config_setting_t *edid_setting,*resolution_setting,*pos_x_setting,
	*pos_y_setting,*disp_group,*disp_width_setting,*disp_height_setting,
	*disp_widthMM_setting,*disp_heightMM_setting,*mon_group,*output_group;

	xcb_randr_get_screen_info_cookie_t screen_info_cookie;
	xcb_randr_get_screen_info_reply_t *screen_info_reply;
	xcb_randr_screen_size_iterator_t screen_size_iterator;
	xcb_randr_screen_size_t screen_size;
	xcb_randr_output_t *output_p;
	xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;

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

	output_p = xcb_randr_get_screen_resources_outputs(screen_resources_reply);
	outputs_length =
		xcb_randr_get_screen_resources_outputs_length(screen_resources_reply);

	for (i=0; i<outputs_length; ++i){
		output_p = output_p + i;
		output_info_cookie = xcb_randr_get_output_info(c, *output_p, XCB_CURRENT_TIME);
		output_info_reply =
			xcb_randr_get_output_info_reply (c, output_info_cookie, e);
		output_group =
			config_setting_add(mon_group,cur_output->outputInfo->name,CONFIG_TYPE_GROUP);
		edid_setting = config_setting_add(output_group,"EDID",CONFIG_TYPE_STRING);
		resolution_setting = config_setting_add(output_group,"resolution",CONFIG_TYPE_STRING);
		pos_group = config_setting_add(output_group,"pos",CONFIG_TYPE_GROUP);
		pos_x_setting = config_setting_add(pos_group,"x",CONFIG_TYPE_INT);
		pos_y_setting = config_setting_add(pos_group,"y",CONFIG_TYPE_INT);



	}
}
