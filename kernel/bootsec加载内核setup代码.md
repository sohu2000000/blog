
# bootsec 加载内核 setup 代码

<h2 id = 'm'> 目录 </h2>

[教学视频](#t)

[1. 加载系统代码整体步骤](#1)

[2. 加载bootsec代码](#2)

[直达底部](#e)

<h2 id = 't'> 教学视频 </h2>

<h2 id = '1'> 1. BOOTSEC内存规划 </h2>

  BIOS 已经把 bootsect 也就是引导程序载入内存了，现在它的作用就是把第二批和第三批程序陆续加载到内存中。 为了把第二批和第三批程序加载到内存中的适当位置，bootsect首先做的工作就是 规划内存。
  
  在实模式状态下，寻址的最大范围是 1 MB，BOOTSEC还属于实模式，寻址范围也是1MB，需要仔细规划内存使用，相关代码如下
![BOOTSEC内存规划代码](https://i.imgur.com/rdtBXM8.png)

  - 包括将要加载的setup程序的扇区数（SETUPLEN） 以及SETUP被加载到的位置（SETUPSEG）；
  - 启动扇区 BOOTSEC 被 BIOS 加载的位置（BOOTSEG） 及将要移动到的新位置（INITSEG）； 
  - 内核（kernel）被加载的位置（SYSSEG）、 内核的末尾位置（ENDSEG）及 根文件系统设备号（ROOT_DEV）。

![实模式下BOOTSEC 的内存使用规划](https://i.imgur.com/vZNByWz.png)
  
  设置这些位置就是为了确保将要载入内存的代码与已经载入内存的代码及数据各在其位，互不覆盖。

<h2 id = '2'> 2. 复制移动bootsec代码 </h2>

  bootsect 启动程序将自身（全部的51B内容）从内存0x07C00（BOOTSEG）处复制至内存0x90000（INITSEG）处。
![BOOTSEC复制自身](https://i.imgur.com/yX3NXPV.png)

  代码实现
![BOOTSEC复制自身代码](https://i.imgur.com/jcIDHJV.png)
  
  在这次复制过程中， ds（ 0x07C0）和 si（ 0x0000）联合使用，构成了源地址 0x07C00； es（0x9000）和 di（0x0000） 联合使用， 构成了目的地址 0x90000， 而 mov cx，# 256 这一行循环控制量，提供了需要复制的“ 字” 数（一个字为 2 字节， 256 个字正好是 512 字节， 也就是第一扇区的字节数）。注意，此时CPU的段寄存器（CS）指向 0x07C0（BOOTSEG）， 即原来bootsect程序所在的位置。

  BOOTSEC跳转到新的代码位置
![BOOTSEC跳转到新位置](https://i.imgur.com/XdyC8qX.png)
  
  了解到当时 CS 的值为 0x07C0， 执行完这个跳转后，CS值变为 0x9000（INITSEG）， IP 的值为从 0x9000（INITSEG）到 go： mov ax, cs 这一行对应指令的偏移
  
  bootsect 复制到了新的地方， 并且要在新的地方继续执行。 因为代码的整体位置发生了变化，所以代码中的各个段也会发生变化。前面已经改变了CS， 现在对DS、ES、SS 和 SP 进行 调整。通过 ax， 用 CS 的值 0x9000 来把数据段寄存器（DS）、附加段寄存器（ES）、栈 基址寄存器（SS） 设置成与代码段寄存器（CS）相同的位置，并将栈顶指针 SP 指向偏移地址 为 0xFF00 处。

  ![调整各个段寄存器](https://i.imgur.com/hLVrf1D.png)
  
  SS 和 SP 联合使用，就构成了栈数据在内存中的位置值。

  至此， bootsect的第一步操作， 即规划内存并把自身从0x07C00的位置复制到 0x90000 的 位置的动作已经完成了。

<h2 id = '3'> 3. 将SETUP程序加载到内存 </h2>

  加载 setup 这个程序， 要借助 BIOS 提供的 int 0x13 中断向量所指向的中断服务程序（ 也就是磁盘服务程序）来完成。

![调用0x13中断](https://i.imgur.com/vI89qp2.png)

  int 0x19 中断向量服务程序是 BIOS 执行的， 而 int 0x13 的 中断服务程序是 Linux 操作系统的 bootsect 执行的。

  使用 int 0x13 中断时， 就要事先将指定的扇区、加载的内存位置等信息传递给服务程序， 即传参。
![0x13磁盘读取程序传参代码](https://i.imgur.com/Kl9jjd4.png)

  4 个 mov 指令可以看出， 系统给 BIOS 中断服务程序传参是通过几个通用寄存器实现的。

![加载SETUP程序](https://i.imgur.com/nN0ejm9.png)

  参数传递完毕后， 执行 int 0x13 指令， 产生 0x13 中断， 找到这个中断服务程序，将软盘/硬盘 第二扇区开始的4个扇区，即 setup. s对应的程序加载至内存的 SETUPSEG（0x90200）处。 复制后的 bootsect 的起始位置是 0x90000， 占用 512 字节的内存空间。 0x90200 紧挨着 bootsect 的尾端， 所以 bootsect 和 setup 是连在一起的。

  现在， 操作系统已经从软盘/硬盘中加载了 5 个扇区的代码。等 bootsect 执行完毕后， setup 这个程序就要开始工作了。


  int 0x19 相对应的中断服务程序的作用 就是把软盘/硬盘 第一扇区中的程序（512 B）加载到内存 中的指定位置。 该中断服务程序功能是 BIOS 事先设计好的， 代码是固定的， 就是“找到软盘/硬盘”并“加载第一扇区”， 其余的什么都不知道。

  int 0x19 中断向量所指向的中断服务程序， 即启动加载服务程序， 将软驱/硬盘 0 号磁头对应盘 面的 0 磁道 1 扇区的内容复制至内存 <font color=red>**0x07C00** </font> 处。



<p id = 'e'> </p>