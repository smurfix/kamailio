/*
 * Copyright (C) 2022 Matthias Urlichs
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * \file
 * \brief Use the system malloc
 * \ingroup mem
 */

#include <stdlib.h>
#include <string.h>

#include "sys_malloc.h"
#include "../dprint.h"
#include "../globals.h"
#include "memdbg.h"
#include "../cfg/cfg.h" /* memlog */

#include "pkg.h"


void* sys_malloc(void* qmp, size_t size)
{
	return malloc(size);
}


void* sys_mallocxz(void* qmp, size_t size)
{
	void *p;

	p = malloc(size);
	if(p) memset(p, 0, size);
	return p;
}

void sys_free(void* qmp, void* p)
{
	free(p);
}


void* sys_realloc(void* qmp, void* p, size_t size)
{
	return realloc(p,size);
}


void* sys_reallocxf(void* qmp, void* p, size_t size)
{
	void *r;

	r = realloc(p, size);

	if(!r && p)
		free(p);
	return r;
}

void* sys_resize(void* qmp, void* p, size_t size)
{
        void *r;
        if(p) free(p);
        r = malloc(size);
        return r;
}

void sys_check(struct sys_block* qm)
{
	/* nothing to do, we hope */
}

void sys_status(void* qmp)
{
	/* dummy */
}

void sys_info(void* qmp, struct mem_info* info)
{
	memset(info,0, sizeof(*info));
}


/* returns how much free memory is available
 * it never returns an error (unlike fm_available) */
unsigned long sys_available(void* qmp)
{
	return 1024*1024*1024;
}

void sys_sums(void *qmp)
{
	return;
}

void sys_mod_get_stats(void *qmp, void **sys_rootp)
{
	LM_WARN("No statistics for the system memory manager\n");
	return;
}

void sys_mod_free_stats(void *sys_rootp)
{
	return;
}


/*memory manager core api*/
static char *_sys_mem_name = "sys_malloc";

/**
 * \brief Destroy memory pool
 */
void sys_malloc_destroy_pkg_manager(void)
{
}

/**
 * \brief Init memory pool
 */
int sys_malloc_init_pkg_manager(void)
{
	sr_pkg_api_t ma;

	memset(&ma, 0, sizeof(sr_pkg_api_t));
	ma.mname = _sys_mem_name;
	ma.xmalloc = sys_malloc;
	ma.xmallocxz = sys_mallocxz;
	ma.xfree = sys_free;
	ma.xrealloc = sys_realloc;
	ma.xreallocxf = sys_reallocxf;
	ma.xstatus = sys_status;
	ma.xinfo = sys_info;
	ma.xavailable = sys_available;
	ma.xsums = sys_sums;
	ma.xmodstats = sys_mod_get_stats;
	ma.xfmodstats = sys_mod_free_stats;

	return pkg_init_api(&ma);
}


/* SHM - shared memory API*/
static gen_lock_t* _sys_shm_lock = 0;

#define sys_shm_lock()    lock_get(_sys_shm_lock)
#define sys_shm_unlock()  lock_release(_sys_shm_lock)

/**
 *
 */
void sys_shm_glock(void* qmp)
{
	lock_get(_sys_shm_lock);
}

/**
 *
 */
void sys_shm_gunlock(void* qmp)
{
	lock_release(_sys_shm_lock);
}

/**
 *
 */
void sys_shm_lock_destroy(void)
{
	if (_sys_shm_lock){
		DBG("destroying the shared memory lock\n");
		lock_destroy(_sys_shm_lock); /* we don't need to dealloc it*/
	}
}

/**
 * init the core lock
 */
static
int sys_shm_lock_init(void)
{
    if (_sys_shm_lock) {
        LM_DBG("shared memory lock initialized\n");
        return 0;
    }

    _sys_shm_lock = malloc(sizeof(gen_lock_t));

    if (_sys_shm_lock==0){
        LOG(L_CRIT, "could not allocate lock\n");
        return -1;
    }
    if (lock_init(_sys_shm_lock)==0){
        LOG(L_CRIT, "could not initialize lock\n");
        return -1;
    }
    return 0;
}

/**
 * \brief Init memory pool
 */
int sys_malloc_init_shm_manager(void)
{
	sr_shm_api_t ma;
	sys_shm_lock_init();

	memset(&ma, 0, sizeof(sr_shm_api_t));
	ma.mname          = _sys_mem_name;
	ma.xmalloc        = sys_malloc;
	ma.xmallocxz      = sys_mallocxz;
	ma.xmalloc_unsafe = sys_malloc;
	ma.xfree          = sys_free;
	ma.xfree_unsafe   = sys_free;
	ma.xrealloc       = sys_realloc;
	ma.xreallocxf     = sys_reallocxf;
	ma.xresize        = sys_resize;
	ma.xstatus        = sys_status;
	ma.xinfo          = sys_info;
	ma.xavailable     = sys_available;
	ma.xsums          = sys_sums;
	ma.xmodstats      = sys_mod_get_stats;
	ma.xfmodstats     = sys_mod_free_stats;
	ma.xglock         = sys_shm_glock;
	ma.xgunlock       = sys_shm_gunlock;

	if(shm_init_api(&ma)<0) {
		LM_ERR("cannot initialize the core shm api\n");
		return -1;
	}
	if(sys_shm_lock_init()<0) {
		LM_ERR("cannot initialize the core shm lock\n");
		return -1;
	}
	return 0;
}

