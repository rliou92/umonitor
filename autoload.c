#include "autoload.h"




void autoload_constructor(autoload_class *self,screen_class *screen_o,config_t *config){

	xcb_generic_event_t *evt;
	xcb_randr_screen_change_notify_event_t *randr_evt;
	xcb_timestamp_t last_time;
  config_setting_t *root,*cur_profile,*mon_group,*group;
  int i,j,output_match_unique=0,num_profiles;
  load_class *load_o;



  self->screen_t_p = screen_o;
  self->config = config;


	xcb_randr_select_input(screen_o->c,screen_o->screen->root,
		XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
  xcb_flush(screen_o->c);

  if (test_run){
    find_profile_and_load(self);
  }
  else {
    while ((evt = xcb_wait_for_event(screen_o->c)) != NULL) {
      if (evt->response_type & XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE) {
        randr_evt = (xcb_randr_screen_change_notify_event_t*) evt;
        if (last_time != randr_evt->timestamp) {
          last_time = randr_evt->timestamp;
          // Find matching profile
  				// Get total connected outputs
          find_profile_and_load(self);

        }
      }
      free(evt);
    }

  }

  //xcb_disconnect(conn);
}


static void match_with_profile(void *self_void,xcb_randr_output_t *output_p){
	// For each profile
	umon_setting_val_t umon_setting_val;
  autoload_class *self = (autoload_class *) self_void;
  char *conf_edid,*edid_string;

  xcb_randr_get_output_property_cookie_t output_property_cookie;
  xcb_randr_get_output_property_reply_t *output_property_reply;

  xcb_randr_get_output_info_cookie_t output_info_cookie;
  xcb_randr_get_output_info_reply_t *output_info_reply;

  int j,output_match_unique;

  output_property_cookie = xcb_randr_get_output_property(self->screen_t_p->c,
    *(output_p),self->screen_t_p->edid_atom->atom,AnyPropertyType,0,100,
    delete,pending);
  output_property_reply = xcb_randr_get_output_property_reply(
      self->screen_t_p->c,output_property_cookie,self->screen_t_p->e);

  //if (VERBOSE) printf("output_property_reply %d\n",output_property_reply);
  output_property_data = xcb_randr_get_output_property_data(
    output_property_reply);
  output_property_length = xcb_randr_get_output_property_data_length(
    output_property_reply);

  if (VERBOSE) printf("Finish fetching info from server\n");
  edid_to_string(output_property_data,output_property_length,
    &edid_string);

  for(j=0;j<self->num_out_pp;j++){

    output_match_unique = 0;
    group = config_setting_get_elem(mon_group,j);
    config_setting_lookup_string(group,"EDID",conf_edid);
    if (!strcmp(conf_edid,edid_string)){
      output_match_unique++;
    }
  }

  if (output_match_unique == 1){
    self->output_match++;
  }
}

static void find_profile_and_load(autoload_class *self){

  screen_o->update_screen(screen_o);

  root = config_root_setting(config);
  num_profiles = config_setting_length(root);
  for (i=0;i<num_profiles;i++){
    self->output_match = 0;
    cur_profile = config_setting_get_elem(root,i);
    self->mon_group = config_setting_lookup(cur_profile,"Monitors");
    self->num_out_pp = config_setting_length(mon_group);
    if (self->num_out_pp ==
       self->screen_t_p->screen_resources_reply->num_outputs){

      for_each_output((void *) self,screen_o->screen_resources_reply,
        match_with_profile);
      if (self->output_match == self->num_out_pp){
        load_o = (load_class *) malloc(sizeof(load_class));
        load_class_constructor(load_o,screen_o,config);
        load_o->load_profile(load_o,profile_group);

      }
    }
  }

}
