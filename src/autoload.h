/*! Class for determining which profile to load based on the current
display settings */

typedef struct _autoload_class {

	// Private Variables
	void *pvar;
	// Methods
	/*! find which profile matches current display settings and loads that
	   profile */
	void (*find_profile_and_load) (struct _autoload_class * self,
				       int test_cur);
	/*! waits for the screen change event, then calls find_profile_and_load */
	void (*wait_for_event) (struct _autoload_class * self);
	void (*get_profile_found) (struct _autoload_class * self, int * profile_found, const char **profile_name);


} autoload_class;

void autoload_constructor(autoload_class ** self_p,
			  screen_class * screen_o, config_t * config);
void autoload_destructor(autoload_class * self);
