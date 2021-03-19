#ifndef PARSER_H__
#define PARSER_H__
#include "gtype.h"

class reader;
class cabac;
class nal;
class SPS;
class PPS;
class slice;
class picture;
#include "matrix.h"

// 解码参数
typedef struct ParametersV__
{
    uint32 ChromaArrayType           ;
    uint32 MaxFrameNum               ;
    
    uint32 PicHeightInMbs;           ;
    uint32 PicWidthInMbs             ;
    uint32 PicHeightInMapUnits       ;
    uint32 FrameHeightInMbs          ;
    uint32 PicSizeInMapUnits         ;
    uint32 PicSizeInMbs              ;


    uint32 BitDepthY                 ;
    uint32 BitDepthC                 ;
    uint32 QpBdOffsetY               ;
    uint32 QpBdOffsetC               ;

    uint32 sliceGroups               ;
    uint32 numRefIdx10DefaultActive  ;
    uint32 numRefIdx11DefaultActive  ;

    uint32 MbWidthInChroma           ;
    uint32 MbHeightInChroma          ;

    uint8 SubWidthC               = 1;
    uint8 SubHeightC              = 1;
}ParametersV;
// 序列参数集、 图像参数集
typedef struct ParametersS__
{
    SPS* sps;
    PPS* pps;
}ParametersS;


class parser
{
private:
    // reader 用来读取不需要上下文的句法元素
    reader* bitstream;
    // cabac 用来读取ae算子的据发元素，
    // 因为其需要用到上下文和解码环境
    cabac* cabac_core;
public:
    parser(FILE *);
    ~parser();

    nal*   cur_nal;
    slice* cur_slice;
    // slice 的计数器，
    int index_cur_slice;


    // 可能要使用到的解码变量：解码环境
    ParametersV* pV;
    // 解码要用到的参数集
    ParametersS* pS;

    // 注意以下两个函数做sps和pps的内存控制

    // 刷新序列参数集
    bool update_ps(SPS* s);
    // 刷新图像参数集
    bool update_ps(PPS* p);

    // 寻找下一个nal
    bool find_nextNAL();

    // // 判断缓冲区是否字节对齐
    // bool  algi();
    // 读入一个bit
    bool  read_bi();
    //从当前缓冲区中读入一个char，会强制对齐
    short read_ch();
    // 强制对齐
    bool  read_al();

    //uint64 next(uint32 size);

    // 读入size个bit，解释为无符号数
    // 目前空定义
    uint64 read_un(int size);
    // 读入size个bit，解释为有符号数
    // 目前空定义
    int64  read_sn(int size);

    // 无符号golomb
    uint64 read_ue();
    // 有符号golomb
    int64  read_se();
    // 映射指数
    uint64 read_me(int mb_type);
    // 截断指数
    uint64 read_te(int range);
    // CAVLC
    // 空定义
    uint64 read_ce();
    // CABAC
    // 
    /// @param syntaxelement 即句法元素，组织结构见gchart里面的说明
    uint64 read_ae(uint32 syntaxelement);
    //以熵编码标志位做选择的两种读取方式
    uint16 read_12();


    void set_cabac_slice_new(picture* pic, slice *sl);
    void set_cabac_slice_end();

    matrix* matrix_4x4Trans;
    matrix* matrix_2x2Trans;
};



#endif