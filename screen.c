#include "classes.h"



void screen_class_constructor(screen_class *);
static void update_screen(screen_class *);
extern int VERBOSE;

void screen_class_constructor(screen_class *self){

	int i,screenNum;
	static const xcb_setup_t *setup;
	static xcb_screen_iterator_t iter;

	xcb_intern_atom_cookie_t atom_cookie;


	uint8_t only_if_exists = 1;
	const char *edid_name = "EDID";
	uint16_t name_len = strlen(edid_name);
	/* Open the connection to the X server. Use the DISPLAY environment variable */
	self->c = xcb_connect (NULL, &screenNum);



	/* Get the screen whose number is screenNum */

	setup = xcb_get_setup (self->c);
	iter = xcb_setup_roots_iterator (setup);

	// we want the screen at index screenNum of the iterator
	for (i = 0; i < screenNum; ++i) {
		xcb_screen_next (&iter);
	}

	self->screen = iter.data;
	if (VERBOSE) printf("Connected to server\n");
	//screen_resources_cookie = xcb_randr_get_screen_resources(c,screen->root);
	atom_cookie = xcb_intern_atom(self->c,only_if_exists,name_len,edid_name);

	self->edid_atom = xcb_intern_atom_reply(self->c,atom_cookie,&self->e);


	self->update_screen = update_screen;
	self->update_screen(self);

	// Want outputs linked list


}

static void update_screen(screen_class *self){
	xcb_randr_get_screen_resources_cookie_t screen_resources_cookie;
	screen_resources_cookie = xcb_randr_get_screen_resources(self->c,
		self->screen->root);
	self->screen_resources_reply =
		xcb_randr_get_screen_resources_reply(self->c,screen_resources_cookie,
			&self->e);

}
