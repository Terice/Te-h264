#include "block.h"

block::block(int init_length)
{
    childBlockLength = 0;
    childBlock = NULL;
    length = init_length;
    value = new int[init_length];
}
block::block()
{
    childBlockLength = 0;
    childBlock = NULL;
    length = 0;
    value = NULL;
}
block::~block()
{
    if(value) delete[] value;
    if(childBlockLength) delete[] childBlock;
}


bool block::append(int length)
{
    value = new int[length];
}
block& block::operator[](int i)
{
    return childBlock[i];
}
bool block::insert(int length)
{
    childBlockLength = length;
    childBlock = new block[length]();
}
bool block::insert(int length, int data_length)
{
    childBlock = new block[length];
    childBlockLength = length;
    for (int i = 0; i < length; i++)
    {
        childBlock[i].append(data_length);
    }
    
}
int  block::get(int i)
{
    return value[i];
}
bool block::set(int i, int v)
{
    value[i] = v;
    return true;
}