#include "save.h"

/*! \file
		\brief Save current settings into the configuration file

*/



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

/*! \brief Save function main
 */

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

  // Get primary output
  xcb_randr_get_output_primary_cookie_t output_primary_cookie =
   xcb_randr_get_output_primary(self->screen_t_p->c,
                                self->screen_t_p->screen->root);
  xcb_randr_get_output_primary_reply_t *output_primary_reply =
   	xcb_randr_get_output_primary_reply(self->screen_t_p->c,
       output_primary_cookie, &self->screen_t_p->e);
  self->primary_output = output_primary_reply->output;
  printf("Primary output %d\n",self->primary_output);
  free(output_primary_reply);

	if (VERBOSE) printf("Before for each output\n");
  for_each_output((void *) self,self->screen_t_p->screen_resources_reply,
    check_output_status);

  //if (VERBOSE) printf("Made it here \n");
	//if (VERBOSE) printf("About to write to config file %s\n",CONFIG_FILE);
	config_write(self->config,CONF_FP);

	if (VERBOSE) printf("Done saving settings to profile\n");

}

/*! \brief For each output determine whether output is connected or enabled
 */

static void check_output_status(void *self_void,xcb_randr_output_t *output_p){

  xcb_randr_get_output_info_cookie_t output_info_cookie;
  char *edid_string,*output_name;

  save_class *self = (save_class *) self_void;
  self->cur_output = output_p;

  output_info_cookie =
  xcb_randr_get_output_info(self->screen_t_p->c,*output_p,XCB_CURRENT_TIME);
  self->output_info_reply =
    xcb_randr_get_output_info_reply(self->screen_t_p->c,
    output_info_cookie, &self->screen_t_p->e);



  if (!self->output_info_reply->connection){
    if (VERBOSE) printf("Found output that is connected\n");

    get_output_name(self->output_info_reply,&output_name);
    self->umon_setting.output_group =
     config_setting_add(self->umon_setting.mon_group,
   				              output_name,
   				              CONFIG_TYPE_GROUP);
    free(output_name);

    self->umon_setting.edid_setting =
  	config_setting_add(self->umon_setting.output_group,"EDID",
      CONFIG_TYPE_STRING);
    self->umon_setting.res_group =
  	 config_setting_add(self->umon_setting.output_group,
      "resolution",CONFIG_TYPE_GROUP);
  	self->umon_setting.res_x =
    	config_setting_add(self->umon_setting.res_group,"x",CONFIG_TYPE_INT);
  	self->umon_setting.res_y =
    	config_setting_add(self->umon_setting.res_group,"y",CONFIG_TYPE_INT);
  	self->umon_setting.pos_group =
     config_setting_add(self->umon_setting.output_group,"pos",
      CONFIG_TYPE_GROUP);
  	self->umon_setting.pos_x =
     config_setting_add(self->umon_setting.pos_group,"x",CONFIG_TYPE_INT);
  	self->umon_setting.pos_y =
     config_setting_add(self->umon_setting.pos_group,"y",CONFIG_TYPE_INT);

    fetch_edid(self->cur_output,self->screen_t_p,&edid_string);
    config_setting_set_string(self->umon_setting.edid_setting,edid_string);
    free(edid_string);

    if (self->output_info_reply->crtc){
    	if (VERBOSE) printf("Found output that is enabled :%d\n",*output_p);
      if (*output_p == self->primary_output){
        if (VERBOSE) printf("Found primary output\n");
        config_setting_t *primary_output_setting = config_setting_add(self->umon_setting.output_group,"primary",CONFIG_TYPE_INT);
        config_setting_set_int(primary_output_setting,1);

      }
      output_info_to_config(self);
    }
    else {
      disabled_to_config(self);
    }

  }
  free(self->output_info_reply);
}

/*! \brief Convert the raw output name obtained from server into char
 */
static void get_output_name(xcb_randr_get_output_info_reply_t *output_info_reply,char **output_name){
  int i;
  uint8_t *output_name_raw = xcb_randr_get_output_info_name(
    output_info_reply);
	int output_name_length =
    xcb_randr_get_output_info_name_length(output_info_reply);
	*output_name = (char *) malloc((output_name_length+1)*sizeof(char));

	for(i=0;i<output_name_length;++i){
		(*output_name)[i] = (char) output_name_raw[i];
	}
    (*output_name)[i] = '\0';

}

/*! \brief Save the crtc x,y and res x,y into config
 */
static void output_info_to_config(save_class *self){

	xcb_randr_get_crtc_info_cookie_t crtc_info_cookie;

	crtc_info_cookie =
    xcb_randr_get_crtc_info(self->screen_t_p->c,self->output_info_reply->crtc,
			self->screen_t_p->screen_resources_reply->config_timestamp);

	//if (VERBOSE) printf("cookies done\n");
	self->crtc_info_reply = xcb_randr_get_crtc_info_reply(self->screen_t_p->c,
    crtc_info_cookie,&self->screen_t_p->e);
  config_setting_set_int(self->umon_setting.pos_x,self->crtc_info_reply->x);
	config_setting_set_int(self->umon_setting.pos_y,self->crtc_info_reply->y);

	for_each_output_mode((void *) self,self->output_info_reply,
    find_res_to_config);

  free(self->crtc_info_reply);
}

/*! \brief Translate the mode id into resolution x,y
 */
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

/*! \brief Just save 0 for all settings if output is connected but disabled
 */
static void disabled_to_config(save_class *self){

  if (VERBOSE) printf("Found output that is disabled\n");

  config_setting_set_int(self->umon_setting.pos_x,0);
	config_setting_set_int(self->umon_setting.pos_y,0);
  config_setting_set_int(self->umon_setting.res_x,0);
  config_setting_set_int(self->umon_setting.res_y,0);

}
