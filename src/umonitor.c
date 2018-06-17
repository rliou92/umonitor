#include "common.h"
#include "umonitor.h"
#include "save.h"
#include "load.h"
#include "autoload.h"
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
/*! \mainpage Main Page
 *
 * \section intro_sec Introduction
 *
 * This program is intended to manage monitor hotplugging.

 	 \section readme README
	 Usage:
	 	1. Setup your configuration using xrandr or related tools
	 	2. Run \code umonitor --save <profile_name> \endcode
		3. Run \code umonitor --listen \endcode to begin automatically detecting and changing monitor configuration
 *
 */

/*! \file
		\brief Main file

		For indentation: indent -kr -i8

*/

typedef enum {
	SAVE,
	DELETE
} save_or_delete_t;

// private prototypes
static void preprocess_arguments(int argc, char **argv);
static void print_info(void);
static void print_current_state(void);
static void sigterm_handler(int signum);
static void start_listening(void);
static void start_load(char *profile_name);
static void start_delete_and_save(save_or_delete_t save_or_delete,
				  char *profile_name);
static void parse_arguments(void);


static const char help_str[] =
    "Usage: umonitor [OPTION]\n"
    "\n"
    "Options:\n"
    "\t-s,--save <profile_name>\tSaves current setup into profile_name\n"
    "\t-d,--delete <profile_name>\tRemoves profile_name from configuration file\n"
    "\t-l,--load <profile_name>\tLoads setup from profile name\n"
    "\t-n,--listen\t\t\tDaemonizes, listens for changes in the setup, and applies the new"
    " configuration automatically\n"
    "\t-a,--autoload\t\t\tLoad profile that matches with current configuration once\n"
    "\t--quiet\t\t\t\tSupress program output\n"
    "\t--help\t\t\t\tDisplay this help and exit\n"
    "\t--version\t\t\tOutput version information and exit\n";

static const char version_str[] =
    "umonitor 20170805\n" "Written by Ricky Liou\n";

static const char lockfile_name[] = "/.umonitor.lock";
char * home_directory;
char * lockfile;

static screen_class screen_o;
static config_t config;
static int c;
static int option_index = 0;
static int verbose = 0, version = 0, help = 0, autoload = 0, quiet = 0, save = 0;

#ifdef LOG
static FILE *log_file;
#endif

static const char *short_options = "s:l:d:naq";
static const struct option long_options[] = {
	{"save", required_argument, 0, 's'},
	{"load", required_argument, 0, 'l'},
	{"delete", required_argument, 0, 'd'},
	{"listen", no_argument, 0, 'n'},
	{"autoload", no_argument, 0, 'a'},
	{"quiet", no_argument, &quiet, 1},
	{"help", no_argument, &help, 1},
	{"version", no_argument, &version, 1},
	{"verbose", no_argument, &verbose, 1}

};


void umon_print(const char *format, ...)
{
	va_list args;

	if (!verbose)
		return;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	fflush(stdout);

#ifdef LOG
	vfprintf(log_file, format, args);
	fflush(log_file);
	fsync(fileno(log_file));
#endif

}

void print_state(const char *format, ...)
{
	va_list args;

	if (quiet)
		return;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);

}

/*! Logic for parsing options here*/

int main(int argc, char **argv)
{
	home_directory = getenv("HOME");
	if(home_directory == NULL)
		exit(NO_HOME_DIR);
	const char *conf_location = "/.config/umon.conf";

	config_init(&config);

#ifdef LOG
	log_file = fopen("umonitor.log", "w");
#endif

	preprocess_arguments(argc, argv);

	print_info();

	screen_class_constructor(&screen_o);
	CONFIG_FILE =
	    umalloc((strlen(home_directory) + strlen(conf_location))+1);
	strcpy(CONFIG_FILE, home_directory);
	strcat(CONFIG_FILE, conf_location);

	optind = 1;
	opterr = 1;
	while (1) {
		c = getopt_long(argc, argv, short_options, long_options,
				&option_index);
		if (c == -1)
			break;

		parse_arguments();
	}

	print_current_state();

	// Free things
	// Screen destructor
	screen_class_destructor(&screen_o);

	config_destroy(&config);
	free(CONFIG_FILE);

}

static void preprocess_arguments(int argc, char **argv)
{
	opterr = 0;
	int arg_counter = 0;
	while (1) {
		c = getopt_long(argc, argv, short_options, long_options,
				&option_index);
		switch (c) {
			case 's':
			case 'l':
			case 'd':
			case 'n':
			case 'a':
			++arg_counter;
			break;
		}
		if (c == -1)
			break;
	}
	if (arg_counter > 1){
		fprintf(stderr,"More than one action specified.\n");
		exit(MULTIPLE_ACTION);
	}
}

static void print_info()
{
	if (version)
		printf("%s", version_str);
	if (help)
		printf("%s", help_str);

}

static void print_current_state()
{
	autoload_class *autoload_o;

	if (help || version || autoload || save)
		return;
	// Print current state
	if (!config_read_file(&config, CONFIG_FILE))
		exit(NO_CONF_FILE_FOUND);
	autoload_constructor(&autoload_o, &screen_o, &config);
	//autoload_o->find_profile_and_load(autoload_o);
	autoload_o->find_profile_and_load(autoload_o, NO_LOAD, PRINT);
	autoload_destructor(autoload_o);

}

static void sigterm_handler(int signum)
{
	// int pid_h;

	// pid_h = open(lockfile, O_RDWR);
	remove(lockfile);
	free(lockfile);
	signal(signum, SIG_DFL);
	kill(getpid(), signum);
}

static void start_listening()
{
	autoload_class *autoload_o;
	int pid_h;
	struct sigaction sa;
	char pid_str[10];

	if (!config_read_file(&config, CONFIG_FILE))
		exit(NO_CONF_FILE_FOUND);

	lockfile =
	    umalloc((strlen(home_directory) + strlen(lockfile_name))+1);
	strcpy(lockfile, home_directory);
	strcat(lockfile, lockfile_name);
	pid_h = open(lockfile, O_RDWR|O_CREAT|O_EXCL, 0600);
	if (pid_h == -1) {
		printf("umonitor process already running.\n");
		printf("Please stop the existing process and try again.\n");
		exit(DAEMON_ALREADY_RUNNING);

	}

	sprintf(pid_str,"%d\n",getpid());
	write(pid_h, pid_str, strlen(pid_str));
	close(pid_h);

	sa.sa_handler = sigterm_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction (SIGINT, &sa, NULL);
	sigaction (SIGTERM, &sa, NULL);
	sigaction (SIGQUIT, &sa, NULL);
	signal (SIGHUP, SIG_IGN);

	// Daemonize
	daemon(0,0);
	autoload_constructor(&autoload_o, &screen_o, &config);
	umon_print("Autoloading\n");
	autoload_o->wait_for_event(autoload_o);
	autoload_destructor(autoload_o);

}



static void start_load(char *profile_name)
{
	load_class *load_o;
	config_setting_t *profile_group;

	if (!config_read_file(&config, CONFIG_FILE))
		exit(NO_CONF_FILE_FOUND);

	umon_print("Loading profile: %s\n", profile_name);
	// Load profile
	profile_group = config_lookup(&config, profile_name);

	if (profile_group == NULL)
		exit(NO_PROFILE_FOUND);

	load_class_constructor(&load_o, &screen_o);
	load_o->load_profile(load_o, profile_group, 0);
	load_class_destructor(load_o);

	print_state("Profile %s loaded!\n", profile_name);
	print_state("---------------------------------\n");

}

static void start_delete_and_save(save_or_delete_t save_or_delete,
				  char *profile_name)
{
	save_class *save_o;
	autoload_class *autoload_o;
	config_setting_t *root, *profile_group;
	int cfg_idx, profile_found;
	const char *cur_loaded_profile_name;

	config_read_file(&config, CONFIG_FILE);


	umon_print
	    ("Saving current settings into profile: %s\n", profile_name);
	umon_print("Checking if current setup matches a profile\n");

	autoload_constructor(&autoload_o, &screen_o, &config);
	autoload_o->find_profile_and_load(autoload_o, NO_LOAD, NO_PRINT);
	autoload_o->get_profile_found(autoload_o, &profile_found, &cur_loaded_profile_name);
	autoload_destructor(autoload_o);




	/*
	 * Always create the new profile group because above code has already
	 * deleted it if it existed before
	 */
	if (profile_found && (save_or_delete == SAVE)) {
		print_state("Current profile is already saved under profile %s\n", cur_loaded_profile_name);
	}
	else {
		profile_group = config_lookup(&config, profile_name);

		if (profile_group != NULL) {
			// Overwrite existing profile
			cfg_idx = config_setting_index(profile_group);
			root = config_root_setting(&config);
			config_setting_remove_elem(root, cfg_idx);
			umon_print("Deleted profile %s\n", profile_name);
			if (save_or_delete == DELETE) {
				umon_print("Profile %s deleted!\n", profile_name);
				config_write_file(&config, CONFIG_FILE);
				return;
			}

		}
		root = config_root_setting(&config);
		profile_group = config_setting_add(root, profile_name, CONFIG_TYPE_GROUP);

		save_class_constructor(&save_o, &screen_o, &config);
		save_o->save_profile(save_o, profile_group);
		save_class_destructor(save_o);
		print_state("Profile %s saved!\n", profile_name);
	}

}

static void start_autoload()
{
	// TODO Print which profile is detected and loaded
	autoload_class *autoload_o;

	autoload = 1;		// Flag to prevent printing state twice
	if (!config_read_file(&config, CONFIG_FILE))
		exit(NO_CONF_FILE_FOUND);
	autoload_constructor(&autoload_o, &screen_o, &config);
	autoload_o->find_profile_and_load(autoload_o, LOAD, PRINT);
	autoload_destructor(autoload_o);

}

static void parse_arguments()
{
	switch (c) {
	case 's':
		save = 1;
		start_delete_and_save(SAVE, optarg);
		break;
	case 'l':
		start_load(optarg);
		break;
	case 'd':
		start_delete_and_save(DELETE, optarg);
		break;
	case 'n':
		start_listening();
		break;
	case 'a':
		start_autoload();
		break;
	case '?':
		fprintf(stderr, "No argument given\n");
		exit(NO_ARGUMENT_GIVEN);
		break;
	case '0':		// just setting flags
		break;
	}

}


/*! \brief Loop over each output

	Calls the callback function for each output
 */
void for_each_output(void *self,
		     xcb_randr_get_screen_resources_reply_t *
		     screen_resources_reply,
		     void (*callback) (void *, xcb_randr_output_t *))
{

	int i, outputs_length;

	xcb_randr_output_t *output_p;

	output_p =
	    xcb_randr_get_screen_resources_outputs(screen_resources_reply);
	outputs_length =
	    xcb_randr_get_screen_resources_outputs_length
	    (screen_resources_reply);

	for (i = 0; i < outputs_length; ++i) {
		callback(self, output_p++);
	}

}

/*! \brief Loop over each output mode

Calls the callback function for each output mode

 */
void for_each_output_mode(void *self,
			  xcb_randr_get_output_info_reply_t *
			  output_info_reply,
			  void (*callback) (void *, xcb_randr_mode_t *))
{

	int j, num_output_modes;

	xcb_randr_mode_t *mode_id_p;

	num_output_modes =
	    xcb_randr_get_output_info_modes_length(output_info_reply);
	mode_id_p = xcb_randr_get_output_info_modes(output_info_reply);

	for (j = 0; j < num_output_modes; ++j) {
		callback(self, mode_id_p++);
	}

}

/*! \brief Converts the edid that is returned from the X11 server into a string
 *
 * @param[in]		output_p		the output whose edid is desired
 * @param[in]		screen_t_p	the connection information
 * @param[out]	edid_string	the edid in string form
 */

void fetch_edid(xcb_randr_output_t * output_p, screen_class * screen_t_p,
		char **edid_string)
{

	int i, j, model_name_found, edid_length;
	uint8_t delete = 0;
	uint8_t pending = 0;
	xcb_randr_get_output_property_cookie_t output_property_cookie;
	xcb_randr_get_output_property_reply_t *output_property_reply;
	uint8_t *edid;
	//char *edid_info;

	char vendor[4];
	// uint16_t product;
	//uint32_t serial;
	char modelname[13];

	output_property_cookie =
	    xcb_randr_get_output_property(screen_t_p->c, *output_p,
					  screen_t_p->edid_atom->atom,
					  AnyPropertyType, 0, 100, delete,
					  pending);

	output_property_reply =
	    xcb_randr_get_output_property_reply(screen_t_p->c,
						output_property_cookie,
						&screen_t_p->e);

	edid_length = xcb_randr_get_output_property_data_length(output_property_reply);
	// umon_print("edid_length: %d", edid_length);
	// edid = umalloc(edid_length *sizeof(uint8_t));


	edid = xcb_randr_get_output_property_data(output_property_reply);

	//umon_print("Starting edid_to_string\n");
	// *edid_string = (char *) malloc((length+1)*sizeof(char));
	// for (z=0;z<length;++z) {
	//      if ((char) edid[z] == '\0') {
	//              *(*edid_string+z) = '0';
	//      }
	//      else {
	//              *(*edid_string+z) = (char) edid[z];
	//      }
	//
	// }
	// *(*edid_string+z) = '\0';

	*edid_string = (char *) umalloc(17 * sizeof(char));

	if (edid_length == 0){
		strcpy(vendor,"N/A");
		strcpy(modelname, "unknown");
		snprintf(*edid_string, 17, "%s %s", vendor, modelname);
		free(output_property_reply);
		return;
	}

	char sc = 'A' - 1;
	vendor[0] = sc + (edid[8] >> 2);
	vendor[1] = sc + (((edid[8] & 0x03) << 3) | (edid[9] >> 5));
	vendor[2] = sc + (edid[9] & 0x1F);
	vendor[3] = '\0';

	//product = (edid[11] << 8) | edid[10];
	// serial = edid[15] << 24 | edid[14] << 16 | edid[13] << 8 | edid[12];
	// edid_info = malloc(length*sizeof(char));
	// snprintf(edid_info, length, "%04X%04X%08X", vendor, product, serial);

	model_name_found = 0;
	// for (i = 0x36; i < 0x7E; i += 0x12) {        //read through descriptor blocks...
	//      if (edid[i] == 0x00) {  //not a timing descriptor
	//              if (edid[i + 3] == 0xfc) {      //Model Name tag
	//                      model_name_found = 1;
	//                      for (j = 0; j < 13; j++) {
	//                              if (edid[i + 5 + j] == 0x0a)
	//                                      modelname[j] = 0x00;
	//                              else
	//                                      modelname[j] =
	//                                          edid[i + 5 + j];
	//                      }
	//              }
	//      }
	// }
	for (i = 0x36; i < 0x7E; i += 0x12) {	//read through descriptor blocks...
		if (edid[i] != 0x00 || edid[i + 3] != 0xfc)
			continue;	//not a timing descriptor
		model_name_found = 1;
		for (j = 0; j < 13; ++j) {
			if (edid[i + 5 + j] == 0x0a)
				modelname[j] = 0x00;
			else
				modelname[j] = edid[i + 5 + j];
		}
	}

	if (!model_name_found)
		strcpy(modelname, "unknown");

	// printf("vendor: %s\n",vendor);
	// printf("modelname: %s\n",modelname);
	// 3 for vendor, 1 for space, 12 for modelname, 1 for null
	snprintf(*edid_string, 17, "%s %s", vendor, modelname);

	free(output_property_reply);
	//umon_print("Finished edid_to_string on output %s\n", *edid_string);
}

void *umalloc(size_t size)
{
	void *mem_addr;
	mem_addr = malloc(size);
	if (mem_addr == NULL)
		exit(INSUFFICIENT_MEMORY);
	return mem_addr;
}

/*! \brief Convert the raw output name obtained from server into char
 */
void get_output_name(xcb_randr_get_output_info_reply_t *
		     output_info_reply, char **output_name)
{
	int i;
	uint8_t *output_name_raw =
	    xcb_randr_get_output_info_name(output_info_reply);
	int output_name_length =
	    xcb_randr_get_output_info_name_length(output_info_reply);
	*output_name =
	    (char *) umalloc((output_name_length + 1) * sizeof(char));

	for (i = 0; i < output_name_length; ++i) {
		(*output_name)[i] = (char) output_name_raw[i];
	}
	(*output_name)[i] = '\0';

}
