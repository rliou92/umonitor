void screen_class_constructor(screen_class *);
void screen_class_destructor(screen_class *);

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
