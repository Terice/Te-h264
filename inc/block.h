#ifndef BLOCK_H__
#define BLOCK_H__
#include "array2d.h"
#include "gtype.h"

class block
{
private:
    block** childBlock;
    int length;
    int childBlockLength;
public:
    int32* value;
    uint8 coded_block_flag;
    //添加count个长度为init_length的block作为当前block的子block
    bool add_childBlock(unsigned int count, int init_length);
    //添加count个空block作为当前block的子block
    bool add_childBlock(unsigned int count);
    //得到第index个block的索引
    block* get_childBlock(int index);
    //设置索引为i处的值为value
    bool set_blockValue(int i, int value);

    //return block*   not value
    block& operator[](int i);

    block(int init_length);
    block();
    ~block();
};




#endif