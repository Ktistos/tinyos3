#include "kernel_cc.h"
#include "kernel_streams.h"

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
	
	/* pipe buffer */
	char BUFFER[PIPE_BUFFER_SIZE];

	/* number of elements in buffer */
	int nelem;


} pipe_cb;


/* Pipe functionality operations */

/* 	This function is used by sys_Write when the user writes to an fid corresponding to a pipe.
*	It tries to write the buf string to the pipe.
*	
*	Returns the number of characters successfully read from the pipe or -1 the read end of the pipe has been closed
*/
int  pipe_write(void * pipecb_t, const char *buf, uint n);


/* 	This function is used by sys_Read when the user writes to an fid corresponding to a pipe.
*	It tries to read from the pipe and store the characters to buf argument.
*	
*	It returns the number of characters successfully read from the pipe.
*/
int  pipe_read(void * pipecb_t, char *buf, uint n);


/* 	This function is used by FCB_decref when the user calls sys_Close 
*	and the fid for the specific FCB (which corresponds to the write end of the pipe).
*	
*	It wakes up every thread that was waiting for characters to be written on the pipe 
*	since the write end closes and nobody can write on the pipe.
*	
*	If the read end of the pipe has been already closed then free the pipe control block.
*/
int  pipe_writer_close(void * _pipecb);

/* 	This function is used by FCB_decref when the user calls sys_Close 
*	and the fid for the specific FCB (which corresponds to the read end of the pipe).
*	
*	It wakes up every thread that were waiting for the pipe to free up some space so they could write it 
*	since the read end closes and nobody can read from the pipe.
*	
*	If the write end of the pipe has been already closed then free the pipe control block.
*/
int  pipe_reader_close(void * _pipecb);



/* This function is used to allocate memory space for the 
*  new pipe cb and to initialize its attributes.
*  It returns a pointer to the newly made and initialized pipe cb.
*/
pipe_cb * initialize_pipe_cb();