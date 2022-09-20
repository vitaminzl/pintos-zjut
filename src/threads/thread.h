#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/synch.h"
#include "fixed_point.h"

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */
/*--------------------- Added by ZL ----------------------*/ 
#define NICE_MIN -20 
#define NICE_MAX 20
#define UNIT32_MAX ((unsigned int)0xffffffff)

struct lock file_lock;

#ifdef USERPROG
/* 记录子线程的一些信息，tid、是否处于就绪状态
 * 信号量、以及退出码 */
struct child_thread
{
  tid_t tid;				/* 子进程pid */
  bool is_waited;			/* 是否被等待过了 */
  struct list_elem elem;		/* 子进程队列节点 */
  struct semaphore sema;		/* 信号量 */
  int exit_code;			/* 退出信息 */
};

struct thread_file
{
  int fd;
  struct file* file;
  struct list_elem elem;
};

#endif

/*--------------------------------------------------------*/ 

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   T_REACHED ();_tid
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    struct list_elem allelem;           /* List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */
	
    /*--------------------- Added by ZL ----------------------*/ 

    int sleepticks;			/* 剩余休眠的时间 */
    int original_priority;              /* 原始优先级 */
    struct list locks;                  /* 占有锁的队列 */
    struct lock* lock_applying; 	/* 正在申请的锁 */
    int nice;
    int recent_cpu;

#ifdef USERPROG
    /* 进程相关系统调用的数据结构 */
    struct thread* parent;		/* 父进程指针，主要用来让子进程告诉父
					   进程自己的装载情况 */
    struct child_thread* child;		/* 子进程 */
    struct list child_list;		/* 子进程队列 */
    struct semaphore sema;		/* 信号量，用于父子进程的同步，主要应
					   用于它们之间的wait */
    bool is_load_success;		/* 子进程是否装载成功，并由子进程通过
					   parrent指针对该标志位置位 */
    int exit_code;			/* 进程终止信息 */


    /* 文件操作系统调用的数据结构 */
    int fd;
    struct list file_list;
    struct file* file_owned;

#endif
    /*--------------------------------------------------------*/ 

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };


/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

/*--------------------- Added by ZL ----------------------*/ 
void check_and_unblock(struct thread* t, void*aux UNUSED);
bool thread_cmp_priority (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);
void thread_hold_the_lock(struct lock* lock);
bool lock_cmp_priority(const struct list_elem* a, const struct list_elem* b, void* aux UNUSED);
void remove_lock (struct lock *lock);
/* 从捐赠者队列中获取最高优先级  */
void update_priority(struct thread* t);
void update_mlfqs_avg_and_recent_cpu(void);
void calculate_mlfqs_load_avg(void);
void update_mlfqs_priority_aux(struct thread* t, void* aux UNUSED);
void update_mlfqs_priority(void);
void update_mlfqs_recent_cpu_aux(struct thread* t, void* aux UNUSED);
void update_mlfqs_recent_cpu(void);
bool is_idle_thread(struct thread* t);
void mlfqs_update_rc_la_pr(void);
bool is_thread_started(void);
void close_all_files(void);

/*--------------------------------------------------------*/
#endif /* threads/thread.h */
