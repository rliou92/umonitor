typedef struct {
  xcb_connection_t *c;
  xcb_screen_t *screen;
  xcb_intern_atom_reply_t *edid_atom;

}screen_class;

void screen_class_constructor(screen_class *);


void screen_class_constructor(screen_class *self){

	int i,screenNum;
	static const xcb_setup_t *setup;
	static xcb_screen_iterator_t iter;

	xcb_intern_atom_cookie_t atom_cookie;
  xcb_generic_error_t **e;

	uint8_t only_if_exists = 1;
	const char *edid_name = "EDID";
	uint16_t name_len = strlen(edid_name);
	/* Open the connection to the X server. Use the DISPLAY environment variable */
	self->c = xcb_connect (NULL, &screenNum);



	/* Get the screen whose number is screenNum */

	setup = xcb_get_setup (c);
	iter = xcb_setup_roots_iterator (setup);

	// we want the screen at index screenNum of the iterator
	for (i = 0; i < screenNum; ++i) {
		xcb_screen_next (&iter);
	}

	self->screen = iter.data;
	if (verbose) printf("Connected to server\n");
	//screen_resources_cookie = xcb_randr_get_screen_resources(c,screen->root);
	atom_cookie = xcb_intern_atom(c,only_if_exists,name_len,edid_name);

	self->edid_atom = xcb_intern_atom_reply(c,atom_cookie,e);

	//screen_resources_reply =
		//xcb_randr_get_screen_resources_reply(c,screen_resources_cookie,e);
}
