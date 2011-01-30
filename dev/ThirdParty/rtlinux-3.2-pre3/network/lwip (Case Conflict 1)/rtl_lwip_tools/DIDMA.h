/*
 * Doubly indexed dynamic memory allocator (DIDMA)
 * Version 0.2
 *
 * Written by Miguel Masmano Tello <mmasmano@disca.upv.es>
 * Copyright (C) Dec, 2002 OCERA Consortium
 * Release under the terms of the GNU General Public License Version 2
 *
 * Version 0.2
 */

#ifndef _INDEXED_MALLOC_H_
#define _INDEXED_MALLOC_H_

/*
 * if __DEBUG__ is defined several functions like mem_dump(), 
 * free_blocks_context() will be availables.
 */
#define __DEBUG__

/* 
 * if __COMPILE_LIKE_A_MODULE__ is defined, indexedmalloc can be compiled 
 * like an independent kernel module 
 */
#define __COMPILE_LIKE_A_MODULE__

/*
 * When QUEUES_FIXES_METHOD is defined, the blocks allocated to one queueu 
 * can't be moved to other queue only it can be moved to initial pointer.
 */
//#define QUEUES_FIXED_METHOD

/* 
 * MAX_FL_INDEX define the max first level number of indexes 
 */
#define MAX_FL_INDEX 30 

/* 
 * MAX_SL_INDEX define the max second level number of indexes 
 */
#define MAX_SL_LOG2_INDEX 5 // Real value is 2**MAX_SL_INDEX

#ifdef QUEUES_FIXED_METHOD
/*
 * MEM_PAGES_SIZE is the size of the pages and moreover it is the maximus 
 * size that can be allocated by malloc ()
 */
#define MEM_PAGES_LOG2_SIZE 14
#define MEM_PAGES_SIZE (1 << MEM_PAGES_LOG2_SIZE)
#endif //#ifdef QUEUES_FIXES_METHOD

#ifdef __KERNEL__
#ifdef __RTL__

/* RTLinux module */

#include <rtl.h>

#define PRINT_MSG rtl_printf
#define PRINT_DBG_C(message) rtl_printf(message)
#define PRINT_DBG_D(message) rtl_printf("%i", message);
//#define PRINT_DBG_F(message) rtl_printf("%f", message);
#define PRINT_DBG_H(message) rtl_printf("%x", message);

#else // #ifdef __RTL__
#include <linux/types.h>

#define PRINT_MSG printk
#define PRINT_DBG_C(message) printk(message)
#define PRINT_DBG_D(message) printk("%i", message);
//#define PRINT_DBG_F(message) printk("%f", message);
#define PRINT_DBG_H(message) printk("%x", message);

#endif // #ifdef __RTL__

#ifdef CONFIG_BIGPHYSAREA

#include <linux/bigphysarea.h>

#define SYSTEM_MALLOC(size) (void *) bigphysarea_alloc(size)
#define SYSTEM_FREE(ptr) bigphysarea_free((caddr_t) ptr, 0);

#else // #ifdef CONFIG_BIGPHYSAREA

#define SYSTEM_MALLOC(size) kmalloc(size, GFP_KERNEL)
#define SYSTEM_FREE(ptr) kfree(ptr)

#endif // #ifdef CONFIG_BIGPHYSAREA

#else // #ifdef __KERNEL__

/* User space */

#include <stdlib.h>
#define PRINT_MSG printf
#define PRINT_DBG_C(message) printf(message)
#define PRINT_DBG_D(message) printf("%i", message);
#define PRINT_DBG_F(message) printf("%f", message);
#define PRINT_DBG_H(message) printf("%x", message);

#define SYSTEM_MALLOC(size) malloc(size)
#define SYSTEM_FREE(ptr) free(ptr)

#endif // #ifdef __KERNEL__


/* init_memory_pool reserves a memory buffer */
#if !defined (__RTL__) && !defined (__KERNEL__)
int init_memory_pool (int max_size);
void destroy_memory_pool(void);
#else /* #if !defined (__RTL__) && !defined (__KERNEL__) */
#ifndef __COMPILE_LIKE_A_MODULE__
int init_memory_pool (int max_size);
void destroy_memory_pool(void);
#endif /* #ifndef _COMPILE_LIKE_A_MODULE_ */
#endif /* #if !defined (__RTL__) && !defined (__KERNEL__) */
#include <sys/types.h>
/* see man malloc */
void *rt_malloc (size_t size);

/* see man realloc */
void *rt_realloc(void *p, size_t new_len);

/* see man calloc */
void *rt_calloc(size_t nelem, size_t elem_size);

/*  
 * see man free 
 *
 * rt_free () is only guaranteed to work if ptr is the address of a block
 * allocated by rt_malloc() (and not yet freed). 
 */
void rt_free (void *ptr);

#ifdef __DEBUG__

/* dump_mem () does a dumped of the memory context */
void dump_mem (void);

/* 
 * free_blocks_context () show the content
 * of free blocks
 */
void free_blocks_context(void);

/* 
 * structure_context () gives information about 
 * algorithm structures
 */
void structure_context (void);

/* 
 * check_mem () checks memory searching 
 * errors and incoherences 
 * return :
 * 0 if there isn't any error
 * or 
 * -1 in other case
 */
int check_mem (void);

/* 
 * global_state () returns overhead, free and used space
 */
void global_state (int *free, int *used, int *overhead);

#endif // #ifdef __DEBUG__
#endif // #ifndef _INDEXED_MALLOC_H_
