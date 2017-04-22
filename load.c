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
	//xcb_randr_set_crtc_config_cookie_t *crtc_config_p;
	//xcb_randr_set_crtc_config_reply_t **crtc_config_reply_pp;

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

	for_each_output(
    (void *) self,self->screen_t_p->screen_resources_reply,match_with_config);

	for(int i=0;i<self->screen_t_p->screen_resources_reply->num_crtcs;++i){
    if (!VERBOSE) {
		//crtc_config_p[i] =
    xcb_randr_set_crtc_config_unchecked(self->screen_t_p->c,crtcs_p[i],
      XCB_CURRENT_TIME,
      self->screen_t_p->screen_resources_reply->config_timestamp,0,0,XCB_NONE,
      XCB_RANDR_ROTATION_ROTATE_0,0,0);
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
    xcb_randr_set_screen_size(self->screen_t_p->c,
      self->screen_t_p->screen->root,self->umon_setting_val.screen.width,
      self->umon_setting_val.screen.height,
      self->umon_setting_val.screen.widthMM,
      self->umon_setting_val.screen.heightMM);
	}
  else{
    printf("Would change screen size here\n");
  }

  if (!VERBOSE) {
    for(cur_crtc_param=self->crtc_param_head;cur_crtc_param;cur_crtc_param=cur_crtc_param->next){

      xcb_randr_set_crtc_config_unchecked(self->screen_t_p->c,
        *(cur_crtc_param->crtc_p),
        XCB_CURRENT_TIME,XCB_CURRENT_TIME,
        cur_crtc_param->pos_x,
        cur_crtc_param->pos_y,
        *(cur_crtc_param->mode_id_p),0, 1, cur_crtc_param->output_p);

    }
	}
  else{
    printf("Would enable crtcs here\n");
  }


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
      self->screen_t_p->e);
	output_property_reply = xcb_randr_get_output_property_reply(
		self->screen_t_p->c,output_property_cookie,self->screen_t_p->e);
	output_property_data = xcb_randr_get_output_property_data(
		output_property_reply);
	output_property_length = xcb_randr_get_output_property_data_length(
		output_property_reply);

	edid_to_string(output_property_data,output_property_length,
		&edid_string);

	for(self->conf_output_idx=0;self->conf_output_idx<self->num_out_pp;++self->conf_output_idx){
		if (!strcmp(self->umon_setting_val.outputs[self->conf_output_idx].edid_val,edid_string)){
			// Found a match between the configuration file edid and currently connected edid. Now have to load correct settings.
			// Need to find the proper crtc to assign the output
			// Which crtc has the same resolution?
			for_each_output_mode((void *) self,self->output_info_reply,
        find_crtc_match);
			//find_crtc_by_res(mySett.res_x,mySett.res_y);
			// Connect correct crtc to correct output

			//crtc_config_p = xcb_randr_set_crtc_config(c,crtcs_p[i],XCB_CURRENT_TIME, self->screen_t_p->screen_resources_reply->config_timestamp,0,0,XCB_NONE,  XCB_RANDR_ROTATION_ROTATE_0,NULL,0);
      //if (VERBOSE) printf("I would disable crtcs here\n");



		}

	}



}

static void find_crtc_match(void *self_void,xcb_randr_mode_t *mode_id_p){


	xcb_randr_crtc_t *crtc_p;
  //xcb_randr_mode_t *crtc_mode;
  xcb_randr_get_crtc_info_cookie_t crtc_info_cookie;
  xcb_randr_get_crtc_info_reply_t *crtc_info_reply;
  //xcb_randr_set_crtc_config_cookie_t set_crtc_cookie;
  //xcb_randr_set_crtc_config_reply_t *set_crtc_reply;
	int i,num_crtcs;
  load_class *self = (load_class *) self_void;
  set_crtc_param *new_crtc_param;



	num_crtcs = xcb_randr_get_output_info_crtcs_length(self->output_info_reply);
	crtc_p = xcb_randr_get_output_info_crtcs(self->output_info_reply);


// TODO Can optimize server fetching times
	for(i=0;i<num_crtcs;++i){

    crtc_info_cookie =
     xcb_randr_get_crtc_info(self->screen_t_p->c,*crtc_p,XCB_CURRENT_TIME);
    crtc_info_reply =
      xcb_randr_get_crtc_info_reply(self->screen_t_p->c,crtc_info_cookie,
         self->screen_t_p->e);
    printf("crtc_info_reply->mode: %d\n",crtc_info_reply->mode);
    printf("mode_id_p: %d\n",*mode_id_p);


    if (crtc_info_reply->mode==*mode_id_p){
      // Found the matching crtc
      if (VERBOSE){
        printf("I found the crtc\n");
      }
      else{

      //set_crtc_cookie =
      new_crtc_param = (set_crtc_param *) malloc(sizeof(set_crtc_param));
      new_crtc_param->crtc_p = crtc_p;
      new_crtc_param->pos_x =
        self->umon_setting_val.outputs[self->conf_output_idx].pos_x;
      new_crtc_param->pos_y =
        self->umon_setting_val.outputs[self->conf_output_idx].pos_y;
      new_crtc_param->mode_id_p = mode_id_p;
      new_crtc_param->output_p = self->cur_output;
      new_crtc_param->next = self->crtc_param_head;
      self->crtc_param_head = new_crtc_param;


      // xcb_randr_set_crtc_config_unchecked(self->screen_t_p->c,*crtc_p,
      //   XCB_CURRENT_TIME,XCB_CURRENT_TIME,
      //   self->umon_setting_val.pos_x[self->conf_output_idx],
      //   self->umon_setting_val.pos_y[self->conf_output_idx],
      //   crtc_info_reply->mode,0, 1, self->cur_output);
        printf("I found the crtc\n");
        printf(
          "pos_x: %d\n",self->umon_setting_val.outputs[self->conf_output_idx].pos_x);
      }
    }
		++crtc_p;
	}

}

static void load_config_val(load_class *self){

	// TODO should use an array of structures
	config_setting_t *mon_group,*group,*res_group,*pos_group;
	int i,num_out_pp;
	mon_group = config_setting_lookup(self->profile_group,"Monitors");
	// printf("Checking group %d\n",mon_group);
	num_out_pp = config_setting_length(mon_group);
  self->umon_setting_val.outputs =
    (umon_setting_output_t *) malloc(num_out_pp *
       sizeof(umon_setting_output_t));

	for(i=0;i<num_out_pp;++i) {
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
