# get\_free\_page代码解读

<h2 id = 'm'> 目录 </h2>

[教学视频](#t)

[1. 初始化IDT](#1)

[直达底部](#e)

<h2 id = 't'> 教学视频 </h2>


<h2 id = '1'> 1. 楔子 </h2>
  get\_free\_page() 是内核代码中比较难以理解的一段代码，熟悉这段代码，对于我们熟悉和掌握内核中汇编代码有很大帮助，这里我们就对其代码实现进行仔细的阅读。

[返回目录](#m)

<h2 id = '2'> 2. 汇编基础知识 </h2>

  我们这里讲解几个本函数用到的汇编代码

1. STOSL指令相当于将EAX中的值保存到 [ES]:EDI 指向的地址中，若设置了 EFLAGS 中的方向位置位(即在STOSL指令前使用STD指令)则EDI自减4，否则(使用CLD指令)EDI自增4 <br/> 
例如 <br/>
	cld;rep;stosl<br/>
cld设置edi或同esi为递增方向，rep做(%ecx)次重复操作，stosl表示edi每次增加4,然后将ax的内容保存到[ES]:EDI中
2. repe和repne：前者是repeat equal，意思是相等的时候重复，后者是repeat not equal，不等的时候重复；每循环一次cx自动减一，重复cx次；
3. scasb指令： 将[al] 中的内容和 es:[edi]中的内容进行比较，并且edi自动增加或减少，std则edi--；cld则edi++
4. sall 指令： sall k,D 等价于 D = D << k 左移


[返回目录](#m)

<h2 id = '2'> 2. GUN AS 参数语法 </h2>

GNU 汇编代码中，对于输入/输出参数有些字符描述符，描述符的意义，作用分别如下

![](https://i.imgur.com/8ZdIakb.png)

![](https://i.imgur.com/2u46Xim.png)

![](https://i.imgur.com/6kRZnNh.png)


[返回目录](#m)

<h2 id = '3'> 3. 代码解读 </h2>

   get\_free\_page 函数的目的是：寻找mem\_map[0..(PAGING\_PAGES-1)]中的空闲项，即mem_map[i]==0的项，如果找到，就返回其物理地址，找不到返回0，也就是NULL

代码实现如下

![](https://i.imgur.com/HQqdTBR.png)

1. register unsigned long \_\_res asm("ax"); <br/>
   \_\_res是寄存器级变量，值保存在ax寄存器中,就是说对\_\_res的操作等于ax寄存器的操作
2. \_\_asm\_\_("std ; repne ; scasb\n\t") <br/>
  循环比较，找出mem\_map[i]==0的页;<br/>
  std设置DF=1，所以scasb执行递减操作，即每次edi--，指令涉及寄存器al, ecx, [es]:(e)di 三个寄存器，在函数尾部的定义中 <br/>
   :"=a" (\_\_res)
   :"0"(0),"i"(LOW\_MEM),"c"(PAGING\_PAGES),"D"(mem\_map+PAGING\_PAGES-1)
   :"di","cx","dx"); <br>
   %0 是 ax寄存器 <br/>
   %1 al = 0, "0" 表示使用与上面同个位置的输出相同的寄存器，eax = 0, 即有 al = 0;    
   如果mem\_map[i] == 0,表示为空闲页，否则为已分配占用,al保存0值，用于和[es]:edi比较 
   %2 LOW\_MEM，低端内存标记，即1M <br/>
   %3 ecx = PAGING\_PAGES; 主内存叶表个数 <br/>
   %4 es:di = (mem\_map+PAGING\_PAGES-1); 内存管理数组最后一项 <br/> <br/>
    
  这句指令的意思是从数组mem\_map[0..(PAGING\_PAGES-1)]的最后一项 mem\_map[PAGING\_PAGES-1]开始，比较mem\_map[i]是否等于0(0值保存在al寄存器中); 每比较一次,[es]:di值自动减1,如果不相等,[es]:di值减1,即mem_map[i--],继续比较,直到ecx==0; 如果相等，则跳出循环。跳出循环的时候，edi已经自动减1了，也就是说指向前面一个元素了。

3. "jne 1f\n\t" 如果mem_map[0..(PAGING_PAGES-1)]均不等于0, 跳转到标签1f处执行,Nf表示向前标签,Nb表示向后标签,N是取值1-10的十进制数字 

4. "movb $1,1(%%edi)\n\t"
    mem\_map[i]==0是mem\_map[0..(PAGING\_PAGES-1)]中逆序第一个找到的等于0的目标，
    将edi已经减1了,也就是已经指向了mem\_map[i] = 0元素的前面一个元素了，这里加1就是重新指向mem\_map[i] = 0 的起始地址上，即mem\_map[i]=1,标志为该页已被占用，不是空闲位

5. "sall $12,%%ecx\n\t" 此时ecx保存的是mem_map[i]的下标i,即相对页面数（从LOW_MEM开始）<br/>
    举例: <br/>
        假设mem\_map[0..(PAGING\_PAGES-1)]最后一个参数
    mem\_map[PAGING\_PAGES-1] == 0，即i == (PAGING\_PAGES-1),
    所以此时*ecx == PAGING\_PAGES-1;
    此时相对页面地址是4k*(PAGING\_PAGES-1), 也就是(PAGING\_PAGES-1) << 12

6.  "addl %2,%%ecx\n\t" 加上低端内存地址，得到实际物理地址, %2等于LOW\_MEM，在如下语句中定义"0" (0),"i" (LOW\_MEM),"c" (PAGING\_PAGES)

7. "movl %%ecx,%%edx\n\t"
    将ecx寄存器的值保存到edx寄存器中，即将实际物理地址保存到edx寄存器中。

8. "movl $1024,%%ecx\n\t"
    将1024保存到ecx寄存器中，因为每一页占用4096字节(4K), 每次写入4个字节，写入1024次

9.  "leal 4092(%%edx),%%edi\n\t", 
    取当前物理页最后一个字节的起始地址： 4096 = 4096-4 = 1023*4 = (1024-1)*4 。
    将当前物理页最后一个字节的起始物理地址保存在edi寄存器中, 即ecx+4092处的地址保存在edi寄存器中。

10. "rep ; stosl\n\t" 从ecx+4092处开始，反方向，步进4字节，重复1024次，将该物理页1024项全部填入eax寄存器的值，在如下代码定义中，eax初始化为0(al=0,eax=0,ax=0)
    :"0" (0),"i" (LOW_MEM),"c" (PAGING_PAGES), <br/>
    所以该物理页项全部清零。

11. "movl %%edx,%%eax\n"，将该物理页面起始地址放入eax寄存器中，Intel的EABI规则中，eax寄存器用于保存函数返回值

12. "1:" 标签1，用于"jne 1f\n\t"语句跳转返回0值，<br/>
    注意：eax寄存器只在"movl %%edx,%%eax\n"中被赋值，eax寄存器初始值是'0'，如果跳转到标签"1:"处，返回值是0，表示没有空闲物理页。

13. :"=a" (__res) 输出寄存器列表，这里只有一个，其中a表示eax寄存器

14. :"0" (0),"i" (LOW\_MEM),"c" (PAGING\_PAGES), <br/>
    "0"表示与上面同个位置的输出相同的寄存器，即"0"等于输出寄存器eax，即eax既是输出寄存器，同时也是输入寄存器，当然，在时间颗粒度最小的情况小，eax不能同时作为输入或者输出寄存器，
    只能作为输入或者输出寄存器; <br/>
    "i" (LOW\_MEM)是%2，从输出寄存器到输入寄存器依次编号%0，%1，%2.....%N,
    其中"i"表示立即数，不是edi的代号，edi的代号是"D"; <br/>
    "c" (PAGING_PAGES)表示将ecx寄存器存入PAGING_PAGES，ecx寄存器代号"c"。

15. "D" (mem_map+PAGING\_PAGES-1)
    "D"使用edi寄存器，即edi寄存器保存的值是(mem\_map+PAGING\_PAGES-1)
    即%%edi = &mem\_map[PAGING\_PAGES-1]。

16. :"di","cx","dx");
    保留寄存器，告诉编译器"di","cx","dx"三个寄存器已经被分配，在编译器编译中，不会将这三个寄存器分配为输入或者输出寄存器。

17. return \_\_res; <br/>
    返回\_\_res保存的值，相当于汇编的ret，隐含将eax寄存器返回，C语言中是显式返回。

[返回目录](#m)


<h2 id = '5'>附录： 常用汇编代码参考 </h2>

**汇编指令** 
GAS中每个操作都是有一个字符的后缀，表明操作数的大小。

<table>
    <thead>
        <tr>
            <th>C声明</th>
            <th>GAS后缀</th>
            <th>大小(字节)</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td>char</td>
            <td>b</td>
            <td>1</td>
        </tr>
        <tr>
            <td>short</td>
            <td>w</td>
            <td>2</td>
        </tr>
        <tr>
            <td>(unsigned) int / long / char*</td>
            <td>l</td>
            <td>4</td>
        </tr>

        <tr>
            <td>float</td>
            <td>s</td>
            <td>4</td>
        </tr>

        <tr>
            <td>double</td>
            <td>l</td>
            <td>8</td>
        </tr>


        <tr>
            <td>long double</td>
            <td>t</td>
            <td>4</td>
        </tr>


        <tr>
            <td>(unsigned) int / long / char*</td>
            <td>l</td>
            <td>10/12</td>
        </tr>
    </tbody>
</table>

注意：GAL使用后缀“l”同时表示4字节整数和8字节双精度浮点数，这不会产生歧义因为浮点数使用的是完全不同的指令和寄存器。

 

操作数格式：

格式

操作数值

名称

样例（GAS = C语言）

$Imm

Imm

立即数寻址

$1 = 1

Ea

R[Ea]

寄存器寻址

%eax = eax

Imm

M[Imm]

绝对寻址

0x104 = *0x104

（Ea）

M[R[Ea]]

间接寻址

（%eax）= *eax

Imm(Ea)

M[Imm+R[Ea]]

(基址+偏移量)寻址

4(%eax) = *(4+eax)

（Ea,Eb）

M[R[Ea]+R[Eb]]

变址

(%eax,%ebx) = *(eax+ebx)

Imm（Ea,Eb）

M[Imm+R[Ea]+R[Eb]]

寻址

9(%eax,%ebx)= *(9+eax+ebx)

(,Ea,s)

M[R[Ea]*s]

伸缩化变址寻址

(,%eax,4)= *(eax*4)

Imm(,Ea,s)

M[Imm+R[Ea]*s]

伸缩化变址寻址

0xfc(,%eax,4)= *(0xfc+eax*4)

(Ea,Eb,s)

M(R[Ea]+R[Eb]*s)

伸缩化变址寻址

(%eax,%ebx,4) = *(eax+ebx*4)

Imm(Ea,Eb,s)

M(Imm+R[Ea]+R[Eb]*s)

伸缩化变址寻址

8(%eax,%ebx,4) = *(8+eax+ebx*4)

注：M[xx]表示在存储器中xx地址的值，R[xx]表示寄存器xx的值，这种表示方法将寄存器、内存都看出一个大数组的形式。

 

 

数据传送指令：

指令

效果

描述

movl S,D

D <-- S

传双字

movw S,D

D <-- S

传字

movb S,D

D <-- S

传字节

movsbl S,D

D <-- 符号扩展S

符号位填充(字节->双字)

movzbl S,D

D <-- 零扩展S

零填充(字节->双字)

pushl S

R[%esp] <-- R[%esp] – 4;

M[R[%esp]] <-- S

压栈

popl D

D <-- M[R[%esp]]；

R[%esp] <-- R[%esp] + 4;

出栈

注：均假设栈往低地址扩展。

 

 

算数和逻辑操作地址：

指令

效果

描述

leal S,D

D = &S

movl地版，S地址入D，D仅能是寄存器

incl D

D++

加1

decl D

D--

减1

negl D

D = -D

取负

notl D

D = ~D

取反

addl S,D

D = D + S

加

subl S,D

D = D – S

减

imull S,D

D = D*S

乘

xorl S,D

D = D ^ S

异或

orl S,D

D = D | S

或

andl S,D

D = D & S

与

sall k,D

D = D << k

左移

shll k,D

D = D << k

左移(同sall)

sarl k,D

D = D >> k

算数右移

shrl k,D

D = D >> k

逻辑右移

 

 

特殊算术操作：

指令

效果

描述

imull S

R[%edx]:R[%eax] = S * R[%eax]

无符号64位乘

mull S

R[%edx]:R[%eax] = S * R[%eax]

有符号64位乘

cltd S

R[%edx]:R[%eax] = 符号位扩展R[%eax]

转换为4字节

idivl S

R[%edx] = R[%edx]:R[%eax] % S;

R[%eax] = R[%edx]:R[%eax] / S;

有符号除法，保存余数和商

divl S

R[%edx] = R[%edx]:R[%eax] % S;

R[%eax] = R[%edx]:R[%eax] / S;

无符号除法，保存余数和商

注：64位数通常存储为，高32位放在edx，低32位放在eax。

 

 

条件码：

条件码寄存器描述了最近的算数或逻辑操作的属性。

CF：进位标志，最高位产生了进位，可用于检查无符号数溢出。

OF：溢出标志，二进制补码溢出——正溢出或负溢出。

ZF：零标志，结果为0。

SF：符号标志，操作结果为负。

 

 

比较指令：

指令

基于

描述

cmpb S2,S1

S1 – S2

比较字节，差关系

testb S2,S1

S1 & S2

测试字节，与关系

cmpw S2,S1

S1 – S2

比较字，差关系

testw S2,S1

S1 & S2

测试字，与关系

cmpl S2,S1

S1 – S2

比较双字，差关系

testl S2,S1

S1 & S2

测试双字，与关系

 

 

访问条件码指令：

指令

同义名

效果

设置条件

sete D

setz

D = ZF

相等/零

setne D

setnz

D = ~ZF

不等/非零

sets D

 

D = SF

负数

setns D

 

D = ~SF

非负数

setg D

setnle

D = ~(SF ^OF) & ZF

大于（有符号>）

setge D

setnl

D = ~(SF ^OF)

小于等于(有符号>=)

setl D

setnge

D = SF ^ OF

小于(有符号<)

setle D

setng

D = (SF ^ OF) | ZF

小于等于(有符号<=)

seta D

setnbe

D = ~CF & ~ZF

超过(无符号>)

setae D

setnb

D = ~CF

超过或等于(无符号>=)

setb D

setnae

D = CF

低于(无符号<)

setbe D

setna

D = CF | ZF

低于或等于(无符号<=)

 

  

跳转指令：

指令

同义名

跳转条件

描述

jmp   Label

 

1

直接跳转

jmp   *Operand

 

1

间接跳转

je     Label

jz

ZF

等于/零

jne    Label

jnz

~ZF

不等/非零

js     Label

 

SF

负数

jnz    Label

 

~SF

非负数

jg     Label

jnle

~(SF^OF) & ~ZF

大于(有符号>)

jge    Label

jnl

~(SF ^ OF)

大于等于(有符号>=)

jl     Label

jnge

SF ^ OF

小于（有符号<）

jle     Label

jng

(SF ^ OF) | ZF

小于等于(有符号<=)

ja     Label

jnbe

~CF & ~ZF

超过(无符号>)

jae    Label

jnb

~CF

超过或等于(无符号>=)

jb     Label

jnae

CF

低于(无符号<)

jbe    Label

jna

CF | ZF

低于或等于(无符号<=)

 

 

转移控制指令：（函数调用）：

指令

描述

call    Label

过程调用，返回地址入栈，跳转到调用过程起始处，返回地址是call后面那条指令的地址

call    *Operand

leave

为返回准备好栈，为ret准备好栈，主要是弹出函数内的栈使用及%ebp


[返回目录](#m)


<p id = 'e'> </p>