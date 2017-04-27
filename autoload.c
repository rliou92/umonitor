#include "autoload.h"




void autoload_constructor(autoload_class *self,screen_class *screen_o,config_t *config){


  //int i,j,output_match_unique=0,num_profiles;



  self->screen_t_p = screen_o;
  self->config = config;
	self->wait_for_event = wait_for_event;

	self->find_profile_and_load = find_profile_and_load;
  load_class_constructor(&(self->load_o),screen_o,config);


	xcb_randr_select_input(screen_o->c,screen_o->screen->root,
		XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
  xcb_flush(screen_o->c);




}

static void wait_for_event(autoload_class *self){

	xcb_generic_event_t *evt;
	xcb_randr_screen_change_notify_event_t *randr_evt;
	xcb_timestamp_t last_time;

	while(1){
	evt = xcb_wait_for_event(self->screen_t_p->c);
	if (evt->response_type & XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE) {
		randr_evt = (xcb_randr_screen_change_notify_event_t*) evt;
			// Find matching profile
			// Get total connected outputs
		printf("event received\n");
			find_profile_and_load(self);
			//Self triggering right now

	}
	}
	free(evt);

//xcb_disconnect(conn);
}


static void match_with_profile(void *self_void,xcb_randr_output_t *output_p){
	// For each profile
	// umon_setting_val_t umon_setting_val;
  autoload_class *self = (autoload_class *) self_void;
  const char *conf_edid;
	char *edid_string;

  xcb_randr_get_output_property_cookie_t output_property_cookie;
  xcb_randr_get_output_property_reply_t *output_property_reply;
	xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;


  // xcb_randr_get_output_info_cookie_t output_info_cookie;
  // xcb_randr_get_output_info_reply_t *output_info_reply;

  int j,output_match_unique;
	uint8_t *output_property_data;
	int output_property_length;
	uint8_t delete = 0;
	uint8_t pending = 0;
	config_setting_t *group;

  output_property_cookie = xcb_randr_get_output_property(self->screen_t_p->c,
    *(output_p),self->screen_t_p->edid_atom->atom,AnyPropertyType,0,100,
    delete,pending);
  output_property_reply = xcb_randr_get_output_property_reply(
      self->screen_t_p->c,output_property_cookie,&self->screen_t_p->e);
	output_info_cookie =
	xcb_randr_get_output_info(self->screen_t_p->c, *output_p,
		XCB_CURRENT_TIME);
	output_info_reply =
		xcb_randr_get_output_info_reply (self->screen_t_p->c,
		output_info_cookie, &self->screen_t_p->e);

	if (!output_info_reply->connection){
		if (VERBOSE) printf("Found output that is connected \n");
			self->num_conn_outputs++;

			output_property_data = xcb_randr_get_output_property_data(
		    output_property_reply);
		  output_property_length = xcb_randr_get_output_property_data_length(
		    output_property_reply);

		  if (VERBOSE) printf("Finish fetching info from server\n");
		  edid_to_string(output_property_data,output_property_length,
		    &edid_string);

		  output_match_unique = 0;
		  for(j=0;j<self->num_out_pp;j++){
			    printf("output match, %d\n",self->output_match);

			    printf("output_match_unique, %d\n",output_match_unique);
		    group = config_setting_get_elem(self->mon_group,j);
		    config_setting_lookup_string(group,"EDID",&conf_edid);
		    printf("conf_edid: %s\n",conf_edid);
		    printf("edid_string: %s\n",edid_string);
		    if (!strcmp(conf_edid,edid_string)){
		      output_match_unique++;
			    printf("match, %d\n",output_match_unique);
		    }
		  }

		  if (output_match_unique == 1){
			    printf("output match, %d\n",self->output_match);
		    self->output_match++;
			    printf("output match, %d\n",self->output_match);
		  }
		}
		else {
			// TODO just disable
		}

  //if (VERBOSE) printf("output_property_reply %d\n",output_property_reply);

}

static void find_profile_and_load(autoload_class *self){

  config_setting_t *root,*cur_profile;

  self->screen_t_p->update_screen(self->screen_t_p);

  root = config_root_setting(self->config);
  int num_profiles = config_setting_length(root);
	if (VERBOSE) printf("Number of profiles:%d\n",num_profiles);
  for (int i=0;i<num_profiles;i++){
	  printf("NEW PROFILE\n");
    self->output_match = 0;
    cur_profile = config_setting_get_elem(root,i);
    self->mon_group = config_setting_lookup(cur_profile,"Monitors");
    self->num_out_pp = config_setting_length(self->mon_group);

    //if (self->num_out_pp ==
       //self->screen_t_p->screen_resources_reply->num_outputs){
			 self->num_conn_outputs = 0;
      for_each_output((void *) self,self->screen_t_p->screen_resources_reply,
        match_with_profile);
      printf("self->output_match: %d\n",self->output_match);
      printf("self->num_out_pp: %d\n",self->num_out_pp);
      printf("self->num_conn_outputs: %d\n",self->num_conn_outputs);
      if ((self->output_match == self->num_out_pp) && (self->num_out_pp == self->num_conn_outputs)){

	      //Only loads first matching profile
				if (VERBOSE) printf("Found matching profile\n");
        self->load_o.load_profile(&(self->load_o),cur_profile);
	break;
      }
    //}
  }

}
