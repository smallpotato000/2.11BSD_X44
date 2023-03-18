/*
 * prop_object.c
 *
 *  Created on: 18 Mar 2023
 *      Author: marti
 */
#include <sys/atomic.h>
#include <sys/malloc.h>
#include <devel/sys/properties.h>

#include "proplib_compat.h"
#include "prop_object.h"

#define atomic_inc_32_nv(x)		(atomic_add_int_nv(x, 1)+1)
#define atomic_dec_32_nv(x)		(atomic_sub_int_nv(x, -1)-1)
#define atomic_inc_32(x)		atomic_add_int(x, 1)
#define atomic_dec_32(x)		atomic_add_int(x, 1)

void
prop_object_init(struct prop_object *po, opaque_t obj, const char *name, void *val, size_t len, uint32_t type)
{
	po->po_obj = obj;
	po->po_name = name;
	po->po_value = val;
	po->po_length = len;
	po->po_type = type;
	po->po_refcnt = 1;
}

int
prop_object_type(opaque_t obj)
{
	struct prop_object *po;

	po = obj;
	if (obj == NULL) {
		return (PROP_TYPE_UNKNOWN);
	}
	return (po->po_type);
}

void
prop_object_retain(opaque_t obj)
{
	struct prop_object *po;
	uint32_t ncnt;

	po = obj;
	ncnt = atomic_inc_32_nv(&po->po_refcnt);
	KASSERT(ncnt != 0);
}

static void
prop_object_release_emergency(opaque_t obj)
{
	struct prop_object *po;
	opaque_t parent = NULL;
	uint32_t ocnt;

	for (;;) {
		po = obj;
		KASSERT(obj);

		ocnt = atomic_dec_32_nv(&po->po_refcnt);
		ocnt++;
		KASSERT(ocnt != 0);

		if (ocnt != 1) {
			break;
		}

		KASSERT(po->po_type);
		if ((po->po_type->pot_free)(&obj) == _PROP_OBJECT_FREE_DONE) {
			break;
		}

		parent = po;
		atomic_inc_32(&po->po_refcnt);
	}
	KASSERT(parent);
	/* One object was just freed. */
	po = parent;
	(*po->po_type->pot_emergency_free)(parent);
}

void
prop_object_release(opaque_t obj)
{
	struct prop_object *po;
	int ret;
	uint32_t ocnt;

	do {
		po = obj;
		KASSERT(obj);

		ocnt = atomic_dec_32_nv(&po->po_refcnt);
		ocnt++;
		KASSERT(ocnt != 0);

		if (ocnt != 1) {
			ret = 0;
			break;
		}

		ret = (po->po_type->pot_free)(&obj);
		if (ret == _PROP_OBJECT_FREE_DONE) {
			break;
		}
		atomic_inc_32(&po->po_refcnt);
	} while (ret == _PROP_OBJECT_FREE_RECURSE);

	if (ret == _PROP_OBJECT_FREE_FAILED) {
		prop_object_release_emergency(obj);
	}
}

void
prop_object_create(struct prop_object *npo, opaque_t obj, const char *name, void *val, size_t len, uint32_t type)
{
	struct prop_object *po;

	po = npo;
	po->po_obj = obj;
	po->po_name = name;
	po->po_value = val;
	po->po_length = len;
	po->po_type = type;
	po->po_refcnt = 1;
}

int
prop_object_set(propdb_t db, struct prop_object *po)
{
	int ret;
	ret = propdb_opaque_set(db, po->po_obj, po->po_name, po->po_value, po->po_length, po->po_type);
	return (ret);
}

int
prop_object_get(propdb_t db, struct prop_object *po)
{
	int ret;
	ret = propdb_opaque_get(db, po->po_obj, po->po_name, po->po_value, po->po_length, po->po_type);
	return (ret);
}
