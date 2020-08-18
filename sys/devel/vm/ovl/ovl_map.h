/*
 * ovl_map.h
 *
 *  Created on: 25 Apr 2020
 *      Author: marti
 */

#ifndef	OVL_MAP_
#define	OVL_MAP_

#include "ovl.h"

struct ovl_map_clist;
struct ovl_map_rb_tree;
RB_PROTOTYPE(ovl_map_rb_tree, ovl_map_entry, ovl_rb_entry, ovl_rb_compare);

union ovl_map_object {
	struct ovl_object					*ovl_object;		/* overlay object */
	//vm_object	/* vm_object */
	//avm_object /* avm_object */
};

struct ovl_map_entry {
  CIRCLEQ_ENTRY(ovl_map_entry)  		ovl_cl_entry;		/* entries in a circular list */
  RB_ENTRY(ovl_map_entry) 				ovl_rb_entry;		/* tree information */
  vm_offset_t		                	ovle_start;			/* start address */
  vm_offset_t		                	ovle_end;			/* end address */
  caddr_t								ovle_ownspace;		/* free space after */
  caddr_t								ovle_space;			/* space in subtree */
  union ovl_map_object			   		ovle_object;		/* object I point to */
  vm_offset_t				          	ovle_offset;		/* offset into object */

  vm_prot_t								ovle_protection;	/* protection code */
  vm_prot_t								ovle_max_protection;/* maximum protection */
  vm_inherit_t							ovle_inheritance;	/* inheritance */
};

CIRCLEQ_HEAD(ovl_map_clist, ovl_map_entry);
RB_HEAD(ovl_map_rb_tree, ovl_map_entry);
struct ovl_map {
	struct ovl_map_clist         		ovl_header;        	/* Circular List of entries */
	struct ovl_map_rb_tree				ovl_root;			/* Tree of entries */
	struct pmap *		           		ovl_pmap;		    /* Physical map */
    lock_data_t		                    ovl_lock;		    /* Lock for overlay data */
    int			                        ovl_nentries;	    /* Number of entries */
    vm_size_t		                    ovl_size;		    /* virtual size */
    boolean_t							ovl_is_vm_map;		/* Am I a vm map? */
    int			                        ovl_ref_count;	    /* Reference count */
	simple_lock_data_t	                ovl_ref_lock;	    /* Lock for ref_count field */
	ovl_map_entry_t						ovl_hint;			/* hint for quick lookups */
    simple_lock_data_t	                ovl_hint_lock;	    /* lock for hint storage */
	ovl_map_entry_t						ovl_first_free;		/* First free space hint */
    unsigned int		                ovl_timestamp;	    /* Version number */

#define	min_offset			    		ovl_header.cqh_first->ovle_start
#define max_offset			    		ovl_header.cqh_first->ovle_end
};

#define	ovl_lock_drain_interlock(ovl) { 								\
	lockmgr(&(ovl)->lock, LK_DRAIN|LK_INTERLOCK, 						\
		&(ovl)->ref_lock, curproc); 									\
	(ovl)->timestamp++; 												\
}

#ifdef DIAGNOSTIC
#define	ovl_lock(ovl) { 												\
	if (lockmgr(&(ovl)->lock, LK_EXCLUSIVE, (void *)0, curproc) != 0) { \
		panic("ova_lock: failed to get lock"); 							\
	} 																	\
	(ova)->timestamp++; 												\
}
#else
#define	ovl_lock(ovl) {  												\
    (lockmgr(&(ovl)->lock, LK_EXCLUSIVE, (void *)0, curproc) != 0); 	\
    (ovl)->timestamp++; 												\
}
#endif /* DIAGNOSTIC */

#define	ovl_unlock(ovl) \
		lockmgr(&(ovl)->lock, LK_RELEASE, (void *)0, curproc)
#define	ovl_lock_read(ovl) \
		lockmgr(&(ovl)->lock, LK_SHARED, (void *)0, curproc)
#define	ovl_unlock_read(ovl) \
		lockmgr(&(ovl)->lock, LK_RELEASE, (void *)0, curproc)

/*
 *	Functions implemented as macros
 */
#define		ovl_map_min(map)	((map)->min_offset)
#define		ovl_map_max(map)	((map)->max_offset)
#define		ovl_map_pmap(map)	((map)->ovl_pmap)

#ifdef _KERNEL
struct pmap;
vm_map_t		ovl_map_create (struct pmap *, vm_offset_t, vm_offset_t, boolean_t);
void			ovl_map_deallocate (ovl_map_t);
int		 		ovl_map_delete (ovl_map_t, vm_offset_t, vm_offset_t);
vm_map_entry_t	ovl_map_entry_create (ovl_map_t);
void			ovl_map_entry_delete (ovl_map_t, ovl_map_entry_t);
void			ovl_map_entry_dispose (ovl_map_t, ovl_map_entry_t);
int		 		ovl_map_find (ovl_map_t, ovl_object_t, vm_offset_t, vm_offset_t *, vm_size_t, boolean_t);
int		 		ovl_map_findspace (ovl_map_t, vm_offset_t, vm_size_t, vm_offset_t *);
void			ovl_map_init (struct ovl_map *, vm_offset_t, vm_offset_t, boolean_t);
int				ovl_map_insert (ovl_map_t, vm_object_t, vm_offset_t, vm_offset_t, vm_offset_t);
boolean_t		ovl_map_lookup_entry (ovl_map_t, vm_offset_t, ovl_map_entry_t *);
//void			vm_map_print (vm_map_t, boolean_t);
void			ovl_map_reference (ovl_map_t);
int		 		ovl_map_remove (ovl_map_t, vm_offset_t, vm_offset_t);
void			ovl_map_startup (void);
#endif
#endif /* OVL_MAP_ */
