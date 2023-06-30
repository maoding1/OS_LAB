
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "global.h"

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK *p_task = task_table;
	PROCESS *p_proc = proc_table;
	char *p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16 selector_ldt = SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++)
	{
		strcpy(p_proc->p_name, p_task->name); // name of the process
		p_proc->pid = i;					  // pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs = ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ds = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.es = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.fs = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ss = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	
	for (int i = 0; i < NR_TASKS; i++)
	{
		proc_table[i].status = 0;
		proc_table[i].sleep_time = 0;
		proc_table[i].ticks = proc_table[2].priority = 10000;
		proc_table[i].hasWorked = 0;
		proc_table[i].count = 0;
	}

	proc_table[0].color = 0x09;
	proc_table[1].color = 0x02;
	proc_table[2].color = 0x04;
	proc_table[3].color = 0x06;
	proc_table[4].color = 0x0d;
	proc_table[5].color = 0x07; //F进程用白色就好

	k_reenter = 0;
	ticks = 0;
	lastTicks = 0;

	mode = 3; //可调整 0 for reader first 1 for writer first 3 for producer-consumer
	readCount = 0;
	writeCount = 0;
	isBlockedF = 0;

	p_proc_ready = proc_table;

	init_clock();
	init_keyboard();

	clean_screen();

	restart(); //从ring0跳转到ring1，执行A进程

	while (1)
	{
	}
}

void atomicP(SEMAPHORE *s)
{
	//disable_irq(CLOCK_IRQ);
	disable_int();
	P(s);
	enable_int();
	//enable_irq(CLOCK_IRQ);
}

void atomicV(SEMAPHORE *s)
{
	//disable_irq(CLOCK_IRQ);
	disable_int();
	V(s);
	enable_int();
	//enable_irq(CLOCK_IRQ);
}

void disp_read_start()
{
	disable_int();

	// disp_color_str(p_proc_ready->p_name, p_proc_ready->color);
	// disp_color_str(" ", p_proc_ready->color);
	// disp_color_str("starts reading\n", p_proc_ready->color);

	enable_int();
}

disp_reading()
{
	disable_int();
	// disp_color_str(p_proc_ready->p_name, p_proc_ready->color);
	// disp_color_str(" ", p_proc_ready->color);
	// disp_color_str("is reading\n", p_proc_ready->color);
	enable_int();
}

disp_read_end()
{
	disable_int();
	//打印读结束信息
	// disp_color_str(p_proc_ready->p_name, p_proc_ready->color);
	// disp_color_str(" ", p_proc_ready->color);
	// disp_color_str("ends reading\n", p_proc_ready->color);

	enable_int();
}

disp_write_start()
{
	disable_int();
	// disp_color_str(p_proc_ready->p_name, p_proc_ready->color);
	// disp_color_str(" begins writing\n", p_proc_ready->color);
	enable_int();
}

disp_writing()
{
	disable_int();
	// disp_color_str(p_proc_ready->p_name, p_proc_ready->color);
	// disp_color_str(" ", p_proc_ready->color);
	// disp_color_str("is writing\n", p_proc_ready->color);
	enable_int();
}

disp_write_end()
{
	disable_int();

	// disp_color_str(p_proc_ready->p_name, p_proc_ready->color);
	// disp_color_str(" ends writing\n", p_proc_ready->color);
	enable_int();
}
/*======================================================================*
                               read
 *======================================================================*/
void read(int slices)
{
	if (mode == 0)
	{ //读者优先

		atomicP(&rmutex);
		if (readCount == 0)
		{

			atomicP(&rw_mutex); //有进程在读的时候不让其它进程写
		}
		readCount++;
		atomicV(&rmutex);

		atomicP(&nr_readers);
		disp_read_start();

		for (int i = 0; i < slices; i++)
		{
			disp_reading();
			milli_delay(TIMESLICE);
		}

		disp_read_end();
		atomicV(&nr_readers);

		atomicP(&rmutex);
		readCount--;
		if (readCount == 0)
		{
			atomicV(&rw_mutex);
		}
		atomicV(&rmutex);
	}
	else if (mode == 1)
	{
		atomicP(&r);
		atomicP(&rmutex);
		if (readCount == 0)
		{
			atomicP(&w);
		}
		readCount++;
		atomicV(&rmutex);
		atomicV(&r);
		//V(&queue);

		atomicP(&nr_readers);
		disp_read_start();

		for (int i = 0; i < slices; i++)
		{
			disp_reading();
			milli_delay(TIMESLICE);
		}

		disp_read_end();
		atomicV(&nr_readers);

		atomicP(&rmutex);
		readCount--;
		if (readCount == 0)
		{
			atomicV(&w);
		}
		atomicV(&rmutex);
	}
}

void write(int slices)
{
	if (mode == 0)
	{
		//读者优先
		atomicP(&rw_mutex);
		writeCount++;
		disp_write_start();
		//milli_delay(slices * TIMESLICE);
		for (int i = 0; i < slices; i++)
		{
			disp_writing();
			milli_delay(TIMESLICE);
		}
		disp_write_end();
		writeCount--;
		atomicV(&rw_mutex);
	}
	else if (mode == 1)
	{
		atomicP(&wmutex);
		if (writeCount == 0)
		{
			atomicP(&r); //申请r锁
		}
		writeCount++;
		atomicV(&wmutex);

		atomicP(&w);
		disp_write_start();
		//milli_delay(slices * TIMESLICE);
		for (int i = 0; i < slices; i++)
		{
			disp_writing();
			milli_delay(TIMESLICE);
		}
		disp_write_end();
		atomicV(&w);

		atomicP(&wmutex);
		writeCount--;
		if (writeCount == 0)
		{
			atomicV(&r);
		}
		atomicV(&wmutex);
	}
}

void produce1() 
{
	atomicP(&cmutex);
	atomicP(&mutex);
	milli_delay(TIMESLICE*3);
	p_proc_ready->count++;
	atomicV(&sget1);
	atomicV(&mutex);
}

void produce2()
{
	atomicP(&cmutex);
	atomicP(&mutex);
	milli_delay(TIMESLICE*3);
	p_proc_ready->count++;
	atomicV(&sget2);
	atomicV(&mutex);
}

void consumer1()
{
	atomicP(&sget1);
	atomicP(&mutex);
	milli_delay(TIMESLICE*3);
	p_proc_ready->count++;
	atomicV(&mutex);
	atomicV(&cmutex);
}

void consumer2and3()
{
	atomicP(&sget2);
	atomicP(&mutex);
	milli_delay(TIMESLICE*3);
	p_proc_ready->count++;
	atomicV(&mutex);
	atomicV(&cmutex);
}
/*======================================================================*
                               ReadA
 *======================================================================*/
void ReadA()// consumer1 when mode == 3
{
	while (1)
	{
		if (mode != 3) {
			read(2);
			delay_milli_seconds(T);
			p_proc_ready->hasWorked = 1;
			while (p_proc_ready->hasWorked == 1)
			{
			}
		} else {
			consumer1();
		}
		
	}
}

/*======================================================================*
                               ReadB
 *======================================================================*/
void ReadB()// consumer2 when mode == 3
{
	while (1)
	{
		if (mode != 3) {
			read(3);
			delay_milli_seconds(T);
			p_proc_ready->hasWorked = 1;
			while (p_proc_ready->hasWorked == 1)
			{
			}
		} else {
			consumer2and3();
		}
	}
}

/*======================================================================*
                               ReadC
 *======================================================================*/
void ReadC()// consumer3 when mode == 3
{
	while (1)
	{
		if (mode != 3) {
			read(3);
			delay_milli_seconds(T);
			p_proc_ready->hasWorked = 1;
			while (p_proc_ready->hasWorked == 1)
			{
			}
		} else {
			consumer2and3();
		}
	}
}

/*======================================================================*
                               WriteD
 *======================================================================*/
void WriteD() // producer1 when mode == 3
{
	while (1)
	{	
		if (mode != 3) {
			write(3);
			delay_milli_seconds(T);
			p_proc_ready->hasWorked = 1;
			while (p_proc_ready->hasWorked == 1)
			{
			}
		} else {
			produce1();
		}
		
	}
}

/*======================================================================*
                               WriteE
 *======================================================================*/
void WriteE() // producer2 when mode == 3
{
	while (1)
	{
		if (mode != 3) {
			write(4);
			delay_milli_seconds(T);
			p_proc_ready->hasWorked = 1;
			while (p_proc_ready->hasWorked == 1)
			{
			}
		} else {
			produce2();
		}
	}
}

/*======================================================================*
                               F
 *======================================================================*/
void F()
{
	int times = 1;
	char *nL = "                                                                  ";
	while (1)
	{
		if (!isBlockedF)
		{
			if (mode != 3) {
				if (times <= 20) {
					disp_int(times);times++;
					if (times <= 10)
						disp_color_str(" ",WHITE);
					disp_color_str(": ", WHITE);
					for (int i = 0; i < NR_TASKS-1; i++) {
						if (proc_table[i].status == 1) {
							disp_color_str("X ", RED);
						} else if (proc_table[i].status == 0) {
							disp_color_str("Z ", BLUE);
						} else {
							disp_color_str("O ", GREEN);
						}
					}
					disp_color_str(nL, WHITE);
				}
			} else {
				if (times <= 20) {
					disp_int(times); times++;
					if (times <= 10)
						disp_color_str(" ",WHITE);
					disp_color_str(": ", WHITE);
					for (int i = 3; i < NR_TASKS-1; i++) {
						disp_int(proc_table[i].count);
						disp_color_str(" ", WHITE);
					}
					for (int i = 0; i < 3; i++) {
						disp_int(proc_table[i].count);
						disp_color_str(" ", WHITE);
					}
					disp_str("\n");
				}
			}
			
			isBlockedF = 1;
		}
	}
}
