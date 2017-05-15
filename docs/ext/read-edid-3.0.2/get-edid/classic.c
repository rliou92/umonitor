/* (c) 2000,2001,2002 John Fremlin */
/* (c) 2010,2011 Matthew Kern */
#ifdef CLASSICBUILD
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/io.h>
#include <unistd.h>

#include <libx86.h>

typedef u_int8_t byte;
typedef u_int32_t uint32;

typedef struct LRMI_regs reg_frame;
typedef byte* real_ptr;

#define access_register(reg_frame,reg) (reg_frame . reg)
#define access_ptr_register(reg_frame,reg) (reg_frame -> reg)
#define access_seg_register(reg_frame,es) reg_frame.es
#define real_mode_int(interrupt,reg_frame_ptr) !LRMI_int(interrupt,reg_frame_ptr)
#define alloc_real(size) LRMI_alloc_real(size)
#define free_real(block) LRMI_free_real(block)
#define dosmemput(buffer,length,offset) memcpy(offset,buffer,length)

#define display(...) if (quiet == 0) { fprintf(stderr, __VA_ARGS__); }
int quiet;

real_ptr far_ptr_to_real_ptr( uint32 farptr )
{
  unsigned segment = (farptr & 0xffff0000)>>16;
  unsigned offset = (farptr & 0xffff);
  return (real_ptr)((size_t)(segment*16+offset));
}




#define MAGIC   0x13

#define EDID_BLOCK_SIZE	128
#define EDID_V1_BLOCKS_TO_GO_OFFSET 126

#define SERVICE_REPORT_DDC	0
#define SERVICE_READ_EDID	1
#define SERVICE_LAST		1  // Read VDIF has been removed from the spec.



const char* ddc1_support_message[] =
  {
    "Monitor and video card combination does not support DDC1 transfers",
    "Monitor and video card combination supports DDC1 transfers"
  }
;
const char* ddc2_support_message[] =
  {
    "Monitor and video card combination does not support DDC2 transfers",
    "Monitor and video card combination supports DDC2 transfers"
  }
;
const char* screen_blanked_message[] =
  {
    "Screen is not blanked during DDC transfer",
    "Screen is blanked during DDC transfer"
  }
;


const char* supported_message[] =
  {
    "Function unsupported",
    "Function supported"
  }
;
const char* call_successful_message[] =
  {
    "Call failed",
    "Call successful"
  }
;

const char* vbe_service_message[] =
  {
    "Report DDC capabilities",
    "Read EDID"
  }
;

void
contact_author()
{
  display("\n\n*********** Something special has happened!\n" );
  display("This happens a lot with TV's, and other devices\n");
  display("with extension blocks. If you have a TV, don't bother.\n");
 #ifdef I2CBUILD
  display("Odds are good that I2C will work for you. Try 'modprobe i2c-dev'.\n");
 #else
  display("I'd suggest building the I2C version and trying that if you can.\n");
 #endif
  display("Otherwise, please contact the author, Matthew Kern\n" );
  display("E-mail: pyrophobicman@gmail.com\n" );
  display("Please include full output from this program (especially that to stderr)\n\n\n\n" );
}


int
read_edid(  unsigned controller, FILE* output );

int do_vbe_ddc_service(unsigned BX,reg_frame* regs);

int
report_ddc_capabilities( unsigned controller );

int
report_vcontroller();
     

int
classicmain( unsigned contr, int qit )
{
  unsigned controller = 0;
  int error;
  FILE* output;

  //fprintf(stderr, "%s: get-edid version %s\n", argv[0], VERSION);
  quiet=qit;

  if( !LRMI_init() )
    {
      display( "error initialising realmode interface\n");
      display( "do you have full superuser (root) permissions?\n");
      return 10;
    }
  
  ioperm(0, 0x400 , 1);
  iopl(3);
  

  /*if ( argc == 1 )
    output = stdout;
  else
    {
      if ( argc > 3 )
	{
	  display( "%s: syntax %s [output-filename] [controller]\n",
		   argv[0],argv[0] );
	  return 3;
	}
      output = fopen( argv[1], "wb" );
      if ( !output )
	{
	  display( "%s: error opening file \"%s\" for binary output\n",
		   argv[0],argv[1] );
	  return 4;
	}
      if ( argc == 3 )
	controller = strtol( argv[2],0,0 );
    }
  */
  output = stdout;
  controller = contr;

  report_vcontroller();
  
  error = report_ddc_capabilities(controller);
  
  if (read_edid(controller,output))
    error = 1;

  fclose(output);
  
  return error;
}

int do_vbe_service(unsigned AX,unsigned BX,reg_frame* regs)
{
  const unsigned interrupt = 0x10;
  unsigned function_sup;
  unsigned success;
  int error = 0;

  access_ptr_register(regs,eax) = AX;
  access_ptr_register(regs,ebx) = BX;

  display( "\n\tPerforming real mode VBE call\n" );

  display( "\tInterrupt 0x%x ax=0x%x bx=0x%x cx=0x%x\n",
	   interrupt, AX, BX, (unsigned)access_ptr_register(regs,ecx) );
  
  if( real_mode_int(interrupt, regs) )
    {
      display( "Error: something went wrong performing real mode interrupt\n" );
      error = 1;
    }
  
  AX = access_ptr_register(regs,eax);

  function_sup = ((AX & 0xff) == 0x4f);
  success = ((AX & 0xff00) == 0);
  
  display( "\t%s\n", supported_message[ function_sup ] );
  display( "\t%s\n\n", call_successful_message[ success ] );

  if (!success)
    error=1;
  if (!function_sup)
    error=2;
  
  return error;
}
  
int do_vbe_ddc_service(unsigned BX,reg_frame* regs)
{
  unsigned service = BX&0xff;
  unsigned AX = 0x4f15;


  display( "\nVBE/DDC service about to be called\n" );
  if ( service > SERVICE_LAST )
    {
      display( "\tUnknown VBE/DDC service\n" );
    }
  else
    {
      display( "\t%s\n", vbe_service_message[ service ] );
    }  
  return do_vbe_service(AX,BX,regs);
}


int
report_ddc_capabilities( unsigned controller )
{
  reg_frame regs;
  int error;
  unsigned seconds_per_edid_block;
  unsigned ddc1_support;
  unsigned ddc2_support;
  unsigned screen_blanked_during_transfer;
  
  memset(&regs, 0, sizeof(regs));
  
  access_register(regs,ecx) = controller;
  
  error = do_vbe_ddc_service( SERVICE_REPORT_DDC,&regs );

  if ( !error )
    {
      seconds_per_edid_block = (access_register(regs,ebx) & 0xff00)>>16;
      ddc1_support = (access_register(regs,ebx) & 0x1)?1:0;
      ddc2_support = (access_register(regs,ebx) & 0x2)?1:0;
      screen_blanked_during_transfer = (access_register(regs,ebx) & 0x4)?1:0;

      display( "\t%s\n", ddc1_support_message[ddc1_support] );
      display( "\t%s\n", ddc2_support_message[ddc2_support] );
      display( "\t%u seconds per 128 byte EDID block transfer\n",
	       seconds_per_edid_block );
      display( "\t%s\n\n", screen_blanked_message[screen_blanked_during_transfer] );
    }
  
  return error;
}


int
read_edid(  unsigned controller, FILE* output )
{
  reg_frame regs;
 
  real_ptr block;
  byte* buffer;
  byte* pointer;
  unsigned blocks_left = 1;
  unsigned last_reported = 0x1000; /* just a large number */
  unsigned author_msg_shown = 0;
  int err = 0;

  block = alloc_real( EDID_BLOCK_SIZE );
  
  if ( !block )
    {
      display( "Error: can't allocate 0x%x bytes of DOS memory for output block\n",
	       EDID_BLOCK_SIZE );
      return 2;
    }


  buffer = block;

  do
    {
      unsigned counter;

      display( "Reading next EDID block\n" );
      memset( block, MAGIC, EDID_BLOCK_SIZE );
      memset(&regs, 0, sizeof(regs));

      access_seg_register(regs,es) = (unsigned)((size_t)block)/16;
      access_register(regs,edi) = 0;
      access_register(regs,ecx) = controller;
      access_register(regs,edx)= blocks_left;

      if ( do_vbe_ddc_service( SERVICE_READ_EDID, &regs ) )
	{
	  display(
		   "The EDID data should not be trusted as the VBE call failed\n" );
	}
      
      for( pointer=block, counter=EDID_BLOCK_SIZE; counter; counter--,pointer++ )
	{
	  if ( *pointer != MAGIC )
	    goto block_ok;
	}
      display( "Error: output block unchanged\n" );
      err = 1;
      break;

    block_ok:

      if ( EDID_BLOCK_SIZE != fwrite( buffer, sizeof( byte ), EDID_BLOCK_SIZE, output ) )
	{
	  display( "\nError: problem writing output\n" );
	  err = 1;
	  break;
	}
      
      blocks_left--;
      if ( buffer[ EDID_V1_BLOCKS_TO_GO_OFFSET ] > blocks_left )
	{
	  int reported = buffer[ EDID_V1_BLOCKS_TO_GO_OFFSET ];
	  
	  display( "EDID claims %u more blocks left\n", reported);
	  if ( last_reported <= reported
		    || reported == 0xff
		    /* 0xff frequently comes up in corrupt edids */
		    || reported == MAGIC
		    )
	    {
	      display(
		       "EDID blocks left is wrong.\n"
		       "Your EDID is probably invalid.\n");
	    }
	  else {
	    if(!author_msg_shown) {
	      contact_author();
	      author_msg_shown = 1;
	    }

	    last_reported = reported;
	    blocks_left = reported;
	  }
	}
    } while( blocks_left );
  
  free_real( block );
  
  return err;
}

int
report_vcontroller()
{
  const unsigned info_block_length = 0x200;
  real_ptr block;
  real_ptr oem_string;
  reg_frame regs;
  byte* buffer;
  uint32 oem_string_ptr;
  byte next;
  unsigned vbe_version=0;

  memset(&regs, 0, sizeof(regs));

  block = alloc_real( info_block_length );
  if ( !block )
    {
      display(
	       "Error: can't allocate %x bytes of DOS memory for videocard info block\n",
	       info_block_length );
      return 2;
    }
  
  buffer = block;

  dosmemput("VBE2",4,block);

  access_seg_register(regs,es) = (unsigned)((size_t)block)/16;
  access_register(regs,edi) = 0;
  
  do_vbe_service(0x4f00,0,&regs);

  memcpy( &vbe_version, buffer+4, 2 );
  memcpy( &oem_string_ptr, buffer+6, 4 );

  oem_string = far_ptr_to_real_ptr( oem_string_ptr );

  display( "\tVBE version %x\n", vbe_version );
  display( "\tVBE string at 0x%x \"", (unsigned)((size_t)oem_string) );
  
  for(;;)
    {
      next = *oem_string;
      if ( !next )
	break;
      fputc( next,stderr );

      oem_string++;
    }
  
  display("\"\n" );
  return 1;
}
#endif
