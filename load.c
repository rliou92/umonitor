#include "load.h"

void load_class_constructor(load_class *self,
  screen_class *screen_t,config_t *config){

  self->load_profile = load_profile;
  self->screen_t_p = screen_t;
  self->config = config;
  self->crtc_param_head = NULL;
}


static void load_profile(load_class *self,config_setting_t *profile_group){

	//config_setting_t *pos_group,*group,*mon_group,*res_group;
	xcb_randr_crtc_t *crtcs_p;
	xcb_randr_set_crtc_config_cookie_t crtc_config_p;
	xcb_randr_set_crtc_config_reply_t *crtc_config_reply_pp;
  int i;

  set_crtc_param *cur_crtc_param;

  self->profile_group = profile_group;
  load_config_val(self);

/*
	 * Plan of attack
	 * 1. Disable all crtcs
	 * 2. Resize the screen
	 * 3. Enabled desired crtcs
*/

	crtcs_p = xcb_randr_get_screen_resources_crtcs(
    self->screen_t_p->screen_resources_reply);
	//crtc_config_p = (xcb_randr_set_crtc_config_cookie_t *)
    //malloc(
      //self->screen_t_p->screen_resources_reply->num_crtcs*sizeof(
      //xcb_randr_set_crtc_config_cookie_t));

	self->crtc_offset = 0;
	for_each_output(
    (void *) self,self->screen_t_p->screen_resources_reply,match_with_config);

	for(i=0;i<self->screen_t_p->screen_resources_reply->num_crtcs;++i){
    if (!VERBOSE) {
		//crtc_config_p[i] =
	printf("Disabling this crtc: %d\n",crtcs_p[i]);
    crtc_config_p = xcb_randr_set_crtc_config(self->screen_t_p->c,crtcs_p[i],
      XCB_CURRENT_TIME,
      XCB_CURRENT_TIME,0,0,XCB_NONE,
      XCB_RANDR_ROTATION_ROTATE_0,0,NULL);
    crtc_config_reply_pp = xcb_randr_set_crtc_config_reply(self->screen_t_p->c,crtc_config_p,&self->screen_t_p->e);
    printf("crtc_config_reply_pp: %d\n",crtc_config_reply_pp->status);
      printf("disable crtcs here\n");
    }
    else{
      printf("Would disable crtcs here\n");
    }
	}

// 	for(i=0;i<self->screen_t_p->screen_resources_reply->num_crtcs;++i){
// 		if (!VERBOSE) {
//       crtc_config_reply_pp[i] =
//     xcb_randr_set_crtc_config_reply(self->screen_t_p->c,crtc_config_p[i],
//       self->screen_t_p->e);
//     }
//     else {
//       printf("Would disable crtcs here\n");
//     }
// 	}

	if (!VERBOSE) {
		//Ignore saved widthMM and heightMM
		int widthMM = 25.4*self->umon_setting_val.screen.width/96;
		int heightMM = 25.4*self->umon_setting_val.screen.height/96;
    printf("screen size width: %d\n",self->umon_setting_val.screen.width);
    printf("screen size height: %d\n",self->umon_setting_val.screen.height);
    printf("screen size widthmm: %d\n",widthMM);
    printf("screen size heightmm: %d\n",heightMM);
    // TODO don't believe this is working
    xcb_void_cookie_t void_cookie = xcb_randr_set_screen_size(self->screen_t_p->c,
      self->screen_t_p->screen->root,(uint16_t)self->umon_setting_val.screen.width,
      (uint16_t)self->umon_setting_val.screen.height,
      (uint32_t)self->umon_setting_val.screen.widthMM,
      (uint32_t)self->umon_setting_val.screen.heightMM);
      xcb_flush(self->screen_t_p->c);
    printf("change screen size here\n");
	}
  else{
    printf("Would change screen size here\n");
  }

  if (!VERBOSE) {
    printf("enable crtcs here\n");
    // TODO check if the crtc can actually be connected to the output
    i = 0;
    for(cur_crtc_param=self->crtc_param_head;cur_crtc_param;cur_crtc_param=cur_crtc_param->next){

	// printf("*(cur_crtc_param->crtc_p): %d\n",cur_crtc);
	// printf("cur_crtc_param->output_p: %d\n",cur_crtc_param->output_p);
	// printf("*(cur_crtc_param->mode_id_p): %d\n",cur_crtc_param->mode_id);
    printf("Enabling crtc %d\n",cur_crtc_param->crtc);
     crtc_config_p = xcb_randr_set_crtc_config(self->screen_t_p->c,
        cur_crtc_param->crtc,
        XCB_CURRENT_TIME,XCB_CURRENT_TIME,
        cur_crtc_param->pos_x,
        cur_crtc_param->pos_y,
        cur_crtc_param->mode_id,XCB_RANDR_ROTATION_ROTATE_0, 1, cur_crtc_param->output_p);
    //if(crtc_config_reply_pp->status==XCB_RANDR_SET_CONFIG_SUCCESS) printf("Enabling crtc should be success\n");
    crtc_config_reply_pp = xcb_randr_set_crtc_config_reply(self->screen_t_p->c,crtc_config_p,&self->screen_t_p->e);
    xcb_flush(self->screen_t_p->c);
    //printf("crtc_config_reply_pp: %d\n",crtc_config_reply_pp->response_type);

    }
  }
  else{
    printf("Would enable crtcs here\n");
  }

//	if (!VERBOSE) {
//    printf("screen size width: %d\n",self->umon_setting_val.screen.width);
//    printf("screen size height: %d\n",self->umon_setting_val.screen.height);
//    printf("screen size widthmm: %d\n",self->umon_setting_val.screen.widthMM);
//    printf("screen size heightmm: %d\n",self->umon_setting_val.screen.heightMM);
//    // TODO don't believe this is working
//    xcb_void_cookie_t void_cookie = xcb_randr_set_screen_size_checked(self->screen_t_p->c,
//      self->screen_t_p->screen->root,(uint16_t)self->umon_setting_val.screen.width,
//      (uint16_t)self->umon_setting_val.screen.height,
//      (uint32_t)self->umon_setting_val.screen.widthMM,
//      (uint32_t)self->umon_setting_val.screen.heightMM);
//      xcb_flush(self->screen_t_p->c);
//    printf("change screen size here\n");
//	}
//  else{
//    printf("Would change screen size here\n");
//  }

}

static void match_with_config(void *self_void,xcb_randr_output_t *output_p){


	xcb_randr_get_output_info_cookie_t output_info_cookie;
  xcb_randr_get_output_property_cookie_t output_property_cookie;
  xcb_randr_get_output_property_reply_t *output_property_reply;
  load_class *self = (load_class *) self_void;

	char *edid_string;
	uint8_t *output_property_data;
	int output_property_length;
	uint8_t delete = 0;
	uint8_t pending = 0;

  self->cur_output = output_p;
	output_info_cookie =
	xcb_randr_get_output_info(self->screen_t_p->c,*output_p,XCB_CURRENT_TIME);

  // TODO Duplicate code, fetching edid info
	output_property_cookie = xcb_randr_get_output_property(self->screen_t_p->c,
    *output_p,self->screen_t_p->edid_atom->atom,AnyPropertyType,0,100,
    delete,pending);
	self->output_info_reply =
		xcb_randr_get_output_info_reply (self->screen_t_p->c,output_info_cookie,
      &self->screen_t_p->e);
	output_property_reply = xcb_randr_get_output_property_reply(
		self->screen_t_p->c,output_property_cookie,&self->screen_t_p->e);
	output_property_data = xcb_randr_get_output_property_data(
		output_property_reply);
	output_property_length = xcb_randr_get_output_property_data_length(
		output_property_reply);

	edid_to_string(output_property_data,output_property_length,
		&edid_string);

	if (VERBOSE) printf("Num out pp: %d\n",self->num_out_pp);

	for(self->conf_output_idx=0;self->conf_output_idx<self->num_out_pp;++self->conf_output_idx){
		if (!strcmp(self->umon_setting_val.outputs[self->conf_output_idx].edid_val,
                edid_string)){
			if (VERBOSE) printf("Before for each output mode\n");
			// Found a match between the configuration file edid and currently connected edid. Now have to load correct settings.
			// Need to find the proper crtc to assign the output
			// Which crtc has the same resolution?
			// for_each_output_mode((void *) self,self->output_info_reply,
      //   find_mode_id);
      find_mode_id(self);
			//find_crtc_by_res(mySett.res_x,mySett.res_y);
			// Connect correct crtc to correct output

  //
	// printf("*(cur_crtc_param->crtc_p): %d\n",self->umon_setting_val.outputs[self->conf_output_idx].crtc_id);
	// printf("cur_crtc_param->output_p: %d\n",self->cur_output);
	// printf("*(cur_crtc_param->mode_id_p): %d\n",
  //   self->umon_setting_val.outputs[self->conf_output_idx].mode_id);
			//crtc_config_p = xcb_randr_set_crtc_config(c,crtcs_p[i],XCB_CURRENT_TIME, self->screen_t_p->screen_resources_reply->config_timestamp,0,0,XCB_NONE,  XCB_RANDR_ROTATION_ROTATE_0,NULL,0);



		}

	}



}

static void find_mode_id(load_class *self){

  int num_screen_modes,k;
	//char res_string[10];
	xcb_randr_mode_info_iterator_t mode_info_iterator;
  set_crtc_param *new_crtc_param;

  mode_info_iterator =
    xcb_randr_get_screen_resources_modes_iterator(
     self->screen_t_p->screen_resources_reply);
	num_screen_modes = xcb_randr_get_screen_resources_modes_length(
		self->screen_t_p->screen_resources_reply);
	 for (k=0;k<num_screen_modes;++k){
		 if ((mode_info_iterator.data->width ==
        self->umon_setting_val.outputs[self->conf_output_idx].res_x) &&
        (mode_info_iterator.data->height ==
        self->umon_setting_val.outputs[self->conf_output_idx].res_y)){
			 if (VERBOSE) printf("Found current mode info\n");
			 //sprintf(res_string,"%dx%d",mode_info_iterator.data->width,mode_info_iterator.data->height);
           new_crtc_param = (set_crtc_param *) malloc(sizeof(set_crtc_param));
           new_crtc_param->crtc = find_available_crtc(self,self->crtc_offset++);
           new_crtc_param->pos_x =
             self->umon_setting_val.outputs[self->conf_output_idx].pos_x;
           new_crtc_param->pos_y =
             self->umon_setting_val.outputs[self->conf_output_idx].pos_y;
           new_crtc_param->mode_id =
             mode_info_iterator.data->id;
           new_crtc_param->output_p = self->cur_output;
           new_crtc_param->next = self->crtc_param_head;
           self->crtc_param_head = new_crtc_param;
       //config_setting_set_int(self->umon_setting.mode_id,
           //(int) *mode_id_p);
			 }
			xcb_randr_mode_info_next(&mode_info_iterator);

		 }
     		//if (VERBOSE) printf("Found current mode id\n");
}

static xcb_randr_crtc_t find_available_crtc(load_class *self,int offset){


  xcb_randr_crtc_t *output_crtcs =
    xcb_randr_get_output_info_crtcs(self->output_info_reply);
  int num_output_crtcs =
   xcb_randr_get_output_info_crtcs_length(self->output_info_reply);

  xcb_randr_crtc_t *available_crtcs = xcb_randr_get_screen_resources_crtcs(
    self->screen_t_p->screen_resources_reply);
  int num_available_crtcs = self->screen_t_p->screen_resources_reply->num_crtcs;

  for (int i=0;i<num_available_crtcs;++i){
    for (int j=0;j<num_output_crtcs;++j){
        if (available_crtcs[i] == output_crtcs[j]){
  printf("offset crtc: %d\n",offset);
  		if(!(offset--)) return available_crtcs[i];
          // This is returning the same crtc unfortunately
        }
      }
  }

  printf("failed to find a matching crtc\n");
  exit(10);

}

static void load_config_val(load_class *self){

	// TODO should use an array of structures
	config_setting_t *mon_group,*group,*res_group,*pos_group;
	int i;
	mon_group = config_setting_lookup(self->profile_group,"Monitors");
	// printf("Checking group %d\n",mon_group);
	self->num_out_pp = config_setting_length(mon_group);
  self->umon_setting_val.outputs =
    (umon_setting_output_t *) malloc(self->num_out_pp *
       sizeof(umon_setting_output_t));

	for(i=0;i<self->num_out_pp;++i) {
		group = config_setting_get_elem(mon_group,i);
		// printf("Checking group %d\n",group);
		res_group = config_setting_lookup(group,"resolution");
		pos_group = config_setting_lookup(group,"pos");
		config_setting_lookup_string(group,"EDID",
			&(self->umon_setting_val.outputs[i].edid_val));
		config_setting_lookup_int(res_group,"x",
      &(self->umon_setting_val.outputs[i].res_x));
		config_setting_lookup_int(res_group,"y",
      &(self->umon_setting_val.outputs[i].res_y));
		config_setting_lookup_int(pos_group,"x",
      &(self->umon_setting_val.outputs[i].pos_x));
		config_setting_lookup_int(pos_group,"y",
      &(self->umon_setting_val.outputs[i].pos_y));
    // config_setting_lookup_int(group,"crtc_id",
    //   &(self->umon_setting_val.outputs[i].crtc_id));
    // config_setting_lookup_int(group,"mode_id",
    //   &(self->umon_setting_val.outputs[i].mode_id));
		// printf("Loaded values: \n");
		// printf("EDID: %s\n",*(mySett->edid_val+i));
		// printf("Resolution: %s\n",*(mySett->resolution_str+i));
		// printf("Pos: x=%d y=%d\n",*(mySett->pos_val+2*i),*(mySett->pos_val+2*i+1));
	}

	group = config_setting_lookup(self->profile_group,"Screen");
	config_setting_lookup_int(group,"width",
    &(self->umon_setting_val.screen.width));
	config_setting_lookup_int(group,"height",
    &(self->umon_setting_val.screen.height));
	config_setting_lookup_int(group,"widthMM",
    &(self->umon_setting_val.screen.widthMM));
	config_setting_lookup_int(group,"heightMM",
    &(self->umon_setting_val.screen.heightMM));

	if (VERBOSE) printf("Done loading values from configuration file\n");


}
