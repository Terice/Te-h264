#include "residual.h"

#include "gvars.h"
#include "gchart.h"

#include "terror.h"
#include "block.h"
#include "macroblock.h"
#include "parser.h"
#include "pps.h"
#include "matrix.h"
#include "pixmap.h"
#include <math.h>
#include <string.h>

residual::residual(macroblock *m, parser* p)
{
    this->mb = m;
    this->pa = p;
    
    codedPatternLuma = m->CodedBlockPatternLuma;
    codedPatternchroma = m->CodedBlockPatternChroma;
    this->TransformBypassModeFlag = m->TransformBypassModeFlag;
    
    cabacFlag = p->pS->pps->entropy_coding_mode_flag;

    // 选择解码方式，确定解码函数
    if(!cabacFlag) residual_block = &residual::residual_block_cavlc;
    else residual_block = &residual::residual_block_cabac;
    

}
residual::~residual()
{
    // 并没有什么需要释放的数据
    // 因为残差数据是由pic管理的
}
void residual::decode()
{
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
        
        // new 两个指针出来，一个放DC，一个放AC
        luma[0].append(16);
        //parse luma
        // DC DC 系数只有 16 个，之后填到 16x15 个AC系数的开头
        // luma[1].insert(16, 15);
        (this->*residual_block)(
            &luma[0],\
            0x00000030U,\
            0, 15, 16); // 所以这里是 16 个
        // AC
        // AC 系数和 Intra_4x4 一样的解码方法
        luma[1].insert(4); // 
        for(uint8 i8x8 = 0; i8x8 < 4; i8x8++)
        {
            luma[1][i8x8].insert(4, 15);//再分4组，每组数据长度15
            for(uint8 i4x4 = 0; i4x4 < 4; i4x4++)
            {
                if(codedPatternLuma & (1 << i8x8)) // 如果这个区块编码的话，那么会有对应的读取
                    (this->*residual_block)(\
                        &luma[1][i8x8][i4x4],\
                        0x00000130U | (i8x8 * 4 + i4x4),\
                        0,14,15);// 而这里就只有15个
                else  // 不编码就全部都是 0 ， 后面类似于这个的句式语义相同
                    for(uint8_t i = 0; i < 15; i++)
                        luma[1][i8x8][i4x4].set(i, 0);
            }
        }
    }
    else if(mb->predmode == Intra_8x8)
    {
        // 8x8 是分成四块，每一个直接解 64 个系数
        luma = new block[1];
        luma[0].insert(4, 64);
        
        for(uint8_t i8x8 = 0; i8x8 < 4; i8x8++)
            if(codedPatternLuma & (1 << i8x8))
                (this->*residual_block)(\
                    &luma[0][i8x8],\
                    0x00000530U | (uint32)i8x8,\
                    0, 63, 64);
            else 
                for(uint8_t i = 0; i < 64; i++) 
                    luma[0][i8x8].set(i, 0);
    }
    else // 对于 intra4x4 和 需要残差的帧间 用的是同一种方法
    {
        //一次解完所有的256个变换系数
        // 注意是分成4组，每组再分成4组，按照每小组 16 个系数来解码的
        luma = new block[1];
        luma[0].insert(4);
        for(uint8_t i8x8 = 0; i8x8 < 4; i8x8++)
        {
            luma[0][i8x8].insert(4, 16);
            for(uint8_t i4x4 = 0; i4x4 < 4; i4x4++) 
            {
                if(codedPatternLuma & (1 << i8x8))\
                    (this->*residual_block)(\
                        &luma[0][i8x8][i4x4],\
                        0x00000230U | (uint32)(i8x8 * 4 + i4x4),\
                        0, 15, 16);
                else 
                    for(uint8_t i = 0; i < 16; i++) 
                        luma[0][i8x8][i4x4].set(i, 0);
            }
        }
    }
    //parser chroma
    //0 - DC 1 - AC
    chroma = new block[2];
    if(pa->pV->ChromaArrayType == 1 || pa->pV->ChromaArrayType == 2)
    {
        uint8_t iCbCr = 0;
        uint8_t NumC8x8 = 4 / (pa->pV->SubWidthC * pa->pV->SubHeightC);
        // 实际上 NumC8x8 = 4;
        chroma[0].insert(2, 4);
        for(iCbCr = 0; iCbCr < 2; iCbCr++) // read chroma DC level
            if(codedPatternchroma & 3) 
                (this->*residual_block)(&chroma[0][iCbCr],\
                0x00000300U | ((uint32)iCbCr << 4),\
                0, 4 * NumC8x8 - 1, 4 * NumC8x8);
            else 
                for(uint8_t i = 0; i < 4 * NumC8x8; i++) 
                    chroma[0][iCbCr].set(i, 0);
        chroma[1].insert(2);
        for(iCbCr = 0; iCbCr < 2; iCbCr++) // read chroma AC level
        {
            for(uint8_t i8x8 = 0; i8x8 < NumC8x8; i8x8++)
            {
                chroma[1][iCbCr].insert(NumC8x8, 15);
                for(uint8_t i4x4 = 0; i4x4 < 4; i4x4++)
                    if(codedPatternchroma & 2)\
                        (this->*residual_block)(&chroma[1][iCbCr][i4x4],\
                        0x00000400U | ((uint32)iCbCr << 4) | (uint32)(i8x8 * 4 + i4x4),\
                        0, 14, 15);
                    else 
                        for(uint8_t i = 0; i < 15; i++) 
                            chroma[1][iCbCr][i4x4].set(i, 0);
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
void residual::residual_block_cabac(block* bl, uint32 requestVlaue, uint8 startIdx, uint8 endIdx, uint8 length)
{
    int16 i = 0, numCoeff = 0;
    int *coeffLevel                   = new int[length];
    int *significant_coeff_flag       = new int[length];
    int *last_significant_coeff_flag  = new int[length];
    int *coeff_abs_level_minus1       = new int[length];
    int *coeff_sign_flag              = new int[length];
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
    delete[] coeffLevel                   ;
    delete[] significant_coeff_flag       ;
    delete[] last_significant_coeff_flag  ;
    delete[] coeff_abs_level_minus1       ;
    delete[] coeff_sign_flag              ;
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
    r += 32;
    r >>= 6;
    
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
void Inverse4x4Scan(int* res, matrix& dst)
{
    int *cur = NULL;
    cur = res;//将res指针给cur
    
    uint8_t r = 0, c = 0;
    for (uint8_t i = 0; i < 16; i++)
    {
        r = Inverse4x4ZigZag[i][0];
        c = Inverse4x4ZigZag[i][1];
        dst[r][c] = cur[i];
    }
}
void residual::Decode_Intra4x4()
{
    int qP = mb->qp.QPY_;
    // matrix residual_Y(16,16,0);
    matrix c(4,4,0);
    matrix *resi = mb->resi;
    int16 sample_result;
    // point start = {.x = 0, .y = 0};
    // area  selec = {.w = 4, .h = 4};

    for(uint8_t i8x8 = 0; i8x8 < 4; i8x8++)
    {
        for (uint8_t i4x4 = 0; i4x4 < 4; i4x4++)
        {
            // c 的值的取值范围： -2^(7 + bitDepth), 2^(7 + bitDepth) -1
            // Inverse4x4Scan(luma[0]->get_childBlock(i8x8)->get_childBlock(i4x4)->value, c);
            c.from(luma[0][i8x8][i4x4].value, 16);
            c.inverse4x4();
            ScalingAndTransform_Residual4x4Blocks(pa->pV->BitDepthY, qP, c);
            
            int row_cur = 0, col_cur = 0;
            mb->get_PosByIndex(4 * i8x8 + i4x4, row_cur, col_cur);
            // start.x = col_cur; start.y = row_cur;
            // // 从 residual_Y 继续选择数据构造一个子 pixmap 
            // pixmap tmp(residual_Y, start, selec);
            // // cout << "i8x8" << (int)i8x8 << " i4x4" << (int)i4x4 << endl << "row " << row_cur << "col " << col_cur << endl << c << endl;
            // //当前4x4块写入，一共写入16次，256个样点
           
            for (uint8_t row = 0; row < 4; row++)
            {
                for (uint8_t col = 0; col < 4; col++)
                {
                    resi[0][row_cur + row][col_cur + col] = c[row][col];
                }
            }
        }
    }
    // cout << (*(mb->pred_Y)) << endl;
}


void residual::Decode_Intra16x16()
{

    int qP = mb->qp.QPY_;

    matrix c(4, 4, 0);
    Inverse4x4Scan(luma[1].value, c);
    //Intra_16x16 DC transform and scaling
    // c  = (*decoder->matrix_4x4Trans * c  * *decoder->matrix_4x4Trans);
    // matrix tmp(4,4,0);
    c = (*pa->matrix_4x4Trans) * c *(*pa->matrix_4x4Trans);

    //原本使用符号重载的抽象公式，但是每次都需要值拷贝两次，所以拆开成多个公式，直接在左值做修改，
    if(qP >= 36)
    {
        c  = ((c  * LevelScale4x4(qP % 6, 0, 0)) << (qP / 6 - 6));
    }
    else 
    {
        c  = (((c  * LevelScale4x4(qP % 6, 0, 0)) + (1 << (5 - qP /6))) >> (6 - qP / 6));
        // c *= LevelScale4x4(qP % 6, 0, 0);
        // c += (1 << (5 - qP /6));
        // c >>= (6 - qP / 6);
    }

    if(terr.residual_transcoeff())
    {
        //打印残差变换系数
    }

    matrix dcY(4,4);
    dcY = c;//直接将c的数据交给dcY


    //4x4个4x4矩阵
    array2d<matrix*> lumaResidual(4, 4, NULL);
    matrix *tmp;;
    for (uint8_t i = 0; i < 16; i++)
    {
        lumaResidual.set(i, new matrix(4,4,0)) ; 
    }    
    for (uint8_t i = 0; i < 16; i++)
    {
        matrix* des = lumaResidual.get(i);
        // cout << "des: " << des << endl;
        //write DC into residual
        (*des)[0][0] = dcY.get(i);
        //write AC into residual

        //提前进行了一次预处理得到luma的block(AC系数所在的block)
        block* bl = &(luma[0][block4x4Index[i]/4][block4x4Index[i]%4]);
        //填入AC系数的时候，AC系数不是按扫描顺序填入DC之后的位置的。而是按照4x4逆扫描。
        //也就是说：读取AC的值的时候按逆扫描来，填入的时候按照光栅扫描填入就行了
        
        // for(uint8_t j = 1; j < 16; j++)
        // {
        //     des->set(j, bl->value[j]);
        // }
        des->from(bl->value, 1, 15);

        //inverse scan AC+DC coefficents
        des->inverse4x4();
        // Inverse4x4Scan((*des).data, (*des));
        ScalingAndTransform_Residual4x4Blocks(8, qP, *des, 1);
        // cout << "i : " << (int)i << endl << (*des) << endl;
    }
    //4x4个4x4矩阵写入到一个16x16矩阵中去
    matrix result(16, 16, 0);
    for (uint8_t i = 0; i < 16; i++)
    {
        for (uint8_t j = 0; j < 16; j++)
        {
            result[i][j] = (*lumaResidual.get(i/4, j/4))[i%4][j%4];
        }
    }
    for (uint8_t i = 0; i < 16; i++){delete lumaResidual.get(i);}

    // result
    mb->resi[0] = result;
}
void residual::Decode_Chroma(int iCbCr)
{
    //取得当前的色度量化参数
    int qP = mb->qp.QPC_;
    matrix* c = new matrix(2,2,0);
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