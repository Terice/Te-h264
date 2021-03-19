#ifndef RESIDUAL_H__
#define RESIDUAL_H__
#include "gtype.h"
#include "array2d.h"

class macroblock;
class parser;
class block;
class pixmap;
class matrix;

#define COEFF_LUMA_DC16 1
#define COEFF_LUMA_AC16 0
#define COEFF_LUMA_4x4  0
#define COEFF_LUMA_8x8  0
#define COEFF_CHRO_DC   1
#define COEFF_CHRO_AC   0

//residual 类应该返回一个对象，包含了宏块的残差的所有数据
//1、宏块残差的类型：这里只有三种，分别是AC+DC     4x4     8x8
//然后还应该有一个表示coded_block_flag的标志，
//对于AC+DC 是1+16个，对于4x4 是16个， 对于8x8 是4个。
class residual
{
private:
    macroblock *mb;
    parser *pa;
    bool TransformBypassModeFlag;
    int qp;
    uint8 codedPatternLuma;
    uint8 codedPatternchroma;
    uint8 cabacFlag;

    pixmap *Y;
    pixmap *U;
    pixmap *V;

    void Decode_Intra4x4();
    void Decode_Intra16x16();
    void Decode_Chroma(int iCbCr);
    void Decode_Zero();


    //if luma of intra_16x16 or chroma residual 
    //mode 的意思：如果是16x16帧内预测或者c是色度的矩阵，那么这个值传1，否则不传
    matrix& ScalingAndTransform_Residual4x4Blocks(int BitDepth, int qP, matrix& c_ij, uint8_t mode = 0);
    // 对于CABAC来说， syntaxValue 占掉低位上 3*4 位
    void (residual::*residual_block)(block* bl, uint32 syntaxValue, uint8 startIdx, uint8 endIdx, uint8 length);
           void residual_block_cavlc(block* bl, uint32 syntaxValue, uint8 startIdx, uint8 endIdx, uint8 length);
           void residual_block_cabac(block* bl, uint32 syntaxValue, uint8 startIdx, uint8 endIdx, uint8 length);
public:
    residual(macroblock *m, parser* p);
    ~residual();

    void decode();

    // 解析解码环境（变换系数）
    void ParseHead();
    // 解码残差值
    void DecodeData();

    // luma[0] 16x16 AC
    //          4x4
    //          8x8
    // luma[1] 16x16 DC
    block *luma;
    // chroma[0] chromaDC
    // chroma[1] chromaAC
    block *chroma;

};




#endif