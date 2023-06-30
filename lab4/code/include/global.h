
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* EXTERN is defined as extern except in global.c */
#ifdef GLOBAL_VARIABLES_HERE
#undef EXTERN
#define EXTERN
#endif

EXTERN int ticks;
EXTERN int lastTicks;

EXTERN int mode;
EXTERN int readCount;
EXTERN int writeCount;
EXTERN int isBlockedF;

EXTERN int disp_pos;
EXTERN u8 gdt_ptr[6]; // 0~15:Limit  16~47:Base
EXTERN DESCRIPTOR gdt[GDT_SIZE];
EXTERN u8 idt_ptr[6]; // 0~15:Limit  16~47:Base
EXTERN GATE idt[IDT_SIZE];

EXTERN u32 k_reenter;

EXTERN TSS tss;
EXTERN PROCESS *p_proc_ready;

extern PROCESS proc_table[];
extern char task_stack[];
extern TASK task_table[];
extern irq_handler irq_table[];

extern PROCESS *sleep_table[];
extern int sleep_size;
// semaphore used for writer-reader problem
extern SEMAPHORE rmutex;
extern SEMAPHORE wmutex;
extern SEMAPHORE rw_mutex;
extern SEMAPHORE nr_readers;
extern SEMAPHORE r;
extern SEMAPHORE w;
// semaphore used for producer-consumer problem
extern SEMAPHORE cmutex;    //仓库货架信号量
extern SEMAPHORE mutex;     //控制对仓库互斥访问
extern SEMAPHORE sget1;     //控制C1去仓库消费P1生产的货物
extern SEMAPHORE sget2;     //控制C2,C3去仓库消费P2生产的货物

extern int schedulable_queue[];
extern int schedulable_queue_size;

void push(int);
void remove(int);
int find();
int numOfNotWorked();
void reWork();