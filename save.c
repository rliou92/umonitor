typedef struct _save_profile_class{
  // Inheriting classes
  screen_class *screen_t_p;

  // Variables
  xcb_randr_get_screen_resources_reply_t *screen_resources_reply;
  umon_setting_p_t umon_setting;

  // Methods
  void (*save_profile)(struct _save_profile_class *,config_setting_t *);




}save_profile_class;



void save_profile_class_constructor(save_profile_class *,screen_class *);

// Public methods
static void save_profile(save_profile_class *,config_setting_t *);

// Private methods
static void output_info_to_config(save_profile_class *,xcb_randr_output_t *);
void for_each_output_mode(
	void (*output_mode_action)(xcb_randr_get_crtc_info_reply_t *,
	xcb_randr_mode_t *),
xcb_randr_get_output_info_reply_t *output_info_reply,
xcb_randr_get_crtc_info_reply_t *crtc_info_reply);
void find_res_to_config(xcb_randr_get_crtc_info_reply_t *,
	xcb_randr_mode_t *);
void disabled_to_config(xcb_randr_get_output_info_reply_t *);
void do_nothing(void);

void save_profile_class_constructor(save_profile_class *self,screen_class *screen_t){
  self->save_profile = save_profile;
  self->screen_class = screen_t;


}




static void save_profile(save_profile_class *self,config_setting_t *profile_group){

	int j,k,num_output_modes,num_screen_modes;

	// xcb_randr_get_screen_info_cookie_t screen_info_cookie;
	// xcb_randr_get_screen_info_reply_t *screen_info_reply;
	// xcb_randr_screen_size_iterator_t screen_size_iterator;
	xcb_randr_screen_size_t screen_size;

	xcb_randr_get_screen_resources_cookie_t screen_resources_cookie;


	self->umon_setting.disp_group = config_setting_add(profile_group,"Screen",CONFIG_TYPE_GROUP);
	self->umon_setting.disp_width = config_setting_add(disp_group,"width",CONFIG_TYPE_INT);
	self->umon_setting.disp_height = config_setting_add(disp_group,"height",CONFIG_TYPE_INT);
	self->umon_setting.disp_widthMM =
		config_setting_add(disp_group,"widthMM",CONFIG_TYPE_INT);
	self->umon_setting.disp_heightMM =
		config_setting_add(disp_group,"heightMM",CONFIG_TYPE_INT);

	config_setting_set_int(disp_width_setting,
    self->screen_t_p->screen->width_in_pixels);
	config_setting_set_int(disp_height_setting,
    self->screen_t_p->screen->height_in_pixels);
	config_setting_set_int(disp_widthMM_setting,
    self->screen_t_p->screen->width_in_millimeters);
	config_setting_set_int(disp_heightMM_setting,
    self->screen_t_p->screen->height_in_millimeters);

	self->umon_setting.mon_group =
    config_setting_add(profile_group,"Monitors",CONFIG_TYPE_GROUP);
// Need to iterate over the outputs now

	screen_resources_cookie =
    xcb_randr_get_screen_resources(c,self->umon_setting.screen->root);

	self->screen_resources_reply =
		xcb_randr_get_screen_resources_reply(c,screen_resources_cookie,e);

	for_each_output((void *) self,&output_info_to_config,&disabled_to_config,
		&do_nothing);

	if (verbose) printf("About to write to config file\n");
	config_write_file(&config,config_file);

	if (verbose) printf("Done saving settings to profile\n");

}

void do_nothing(void *self){

}


void edid_to_string(uint8_t *edid, int length, char **edid_string){

	/*
	 * Converts the edid that is returned from the X11 server into a string
	 * Inputs: 	edid	 	the bits return from X11 server
	 * Outputs: 	edid_string	 edid in string form
	 */

	int z;

	if (verbose) printf("Starting edid_to_string\n");
	*edid_string = (char *) malloc((length+1)*sizeof(char));
	for (z=0;z<length;++z) {
		if ((char) edid[z] == '\0') {
			*(*edid_string+z) = '0';
		}
		else {
			*(*edid_string+z) = (char) edid[z];
		}
		//printf("\n");
		//printf("%c",*((*edid_string)+z));
	}
	*(*edid_string+z) = '\0';

	if (verbose) printf("Finished edid_to_string\n");
}


static void output_info_to_config(void *self_void,xcb_randr_output_t *output_p){

	xcb_randr_get_crtc_info_cookie_t crtc_info_cookie;
	xcb_randr_get_output_property_cookie_t output_property_cookie;
	xcb_randr_get_output_property_reply_t *output_property_reply;

	xcb_randr_get_crtc_info_reply_t *crtc_info_reply;
	xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;

	char *edid_string;
	uint8_t *output_property_data;
	int output_property_length;
	uint8_t delete = 0;
	uint8_t pending = 0;
  save_profile_class *self;

  self = (save_profile_class *) self_void;
	output_info_cookie =
	xcb_randr_get_output_info(self->screen_t_p->c, output_p, XCB_CURRENT_TIME);
	output_info_reply =
		xcb_randr_get_output_info_reply (self->screen_t_p->c, output_info_cookie,
    self->screen_t_p->e);


	self->umon_setting.output_group =
		config_setting_add(self->umon_setting.mon_group,
											xcb_randr_get_output_info_name(output_info_reply),
											CONFIG_TYPE_GROUP);
	if (verbose) printf("Found output that is enabled\n");
	self->umon_setting.edid_setting =
	config_setting_add(output_group,"EDID",CONFIG_TYPE_STRING);
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
	if (verbose) printf("Finish setting up settings\n");

	output_property_cookie = xcb_randr_get_output_property(self->screen_t_p->c,
		*output_p,self->screen_t_p->edid_atom->atom,AnyPropertyType,0,100,
    delete,pending);

	crtc_info_cookie = xcb_randr_get_crtc_info(c,output_info_reply->crtc,
			screen_resources_reply->config_timestamp);

			if (verbose) printf("cookies done\n");
	crtc_info_reply = xcb_randr_get_crtc_info_reply(c,crtc_info_cookie,e);
	output_property_reply = xcb_randr_get_output_property_reply(
		c,output_property_cookie,e);

	if (verbose) printf("output_property_reply %d\n",output_property_reply);
	output_property_data = xcb_randr_get_output_property_data(
		output_property_reply);
	output_property_length = xcb_randr_get_output_property_data_length(
		output_property_reply);

	if (verbose) printf("Finish fetching info from server\n");
	edid_to_string(output_property_data,output_property_length,
		&edid_string);
	config_setting_set_string(self->umon_setting.edid_setting,edid_string);
	// Free edid_string?

	// Need to find the mode info now
	// Look at output crtc
	if (verbose) printf("finished edid_setting config\n");
	for_each_output_mode(&find_res_to_config,output_info_reply,
		crtc_info_reply);

	config_setting_set_int(self->umon_setting.pos_x,crtc_info_reply->x);
	config_setting_set_int(self->umon_setting.pos_y,crtc_info_reply->y);


}

void for_each_output_mode(
	void (*output_mode_action)(xcb_randr_get_crtc_info_reply_t *,
		xcb_randr_mode_t *),
xcb_randr_get_output_info_reply_t *output_info_reply,
xcb_randr_get_crtc_info_reply_t *crtc_info_reply){

	int j,num_output_modes;

	xcb_randr_mode_t *mode_id_p;

	num_output_modes =
		xcb_randr_get_output_info_modes_length(output_info_reply);
	if (verbose) printf("number of modes %d\n",num_output_modes);
	mode_id_p = xcb_randr_get_output_info_modes(output_info_reply);
	if (verbose) printf("First mode id %d\n",*mode_id_p);
	if (verbose) printf("current crtc mode id %d\n",crtc_info_reply->mode);
	for (j=0;j<num_output_modes;++j){
		(*output_mode_action)(crtc_info_reply,mode_id_p);
		++mode_id_p;
	}


}


void find_res_to_config(xcb_randr_get_crtc_info_reply_t *crtc_info_reply,
	xcb_randr_mode_t *mode_id_p){

	int num_screen_modes,k;
	char res_string[10];
	xcb_randr_mode_info_iterator_t mode_info_iterator;
	//if (verbose) printf("current mode id %d\n",*mode_id_p);
	if (*mode_id_p == crtc_info_reply->mode){
		if (verbose) printf("Found current mode id\n");
		// Get output info iterator
		 mode_info_iterator =
	xcb_randr_get_screen_resources_modes_iterator(screen_resources_reply);
		num_screen_modes = xcb_randr_get_screen_resources_modes_length(
			screen_resources_reply);
		 for (k=0;k<num_screen_modes;++k){
			 if (mode_info_iterator.data->id == *mode_id_p){
				 if (verbose) printf("Found current mode info\n");
				 //sprintf(res_string,"%dx%d",mode_info_iterator.data->width,mode_info_iterator.data->height);
				 config_setting_set_string(res_x,mode_info_iterator.data->width);
				 config_setting_set_string(res_y,mode_info_iterator.data->height);
			 }
			xcb_randr_mode_info_next(&mode_info_iterator);

		 }
	}


}

void disabled_to_config(xcb_randr_get_output_info_reply_t *output_info_reply){

	xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;

	output_info_cookie =
	xcb_randr_get_output_info(c, output_p, XCB_CURRENT_TIME);
	output_info_reply =
		xcb_randr_get_output_info_reply (c, output_info_cookie, e);
	output_group =
		config_setting_add(mon_group,
											xcb_randr_get_output_info_name(output_info_reply),
											CONFIG_TYPE_GROUP);
	if (verbose) printf("Found output that is disabled\n");
	status_setting =
	config_setting_add(output_group,"Status",CONFIG_TYPE_STRING);
	config_setting_set_string(status_setting,"Disabled");


}
