
#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_dev.h"
#include "kernel_cc.h"
#include "kernel_pipe.h"


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
	pipeCB->nelem=0;

	return pipeCB;

}
/* This function is used to increment the read and write position in the of the pipe buffer
*  It is used in the write_to_buffer and read_from_buffer functions.
*/
void increm_pos(int* position) 
{
	*position=(*position+1)%PIPE_BUFFER_SIZE;
}

/*
*	This function is used to write characters into the pipe buffer
*	from the buf string.
*	
*	It returns the number of characters succesfully written to the buffer.
*	
*	If the pipe buffer gets full before all characters of the buf string are written to 
*	it stops and returns the number of sucessfully written chars.
*/
int write_to_buffer(pipe_cb * pipeCB, const char *buf, uint n)
{
	/* accessing pipe buffer and write position of the buffer from the pipe control block */
	char* pipe_buffer=pipeCB->BUFFER;
	int* w_pos=&pipeCB->w_position;

	int count;
	for(count=0; count<n; count++){
		/* if the pipe buffer gets full stop the writting*/
		if(pipeCB->nelem==PIPE_BUFFER_SIZE)
			break;

		pipe_buffer[*w_pos] = buf[count];
		/* increment the write position */
		increm_pos(w_pos);
		/* increase the number of elements in the buffer */
		pipeCB->nelem++;

	}
	/* returning the number of successfully written chars */
	return count;
}

/*
*	This function is used to read characters from the pipe buffer
*	and to store them to the buf string.
*	
*	It returns the number of characters succesfully read from the pipe.
*/
int read_from_buffer(pipe_cb* pipeCB, char *buf, uint n)
{
	/* accessing pipe buffer and read position of the buffer from the pipe control block */
	char* pipe_buffer=pipeCB->BUFFER;
	int* r_pos=&pipeCB->r_position;
	
	int count;
	for(count=0; count<n; count++){
		/* if the pipe buffer gets empty stop reading from it*/
		if(pipeCB->nelem==0)
			break;

		buf[count]=pipe_buffer[*r_pos];
		/* increment the read position */
		increm_pos(r_pos);
		/* decrease the number of elements in the buffer */
		pipeCB->nelem--;
	}	

	/* returning the number of successfully read chars */
	return count;

}

int  pipe_write(void * pipecb_t, const char *buf, uint n)
{

	
	/* access the pipe control block */
	pipe_cb * pipeCB = (pipe_cb*) pipecb_t;

	if(pipeCB->writer==NULL)
		return -1;

	int count=0;
	/* 	If the pipe is full then wait until someone reads from the read end and free up some space
	*	Don't wait if the read end has been closed.
	*/
	while(pipeCB->nelem==PIPE_BUFFER_SIZE && pipeCB->reader)
		kernel_wait(&pipeCB->has_space, SCHED_USER);

	/* If the read end of the pipe is closed then return error -1 */
	if(!pipeCB->reader)
		return -1;

	/* Since we can write to buffer, write as many characters as possible from buf string to pipe */
	count=write_to_buffer(pipeCB, buf,n);

	/* wake up all the those who tried to read from the read end of the pipe it was empty. */
	kernel_broadcast(&pipeCB->has_data);

	/* return the number of characters successfully written in the pipe */
	return count;

}
	

int  pipe_writer_close(void * _pipecb)
{
	/* access the pipe control block */
	pipe_cb * pipeCB = (pipe_cb*) _pipecb;

	/* Since the write end closes make it NULL on the pipe control block*/
	pipeCB->writer=NULL;

	/* wake up the threads waiting for the pipe to get full */
	kernel_broadcast(&pipeCB->has_data);

	/* if the read end has already been closed then free the pipe control block */
	if(!pipeCB->reader)
		 free(pipeCB);

	return 0;
}


int  pipe_read(void * pipecb_t, char *buf, uint n)
{
	/* access the pipe control block */
	pipe_cb * pipeCB = (pipe_cb*) pipecb_t;
	
	if(pipeCB->reader==NULL)
		return -1;

	int count=0;

	/* If the pipe is empty then wait until someone writes to it */
	while(pipeCB->nelem==0 && pipeCB->writer)
		kernel_wait(&pipeCB->has_data, SCHED_USER);

	/* Since we can read from the pipe, read as many characters as possible from the pipe and store them to buf */
	count=read_from_buffer(pipeCB, buf,n);

	/* wake up all the those who tried to write to the pipe and it was full . */
	kernel_broadcast(&pipeCB->has_space);

	/* return the number of characters successfully written in the pipe */
	return count;
}


int  pipe_reader_close(void * _pipecb)
{
	/* access the pipe control block */
	pipe_cb * pipeCB = (pipe_cb*) _pipecb;

	/* Since the read end closes make it NULL on the pipe control block*/
	pipeCB->reader=NULL;

	/* wake up the threads waiting for the pipe to free some space */
	kernel_broadcast(&pipeCB->has_space);

	/* if the write end has already been closed then free the pipe control block */
	if(!pipeCB->writer)
		 free(pipeCB);

	return 0;
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

