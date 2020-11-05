#include "kernel_threads.h"
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "util.h"


#define PTCB_SIZE (sizeof(PTCB))

PTCB* initialize_PTCB(Task call, int argl, void * args )
{
  //allocate size for the new ptcb
  PTCB* ptcb=(PTCB*) xmalloc(PTCB_SIZE);
  
  //initializing the attributes of ptcb
  ptcb->tcb=NULL;
  ptcb->task=call;
  ptcb->exit_cv=COND_INIT;
  rlnode_init(&ptcb->ptcb_list_node,ptcb);

  ptcb->exited=0;
  ptcb->detached=0;
  ptcb->refcount=0;

  ptcb->argl=argl;
  if(args!=NULL) {
    ptcb->args = malloc(argl);
    memcpy(ptcb->args, args, argl);
  }
  else
    ptcb->args=NULL;

  return ptcb;
}

void start_thread(){

  int thread_exitval;

  Task call =  CURTHREAD->ptcb->task;
  int argl = CURTHREAD->ptcb->argl;
  void* args = CURTHREAD->ptcb->args;

  thread_exitval = call(argl,args);
  sys_ThreadExit(thread_exitval);
}

/** 
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{
 if(task != NULL) {
    PCB* curproc=CURPROC;
    TCB* tcb = spawn_thread(curproc, start_thread);
  
    //aquiring the newly made ptcb 
    PTCB* ptcb=initialize_PTCB(task,argl,args);

    //linking ptcb with tcb
    ptcb->tcb=tcb;
    tcb->ptcb =ptcb;

    //linking ptcb to pcb by pushing it in the list of ptcbs of the current process
    rlist_push_front(&curproc->ptcb_list, &ptcb->ptcb_list_node);
    curproc->thread_count++;

    wakeup(tcb);

    return (Tid_t) ptcb;

 }

return NOTHREAD;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) CURTHREAD;
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
	return -1;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
	return -1;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{

}

