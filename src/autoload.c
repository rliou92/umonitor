#include "common.h"
#include "umonitor.h"
#include "autoload.h"
#include "load.h"

/*! \file
	\brief Detect which profile to load

*/

#define PVAR ((autoload_pvar *) (self->pvar))

static void find_profile_and_load(autoload_class * self, int test_cur);
static void determine_profile_match(autoload_class * self);
static void wait_for_event(autoload_class * self);
static void validate_timestamp_and_load(autoload_class * self);
static void count_output_match(void *self_void,
			       xcb_randr_output_t * output_p);
static void determine_output_match(autoload_class * self,
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

/*! \brief Find which profile matches the current setup and load it

Also prints out the list of configurations along with the current configuration.
The current configuration is either the current configuration (applying no
settings), or the newly loaded configuration. */

static void find_profile_and_load(autoload_class * self, int test_cur)
{
	config_setting_t *root;

	int num_profiles, profile_found, i;

	PVAR->test_cur = test_cur;
	profile_found = 0;
	root = config_root_setting(PVAR->config);
	num_profiles = config_setting_length(root);
	umon_print("Number of profiles:%d\n", num_profiles);
	for (i = 0; i < num_profiles; i++) {
		PVAR->cur_profile = config_setting_get_elem(root, i);
		printf("%s", config_setting_name(PVAR->cur_profile));
		if (profile_found)
			break;
		PVAR->output_match = 0;
		PVAR->mon_group =
		    config_setting_lookup(PVAR->cur_profile, "Monitors");
		PVAR->num_out_pp = config_setting_length(PVAR->mon_group);

		//if (PVAR->num_out_pp ==
		//PVAR->screen_o->screen_resources_reply->num_outputs){
		PVAR->num_conn_outputs = 0;
		for_each_output((void *) self,
				PVAR->screen_o->screen_resources_reply,
				count_output_match);

		determine_profile_match(self);


		printf("\n");
	}
	if (!PVAR->profile_found)
		printf("Unknown profile*\n");
	printf("---------------------------------\n");

}


static void determine_profile_match(autoload_class * self)
{
	int cur_loaded;

	if ((PVAR->output_match == PVAR->num_out_pp) &&
	    (PVAR->num_out_pp == PVAR->num_conn_outputs)) {
		//Only loads first matching profile
		umon_print("Found matching profile\n");
		PVAR->load_o->load_profile(PVAR->load_o,
					   PVAR->cur_profile,
					   PVAR->test_cur);
		PVAR->load_o->get_cur_loaded(PVAR->load_o, &cur_loaded);
		if (cur_loaded == 1 && PVAR->test_cur) {
			PVAR->profile_found = 1;
			printf("*");
		}
		if (!PVAR->test_cur) {
			PVAR->profile_found = 1;
			printf("*");
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


	output_info_cookie =
	    xcb_randr_get_output_info(PVAR->screen_o->c, *output_p,
				      XCB_CURRENT_TIME);
	output_info_reply =
	    xcb_randr_get_output_info_reply(PVAR->screen_o->c,
					    output_info_cookie,
					    &(PVAR->screen_o->e));

	if (!output_info_reply->connection) {
		umon_print("Found output that is connected \n");
		PVAR->num_conn_outputs++;

		determine_output_match(self, output_p);


	} else {
		// TODO just disable
	}


	free(output_info_reply);
	//if (VERBOSE) printf("output_property_reply %d\n",output_property_reply);

}

static void determine_output_match(autoload_class * self,
				   xcb_randr_output_t * output_p)
{
	int j, output_match_unique;
	config_setting_t *group;
	const char *conf_edid;
	char *edid_string;


	output_match_unique = 0;
	fetch_edid(output_p, PVAR->screen_o, &edid_string);

	for (j = 0; j < PVAR->num_out_pp; j++) {
		//if (VERBOSE) printf("output match, %d\n",PVAR->output_match);

		//if (VERBOSE) printf("output_match_unique, %d\n",output_match_unique);
		group = config_setting_get_elem(PVAR->mon_group, j);
		config_setting_lookup_string(group, "EDID", &conf_edid);
		// if (VERBOSE) printf("conf_edid: %s\n",conf_edid);
		// if (VERBOSE) printf("edid_string: %s\n",edid_string);
		if (!strcmp(conf_edid, edid_string)) {
			output_match_unique++;
			//if (VERBOSE) printf("match, %d\n",output_match_unique);
		}

	}
	free(edid_string);

	if (output_match_unique == 1) {
		umon_print("output match, %d\n", PVAR->output_match);
		PVAR->output_match++;
	}


}
