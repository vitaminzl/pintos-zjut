#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
/*--------------------- Added by ZL ----------------------*/
#define SYSCALL_MAX 13
static int get_user (const uint8_t *uaddr);

/* 将系统调用设置为静态函数，以使这些函数只能在
 * 本cpp文件中调用，保证系统安全 */
static void sys_exec(struct intr_frame *);
static void sys_wait(struct intr_frame *);
static void sys_exit(struct intr_frame *);
static void sys_halt(struct intr_frame *);
static void sys_create(struct intr_frame *);
static void sys_remove(struct intr_frame *);
static void sys_open(struct intr_frame *);
static void sys_filesize(struct intr_frame* );
static void sys_read(struct intr_frame* );
static void sys_write(struct intr_frame*);
static void sys_seek(struct intr_frame*);
static void sys_tell(struct intr_frame*);
static void sys_close(struct intr_frame*);

/* 通过fd寻找file */
struct thread_file* find_file_by_fd(int fd);

static void (*syscalls[])(struct intr_frame*) =
{
  [SYS_HALT] sys_halt,
  [SYS_EXIT] sys_exit,
  [SYS_EXEC] sys_exec,
  [SYS_WAIT] sys_wait,
  [SYS_CREATE] sys_create,
  [SYS_REMOVE] sys_remove,
  [SYS_OPEN] sys_open, 
  [SYS_FILESIZE] sys_filesize,
  [SYS_READ] sys_read,
  [SYS_WRITE] sys_write,
  [SYS_SEEK] sys_seek,
  [SYS_TELL] sys_tell,
  [SYS_CLOSE] sys_close,
};
/*--------------------------------------------------------*/

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* 重写syscall中断处理函数
 * 首先检查参数用户空间的合法性
 * 然后再检查中断号是否处在正确范围内 */
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  /*--------------------- Added by ZL ----------------------*/
  int * p = f->esp;
  /* 检查参数空间是否处于用户空间内 */
  check_valid(p + 1);
  /* 从栈顶取出中断号 */
  int syscall_code = * (int* ) f->esp;
  if(syscall_code <= 0 || syscall_code >= SYSCALL_MAX)
  {
    exit_error();
  }
  syscalls[syscall_code](f);
  /*--------------------------------------------------------*/
}

/*--------------------- Added by ZL ----------------------*/
/* 所有的syscall基本上可以总结为以下三步骤：
 * 1. 合法性检查
 * 2. 调用内核例程，实现系统调用
 * 3. 将返回值（如果有的化）存至EAX中 */

/* 对于exec来说，输入是文件名+参数的字符串,首先是user_ptr
 * 存的是参数的地址，而输入的参数是字符串，即char*类型。
 * 首先应取出参数，然后将参数转换为char*类型即可。
 * 这里有点绕，简单来说就是*user_ptr取参数，(char*)将参数
 * 强制转换。 */
static void 
sys_exec(struct intr_frame* f)
{
  uint32_t* user_ptr = f->esp;
  check_valid(++user_ptr);
  check_valid(*user_ptr);
  f->eax = process_execute((char*)* user_ptr);
}

/* wait输入的是1个pid，直接取出来即可，并且wait是有
 * 输出的，其输出为process的返回值，即退出信息码 */
static void 
sys_wait(struct intr_frame* f)
{
  uint32_t* user_ptr = f->esp;
  check_valid(++user_ptr);
  f->eax = process_wait(*user_ptr);
}

/* exit输入的是1个整型变量即退出状态，将其赋值给
 * 当前进程的退出信息数据中，然后调用线程退出函数。  */
static void 
sys_exit(struct intr_frame* f)
{
  uint32_t* user_ptr = f->esp;
  check_valid(++user_ptr);
  thread_current()->exit_code = *user_ptr;
  thread_exit();
}

/* 关机函数，无输入参数，无返回值，直接调用关机
 * 函数即可。 */
static void 
sys_halt(struct intr_frame* f)
{
  shutdown_power_off();
  NOT_REACHED();
}

	

/* 文件创建操作输入的参数 */
static void 
sys_create(struct intr_frame* f)
{
  uint32_t *user_ptr = f->esp;
  /* ??? */
  check_valid(user_ptr + 5);
  check_valid(*(user_ptr + 4));
  user_ptr++;

  acquire_file_lock();
  f->eax = filesys_create ((const char *)*user_ptr, *(user_ptr+1));
  release_file_lock();
}

/* 文件删除操作输入的参数为文件名 */
static void 
sys_remove(struct intr_frame* f)
{

  uint32_t *user_ptr = f->esp;
  check_valid(++user_ptr);
  check_valid(*user_ptr);

  acquire_file_lock();
  f->eax = filesys_remove ((const char *)*user_ptr);
  release_file_lock();

}


static void 
sys_open(struct intr_frame* f)
{
  uint32_t *user_ptr = f->esp;
  check_valid(user_ptr + 1);
  check_valid(*(user_ptr + 1));
  *user_ptr++;

  acquire_file_lock();
  struct file * file_opened = filesys_open((const char *)*user_ptr);
  release_file_lock();

  struct thread * t = thread_current();

  if (file_opened)
  {
    struct thread_file *thread_file_temp = malloc(sizeof(struct thread_file));
    thread_file_temp->fd = t->fd++;
    thread_file_temp->file = file_opened;
    list_push_back (&t->file_list, &thread_file_temp->elem);
    f->eax = thread_file_temp->fd;
  } 
  else
  {
    f->eax = -1;
  }

}

static void 
sys_close(struct intr_frame* f)
{
  uint32_t *user_ptr = f->esp;
  check_valid(++user_ptr);

  struct thread_file * opened_file = find_file_by_fd (*user_ptr);
  if (opened_file)
  {
    acquire_file_lock ();
    file_close (opened_file->file);
    release_file_lock ();
    list_remove (&opened_file->elem);
    free(opened_file);
  }

}

static void 
sys_read(struct intr_frame* f)
{  
  int32_t *user_ptr = f->esp;
  *user_ptr++;
  int fd = *user_ptr;
  int i;
  uint8_t * buffer = (uint8_t*)*(user_ptr+1);
  uint32_t size = *(user_ptr+2);
  if (!is_valid_args(buffer, 1) || !is_valid_args(buffer + size,1))
  {
     exit_error();
  }

  if (fd == 0) 
  {
    for (i = 0; i < size; i++)
    buffer[i] = input_getc();
    f->eax = size;
  }
  else
  {
    struct thread_file * thread_file_temp = find_file_by_fd(*user_ptr);
    if (thread_file_temp)
    {
      acquire_file_lock();
      f->eax = file_read (thread_file_temp->file, buffer, size);
      release_file_lock();
    } 
    else
    {
      f->eax = -1;
    }
  }

}

static void 
sys_write(struct intr_frame* f)
{
  uint32_t *user_ptr = f->esp;
  check_valid(user_ptr + 7);
  check_valid(*(user_ptr + 6));
  *user_ptr++;
  int temp2 = *user_ptr;
  const char * buffer = (const char *)*(user_ptr+1);
  uint32_t size = *(user_ptr+2);
  if (temp2 == 1) 
  {
    printf("%s", buffer);
    f->eax = size;
  }
  else
  {
    struct thread_file * thread_file_temp = find_file_by_fd(*user_ptr);
    if (thread_file_temp)
    {
      acquire_file_lock();
      f->eax = file_write (thread_file_temp->file, buffer, size);
      release_file_lock();
    } 
    else
    {
     f->eax = 0;
    }
  }

}

/* 通过输入的fd先取找到相应的file数据结构，在执行操作 */
static void 
sys_filesize(struct intr_frame* f)
{
  uint32_t *user_ptr = f->esp;
  check_valid(++user_ptr);

  struct thread_file* file_target = find_file_by_fd(*user_ptr);

  if(file_target != NULL)
  {
    acquire_file_lock();
    f->eax = file_length(file_target->file);
    release_file_lock();
  } 
  else
  {
    f->eax = -1;
  }

}

static void 
sys_seek(struct intr_frame* f)
{

  uint32_t *user_ptr = f->esp;
  check_valid(user_ptr + 5);
  user_ptr++;
  
  struct thread_file* file_target = find_file_by_fd(*user_ptr);
  if(file_target)
  {
    acquire_file_lock();
    file_seek(file_target->file, *(user_ptr+1));
    release_file_lock();
  }
  else
  {
    f->eax = -1;
  }
}

static void 
sys_tell(struct intr_frame* f)
{
  uint32_t *user_ptr = f->esp;
  check_valid(++user_ptr);

  struct thread_file* file_target = find_file_by_fd(*user_ptr);
  if(file_target)
  {
    acquire_file_lock();
    f->eax = file_tell(file_target->file);
    release_file_lock();
  }
  else
  {
    f->eax = -1;
  }

}

/* 通过fd寻找file */
struct thread_file*
find_file_by_fd(int fd)
{

  struct list_elem* e;
  struct thread_file* thread_file_ptr = NULL;
  struct list* files = &thread_current()->file_list;
  for(e = list_begin (files); e != list_end (files); e = list_next(e))
  {
    thread_file_ptr = list_entry(e, struct thread_file, elem);
    if (fd == thread_file_ptr->fd)
      return thread_file_ptr;
  }
  return NULL;
}

/* 手册提供的判断页面可否访问的代码 */
static int 
get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:" : "=&a" (result) : "m" (*uaddr));
  return result;
}

void 
acquire_file_lock()
{
  lock_acquire(&file_lock);
}

void 
release_file_lock()
{
  lock_release(&file_lock);
}



/* 主要判断3个方面：
 * 1. 地址是狗在用户虚拟内存范围内
 * 2. 用户虚拟内存是否对应存在的物理空间
 * 3. 
 * 最后返回用户虚拟内存的返回物理地址 */
void*
check_valid(void* vaddr)
{
  if(!is_user_vaddr(vaddr))
  {
    exit_error ();
  }

  /* 返回物理地址 */
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if(ptr == NULL)
  {
    exit_error ();
  }

  
  /* 逐次检测用户指针的4个字节，判断是否都能够
   * 成功访问。 */
  uint8_t *check_byteptr = (uint8_t *) vaddr;
  for(uint8_t i = 0; i < 4; i++) 
  {
    if(get_user(check_byteptr + i) == -1)
    {
      exit_error ();
    }
  }
  return ptr;
}

/* 判断压入的参数是否全都合法 */
bool
is_valid_args(void* esp, uint8_t argc)
{
  for (uint8_t i = 0; i < argc; ++i)
  {
    if( (is_user_vaddr(esp) == NULL) || (pagedir_get_page(thread_current()->pagedir, esp) == NULL) )
    {
      return false;
    }
  }
  return true;
}


/* 非正常退出  */
void
exit_error()
{
  thread_current()->exit_code = -1;
  thread_exit();
}

/*--------------------------------------------------------*/
