# Linux内核启动：异常和中断服务程序的挂接

<h2 id = 'm'> 目录 </h2>

[教学视频](#t)

[1. 初始化IDT](#1)

[直达底部](#e)

<h2 id = 't'> 教学视频 </h2>

[视频教程 Linux内核启动：异常和中断服务程序的挂接](http://toutiao.com/item/6657503069766418951/ "视频教程 Linux内核启动：异常和中断服务程序的挂接")

<h2 id = '1'> 1. 初始化IDT </h2>

  操作系统需要经常处理中断或异常。中断技术也是广泛使用的，系统调用就是利用中断技术实现的。 中断、异常都需要具体的服务程序来执行。 trap\_init() 函数将中断、异常处理的服务 程序与IDT进行挂接来逐步重建中断服务体系。 

![](https://i.imgur.com/49IB0H0.png)

 代码如下：
![](https://i.imgur.com/f0Yorr6.png)

![](https://i.imgur.com/AssMBqu.png)

![](https://i.imgur.com/IeiK9lX.png)

![](https://i.imgur.com/hkucZkj.png)

  举个例子，n是0；gate\_addr是＆idt[0]，也就是idt的第一项中断描述符的地址；type是 15；dpl（描述符特权级）是0；addr是中断服务程序divide\_error（void）的入口地址。

  - “movw%% dx，%%ax\n\t” 是把edx的低字赋值给eax的低字；edx是（char*）(addr），也就是＆divide\_error； eax的值是0x00080000，8应该看成1000，每一位都有意义，这样 eax的值就是0x00080000+（（char *）（addr）的低字），其中的0x0008是段选择符。 
  - "movw% 0，%%dx\n\t”是把（short）（0x8000+（dpl<<13）+（type<<8））赋值给dx。edx是（char *）（addr），也就是＆divide\_error。 因为这部分数据是按位拼接的，必须计算精确，我们耐心详细计算一下： 0x8000就是二进制的 1000 0000 0000 0000；dpl 是00，dpl<<13就是000 0000 0000 0000；type是15，type<<8 就是 1111 0000 0000；加起来就是1000 1111 0000 0000，这就是dx的值。 edx的计算结果就是（char *）（addr）的高字即＆divide\_error的高字+ 1000 1111 0000 0000。 
  - "movl%% eax，% 1\n\t” 是把eax的值赋给*（（char *）（gate\_addr）），就是赋 给idt[0]的前4字节。 
  - "movl%% edx，%2”是把edx的值赋给*（ 4+（char *）（gate_ addr）），就是赋给idt[0]的后4字节。 8字节合起来就是完整的idt[0]。 
  
![](https://i.imgur.com/mXvZVIW.png)

  所有中断服务程序与IDT的初始化基本上都类似。 set\_system\_gate（n, addr）与set\_ trap\_ gate（n, addr）用的\_set\_gate（gate\_ addr, type, dpl, addr）是一样 的； 差别是set\_trap\_gate的dpl是0，而set\_system\_ gate的dpl是3。
  
> dpl为0的意思是只能由内核处理，dpl为3的意思是系统系统调用可以由3特权级（用户特权级）调用。 

  接下来将IDT的int 0x11～int0x2F都初始化，将IDT中对应的指向中断服务程序的指针设置为 reserved（保留）。 设置协处理器的IDT项。允许主8259A中断控制器的IRQ2、 IRQ3的中断请求。 设置并口（可以接打印机）的IDT项。32位中断服务体系是为适应中断信号机制而建立的。

  这些代码的目的就是要拼出中断描述符。回顾一下中断描述符的格式，如下

![](https://i.imgur.com/iAEK4Jk.png)

  异常描述符设定效果如下

![](https://i.imgur.com/3U4P15v.png)

[返回目录](#m)

<p id = 'e'> </p>