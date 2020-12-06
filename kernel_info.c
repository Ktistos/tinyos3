#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_proc.h"

typedef struct procinfo_cb
{
    procinfo pinfo;

    PCB* cursor;


}procinfo_cb;

/* Helper Functions */


procinfo_cb* initialize_procinfo_cb()
{
    procinfo_cb* pinfoCB = (procinfo_cb *) xmalloc(sizeof(procinfo_cb));

    pinfoCB->pinfo.pid=NOPROC;

    pinfoCB->pinfo.ppid=NOPROC;

    pinfoCB->pinfo.alive=0;

    pinfoCB->pinfo.thread_count=0;

    pinfoCB->pinfo.main_task=NULL;

    pinfoCB->pinfo.argl=0;

    pinfoCB->cursor=NULL;

    return pinfoCB;

}



void get_procinfo(PCB * proc,procinfo* pinfo)
{
    

    pinfo->pid=get_pid(proc);

    pinfo->ppid= get_pid(proc->parent);

    pinfo->alive= proc->pstate==ALIVE ;

    pinfo->thread_count= proc->thread_count;

    pinfo->main_task = proc->main_task;

    pinfo->argl = proc->argl;

    if(proc->args!=NULL)
    {   
        int size = proc->argl<=PROCINFO_MAX_ARGS_SIZE ? pinfo->argl : PROCINFO_MAX_ARGS_SIZE;
        memcpy(pinfo->args, proc->args, size);
    }
}



int procinfo_read(void * procinf, char* buf,uint size)
{
    procinfo_cb * procinfCB = (procinfo_cb *) procinf; 

    Pid_t curr_pid=get_pid(procinfCB->cursor);
    
    curr_pid++;

    while(get_pcb(curr_pid)==NULL && curr_pid<MAX_PROC-1)
        curr_pid++;
    
    if (curr_pid>MAX_PROC-1 || get_pcb(curr_pid)==NULL)
        return 0;

    procinfCB->cursor=get_pcb(curr_pid);
    
    get_procinfo(procinfCB->cursor,&procinfCB->pinfo);

    size =  size<=sizeof(procinfCB->pinfo) ? size : sizeof(procinfCB->pinfo);
    
    memcpy(buf, (char *)& procinfCB->pinfo, size);

    return size;
}


int procinfo_close(void* procinf)
{
    procinfo_cb * procinfCB = (procinfo_cb *) procinf; 

    free(procinfCB);

    return 0;
}


static file_ops procinfo_fops = {
  .Open = NULL,
  .Read = procinfo_read,
  .Write = NULL,
  .Close = procinfo_close
};


Fid_t sys_OpenInfo()
{
    Fid_t fid;
	FCB * fcb;

    if(!FCB_reserve(1,&fid,&fcb))
		/* return error if there are not available fids or fcbs */
		return NOFILE;
     
    procinfo_cb * pinfoCB = initialize_procinfo_cb();

    fcb->streamobj=pinfoCB;

    fcb->streamfunc=&procinfo_fops;

    return fid;

}