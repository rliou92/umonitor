extern void for_each_output_mode(void *self,
				 xcb_randr_get_output_info_reply_t *
				 output_info_reply,
				 void (*callback) (void *,
						   xcb_randr_mode_t *)
    );
extern void for_each_output(void *self,
			    xcb_randr_get_screen_resources_reply_t *
			    screen_resources_reply,
			    void (*callback) (void *, xcb_randr_output_t *)
    );

void load_class_constructor(load_class ** self, screen_class * screen_t);
void load_class_destructor(load_class *);

static void load_profile(load_class * self,
			 config_setting_t * profile_group, int);
static void match_with_config(void *self_void,
			      xcb_randr_output_t * output_p);
static void find_mode_id(load_class * self);
static void load_config_val(load_class *);
static void find_available_crtc(load_class *, xcb_randr_crtc_t *);
static void apply_settings(load_class *);
void umon_print(const char *, ...);

extern void fetch_edid(xcb_randr_output_t *, screen_class *,
		       char **edid_string);

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

typedef struct _crtc_ll {
	xcb_randr_crtc_t crtc;
	struct _crtc_ll *next;
} crtc_ll;

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
