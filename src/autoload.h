#include "classes.h"

extern void fetch_edid(xcb_randr_output_t *, screen_class *,
		       char **edid_string);
extern void output_iter(void *self,
			    xcb_randr_get_screen_resources_reply_t *
			    screen_resources_reply,
			    void (*callback) (void *, xcb_randr_output_t *)
    );
extern load_class * load_class_constructor(screen_class *,
				   config_t *);
extern void load_class_destructor(load_class *);
extern void umon_print(const char *, ...);

autoload_class * autoload_constructor(screen_class * screen_o, config_t * config);
void autoload_destructor(autoload_class * self);

static void find_profile_and_load(autoload_class * self, int test_cur);
static void determine_profile_match(autoload_class * self);
static void wait_for_event(autoload_class * self);
static void validate_timestamp_and_load(autoload_class *self);
static void count_output_match(void *self_void,
			       xcb_randr_output_t * output_p);
static void determine_output_match(autoload_class * self);

typedef struct _autoload_class {

	// Private Variables
	void *pvar;
	// Methods
	/*! find which profile matches current display settings and loads that
	   profile */
	void (*find_profile_and_load) (struct _autoload_class * self, int test_cur);
	/*! waits for the screen change event, then calls find_profile_and_load */
	void (*wait_for_event) (struct _autoload_class * self);


} autoload_class;
