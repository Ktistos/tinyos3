#include "kernel_proc.h"
#include "kernel_sched.h"
#include "tinyos.h"




typedef struct process_thread_control_block {
	
    TCB* tcb;

    Task task;

    int argl;

    void * args;

    int exitval;

    int exited;

    int detached;

    CondVar exit_cv;
      
    int refcount;

    rlnode ptcb_list_node;

} PTCB;




