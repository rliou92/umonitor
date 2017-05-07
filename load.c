#include "load.h"

void load_class_constructor(load_class **self,screen_class *screen_t){

  *self = (load_class *) malloc(sizeof(load_class));
  (*self)->load_profile = load_profile;
  (*self)->screen_t_p = screen_t;
  (*self)->crtc_param_head = NULL;
  (*self)->umon_setting_val.outputs = NULL;
  (*self)->output_info_reply = NULL;

}

void load_class_destructor(load_class *self){

  free(self->umon_setting_val.outputs);
  free(self->output_info_reply);

  free(self);

}

static void load_profile(load_class *self,config_setting_t *profile_group){

  set_crtc_param *cur_crtc_param;

  xcb_randr_get_crtc_info_cookie_t crtc_info_cookie;
  xcb_randr_get_crtc_info_reply_t *crtc_info_reply;
  xcb_randr_output_t *conn_output;
  int i,crtc_match;



  self->profile_group = profile_group;
  load_config_val(self);

/*
	 * Plan of attack
	 * 1. Disable all crtcs
	 * 2. Resize the screen
	 * 3. Enabled desired crtcs
*/

	self->crtcs_p = xcb_randr_get_screen_resources_crtcs(
    self->screen_t_p->screen_resources_reply);

	self->crtc_offset = 0;
  self->crtc_ll_length = 0;
	for_each_output(
    (void *) self,self->screen_t_p->screen_resources_reply,match_with_config);

  // Check against redaundant loading
  //TODO Only assuming 1 output connected to crtc
  crtc_match = 0;
  for(i=0;i<self->screen_t_p->screen_resources_reply->num_crtcs;++i){
    crtc_info_cookie =
      xcb_randr_get_crtc_info(self->screen_t_p->c,self->crtcs_p[i],
        XCB_CURRENT_TIME);
    crtc_info_reply = xcb_randr_get_crtc_info_reply(self->screen_t_p->c,
          crtc_info_cookie,&self->screen_t_p->e);
    conn_output = xcb_randr_get_crtc_info_outputs(crtc_info_reply);
    for(cur_crtc_param=self->crtc_param_head;cur_crtc_param;
        cur_crtc_param=cur_crtc_param->next){

          // printf("current crtc params: %d,%d,%d,%d\n",cur_crtc_param->pos_x,
          // cur_crtc_param->pos_y,cur_crtc_param->mode_id,
          // cur_crtc_param->output_p[0]);
          // printf("comparing to crtc: %d,%d,%d,%d\n",crtc_info_reply->x,
          // crtc_info_reply->y,crtc_info_reply->mode,conn_output[0]);
      if(cur_crtc_param->pos_x == crtc_info_reply->x &&
         cur_crtc_param->pos_y == crtc_info_reply->y &&
         cur_crtc_param->mode_id == crtc_info_reply->mode &&
         cur_crtc_param->output_p[0] == conn_output[0]){
           crtc_match++;
         }

    }
    free(crtc_info_reply);
  }
  if (crtc_match != self->crtc_ll_length){
    apply_settings(self);
    self->cur_loaded = 0;
  }
  else{
    self->cur_loaded = 1;
  }
  // printf("crtc matches: %d\n",crtc_match);
  // printf("crtc linked list length: %d\n",self->crtc_ll_length);
  // printf("is currently loaded? %d\n",self->cur_loaded);



}

static void apply_settings(load_class *self){
  xcb_randr_set_crtc_config_cookie_t crtc_config_cookie;
	xcb_randr_set_crtc_config_reply_t *crtc_config_reply;
  int i;
  set_crtc_param *cur_crtc_param,*old_crtc_param;

  for(i=0;i<self->screen_t_p->screen_resources_reply->num_crtcs;++i){

    //printf("Disabling this crtc: %d\n",crtcs_p[i]);
    crtc_config_cookie =
    xcb_randr_set_crtc_config(self->screen_t_p->c,self->crtcs_p[i],
      XCB_CURRENT_TIME,
      XCB_CURRENT_TIME,
      0,
      0,
      XCB_NONE,XCB_RANDR_ROTATION_ROTATE_0,0,
      NULL);
    // TODO Do I need to fetch the reply for the code to work?
    // crtc_config_reply =
    //   xcb_randr_set_crtc_config_reply(self->screen_t_p->c,crtc_config_cookie,
    //     &self->screen_t_p->e);
    // free(crtc_config_reply);

    //printf("crtc_config_reply_pp: %d\n",crtc_config_reply_pp->status);
    //printf("disable crtcs here\n");

    if (VERBOSE) printf("Would disable crtcs here\n");

	}
  xcb_flush(self->screen_t_p->c);


  xcb_randr_set_screen_size(self->screen_t_p->c,
    self->screen_t_p->screen->root,
    (uint16_t)self->umon_setting_val.screen.width,
    (uint16_t)self->umon_setting_val.screen.height,
    (uint32_t)self->umon_setting_val.screen.widthMM,
    (uint32_t)self->umon_setting_val.screen.heightMM);
  xcb_flush(self->screen_t_p->c);
  //printf("change screen size here\n");

  if (VERBOSE) printf("Would change screen size here\n");


  //TODO implement
  //find_duplicate_crtc(self);


  //printf("enable crtcs here\n");
  // TODO check if the crtc can actually be connected to the output
  i = 0;
  cur_crtc_param=self->crtc_param_head;
  while(cur_crtc_param){

// printf("cur_crtc_param->output_p: %d\n",cur_crtc_param->output_p);
// printf("*(cur_crtc_param->mode_id_p): %d\n",cur_crtc_param->mode_id);
  //printf("Enabling crtc %d\n",cur_crtc_param->crtc);
    //printf("About to load this pos_x: %d\n",cur_crtc_param->pos_x);
     crtc_config_cookie = xcb_randr_set_crtc_config(self->screen_t_p->c,
        cur_crtc_param->crtc,
        XCB_CURRENT_TIME,XCB_CURRENT_TIME,
        cur_crtc_param->pos_x,
        cur_crtc_param->pos_y,
        cur_crtc_param->mode_id,XCB_RANDR_ROTATION_ROTATE_0, 1,
        cur_crtc_param->output_p);
  //if(crtc_config_reply_pp->status==XCB_RANDR_SET_CONFIG_SUCCESS) printf("Enabling crtc should be success\n");

         //printf("Configuring time: %" PRIu32 "\n",crtc_config_reply_pp->timestamp);
    xcb_flush(self->screen_t_p->c);
    //printf("crtc_config_reply_pp: %d\n",crtc_config_reply_pp->response_type);
    old_crtc_param = cur_crtc_param;
    cur_crtc_param = cur_crtc_param->next;
    free(old_crtc_param);
  }
  if (VERBOSE) printf("Would enable crtcs here\n");

  crtc_config_reply =
   xcb_randr_set_crtc_config_reply(self->screen_t_p->c,crtc_config_cookie,
     &self->screen_t_p->e);
  self->last_time = crtc_config_reply->timestamp;
  self->crtc_param_head = NULL;

  free(crtc_config_reply);


}


static void match_with_config(void *self_void,xcb_randr_output_t *output_p){


	xcb_randr_get_output_info_cookie_t output_info_cookie;
  load_class *self = (load_class *) self_void;

	char *edid_string;

  self->cur_output = output_p;
	output_info_cookie =
	xcb_randr_get_output_info(self->screen_t_p->c,*output_p,XCB_CURRENT_TIME);

  free(self->output_info_reply);
	self->output_info_reply =
		xcb_randr_get_output_info_reply (self->screen_t_p->c,output_info_cookie,
      &self->screen_t_p->e);


	fetch_edid(self->cur_output,self->screen_t_p,&edid_string);

	if (VERBOSE) printf("Num out pp: %d\n",self->num_out_pp);

	for(self->conf_output_idx=0;self->conf_output_idx<self->num_out_pp;++self->conf_output_idx){
		if (!strcmp(self->umon_setting_val.outputs[self->conf_output_idx].edid_val,
                edid_string)){
			if (VERBOSE) printf("Before for each output mode\n");
			// Found a match between the configuration file edid and currently connected edid. Now have to load correct settings.
			// Need to find the proper crtc to assign the output
			// Which crtc has the same resolution?

      find_mode_id(self);

		}

	}

  free(edid_string);

}

static void find_mode_id(load_class *self){

  int num_screen_modes,k;

	xcb_randr_mode_info_iterator_t mode_info_iterator;
  set_crtc_param *new_crtc_param;

  mode_info_iterator =
    xcb_randr_get_screen_resources_modes_iterator(
     self->screen_t_p->screen_resources_reply);
	num_screen_modes = xcb_randr_get_screen_resources_modes_length(
		self->screen_t_p->screen_resources_reply);
  if ((self->umon_setting_val.outputs[self->conf_output_idx].res_x != 0) &&
      (self->umon_setting_val.outputs[self->conf_output_idx].res_y != 0)){


    for (k=0;k<num_screen_modes;++k){
  		 if ((mode_info_iterator.data->width ==
         self->umon_setting_val.outputs[self->conf_output_idx].res_x) &&
         (mode_info_iterator.data->height ==
         self->umon_setting_val.outputs[self->conf_output_idx].res_y)){
  			 if (VERBOSE) printf("Found current mode info\n");
  			 //sprintf(res_string,"%dx%d",mode_info_iterator.data->width,mode_info_iterator.data->height);
            new_crtc_param = (set_crtc_param *) malloc(sizeof(set_crtc_param));
            find_available_crtc(self,self->crtc_offset++,
              &(new_crtc_param->crtc));
            if (VERBOSE) {
              printf("Queing up crtc to load: %d\n",new_crtc_param->crtc);
            }
            new_crtc_param->pos_x =
              self->umon_setting_val.outputs[self->conf_output_idx].pos_x;
            //printf("pos_x: %d\n",new_crtc_param->pos_x);
            new_crtc_param->pos_y =
              self->umon_setting_val.outputs[self->conf_output_idx].pos_y;
            new_crtc_param->mode_id = mode_info_iterator.data->id;
            new_crtc_param->output_p = self->cur_output;
            new_crtc_param->next = self->crtc_param_head;
            self->crtc_param_head = new_crtc_param;
            self->crtc_ll_length++;
  			 }
  			xcb_randr_mode_info_next(&mode_info_iterator);
  		 }
  }
     		//if (VERBOSE) printf("Found current mode id\n");
}

static void find_available_crtc(load_class *self,int offset,
  xcb_randr_crtc_t *crtc_p){


  xcb_randr_crtc_t *output_crtcs =
    xcb_randr_get_output_info_crtcs(self->output_info_reply);
  int num_output_crtcs =
   xcb_randr_get_output_info_crtcs_length(self->output_info_reply);

  xcb_randr_crtc_t *available_crtcs = xcb_randr_get_screen_resources_crtcs(
    self->screen_t_p->screen_resources_reply);
  int num_available_crtcs = self->screen_t_p->screen_resources_reply->num_crtcs;

  if (VERBOSE) printf("num_available_crtcs: %d\n",num_available_crtcs);
  if (VERBOSE) printf("num_output_crtcs: %d\n",num_output_crtcs);

  for (int i=0;i<num_available_crtcs;++i){
    for (int j=0;j<num_output_crtcs;++j){
        if (available_crtcs[i] == output_crtcs[j]){
          //if (VERBOSE) printf("offset crtc: %d\n",offset);
          if(!(offset--)){
            *crtc_p = available_crtcs[i];
            return;
          }
        }
      }
  }

  printf("failed to find a matching crtc\n");
  exit(10);

}

static void load_config_val(load_class *self){

	config_setting_t *mon_group,*group,*res_group,*pos_group;
	int i;
	mon_group = config_setting_lookup(self->profile_group,"Monitors");
	// printf("Checking group %d\n",mon_group);
	self->num_out_pp = config_setting_length(mon_group);
  free(self->umon_setting_val.outputs);
  self->umon_setting_val.outputs =
    (umon_setting_output_t *) malloc(self->num_out_pp *
       sizeof(umon_setting_output_t));

	for(i=0;i<self->num_out_pp;++i) {
		group = config_setting_get_elem(mon_group,i);
		// printf("Checking group %d\n",group);
    // Check if output is disabled

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
