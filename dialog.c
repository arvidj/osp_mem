/****************************************************************************/
/*                                                                          */
/* DIALOG                                                                   */
/*                                                                          */
/* file: dialog.c                                                           */
/*                                                                          */
/* description:                                                             */
/* DIALOG provides a run-time interface to OSP; it allows the user to       */
/* change simulation parameters for debugging purposes; any user may        */
/* provide his own copy of this routine, provided he knows what he is doing */
/*                                                                          */
/****************************************************************************/

extern change_sim_params();

/* these routines show what the simulator thinks should be the case */

extern print_sim_dev_tbl();
extern print_sim_frame_tbl();
extern print_sim_pcb_tbl();
extern print_sim_rsrc_tbl();
extern print_sim_open_files_tbl();
extern print_sim_disk_map();
extern print_sim_waiting_queue();
extern print_sim_prrb_queue();
extern print_sim_socket_tbl();

/* these routines print YOUR tables, not simcore's; you can use these
	routines instead of writing your own ones.    		     */

extern print_dev_tbl();
extern print_open_files_tbl();
extern print_frame_tbl();
extern print_rsrc_tbl();
extern print_pcb_tbl();
extern print_prrb_queue();
extern print_socket_tbl();


at_snapshot()
{
    change_sim_params();

/*  this routine is called by SIMCORE right after each snapshot is printed  */
/*  you can modify this routine to do whatever you want at the snapshot time*/

}



at_warning()
{

/*  this routine is called by SIMCORE right after each warning is printed   */
/*  you can insert various print statements here to find out the status of  */
/*  variables at this point. Particularly useful may be the routines        */
/*  provided by SIMCORE to print the correct state of OSP data structures:  */
/* 	print_sim_frame_tbl()						    */
/* 	print_sim_dev_tbl()					     	    */
/* 	print_sim_pcb_tbl()					     	    */
/* 	print_sim_rsrc_tbl()					     	    */
/* 	print_sim_disk_map()					     	    */
/* 	print_sim_waiting_queue()				     	    */
/*	print_sim_prrb_queue();						    */
/*	print_sim_socket_tbl();						    */
/*  some of these routines are called by SIMCORE automatically when an      */
/*  appropriate warning is issued                                           */

}



at_error()
{

/*  this routine is called by SIMCORE right after an error is printed       */
/*  but before the system is aborted					    */
/*  you can insert various print statements here to find out the status of  */
/*  variables at this point. Particularly useful may be the routines        */
/*  provided by SIMCORE to print the correct state of OSP data structures:  */
/* 	print_sim_frame_tbl()						    */
/* 	print_sim_dev_tbl()					     	    */
/* 	print_sim_pcb_tbl()					     	    */
/* 	print_sim_rsrc_tbl()					     	    */
/* 	print_sim_disk_map()					     	    */
/* 	print_sim_waiting_queue()				     	    */
/*	print_sim_prrb_queue();						    */
/*	print_sim_socket_tbl();						    */
/*  some of these routines are called by SIMCORE automatically when an      */
/*  appropriate error is detected                                           */

}
