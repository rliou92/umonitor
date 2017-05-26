#include "autoload.h"

/*! \file
		\brief Detect which profile to load

*/

/*! Autload constructor*/
void autoload_constructor(autoload_class **self,screen_class *screen_o,
  config_t *config){

	*self = (autoload_class *) malloc(sizeof(autoload_class));

  (*self)->screen_t_p = screen_o;
  (*self)->config = config;
	(*self)->wait_for_event = wait_for_event;

	(*self)->find_profile_and_load = find_profile_and_load;
  load_class_constructor(&((*self)->load_o),screen_o,config);


	xcb_randr_select_input(screen_o->c,screen_o->screen->root,
		XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE);
  xcb_flush(screen_o->c);

}

/*! Autload destructor*/
void autoload_destructor(autoload_class *self){

  load_class_destructor(self->load_o);
  free(self);


}

/*! \brief Use xcb_wait_for_event to detect screen changes
 */

static void wait_for_event(autoload_class *self){

	xcb_generic_event_t *evt;
	xcb_randr_screen_change_notify_event_t *randr_evt;

  find_profile_and_load(self,0); //In order to get an output the first time
	while(1){

    umon_print("Waiting for event\n");
  	evt = xcb_wait_for_event(self->screen_t_p->c);
    umon_print("Event type: %"PRIu8"\n",evt->response_type);
    umon_print("screen change mask: %"PRIu16"\n",XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
    umon_print("output change mask: %"PRIu16"\n",XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE);

  	if (evt->response_type) {
  		randr_evt = (xcb_randr_screen_change_notify_event_t*) evt;
      //printf("event received, should I load?\n");
      umon_print("Last time of configuration: %" PRIu32 "\n",self->load_o->last_time);
      umon_print("Time of event: %" PRIu32 "\n",randr_evt->timestamp);
      //xcb_timestamp_t time_difference =  randr_evt->timestamp - self->load_o->last_time;
      umon_print("Screen change event detected\n");


      if (randr_evt->timestamp >= self->load_o->last_time){
        umon_print("Now I should load\n");
        self->screen_t_p->update_screen(self->screen_t_p);
        find_profile_and_load(self,0);
      }
  			// Find matching profile
  			// Get total connected outputs

  		sleep(1);
            //self->screen_t_p->update_screen(self->screen_t_p);
      xcb_flush(self->screen_t_p->c);

  	//xcb_randr_select_input(self->screen_t_p->c,self->screen_t_p->screen->root,
  		//XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
  //	evt = xcb_wait_for_event(self->screen_t_p->c);
  //		printf("event received\n");
  //	evt = xcb_wait_for_event(self->screen_t_p->c);
  //		printf("event received\n");
  			//Self triggering right now

  	}
    free(evt);
	}



}

/*! \brief For each connected output find which profile contains that output
 */
static void match_with_profile(void *self_void,xcb_randr_output_t *output_p){

  autoload_class *self = (autoload_class *) self_void;
  const char *conf_edid;
	char *edid_string;

	xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;

  int j,output_match_unique;

	config_setting_t *group;

	output_info_cookie =
	xcb_randr_get_output_info(self->screen_t_p->c, *output_p,
		XCB_CURRENT_TIME);
	output_info_reply =
		xcb_randr_get_output_info_reply (self->screen_t_p->c,
		output_info_cookie, &self->screen_t_p->e);

	if (!output_info_reply->connection){
		umon_print("Found output that is connected \n");
			self->num_conn_outputs++;

		  fetch_edid(output_p,self->screen_t_p,&edid_string);

		  output_match_unique = 0;
		  for(j=0;j<self->num_out_pp;j++){
			    //if (VERBOSE) printf("output match, %d\n",self->output_match);

			    //if (VERBOSE) printf("output_match_unique, %d\n",output_match_unique);
  		    group = config_setting_get_elem(self->mon_group,j);
  		    config_setting_lookup_string(group,"EDID",&conf_edid);
  		    // if (VERBOSE) printf("conf_edid: %s\n",conf_edid);
  		    // if (VERBOSE) printf("edid_string: %s\n",edid_string);
  		    if (!strcmp(conf_edid,edid_string)){
  		      output_match_unique++;
  			    //if (VERBOSE) printf("match, %d\n",output_match_unique);
  		    }

		  }
      free(edid_string);

		  if (output_match_unique == 1){
			  umon_print("output match, %d\n",self->output_match);
		    self->output_match++;
		  }
		}
		else {
			// TODO just disable
		}


    free(output_info_reply);
  //if (VERBOSE) printf("output_property_reply %d\n",output_property_reply);

}

/*! \brief Find which profile matches the current setup and load it

Also prints out the list of configurations along with the current configuration.
The current configuration is either the current configuration (applying no
settings), or the newly loaded configuration. */

static void find_profile_and_load(autoload_class *self, int test_cur){

  config_setting_t *root,*cur_profile;
  int profile_found = 0;

  root = config_root_setting(self->config);
  int num_profiles = config_setting_length(root);
	umon_print("Number of profiles:%d\n",num_profiles);
  for (int i=0;i<num_profiles;i++){
    cur_profile = config_setting_get_elem(root,i);
    printf("%s",config_setting_name(cur_profile));
    if(!profile_found){
      self->output_match = 0;
      self->mon_group = config_setting_lookup(cur_profile,"Monitors");
      self->num_out_pp = config_setting_length(self->mon_group);

      //if (self->num_out_pp ==
         //self->screen_t_p->screen_resources_reply->num_outputs){
  		self->num_conn_outputs = 0;
      for_each_output((void *) self,self->screen_t_p->screen_resources_reply,
        match_with_profile);
      // if (VERBOSE) printf("self->output_match: %d\n",self->output_match);
      // if (VERBOSE) printf("self->num_out_pp: %d\n",self->num_out_pp);
      // if (VERBOSE) printf("self->num_conn_outputs: %d\n",self->num_conn_outputs);
      if ((self->output_match == self->num_out_pp) &&
          (self->num_out_pp == self->num_conn_outputs)){
        //Only loads first matching profile
  			umon_print("Found matching profile\n");
        self->load_o->load_profile(self->load_o,cur_profile,test_cur);
        if (self->load_o->cur_loaded == 1 && test_cur){
          profile_found = 1;
          printf("*");
        }
        if (!test_cur){
          profile_found = 1;
          printf("*");
        }
      }
    }

    printf("\n");
  }
  if (!profile_found) printf("Unknown profile*\n");
  printf("---------------------------------\n");

}
