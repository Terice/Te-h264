#ifndef BLOCK_H__
#define BLOCK_H__
#include "array2d.h"
#include "gtype.h"

class block
{
private:
    block* childBlock;
    int length;
    int childBlockLength;
public:
    int32* value;
    uint8 coded_block_flag;

    //return child block   not value
    // 得到子box
    block& operator[](int i);

    // 增加数据长度到length
    bool append(int length);

    // 添加 length 个box
    bool insert(int length);
    // 添加 length 个box， 每个 box 带长度数据
    bool insert(int length, int data_length);
    // 得到 i 处的值
    int get(int i);
    // 设置 i 处的值
    bool set(int i, int v);

    block(int init_length);
    block();
    ~block();
};




#endif