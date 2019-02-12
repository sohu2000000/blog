#define move_to_user_mode() \  //模仿中断硬件压栈顺序，ss,esp,eflags,cs,eip
__asm__ ("movl %%esp,%%eax\n\t" \
	"pushl $0x17\n\t" \  //SS进栈，0x17即二进制的10111(3特权级，LDT，数据段)
	"pushl %%eax\n\t" \ //ESP进栈
	"pushfl\n\t" \ //EFLAGS进栈
	"pushl $0x0f\n\t" \ //CS进栈，0x0f即01111(3特权级，LDT, 数据段)
	"pushl $1f\n\t" \ //EIP进栈
	"iret\n" \ //出栈恢复现场，反战特权级从0到3
	"1:\tmovl $0x17,%%eax\n\t" \ //下面的代码使得ds,es,fs,gs与ss一致
	"movw %%ax,%%ds\n\t" \
	"movw %%ax,%%es\n\t" \
	"movw %%ax,%%fs\n\t" \
	"movw %%ax,%%gs" \
	:::"ax")

#define sti() __asm__ ("sti"::) //开中断
#define cli() __asm__ ("cli"::) //关中断
#define nop() __asm__ ("nop"::)

#define iret() __asm__ ("iret"::)

#define _set_gate(gate_addr,type,dpl,addr) \
__asm__ ("movw %%dx,%%ax\n\t" \
	"movw %0,%%dx\n\t" \
	"movl %%eax,%1\n\t" \
	"movl %%edx,%2" \
	: \
	: "i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	"o" (*((char *) (gate_addr))), \
	"o" (*(4+(char *) (gate_addr))), \
	"d" ((char *) (addr)),"a" (0x00080000))

#define set_intr_gate(n,addr) \
	_set_gate(&idt[n],14,0,addr)

#define set_trap_gate(n,addr) \
	_set_gate(&idt[n],15,0,addr)

#define set_system_gate(n,addr) \
	_set_gate(&idt[n],15,3,addr)

#define _set_seg_desc(gate_addr,type,dpl,base,limit) {\
	*(gate_addr) = ((base) & 0xff000000) | \
		(((base) & 0x00ff0000)>>16) | \
		((limit) & 0xf0000) | \
		((dpl)<<13) | \
		(0x00408000) | \
		((type)<<8); \
	*((gate_addr)+1) = (((base) & 0x0000ffff)<<16) | \
		((limit) & 0x0ffff); }

#define _set_tssldt_desc(n,addr,type) \
__asm__ ("movw $104,%1\n\t" \   //将104，即01101000存入描述符的第1,2字节
	"movw %%ax,%2\n\t" \  //将tss或ldt的基地址低16位存入描述符的3,4字节
	"rorl $16,%%eax\n\t" \  //循环右移16位，即高、地字节互换
	"movb %%al,%3\n\t" \  //将互换完的第一字节，即地址的第3字节存入第五字节
	"movb $" type ",%4\n\t" \ //将0x89或0x82存入第6字节
	"movb $0x00,%5\n\t" \ //将0x00存入第7字节
	"movb %%ah,%6\n\t" \ //将互换完的第二字节，即地址的第4字节存入第8字节
	"rorl $16,%%eax" \ //复原eax
	::"a" (addr), "m" (*(n)), "m" (*(n+2)), "m" (*(n+4)), \
	 "m" (*(n+5)), "m" (*(n+6)), "m" (*(n+7)) \ //“m"(*(n)) 是gdt第n项描述符的地址开始的内存单元；"m"(*(n+2))是gdt第n项描述符的地址向上3个字节开始的内存单元(index = 2);其他依次类推
	)

#define set_tss_desc(n,addr) _set_tssldt_desc(((char *) (n)),addr,"0x89")
#define set_ldt_desc(n,addr) _set_tssldt_desc(((char *) (n)),addr,"0x82")
