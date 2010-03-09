/****************************************************************************/
/*                                                                          */
/* 			     Module MEMORY                                  */
/* 			External Declarations 	                            */
/*                                                                          */
/****************************************************************************/

#include <stdio.h>
#include <assert.h>

/* OSP constants */

#define MAX_PAGE       16                 /* max size of page tables        */
#define MAX_FRAME      32                 /* size of the physical memory    */
#define PAGE_SIZE      512                /* size of a page in bytes        */

#define   COST_OF_PAGE_TRANSFER      6  /* cost of reading page  from drum  */


/* external enumeration constants */

typedef enum {
    false, true                         /* the boolean data type            */
} BOOL;

typedef enum {
    read, write                         /* type of actions for I/O requests */
} IO_ACTION;

typedef enum {
    load, store                         /* types of memory reference        */
} REFER_ACTION;

typedef enum {
    running, ready, waiting, done       /* types of status                  */
} STATUS;

typedef enum {
    iosvc, devint,                      /* types of interrupt               */
    pagefault, startsvc,
    termsvc, killsvc,
    waitsvc, sigsvc, timeint
} INT_TYPE;



/* external type definitions */

typedef struct page_entry_node PAGE_ENTRY;
typedef struct page_tbl_node PAGE_TBL;
typedef struct event_node EVENT;
typedef struct ofile_node OFILE;
typedef struct pcb_node PCB;
typedef struct iorb_node IORB;
typedef struct int_vector_node INT_VECTOR;
typedef struct frame_node FRAME;



/* external data structures */

struct frame_node {
    BOOL   free;        /* = true, if free                                  */
    PCB    *pcb;        /* process to which the frame currently belongs     */
    int    page_id;     /* vitrual page id - an index to the PCB's page tbl */
    BOOL   dirty;       /* indicates if the frame has been modified         */
    int    lock_count;  /* number of locks set on page involved in an       */
                        /* active I/O                                       */
    int    *hook;       /* can hook up anything here                        */
};

struct page_entry_node {
    int    frame_id;    /* frame id holding this page                       */
    BOOL   valid;       /* page in main memory : valid = true; not : false  */
    BOOL   ref;         /* set to true every time page is referenced AD     */
    int    *hook;       /* can hook up anything here                        */
};

struct page_tbl_node {
    PCB    *pcb;        /* PCB of the process in question                   */
    PAGE_ENTRY page_entry[MAX_PAGE];
    int    *hook;       /* can hook up anything here                        */
};

struct pcb_node {
    int    pcb_id;         /* PCB id                                        */
    int    size;           /* process size in bytes; assigned by SIMCORE    */
    int    creation_time;  /* assigned by SIMCORE                           */
    int    last_dispatch;  /* last time the process was dispatched          */
    int    last_cpuburst;  /* length of the previous cpu burst              */
    int    accumulated_cpu;/* accumulated CPU time                          */
    PAGE_TBL *page_tbl;    /* page table associated with the PCB            */
    STATUS status;         /* status of process                             */
    EVENT  *event;         /* event upon which process may be suspended     */
    int    priority;       /* user-defined priority; used for scheduling    */
    PCB    *next;          /* next pcb in whatever queue                    */
    PCB    *prev;          /* previous pcb in whatever queue                */
    int    *hook;          /* can hook up anything here                     */
};

struct iorb_node {
    int    iorb_id;     /* iorb id                                          */
    int    dev_id;      /* associated device; index into the device table   */
    IO_ACTION action;   /* read/write                                       */
    int    block_id;    /* block involved in the I/O                        */
    int    page_id;     /* buffer page in the main memory                   */
    PCB    *pcb;        /* PCB of the process that issued the request       */
    EVENT  *event;      /* event used to synchronize processes with I/O     */
    OFILE  *file;       /* associated entry in the open files table         */
    IORB   *next;       /* next iorb in the device queue                    */
    IORB   *prev;       /* previous iorb in the device queue                */
    int    *hook;       /* can hook up anything here                        */
};

struct int_vector_node {
    INT_TYPE cause;           /* cause of interrupt                         */
    PCB    *pcb;              /* PCB to be started (if startsvc) or pcb that*/
			      /* caused page fault (if fagefault interrupt) */
    int    page_id;           /* page causing pagefault                     */
    int    dev_id;            /* device causing devint                      */
    EVENT  *event;            /* event involved in waitsvc and sigsvc calls */
    IORB   *iorb;             /* IORB involved in iosvc call                */
};



/* extern variables */

extern INT_VECTOR Int_Vector;           /* interrupt vector         	     */
extern PAGE_TBL *PTBR;                  /* page table base register 	     */
extern FRAME Frame_Tbl[MAX_FRAME];      /* frame table              	     */
extern int Prepage_Degree;		/* global degree of prepaging (0-10) */



/* external routines */

extern siodrum(/* action, pcb, page_id, frame_id */);
/*  IO_ACTION   action;
    PCB         *pcb; 
    int         page_id, frame_id;  */
extern int get_clock();
extern gen_int_handler();


/****************************************************************************/
/*                                                                          */
/* 			     Module MEMORY                                  */
/* 			Internal Declarations 	                            */
/*                                                                          */
/****************************************************************************/

#define   PRIVATE         static
#define   PUBLIC
#define   TRUE            1
#define   FALSE           0
//#define   NULL            0	     /*  NULL pointer   */
#define   UNLOCKED        0
#define   MAX_SIZE      MAX_PAGE*PAGE_SIZE /* max size of a job allowed     */

#define   MIN_FREE        7
#define   LOTS_FREE       3

#define get_page_tbl(pcb)		pcb->page_tbl

#define lock_frame(frame_id)		Frame_Tbl[frame_id].lock_count++
#define unlock_frame(frame_id)		Frame_Tbl[frame_id].lock_count--
#define set_frame_dirty(frame_id)	Frame_Tbl[frame_id].dirty = true

// true when frame  is _not_ FREE and _not_ LOCKED
#define swapout_able(frame_id) (!(Frame_Tbl[frame_id].free) && Frame_Tbl[frame_id].lock_count == 0)

/* external variables */

static int trace = FALSE;	/* Internal trace flag */
static int clock_hand = 0;



void page_daemon();

/**************************************************************************/
/*									  */
/*			memory_init()					  */
/*									  */
/*   Description  : initialize the data structure in memory module        */
/*									  */
/*   called by    : SIMCORE module					  */
/*									  */
/**************************************************************************/
PUBLIC
memory_init()
{

}


/**************************************************************************/
/*									  */
/*			get_page					  */
/*									  */
/*	Description : To  transfer a page from drum to main memory        */
/*     									  */
/*      Called by   : PAGEINT module					  */
/*									  */
/*									  */
/**************************************************************************/

int n_free_frames = MAX_FRAME;

PUBLIC
get_page(pcb,page_id)
PCB   *pcb;
int    page_id;
{
	// 1) is there a frame f that is free? if so:
	// 2) update PCB->page_tbl[page_id]->frame_id = f.
	// 3) PCB->page_tbl[page_id]->valid = true

	// Find a free frame and put the requested page in there.

	if (n_free_frames < MIN_FREE) {
		// printf("Start page daemon in get page because n free: %d\n", n_free_frames);
		page_daemon(); 
	}

	int i;
	for (i = 0; i < MAX_FRAME;  i++) {
		// if the frame is free, give it to this process
		if (Frame_Tbl[i].free && Frame_Tbl[i].lock_count == 0) {
			// assert(Frame_Tbl[i].lock_count == 0);
			
			// actually read the page from the drum into the frame
			siodrum(read, pcb, page_id, i);

			/*
			printf(
				"get page: %d, pcb_id: %d, frame: %d, old page_id for this frame: %d\n",
				page_id,
				pcb->pcb_id,
				i,
				Frame_Tbl[i].page_id
			);
			*/
			// printf("Old page id was: %d\n", );
			
			
			Frame_Tbl[i].pcb = pcb;
			Frame_Tbl[i].page_id = page_id;
			
			
			// it is no longer free
			Frame_Tbl[i].free = false;

			// since it is newly brought in, it can not be dirty
			
			Frame_Tbl[i].dirty = false;

			// assign it to i
			pcb->page_tbl->page_entry[page_id].frame_id = i;

			// it is valid since it now resides in main memory
			pcb->page_tbl->page_entry[page_id].valid = true;
			pcb->page_tbl->page_entry[page_id].ref = false;
			
			n_free_frames--;
			// finished

			/* print_frame_tbl(); */
/* 			print_sim_frame_tbl(); */
			return;
		}
	}

	//assert(false);
	/* printf("random\n"); */
/* 	int freed = 0; */
/* 	int max_freed = 5; */
/* 	for (i = 0; i < MAX_FRAME && freed < max_freed;  i++) { */
/* 		if (swapout_able(i)) { */
/* 			// If the frame is dirty, swap it out of memory */
/* 			if (Frame_Tbl[i].dirty) { */
/* 				// writes the frame from memory to drum */
/* 				siodrum(write, pcb, page_id, i); */
/* 			} */
			

			
/* 			// it can no longer be dirty */
/* 			printf("Setting %d clean in random\n", i); */
/* 			Frame_Tbl[i].dirty = false; */
			
/* 			// the frame is now free */
/* 			Frame_Tbl[i].free = true; */
			
/* 			// update the page_tbl of the process that was holding the */
/* 			// flag, so that that process knows that its precious page */
/* 			// is now banned to the drum */
/* 			Frame_Tbl[i].pcb->page_tbl-> */
/* 				page_entry[Frame_Tbl[i].page_id].valid = false; */
/* 		} */
/* 	} */

/* 	if (n_free_frames < MIN_FREE) { */
/* 		printf("Start page daemon after random because n free: %d\n", n_free_frames); */
/* 		page_daemon(); */
/* 	} */
	
	// read it it?
	
	
	// 4) if less than x free frames exist:
	// 5) ...

	
	// find a page r in memory to replace with page_id
	// maybe there is a free page?
	// if this page r is dirty, save it to drum
	
}

void page_daemon() {
	// printf("> Page daemon\n");

	// print_sim_frame_tbl();
	
	int freed = LOTS_FREE;

	// int max_loops = 2*MAX_FRAME;
	// int i = 0;
	
	while (freed > 0) { //  && i < max_loops) {
		//assert(clock_hand >= 0 && clock_hand < MAX_FRAME);
		/*
		printf("checking frame %d, which is free: %d and lock %d\n",
			   clock_hand,
			   Frame_Tbl[clock_hand].free,
			   Frame_Tbl[clock_hand].lock_count
		);
		*/
		
		if ((!Frame_Tbl[clock_hand].free) && Frame_Tbl[clock_hand].lock_count == 0) {
			// printf("swappable\n");
			
			int page_id = Frame_Tbl[clock_hand].page_id;
			BOOL ref = Frame_Tbl[clock_hand].pcb->page_tbl->page_entry[page_id].ref; 

			// handle the ref bit
			if (ref) {
				Frame_Tbl[clock_hand].pcb->page_tbl->page_entry[page_id].ref = false;
			} else {
				// print_sim_frame_tbl();
				// If the frame is dirty, swap it out of memory
				if (Frame_Tbl[clock_hand].dirty) {
					// writes the frame from memory to drum
					siodrum(
						write,
						Frame_Tbl[clock_hand].pcb,
						Frame_Tbl[clock_hand].page_id,
						clock_hand
					);
				}
			
			
				// it can no longer be dirty
				// printf("Setting %d clean in daemon\n", clock_hand);
				Frame_Tbl[clock_hand].dirty = false;
				Frame_Tbl[clock_hand].page_id = -1;

				// the frame is now free
				Frame_Tbl[clock_hand].free = true;
				
				// update the page_tbl of the process that was holding the
				// flag, so that that process knows that its precious page
				// is now banned to the drum
				Frame_Tbl[clock_hand].pcb->page_tbl->page_entry[page_id].valid = false;
				Frame_Tbl[clock_hand].pcb->page_tbl->page_entry[page_id].ref = false; 
		
				Frame_Tbl[clock_hand].pcb = NULL;
				
				// we have freed another page for the glory of the
				// revolution.
				n_free_frames++;
				freed--;
				/* printf("FREEDOM\n"); */
/* 				print_sim_frame_tbl(); */
			}
		}

		// i++;
		clock_hand = (clock_hand + 1) % MAX_FRAME;
    }

	/* if (i > max_loops) { */
/* 		printf("quit due to max loops\n"); */
/* 	} */
	
		
	
	// printf("< Page daemon \n");
	
}
	

/**************************************************************************/
/*									  */
/*                  deallocate						  */
/*									  */
/*   Description : The job is history now so free the memory frames       */
/*                 occupied by the process.		                  */
/*                 Set the pcb to NULL                                    */
/*  								          */
/*   called by   : PROCSVC module                                         */
/*									  */
/**************************************************************************/
PUBLIC
deallocate(pcb)
PCB *pcb;
{
	// printf("> deallocate for %d\n", pcb->pcb_id);
	/* print_frame_tbl(pcb->page_tbl); */
/* 	print_sim_frame_tbl(pcb->page_tbl); */
/* 	print_page_tbl(pcb->page_tbl); */
	
	
		
	// clear the free flag
	int i;
	for (i = 0; i < MAX_PAGE;  i++) { 
		int frame_id = pcb->page_tbl->page_entry[i].frame_id;
		if (pcb->page_tbl->page_entry[i].valid) {
			/* printf( */
/* 				"clearing page %d associated with frame %d\n", */
/* 				i, frame_id); */
/* 			printf("Setting frame %d clean in deallocate for process %d\n", frame_id, pcb->pcb_id); */

			pcb->page_tbl->page_entry[i].valid = false;
			
			// do not know if needed
			Frame_Tbl[frame_id].dirty = false;
			// Frame_Tbl[frame_id].valid = ?;  //valid flag should be cleared
			Frame_Tbl[frame_id].free = true;
			Frame_Tbl[frame_id].pcb = NULL; 
 			Frame_Tbl[frame_id].page_id = -1; 

			n_free_frames++;
		}
		

		// doesnt matter if dirty
		// or if lock_count > 0?=????
	}
	/* print_frame_tbl(); */
/* 	print_sim_frame_tbl(); */
	// printf("< deallocate\n");
}
  

/************************************************************************/
/*									*/
/*		   prepage						*/
/*									*/
/*    Description : Swap the process specified in the argument from     */
/*                  drum/disk to main memory.                           */
/*	            Will  use the prepaging policy.                     */
/*   									*/
/*   called by    : CPU module						*/
/*									*/
/************************************************************************/
PUBLIC
prepage(pcb)
PCB *pcb;
{
	/* Not part of lab. Leave empty  */
}   
					 
/************************************************************************/
/*									*/
/*		   start_cost						*/
/*									*/
/*   called by    : CPU module						*/
/*									*/
/************************************************************************/
PUBLIC
int start_cost(pcb)
PCB *pcb;
{
	/* Not part of lab. Leave empty  */
}   

 
/***************************************************************************/
/*									   */
/*   			refer					           */
/*									   */
/*	Description : Called by SIMCORE module to simulate memory          */
/*		      referencing by processes.                            */
/*									   */
/*      Called by   : SIMCORE module					   */
/*									   */
/*	Call        : gen_int_handler()				           */
/*									  */
/*   You are not expected to change this routine                            */
/*									   */
/***************************************************************************/
PUBLIC
refer(logic_addr,action)
int          logic_addr;   /* logical address                              */
REFER_ACTION action;       /* a store operation will change memory content */
{
  int            job_size,
                 page_no,
                 frame_id;
  PAGE_TBL       *page_tbl_ptr;
  PCB            *pcb;
 
  if (PTBR != NULL)
    check_page_table(PTBR,1,"MEMORY.refer","","upon entering routine");

  pcb = PTBR->pcb;

  if (trace)
    printf("Hello refer(pcb: %d,logic_addr: %d,action: %d)\n",
					pcb->pcb_id,logic_addr,action);
  

  job_size = pcb->size;
  page_tbl_ptr = get_page_tbl(pcb);	/* macro */

  if (logic_addr < MAX_SIZE && logic_addr < job_size && logic_addr >= 0){

       page_no = logic_addr / PAGE_SIZE;

       if (page_tbl_ptr->page_entry[page_no].valid == false) {
          /* page fault                                             */
          /* set interrupt vector Int_Vector to indicate page fault */
          /* call interrupt handler                                 */
       
          Int_Vector.pcb = pcb;
          Int_Vector.page_id = page_no;
          Int_Vector.cause   = pagefault;
          gen_int_handler();
       }

       page_tbl_ptr->page_entry[page_no].ref = true;

       if (( page_tbl_ptr->page_entry[page_no].valid == true) &&
                 (action == store)) {

         frame_id = page_tbl_ptr->page_entry[page_no].frame_id;
         set_frame_dirty(frame_id);	/* macro */

       }

  }
  else {
    printf("CLOCK> %6d#*** ERROR: MEMORY.refer>\n\t\tPCB %d: Invalid address passed to refer(%d, ...)\n\n\n",
				get_clock(),pcb->pcb_id,logic_addr);
    print_sim_frame_tbl();
    osp_abort();
  }

} 


/*************************************************************************/
/*									 */
/*			lock_page					 */
/*									 */
/*	Description:   To lock the chunk of memory mentioned in iorb     */
/*		       to protect it from being swapped out.             */
/*									 */
/*	Called by  :   DEVICES module					 */
/*                                                                       */
/*      call       :   gen_int_handler in INTER module                   */
/*                     lock_frame                                        */
/*									  */
/*   You are not expected to change this routine                            */
/*									 */
/*************************************************************************/
PUBLIC
lock_page(iorb)
IORB *iorb;
{
   int           page_id ;
   int           frame_id;
   PAGE_TBL      *page_tbl_ptr;

    if (trace)
        printf("Hello lock_page(iorb). The pcb is %d\n",iorb->pcb->pcb_id);

    check_iorb(iorb,1,"MEMORY.lock_page","","upon entering routine");

    page_tbl_ptr = get_page_tbl(iorb->pcb);	/* macro */
    page_id      =  iorb->page_id;

    if (page_tbl_ptr->page_entry[page_id].valid == false) {
       
          Int_Vector.pcb = iorb->pcb;
          Int_Vector.page_id = page_id;
          Int_Vector.cause   = pagefault;
          gen_int_handler();
    }

    frame_id = page_tbl_ptr->page_entry[page_id].frame_id;


    if (iorb->action == read)
        set_frame_dirty(frame_id);	/* macro */
    lock_frame(frame_id);	/* macro */
 } 


/*************************************************************************/
/*                                                                       */
/*              unlock_page()                                            */
/*                                                                       */
/*          description : Unlocked the page which has finished I/O       */
/*                                                                       */
/*          Called by   : DEVICES module                                 */
/*                                                                       */
/*          Call        : unlock_frame                                   */
/*									  */
/*   You are not expected to change this routine                            */
/*                                                                       */
/*************************************************************************/
PUBLIC
unlock_page(iorb)
IORB *iorb;
{
   int              page_id ;
   int              frame_id;
   PAGE_TBL        *page_tbl_ptr;

   if (trace)
       printf("Hello unlock_page(iorb). The pcb is %d\n",iorb->pcb->pcb_id);

   check_iorb(iorb,1,"MEMORY.unlock_page","","upon entering routine");

   page_tbl_ptr = get_page_tbl(iorb->pcb);	/* macro */
   page_id      = iorb->page_id;
   frame_id = page_tbl_ptr->page_entry[page_id].frame_id;


   unlock_frame(frame_id);	/* macro */

} 

/****************************************************************************/
/*									    */
/*			print_page_tbl					    */
/*									    */
/* 	Description : Print the page table of a process.		    */
/*                    For debugging purpose                                 */
/*									    */
/****************************************************************************/


print_page_tbl(page_tbl_ptr)
PAGE_TBL   *page_tbl_ptr;
{
  int i;

  printf("\n\n");
  for (i=0; i<MAX_PAGE; i++)
        printf("pg=%d    valid=%d  ref=%d  frame=%d\n",
                i, page_tbl_ptr->page_entry[i].valid,
                page_tbl_ptr->page_entry[i].ref,
                page_tbl_ptr->page_entry[i].frame_id);

  printf("\n\n");
}
