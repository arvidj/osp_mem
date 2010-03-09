

/****************************************************************************/
/*                                                                          */
/* 			     Module PAGEINT                                 */
/*			External Declarations                               */
/*                                                                          */
/****************************************************************************/



/* external define constants */

#define MAX_PAGE       16                 /* max size of page tables        */



/* external enumeration constants */

typedef enum {
    false, true                         /* the boolean data type            */
} BOOL;

typedef enum {
    running, ready, waiting, done       /* types of status                  */
} STATUS;



/* external type definitions */

typedef struct pcb_node PCB;
typedef struct page_entry_node PAGE_ENTRY;
typedef struct page_tbl_node PAGE_TBL;
typedef struct event_node EVENT;



/* external data structures */

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



/* external variables */

extern PAGE_TBL *PTBR;				/* page table base register */


/* external routines */

extern get_page(/* pcb, page_id */);
/*  PCB    *pcb;
    int    page_id; */





/****************************************************************************/
/*                                                                          */
/*                                                                          */
/*                              Module PAGEINT                              */
/*                             Internal Routines                            */
/*                                                                          */
/*                                                                          */
/****************************************************************************/





void pagefault_handler(pcb, page_id)
PCB *pcb;
int page_id;
{
	get_page(pcb, page_id);
}

/* end of module */
