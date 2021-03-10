#ifndef NAL_H__
#define NAL_H__

#include "gtype.h"

// NAL_TYPE写出宏定义直接用数值来表示
// 这里宏定义的数据就是真实的参数，前后顺序不变
// 免得枚举转来转去头晕
#define NAL_TYPE_UNDEF  0
#define NAL_TYPE_NONIDR 1
#define NAL_TYPE_SLICEA 2
#define NAL_TYPE_SLICEB 3
#define NAL_TYPE_SLICEC 4
#define NAL_TYPE_IDR    5
#define NAL_TYPE_SEI    6
#define NAL_TYPE_SPS    7
#define NAL_TYPE_PPS    8

class parser;
class decoder;

// nal需要完成的功能，1 2 5以及其信息的解码
class nal
{
private:
    parser *pa;//  解码环境
    decoder *de;// 解码控制器

    void decode_PPS(); // 
    void decode_SPS(); // 
    void decode_PIC(); // 
    void decode_IDR(); // 其实就是在上一个函数上加上了IDR的处理
    void decode_SEI(); // 目前空定义
    
public:
    bool decode();//deocde函数用来解码以下的三个参数
    uint8 forbidden_zero_bit  ;//1
    uint8 nal_ref_idc         ;//2
    uint8 nal_unit_type       ;//5

    // 指明当前nal是不是图像型的nal
    // 在用到data这个指针来访问图像nal时使用这个参数来判断
    bool ispic;

    // 用来表示不同的数据，
    // 如果是参数那么就是参数data(SPS/PPS)，
    // 如果是图像nal那么就是picture*
    void* data; 

    // 由于解码环境、解码控制都是独立于nal（是在多个nal间进行的）
    // 所以需要传递进来
    nal(parser*, decoder*);
    ~nal();
};

#endif