#include "common.h"
#include "umonitor.h"
#include "autoload.h"
#include "load.h"

/*! \file
	\brief Detect which profile to load

*/

#define PVAR ((autoload_pvar *) (self->pvar))

static void find_profile_and_load(autoload_class * self, int test_cur);
static void get_profile_found(autoload_class * self, int * profile_found, const char **profile_name);
static void determine_profile_match(autoload_class * self);
static void wait_for_event(autoload_class * self);
static void validate_timestamp_and_load(autoload_class * self);
static void count_output_match(void *self_void,
			       xcb_randr_output_t * output_p);
static void determine_output_match(autoload_class * self,
				   xcb_randr_output_t * output_p,
				   char *output_name);
static void print_connected_outputs(void *self_void,
				    xcb_randr_output_t * output_p);




// Autoload class private variables
typedef struct {
	// Inheriting classes
	screen_class *screen_o;	/*!< Connection information to server */
	/*! load object for loading after matching profile is found */
	load_class *load_o;

	// Variables
	config_t *config;	/*!< config handle */
	config_setting_t *mon_group, *cur_profile;	/*!< current monitor group */
	/*! how many outputs in configuration file match current display settings */
	int output_match;
	int num_out_pp;		/*!< Number of outputs per profile */
	int num_conn_outputs;	/*!< number of connected outputs */
	int test_cur;
	int profile_found;
	xcb_randr_screen_change_notify_event_t *randr_evt;
	const char *profile_name;

} autoload_pvar;


/*! Autload constructor*/
void autoload_constructor(autoload_class ** self_p,
			  screen_class * screen_o, config_t * config)
{
	autoload_class *self;

	self = (autoload_class *) umalloc(sizeof(autoload_class));
	self->pvar = (void *) umalloc(sizeof(autoload_pvar));

	PVAR->screen_o = screen_o;
	PVAR->config = config;
	self->wait_for_event = wait_for_event;

	self->find_profile_and_load = find_profile_and_load;
	self->get_profile_found = get_profile_found;
	load_class_constructor(&(PVAR->load_o), screen_o);

	xcb_randr_select_input(screen_o->c, screen_o->screen->root,
			       XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
	xcb_flush(screen_o->c);

	*self_p = self;

}

/*! Autload destructor*/
void autoload_destructor(autoload_class * self)
{

	load_class_destructor(PVAR->load_o);
	free(PVAR);
	free(self);

}

static void get_profile_found(autoload_class * self, int * profile_found, const char **profile_name)
{
	*profile_found = PVAR->profile_found;
	*profile_name = PVAR->profile_name;
}
/*! \brief Find which profile matches the current setup and load it

Also prints out the list of configurations along with the current configuration.
The current configuration is either the current configuration (applying no
settings), or the newly loaded configuration. */

static void find_profile_and_load(autoload_class * self, int test_cur)
{
	config_setting_t *root;

	int num_profiles, i;

// #ifdef DEBUG
	int j;
	config_setting_t *group;
	const char *conf_output, *conf_edid, *profile_name;
// #endif

	PVAR->test_cur = test_cur;
	PVAR->profile_found = 0;
	root = config_root_setting(PVAR->config);
	num_profiles = config_setting_length(root);
	umon_print("Number of profiles: %d\n", num_profiles);
	umon_print("Trying to find which profile matches current setup\n");
	umon_print("Current connected outputs: ");
//#ifdef DEBUG
	for_each_output((void *) self,
			PVAR->screen_o->screen_resources_reply,
			print_connected_outputs);
	umon_print("\n");
//#endif
	for (i = 0; i < num_profiles; i++) {
		PVAR->cur_profile = config_setting_get_elem(root, i);
		umon_print("Looping over profile ");
		profile_name = config_setting_name(PVAR->cur_profile);
		//if (print)
			print_state("%s", profile_name);
		umon_print("\n");
		PVAR->output_match = 0;
		PVAR->mon_group =
		    config_setting_lookup(PVAR->cur_profile, "Monitors");
		PVAR->num_out_pp = config_setting_length(PVAR->mon_group);
//#ifdef DEBUG
		umon_print("Configuration outputs: ");
		for (j = 0; j < PVAR->num_out_pp; j++) {
			group =
			    config_setting_get_elem(PVAR->mon_group, j);
			conf_output = config_setting_name(group);
			config_setting_lookup_string(group, "EDID", &conf_edid);
			umon_print("%s (%s) ", conf_output, conf_edid);
		}
		umon_print("\n");
//#endif
		//if (PVAR->num_out_pp ==
		//PVAR->screen_o->screen_resources_reply->num_outputs){
		PVAR->num_conn_outputs = 0;
		for_each_output((void *) self,
				PVAR->screen_o->screen_resources_reply,
				count_output_match);

		determine_profile_match(self);
		if (PVAR->profile_found) {
			PVAR->profile_name = profile_name;
		}
		print_state("\n");
	}
	if (!PVAR->profile_found)
		print_state("Unknown profile*\n");
	print_state("---------------------------------\n");

}

// #ifdef DEBUG
static void print_connected_outputs(void *self_void,
				    xcb_randr_output_t * output_p)
{
	autoload_class *self = (autoload_class *) self_void;

	xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;
	char *output_name, *edid_string;

	output_info_cookie =
	    xcb_randr_get_output_info(PVAR->screen_o->c, *output_p,
				      XCB_CURRENT_TIME);
	output_info_reply =
	    xcb_randr_get_output_info_reply(PVAR->screen_o->c,
					    output_info_cookie,
					    &(PVAR->screen_o->e));
	get_output_name(output_info_reply, &output_name);
	fetch_edid(output_p, PVAR->screen_o, &edid_string);

	if (!output_info_reply->connection)
		umon_print("%s (%s) ", output_name, edid_string);

	free(output_name);
	free(output_info_reply);
	free(edid_string);

}
// #endif

static void determine_profile_match(autoload_class * self)
{
	int cur_loaded;
	char *profile_name;

	if ((PVAR->output_match == PVAR->num_out_pp) &&
	    (PVAR->num_out_pp == PVAR->num_conn_outputs)) {
		//Only loads first matching profile
		profile_name = config_setting_name(PVAR->cur_profile);
		umon_print("Profile %s matches current setup\n",
			   profile_name);
		PVAR->load_o->load_profile(PVAR->load_o, PVAR->cur_profile,
					   PVAR->test_cur);
		PVAR->load_o->get_cur_loaded(PVAR->load_o, &cur_loaded);
		if ((cur_loaded == 1 && PVAR->test_cur) || !PVAR->test_cur) {
			PVAR->profile_found = 1;
			print_state("*");
			umon_print("\n");
		}
	}
}


/*! \brief Use xcb_wait_for_event to detect screen changes
 */

static void wait_for_event(autoload_class * self)
{

	// In order to get an output the first time
	find_profile_and_load(self, 0);
	xcb_generic_event_t *evt;

	while (1) {
		umon_print("Waiting for event\n");
		evt = xcb_wait_for_event(PVAR->screen_o->c);
		umon_print("Event type: %" PRIu8 "\n", evt->response_type);
		umon_print("screen change masked: %" PRIu8 "\n",
			   evt->response_type &
			   XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
		umon_print("output change masked: %" PRIu8 "\n",
			   evt->response_type &
			   XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE);

		if (evt->response_type &
		    XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE) {

			PVAR->randr_evt =
			    (xcb_randr_screen_change_notify_event_t *)
			    evt;
			validate_timestamp_and_load(self);
		}
		free(evt);
	}


}

static void validate_timestamp_and_load(autoload_class * self)
{
	xcb_timestamp_t load_last_time;

	PVAR->load_o->get_last_time(PVAR->load_o, &load_last_time);
	//randr_evt = (xcb_randr_output_change_t*) evt;
	//printf("event received, should I load?\n");
	umon_print("Last time of configuration: %" PRIu32
		   "\n", load_last_time);
	umon_print("Time of event: %" PRIu32 "\n",
		   PVAR->randr_evt->timestamp);
	//xcb_timestamp_t time_difference =
	// randr_evt->timestamp -
	// self->load_o->last_time;
	umon_print("Screen change event detected\n");


	if (PVAR->randr_evt->timestamp >= load_last_time) {
		umon_print("Now I should load\n");
		PVAR->screen_o->update_screen(PVAR->screen_o);
		if (!config_read_file(PVAR->config, CONFIG_FILE))
			exit(NO_CONF_FILE_FOUND);
		find_profile_and_load(self, 0);
	}
	// Find matching profile
	// Get total connected outputs

	sleep(1);
	//self->screen_o->update_screen(self->screen_o);
	xcb_flush(PVAR->screen_o->c);

	//xcb_randr_select_input(self->screen_o->c,self->screen_o->screen->root,
	//XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
	//    evt = xcb_wait_for_event(self->screen_o->c);
	//            printf("event received\n");
	//    evt = xcb_wait_for_event(self->screen_o->c);
	//            printf("event received\n");
	//Self triggering right now



}


/*! \brief For each connected output find which profile contains that output
 */
static void count_output_match(void *self_void,
			       xcb_randr_output_t * output_p)
{

	autoload_class *self = (autoload_class *) self_void;

	xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;
	char *output_name;


	output_info_cookie =
	    xcb_randr_get_output_info(PVAR->screen_o->c, *output_p,
				      XCB_CURRENT_TIME);
	output_info_reply =
	    xcb_randr_get_output_info_reply(PVAR->screen_o->c,
					    output_info_cookie,
					    &(PVAR->screen_o->e));
	get_output_name(output_info_reply, &output_name);
	if (!output_info_reply->connection) {
		PVAR->num_conn_outputs++;
		//umon_print("output %s is connected \n", output_name);
		determine_output_match(self, output_p, output_name);


	} else {
		// TODO just disable
	}

	free(output_name);
	free(output_info_reply);
	//if (VERBOSE) printf("output_property_reply %d\n",output_property_reply);

}

static void determine_output_match(autoload_class * self,
				   xcb_randr_output_t * output_p,
				   char *output_name)
{
	int j, output_match_unique;
	config_setting_t *group;
	const char *conf_edid;
	char *edid_string, *conf_output;


	output_match_unique = 0;
	fetch_edid(output_p, PVAR->screen_o, &edid_string);


	for (j = 0; j < PVAR->num_out_pp; j++) {
		//if (VERBOSE) printf("output match, %d\n",PVAR->output_match);

		//if (VERBOSE) printf("output_match_unique, %d\n",output_match_unique);
		group = config_setting_get_elem(PVAR->mon_group, j);
		config_setting_lookup_string(group, "EDID", &conf_edid);
		conf_output = config_setting_name(group);
		// if (VERBOSE) printf("conf_edid: %s\n",conf_edid);
		// if (VERBOSE) printf("edid_string: %s\n",edid_string);
		if (!strcmp(conf_edid, edid_string)
		    && !strcmp(output_name, conf_output)) {
			output_match_unique++;
			//if (VERBOSE) printf("match, %d\n",output_match_unique);
		}

	}
	free(edid_string);

	if (output_match_unique == 1) {
		//umon_print("output match, %d\n", PVAR->output_match);
		PVAR->output_match++;
	}
	else if(output_match_unique > 1) {
		umon_print("ERROR: Found more than one edid/output combo in configuration file that matches current setup\n");
		exit(DUPLICATE_OUTPUT);
	}


}
