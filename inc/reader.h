#ifndef READER_H__
#define READER_H__

// 缓冲区的字节长度
#define READER_SIZE_BUFFER 1024
// 1从最高位开始的掩码
#define READER_MASK_INIT 0x80U;

#include "gtype.h"
#include <stdio.h>

// reader作为与外界数据交换的中介
// 防止竞争的方法不在这里处理
// 处理的数据流： {[NAL起始码][NAL][NAL起始码][NAL]...}
// 也就是说这里的nal不包含防止竞争位，全部都是NAL真正的数据流
// 数据流入的方式，
class reader
{
private:
    uint8 *data;     // 当前的缓冲区指针
  
    // 这个参数说明当前缓冲区末尾所在的位置
    int pos_max; // 缓冲区的最大字节长度
    // 这两个参数一起使用，说明当前bit在第几字符，这个字符的第几bit
    int pos_char;// 字节所在的位置
    int pos_bits;// 索引所在的位置

    uint8 mask;      // 用来读取数据的 与 操作模

    //buf1是主缓冲区 buf2的作用是备用缓冲，一号缓冲区读到末尾但是还需要读取的时候，就使用二号来扩展
    uint8 *buf1; bool state_buf1;
    uint8 *buf2; bool state_buf2;

    // 文件指针，用来和文件做交换
    FILE* res;

    void bflin(uint8 *data);
    void bflin(FILE *data);

    // 强制刷新1号缓冲区
    void bfrsh();
    // 开启二号缓冲区
    void bopen();
    // 判断缓冲区是否读完
    bool biend();
    // 判断数据流是不是结束-目前空定义
    bool bieof();
    // 初始化字节指针，比特指针，掩模
    void binit();
    // 比特指针、字节指针、掩模同步移动
    void brght();
public:
    reader();
    ~reader();

    //判断是否对齐到字节边界
    bool balgi();
    // 读入接下来的size个bit
    // 但是指针不会移动
    // 现在是空定义，
    uint64 bnext(uint8 size);

    // 以下是指针会发生移动的读取

    // 强制向下一个字节的开始位对齐
    bool bforc_ne();

    
    //读入一个bit
    char bread_bi();
    // 读入当前比特位所在的字节位上的字节
    // 必须要对齐在边界上
    char bread_ch();

    // bi 和 ch 是基本的两个方法，
    // 有基本的边界判断的问题，
    // 以下的方法都是在这两种方法上衍生出来的

    //读入接下来的size个bit
    uint64 bread_bn(uint8 size);
    //signed golomb
    int64  bread_se();
    //unsigned golomb
    uint64 bread_ue();
    //映射指数
    uint64 bread_me(uint16 ChromaArrayType, uint32 mb_type);
    //截断指数
    uint64 bread_te(uint32 range);

    // 以下是上下文适应的读取方式
    // 这两者都是空定义，ce没写，ae在cabac对象里面
    uint64 bread_ce();//上下文自适应的二进制算数熵编码 CAVLC
    uint64 bread_ae();//上下文自适应的可变长熵编码    CABAC
    




    int64  bread_in(uint16 n);
    uint64 bread_un(uint16 n);
    uint64 bread_fn(uint16 n);

};
#endif