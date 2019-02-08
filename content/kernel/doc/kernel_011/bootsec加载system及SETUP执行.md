
# bootsec加载内核system及SETUP初始执行

<h2 id = 'm'> 目录 </h2>

[教学视频](#t)

[bootsec加载内核system程序及SETUP程序初始执行](http://toutiao.com/item/6655543854214676996/ "bootsec加载内核system程序及SETUP程序初始执行")

[1. 载入SYSTEM代码](#1)

[2. 确认根文件系统设备号](#2)

[3. 跳转到 SETUP 程序](#3)

[5. 执行 SETUP 程序](#5)

[直达底部](#e)

<h2 id = 't'> 教学视频 </h2>

<h2 id = '1'> 1. 载入SYSTEM代码 </h2>
  
  第二批代码 setup 已经载入内存， 现在要加载第三批代码 system 。 仍然使用 BIOS 提供的 int 0x13 中断。 bootsect 程序要执行第三批程序的载入工作， 即将系统模块载入内存。

![BOOTSEC再次int0x13中断](https://i.imgur.com/Gy06cYb.png)

  这次加载的扇区数是 240 个，由于等待时间较长， 在屏幕上显示一行屏幕信息“ Loading system……” 以提示用户此时正在加载系统。

![](https://i.imgur.com/JKZDHxq.png)

![](https://i.imgur.com/202ABax.png)

  SYSTEM模块加载工作主要是由 bootsect 调用 read_ it 子程序完成的。
  
![](https://i.imgur.com/jTndSt7.png)

![](https://i.imgur.com/BI704kc.png)

![](https://i.imgur.com/Do1MRsI.png)

  这个程序将硬盘第六个扇区开始的的约 240 个扇区的 system 模块加载至内存的 SYSSEG（ 0x10000） 处往后的 120 KB 空间中。

![](https://i.imgur.com/V5IT59Y.png)

[返回目录](#m)

<h2 id = '2'> 2. 确认根文件系统设备号 </h2>

  第三批程序 SYSTEM 已经加载完毕， 整个操作系统的代码已全部加载至内存。 bootsect 的主体 工作已经做完了， 还有一点小事， 就是要再次确定一下根设备号

![](https://i.imgur.com/jtJmj2d.png)

> Linux 0. 11 使用 Minix 操作系统的文件系统管理方式， 要求系统必须存在一个根文件系统， 其他文件系统挂接其上， 而不是同等地位。 Linux 0. 11 没有提供在设备上建立文件系统的工具， 故 必须在一个正在运行的系统上利用工具（类似 FDISK 和 Format）做出一个文件系统并加载至本机。
> 因此 Linux 0. 11 的启动需要两部分数据， 即系统内核镜像和根文件系统。
> <font color=red face="微软雅黑">**在内存中开辟了 2 MB 的空间作为虚拟盘（main 函数中）**</font>， 并在 BIOS 中设置软盘/硬盘驱动器为启动盘， 所以， 经过一系列检测， 确认计算机中实际安装的软盘/硬盘驱动器为根设备，并将信息 写入机器系统数据。 main 函数一开始就用机器系统数据中的这个信息设置根设备， 并为“根文件系统 加载”奠定基础。

 代码实现如下
![](https://i.imgur.com/ebTdIAb.png)

[返回目录](#m)

<h2 id = '3'> 3. 跳转到 SETUP 程序 </h2>

  下面 BOOTSEC 要通过执行“ jmpi 0， SETUPSEG” 这行语句跳转至 0x90200 处， 就是前面讲过的第二批程序—— setup 程序加载的位置。 CS：IP 指向 setup 程序的第一 条指令， 意味着 由 setup 程序接着 bootsect 程序继续执行。

![](https://i.imgur.com/Zv0UpEA.png)

![](https://i.imgur.com/0OU91us.png)
   
  此时 bootsec 代码已经执行完成，其代码 0x9000:0 开始的 512B 的内存内容已经可以覆盖。


<h2 id = '5'> 5. 执行 SETUP 程序 </h2>

  setup 程序现在开始执行。 它做的第一件事情就是利用 BIOS 提供的中断服务程序从设备上提取内核运行所需的机器系统数据， 其中包括光标位置、显示页面等数据，并分别从中断向量 0x41 和 0x46 向量值所指的内存地址处获取硬盘参数表 1、 硬盘参数表 2， 把它们存放在 0x9000： 0x0080 和 0x9000： 0x0090 处。 这些机器系统数据被加载到内存的 0x90000 ～ 0x901FC 位置，覆盖了已经执行完成bootsec程序所在内存。这些数据是系统启动内核所需要的数据，在系统启动读取它们之前，一定不能被覆盖。

![SETUP加载机器系统数据](https://i.imgur.com/AS8UNpq.png)


  机器系统数据所占的内存空间为 0x90000 ～ 0x901FD， 共 510 字节， 即原来 bootsect 只有 2 字节未被覆盖。 
   
  到此为止， 操作系统内核程序的加载工作已经完成。
  接下来的操作系统通过已经加载到内存中的代码，将从实模式到保护模式的转变，成为真正的“现代” 操作系统。


[返回目录](#m)
<p id = 'e'> </p>