#include "residual.h"

#include "gvars.h"
#include "gchart.h"

#include <math.h>
#include <string.h>


#include "terror.h"
#include "block.h"
#include "macroblock.h"
#include "parser.h"
#include "pps.h"
#include "matrix.h"
#include "pixmap.h"

#include "gfunc.h"

residual::residual(macroblock *m, parser* p)
{
    this->mb = m;
    this->pa = p;


    luma = NULL;
    chroma = NULL;
    codedPatternLuma = m->CodedBlockPatternLuma;
    codedPatternchroma = m->CodedBlockPatternChroma;
    this->TransformBypassModeFlag = m->TransformBypassModeFlag;
    
    cabacFlag = p->pS->pps->entropy_coding_mode_flag;

    // 选择解码方式，确定解码函数
    if(!cabacFlag) residual_block = &residual:: residual_block_cavlc;
    else residual_block = &residual::residual_block_cabac;
    

}
residual::~residual()
{
    // 因为残差数据是由pic管理的

    // 需要释放放着变换系数的block
    if(luma)   delete[] luma;
    if(chroma) delete[] chroma;
}
void residual::decode()
{    
    if(mb->mb_skip_flag) return;
    if(mb->type == I_PCM) return;
    ParseHead();
    DecodeData();
}

void residual::ParseHead()
{
    // 每一个残差的结构都是16x16
    // 每一个残差系数是int32类型的

    if(mb->predmode == Intra_16x16)
    {
        // 对 intra_16x16 来说是 
        // 1个DC， 15个AC
        luma = new block[2]();
        // new 两个指针出来，一个放DC，一个放AC
        luma[COEFF_LUMA_DC16].append(16);
        //parse luma
        // DC DC 系数只有 16 个，之后填到 16x15 个AC系数的开头
        // luma[1].insert(16, 15);
        (this->*residual_block)(
            &luma[COEFF_LUMA_DC16],\
            0x00000030U,\
            0, 15, 16); // 所以这里是 16 个
        // AC
        // AC 系数和 Intra_4x4 一样的解码方法
        luma[COEFF_LUMA_AC16].insert(4); // 
        for(uint8 i8x8 = 0; i8x8 < 4; i8x8++)
        {

            luma[COEFF_LUMA_AC16][i8x8].insert(4, 15);//再分4组，每组数据长度15
            for(uint8 i4x4 = 0; i4x4 < 4; i4x4++)
            {
                if(codedPatternLuma & (1 << i8x8)) // 如果这个区块编码的话，那么会有对应的读取
                    (this->*residual_block)(\
                        &luma[COEFF_LUMA_AC16][i8x8][i4x4],\
                        0x00000130U | (i8x8 * 4 + i4x4),\
                        0,14,15);// 而这里就只有15个
                else  // 不编码就全部都是 0 ， 后面类似于这个的句式语义相同
                    for(uint8_t i = 0; i < 15; i++)
                        luma[COEFF_LUMA_AC16][i8x8][i4x4].set(i, 0);
            }
        }
    }
    else if(mb->predmode == Intra_8x8)
    {
        // 8x8 是分成四块，每一个直接解 64 个系数
        luma = new block[1];
        luma[COEFF_LUMA_8x8].insert(4, 64);
        for(uint8_t i8x8 = 0; i8x8 < 4; i8x8++)
            if(codedPatternLuma & (1 << i8x8))
                (this->*residual_block)(\
                    &luma[COEFF_LUMA_8x8][i8x8],\
                    0x00000530U | (uint32)i8x8,\
                    0, 63, 64);
            else 
                for(uint8_t i = 0; i < 64; i++) 
                    luma[COEFF_LUMA_8x8][i8x8].set(i, 0);
    }
    else // 对于 intra4x4 和 需要残差的帧间 用的是同一种方法
    {
        //一次解完所有的256个变换系数
        // 注意是分成4组，每组再分成4组，按照每小组 16 个系数来解码的
        luma = new block[1];
        luma[COEFF_LUMA_4x4].insert(4);
        for(uint8_t i8x8 = 0; i8x8 < 4; i8x8++)
        {
            luma[COEFF_LUMA_4x4][i8x8].insert(4, 16);
            for(uint8_t i4x4 = 0; i4x4 < 4; i4x4++) 
            {
                if(codedPatternLuma & (1 << i8x8))\
                    (this->*residual_block)(\
                        &luma[COEFF_LUMA_4x4][i8x8][i4x4],\
                        0x00000230U | (uint32)(i8x8 * 4 + i4x4),\
                        0, 15, 16);
                else 
                    for(uint8_t i = 0; i < 16; i++) 
                        luma[COEFF_LUMA_4x4][i8x8][i4x4].set(i, 0);
            }
        }
    }
    //parser chroma
    //0 - DC 1 - AC

        //
    chroma = new block[2];
    if(pa->pV->ChromaArrayType == 1 || pa->pV->ChromaArrayType == 2)
    {
        uint8_t iCbCr = 0;
        uint8_t NumC8x8 = 4 / (pa->pV->SubWidthC * pa->pV->SubHeightC);
        // 实际上 NumC8x8 = 4;
        chroma[COEFF_CHRO_DC].insert(2, 4);
        for(iCbCr = 0; iCbCr < 2; iCbCr++) // read chroma DC level
            if(codedPatternchroma & 3) 
                (this->*residual_block)(\
                &chroma[COEFF_CHRO_DC][iCbCr],\
                0x00000300U | ((uint32)iCbCr << 4),\
                0, 4 * NumC8x8 - 1, 4 * NumC8x8);
            else 
                for(uint8_t i = 0; i < 4 * NumC8x8; i++) 
                    chroma[COEFF_CHRO_DC][iCbCr].set(i, 0);
        chroma[COEFF_CHRO_AC].insert(2);
        for(iCbCr = 0; iCbCr < 2; iCbCr++) // read chroma AC level
        {
            chroma[COEFF_CHRO_AC][iCbCr].insert(NumC8x8);
            for(uint8_t i8x8 = 0; i8x8 < NumC8x8; i8x8++)
            {
                chroma[COEFF_CHRO_AC][iCbCr][i8x8].insert(4,15);
                for(uint8_t i4x4 = 0; i4x4 < 4; i4x4++)
                {
                    if(codedPatternchroma & 2)\
                        (this->*residual_block)(\
                        &chroma[COEFF_CHRO_AC][iCbCr][i8x8][i4x4],\
                        0x00000400U | ((uint32)iCbCr << 4) | (uint32)(i8x8 * 4 + i4x4),\
                        0, 14, 15);
                    else 
                        for(uint8_t i = 0; i < 15; i++) 
                            chroma[COEFF_CHRO_AC][iCbCr][i8x8][i4x4].set(i, 0);
                    // printf(" chroma[%d][%d][%d][%d], addr: %p\n", 1, iCbCr, i8x8, i4x4, &chroma[1][iCbCr][i8x8][i4x4]);
                    }
            }
        }
    }
}
void residual::DecodeData()
{

    if(mb->is_intrapred())
        switch (mb->predmode)
        {
            case Intra_16x16:Decode_Intra16x16(); break;
            case Intra_4x4:  Decode_Intra4x4()  ; break;
            default: break;
        }
    else //if(mb->is_interpred())
    {
        Decode_Intra4x4();
    }
    Decode_Chroma(0);
    Decode_Chroma(1);
}

// CABAC 解码变换系数
// 注意 requestVlaue 使用低位上的 12 个bit（十六进制3个0）
// 反正一直都是要用的，直接写成静态变量了
static int coeffLevel                 [64];
static int significant_coeff_flag     [64];
static int last_significant_coeff_flag[64];
static int coeff_abs_level_minus1     [64];
static int coeff_sign_flag            [64];
void residual::residual_block_cabac(block* bl, uint32 requestVlaue, uint8 startIdx, uint8 endIdx, uint8 length)
{
    int16 i = 0, numCoeff = 0;
    uint32 a= 0 ,b = 0;
    if(length != 64 || pa->pV->ChromaArrayType == 3) 
        bl->coded_block_flag = pa->read_ae(0x00061000U | requestVlaue);
    else bl->coded_block_flag = 1;
    for(i = 0; i < length; i++) 
        coeffLevel[i] = 0;
    if(bl->coded_block_flag)
    {
        numCoeff = endIdx + 1 ;
        i = startIdx ;
        while(i < numCoeff - 1) 
        {
            significant_coeff_flag[i] = pa->read_ae(0x00062000U | requestVlaue | (((uint32)i) << 20));
            if(significant_coeff_flag[i]) {
                last_significant_coeff_flag[i] = pa->read_ae(0x00063000U | requestVlaue | (((uint32)i) << 20));  
                if(last_significant_coeff_flag[i]) 
                    numCoeff = i + 1;
            }
            i++;
        }
        coeff_abs_level_minus1[numCoeff - 1] = pa->read_ae(0x00064000U | requestVlaue | (((uint32)a) << 24) | (((uint32)b )<< 20));
        coeff_sign_flag[numCoeff - 1]        = pa->read_ae(0x00065000U); //use bypass decode
        coeffLevel[numCoeff - 1] = (coeff_abs_level_minus1[numCoeff - 1] + 1) * (1 - 2 * coeff_sign_flag[numCoeff - 1]) ;
        if(coeffLevel[numCoeff - 1] == 1 || coeffLevel[numCoeff - 1] == -1)a++;
        if(coeffLevel[numCoeff - 1] > 1  || coeffLevel[numCoeff - 1] <  -1)b++;
        for(i = numCoeff - 2; i >= startIdx; i--){
            if(significant_coeff_flag[i])
            {                                        //    ab
                coeff_abs_level_minus1[i] = pa->read_ae(0x00064000U | requestVlaue | (((uint32)a) << 24) | (((uint32)b )<< 20));  
                coeff_sign_flag[i]        = pa->read_ae(0x00065000U); // 使用的是旁路解码
                coeffLevel[i] = (coeff_abs_level_minus1[i] + 1) * (1 - 2 * coeff_sign_flag[i]);

                if(coeffLevel[i] == 1 || coeffLevel[i] == -1)a++;
                if(coeffLevel[i] > 1  || coeffLevel[i] <  -1)b++;
            }
        }
    }
    // for (uint16 i = 0; i < length; i++) {bl->set_blockValue(i, coeffLevel[i]);}
    memcpy(bl->value, coeffLevel, length * sizeof(int));

    if(terr.cabac_result_residual())
    {
        printf(">> residu:result of cabac is: \n");
        for (uint16 i = 0; i < length; i++) 
        {printf(" index %2d, value : %d\n", i, coeffLevel[i]);}
    }
}
void residual::residual_block_cavlc(block* bl, uint32 syntaxValue, uint8_t startIdx, uint8_t endIdx, uint8_t length)
{
    // uint8_t i = 0;
    // uint32_t coeff_token = 0;
    // uint8_t suffixLength = 0;
    // int16_t levelCode = 0;
    // uint8_t zerosLeft;

    // int16_t trailing_ones_sign_flag;

    // for(i = 0; i < length; i++) 
    // coeffLevel[i] = 0;
    // coeff_token = parser_g->read_ce();

    // int32_t* levelVal = new int32_t[Get_TotalCoeff(coeff_token)];
    // if(Get_TotalCoeff(coeff_token) > 0)
    // {
    //     if(Get_TotalCoeff(coeff_token) > 10 && Get_TrailingOnes(coeff_token) < 3)  suffixLength = 1;
    //     else suffixLength = 0;
    //     for(i = 0; i < Get_TotalCoeff(coeff_token); i++)
    //     {
    //         if(i < Get_TrailingOnes(coeff_token))
    //         { 
    //             trailing_ones_sign_flag = parser_g->read_ce();
    //             levelVal[i] = 1 - 2 * trailing_ones_sign_flag;
    //         }
    //         else 
    //         { 
    //             level_prefix = parser_g->read_ce();
    //                                                                                         levelCode = (((15 <= level_prefix) ? 15 : level_prefix) << suffixLength);
    //             if(suffixLength > 0 || level_prefix >= 14)
    //             {
    //                 level_suffix = parser_g->read_un(suffixLength);
    //                                                                                         levelCode += level_suffix;
    //             }
    //             if(level_prefix >= 15 && suffixLength == 0)                                 levelCode += 15;
    //             if(level_prefix >= 16)                                                      levelCode += (1 << (level_prefix - 3)) - 4096;
    //             if(i == Get_TrailingOnes(coeff_token) && Get_TrailingOnes(coeff_token) < 3) levelCode += 2;
    //             if(levelCode % 2 == 0)                                                      levelVal[i] = (levelCode + 2) >> 1;
    //             else                                                                        levelVal[i] = (-levelCode - 1) >> 1;
    //             if(suffixLength == 0)                                                       suffixLength = 1;
    //             if(abs(levelVal[i]) > (3 << (suffixLength - 1)) && suffixLength < 6)        suffixLength++;
    //         }
    //     }
    //     if(Get_TotalCoeff(coeff_token) < endIdx - startIdx + 1)
    //     {
    //         total_zeros  = parser_g->read_ce();
    //         zerosLeft = total_zeros;
    //     }
    //     else zerosLeft = 0;
    //     uint16_t* runVal = new uint16_t[Get_TotalCoeff(coeff_token)];
    //     for(i = 0; i < Get_TotalCoeff(coeff_token) - 1; i++)
    //     { 
    //         if(zerosLeft > 0)
    //         {
    //             run_before = parser_g->read_ce();
    //             runVal[i] = run_before;
    //         }
    //         else runVal[i] = 0;
    //         zerosLeft = zerosLeft - runVal[i];
    //     } 
    //     runVal[Get_TotalCoeff(coeff_token) - 1] = zerosLeft ;
    //     int8_t coeffNum;
    //     coeffNum = -1 ;
    //     for(i = Get_TotalCoeff(coeff_token) - 1; i >= 0; i--) 
    //     { 
    //         coeffNum += runVal[i] + 1 ;
    //         coeffLevel[startIdx + coeffNum] = levelVal[i] ;
    //     }
    //     delete[] runVal;
    // }
    // delete[] levelVal;
}



// LevelScale4x4需要的表
uint8_t v[6][3] = {
        {10, 16, 13},    
        {11, 18, 14},
        {13, 20, 16},
        {14, 23, 18},
        {16, 25, 20},
        {18, 29, 23}
    };
int LevelScale4x4(int parameter, int i, int j)
{
    if(i % 2 == 0 && j % 2 ==0) return 16*v[parameter][0];
    else if(i % 2 == 1 && j % 2 ==1) return 16*v[parameter][1];
    else return 16*v[parameter][2];
}
//缩放变换一个4x4矩阵
/// @param mode c relates to a luma residual block coded using Intra_16x16 macroblock prediction mode or c relates to a chromaresidual block.
matrix& residual::ScalingAndTransform_Residual4x4Blocks(int BitDepth, int qP, matrix& c, uint8_t mode)
{
    uint8_t i = 0, j = 0;
    //d_ij
    matrix d(4, 4, 0);
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if(qP >= 24) d[i][j] = (c[i][j] * LevelScale4x4(qP % 6, i, j)) << (qP / 6 - 4);
            else 
            {
                d[i][j] = (c[i][j] * LevelScale4x4(qP % 6, i, j) + (int)powl(2, 3 - qP / 6)) >> (4 - qP / 6);
            }
        }
    }
    if(mode == 1) d[0][0] = c[0][0];
    //e_ij
    matrix e(4,4,0);
    for(i = 0; i < 4; i++) {e[i][0] = d[i][0] + d[i][2];}
    for(i = 0; i < 4; i++) {e[i][1] = d[i][0] - d[i][2];}
    for(i = 0; i < 4; i++) {e[i][2] =(d[i][1] >> 1 )- d[i][3];}
    for(i = 0; i < 4; i++) {e[i][3] = d[i][1] + (d[i][3] >> 1);}

    //f_ij
    matrix f(4,4);
    f = d;//不需要的空间重新利用
    for(i = 0; i < 4; i++) {f[i][0] = e[i][0] + e[i][3];}
    for(i = 0; i < 4; i++) {f[i][1] = e[i][1] + e[i][2];}
    for(i = 0; i < 4; i++) {f[i][2] = e[i][1] - e[i][2];}
    for(i = 0; i < 4; i++) {f[i][3] = e[i][0] - e[i][3];}

    //g_ij
    matrix g(4,4);
    g = e;//不需要的空间重新利用
    for(j = 0; j < 4; j++) {g[0][j] = f[0][j] + f[2][j];}
    for(j = 0; j < 4; j++) {g[1][j] = f[0][j] - f[2][j];}
    for(j = 0; j < 4; j++) {g[2][j] =(f[1][j] >> 1) - f[3][j];}
    for(j = 0; j < 4; j++) {g[3][j] = f[1][j] + (f[3][j] >> 1);}

    //h_ij
    matrix h(4,4);
    h = f;//不需要的空间重新利用
    for(j = 0; j < 4; j++) {h[0][j] = g[0][j] + g[3][j];}
    for(j = 0; j < 4; j++) {h[1][j] = g[1][j] + g[2][j];}
    for(j = 0; j < 4; j++) {h[2][j] = g[1][j] - g[2][j];}
    for(j = 0; j < 4; j++) {h[3][j] = g[0][j] - g[3][j];}
    //这个if已经无法使用
    // if(0)
    // {
    //     cout << ">>residual: d e f  g h:-----------" << endl;
    //     cout << d << endl;
    //     cout << e << endl;
    //     cout << f << endl;
    //     cout << g << endl;
    //     cout << h << endl;
    //     cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << endl;
    // }
    //r_ij
    matrix r(4,4);
    //r = ((h + 32) >> 6);
    r = h;//h的结果直接交给r并在r上运算
    r = (r + 32);
    r = (r >> 6);
    
    c = r;
    return c;
}
//数组变换为矩阵的变换表
//变换矩阵：输入的两个索引分别是
//1   在数组中的索引
//2   0代表row 1代表col
static uint8_t Inverse4x4ZigZag[16][2] = {
    {0,0}, {0,1}, {1,0}, {2,0}, 
    {1,1}, {0,2}, {0,3}, {1,2}, 
    {2,1}, {3,0}, {3,1}, {2,2}, 
    {1,3}, {2,3}, {3,2}, {3,3}
};

//把一个数组[16]变换为一个4x4矩阵并写入到第二个参数的矩阵中
//第一个参数是要变换的数组(int[16])，第二个参数是目标矩阵(4x4)

static int Inverse4x4[16];
void Inverse4x4Scan(int* res, matrix& dst)
{
    memcpy(Inverse4x4, res, 16 * sizeof(int));
    
    uint8_t r = 0, c = 0;
    for (uint8_t i = 0; i < 16; i++)
    {
        r = Inverse4x4ZigZag[i][0];
        c = Inverse4x4ZigZag[i][1];
        dst[r][c] = Inverse4x4[i];
    }
}
void residual::Decode_Intra4x4()
{
    int qP = mb->qp.QPY;
    // matrix residual_Y(16,16,0);
    matrix c(4,4,0);
    matrix *resi = mb->resi;
    int16 sample_result;
    int sizeof4int = sizeof(int) * 4;

    for(uint8_t i8x8 = 0; i8x8 < 4; i8x8++)
    {
        for (uint8_t i4x4 = 0; i4x4 < 4; i4x4++)
        {
            // c 的值的取值范围： -2^(7 + bitDepth), 2^(7 + bitDepth) -1
            // Inverse4x4Scan(luma[0]->get_childBlock(i8x8)->get_childBlock(i4x4)->value, c);
            Inverse4x4Scan(luma[COEFF_LUMA_4x4][i8x8][i4x4].value, c);
            ScalingAndTransform_Residual4x4Blocks(pa->pV->BitDepthY, qP, c);
            
            int row_cur = 0, col_cur = 0;
            InverseRasterScan_luma4x4(4 * i8x8 + i4x4, &col_cur, &row_cur);

            // std::cout << "i8x8 " << (int)i8x8 << " i4x4 " << (int)i4x4 << std::endl\
            // << "row :" << row_cur << " col :" << col_cur << std::endl \
            // << c << std::endl;
            
            // //当前4x4块写入，一共写入16次，256个样点

            memcpy(resi[0][row_cur + 0] + col_cur, c[0], sizeof4int);
            memcpy(resi[0][row_cur + 1] + col_cur, c[1], sizeof4int);
            memcpy(resi[0][row_cur + 2] + col_cur, c[2], sizeof4int);
            memcpy(resi[0][row_cur + 3] + col_cur, c[3], sizeof4int);
           
            // for (uint8_t row = 0; row < 4; row++)
            // {
            //     for (uint8_t col = 0; col < 4; col++)
            //     {
            //         resi[0][row_cur + row][col_cur + col] = c[row][col];
            //     }
            // }
        }
    }
    // cout << (*(mb->pred_Y)) << endl;
}


void residual::Decode_Intra16x16()
{

    /*
    ----DC 缩放变换

    ----DC & AC -> lumaList[16][16] : (combine 系数组合)

        索引类型 DC[i4x4]    AC[i8x8][i4x4]
        lumaList[16][16] = 
        | DC[0]    AC[0][0] AC[0][1] AC[0][2] | DC[1] AC[1][0] ... | DC[2] ... | DC[3] ... |
        | AC[0][3] AC[0][4] AC[0][5] ...      |                    |           |           |
        |          ......                     |         ...        |              
        | AC[0][11]   ...           AC[0][14] |                    |
        ------------------------------------------------------------------------------------
        | DC[4]   ...                         | DC[5]
        | ...                                           ...
        ------------------------------------------------------------------------------------
        | DC[8]                               | DC[9]   ...        | DC[10]... | DC[11] ...|
        |
        ------------------------------------------------------------------------------------
        | ...                                                                    DC[15] ...|
        |                                           ...                                    |
    
    ----lumaList[16][16] -> c : (inverse 逆扫描4x4)
        
        for i8x8 = 0 : 15 
        {
        DC   AC0  AC1  AC2                   DC   AC0  AC3  AC4 
        AC3  AC4  AC5  AC6    inverse 4x4    AC1  AC2  AC5  AC6 
        AC7  AC8  AC9  AC10    ------->      AC7  AC8  AC11 AC12
        AC11 AC12 AC13 AC14                  AC9  AC10 AC13 AC14
        }
        c = lumaList;

    ----c -> r:(缩放变换)
        
        按i8x8进行
        for i8x8 = 0 : 15
        r[i8x8] = ScalingAndTransform(c[i8x8])

    ----r -> residual:（残差赋值）

        residual = r
    */

    int qP = mb->qp.QPY;

    matrix c(4, 4, 0);
    // 逆扫描把一维变成二维
    Inverse4x4Scan(luma[COEFF_LUMA_DC16].value, c); 

    matrix dcY(4,4);// dcY 不需要数据空间，因为马上会被赋值
    if(TransformBypassModeFlag)
        dcY = c;//直接将c的数据交给dcY
    else
    {
        c = ((*pa->matrix_4x4Trans) * c *(*pa->matrix_4x4Trans));
        if(qP >= 36)
            c  = (((c  * LevelScale4x4(qP % 6, 0, 0)) << (qP / 6 - 6)));
        else 
            c  = (((c  * LevelScale4x4(qP % 6, 0, 0)) + (1 << (5 - qP /6))) >> (6 - qP / 6));
        dcY = c;
    }

    if(terr.residual_transcoeff())
    {
        //打印残差变换系数
    }

    //4x4个4x4矩阵
    // luma[0] 也就是DC系数的分布是按照 i8x8 -> i4x4 ->[15] 的顺序来的
    // 读入的时候每一个 block 读入到 一个 int* 中
    array2d<matrix*> lumaResidual(4, 4, NULL);
    // new 空间出来
    for (int i = 0; i < 16; i++) lumaResidual.set(i, new matrix(4,4,0));
    
    // des作为取lumaResidual的临时空间
    matrix* des;

    int residual4x4blk[16];
    block* acblk;
    for (uint8_t i = 0; i < 16; i++)
    {
        // 取出对应位置上的指针
        des = lumaResidual.get(i);
        // cout << "des: " << des << endl;
        //write DC into residual
        // 填入DC系数
        residual4x4blk[0] = dcY.get(i);
        //提前进行了一次预处理得到luma的block(AC系数所在的block)
        acblk = &(luma[COEFF_LUMA_AC16][block4x4Index[i]/4][block4x4Index[i]%4]);
        // 填入后面的15个AC系数
        memcpy(residual4x4blk + 1, acblk->value, 15 * sizeof(int));
        // 逆扫描换到二维空间
        Inverse4x4Scan(residual4x4blk, *des);
        // 缩放变换
        ScalingAndTransform_Residual4x4Blocks(8, qP, *des, 1);
        // cout << "i : " << (int)i << endl << (*des) << endl;
    }
    //4x4个4x4矩阵写入到一个16x16矩阵中去
    matrix result(16, 16, 0);
    int i, j;
    int rs,cs;// rs 是row start地址，cs 是collum start地址
    int sizeof4int = 4 * sizeof(int);
    for (i = 0; i < 4; i++)
    {
        rs = i*4;
        for (j = 0; j < 4; j++)
        {
            des = lumaResidual[i][j];
            cs = j*4;
            memcpy(result[rs + 0] + cs, (*des)[0], sizeof4int);
            memcpy(result[rs + 1] + cs, (*des)[1], sizeof4int);
            memcpy(result[rs + 2] + cs, (*des)[2], sizeof4int);
            memcpy(result[rs + 3] + cs, (*des)[3], sizeof4int);
        }
    }
    
    for (uint8_t i = 0; i < 16; i++){delete lumaResidual.get(i);}

    // result的结果直接给回resi，resi原本的空间释放
    mb->resi[0] = result;
}
void residual::Decode_Chroma(int iCbCr)
{
    //取得当前的色度量化参数
    int qP = mb->qp.QPC_;
    // matrix* c = new matrix(2,2,0);
    // for (uint8_t i = 0; i < 4; i++) (*c).get_value_i(i) = (*chroma[0])[iCbCr]->value[i];
    

    // if(terr.residual_transcoeff())
    //     std::cout <<">>decode: decode residual coef  chroma: " << iCbCr << std::endl << "       input DC:" << std::endl << (*c) << std::endl;


    // (*c)  = (*pa->matrix_2x2Trans * (*c)  * *pa->matrix_2x2Trans);
    // (*c)  = ((((*c)  * LevelScale4x4(qP % 6, 0, 0)) << (qP / 6)) >> 5);
    // matrix* dcC = new matrix(2, 2);
    // (*dcC) << (*c) ;
    // delete c;
    
    // array2d<matrix*>* lumaResidual = new array2d<matrix*>(2,2,NULL);
    // for (uint8_t i = 0; i < 4; i++)
    // {lumaResidual->get_value_i(i) = new matrix(4,4,1);}
    
    // for (uint8_t i = 0; i < 4; i++)
    // {
    //     auto des = lumaResidual->get_value_i(i);
    //     (*des)[0][0] = (*dcC).get_value_i(i);
    //     // cout << (*des) << endl;
    //     for (uint j = 0; j < 15; j++)
    //     //block类的[]运算符是取到子box而不是值value
    //     {(*des).get_value_i(j + 1) = (*(*chroma[1])[iCbCr])[i]->value[j];}

    //     Inverse4x4Scan((*des).data, (*des));
    //     (*des) = ScalingAndTransform_Residual4x4Blocks(8, this->qP, des, 1);

    // }
    // delete dcC;

    // //wtire chroma into out matrix
    // matrix* result = new matrix(8,8,1);
    // for (uint8_t i = 0; i < 8; i++)
    // {
    //     for (uint8_t j = 0; j < 8; j++)
    //     {
    //        (*result)[i][j] = (*(*lumaResidual)[i/4][j/4])[i%4][j%4];
    //     }
    // }
    // for (uint8_t i = 0; i < 4; i++){delete lumaResidual->get_value_i(i);}
    // delete lumaResidual;


    // if(iCbCr == 0) residual_U = result;
    // else residual_V = result;

    // if(terr.residual_transcoeff()) 
    // cout << "      output:" << endl << (*result) << endl;
}
void residual::Decode_Zero()
{
    //块置零
    //数据置零

    // if(mb->is_intrapred())
    // {
    //     if(mb->predmode == Intra_16x16)
    //     {
    //         luma = new block[2];
    //         // luma[1] = new block(16);
    //         luma[0].append(16);
    //         // luma[0] = new block();
    //         luma[0].insert(4);
    //         for (uint8_t i = 0; i < 4; i++){luma[0]->get_childBlock(i)->add_childBlock(4, 0);}

    //     }
    //     else if(mb->predmode == Intra_4x4)
    //     {
    //         luma = new block*[1];
    //         luma[0] = new block();
    //         luma[0]->add_childBlock(4);
    //         for (uint8_t i = 0; i < 4; i++){luma[0]->get_childBlock(i)->add_childBlock(4, 0);}
    //     }
    //     else {
    //         int a = 0;/*这里是8x8块8*/
    //     }
    // }
    // else
    // {
    //     luma = new block*[1];
    //     luma[0] = new block();
    //     luma[0]->add_childBlock(4);
    //     for (uint8_t i = 0; i < 4; i++){luma[0]->get_childBlock(i)->add_childBlock(4, 0);}
    // }
    

    // //亮度的块置零
    // chroma = new block*[2];
    // chroma[0] = new block();
    // chroma[0]->add_childBlock(2, 4);
    // chroma[1] = new block();
    // chroma[1]->add_childBlock(2);
    // for(uint8_t iCbCr = 0; iCbCr < 2; iCbCr++){chroma[1]->add_childBlock(4,0);}

    //样点残差块置零
    // residual_Y = new matrix(16,16,0);
    // residual_U = new matrix(8,8,0);
    // residual_V = new matrix(8,8,0);
    // 本来初始化就是0, 所以这里不用做多余的动作了
}