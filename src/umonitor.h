// Global variables
const char *CONFIG_FILE; /*!< Configuration file path name*/

// Some helper functions
void fetch_edid(xcb_randr_output_t * output_p, screen_class * screen_t_p,
		char **edid_string);
void for_each_output(
	void *self,
	xcb_randr_get_screen_resources_reply_t *screen_resources_reply,
	void (*callback)(void *,xcb_randr_output_t *)
);
void for_each_output_mode(
  void *self,
  xcb_randr_get_output_info_reply_t *output_info_reply,
	void (*callback)(void *,xcb_randr_mode_t *)
);
void umon_print(const char*,...);
