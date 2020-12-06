
#include "tinyos.h"

#include "kernel_proc.h"

#include "kernel_socket.h"




SCB* PORT_MAP[MAX_PORT+1];


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

	socket->refcount=0;

	socket->fcb=NULL;

	socket->type=SOCKET_UNBOUND;

	socket->port=NOPORT;

	return socket;
}

request * initialize_request()
{
	request* req= (request*) xmalloc(sizeof(request));

	req->admitted=0;

	req->peer=NULL;

	req->connected_cv=COND_INIT;
	
	rlnode_init(&req->queue_node,req);

	return req;
}

int check_legal_fid(Fid_t fid){

	return (fid>=0 && fid<=MAX_FILEID-1 && CURPROC->FIDT[fid]!=NULL && CURPROC->FIDT[fid]->streamobj!=NULL); 
}

int check_legal_port(port_t port)
{
	return !(port<NOPORT || port>MAX_PORT);
}

void SCB_incref(SCB* scb)
{
	scb->refcount++;
}

void SCB_decref(SCB* scb)
{
	scb->refcount--;
	if(scb->refcount==0)
		free(scb);
}


void connect_peers(SCB* socket1,SCB* socket2)
{
	socket1->type=SOCKET_PEER;
	socket2->type=SOCKET_PEER;

	peer_socket * peer1=&socket1->peer_s;
	peer_socket * peer2=&socket2->peer_s;

	pipe_cb * pipe1=initialize_pipe_cb();
	pipe_cb * pipe2=initialize_pipe_cb();

	peer1->peer=socket2;
	peer2->peer=socket1;

	peer1->read_pipe=pipe1;
	peer2->write_pipe=pipe1;

	peer2->read_pipe=pipe2;
	peer1->write_pipe=pipe2;

	pipe1->reader=socket1->fcb;
	pipe1->writer=socket2->fcb;

	pipe2->reader=socket2->fcb;
	pipe2->writer=socket1->fcb;

}


int socket_write(void * scb, const char *buf, uint n)
{
	SCB* socket= (SCB*) scb;
	
	return socket->type==SOCKET_PEER && socket->peer_s.write_pipe->writer ? pipe_write( socket->peer_s.write_pipe,buf,n) : -1;
}

int socket_read(void * scb, char *buf, uint n)
{
	SCB* socket= (SCB*) scb;

	return socket->type==SOCKET_PEER && socket->peer_s.read_pipe->reader ? pipe_read( socket->peer_s.read_pipe,buf,n) : -1;
}

int socket_close(void* scb )
{
	SCB* socket= (SCB*) scb;

	socket->fcb=NULL;

	if (socket->type==SOCKET_LISTENER)
	{
		listener_socket* listener = &socket->listener_s;

		PORT_MAP[socket->port]=NULL;
		kernel_broadcast(&socket->listener_s.req_available);
		//signal everybody else
		while (!is_rlist_empty(& listener->queue))
			rlist_pop_front(& listener->queue);
	}
	else if(socket->type==SOCKET_PEER)
	{
		peer_socket* psocket= &socket->peer_s;
		//check these conditions
		if(psocket->write_pipe->writer!=NULL )
			pipe_writer_close(psocket->write_pipe);
		//check 
		if(psocket->read_pipe->reader!=NULL )
			pipe_reader_close(psocket->read_pipe);
	
	}
	
	SCB_decref(socket);

	return 0;
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

	if( !check_legal_port(port) || !FCB_reserve(1,&fid,&fcb))
		/* return error if there are not available fids or fcbs */
		return NOFILE;

	SCB* socket = initialize_SCB();

	socket->fcb=fcb;

	socket->fcb->streamobj=socket;

	SCB_incref(socket);

	socket->fcb->streamfunc=&socket_fops;

	socket->port=port;

	return fid;
}

int sys_Listen(Fid_t sock)
{
	if (!check_legal_fid(sock))
		return -1;
	
	SCB* socket =(SCB*) CURPROC->FIDT[sock]->streamobj; 

	int port=socket->port;

	if(port == NOPORT || socket->type!=SOCKET_UNBOUND || PORT_MAP[socket->port]!=NULL)
		return -1;

	socket->type=SOCKET_LISTENER;

	socket->listener_s.req_available=COND_INIT;

	rlnode_init(& socket->listener_s.queue, NULL);
	
	PORT_MAP[port]=socket;
	
	return 0;
}


Fid_t sys_Accept(Fid_t lsock)
{
	Fid_t retval=NOFILE;

	if (!check_legal_fid(lsock) )
		return retval;

	SCB* lsocket =(SCB*) CURPROC->FIDT[lsock]->streamobj; 

	if(lsocket->type!=SOCKET_LISTENER)
		return retval;
	
	SCB_incref(lsocket);

	listener_socket* listener= &lsocket->listener_s;

	if (is_rlist_empty(&listener->queue))
		kernel_wait(&listener->req_available,SCHED_USER);
	
	if (lsocket->fcb)
	{
		request * req= rlist_pop_front(&listener->queue)->req;
		Fid_t sock_fid=sys_Socket(NOPORT);
		if (sock_fid==NOFILE)
			return retval;

		req->admitted=1;

		SCB* psocket =(SCB*) CURPROC->FIDT[sock_fid]->streamobj; 

		connect_peers(psocket,req->peer);

		kernel_broadcast(&req->connected_cv);

		retval=sock_fid;
	} 
	
	SCB_decref(lsocket);

	return retval;
}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	if (!check_legal_fid(sock) || !check_legal_port(port) || !PORT_MAP[port] )
		return -1;

	SCB* socket =(SCB*) CURPROC->FIDT[sock]->streamobj; 
	
	if(socket->type!=SOCKET_UNBOUND)
		return -1;

	SCB_incref(socket);

	request* req= initialize_request();

	req->peer = socket; 

	listener_socket* listener=& PORT_MAP[port]->listener_s;

	rlist_push_back(&listener->queue,&req->queue_node);

	kernel_broadcast(&listener->req_available);

	kernel_timedwait(&req->connected_cv,SCHED_USER,timeout);

	int isAdmitted=req->admitted;

	free(req);

	SCB_decref(socket);

	return isAdmitted-1 ;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	int retval=-1;

	if (!check_legal_fid(sock) )
		return retval;

	SCB* socket =(SCB*) CURPROC->FIDT[sock]->streamobj; 
	if (socket->type!= SOCKET_PEER)
		return retval;
	

	switch (how)
	{
	case SHUTDOWN_READ:
		
		retval= pipe_reader_close(socket->peer_s.read_pipe);

		break;
	case SHUTDOWN_WRITE:
		retval = pipe_writer_close(socket->peer_s.write_pipe);

		break;
	case SHUTDOWN_BOTH:
		retval = pipe_writer_close(socket->peer_s.write_pipe);
		retval= pipe_reader_close(socket->peer_s.read_pipe);
		break;

	default:
		break;
	}
	return retval;
}

