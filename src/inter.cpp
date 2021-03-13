#include "gfunc.h"
#include "gchart.h"
#include "gtype.h"
#include <math.h>

#include "matrix.h"
#include "slice.h"
#include "macroblock.h"
#include "picture.h"
#include "decoder.h"
#include "parser.h"
#include "sps.h"

// void Weight_CoefficWeight(bool is_explicit, matrix& m0, matrix& m1, matrix& out, int , int, bool flag_0, bool falg_1);
// void Weight_defaultWeight(                  matrix& m0, matrix& m1, matrix& out,            bool flag_0, bool falg_1);


int Prediction_Inter_Direct(\
    macroblock* current,\
    int mbPartIdx, int subMbPartIdx,\
    parser* pa, decoder *de, picture* pic\
)
{
    int mvCol[2] = {0,0};
    int refIdxCol = 0;
    //推导并置pic，和并置宏块
    picture* colPic = NULL;
    macroblock* mbAddrCol = NULL;

    MotionVector **mv_l0 = current->inter->mv.mv_l0;
    MotionVector **mv_l1 = current->inter->mv.mv_l1;
    int8* ref_idx_l0 = current->inter->ref_idx_l0;
    int8* ref_idx_l1 = current->inter->ref_idx_l1;
    bool* predFlagL0 = current->inter->predFlagL0;
    bool* predFlagL1 = current->inter->predFlagL1;

    //临时采用的推导方式
    
    colPic = de->list_Ref1[0];//选择参考表1的第一帧作为并置pic
    //选择和当前宏块同一位置的宏块作为并置宏块
    mbAddrCol = colPic->mb->get(current->pos.x, current->pos.y);

    int luma4x4BlkIdx = 0;//用来推导子块索引
    if(pa->pS->sps->direct_8x8_inference_flag) luma4x4BlkIdx = 5*mbPartIdx;
    else luma4x4BlkIdx = 4*mbPartIdx + subMbPartIdx;

    if(mbAddrCol->is_intrapred()) {mvCol[0] = 0; mvCol[1] = 0; refIdxCol = -1;}
    else 
    {
    }

    //时空预测
    // if(up_slice->ps->direct_spatial_mv_pred_flag)//空间预测
    // {
        //这里有两种情况
        //1、宏块随着周围的宏块一起运动
        pic->neighbour_motionvector(current, mbPartIdx, subMbPartIdx, 0, mv_l0);
        pic->neighbour_motionvector(current, mbPartIdx, subMbPartIdx, 1, mv_l1);
        //2、宏块静止不动

        //参考索引简单都设置为0
        ref_idx_l0[mbPartIdx] = 0;
        ref_idx_l1[mbPartIdx] = 0;

        //是必须会有一个参考方向的，要么双向或者单向参考，不会没有参考方向
        if(ref_idx_l0[mbPartIdx] >= 0 && ref_idx_l1[mbPartIdx] >= 0)    
        {
            predFlagL0[mbPartIdx] = 1; predFlagL1[mbPartIdx] = 1;
        }
        else if(ref_idx_l0[mbPartIdx] >= 0 && ref_idx_l1[mbPartIdx] < 0)
        {
            predFlagL0[mbPartIdx] = 1; predFlagL1[mbPartIdx] = 0;
        }
        else //if(ref_idx_l0[mbPartIdx] < 0 && ref_idx_l1[mbPartIdx] >= 0)
        {
            predFlagL0[mbPartIdx] = 0; predFlagL1[mbPartIdx] = 1;
        }
    // }
    // else//时间预测(还没有写，所以全部是采用空间预测随周围运动)
    // {
    //    
    // }


    //
    //对于 B_Direct_8x8 这里解码完毕
    //对于 B_Direct_16x16 B_Skip 这个函数需要分别执行4次
    // 这里指明了subMbPartIdx 所以是针对与子块来说的
    // 也就是说，如果需要重复解码的话，需要在这个函数的外面进行，而不是在这里
    //到这里运动矢量、参考索引、预测标志位 就已经解码完毕

    

    return 1;
}
// int Prediction_Inter_LumaSampleInterpolation(int xIntL, int yIntL, int xFracL, int yFracL, picture* ref_pic)

void Weight_defaultWeight(matrix& m0, matrix& m1, matrix& out, bool flag_0, bool flag_1)
{
    matrix zero(out.w, out.h, 0);
    if(flag_0 && flag_1)
    {out = (out + (m0 + m1));}
    else
    {out = (out + ((flag_0?m0:zero) + (flag_1?m1:zero)));}
}
// is_explicit 指明是否是显式加权
// 显式加权会用到slice中的加权参数
void Weight_CoefficWeight(
    bool is_explicit,\
    matrix& m0, matrix& m1, matrix& out, \
    int refidx_l0, int refidx_l1, \
    bool   flag_0, bool   flag_1,\
    int BitDepth_Y,\
    PredWeight* pw\
)
{

    int logWD_C= 5;
    int W_0C   = 32;
    int W_1C   = 32;
    int O_0C   = 0;
    int O_1C   = 0;
    if(is_explicit)
    {
        logWD_C= pw->luma_log2_weight_denom;
        W_0C   = pw->luma_weight_l0[refidx_l0];
        O_0C   = pw->luma_offset_l0[refidx_l0] * (1 << (BitDepth_Y - 8));
        if(flag_1)
        {
            W_1C   = pw->luma_weight_l1[refidx_l1];
            O_1C   = pw->luma_offset_l1[refidx_l1] * (1 << (BitDepth_Y - 8));
        }
    }
    if(flag_0 && !flag_1)
    {
        for (uint8_t r = 0; r < out.h ; r++)
        {
            for (uint8_t c = 0; c < out.w; c++)
            {
                if(logWD_C >= 1) 
                    out[r][c] = Clip1Y(   ((int)((m0[r][c] * W_0C) + powl(2, logWD_C - 1)) >> logWD_C) + O_0C, BitDepth_Y);
                else out[r][c] = Clip1Y(m0[r][c] * W_0C + O_0C, BitDepth_Y);
            }
        }
    }
    else if(!flag_0 && flag_1)
    {
        for (uint8_t r = 0; r < out.h ; r++)
        {
            for (uint8_t c = 0; c < out.w; c++)
            {
                if(logWD_C >= 1)
                    out[r][c] = Clip1Y(   ((int)((m1[r][c] * W_1C) + powl(2, logWD_C - 1)) >> logWD_C) + O_1C, BitDepth_Y);
                else out[r][c] = Clip1Y(m0[r][c] * W_1C + O_1C, BitDepth_Y);
            }
        }
    }
    else
    {
        for (uint8_t r = 0; r < out.h ; r++)
        {
            for (uint8_t c = 0; c < out.w; c++)
            {
                out[r][c] = Clip1Y(   ((int)((m0[r][c] * W_0C) + ((m1[r][c] * W_1C)) + powl(2, logWD_C)) >> (logWD_C + 1)) + ((O_1C + O_0C + 1) >> 1), BitDepth_Y);
            }
        }
    }  
}

static int  A__ = 0, B__ = 0, C__ = 0, D__ = 0, 
            E__ = 0, F__ = 0, G__ = 0, H__ = 0, 
            I__ = 0, J__ = 0, K__ = 0, L__ = 0, 
            M__ = 0, N__ = 0, P__ = 0, Q__ = 0, 
            R__ = 0, S__ = 0, T__ = 0, U__ = 0;
static int* sampleX[20] = {
    &A__,&B__,&C__,&D__,\
    &E__,&F__,&G__,&H__,\
    &I__,&J__,&K__,&L__,\
    &M__,&N__,&P__,&Q__,\
    &R__,&S__,&T__,&U__};
//分数像素内插，大写字母都是整数像素，小写字母都是分数像素
int Prediction_Inter_LumaSampleInterpolation(\
    int xIntL,  int yIntL,\
    int xFracL, int yFracL,\
    int PicWidthInMbs, int PicHeightInMbs,\
    picture* ref_pic, int BitDepthY)
{
    // int A = 0, B = 0, C = 0, D = 0, E = 0, F = 0, G = 0, H = 0, I = 0, J = 0,\
    //     K = 0, L = 0, M = 0, N = 0, P = 0, Q = 0, R = 0, S = 0, T = 0, U = 0;
    // int* sampleX[20] = {&A,&B,&C,&D,&E,&F,&G,&H,&I,&J,&K,&L,&M,&N,&P,&Q,&R,&S,&T,&U};
    int PicWidthInSample  = PicWidthInMbs  * 16;
    int PicHeightInSample = PicHeightInMbs * 16;
    auto func = [xIntL, yIntL, ref_pic, PicWidthInSample, PicHeightInSample](int xDZL, int yDZL)->int{
        int sample = 0;
        int xZL = 0, yZL = 0;

        xZL = Clip3(0, PicWidthInSample -1, xIntL + xDZL);
        yZL = Clip3(0, PicHeightInSample -1, yIntL + yDZL);
        int8 data_out;
        sample = (int)(ref_pic->cons->get(xZL, yZL));
        return sample;
    };
    //如果分数都是0，那么直接返回G样点，不做后续的计算
    G__ = func(xyDZL[6][0], xyDZL[6][1]);
    if(xFracL == 0 && yFracL == 0) return G__;

    for (uint8_t i = 0; i < 20; i++)
    {
        *(sampleX[i]) = func(xyDZL[i][0], xyDZL[i][1]);
    }

    int A1 = func(-2,-2), A0 = func(-1, -2), B0 = func(2, -2), B1 = func(3, -2);
    int C1 = func(-2,-2), C0 = func(-1, -2), D0 = func(2, -2), D1 = func(3, -2);
    int R1 = func(-2, 2), R0 = func(-1,  2), S0 = func(2, -2), S1 = func(3, -2);
    int T1 = func(-2, 3), T0 = func(-1,  3), U0 = func(2, -3), U1 = func(3, -3);

    // printf(">>Inter: insert sample:\n");
    // printf("        int pos :xIntL  yIntL  (%3d, %3d)\n", xIntL, yIntL);
    // printf("        fra pos :xFracL yFracL (%3d, %3d)\n", xFracL, yFracL);
    // printf("        %d, %d,  %d, %d, %d, %d\n", A1, A0, A, B, B0, B1);
    // printf("        %d, %d,  %d, %d, %d, %d\n", C1, C0, C, D, D0, D1);
    // printf("        %d, %d, [%d, %d, %d, %d]\n", E, F, G, H, I, J);
    // printf("        %d, %d, [%d, %d, %d, %d]\n", K, L, M, N, P, Q);
    // printf("        %d, %d, [%d, %d, %d, %d]\n", R1, R0, R, S, S0, S1);
    // printf("        %d, %d, [%d, %d, %d, %d]\n", T1, T0, T, U, U0, U1);
    // printf("==Inter: \n");


    int aa = SixTapFliter(A1, A0,   A__, B__, B0, B1);
    int bb = SixTapFliter(C1, C0,   C__, D__, D0, D1);
    int gg = SixTapFliter(R1, R0,   R__, S__, S0, S1);
    int hh = SixTapFliter(T1, T0,   T__, U__, U0, U1);
    int b1 = SixTapFliter(E__, F__, G__, H__, I__, J__);
    int h1 = SixTapFliter(A__, C__, G__, M__, R__, T__);
    int s1 = SixTapFliter(K__, L__, M__, N__, P__, Q__);
    int m1 = SixTapFliter(B__, D__, H__, N__, S__, U__);
    int b = Clip1Y((b1 + 16) >> 5, BitDepthY);
    int h = Clip1Y((h1 + 16) >> 5, BitDepthY);


    int j1 = SixTapFliter(aa, bb, b1, s1, gg, hh);
    int j = Clip1Y((j1 + 512) >> 10, BitDepthY);

    int s = Clip1Y((s1 + 16) >> 5, BitDepthY);
    int m = Clip1Y((m1 + 16) >> 5, BitDepthY);

    int a = (G__+ b + 1) >> 1;
    int c = (H__+ b + 1) >> 1;
    int d = (G__+ h + 1) >> 1;
    int n = (M__+ h + 1) >> 1;
    int f = (b + j + 1) >> 1;
    int i = (h + j + 1) >> 1;
    int k = (j + m + 1) >> 1;
    int q = (j + s + 1) >> 1;

    int e = (b + h + 1) >> 1;
    int g = (b + m + 1) >> 1;
    int p = (h + s + 1) >> 1;
    int r = (m + s + 1) >> 1;

    int* predPart[4][4] = 
    {
        {&G__, &d, &h, &n},
        {&a  , &e, &i, &p},
        {&b  , &f, &j, &q},
        {&c  , &g, &k, &r}
    };
    // printf(">>Inter: result: %d\n", *(predPart[xFracL][yFracL]));
    return *(predPart[xFracL][yFracL]);
}
void Prediction_Inter(\
    macroblock* current, matrix& out, \
    int PicWidthInMbs, int PicHeightInMbs,\
    int BitDepthY,\
    uint8_t mbPartIdx, uint8_t subMbPartIdx,\
    uint8_t width_part, uint8_t height_part,\
    picture* ref_pic, MotionVector **mv_lx, bool predFlag)
{
    type_macroblock type = current->type;
    int part = current->num_mb_part;

    int mvLX[2] = {mv_lx[mbPartIdx][subMbPartIdx][0], mv_lx[mbPartIdx][subMbPartIdx][1]};
    //xAL 是子块、子子块的图片中的绝对起始坐标
    int yAL = current->pos.x * 16, yBL = 0;
    int xAL = current->pos.y * 16, xBL = 0;
    //xL yL 是在块中的坐标
    int xL = 0, yL = 0, xFracL = 0, yFracL = 0;
    int xIntL = 0, yIntL = 0;
    //得到子块的起始绝对坐标

    int mb_partWidth =  MbPartWidth(type);
    int mb_partHeigh = MbPartHeight(type);
    if(type != B_Direct_16x16 && type != B_Skip)
    {
        xAL += (mbPartIdx % (16 / mb_partWidth)) * mb_partWidth + (part == 4 ? ((subMbPartIdx * SubMbPartWidth(current->inter->sub->type)) % 8) : 0);
        yAL += (mbPartIdx / (16 / mb_partWidth)) * mb_partHeigh + (part == 4 ? ((subMbPartIdx * SubMbPartWidth(current->inter->sub->type)) / 8) : 0);
    }
    else
    {
        xAL += (mbPartIdx % (16 / mb_partWidth)) * mb_partWidth;
        yAL += (mbPartIdx / (16 / mb_partWidth)) * mb_partHeigh;
    }
    
    //下面这步是直接求并和移位（负数也是，不需要处理）
    xFracL = (mvLX[0] & 3); 
    yFracL = (mvLX[1] & 3);/*Sig(mvLX[1]) */ 
    xBL = xAL + (mvLX[0] >> 2); 
    yBL = yAL +  /*Sig(mvLX[1]) **/ (mvLX[1] >> 2);
    //每一个样点都需要内插
    for(uint8_t yL = 0; yL < height_part; yL++)
    {
        for(uint8_t xL = 0; xL < width_part; xL++)
        {
            //加上运动矢量的整数坐标
            //对应在参考图片中的起始坐标加上块内的坐标分别得到整数绝对坐标
            xIntL = xBL + xL;
            yIntL = yBL + yL;
            //前向参考直接加到预测样点矩阵中，
            out[yL][xL] += Prediction_Inter_LumaSampleInterpolation(xIntL, yIntL, xFracL, yFracL,PicWidthInMbs,PicHeightInMbs, ref_pic, BitDepthY );
        }
    }
}

