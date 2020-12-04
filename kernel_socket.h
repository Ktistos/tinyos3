#include "util.h"
#include "kernel_pipe.h"
#include "tinyos.h"


/**
  @brief Designate different origins of scheduler invocation.

  This is used in the scheduler heuristics to determine how to
  adjust the dynamic priority of the current thread.
 */

typedef struct socket_control_block SCB;

typedef enum SOCKET_TYPE {
	SOCKET_LISTENER,
    SOCKET_UNBOUND,
    SOCKET_PEER
} socket_type;


typedef struct listener_socket
{
    rlnode queue;

    CondVar req_available;
} listener_socket;


typedef struct peer_socket
{
    SCB* peer;

    pipe_cb* write_pipe;

    pipe_cb* read_pipe;

} peer_socket;

/* 
*	Pipe control block
*  	This structure holds all information pertaining to a pipe.
*/
struct socket_control_block
{
	uint refcount;
    
    /* File control block linked to the read end of the pipe */
	FCB * fcb; 

	socket_type type;

    port_t port;

	union 
    {
        listener_socket listener_s;
        peer_socket peer_s;
    };
    
};





