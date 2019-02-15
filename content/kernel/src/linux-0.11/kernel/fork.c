/*
 *  linux/kernel/fork.c
 *
 *  (C) 1991  Linus Torvalds
 */

/*
 *  'fork.c' contains the help-routines for the 'fork' system call
 * (see also system_call.s), and some misc functions ('verify_area').
 * Fork is rather simple, once you get the hang of it, but the memory
 * management can be a bitch. See 'mm/mm.c': 'copy_page_tables()'
 */
#include <errno.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>
#include <asm/system.h>

extern void write_verify(unsigned long address);

long last_pid=0;

void verify_area(void * addr,int size)
{
	unsigned long start;

	start = (unsigned long) addr;
	size += start & 0xfff;
	start &= 0xfffff000;
	start += get_base(current->ldt[2]);
	while (size>0) {
		size -= 4096;
		write_verify(start);
		start += 4096;
	}
}

/*
 * 设置子进程的代码段，数据段及创建、复制子进程的第一个页表
 */
int copy_mem(int nr,struct task_struct * p)
{
	unsigned long old_data_base,new_data_base,data_limit;
	unsigned long old_code_base,new_code_base,code_limit;

    /*
     * 取得子进程的代码、数据段限长
     *
     * 关于局部表数据段描述符的选择符为什么是(0x17)，段选择子的格式一共16位，高13位表示段描述符在描述符表中的索引，
     * 接下来一位，即第[2]位TI指示位，用于指示此描述符为GDT还是LDT，[2]=0表示描述符在GDT中，而[2]=1,表示在LDT表中！,
     * 接下来[1][0]为RPL位，用于指示当进程对段访问第请求权限,
     * 而：0x17=0000 0000 0001 0111,表示所以是LDT表中的第二项(数据段描述符).
     *     0x0f=0000 0000 0000 1111,表示LDT表中第一项（代码段描述符）,
     * 第0项为空描述符,在linux-0.11版本中LDT中只有三项：NULL描述符,数据段描述符,代码段描述符。
     */
	code_limit=get_limit(0x0f); //0x0f即01111：代码段(index = 01 代码段)，LDT(1)，3特权级(11)
	data_limit=get_limit(0x17); //0x17即10111：数据段(index = 10 数据段)，LDT(1)，3特权级(11)
    //获取父进程（现在是进程0）的代码段、数据段基地址
	old_code_base = get_base(current->ldt[1]);
	old_data_base = get_base(current->ldt[2]);

    //在0.11版本内核中代码段和数据段起始位置相同
	if (old_data_base != old_code_base)
		panic("We don't support separate I&D");

    //数据段一般位于代码后面，故而数据段限长不小于代码段
	if (data_limit < code_limit)
		panic("Bad data_limit");
    //现在nr是1，0x4000000是64MB
    //在0.11版本内核中，某个进程(nr)的起始地址为任务号nr*64MB, 也就是说每个进程代码段+数据段最多使用64MB, 注意这里说的基地址是线性地址，不是物理地址，所以可以超过16MB
	new_data_base = new_code_base = nr * 0x4000000;
	p->start_code = new_code_base;

    //设置子进程代码段基地址
	set_base(p->ldt[1],new_code_base);
    //设置子进程数据段基地址
	set_base(p->ldt[2],new_data_base);

    //设置新进程的页目录表项和页表项。即把新进程的线性地址内存页对应到实际物理地址内存页面上
    //设置页目录表和复制页表
    //为进程1创建第一个页表、复制进程0的页表，设置进程1的页目录项
	if (copy_page_tables(old_data_base,new_data_base,data_limit)) {
		free_page_tables(new_data_base,data_limit);
		return -ENOMEM;
	}
	return 0;
}

/*
 *  Ok, this is the main fork-routine. It copies the system process
 * information (task[nr]) and sets up the necessary registers. It
 * also copies the data segment in it's entirety.
 */
int copy_process(int nr,long ebp,long edi,long esi,long gs,long none,
		long ebx,long ecx,long edx,
		long fs,long es,long ds,
		long eip,long cs,long eflags,long esp,long ss) //注意：这些参数是 int 0x80、system_call、sys_fork多次累积压栈的结果，顺序玩去是一致的
{
	struct task_struct *p;
	int i;
	struct file *f;

    // 在16MB 内存的最高端获取一页，强制类型转换的潜台词是讲这个页当task_union使用
	p = (struct task_struct *) get_free_page();
	if (!p)
		return -EAGAIN;
	task[nr] = p;  //此时的nr就是1，指向进程1的task_struct页面
	*p = *current;	/* NOTE! this doesn't copy the supervisor stack */
	p->state = TASK_UNINTERRUPTIBLE;
	p->pid = last_pid;
	p->father = current->pid;
	p->counter = p->priority;
	p->signal = 0;
	p->alarm = 0;
	p->leader = 0;		/* process leadership doesn't inherit */
	p->utime = p->stime = 0;
	p->cutime = p->cstime = 0;
	p->start_time = jiffies;
	p->tss.back_link = 0;
	p->tss.esp0 = PAGE_SIZE + (long) p;
	p->tss.ss0 = 0x10;
	p->tss.eip = eip;
	p->tss.eflags = eflags;
	p->tss.eax = 0;
	p->tss.ecx = ecx;
	p->tss.edx = edx;
	p->tss.ebx = ebx;
	p->tss.esp = esp;
	p->tss.ebp = ebp;
	p->tss.esi = esi;
	p->tss.edi = edi;
	p->tss.es = es & 0xffff;
	p->tss.cs = cs & 0xffff;
	p->tss.ss = ss & 0xffff;
	p->tss.ds = ds & 0xffff;
	p->tss.fs = fs & 0xffff;
	p->tss.gs = gs & 0xffff;
	p->tss.ldt = _LDT(nr);
	p->tss.trace_bitmap = 0x80000000;
	if (last_task_used_math == current)
		__asm__("clts ; fnsave %0"::"m" (p->tss.i387));
	if (copy_mem(nr,p)) {
		task[nr] = NULL;
		free_page((long) p);
		return -EAGAIN;
	}
	for (i=0; i<NR_OPEN;i++)
		if (f=p->filp[i])
			f->f_count++;
	if (current->pwd)
		current->pwd->i_count++;
	if (current->root)
		current->root->i_count++;
	if (current->executable)
		current->executable->i_count++;
	set_tss_desc(gdt+(nr<<1)+FIRST_TSS_ENTRY,&(p->tss));
	set_ldt_desc(gdt+(nr<<1)+FIRST_LDT_ENTRY,&(p->ldt));
	p->state = TASK_RUNNING;	/* do this last, just in case */
	return last_pid;
}

/*
 * 为新创建的进程找到一个空闲的位置， NR_TASKS是64
 */
int find_empty_process(void)
{
	int i;

	repeat:
		if ((++last_pid)<0) last_pid=1; //如果++后last_pid溢出，则置1
        //现在，++后last_pid为1，找到有效的last_pid
		for(i=0 ; i<NR_TASKS ; i++)
			if (task[i] && task[i]->pid == last_pid) goto repeat; //条件成立说明pid已经被last_pid已经被使用，last_pid++，直到获得用于新进程的进程好 
    //返回第一个空闲的i
	for(i=1 ; i<NR_TASKS ; i++) //第二次遍历task[64],获得一个空闲的i，成为任务号
		if (!task[i])
			return i;
	return -EAGAIN; //EAGAIN是11
}
