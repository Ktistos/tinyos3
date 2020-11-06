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



/**
* Initiliazing and returning a new ptcb.
* Not responsible for the linking between pcb,tcb and ptcb blocks
 */
PTCB* initialize_PTCB(Task call, int argl, void * args );

/**
 * Unlinking ptcb from the ptcb_list of the process,decrementing the number of threads which belong to the process
 * and freeing up the memory allocated for the arguments and the ptcb itself
*/
void release_PTCB(PTCB* ptcb);

/**
 * Checking whether the given tid belongs to the PCB list of PTCBs
 */
int check_valid_Tid(PTCB* ptcb);
