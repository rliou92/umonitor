// TODO some of these includes are not needed

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <libconfig.h>
#include <unistd.h>
#include <X11/extensions/Xrandr.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <stdarg.h>


typedef struct _screen_class{
  xcb_connection_t *c;
  xcb_generic_error_t *e;
  xcb_screen_t *screen;
  xcb_intern_atom_reply_t *edid_atom;
  xcb_randr_get_screen_resources_reply_t *screen_resources_reply;

  void (*update_screen)(struct _screen_class*);

}screen_class;

typedef struct _set_crtc_param{
  xcb_randr_crtc_t crtc;
  int pos_x,pos_y;
  xcb_randr_mode_t mode_id;
  xcb_randr_output_t *output_p;
  struct _set_crtc_param *next;

}set_crtc_param;

/*
 * Structures for loading and saving the configuration file
 */
typedef struct {
	int width,height,widthMM,heightMM;
}umon_setting_screen_t;


typedef struct {
	const char *edid_val;
	int pos_x,pos_y,res_x,res_y,crtc_id,mode_id;
}umon_setting_output_t;

typedef struct {
	umon_setting_screen_t screen;
  umon_setting_output_t *outputs;
}umon_setting_val_t;



typedef struct {
	config_setting_t *edid,*res_group,*res_x,*res_y,*pos_x,
		*pos_y,*disp_group,*disp_width,*disp_height,
		*disp_widthMM,*disp_heightMM,*mon_group,*output_group,
		*pos_group,*status,*edid_setting,*crtc_id,*mode_id;
}umon_setting_t;

typedef struct _save_class{
  // Inheriting classes
  screen_class *screen_t_p;

  // Variables
  umon_setting_t umon_setting;
  xcb_randr_get_crtc_info_reply_t *crtc_info_reply;
  xcb_randr_get_output_info_reply_t *output_info_reply;
  xcb_randr_output_t *cur_output;
  config_t *config;

  // Methods
  void (*save_profile)(struct _save_class *,config_setting_t *);
}save_class;

typedef struct _load_class{
  // Inheriting classes
  screen_class *screen_t_p;

  // Variables
  umon_setting_val_t umon_setting_val;
  xcb_randr_get_output_info_reply_t *output_info_reply;
  xcb_randr_output_t *cur_output;
  int conf_output_idx,num_out_pp,crtc_offset;
  set_crtc_param *crtc_param_head;
  config_setting_t *profile_group;
  xcb_timestamp_t last_time;

  // Methods
  void (*load_profile)(struct _load_class *,config_setting_t *);
}load_class;

typedef struct _autoload_class{
  // Inheriting classes
  screen_class *screen_t_p;
  load_class load_o;

  // Variables
  config_t *config;
  config_setting_t *mon_group;
  int output_match,num_out_pp,num_conn_outputs;

  // Methods
  void (*find_profile_and_load)(struct _autoload_class *);
  void (*wait_for_event)(struct _autoload_class *);


}autoload_class;
