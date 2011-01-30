/*
 * MBUFF kernel virtual memory driver for Linux,
 * Proc filesystem interface
 *
 * Much code of this code was based on linux/drivers/block/ide-proc.c
 * Copyright (C) 1997-1998     Mark Lord
 *
 */

#ifndef _mbuff_proc_h_
#define _mbuff_proc_h

void proc_mbuff_add_mbuff(struct mbuff *mbuff);
void proc_mbuff_remove_mbuff(struct mbuff *mbuff);
void proc_mbuff_create(void);
void proc_mbuff_destroy(void);

#endif
