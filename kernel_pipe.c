
#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_dev.h"
/* The size of the pipe buffer */
#define PIPE_BUFFER_SIZE 1024

/* 
*	Pipe control block
*  	This structure holds all information pertaining to a pipe.
*/
typedef struct pipe_control_block
{
	/* File control block linked to the read end of the pipe */
	FCB * reader; 

	/* File control block linked to the write end of the pipe */
	FCB *writer;

	CondVar has_space;

	CondVar has_data;

	/* write position of the pipe buffer */
	int w_position;
	/* read position of the pipe buffer */
	int r_position;
	
	//refcount

	/* pipe buffer */
	char BUFFER[PIPE_BUFFER_SIZE];
} pipe_cb;

/* Helper functions */

/* This function is used to allocate memory space for the 
*  new pipe cb and to initialize its attributes.
*  It returns a pointer to the newly made and initialized pipe cb.
*/
pipe_cb * initialize_pipe_cb()
{
	/* allocating size for the new pipe control block */
	pipe_cb* pipeCB = (pipe_cb *) xmalloc(sizeof(pipe_cb));

	/* initializing attributes of the pipe_cb */
	pipeCB->reader=NULL;
	pipeCB->writer=NULL;
	pipeCB->has_space=COND_INIT;
	pipeCB->has_data=COND_INIT;
	pipeCB->r_position=0;
	pipeCB->w_position=0;
}



int  pipe_write(void * pipecb_t, const char *buf, uint n)
{
	pipe_cb * pipeCB = (pipe_cb*) pipecb_t;
	return -1;
}

int  pipe_writer_close(void * _pipecb)
{
	pipe_cb * pipeCB = (pipe_cb*) _pipecb;
	return -1;
}

int  pipe_read(void * pipecb_t, char *buf, uint n)
{
	pipe_cb * pipeCB = (pipe_cb*) pipecb_t;
	return -1;
}


int  pipe_reader_close(void * _pipecb)
{
	pipe_cb * pipeCB = (pipe_cb*) _pipecb;
	return -1;
}

/* Operations allowed for the read end of the pipe */
static file_ops pipe_read_fops = {
  .Open = NULL,
  .Read = pipe_read,
  .Write = NULL,
  .Close = pipe_reader_close
};
/* Operations allowed for the write end of the pipe */
static file_ops pipe_write_fops = {
  .Open = NULL,
  .Read = NULL,
  .Write = pipe_write,
  .Close = pipe_writer_close
};

int sys_Pipe(pipe_t* pipe)
{
	Fid_t fids[2];
	FCB * fcbs[2];
	
	/* get 2 from the available fids and fcbs of the process */
	if(!FCB_reserve(2,fids,fcbs))
		/* return error if there are not available fids or fcbs */
		return -1;
	
	/* initializing the control block responsible for the new pipe */
	pipe_cb * pipeCB = initialize_pipe_cb();

	/* assigning the available fids of the process to read and write fids of the pipe */
	pipe->read=fids[0];
	pipe->write=fids[1];
	/* assigning the available fcbs of the process to reader and writer fcbs of the pipe cb */
	pipeCB->reader=fcbs[0];
	pipeCB->writer=fcbs[1];

	/* assigning each streamobj of the available fcbs to the pipe cb */
	pipeCB->reader->streamobj=pipeCB;
	pipeCB->writer->streamobj=pipeCB;

	/* assigning the allowed operations for each end of the pipe to the fcbs responsible for every end */
	pipeCB->reader->streamfunc=&pipe_read_fops;
	pipeCB->writer->streamfunc=&pipe_write_fops;

	return 0;
}

