
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
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
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	if (ticks - lastTicks >= TIMESLICE)
	{
		lastTicks = ticks;
		p_proc_ready = proc_table + NR_TASKS - 1; //选中F进程
		isBlockedF = 0;
		return;
	}

	PROCESS *p;

	if (schedulable_queue_size == 0)
	{
		disp_str("no process is schedulable\n");
	}
	else
	{
		if (!numOfNotWorked())
		{
			reWork();
		}

		do
		{
			int process = schedulable_queue[0];
			p_proc_ready = proc_table + process;
			remove(0);	   //删除
			push(process); //移到队末
		} while (p_proc_ready->hasWorked == 1);

		/* int process = schedulable_queue[0];
		p_proc_ready = proc_table + process;
		remove(0);	   //删除
		push(process); //移到队末 */
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

/*======================================================================*
                           sys_delay_milli_seconds
 *======================================================================*/
PUBLIC void sys_delay_milli_seconds(int slice) //todo
{
	p_proc_ready->sleep_time = slice*200; //睡眠时间以tick为单位，一个tick就是对应的一个ms
	p_proc_ready->status = 0;
	int index = find();						 //把该进程移出可调度队列
	if (index != -1)
	{
		remove(index);
	}
	schedule();
	restart();
}

PUBLIC void sys_print_str(char *str)
{
	disp_str(str);
}

PUBLIC void sys_p(SEMAPHORE *s)
{
	s->value--;
	if (s->value < 0)
	{
		sleep(s);
	}
}

PUBLIC void sys_v(SEMAPHORE *s)
{
	s->value++;
	if (s->value <= 0)
	{
		wakeup(s);
	}
}

void sleep(SEMAPHORE *s)
{
	//需要将当前的进程从可调度队列中移除
	int index = find(); //先找到当前进程在可调度队列中的下标，然后再删除当前进程
	if (index != -1)
	{
		remove(index); //todo
	}
	p_proc_ready->status = 1;
	s->list[s->size] = p_proc_ready;
	s->size++;

	schedule();
	restart();
}

void wakeup(SEMAPHORE *s)
{
	if (s->size > 0)
	{
		push(s->list[0]->pid); //移入可调度队列

		s->list[0]->status = 2; //从当前信号量的等待队列中移出队首的等待进程
		for (int i = 0; i < s->size - 1; i++)
		{
			s->list[i] = s->list[i + 1];
		}
		s->size--;
	}
}
