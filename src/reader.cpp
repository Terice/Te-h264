#include "reader.h"

#define ANNEX_B

#include "reader.h"
#include <cmath>
#include <assert.h>



reader::reader()
{
    mask = READER_MASK_INIT;
    pos_char = 0;
    pos_bits = 0;
    pos_max  = 0;
    

    buf2 = new uint8[READER_SIZE_BUFFER];
    state_buf2 = false;
    buf1 = new uint8[READER_SIZE_BUFFER];
    state_buf1 = true;
    data = buf1;

    bfrsh();
}
reader::~reader()
{
    if(buf1) delete[] buf1;
    if(buf2) delete[] buf2;
}

//--------局部函数

//--------

//-------- 成员函数

inline void reader::binit()
{
    pos_char = 0;
    pos_bits = 0;
    mask = READER_MASK_INIT;
}
inline bool reader::biend()
{
    return pos_char >= pos_max ? true : false;
}
inline void reader::bfrsh()
{
    // 初始化所有指针
    binit();
    // 备用缓冲区在使用说明其中是接下来的数据
    if(state_buf2) {bflin(buf2); state_buf2 = false;}
    // 否则从文件指针中来获得数据
    else bflin(res);
}

void reader::bflin(uint8 *data)
{

}
void reader::bflin(FILE *res)
{
    unsigned long re = 0;
#ifndef ANNEX_B
    if(res)
        re = fread(buf1, sizeof(char), READER_SIZE_BUFFER, res);
    else re = 0;
#else
    if(res)
    {
        int state = 0;//去掉防止竞争字节的状态机
        int ch = 0;
        for (size_t i = 0; i < READER_SIZE_BUFFER; i++)
        {
            ch = fgetc(res);
            if(ch == EOF) break;

            //0表示正常，1表示出现第一个0x00， 2表示出现第二个0x00并且等待下一个03   3表示出现03   4表示出现 0x01 或者 0x02 或者 0x03
            switch (state)
            {
                case 0:if(ch == 0) state = 1; break;
                case 1:if(ch == 0) state = 2; else state = 0; break;
                case 2:if(ch == 3) state = 3; else if(ch == 0) state = 2; else state = 0; break;//如果出现03那么下一个状态，如果当前仍然是0，那么维持状态，否则解除状态
                case 3:if(ch == 1 || ch == 2 || ch == 3 || ch == 0) state = 4; else state = 0; break;
                default:state = 0; break;
            }
            //如果出现防止竞争序列，索引后退一个char，当前字符读入0x03位置上，然后count自增
            if(state == 4)
            {
                re--;
                state = 0;
            }
            data[re] = (unsigned char)ch;
            re++;
        }
        //如果最后一个字节是危险的字节，
        //那么尝试读取下一个字符来判断这个字节是否应该去掉
        if(state == 3)
        {
            ch = fgetc(res);
            if(ch == 1 || ch == 2 || ch == 3 || ch == 0)
            {
                data[READER_SIZE_BUFFER - 1] = (unsigned char)ch;
                state = 0;
            }
            else fseek(res, -1L, SEEK_CUR);
        }
    }
    else
        re = 0;
#endif

    pos_max = re;
}
inline void reader::bopen()
{
    fread(buf2, sizeof(char), READER_SIZE_BUFFER, (FILE*)res);
    state_buf2 = true;
}

inline void reader::brght()
{
    ++pos_bits %= 8;
    mask >>= 1;
    if(!mask) 
    {
        mask = READER_MASK_INIT;
        pos_char++;
    }
}

inline bool reader::balgi()
{
    // pos_bits不是0那么说明就没在字节边界，
    if(!pos_bits) return true;
    else return false;
}

bool reader::bforc_ne()
{
    if(!pos_bits) return true;
    pos_bits = 0;
    pos_char++;
    if(biend()) {bfrsh();} // 如果到达缓冲区末尾，那么刷新缓冲区，
}
char reader::bread_bi()
{
    if(biend()) bfrsh();
    unsigned char cur = data[pos_char];
    unsigned char result = cur & mask;
    brght();
}
char reader::bread_ch()
{
    assert(balgi());

    if(biend()) bfrsh();
    // pos_char++ 是因为读完这个字节需要往下移动，
    // 但是因为已经对齐，所以比特可以不用管
    return data[pos_char++];
}
uint64 reader::bread_bn(uint8 size)
{
    // 以下两个变量能够表示这次读取有多少bit有多少字节，
    int s_char = size / 8;
    int s_bits = size % 8;
    uint64 result = 0UL;
    
    // 这一块自认为改进的还比较好看呢哈哈。
    while(!balgi()) // 先将开头未对齐的比特位按照位运算读出来
    {
        result <<= 1;
        result |= bread_bi();
        s_bits--;
    }
    // s_bits 小于0，说明需要修正，因为此时读取的位数是整字节，但是指针没有对齐到字节上
    // 修正之后就得到了末尾的未对齐比特数
    if(s_bits < 0){s_bits += 8; s_char -= 1;}
    while(s_char)
    {
        result <<= 8;
        result |= bread_ch();
        s_char--;
    }
    // 然后读取末尾未对齐的比特数
    while(s_bits)
    {
        result <<= 1;
        result |= bread_bi();
        s_bits--;
    }
    return result;
}


int64  reader::bread_se()
{    
    
    float num_cur = 0;
    int64 result = 0;

    num_cur = bread_ue();
    result = (int64)pow((-1), (num_cur + 1)) * (int64)(ceil(num_cur / 2));
    
    return result;
}
uint64 reader::bread_ue()
{
    uint16 M = 0;
    uint32 M_cur = 0;
    uint32 num_cur = 0;
    uint64 result = 0;

    while ((num_cur = bread_bi()) == 0) M++;
    result += (num_cur << M);
    for (M_cur = M; M_cur > 0 && M > 0; M_cur--)
    {
        num_cur = bread_bi();
        result += num_cur << (M_cur - 1);
    }
    
    result -= 1;
    return result;
}


static const uint16 me_chart12[48][2] = {
    { 47, 0  }, { 31, 16 }, { 15, 1  }, {  0, 2  }, 
    { 23, 4  }, { 27, 8  }, { 29, 32 }, { 30, 3  }, 
    {  7, 5  }, { 11, 10 }, { 13, 12 }, { 14, 15 }, 
    { 39, 47 }, { 43, 7  }, { 45, 11 }, { 46, 13 }, 
    { 16, 14 }, {  3, 6  }, {  5, 9  }, { 10, 31 }, 
    { 12, 35 }, { 19, 37 }, { 21, 42 }, { 26, 44 }, 
    { 28, 33 }, { 35, 34 }, { 37, 36 }, { 42, 40 }, 
    { 44, 39 }, {  1, 43 }, {  2, 45 }, {  4, 46 }, 
    {  8, 17 }, { 17, 18 }, { 18, 20 }, { 20, 24 }, 
    { 24, 19 }, {  6, 21 }, {  9, 26 }, { 22, 28 }, 
    { 25, 23 }, { 32, 27 }, { 33, 29 }, { 34, 30 }, 
    { 36, 22 }, { 40, 25 }, { 38, 38 }, { 41, 41 } 
};
static const uint16 me_chart03[16][2] = {
    { 15, 0 }, {  0, 1 }, {  7, 2 }, { 11, 4 }, { 13, 8 },
    { 14, 3 }, {  3, 5 }, {  5, 10}, { 10, 12}, { 12, 15}, 
    {  1, 7 }, {  2, 11}, {  4, 13}, {  8, 14}, {  6, 6 },
    {  9, 9 }
};
uint64 reader::bread_me(uint16 ChromaArrayType, uint32 mb_type)
{
    uint64 result;
    result = bread_ue();
    if(ChromaArrayType == 0 || ChromaArrayType == 3) return me_chart03[result][mb_type];
    else return me_chart12[result][mb_type];
}
uint64 reader::bread_te(uint32 range)
{
    if(range > 1) return bread_ue();
    else 
    {
        if(bread_bi() == 1) return 0;
        else return 1;
    }
}

uint64 reader::bnext(uint8 size)
{

}