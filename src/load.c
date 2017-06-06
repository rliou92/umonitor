#include "load.h"
#include "common.h"

#DEFINE PVAR ((load_pvar *) (self->pvar))

static void load_profile(load_class * self,
			 config_setting_t * profile_group, int);
static void match_with_config(void *self_void,
			      xcb_randr_output_t * output_p);
static void find_mode_id(load_class * self);
static void load_config_val(load_class *);
static void find_available_crtc(load_class *, xcb_randr_crtc_t *);
static void apply_settings(load_class *);

/*! \file
    \brief Load a profile and apply the changes to the current
configuration (if needed)

*/

typedef struct {
	screen_class *screen_o;	/*!< Connection information to server */

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
	int i;
	int should_disable;
	set_crtc_param *cur_crtc_param;
	set_crtc_param *crtc_param_head;	/*!< head of crtc queue */
	config_setting_t *profile_group;	/*!< which profile to be loaded */
	xcb_timestamp_t last_time;	/*!< last time the crtc setting was changed */
	xcb_randr_crtc_t *crtcs_p;	/*!< crtcs associated with the screen */
	crtc_ll *disable_crtc_head, *assigned_crtc_head;	/*!< disable crtcs linked list */

}load_pvar;
// Inheriting classes



/*! load constructor*/

void load_class_constructor(load_class ** self_p, screen_class * screen_o)
{
	load_class *self;
	self = (load_class *) malloc(sizeof(load_class));
	self->pvar = (void *) malloc(sizeof(load_pvar));
	self->load_profile = load_profile;
	PVAR->screen_o = screen_o;
	PVAR->crtc_param_head = NULL;
	PVAR->umon_setting_val.outputs = NULL;
	PVAR->output_info_reply = NULL;
	PVAR->disable_crtc_head = NULL;
	PVAR->assigned_crtc_head = NULL;
	PVAR->last_time = (xcb_timestamp_t) 0;

	*self_p = self;

}

/*! load deconstructor*/

void load_class_destructor(load_class * self)
{

	free(PVAR->umon_setting_val.outputs);
	free(PVAR->output_info_reply);

	free(self);

}

/*! \brief Load the specified profile
 */

static void load_profile(load_class * self,
			 config_setting_t * profile_group, int test_cur)
{

	set_crtc_param *old_crtc_param;
	crtc_ll *cur_free_crtc, *old_free_crtc;
	xcb_randr_output_t *conn_output;
	int crtc_match;

	PVAR->profile_group = profile_group;
	load_config_val(self);
	PVAR->crtcs_p =
	    xcb_randr_get_screen_resources_crtcs(PVAR->
						 screen_o->screen_resources_reply);
	for_each_output((void *) self,
			PVAR->screen_o->screen_resources_reply,
			match_with_config);
	// Check against redundant loading
	//TODO Only assuming 1 output connected to crtc
	crtc_match = 0;
	for (PVAR->i = 0;
	     PVAR->i < PVAR->screen_o->screen_resources_reply->num_crtcs;
	     ++PVAR->i) {
		     modify_crtc_ll(self);

	}
	if ((PVAR->crtc_param_head == NULL) &&
	    (PVAR->disable_crtc_head == NULL)) {
		PVAR->cur_loaded = 1;
	} else {
		if (!test_cur) {
			apply_settings(self);
		} else {
			free_crtc_ll(PVAR->crtc_param_head);
			free_crtc_ll(PVAR->disable_crtc_head);
		}
		PVAR->cur_loaded = 0;
	}

	PVAR->crtc_param_head = NULL;
	PVAR->disable_crtc_head = NULL;
	free_crtc_ll(PVAR->assigned_crtc_head);
	PVAR->assigned_crtc_head = NULL;

}

static void free_crtc_ll(crtc_ll *crtc_ll_head)
{
	crtc_ll *cur_free_crtc,*old_free_crtc;
	cur_free_crtc = crtc_ll_head;
	while (cur_free_crtc) {
		old_free_crtc = cur_free_crtc;
		cur_free_crtc = cur_free_crtc->next;
		free(old_free_crtc);
	}
}

static void modify_crtc_ll(load_class * self)
{
	xcb_randr_get_crtc_info_cookie_t crtc_info_cookie;

	PVAR->should_disable = 1;
	crtc_info_cookie =
	    xcb_randr_get_crtc_info(PVAR->screen_o->c,
				    PVAR->crtcs_p[PVAR->i],
				    XCB_CURRENT_TIME);
	PVAR->crtc_info_reply =
	    xcb_randr_get_crtc_info_reply(PVAR->screen_o->c,
					  crtc_info_cookie,
					  &PVAR->screen_o->e);
	PVAR->conn_output =
	    xcb_randr_get_crtc_info_outputs(crtc_info_reply);
	PVAR->cur_crtc_param = PVAR->crtc_param_head;
	while (PVAR->cur_crtc_param) {
		remove_matching_crtc_from_ll(self);
	}
	if (PVAR->should_disable && PVAR->crtc_info_reply->mode) {
		add_disable_crtc(self);
	}
	umon_print("Crtc %d should be disabled? %d\n",
		   PVAR->crtcs_p[i], should_disable);
	free(crtc_info_reply);

}

static void add_disable_crtc(load_class * self)
{
	crtc_ll *new_disable_crtc;

	new_disable_crtc =
	    (crtc_ll *) malloc(sizeof(crtc_ll));
	new_disable_crtc->crtc = PVAR->crtcs_p[PVAR->i];
	new_disable_crtc->next = PVAR->disable_crtc_head;
	PVAR->disable_crtc_head = new_disable_crtc;

}

static void remove_matching_crtc_from_ll(load_class * self)
{
	set_crtc_param *old_crtc_param;

	if (PVAR->cur_crtc_param->pos_x == PVAR->crtc_info_reply->x &&
	    PVAR->cur_crtc_param->pos_y == PVAR->crtc_info_reply->y &&
	    PVAR->cur_crtc_param->mode_id ==
	    PVAR->crtc_info_reply->mode
	    && PVAR->cur_crtc_param->output_p[0] ==
	    PVAR->conn_output[0]) {
		PVAR->crtc_match++;
		PVAR->should_disable = 0;
		//Remove current crtc_param from linked list
		//printf("Remove current crtc_param from ll\n");
		if (PVAR->cur_crtc_param->prev) {
			PVAR->cur_crtc_param->prev->next =
			    PVAR->cur_crtc_param->next;
		} else {
			PVAR->crtc_param_head =
			    PVAR->cur_crtc_param->next;
		}
		if (PVAR->cur_crtc_param->next)
			PVAR->cur_crtc_param->next->prev =
			    PVAR->cur_crtc_param->prev;
		old_crtc_param = cur_crtc_param;
		PVAR->cur_crtc_param = PVAR->cur_crtc_param->next;
		free(old_crtc_param);
	} else {
		PVAR->cur_crtc_param = PVAR->cur_crtc_param->next;
	}


}


/*! \brief Set the crtcs to the queued up configuration stored in the
linked list crtc_param_head */

static void apply_settings(load_class * self)
{
	xcb_randr_set_crtc_config_cookie_t crtc_config_cookie;
	xcb_randr_set_crtc_config_reply_t *crtc_config_reply;
	set_crtc_param *cur_crtc_param, *old_crtc_param;
	crtc_ll *cur_disable_crtc, *old_disable_crtc;

	umon_print("Disable crtcs here\n");
	cur_disable_crtc = PVAR->disable_crtc_head;
	if (cur_disable_crtc) {
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
	xcb_flush(PVAR->screen_o->c);



	//TODO implement
	//find_duplicate_crtc(self);

	cur_crtc_param = PVAR->crtc_param_head;
	if (cur_crtc_param) {
		while (cur_crtc_param) {

			// printf("cur_crtc_param->output_p: %d\n",cur_crtc_param->output_p);
			// printf("*(cur_crtc_param->mode_id_p): %d\n",cur_crtc_param->mode_id);
			//printf("Enabling crtc %d\n",cur_crtc_param->crtc);
			//printf("About to load this pos_x: %d\n",cur_crtc_param->pos_x);
			crtc_config_cookie =
			    xcb_randr_set_crtc_config(PVAR->screen_o->c,
						      cur_crtc_param->crtc,
						      XCB_CURRENT_TIME,
						      XCB_CURRENT_TIME,
						      cur_crtc_param->pos_x,
						      cur_crtc_param->pos_y,
						      cur_crtc_param->mode_id,
						      XCB_RANDR_ROTATION_ROTATE_0,
						      1,
						      cur_crtc_param->output_p);

			if (cur_crtc_param->is_primary) {
				xcb_randr_set_output_primary
				    (PVAR->screen_o->c,
				     PVAR->screen_o->screen->root,
				     *(cur_crtc_param->output_p));
			}
			//if(crtc_config_reply_pp->status==XCB_RANDR_SET_CONFIG_SUCCESS) printf("Enabling crtc should be success\n");

			//printf("Configuring time: %" PRIu32 "\n",crtc_config_reply_pp->timestamp);
			// xcb_flush(PVAR->screen_o->c);
			//printf("crtc_config_reply_pp: %d\n",crtc_config_reply_pp->response_type);
			old_crtc_param = cur_crtc_param;
			cur_crtc_param = cur_crtc_param->next;
			free(old_crtc_param);
		}
		umon_print("Enable crtcs here\n");

		// xcb_flush(PVAR->screen_o->c);
		// Will segfault if no cur_crtc_param

		crtc_config_reply =
		    xcb_randr_set_crtc_config_reply(PVAR->screen_o->c,
						    crtc_config_cookie,
						    &PVAR->screen_o->e);
		PVAR->last_time = crtc_config_reply->timestamp;

		free(crtc_config_reply);
	}

}

/*! \brief For each connected output, find the configuration monitor edid that
matches connected monitor edid */

static void match_with_config(void *self_void,
			      xcb_randr_output_t * output_p)
{


	xcb_randr_get_output_info_cookie_t output_info_cookie;
	load_class *self = (load_class *) self_void;

	char *edid_string;

	PVAR->cur_output = output_p;
	output_info_cookie =
	    xcb_randr_get_output_info(PVAR->screen_o->c, *output_p,
				      XCB_CURRENT_TIME);

	free(PVAR->output_info_reply);
	PVAR->output_info_reply =
	    xcb_randr_get_output_info_reply(PVAR->screen_o->c,
					    output_info_cookie,
					    &PVAR->screen_o->e);

	fetch_edid(PVAR->cur_output, PVAR->screen_o, &edid_string);

	umon_print("Num outputs per profile: %d\n", PVAR->num_out_pp);

	for (PVAR->conf_output_idx = 0;
	     PVAR->conf_output_idx < PVAR->num_out_pp;
	     ++PVAR->conf_output_idx) {
		if (!strcmp
		    (PVAR->umon_setting_val.
		     outputs[PVAR->conf_output_idx].edid_val,
		     edid_string)) {
			//if (VERBOSE) printf("Before for each output mode\n");
			// Which crtc has the same resolution?
			find_mode_id(self);
			//if (VERBOSE) printf("Finish find_mode_id\n");
		}
	}
	free(edid_string);
}

/*! \brief After matching edid has been found, find the matching mode id from
the configuration's resolution x and y values

  Queues up the crtc to be loaded in apply_settings
 */

static void find_mode_id(load_class * self)
{

	int num_screen_modes, num_output_modes, i, j;

	xcb_randr_mode_info_iterator_t mode_info_iterator;
	set_crtc_param *new_crtc_param;
	xcb_randr_mode_t *output_modes;

	mode_info_iterator =
	    xcb_randr_get_screen_resources_modes_iterator
	    (PVAR->screen_o->screen_resources_reply);
	num_screen_modes =
	    xcb_randr_get_screen_resources_modes_length(PVAR->
							screen_o->screen_resources_reply);

	output_modes =
	    xcb_randr_get_output_info_modes(PVAR->output_info_reply);
	num_output_modes =
	    xcb_randr_get_output_info_modes_length
	    (PVAR->output_info_reply);

	if ((PVAR->umon_setting_val.outputs[PVAR->conf_output_idx].res_x !=
	     0)
	    && (PVAR->umon_setting_val.
		outputs[PVAR->conf_output_idx].res_y != 0)) {

		for (i = 0; i < num_screen_modes; ++i) {
			for (j = 0; j < num_output_modes; ++j) {
				if ((mode_info_iterator.data->width ==
				     PVAR->umon_setting_val.
				     outputs[PVAR->conf_output_idx].res_x)
				    && (mode_info_iterator.data->height ==
					PVAR->
					umon_setting_val.outputs[PVAR->
								 conf_output_idx].res_y)
				    && (mode_info_iterator.data->id ==
					output_modes[j])) {
					//if (VERBOSE) printf("Found current mode info\n");
					//sprintf(res_string,"%dx%d",mode_info_iterator.data->width,mode_info_iterator.data->height);
					new_crtc_param = (set_crtc_param *)
					    malloc(sizeof(set_crtc_param));
					find_available_crtc(self,
							    &
							    (new_crtc_param->crtc));
					umon_print
					    ("Queing up crtc to load: %d\n",
					     new_crtc_param->crtc);
					new_crtc_param->pos_x =
					    PVAR->
					    umon_setting_val.outputs[PVAR->
								     conf_output_idx].pos_x;
					new_crtc_param->pos_y =
					    PVAR->
					    umon_setting_val.outputs[PVAR->
								     conf_output_idx].pos_y;
					new_crtc_param->is_primary =
					    PVAR->
					    umon_setting_val.outputs[PVAR->
								     conf_output_idx].is_primary;
					new_crtc_param->mode_id =
					    mode_info_iterator.data->id;
					new_crtc_param->output_p =
					    PVAR->cur_output;
					new_crtc_param->next =
					    PVAR->crtc_param_head;

					new_crtc_param->prev = NULL;
					//printf("did I make it here\n");

					PVAR->crtc_param_head =
					    new_crtc_param;
					//printf("PVAR->crtc_param_head->next: %d\n",PVAR->crtc_param_head->next);
					if (PVAR->crtc_param_head->next)
						PVAR->
						    crtc_param_head->next->
						    prev = new_crtc_param;
					//PVAR->crtc_ll_length++;
					//printf("did I make it here\n");
				}
			}
			xcb_randr_mode_info_next(&mode_info_iterator);

		}
	}
	//if (VERBOSE) printf("Found current mode id\n");
}

/*! \brief Find an available crtc for the current output with a specified offset
 */
static void find_available_crtc(load_class * self,
				xcb_randr_crtc_t * crtc_p)
{

	crtc_ll *new_assigned_crtc, *cur_assigned_crtc;
	int already_assigned;

	xcb_randr_crtc_t *output_crtcs =
	    xcb_randr_get_output_info_crtcs(PVAR->output_info_reply);
	int num_output_crtcs =
	    xcb_randr_get_output_info_crtcs_length
	    (PVAR->output_info_reply);

	xcb_randr_crtc_t *available_crtcs =
	    xcb_randr_get_screen_resources_crtcs(PVAR->
						 screen_o->screen_resources_reply);
	int num_available_crtcs =
	    PVAR->screen_o->screen_resources_reply->num_crtcs;

	// if (VERBOSE) printf("num_available_crtcs: %d\n",num_available_crtcs);
	// if (VERBOSE) printf("num_output_crtcs: %d\n",num_output_crtcs);

	for (int i = 0; i < num_available_crtcs; ++i) {
		for (int j = 0; j < num_output_crtcs; ++j) {
			if (available_crtcs[i] == output_crtcs[j]) {
				already_assigned = 0;
				umon_print
				    ("Found potential crtc %d. Is it already assigned?\n",
				     available_crtcs[i]);
				for (cur_assigned_crtc =
				     PVAR->assigned_crtc_head;
				     cur_assigned_crtc;
				     cur_assigned_crtc =
				     cur_assigned_crtc->next) {
					umon_print
					    ("cur_assigned_crtc %d\n",
					     cur_assigned_crtc->crtc);
					if (cur_assigned_crtc->crtc ==
					    available_crtcs[i]) {
						umon_print
						    ("Crtc %d already assigned!\n",
						     available_crtcs[i]);
						already_assigned = 1;
					}
				}
				if (!already_assigned) {
					new_assigned_crtc = (crtc_ll *)
					    malloc(sizeof(crtc_ll));
					new_assigned_crtc->crtc =
					    available_crtcs[i];
					new_assigned_crtc->next =
					    PVAR->assigned_crtc_head;
					PVAR->assigned_crtc_head =
					    new_assigned_crtc;
					*crtc_p = available_crtcs[i];
					return;
				}
			}
		}
	}

	printf("failed to find a matching crtc\n");
	exit(10);

}

/*! \brief Load the values stored in the configuration file into this program
 */
static void load_config_val(load_class * self)
{

	config_setting_t *mon_group, *group, *res_group, *pos_group;
	int i;
	mon_group = config_setting_lookup(PVAR->profile_group, "Monitors");
	// printf("Checking group %d\n",mon_group);
	PVAR->num_out_pp = config_setting_length(mon_group);
	free(PVAR->umon_setting_val.outputs);
	PVAR->umon_setting_val.outputs =
	    (umon_setting_output_t *) malloc(PVAR->num_out_pp *
					     sizeof
					     (umon_setting_output_t));

	for (i = 0; i < PVAR->num_out_pp; ++i) {
		group = config_setting_get_elem(mon_group, i);
		// printf("Checking group %d\n",group);
		// Check if output is disabled

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

	}

	group = config_setting_lookup(PVAR->profile_group, "Screen");
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
