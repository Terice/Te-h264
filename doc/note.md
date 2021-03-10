# 运行路线

总的路线就是沿着decode这个函数一路走下去

ther 运行

reader 从外界读入NAL码流

初始化：
- decoder用来处理一些解码命令
- terror 用来处理错误
- parser 用来解析解码环境

解NAL头部

根据type:

如果是参数NAL

- 解SPS
- 解PPS
SPS， PPS 赋值给 解析器用来设置参数

如果是图像NAL

- 预测
- 残差
- 重建

数据交回picture，picture交给decoder

writer向外界写出picture

# 文件说明

前缀带g如gtype.h 等说明的是全局的数据类型，变量等

对于slice、mbcroblock等，没有说明的时候
type一律指的是枚举类型
slice_type等一律指的是读取出来的句法元素