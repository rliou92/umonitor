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
