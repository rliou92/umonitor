#include "save.h"

void save_profile_class_constructor(save_profile_class *self,
  screen_class *screen_t,config_t *config){
  self->save_profile = save_profile;
  self->screen_t_p = screen_t;
  self->config = config;
}


static void save_profile(save_profile_class *self,config_setting_t *profile_group){

	//int j,k;
  // int num_output_modes,num_screen_modes;

	// xcb_randr_get_screen_info_cookie_t screen_info_cookie;
	// xcb_randr_get_screen_info_reply_t *screen_info_reply;
	// xcb_randr_screen_size_iterator_t screen_size_iterator;
	// xcb_randr_screen_size_t screen_size;




	self->umon_setting.disp_group =
   config_setting_add(profile_group,"Screen",CONFIG_TYPE_GROUP);
	self->umon_setting.disp_width =
   config_setting_add(self->umon_setting.disp_group,"width",CONFIG_TYPE_INT);
	self->umon_setting.disp_height =
   config_setting_add(self->umon_setting.disp_group,"height",CONFIG_TYPE_INT);
	self->umon_setting.disp_widthMM =
		config_setting_add(self->umon_setting.disp_group,"widthMM",CONFIG_TYPE_INT);
	self->umon_setting.disp_heightMM =
		config_setting_add(self->umon_setting.disp_group,"heightMM",
      CONFIG_TYPE_INT);

	config_setting_set_int(self->umon_setting.disp_width,
    self->screen_t_p->screen->width_in_pixels);
	config_setting_set_int(self->umon_setting.disp_height,
    self->screen_t_p->screen->height_in_pixels);
	config_setting_set_int(self->umon_setting.disp_widthMM,
    self->screen_t_p->screen->width_in_millimeters);
	config_setting_set_int(self->umon_setting.disp_heightMM,
    self->screen_t_p->screen->height_in_millimeters);

	self->umon_setting.mon_group =
    config_setting_add(profile_group,"Monitors",CONFIG_TYPE_GROUP);
// Need to iterate over the outputs now


	// for_each_output(self->screen_t_p->screen_resources_reply,
    //&output_info_to_config,&disabled_to_config,&do_nothing);
  for_each_output((void *) self,self->screen_t_p->screen_resources_reply,
    check_output_status);

  //if (VERBOSE) printf("Made it here \n");
	if (VERBOSE) printf("About to write to config file %s\n",CONFIG_FILE);
	config_write_file(self->config,CONFIG_FILE);

	if (VERBOSE) printf("Done saving settings to profile\n");

}


static void check_output_status(void *self_void,xcb_randr_output_t *output_p){

  xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;

  save_profile_class *self = (save_profile_class *) self_void;
  self->cur_output = output_p;

  output_info_cookie =
  xcb_randr_get_output_info(self->screen_t_p->c, *output_p,
    XCB_CURRENT_TIME);
  output_info_reply =
    xcb_randr_get_output_info_reply (self->screen_t_p->c,
    output_info_cookie, self->screen_t_p->e);

  if (VERBOSE) printf("Looping over output %s\n",xcb_randr_get_output_info_name(output_info_reply));
  if (!output_info_reply->connection){
    if (VERBOSE) printf("Found output that is connected\n");

    if (output_info_reply->crtc){
      output_info_to_config(self);
    }
    else {
      disabled_to_config(self);
    }

  }
}




static void output_info_to_config(save_profile_class *self){

	xcb_randr_get_crtc_info_cookie_t crtc_info_cookie;
	xcb_randr_get_output_property_cookie_t output_property_cookie;
	xcb_randr_get_output_property_reply_t *output_property_reply;

	xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;

	char *edid_string;
	uint8_t *output_property_data;
	int output_property_length;
	uint8_t delete = 0;
	uint8_t pending = 0;

	output_info_cookie =
	xcb_randr_get_output_info(self->screen_t_p->c, *(self->cur_output),
    XCB_CURRENT_TIME);
	output_info_reply =
		xcb_randr_get_output_info_reply (self->screen_t_p->c, output_info_cookie,
    self->screen_t_p->e);


	self->umon_setting.output_group =
		config_setting_add(self->umon_setting.mon_group,
		    (char *) xcb_randr_get_output_info_name(output_info_reply),
				CONFIG_TYPE_GROUP);
	if (VERBOSE) printf("Found output that is enabled\n");
	self->umon_setting.edid_setting =
	config_setting_add(self->umon_setting.output_group,"EDID",CONFIG_TYPE_STRING);
	self->umon_setting.res_group =
	 config_setting_add(self->umon_setting.output_group,
    "resolution",CONFIG_TYPE_GROUP);
	self->umon_setting.res_x =
  	config_setting_add(self->umon_setting.res_group,"x",CONFIG_TYPE_INT);
	self->umon_setting.res_y =
  	config_setting_add(self->umon_setting.res_group,"y",CONFIG_TYPE_INT);
	self->umon_setting.pos_group =
   config_setting_add(self->umon_setting.output_group,"pos",CONFIG_TYPE_GROUP);
	self->umon_setting.pos_x =
   config_setting_add(self->umon_setting.pos_group,"x",CONFIG_TYPE_INT);
	self->umon_setting.pos_y =
   config_setting_add(self->umon_setting.pos_group,"y",CONFIG_TYPE_INT);
	if (VERBOSE) printf("Finish setting up settings\n");

	output_property_cookie = xcb_randr_get_output_property(self->screen_t_p->c,
		*(self->cur_output),self->screen_t_p->edid_atom->atom,AnyPropertyType,0,100,
    delete,pending);

	crtc_info_cookie =
  xcb_randr_get_crtc_info(self->screen_t_p->c,output_info_reply->crtc,
			self->screen_t_p->screen_resources_reply->config_timestamp);

	if (VERBOSE) printf("cookies done\n");
	self->crtc_info_reply =
  xcb_randr_get_crtc_info_reply(self->screen_t_p->c,crtc_info_cookie,
    self->screen_t_p->e);
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
	config_setting_set_string(self->umon_setting.edid_setting,edid_string);
	// Free edid_string?

	// Need to find the mode info now
	// Look at output crtc
	if (VERBOSE) printf("finished edid_setting config\n");
	for_each_output_mode((void *) self,output_info_reply,find_res_to_config);

	config_setting_set_int(self->umon_setting.pos_x,self->crtc_info_reply->x);
	config_setting_set_int(self->umon_setting.pos_y,self->crtc_info_reply->y);


}

static void find_res_to_config(void * self_void,xcb_randr_mode_t *mode_id_p){

	int num_screen_modes,k;
	//char res_string[10];
	xcb_randr_mode_info_iterator_t mode_info_iterator;
  save_profile_class *self = (save_profile_class *) self_void;
	if (VERBOSE) printf("current mode id %d\n",*mode_id_p);
	if (*mode_id_p == self->crtc_info_reply->mode){
		if (VERBOSE) printf("Found current mode id\n");
		// Get output info iterator
		 mode_info_iterator =
	xcb_randr_get_screen_resources_modes_iterator(
    self->screen_t_p->screen_resources_reply);
		num_screen_modes = xcb_randr_get_screen_resources_modes_length(
			self->screen_t_p->screen_resources_reply);
		 for (k=0;k<num_screen_modes;++k){
			 if (mode_info_iterator.data->id == *mode_id_p){
				 if (VERBOSE) printf("Found current mode info\n");
				 //sprintf(res_string,"%dx%d",mode_info_iterator.data->width,mode_info_iterator.data->height);
				 config_setting_set_int(self->umon_setting.res_x,
           mode_info_iterator.data->width);
				 config_setting_set_int(self->umon_setting.res_y,
           mode_info_iterator.data->height);
			 }
			xcb_randr_mode_info_next(&mode_info_iterator);

		 }
	}


}

static void disabled_to_config(save_profile_class *self){

	xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;

	output_info_cookie =
	xcb_randr_get_output_info(self->screen_t_p->c,*(self->cur_output),
    XCB_CURRENT_TIME);
	output_info_reply =
		xcb_randr_get_output_info_reply (self->screen_t_p->c,
    output_info_cookie,self->screen_t_p->e);
	self->umon_setting.output_group =
		config_setting_add(self->umon_setting.mon_group,
			(char *) xcb_randr_get_output_info_name(output_info_reply),
      CONFIG_TYPE_GROUP);
	if (VERBOSE) printf("Found output that is disabled\n");
	self->umon_setting.status =
	config_setting_add(self->umon_setting.output_group,"Status",
    CONFIG_TYPE_STRING);
	config_setting_set_string(self->umon_setting.status,"Disabled");


}


/*void for_each_output(
	void *self,
	xcb_randr_get_screen_t_p->screen_resources_reply_t *screen_t_p->screen_resources_reply,
  void (*con_enabled)(void *,xcb_randr_output_t *),
  void (*con_disabled)(void *,xcb_randr_output_t *),
  void (*discon)(void *)){

	int i;

	int outputs_length;

	xcb_randr_output_t *output_p;
	xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;

	output_p = xcb_randr_get_screen_resources_outputs(screen_t_p->screen_resources_reply);
	outputs_length =
		xcb_randr_get_screen_resources_outputs_length(screen_t_p->screen_resources_reply);

	for (i=0; i<outputs_length; ++i){
		output_info_cookie =
		xcb_randr_get_output_info(c, output_p, XCB_CURRENT_TIME);
		output_info_reply =
			xcb_randr_get_output_info_reply (c, output_info_cookie, e);

		if (VERBOSE) printf("Looping over output %s\n",xcb_randr_get_output_info_name(output_info_reply));
		if (!output_info_reply->connection){
			if (VERBOSE) printf("Found output that is connected\n");

			if (output_info_reply->crtc){
				(*con_enabled)(self,output_p);
			}
			else {
				(*con_disabled)(self,output_p);
			}

		}
		else {
			(*discon)(self);
		}

		++output_p;
	}


}
*/
