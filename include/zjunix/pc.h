#ifndef _ZJUNIX_PC_H
#define _ZJUNIX_PC_H

#include <zjunix/list.h>
#include <zjunix/pid.h>
#include <zjunix/vm.h>

#define KERNEL_STACK_SIZE 4096      //内核栈大小
#define TASK_NAME_LEN 32            //进程名长度
#define START_TIME_LEN 16           //进程开始时间长度
#define PRORITY_NUM 32              //优先级等级
#define PRORITY_BYTES ((PRORITY_NUM + 7) >> 3)  //用于优先级位图 
#define TASK_UNINIT 0               //未初始化
#define TASK_READY  1               //就绪
#define TASK_RUNNING 2              //运行
#define TASK_WAITING 3              //等待
#define TASK_TERMINAL 4             //终结
#define MIN_TIMESLICE 1             //最小时间片数量
#define MAX_TIMESLICE 0xffffffff    //最大时间片

#define PC_DEBUG

struct regs_context{
    unsigned int epc;
    unsigned int at;
    unsigned int v0, v1;
    unsigned int a0, a1, a2, a3;
    unsigned int t0, t1, t2, t3, t4, t5, t6, t7;
    unsigned int s0, s1, s2, s3, s4, s5, s6, s7;
    unsigned int t8, t9;
    unsigned int hi, lo;
    unsigned int gp;
    unsigned int sp;
    unsigned int fp;
    unsigned int ra;
};
typedef struct regs_context context;

struct task_struct {
    pid_t                   pid;                        //进程pid号
    int                     ASID;                       //进程地址空间id号
    volatile int            state;                      //进程状态
    pid_t                   ppid;                       //父进程pid号
    unsigned char           name[TASK_NAME_LEN];        //进程名
    long                    static_prority;             //静态优先级
    long                    dynamic_prority;            //动态优先级
    unsigned int            counter;                    //进程剩余时间片数
    unsigned char           start_time[START_TIME_LEN]; //进程创建时间
    long                    sleep_avg;                  //平均睡眠时间
    int                     is_changed;                 //是否改变优先级
    struct regs_context     context;                    //进程寄存器信息

    struct list_head        sched;                      //用于进程调度       
    struct list_head        list;                       //用于进程链表

    //struct task_struct *    p_pptr;                 //指向父进程
    //struct task_struct *    p_cptr;                 //指向子进程
    //struct task_struct *    p_bptr;                 //指向兄弟进程

    struct mm_struct *      mm;                         //进程地址空间结构指针
    char *                  files;                      //进程打开文件指针
};
typedef struct task_struct task_struct;

union task_union{
    task_struct task;                               //进程控制块
    unsigned char kernel_stack[KERNEL_STACK_SIZE];  //进程内核栈
};
typedef union task_union task_union;

extern struct list_head tasks;                      //存放所有进程
extern struct list_head sched[PRORITY_NUM + 1];     //调度链表
extern task_struct *current_task;                   //当前进程           
unsigned char pro_map[PRORITY_BYTES];               //优先级位图

void init_pc_list();
void init_pro_map();
void add_tasks(task_struct * task);
void add_sched(task_struct * task);
void add_wait(task_struct * task);
void add_pro_map(task_struct * task);
void init_pc();
int task_create(char * task_name, long static_prority, void (*entry)(unsigned int argc, void * argv),
                unsigned int argc, void * argv, pid_t * ret_pid, int is_user);
void remove_terminal(task_struct * task);
void remove_tasks(task_struct * task);
void clear_terminal();
void update_sleep_avg();
void update_dynamic_prority();
void update_pro_map();
task_struct * find_in_pro_map();
task_struct * find_next_task();
static void copy_context(context* src, context* dest);
void activate_mm(task_struct * task);
void pc_schedule(unsigned int status, unsigned int cause, context* pt_context);
int print_proc();
void print_task_struct();
task_struct * find_in_tasks(pid_t pid);
void add_terminal(task_struct * task);
void task_files_delete(task_struct * task);
int pc_kill(pid_t pid);
int kernel_proc(unsigned int argc, void * argv);
int exec_kernel(void *argv, int is_wait, int is_user);
task_struct * wait_check(pid_t pid);
void task_exit();
void wakeup_parent();
void wait_pid(pid_t pid);
extern void switch_ex(struct regs_context* regs);
extern void switch_wa(struct regs_context* des, struct regs_context* src);

#endif  // !_ZJUNIX_PC_H