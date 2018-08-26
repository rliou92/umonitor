/*! Class for loading specified profile from configuration file
*/
typedef struct _load_class {
	void *pvar;
	// Methods
	/*! loads specified profile from configuration file */
	void (*load_profile) (struct _load_class *, config_setting_t *,
			      int);
	void (*get_last_time) (struct _load_class *, xcb_timestamp_t *);
	void (*get_cur_loaded) (struct _load_class *, int *cur_loaded);
	void (*set_force_load) (struct _load_class *, int force_load);
} load_class;

void load_class_constructor(load_class ** self, screen_class * screen_o);
void load_class_destructor(load_class *);
