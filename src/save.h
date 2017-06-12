/*! Class for saving current display settings into the configuration file
*/

typedef struct _save_class {
	void *pvar;
	// Methods
	/*! saves the current display settings into configuration file */
	void (*save_profile) (struct _save_class *, config_setting_t *);
} save_class;

void save_profile_class_constructor(save_class **, screen_class *,
				    config_t *);
void save_class_destructor(save_class *);
