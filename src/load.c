#include "common.h"
#include "umonitor.h"
#include "load.h"

#define PVAR ((load_pvar *) (self->pvar))

/*! Queue of crtcs to be loaded*/
typedef struct _set_crtc_param {
	xcb_randr_crtc_t crtc;	/*!< crtc to be loaded */
	int pos_x;		/*!< x position of crtc */
	int pos_y;		/*!< y position of crtc */
	int is_primary;		/*!< Whether or not the output associated with this
				   crtc is the primary output */
	int rotation;			/*!< crtc rotation */
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

typedef struct _output_reply_ll {
	xcb_randr_get_output_info_reply_t *output_info_reply;
	xcb_randr_output_t * output_p;
	struct _output_reply_ll *next;
} output_reply_ll;

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
	const char *output_name;
	const char *edid_val;	/*!< the edid value as a string */
	int pos_x;		/*!< the output x coordinate */
	int pos_y;		/*!< the output y coordinate */
	int res_x;		/*!< the output x resolution */
	int res_y;		/*!< the output y resolution */
	int crtc_id;		/*!< the output's crtc id */
	int mode_id;		/*!< the output's mode id */
	int is_primary;		/*!< whether or not the output is the primary output */
	int rotation;		/*!< the output's rotation */
} umon_setting_output_t;

/*! Settings that are stored for one profile*/
typedef struct {
	umon_setting_screen_t screen;	/*!< the screen settings */
	umon_setting_output_t *outputs;	/*!< the output settings */
} umon_setting_val_t;

struct modify_crtc_ll_param_t {
	int *i_p;
	xcb_randr_crtc_t *crtcs_p;	/*!< crtcs associated with the screen */
};

struct add_disable_crtc_param_t {
	int *i_p;
	xcb_randr_crtc_t *crtcs_p;	/*!< crtcs associated with the screen */
};

struct remove_matching_crtc_from_ll_param_t {
	int *should_disable_p;
	set_crtc_param **cur_crtc_param_p;
	xcb_randr_get_crtc_info_reply_t *crtc_info_reply;
	xcb_randr_output_t *conn_output;
	xcb_randr_crtc_t crtc;
};

struct find_mode_id_param_t {
	int *conf_output_idx_p;
	xcb_randr_get_output_info_reply_t *output_info_reply;
	xcb_randr_output_t *cur_output;
};

struct determine_mode_id_match_param_t {
	int *j_p, conf_output_idx, mode_found;
	xcb_randr_mode_info_iterator_t *mode_info_iterator_p;
	xcb_randr_mode_t *output_modes;
	xcb_randr_output_t *cur_output;
	xcb_randr_get_output_info_reply_t *output_info_reply;
};

struct add_crtc_param_t {
	int conf_output_idx;
	xcb_randr_output_t *cur_output;
	xcb_randr_mode_info_iterator_t *mode_info_iterator_p;
	xcb_randr_get_output_info_reply_t *output_info_reply;
};

struct find_available_crtc_param_t {
	xcb_randr_get_output_info_reply_t *output_info_reply;
};

struct determine_crtc_match_param_t {
	int num_crtcs;
	xcb_randr_crtc_t *crtc_p, *output_crtcs;
};

static void set_force_load(load_class * self, int force_load);
static void get_cur_loaded(load_class * self, int *cur_loaded);
static void get_last_time(load_class * self, xcb_timestamp_t * last_time);
static void free_crtc_param_ll(set_crtc_param * head);
static void free_crtc_ll(crtc_ll * crtc_ll_head);
static void free_output_reply_ll(output_reply_ll * reply_ll_head);
static void load_profile(load_class * self,
			 config_setting_t * profile_group, int);
static void load_config_val(load_class * self,
			    config_setting_t * profile_group);
static void construct_output_reply_ll(void *self_void,
				      xcb_randr_output_t *output_p);
static void match_with_config(load_class *self,
	output_reply_ll *);
static void find_mode_id(load_class * self,
			 struct find_mode_id_param_t *param);
static void determine_mode_id_match(load_class * self, struct determine_mode_id_match_param_t
				    *param);
static void add_crtc_param(load_class * self,
			   struct add_crtc_param_t *param);
static void find_available_crtc(load_class * self,
				struct find_available_crtc_param_t *param,
				xcb_randr_crtc_t * crtc_p);
static void determine_crtc_match(load_class * self, struct determine_crtc_match_param_t
				 *param);
static void modify_crtc_ll(load_class * self,
			   struct modify_crtc_ll_param_t *param);
static void remove_matching_crtc_from_ll(load_class * self, struct remove_matching_crtc_from_ll_param_t
					 *param);
static void add_disable_crtc(load_class * self,
			     struct add_disable_crtc_param_t *param);
static void apply_settings(load_class * self);
static void apply_settings_disable_crtcs(load_class * self);
static void apply_settings_screen_size(load_class * self);
static void apply_settings_enable_crtcs(load_class * self);
static void load_config_val(load_class * self,
			    config_setting_t * profile_group);

/*! \file
    \brief Load a profile and apply the changes to the current
configuration (if needed)

*/

typedef struct {
	screen_class *screen_o;	/*!< Connection information to server */

	// Variables
	/*! contains the profile information that is loaded from the conf file */
	umon_setting_val_t umon_setting_val;
	set_crtc_param *crtc_param_head;
	crtc_ll *assigned_crtc_head;
	crtc_ll *disable_crtc_head;
	output_reply_ll *output_reply_head;

	int cur_loaded;
	int num_out_pp;
	xcb_timestamp_t last_time;	/*!< last time the crtc setting was changed */
	int force_load;
} load_pvar;

/*! load constructor*/

void load_class_constructor(load_class ** self_p, screen_class * screen_o)
{
	load_class *self;

	self = (load_class *) umalloc(sizeof(load_class));
	self->pvar = (void *) umalloc(sizeof(load_pvar));

	self->load_profile = load_profile;
	self->get_last_time = get_last_time;
	self->get_cur_loaded = get_cur_loaded;
	self->set_force_load = set_force_load;

	PVAR->screen_o = screen_o;
	PVAR->crtc_param_head = NULL;
	PVAR->umon_setting_val.outputs = NULL;
	PVAR->last_time = (xcb_timestamp_t) 0;
	PVAR->cur_loaded = 0;
	PVAR->assigned_crtc_head = NULL;
	PVAR->force_load = 0;
	PVAR->output_reply_head = NULL;

	*self_p = self;

}

/*! load deconstructor*/

void load_class_destructor(load_class * self)
{

	free(PVAR->umon_setting_val.outputs);
	free(PVAR);
	free(self);

}

static void set_force_load(load_class * self, int force_load)
{
	PVAR->force_load = force_load;
}

static void get_cur_loaded(load_class * self, int *cur_loaded)
{
	*cur_loaded = PVAR->cur_loaded;
}

static void get_last_time(load_class * self, xcb_timestamp_t * last_time)
{
	*last_time = PVAR->last_time;
}

static void free_crtc_param_ll(set_crtc_param * head)
{
	set_crtc_param *cur_free_crtc, *old_free_crtc;
	cur_free_crtc = head;
	while (cur_free_crtc) {
		old_free_crtc = cur_free_crtc;
		cur_free_crtc = cur_free_crtc->next;
		free(old_free_crtc);
	}
}

static void free_crtc_ll(crtc_ll * crtc_ll_head)
{
	crtc_ll *cur_free_crtc, *old_free_crtc;
	cur_free_crtc = crtc_ll_head;
	while (cur_free_crtc) {
		old_free_crtc = cur_free_crtc;
		cur_free_crtc = cur_free_crtc->next;
		free(old_free_crtc);
	}
}

static void free_output_reply_ll(output_reply_ll * reply_ll_head)
{
	output_reply_ll *cur_free_reply, *old_free_reply;
	cur_free_reply = reply_ll_head;
	while (cur_free_reply) {
		old_free_reply = cur_free_reply;
		cur_free_reply = cur_free_reply->next;
		free(old_free_reply->output_info_reply);
		free(old_free_reply);
	}
}


/*! \brief Load the specified profile
 */

static void load_profile(load_class * self,
			 config_setting_t * profile_group, int test_cur)
{
	int i;
	struct modify_crtc_ll_param_t modify_crtc_ll_param;
	xcb_randr_crtc_t *crtcs_p;
	char *profile_name;
	output_reply_ll *cur_output_reply;
	crtc_ll *new_assigned_crtc;

	PVAR->disable_crtc_head = NULL;
	load_config_val(self, profile_group);
	profile_name = config_setting_name(profile_group);
	umon_print("Loading profile %s\n", profile_name);
	umon_print("Num outputs per profile %s: %d\n", profile_name,
		   PVAR->num_out_pp);

	// Construct output reply linked list
	for_each_output((void *) self,
			PVAR->screen_o->screen_resources_reply,
			construct_output_reply_ll);

	// Find the crtcs that are being used
	cur_output_reply = PVAR->output_reply_head;
	while (cur_output_reply) {
		if (cur_output_reply->output_info_reply->crtc != 0) {
			new_assigned_crtc = (crtc_ll *)
			    umalloc(sizeof(crtc_ll));
			new_assigned_crtc->crtc = cur_output_reply->output_info_reply->crtc;
			new_assigned_crtc->next = PVAR->assigned_crtc_head;
			PVAR->assigned_crtc_head = new_assigned_crtc;
		}
		cur_output_reply = cur_output_reply->next;
	}

	// Queue up crtc linked list to load
	cur_output_reply = PVAR->output_reply_head;
	while (cur_output_reply) {
		match_with_config(self, cur_output_reply);
		cur_output_reply = cur_output_reply->next;
	}
	// Check against redundant loading
	//TODO Only assuming 1 output connected to crtc
	// Examine current configuration state to determine which crtcs should actually be loaded and which should be disabled
	crtcs_p =
	    xcb_randr_get_screen_resources_crtcs(PVAR->
						 screen_o->screen_resources_reply);

	modify_crtc_ll_param.i_p = &i;
	modify_crtc_ll_param.crtcs_p = crtcs_p;
	for (i = 0;
	     i < PVAR->screen_o->screen_resources_reply->num_crtcs; ++i) {
		modify_crtc_ll(self, &modify_crtc_ll_param);
	}
	if ((PVAR->crtc_param_head == NULL)
	    && (PVAR->disable_crtc_head == NULL)) {
		umon_print("No crtcs to disable or enable. Profile %s is currently active\n", profile_name);
		PVAR->cur_loaded = 1;
	} else {
		//printf("Am I here?\n");
		if (!test_cur) {
			apply_settings(self);
		} else {
			free_crtc_param_ll(PVAR->crtc_param_head);
			free_crtc_ll(PVAR->disable_crtc_head);
		}
		PVAR->cur_loaded = 0;
	}

	free_crtc_ll(PVAR->assigned_crtc_head);
	free_output_reply_ll(PVAR->output_reply_head);

	PVAR->crtc_param_head = NULL;
	PVAR->disable_crtc_head = NULL;
	PVAR->assigned_crtc_head = NULL;
	PVAR->output_reply_head = NULL;
}

static void construct_output_reply_ll(void *self_void,
			      xcb_randr_output_t * output_p)
{
	xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;
	output_reply_ll *new_output_reply;

	load_class *self = (load_class *) self_void;

	output_info_cookie =
	    xcb_randr_get_output_info(PVAR->screen_o->c, *output_p,
				      XCB_CURRENT_TIME);

	output_info_reply =
	    xcb_randr_get_output_info_reply(PVAR->screen_o->c,
					    output_info_cookie,
					    &PVAR->screen_o->e);

	if (output_info_reply->connection) {
		free(output_info_reply);
		return;
	}

	new_output_reply = (output_reply_ll *) umalloc(sizeof(output_reply_ll));
	new_output_reply->output_info_reply = output_info_reply;
	new_output_reply->output_p = output_p;
	new_output_reply->next = PVAR->output_reply_head;
	PVAR->output_reply_head = new_output_reply;


}


static void match_with_config(load_class *self,
	output_reply_ll *cur_output_reply)
{

	int conf_output_idx;
	struct find_mode_id_param_t mode_id_param;
	char *output_name;
	char *edid_string;

	get_output_name(cur_output_reply->output_info_reply, &output_name);
	fetch_edid(cur_output_reply->output_p, PVAR->screen_o, &edid_string);
	umon_print
	    ("Trying to find matching setting for %s (%s)\n",
	     output_name, edid_string);

	mode_id_param.conf_output_idx_p = &conf_output_idx;
	mode_id_param.output_info_reply = cur_output_reply->output_info_reply;
	mode_id_param.cur_output = cur_output_reply->output_p;
	for (conf_output_idx = 0; conf_output_idx < PVAR->num_out_pp;
	     ++conf_output_idx) {
		// umon_print("")
		if (!strcmp
		    (PVAR->umon_setting_val.outputs[conf_output_idx].
		     edid_val, edid_string)
		    && !strcmp(PVAR->umon_setting_val.
			       outputs[conf_output_idx].output_name,
			       output_name)) {
			umon_print
			    ("Found output in configuration file that matches %s (%s)\n",
			     output_name, edid_string);
			umon_print
			    ("Trying match up the configuration resolution with the mode id\n");
			find_mode_id(self, &mode_id_param);
		}
	}
	free(edid_string);
	free(output_name);
}

/*! \brief After matching edid has been found, find the matching mode id from
the configuration's resolution x and y values

  Queues up the crtc to be loaded in apply_settings
 */

static void find_mode_id(load_class * self,
			 struct find_mode_id_param_t *param)
{
	int num_screen_modes, num_output_modes, i, j, conf_output_idx;
	xcb_randr_mode_info_iterator_t mode_info_iterator;
	struct determine_mode_id_match_param_t id_match_param;
	xcb_randr_mode_t *output_modes;

	conf_output_idx = *(param->conf_output_idx_p);
	mode_info_iterator =
	    xcb_randr_get_screen_resources_modes_iterator
	    (PVAR->screen_o->screen_resources_reply);
	num_screen_modes =
	    xcb_randr_get_screen_resources_modes_length(PVAR->
							screen_o->screen_resources_reply);

	output_modes =
	    xcb_randr_get_output_info_modes(param->output_info_reply);
	num_output_modes =
	    xcb_randr_get_output_info_modes_length
	    (param->output_info_reply);

	if ((PVAR->umon_setting_val.outputs[conf_output_idx].res_x == 0)
	    || (PVAR->umon_setting_val.outputs[conf_output_idx].res_y ==
		0)) {
		umon_print
		    ("No need to find mode id, just let this output be disabled later\n");
		return;
	}

	id_match_param.mode_found = 0;
	id_match_param.conf_output_idx = conf_output_idx;
	id_match_param.j_p = &j;
	id_match_param.output_modes = output_modes;
	id_match_param.cur_output = param->cur_output;
	id_match_param.output_info_reply = param->output_info_reply;
	id_match_param.mode_info_iterator_p = &mode_info_iterator;
	for (i = 0; i < num_screen_modes; ++i) {
		id_match_param.mode_info_iterator_p = &mode_info_iterator;
		for (j = 0; j < num_output_modes; ++j) {
			determine_mode_id_match(self, &id_match_param);
		}
		if (id_match_param.mode_found == 1)
			return;
		xcb_randr_mode_info_next(&mode_info_iterator);
	}


	//if (VERBOSE) printf("Found current mode id\n");
}

/*! \brief For each connected output, find the configuration monitor edid that
matches connected monitor edid */

static void determine_mode_id_match(load_class * self, struct determine_mode_id_match_param_t
				    *param)
{
	int j;
	xcb_randr_mode_info_iterator_t mode_info_iterator;
	struct add_crtc_param_t crtc_parameters;

	j = *(param->j_p);
	mode_info_iterator = *(param->mode_info_iterator_p);
	if ((mode_info_iterator.data->width !=
	     PVAR->umon_setting_val.outputs[param->conf_output_idx].res_x)
	    || (mode_info_iterator.data->height !=
		PVAR->umon_setting_val.outputs[param->
					       conf_output_idx].res_y)
	    || (mode_info_iterator.data->id != param->output_modes[j]))
		return;

	umon_print("Found matching mode id: %d\n",
		   mode_info_iterator.data->id);
	//sprintf(res_string,"%dx%d",mode_info_iterator.data->width,mode_info_iterator.data->height);
	param->mode_found = 1;

	crtc_parameters.conf_output_idx = param->conf_output_idx;
	crtc_parameters.mode_info_iterator_p = param->mode_info_iterator_p;
	crtc_parameters.cur_output = param->cur_output;
	crtc_parameters.output_info_reply = param->output_info_reply;
	add_crtc_param(self, &crtc_parameters);


}

static void add_crtc_param(load_class * self,
			   struct add_crtc_param_t *param)
{
	set_crtc_param *new_crtc_param;
	struct find_available_crtc_param_t available_crtc_param;
	xcb_randr_mode_info_iterator_t mode_info_iterator;
	char *output_name;

	mode_info_iterator = *(param->mode_info_iterator_p);
	new_crtc_param = (set_crtc_param *)
	    umalloc(sizeof(set_crtc_param));
	get_output_name(param->output_info_reply, &output_name);

	available_crtc_param.output_info_reply = param->output_info_reply;
	find_available_crtc(self, &available_crtc_param,
			    &(new_crtc_param->crtc));
	umon_print("Queing up crtc to load: %d\n", new_crtc_param->crtc);
	umon_print
	    ("Crtc settings: x:%d, y:%d, rotation: %d, is_primary: %d, mode_id: %d, output: %s\n",
	     PVAR->umon_setting_val.outputs[param->conf_output_idx].pos_x,
	     PVAR->umon_setting_val.outputs[param->conf_output_idx].pos_y,
	     PVAR->umon_setting_val.outputs[param->conf_output_idx].rotation,
	     PVAR->umon_setting_val.outputs[param->conf_output_idx].
	     is_primary, mode_info_iterator.data->id, output_name);

	free(output_name);
	new_crtc_param->pos_x =
	    PVAR->umon_setting_val.outputs[param->conf_output_idx].pos_x;
	new_crtc_param->pos_y =
	    PVAR->umon_setting_val.outputs[param->conf_output_idx].pos_y;
	new_crtc_param->is_primary =
	    PVAR->umon_setting_val.outputs[param->
					   conf_output_idx].is_primary;
	new_crtc_param->rotation = PVAR->umon_setting_val.outputs[param->
				       conf_output_idx].rotation;
	new_crtc_param->mode_id = mode_info_iterator.data->id;
	new_crtc_param->output_p = param->cur_output;
	new_crtc_param->next = PVAR->crtc_param_head;

	new_crtc_param->prev = NULL;
	//printf("did I make it here\n");

	PVAR->crtc_param_head = new_crtc_param;
	//printf("PVAR->crtc_param_head->next: %d\n",PVAR->crtc_param_head->next);
	if (PVAR->crtc_param_head->next)
		PVAR->crtc_param_head->next->prev = new_crtc_param;

}

/*! \brief Find an available crtc for the current output
 */

static void find_available_crtc(load_class * self,
				struct find_available_crtc_param_t *param,
				xcb_randr_crtc_t * crtc_p)
{
	struct determine_crtc_match_param_t crtc_match_param;
	xcb_randr_crtc_t *output_crtcs;

	// already_assigned = 1;
	// available_crtcs =
	//     xcb_randr_get_screen_resources_crtcs(PVAR->
	// 					 screen_o->screen_resources_reply);

	output_crtcs =
	    xcb_randr_get_output_info_crtcs(param->output_info_reply);

	umon_print("Candidate crtc for this output: %d\n", param->output_info_reply->crtc);

	if (param->output_info_reply->crtc != 0) {
		*(crtc_p) = param->output_info_reply->crtc;
		return;
	}

	crtc_match_param.crtc_p = crtc_p;
	crtc_match_param.num_crtcs = param->output_info_reply->num_crtcs;
	crtc_match_param.output_crtcs = output_crtcs;

	determine_crtc_match(self, &crtc_match_param);
	umon_print("Candidate crtc will not work, using %d instead\n", *crtc_p);


	// num_output_crtcs =
	//     xcb_randr_get_output_info_crtcs_length
	//     (param->output_info_reply);
	//
	// num_available_crtcs =
	//     PVAR->screen_o->screen_resources_reply->num_crtcs;
	//
	// crtc_match_param.crtc_p = crtc_p;
	// crtc_match_param.i_p = &i;
	// crtc_match_param.j_p = &j;
	// crtc_match_param.already_assigned_p = &already_assigned;
	// crtc_match_param.available_crtcs = available_crtcs;
	// crtc_match_param.output_crtcs = output_crtcs;
	// for (i = 0; i < num_available_crtcs; ++i) {
	// 	for (j = 0; j < num_output_crtcs; ++j) {
	// 		determine_crtc_match(self, &crtc_match_param);
	// 		if (!already_assigned) {
	// 			return;
	// 		}
	// 	}
	// }
	//

}

static void determine_crtc_match(load_class * self, struct determine_crtc_match_param_t
				 *param)
{
	int i, already_assigned;
	crtc_ll *cur_assigned_crtc, *new_assigned_crtc;

	for (i = 0; i < param->num_crtcs; ++i) {
		already_assigned = 0;
		cur_assigned_crtc = PVAR->assigned_crtc_head;
		while (cur_assigned_crtc) {
			if (cur_assigned_crtc->crtc == param->output_crtcs[i]) {
				// umon_print
				//     ("cur_assigned_crtc: %d, param->output_crtcs[i]: %d\n",
				//      cur_assigned_crtc->crtc, param->output_crtcs[i]);
				already_assigned = 1;
			}
			cur_assigned_crtc = cur_assigned_crtc->next;
		}
		if (already_assigned == 0) {
			*(param->crtc_p) = param->output_crtcs[i];
			// Add newly assigned crtc to the list
			new_assigned_crtc = (crtc_ll *)
			    umalloc(sizeof(crtc_ll));
			new_assigned_crtc->crtc = param->output_crtcs[i];
			new_assigned_crtc->next = PVAR->assigned_crtc_head;
			PVAR->assigned_crtc_head = new_assigned_crtc;
			return;
		}
	}
	umon_print("ERROR: No available crtc found\n");
	exit(NO_AVAILABLE_CRTC);
}


static void modify_crtc_ll(load_class * self,
			   struct modify_crtc_ll_param_t *param)
{
	int i, should_disable;
	xcb_randr_get_crtc_info_cookie_t crtc_info_cookie;
	xcb_randr_get_crtc_info_reply_t *crtc_info_reply;
	xcb_randr_output_t *conn_output;
	set_crtc_param *cur_crtc_param;
	struct remove_matching_crtc_from_ll_param_t remove_crtc_param;
	struct add_disable_crtc_param_t add_disable_param;
// #ifdef DEBUG
	xcb_randr_get_output_info_cookie_t output_info_cookie;
	xcb_randr_get_output_info_reply_t *output_info_reply;
	char *output_name;
// #endif


	i = *(param->i_p);
	should_disable = 1;
	crtc_info_cookie =
	    xcb_randr_get_crtc_info(PVAR->screen_o->c,
				    param->crtcs_p[i], XCB_CURRENT_TIME);
	crtc_info_reply =
	    xcb_randr_get_crtc_info_reply(PVAR->screen_o->c,
					  crtc_info_cookie,
					  &PVAR->screen_o->e);
	conn_output = xcb_randr_get_crtc_info_outputs(crtc_info_reply);
	cur_crtc_param = PVAR->crtc_param_head;

// #ifdef DEBUG
	umon_print("Does any of queued crtc match currently loaded crtc %d?\n", param->crtcs_p[i]);
	output_info_cookie =
	    xcb_randr_get_output_info(PVAR->screen_o->c, xcb_randr_get_crtc_info_outputs(crtc_info_reply)[0],
				      XCB_CURRENT_TIME);

	output_info_reply =
	    xcb_randr_get_output_info_reply(PVAR->screen_o->c,
					    output_info_cookie,
					    &PVAR->screen_o->e);
	get_output_name(output_info_reply, &output_name);

	umon_print
	    ("Crtc: x:%d, y:%d, rotation: %d, mode_id: %d, output: %s\n",
	     crtc_info_reply->x,
	     crtc_info_reply->y,
	     crtc_info_reply->rotation,
	     crtc_info_reply->mode, output_name);
	free(output_info_reply);
	free(output_name);
// #endif
	remove_crtc_param.should_disable_p = &should_disable;
	remove_crtc_param.cur_crtc_param_p = &cur_crtc_param;
	remove_crtc_param.crtc_info_reply = crtc_info_reply;
	remove_crtc_param.conn_output = conn_output;
	remove_crtc_param.crtc = param->crtcs_p[i];
	// TODO if using force load, determining currently loaded profile might be affected
	while (cur_crtc_param && !PVAR->force_load) {
		remove_matching_crtc_from_ll(self, &remove_crtc_param);
	}

	add_disable_param.i_p = param->i_p;
	add_disable_param.crtcs_p = param->crtcs_p;
	if (should_disable && crtc_info_reply->mode) {
		add_disable_crtc(self, &add_disable_param);
	}
	//umon_print("Crtc %d should be disabled? %d\n",
	//param->crtcs_p[i], should_disable);
	free(crtc_info_reply);

}

static void remove_matching_crtc_from_ll(load_class * self, struct remove_matching_crtc_from_ll_param_t
					 *param)
{
	set_crtc_param *old_crtc_param, *cur_crtc_param;

	cur_crtc_param = *(param->cur_crtc_param_p);
	if (cur_crtc_param->pos_x == param->crtc_info_reply->x &&
	    cur_crtc_param->pos_y == param->crtc_info_reply->y &&
	    cur_crtc_param->rotation == param->crtc_info_reply->rotation &&
	    cur_crtc_param->mode_id ==
	    param->crtc_info_reply->mode
	    && cur_crtc_param->output_p[0] == param->conn_output[0] &&
	    cur_crtc_param->crtc == param->crtc) {
		*(param->should_disable_p) = 0;
		//Remove current crtc_param from linked list
		umon_print("Queued crtc %d matches! ", cur_crtc_param->crtc);
		umon_print("Remove duplicate crtc %d from ll\n",
			   cur_crtc_param->crtc);
		if (cur_crtc_param->prev) {
			cur_crtc_param->prev->next = cur_crtc_param->next;
		} else {
			PVAR->crtc_param_head = cur_crtc_param->next;
		}
		if (cur_crtc_param->next)
			cur_crtc_param->next->prev = cur_crtc_param->prev;
		old_crtc_param = cur_crtc_param;
		*(param->cur_crtc_param_p) = cur_crtc_param->next;
		free(old_crtc_param);
	} else {
		*(param->cur_crtc_param_p) = cur_crtc_param->next;
	}
}

static void add_disable_crtc(load_class * self,
			     struct add_disable_crtc_param_t *param)
{
	int i;
	crtc_ll *new_disable_crtc;

	i = *(param->i_p);
	umon_print("Queing disable crtc %d\n", param->crtcs_p[i]);
	new_disable_crtc = (crtc_ll *) umalloc(sizeof(crtc_ll));
	new_disable_crtc->crtc = param->crtcs_p[i];
	new_disable_crtc->next = PVAR->disable_crtc_head;
	PVAR->disable_crtc_head = new_disable_crtc;

}



/*! \brief Set the crtcs to the queued up configuration stored in the
linked list crtc_param_head */

static void apply_settings(load_class * self)
{

	apply_settings_disable_crtcs(self);

	apply_settings_screen_size(self);
	//TODO implement
	//find_duplicate_crtc(self);
	apply_settings_enable_crtcs(self);

}

static void apply_settings_disable_crtcs(load_class * self)
{
	xcb_randr_set_crtc_config_cookie_t crtc_config_cookie;
	xcb_randr_set_crtc_config_reply_t *crtc_config_reply;
	crtc_ll *cur_disable_crtc, *old_disable_crtc;

	//umon_print("Disable crtcs here\n");
	if (!PVAR->disable_crtc_head)
		return;
	cur_disable_crtc = PVAR->disable_crtc_head;
	while (cur_disable_crtc) {
		umon_print("Disabling this crtc: %d\n",
			   cur_disable_crtc->crtc);
		crtc_config_cookie =
		    xcb_randr_set_crtc_config(PVAR->screen_o->c,
					      cur_disable_crtc->crtc,
					      XCB_CURRENT_TIME,
					      XCB_CURRENT_TIME, 0,
					      0, XCB_NONE,
					      XCB_RANDR_ROTATION_ROTATE_0,
					      0, NULL);

		old_disable_crtc = cur_disable_crtc;
		cur_disable_crtc = cur_disable_crtc->next;
		free(old_disable_crtc);
	}
	xcb_flush(PVAR->screen_o->c);
	crtc_config_reply =
	    xcb_randr_set_crtc_config_reply(PVAR->screen_o->c,
					    crtc_config_cookie,
					    &PVAR->screen_o->e);
	PVAR->last_time = crtc_config_reply->timestamp;
	free(crtc_config_reply);

}

static void apply_settings_screen_size(load_class * self)
{
	umon_print("Change screen size here\n");
	xcb_randr_set_screen_size(PVAR->screen_o->c,
				  PVAR->screen_o->screen->root,
				  (uint16_t) PVAR->umon_setting_val.
				  screen.width,
				  (uint16_t) PVAR->umon_setting_val.
				  screen.height,
				  (uint32_t) PVAR->umon_setting_val.
				  screen.widthMM,
				  (uint32_t) PVAR->umon_setting_val.
				  screen.heightMM);
	//xcb_flush(PVAR->screen_o->c);

}

static void apply_settings_enable_crtcs(load_class * self)
{
	xcb_randr_set_crtc_config_cookie_t crtc_config_cookie;
	xcb_randr_set_crtc_config_reply_t *crtc_config_reply;
	set_crtc_param *cur_crtc_param, *old_crtc_param;

	if (!PVAR->crtc_param_head)
		return;
	cur_crtc_param = PVAR->crtc_param_head;
	while (cur_crtc_param) {
		umon_print("Enable crtc %d\n", cur_crtc_param->crtc);
		crtc_config_cookie =
		    xcb_randr_set_crtc_config(PVAR->screen_o->c,
					      cur_crtc_param->crtc,
					      XCB_CURRENT_TIME,
					      XCB_CURRENT_TIME,
					      cur_crtc_param->pos_x,
					      cur_crtc_param->pos_y,
					      cur_crtc_param->mode_id,
					      cur_crtc_param->rotation,
					      1, cur_crtc_param->output_p);

		if (cur_crtc_param->is_primary) {
			xcb_randr_set_output_primary
			    (PVAR->screen_o->c,
			     PVAR->screen_o->screen->root,
			     *(cur_crtc_param->output_p));
		}
		old_crtc_param = cur_crtc_param;
		cur_crtc_param = cur_crtc_param->next;
		free(old_crtc_param);
	}
	// umon_print("Enable crtcs here\n");

	crtc_config_reply =
	    xcb_randr_set_crtc_config_reply(PVAR->screen_o->c,
					    crtc_config_cookie,
					    &PVAR->screen_o->e);
	PVAR->last_time = crtc_config_reply->timestamp;

	free(crtc_config_reply);

}



/*! \brief Load the values stored in the configuration file into this program
 */
static void load_config_val(load_class * self,
			    config_setting_t * profile_group)
{

	config_setting_t *mon_group, *group, *res_group, *pos_group;
	int i;
	mon_group = config_setting_lookup(profile_group, "Monitors");
	// printf("Checking group %d\n",mon_group);
	PVAR->num_out_pp = config_setting_length(mon_group);
	free(PVAR->umon_setting_val.outputs);
	PVAR->umon_setting_val.outputs =
	    (umon_setting_output_t *) umalloc(PVAR->num_out_pp *
					      sizeof
					      (umon_setting_output_t));

	for (i = 0; i < PVAR->num_out_pp; ++i) {
		group = config_setting_get_elem(mon_group, i);
		// printf("Checking group %d\n",group);
		// Check if output is disabled
		PVAR->umon_setting_val.outputs[i].output_name =
		    config_setting_name(group);
		res_group = config_setting_lookup(group, "resolution");
		pos_group = config_setting_lookup(group, "pos");
		config_setting_lookup_string(group, "EDID",
					     &(PVAR->
					       umon_setting_val.outputs[i].
					       edid_val));
		if (!config_setting_lookup_int
		    (group, "primary",
		     &(PVAR->umon_setting_val.outputs[i].is_primary))) {
			PVAR->umon_setting_val.outputs[i].is_primary = 0;
		}
		config_setting_lookup_int(res_group, "x",
					  &(PVAR->
					    umon_setting_val.outputs[i].
					    res_x));
		config_setting_lookup_int(res_group, "y",
					  &(PVAR->
					    umon_setting_val.outputs[i].
					    res_y));
		config_setting_lookup_int(pos_group, "x",
					  &(PVAR->
					    umon_setting_val.outputs[i].
					    pos_x));
		config_setting_lookup_int(pos_group, "y",
					  &(PVAR->
					    umon_setting_val.outputs[i].
					    pos_y));
		if(config_setting_lookup_int(group, "rotation", &(PVAR->umon_setting_val.outputs[i].rotation)) == CONFIG_FALSE)
			PVAR->umon_setting_val.outputs[i].rotation = 1;

	}

	group = config_setting_lookup(profile_group, "Screen");
	config_setting_lookup_int(group, "width",
				  &(PVAR->umon_setting_val.screen.width));
	config_setting_lookup_int(group, "height",
				  &(PVAR->umon_setting_val.screen.height));
	config_setting_lookup_int(group, "widthMM",
				  &(PVAR->umon_setting_val.
				    screen.widthMM));
	config_setting_lookup_int(group, "heightMM",
				  &(PVAR->umon_setting_val.
				    screen.heightMM));

	umon_print("Done loading values from configuration file\n");
}
