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

/*! Contains the connection and screen information
*/
typedef struct _screen_class {
	/*! connection to server */
	xcb_connection_t *c;
	/*! xcb generic error structure */
	xcb_generic_error_t *e;
	/*! default screen number of the connection */
	xcb_screen_t *screen;
	/*! edid atom fetched from the server */
	xcb_intern_atom_reply_t *edid_atom;
	/*! screen resources information */
	xcb_randr_get_screen_resources_reply_t *screen_resources_reply;

	/*! Update screen resources after the screen configuration has changed */
	void (*update_screen) (struct _screen_class *);

} screen_class;

/*! Queue of crtcs to be loaded*/
typedef struct _set_crtc_param {
	xcb_randr_crtc_t crtc;	/*!< crtc to be loaded */
	int pos_x;		/*!< x position of crtc */
	int pos_y;		/*!< y position of crtc */
	int is_primary;		/*!< Whether or not the output associated with this
				   crtc is the primary output */
	xcb_randr_mode_t mode_id;	/*!< mode id of crtc */
	xcb_randr_output_t *output_p;	/*!< to which output the crtc should be
					   connected */
	struct _set_crtc_param *next;	/*!< next crtc param */
	struct _set_crtc_param *prev;	/*!< previous crtc param */

} set_crtc_param;

/*! Screen information that is saved into configuration file
*/
typedef struct {
	int width;		/*!< the width of the screen in pixels */
	int height;		/*!< the height of the screen in pixels */
	int widthMM;		/*!< the width of the screen in mm */
	int heightMM;		/*!< the height of the screen in mm */
} umon_setting_screen_t;

/*! Output information that is saved into configuration file
*/
typedef struct {
	const char *edid_val;	/*!< the edid value as a string */
	int pos_x;		/*!< the output x coordinate */
	int pos_y;		/*!< the output y coordinate */
	int res_x;		/*!< the output x resolution */
	int res_y;		/*!< the output y resolution */
	int crtc_id;		/*!< the output's crtc id */
	int mode_id;		/*!< the output's mode id */
	int is_primary;		/*!< whether or not the output is the primary output */
} umon_setting_output_t;

/*! Settings that are stored for one profile*/
typedef struct {
	umon_setting_screen_t screen;	/*!< the screen settings */
	umon_setting_output_t *outputs;	/*!< the output settings */
} umon_setting_val_t;

/*! libconfig settings that are used to save into the configuration
 file*/
typedef struct {
	config_setting_t *res_group;	/*!< resolution group setting */
	config_setting_t *res_x;	/*!< crtc resolution x dimension */
	config_setting_t *res_y;	/*!< crtc resolution y dimension */
	config_setting_t *pos_x;	/*!< crtc position x coordinate */
	config_setting_t *pos_y;	/*!< crtc position y coordinate */
	config_setting_t *disp_group;	/*!< screen group setting */
	config_setting_t *disp_width;	/*!< screen width setting */
	config_setting_t *disp_height;	/*!< screen height setting */
	config_setting_t *disp_widthMM;	/*!< screen widthmm setting */
	config_setting_t *disp_heightMM;	/*!< screen heightmm setting */
	config_setting_t *mon_group;	/*!< Monitors group setting */
	config_setting_t *output_group;	/*!< output group setting */
	config_setting_t *pos_group;	/*!< position group setting */
	config_setting_t *edid_setting;	/*!< edid setting */
} umon_setting_t;

/*! Class for saving current display settings into the configuration file
*/

typedef struct _save_class {
	// Inheriting classes
	screen_class *screen_t_p;	/*!< Connection information to server */

	// Variables
	umon_setting_t umon_setting;	/*!< Settings container to be saved into file */
	/*! crtc info of current output */
	xcb_randr_get_crtc_info_reply_t *crtc_info_reply;
	/*! output info of current output */
	xcb_randr_get_output_info_reply_t *output_info_reply;
	/*! current output id */
	xcb_randr_output_t *cur_output;
	/*! config handle */
	config_t *config;
	/*! which output is primary */
	xcb_randr_output_t primary_output;

	// Methods
	/*! saves the current display settings into configuration file */
	void (*save_profile) (struct _save_class *, config_setting_t *);
} save_class;


typedef struct _crtc_ll {
	xcb_randr_crtc_t crtc;
	struct _crtc_ll *next;
} crtc_ll;


/*! Class for loading specified profile from configuration file
*/

typedef struct _load_class {
	// Inheriting classes
	screen_class *screen_t_p;	/*!< Connection information to server */

	// Variables
	/*! contains the profile information that is loaded from the conf file */
	umon_setting_val_t umon_setting_val;
	/*! current output info */
	xcb_randr_get_output_info_reply_t *output_info_reply;
	/*! current output id */
	xcb_randr_output_t *cur_output;

	int conf_output_idx;	/*!< current output index */
	int num_out_pp;		/*!< number of outputs per profile */
	int crtc_offset;	/*!< how many available crtcs to skip before returning */
	//int num_conn_outputs;   /*!< how many outputs are currently enabled*/
	int cur_loaded;		/*!< whether or not current profile is loaded */
	set_crtc_param *crtc_param_head;	/*!< head of crtc queue */
	config_setting_t *profile_group;	/*!< which profile to be loaded */
	xcb_timestamp_t last_time;	/*!< last time the crtc setting was changed */
	xcb_randr_crtc_t *crtcs_p;	/*!< crtcs associated with the screen */
	crtc_ll *disable_crtc_head, *assigned_crtc_head;	/*!< disable crtcs linked list */

	// Methods
	/*! loads specified profile from configuration file */
	void (*load_profile) (struct _load_class *, config_setting_t *,
			      int);
} load_class;

/*! Class for determining which profile to load based on the current
display settings */
