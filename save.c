#include "save.h"

void save_class_constructor(save_class **self,screen_class *screen_t,
  config_t *config){

	*self = (save_class *) malloc(sizeof(save_class));
  (*self)->save_profile = save_profile;
  (*self)->screen_t_p = screen_t;
  (*self)->config = config;
  if (VERBOSE) printf("Done constructing save class\n");

}

void save_class_destructor(save_class *self){
  free(self);
}


static void save_profile(save_class *self,config_setting_t *profile_group){

	if (VERBOSE) printf("Before for each output\n");
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

	//if (VERBOSE) printf("Screen width in pixels: %d\n",self->screen_t_p->screen->width_in_pixels);
	config_setting_set_int(self->umon_setting.disp_width,
    self->screen_t_p->screen->width_in_pixels);
	//if (VERBOSE) printf("Screen height in pixels: %d\n",self->screen_t_p->screen->height_in_pixels);
	config_setting_set_int(self->umon_setting.disp_height,
    self->screen_t_p->screen->height_in_pixels);
	//printf("Screen width in millimeters: %d\n",self->screen_t_p->screen->width_in_millimeters);
	config_setting_set_int(self->umon_setting.disp_widthMM,
    self->screen_t_p->screen->width_in_millimeters);
	config_setting_set_int(self->umon_setting.disp_heightMM,
    self->screen_t_p->screen->height_in_millimeters);

	self->umon_setting.mon_group =
    config_setting_add(profile_group,"Monitors",CONFIG_TYPE_GROUP);

	if (VERBOSE) printf("Before for each output\n");
  for_each_output((void *) self,self->screen_t_p->screen_resources_reply,
    check_output_status);

  //if (VERBOSE) printf("Made it here \n");
	if (VERBOSE) printf("About to write to config file %s\n",CONFIG_FILE);
	config_write_file(self->config,CONFIG_FILE);

	if (VERBOSE) printf("Done saving settings to profile\n");

}


static void check_output_status(void *self_void,xcb_randr_output_t *output_p){

  xcb_randr_get_output_info_cookie_t output_info_cookie;

  save_class *self = (save_class *) self_void;
  self->cur_output = output_p;

  output_info_cookie =
  xcb_randr_get_output_info(self->screen_t_p->c,*output_p,XCB_CURRENT_TIME);
  self->output_info_reply =
    xcb_randr_get_output_info_reply(self->screen_t_p->c,
    output_info_cookie, &self->screen_t_p->e);

  //if (VERBOSE) printf("Looping over output %s\n",xcb_randr_get_output_info_name(output_info_reply));
  if (!self->output_info_reply->connection){
    if (VERBOSE) printf("Found output that is connected\n");

    if (self->output_info_reply->crtc){
    	if (VERBOSE) printf("Found output that is enabled\n");
      output_info_to_config(self);
    }
    else {
      disabled_to_config(self);
    }

  }
  free(self->output_info_reply);
}




static void output_info_to_config(save_class *self){

	xcb_randr_get_crtc_info_cookie_t crtc_info_cookie;
  int i;
	char *edid_string,*output_name;

	uint8_t *output_name_raw = xcb_randr_get_output_info_name(
    self->output_info_reply);
	int output_name_length =
    xcb_randr_get_output_info_name_length(self->output_info_reply);
	output_name = malloc((output_name_length+1)*sizeof(char));

	for(i=0;i<output_name_length;++i){
		output_name[i] = (char) output_name_raw[i];
	}
  output_name[i] = '\0';

	self->umon_setting.output_group =
		config_setting_add(self->umon_setting.mon_group,
				output_name,
				CONFIG_TYPE_GROUP);
  free(output_name);

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

	//if (VERBOSE) printf("Finish setting up settings\n");

  fetch_edid(self->cur_output,self->screen_t_p,&edid_string);
	config_setting_set_string(self->umon_setting.edid_setting,edid_string);
  free(edid_string);

	crtc_info_cookie =
  xcb_randr_get_crtc_info(self->screen_t_p->c,self->output_info_reply->crtc,
			self->screen_t_p->screen_resources_reply->config_timestamp);

	//if (VERBOSE) printf("cookies done\n");
	self->crtc_info_reply =
  xcb_randr_get_crtc_info_reply(self->screen_t_p->c,crtc_info_cookie,
    &self->screen_t_p->e);
  config_setting_set_int(self->umon_setting.pos_x,self->crtc_info_reply->x);
	config_setting_set_int(self->umon_setting.pos_y,self->crtc_info_reply->y);


	//if (VERBOSE) printf("Finish fetching info from server\n");

	// Need to find the mode info now
	// Look at output crtc
	//if (VERBOSE) printf("finished edid_setting config\n");
	for_each_output_mode((void *) self,self->output_info_reply,
    find_res_to_config);

  free(self->crtc_info_reply);
}

static void find_res_to_config(void * self_void,xcb_randr_mode_t *mode_id_p){

	int num_screen_modes,k;

	xcb_randr_mode_info_iterator_t mode_info_iterator;
  save_class *self = (save_class *) self_void;
	if (VERBOSE) printf("current mode id %d\n",*mode_id_p);
	if (*mode_id_p == self->crtc_info_reply->mode){
		if (VERBOSE) printf("Found current mode id\n");
		// Get output info iterator
	  mode_info_iterator =	xcb_randr_get_screen_resources_modes_iterator(
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

static void disabled_to_config(save_class *self){

  // TODO Implement

	// xcb_randr_get_output_info_cookie_t output_info_cookie;
	// xcb_randr_get_output_info_reply_t *output_info_reply;
  //
	// output_info_cookie =
	// xcb_randr_get_output_info(self->screen_t_p->c,*(self->cur_output),
  //   XCB_CURRENT_TIME);
	// output_info_reply =
	// 	xcb_randr_get_output_info_reply (self->screen_t_p->c,
  //   output_info_cookie,&self->screen_t_p->e);
	// self->umon_setting.output_group =
	// 	config_setting_add(self->umon_setting.mon_group,
	// 		(char *) xcb_randr_get_output_info_name(output_info_reply),
  //     CONFIG_TYPE_GROUP);
	// if (VERBOSE) printf("Found output that is disabled\n");
	// self->umon_setting.status =
	// config_setting_add(self->umon_setting.output_group,"Status",
  //   CONFIG_TYPE_STRING);
	// config_setting_set_string(self->umon_setting.status,"Disabled");
  //
  // free(output_info_reply);


}
