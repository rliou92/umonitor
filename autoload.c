




void autoload_constructor(autoload_class *self,screen_class *screen_o,config_t *config){

	xcb_generic_event_t *evt;
	xcb_randr_screen_change_notify_event_t *randr_evt;
	xcb_timestamp_t last_time;

  self->screen_t_p = screen_o;
  self->config = config;

	xcb_randr_select_input(screen_o->c,screen_o->screen->root,
		XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE);
  xcb_flush(screen_o->c);

  while ((evt = xcb_wait_for_event(screen_o->c)) != NULL) {
    if (evt->response_type & XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE) {
      randr_evt = (xcb_randr_screen_change_notify_event_t*) evt;
      if (last_time != randr_evt->timestamp) {
        last_time = randr_evt->timestamp;
        // Find matching profile
				// Get total connected outputs
				screen_o->update_screen(screen_o);
				for_each_output((void *) self,screen_o->screen_resources_reply,
					match_with_profile);
      }
    }
    free(evt);
  }
  //xcb_disconnect(conn);
}


void match_with_profile(void *self_void,xcb_randr_output_t *output_p){
	// For each profile
	int i,j,match,output_match_unique,num_out_pp,num_profiles;
	umon_setting_val_t umon_setting_val;
  config_setting_t *root,*cur_profile,*mon_group,*group;
  autoload_class *self = (autoload_class *) self_void;
  char *conf_edid,*conn_edid;

  xcb_randr_get_output_property_cookie_t output_property_cookie;
  xcb_randr_get_output_property_reply_t *output_property_reply;

  xcb_randr_get_output_info_cookie_t output_info_cookie;
  xcb_randr_get_output_info_reply_t *output_info_reply;

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

  conn_edid =

  root = config_root_setting(self->config);
  num_profiles = config_setting_length(root);

  for (i=0;i<num_profiles;i++){
    cur_profile = config_setting_get_elem(root,i);
    mon_group = config_setting_lookup(cur_profile,"Monitors");
    num_out_pp = config_setting_length(mon_group);

    if (num_out_pp == self->screen_t_p->screen_resources_reply->num_outputs){
      for(j=0;j<num_out_pp;j++){
        group = config_setting_get_elem(mon_group,j);
        config_setting_lookup_string(group,"EDID",conf_edid);
        if (!strcmp(conf_edid,))
      }

    }

  }



}
