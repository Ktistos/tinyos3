#include "kernel_threads.h"
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "util.h"
#include "kernel_cc.h"
#include "kernel_streams.h"


#define PTCB_SIZE (sizeof(PTCB))


int check_valid_Tid(PTCB* ptcb)
{
    PCB* curproc=CURPROC;
    return rlist_find(&curproc->ptcb_list,ptcb,NULL)!=NULL;
}



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

  // Copy the arguments to new storage
  ptcb->argl=argl;
  if(args!=NULL) {
    ptcb->args = xmalloc(sizeof(args));
    ptcb->args=args;

  }
  else
    ptcb->args=NULL;

  return ptcb;
}
/**
 * This function frees up the memory allocated for the ptcb block and its arguments
 */
void release_PTCB(PTCB* ptcb)
{
  rlist_remove(&ptcb->ptcb_list_node);
  if(ptcb->args) {
      //free(ptcb->args);
      ptcb->args = NULL;
    }
  free(ptcb);
}

/**
 * This function frees up from the pcb all its ptcbs
 */
void clean_process_PTCBs()
{
  PCB* curproc=CURPROC;
  while (!is_rlist_empty(& curproc->ptcb_list))
  {
    rlnode* ptcb_node = rlist_pop_front(& curproc->ptcb_list);
    release_PTCB(ptcb_node->ptcb);
  }

}



void process_cleanup()
{


  PCB *curproc = CURPROC; 
  //clean up ptcbs
    clean_process_PTCBs();

  /* Do all the other cleanup we want here, close files etc. */
  if(curproc->args) {
    free(curproc->args);
    curproc->args = NULL;
  }

  /* Clean up FIDT */
  for(int i=0;i<MAX_FILEID;i++) {
    if(curproc->FIDT[i] != NULL) {
      FCB_decref(curproc->FIDT[i]);
      curproc->FIDT[i] = NULL;
    }
  }

  /* Reparent any children of the exiting process to the 
    initial task */
  PCB* initpcb = get_pcb(1);
  while(!is_rlist_empty(& curproc->children_list)) {
    rlnode* child = rlist_pop_front(& curproc->children_list);
    child->pcb->parent = initpcb;
    rlist_push_front(& initpcb->children_list, child);
  }

  /* Add exited children to the initial task's exited list 
    and signal the initial task */
  if(!is_rlist_empty(& curproc->exited_list)) {
    rlist_append(& initpcb->exited_list, &curproc->exited_list);
    kernel_broadcast(& initpcb->child_exit);
  }

  /* Put me into my parent's exited list */
  if(curproc->parent != NULL) {   /* Maybe this is init */
    rlist_push_front(& curproc->parent->exited_list, &curproc->exited_node);
    kernel_broadcast(& curproc->parent->child_exit);

  }
  
  /* Disconnect my main_thread */
  curproc->main_thread = NULL;
  /* Now, mark the process as exited. */
  curproc->pstate = ZOMBIE;

}

void start_thread()
{

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

    return (Tid_t) tcb->ptcb;

 }

return NOTHREAD;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) CURTHREAD->ptcb;
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
  PTCB* ptcb=(PTCB*) tid;

  /**Checking all the possible errors
  *- there is no thread with the given tid in this process.
  *- the tid corresponds to the current thread.
  *- the tid corresponds to a detached thread. 
  */
  if(!check_valid_Tid(ptcb)||(sys_ThreadSelf()==tid)||ptcb->detached)
    return -1;
  /**
   * The loop will break when the threads that called kernel_wait
   * get broadcasted either by ThreadExit or ThreadDetach.
   * 
   */
  while (!ptcb->exited && !ptcb->detached){
    //incrementing the number of threads that wait the specified tid
    ptcb->refcount++;
    kernel_wait(&ptcb->exit_cv, SCHED_USER);
    //decrementing that number since a thread wakes up
    ptcb->refcount--;
  }
  /**
   * Returning error code if the thread woke up by ThreadDetach since the thread 
   * that it had to join is now detached. 
   */
  if(ptcb->detached!=1){
    //retrieve the exitval of the exited thread 
    if(exitval!=NULL)
      *exitval=ptcb->exitval;
    /*If the newly awoken thread is the last or the only one 
    * that was waiting for the tid to exit,then free then release its ptcb.
    */
    if(ptcb->refcount<1)
        release_PTCB(ptcb);
    }
  else
    return -1;

  return 0;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
  PTCB* ptcb=(PTCB*) tid;
  //check whether tid exits in pcb list of ptcbs or ptcb is exited
	if(!check_valid_Tid(ptcb)||ptcb->exited)
    return -1;

  //flag ptcb as detached
  ptcb->detached=1;
  //reset the refcount since no thread is going to join the detached one
  ptcb->refcount=0;

  //wakeup all the threads that might have joined this thread before it became detached
  kernel_broadcast(&ptcb->exit_cv);

  return 0;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{

  TCB* curthread=CURTHREAD;

  curthread->ptcb->exitval=exitval;

  curthread->ptcb->exited=1;

  kernel_broadcast(&curthread->ptcb->exit_cv);
  
  if(curthread->ptcb->detached)
    release_PTCB(curthread->ptcb);

  //condition start
   /* cache for efficiency */

  PCB* curproc=CURPROC;

  if(curproc->thread_count<=1)
    process_cleanup();
    
  /* Bye-bye cruel world */
  
  curproc->thread_count--;
  kernel_sleep(EXITED, SCHED_USER);
}

