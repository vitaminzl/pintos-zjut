#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
/*--------------------- Added by ZL ----------------------*/
#include <stdint.h>
#include <syscall-nr.h>
#include <stdio.h>
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"

/*--------------------- Added by ZL ----------------------*/
void* check_valid(void* vaddr);
void exit_error();
bool is_valid_args(void* esp, uint8_t argc);
void acquire_file_lock();
void release_file_lock();
/*--------------------------------------------------------*/

void syscall_init (void);

#endif /* userprog/syscall.h */
