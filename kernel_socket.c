
#include "tinyos.h"
#include "kernel_socket.h"
#include "kernel_proc.h"


SCB* PORT_MAP[MAX_PORT];

typedef struct connection_request
{
	int admitted;
	SCB* peer;

	CondVar connected_cv;
	rlnode queue_node;
}request;

/* Helper functions */

SCB * initialize_SCB()
{
	SCB * socket = (SCB *) xmalloc(sizeof(SCB));

	//to be changed
	socket->refcount=0;

	socket->fcb=NULL;

	socket->type=SOCKET_UNBOUND;

	socket->port=NOPORT;

	return socket;
}

int check_legal_fid(Fid_t fid){

	return !(fid<0 || fid>MAX_FILEID-1 || !CURPROC->FIDT[fid]); 
}



int socket_write(void * SCB, const char *buf, uint n)
{

}

int socket_read(void * SCB, char *buf, uint n)
{

}

int socket_close(void* SCB)
{

}


static file_ops socket_fops = {
  .Open = NULL,
  .Read = socket_read,
  .Write = socket_write,
  .Close = socket_close
};


Fid_t sys_Socket(port_t port)
{
	Fid_t fid;
	FCB * fcb;

	int illegal_port= port<NOPORT && port>MAX_PORT-1; 

	if(!FCB_reserve(1,fid,fcb) || illegal_port )
		/* return error if there are not available fids or fcbs */
		return NOFILE;

	SCB* socket = initialize_SCB();

	socket->fcb=fcb;

	socket->fcb->streamobj=socket;

	socket->fcb->streamfunc=&socket_fops;

	socket->port=port;

	return fid;
}

int sys_Listen(Fid_t sock)
{
	if (!check_legal_fid(sock))
		return -1;
	
	FCB* fcb= CURPROC->FIDT[sock]; 

	SCB* socket = (SCB*) fcb->streamobj;

	int port=socket->port;

	if(port == NOPORT || socket->type!=SOCKET_UNBOUND)
		return -1;

	socket->type=SOCKET_LISTENER;

	socket->listener_s.req_available=COND_INIT;

	rlnode_init(& socket->listener_s.queue, NULL);
	
	PORT_MAP[port]=socket;
	
	return 0;
}


Fid_t sys_Accept(Fid_t lsock)
{

	if (!check_legal_fid(lsock) )
		return -1;

	FCB* fcb= CURPROC->FIDT[lsock]; 

	SCB* socket = (SCB*) fcb->streamobj;

	if(socket->type!=SOCKET_LISTENER)
		return -1;
	

	return NOFILE;
}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	return -1;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	return -1;
}

