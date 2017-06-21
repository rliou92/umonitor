#include "common.h"
#include "umonitor.h"
#include "save.h"

#define PVAR ((save_pvar *) (self->pvar))

/*! \file
		\brief Save current settings into the configuration file

*/

/*! save constructor*/

static void save_profile(save_class *, config_setting_t *);
static void output_info_to_config(save_class *);
static void find_res_to_config(void *, xcb_randr_mode_t *);
static void disabled_to_config(save_class *);
static void check_output_status(void *, xcb_randr_output_t *);
static void get_output_name(xcb_randr_get_output_info_reply_t *, char **);
static void check_for_duplicate_crtc(save_class * self);


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

typedef struct {
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

} save_pvar;

void save_class_constructor(save_class ** self_p, screen_class * screen_t,
			    config_t * config)
{
	save_class *self;

	self = (save_class *) umalloc(sizeof(save_class));
	self->pvar = (void *) umalloc(sizeof(save_pvar));
	self->save_profile = save_profile;

	PVAR->screen_t_p = screen_t;
	PVAR->config = config;

	*self_p = self;
	umon_print("Done constructing save class\n");

}

/*! save deconstructor*/

void save_class_destructor(save_class * self)
{
	free(PVAR);
	free(self);
}

/*! \brief Save function main
 */

static void save_profile(save_class * self,
			 config_setting_t * profile_group)
{

	PVAR->umon_setting.disp_group =
	    config_setting_add(profile_group, "Screen", CONFIG_TYPE_GROUP);
	PVAR->umon_setting.disp_width =
	    config_setting_add(PVAR->umon_setting.disp_group, "width",
			       CONFIG_TYPE_INT);
	PVAR->umon_setting.disp_height =
	    config_setting_add(PVAR->umon_setting.disp_group, "height",
			       CONFIG_TYPE_INT);
	PVAR->umon_setting.disp_widthMM =
	    config_setting_add(PVAR->umon_setting.disp_group, "widthMM",
			       CONFIG_TYPE_INT);
	PVAR->umon_setting.disp_heightMM =
	    config_setting_add(PVAR->umon_setting.disp_group, "heightMM",
			       CONFIG_TYPE_INT);

	//if (VERBOSE) printf("Screen width in pixels: %d\n",PVAR->screen_t_p->screen->width_in_pixels);
	config_setting_set_int(PVAR->umon_setting.disp_width,
			       PVAR->screen_t_p->screen->width_in_pixels);
	//if (VERBOSE) printf("Screen height in pixels: %d\n",PVAR->screen_t_p->screen->height_in_pixels);
	config_setting_set_int(PVAR->umon_setting.disp_height,
			       PVAR->screen_t_p->screen->height_in_pixels);
	//printf("Screen width in millimeters: %d\n",PVAR->screen_t_p->screen->width_in_millimeters);
	config_setting_set_int(PVAR->umon_setting.disp_widthMM,
			       PVAR->screen_t_p->screen->
			       width_in_millimeters);
	config_setting_set_int(PVAR->umon_setting.disp_heightMM,
			       PVAR->screen_t_p->screen->
			       height_in_millimeters);

	PVAR->umon_setting.mon_group =
	    config_setting_add(profile_group, "Monitors",
			       CONFIG_TYPE_GROUP);

	// Get primary output
	xcb_randr_get_output_primary_cookie_t output_primary_cookie =
	    xcb_randr_get_output_primary(PVAR->screen_t_p->c,
					 PVAR->screen_t_p->screen->root);
	xcb_randr_get_output_primary_reply_t *output_primary_reply =
	    xcb_randr_get_output_primary_reply(PVAR->screen_t_p->c,
					       output_primary_cookie,
					       &PVAR->screen_t_p->e);
	PVAR->primary_output = output_primary_reply->output;
	umon_print("Primary output %d\n", PVAR->primary_output);
	free(output_primary_reply);

	for_each_output((void *) self,
			PVAR->screen_t_p->screen_resources_reply,
			check_output_status);

	//if (VERBOSE) printf("Made it here \n");
	//if (VERBOSE) printf("About to write to config file %s\n",CONFIG_FILE);
	config_write_file(PVAR->config, CONFIG_FILE);

	umon_print("Done saving settings to profile\n");

}

/*! \brief For each output determine whether output is connected or enabled
 */

static void check_output_status(void *self_void,
				xcb_randr_output_t * output_p)
{

	xcb_randr_get_output_info_cookie_t output_info_cookie;
	char *edid_string, *output_name;
	config_setting_t *primary_output_setting;

	save_class *self = (save_class *) self_void;
	PVAR->cur_output = output_p;

	output_info_cookie =
	    xcb_randr_get_output_info(PVAR->screen_t_p->c, *output_p,
				      XCB_CURRENT_TIME);
	PVAR->output_info_reply =
	    xcb_randr_get_output_info_reply(PVAR->screen_t_p->c,
					    output_info_cookie,
					    &PVAR->screen_t_p->e);



	if (PVAR->output_info_reply->connection) {
		free(PVAR->output_info_reply);
		return;
	}

	umon_print("Found output that is connected\n");

	get_output_name(PVAR->output_info_reply, &output_name);
	PVAR->umon_setting.output_group =
	    config_setting_add(PVAR->umon_setting.mon_group,
			       output_name, CONFIG_TYPE_GROUP);
	free(output_name);

	PVAR->umon_setting.edid_setting =
	    config_setting_add(PVAR->umon_setting.output_group,
			       "EDID", CONFIG_TYPE_STRING);
	PVAR->umon_setting.res_group =
	    config_setting_add(PVAR->umon_setting.output_group,
			       "resolution", CONFIG_TYPE_GROUP);
	PVAR->umon_setting.res_x =
	    config_setting_add(PVAR->umon_setting.res_group, "x",
			       CONFIG_TYPE_INT);
	PVAR->umon_setting.res_y =
	    config_setting_add(PVAR->umon_setting.res_group, "y",
			       CONFIG_TYPE_INT);
	PVAR->umon_setting.pos_group =
	    config_setting_add(PVAR->umon_setting.output_group,
			       "pos", CONFIG_TYPE_GROUP);
	PVAR->umon_setting.pos_x =
	    config_setting_add(PVAR->umon_setting.pos_group, "x",
			       CONFIG_TYPE_INT);
	PVAR->umon_setting.pos_y =
	    config_setting_add(PVAR->umon_setting.pos_group, "y",
			       CONFIG_TYPE_INT);

	fetch_edid(PVAR->cur_output, PVAR->screen_t_p, &edid_string);
	config_setting_set_string(PVAR->umon_setting.edid_setting,
				  edid_string);
	free(edid_string);

	if (PVAR->output_info_reply->crtc) {
		umon_print("Found output that is enabled :%d\n",
			   *output_p);
		if (*output_p == PVAR->primary_output) {
			umon_print("Found primary output\n");
			primary_output_setting =
			    config_setting_add(PVAR->umon_setting.
					       output_group, "primary",
					       CONFIG_TYPE_INT);
			config_setting_set_int(primary_output_setting, 1);

		}
		output_info_to_config(self);
	} else {
		disabled_to_config(self);
	}

	free(PVAR->output_info_reply);

}

/*! \brief Convert the raw output name obtained from server into char
 */
static void get_output_name(xcb_randr_get_output_info_reply_t *
			    output_info_reply, char **output_name)
{
	int i;
	uint8_t *output_name_raw =
	    xcb_randr_get_output_info_name(output_info_reply);
	int output_name_length =
	    xcb_randr_get_output_info_name_length(output_info_reply);
	*output_name =
	    (char *) umalloc((output_name_length + 1) * sizeof(char));

	for (i = 0; i < output_name_length; ++i) {
		(*output_name)[i] = (char) output_name_raw[i];
	}
	(*output_name)[i] = '\0';

}

/*! \brief Save the crtc x,y and res x,y into config
 */
static void output_info_to_config(save_class * self)
{

	xcb_randr_get_crtc_info_cookie_t crtc_info_cookie;

	crtc_info_cookie =
	    xcb_randr_get_crtc_info(PVAR->screen_t_p->c,
				    PVAR->output_info_reply->crtc,
				    PVAR->screen_t_p->
				    screen_resources_reply->
				    config_timestamp);
	PVAR->crtc_info_reply =
	    xcb_randr_get_crtc_info_reply(PVAR->screen_t_p->c,
					  crtc_info_cookie,
					  &PVAR->screen_t_p->e);

	check_for_duplicate_crtc(self);

	config_setting_set_int(PVAR->umon_setting.pos_x,
			       PVAR->crtc_info_reply->x);
	config_setting_set_int(PVAR->umon_setting.pos_y,
			       PVAR->crtc_info_reply->y);

	for_each_output_mode((void *) self, PVAR->output_info_reply,
			     find_res_to_config);

	free(PVAR->crtc_info_reply);
}

static void check_for_duplicate_crtc(save_class * self)
{
	int i, outputs_length;
	xcb_randr_output_t *dup_outputs;
	config_setting_t *dup_outputs_setting;
	xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;
	char *output_name;

	outputs_length =
	    xcb_randr_get_crtc_info_outputs_length(PVAR->crtc_info_reply);
	if (outputs_length <= 1)
		return;

	umon_print("Duplicate output!\n");
	dup_outputs =
	    xcb_randr_get_crtc_info_outputs(PVAR->crtc_info_reply);
	dup_outputs_setting =
	    config_setting_add(PVAR->umon_setting.mon_group,
			       "duplicates", CONFIG_TYPE_LIST);
	// Record which outputs are duplicate
	for (i = 0; i < outputs_length; ++i) {
		output_info_cookie =
		    xcb_randr_get_output_info(PVAR->screen_t_p->c,
					      dup_outputs[i],
					      XCB_CURRENT_TIME);
		output_info_reply =
		    xcb_randr_get_output_info_reply
		    (PVAR->screen_t_p->c, output_info_cookie,
		     &PVAR->screen_t_p->e);

		get_output_name(output_info_reply, &output_name);
		config_setting_set_string_elem(dup_outputs_setting,
					       i, output_name);
		free(output_name);
		free(output_info_reply);
	}

}


/*! \brief Translate the mode id into resolution x,y
 */
static void find_res_to_config(void *self_void,
			       xcb_randr_mode_t * mode_id_p)
{

	int num_screen_modes, k;

	xcb_randr_mode_info_iterator_t mode_info_iterator;
	save_class *self = (save_class *) self_void;
	//if (VERBOSE) printf("current mode id %d\n",*mode_id_p);
	if (*mode_id_p != PVAR->crtc_info_reply->mode)
		return;
	//if (VERBOSE) printf("Found current mode id\n");
	// Get output info iterator
	mode_info_iterator =
	    xcb_randr_get_screen_resources_modes_iterator
	    (PVAR->screen_t_p->screen_resources_reply);
	num_screen_modes =
	    xcb_randr_get_screen_resources_modes_length
	    (PVAR->screen_t_p->screen_resources_reply);
	for (k = 0; k < num_screen_modes; ++k) {
		if (mode_info_iterator.data->id == *mode_id_p) {
			//if (VERBOSE) printf("Found current mode info\n");
			//sprintf(res_string,"%dx%d",mode_info_iterator.data->width,mode_info_iterator.data->height);
			config_setting_set_int(PVAR->umon_setting.res_x,
					       mode_info_iterator.
					       data->width);
			config_setting_set_int(PVAR->umon_setting.res_y,
					       mode_info_iterator.
					       data->height);
		}
		xcb_randr_mode_info_next(&mode_info_iterator);

	}

}

/*! \brief Just save 0 for all settings if output is connected but disabled
 */
static void disabled_to_config(save_class * self)
{

	umon_print("Found output that is disabled\n");

	config_setting_set_int(PVAR->umon_setting.pos_x, 0);
	config_setting_set_int(PVAR->umon_setting.pos_y, 0);
	config_setting_set_int(PVAR->umon_setting.res_x, 0);
	config_setting_set_int(PVAR->umon_setting.res_y, 0);

}
