#include <stdlib.h>
#include <stdio.h>

#define PVAR ((autoload_pvar *) (self->pvar))



typedef struct _autoload_class {

	// Private Variables
	void *pvar;

	// Public variable
	int x;


} autoload_class;

typedef struct {

	int output_match;
	int num_out_pp;		/*!< Number of outputs per profile */
	int num_conn_outputs;	/*!< number of connected outputs */

}autoload_pvar;

autoload_class* autoload_constructor(void);

autoload_class* autoload_constructor()
{
	autoload_class *self;
	self = (autoload_class *) malloc(sizeof(autoload_class));
	self->pvar = (void *) malloc(sizeof(autoload_pvar));
	PVAR->output_match = 1;
	return self;


}

int main(int argc, char **argv)
{

	autoload_class *autoload_o;
	autoload_o = autoload_constructor();
	printf("output_match: %d\n",((autoload_pvar *) autoload_o->pvar)->output_match);


}
