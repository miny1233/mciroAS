# mciroAS
 适用于西安唐都仪器计算机组成原理的汇编器

 在课设中，如果使用实验手册上的微程序，用他所提供的助记符，能够直接生成机器码
 > 注意：编译需要使用C++20，非GCC需要删掉binary的不要内存对齐标志

# 关于这个项目
- 开始的原因：  
 这个项目是在课设开始前一天下午实现的，当时只是写着玩玩，没想到派上大用场了
 这个汇编器极大的提高了效率，特别是在实现非常复杂的程序上省下了很多的时间。
 特别是需要经常修改程序的情况下
- 功能：
  - 目前功能仍旧很简单，首先能编译，有代码错误提示，能指出具体行。
- 缺点  
  - 没有伪指令，不能简单实现跳转，需要自己计算指令的地址位置，或者编译后查看地址，再手动设置
> 大概率不更新这个项目了，上传只是记录一下曾经自己写过的东西，如果能帮到你就更好了。  
> 自从计组课设结束后，就没有动力继续完善了，因为以后也用不到了。
