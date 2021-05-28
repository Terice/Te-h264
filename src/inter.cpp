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

#include "neighbour.h"

#define six_tap_filter(a,b,c,d,e,f) (a - 5 * b + 20 * c + 20 * d - 5 * e + f)
// 平均值
#define two_tap_filter(a,b) ((a + b + 1) >> 1)

// 计算当前方向是不是不动的方向
bool Direct_Col_Zero(\
    macroblock* current,\
    int mbPartIdx, int subPartIdx,\
    parser* pa, decoder *de
)
{
    bool colZeroFlag; MotionVector mvCol; int refIdxCol;

    // 判断当前宏块是不是不动的宏块
    if(!de->list_Ref1[0]->is_UsedForShort()) 
        colZeroFlag = 0;
    else
    {
        // 计算相邻的并置块的运动矢量和参考索引
        col_located_4x4_sub_Partions(\
        current, mbPartIdx, subPartIdx, \
        pa->pS->sps->direct_8x8_inference_flag,\
        de->list_Ref1[0], mvCol, &refIdxCol);
        
        if(refIdxCol == 0)
        /* 在P166，对于并置块运动矢量在-1, 1 范围内是这样说明的
            – If the co-located macroblock is a frame macroblock, the units of mvCol[ 0 ]
            and mvCol[ 1 ] are units of quarter luma frame samples. 
            – Otherwise (the co-located macroblock is a field macroblock), the units of 
            mvCol[ 0 ] and mvCol[ 1 ] are units of quarter luma field samples.
        */
            // if(mvCol[0] >= -1 && mvCol[0] <= 1 && mvCol[1] >= -1 && mvCol[1] <= 1)
            if(mvCol[0] & 3 && mvCol[1] & 3)
                colZeroFlag = 1;
            else 
                colZeroFlag = 0;
        else
            colZeroFlag = 0;
    }
    return colZeroFlag;
}


// 空间上 Direct 预测
// 表示宏块的信息可由相邻空间上的宏块的信息推导出来
void Prediction_Inter_Direct_Spatial(\
    macroblock* current,\
    int mbPartIdx,\
    parser* pa, decoder *de
)
{
    MotionVector **mv_l0 = current->inter->mv.mv_l0  ;
    MotionVector **mv_l1 = current->inter->mv.mv_l1  ;
    int8* ref_idx_l0     = current->inter->ref_idx_l0;
    int8* ref_idx_l1     = current->inter->ref_idx_l1;
    bool* predFlagL0     = current->inter->predFlagL0;
    bool* predFlagL1     = current->inter->predFlagL1;

    // 寻找周围的参考索引并且计算运动矢量
    int refIdxL0N[3]; int refIdxL0;
    int refIdxL1N[3]; int refIdxL1;
    int directZeroPredictionFlag;

    MotionVector mva_l0;
    MotionVector mva_l1;

    neighbour_motionvector_data(current, 0, 0, 0, mva_l0, refIdxL0N);
    neighbour_motionvector_data(current, 0, 0, 1, mva_l1, refIdxL1N);

    refIdxL0 = MinPositive(refIdxL0N[0], MinPositive(refIdxL0N[1], refIdxL0N[2]));
    refIdxL1 = MinPositive(refIdxL1N[0], MinPositive(refIdxL1N[1], refIdxL1N[2]));
    directZeroPredictionFlag = 0;

    if(refIdxL0 < 0 && refIdxL1 < 0)
    {
        directZeroPredictionFlag = 1;
        refIdxL1 = 0;
        refIdxL0 = 0;
    }
    for (int subPartIdx = 0; subPartIdx < 4; subPartIdx++)
    {
        // 判断当前宏块是不是不动的宏块
        bool colZeroFlag;
        colZeroFlag = Direct_Col_Zero(current, mbPartIdx, subPartIdx, pa,de);

        mv_l0[mbPartIdx][subPartIdx][0] = mva_l0[0];
        mv_l0[mbPartIdx][subPartIdx][1] = mva_l0[1];
        mv_l1[mbPartIdx][subPartIdx][0] = mva_l1[0];
        mv_l1[mbPartIdx][subPartIdx][1] = mva_l1[1];
        // 如果是，则将运动矢量置零
        if(directZeroPredictionFlag || refIdxL0 < 0 || (!refIdxL0 && colZeroFlag)) 
        {
            mv_l0[mbPartIdx][subPartIdx][0] = 0;
            mv_l0[mbPartIdx][subPartIdx][1] = 0;
        }
        if(directZeroPredictionFlag || refIdxL1 < 0 || (!refIdxL1 && colZeroFlag)) 
        {
            mv_l1[mbPartIdx][subPartIdx][0] = 0;
            mv_l1[mbPartIdx][subPartIdx][1] = 0;
        }
    }
    // 确定参考索引
    ref_idx_l0[mbPartIdx] = refIdxL0;
    ref_idx_l1[mbPartIdx] = refIdxL1;

    //是必须会有一个参考方向的，要么双向或者单向参考，不会没有参考方向
    predFlagL0[mbPartIdx] = ref_idx_l0[mbPartIdx] >= 0 ? 1 : 0;
    predFlagL1[mbPartIdx] = ref_idx_l1[mbPartIdx] >= 0 ? 1 : 0;

}

// Direct 预测，用来计算运动适量和参考索引
int Prediction_Inter_Direct(\
    macroblock* current,\
    int mbPartIdx,\
    parser* pa, decoder *de
)
{
    if(current->idx_slice == 5 && current->pos.x == 0 && current->pos.y == 7)
        int a = 0;
    //空间预测 spatial direct luma motion vector
    // if(pa->cur_slice->ps->direct_spatial_mv_pred_flag)//空间预测
    // {
        // MotionVector **mv0 = current->inter->mv.mv_l0;
        // MotionVector **mv1 = current->inter->mv.mv_l1;
        // for (int subMbPartIdx = 0; subMbPartIdx < 4; subMbPartIdx++)
        // {
            Prediction_Inter_Direct_Spatial(current, mbPartIdx, pa, de);
            // printf("direct [%2d, %2d]: part: %d, sub: %d, mov0: [%2d,%2d], mov1: [%2d,%2d]\n",current->pos.x, current->pos.y, mbPartIdx, subMbPartIdx, mv0[mbPartIdx][subMbPartIdx][0],mv0[mbPartIdx][subMbPartIdx][1],mv1[mbPartIdx][subMbPartIdx][0],mv1[mbPartIdx][subMbPartIdx][1]);
        // }
    // }
    // else//时间预测 
    // {
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
    {
        out = ((m0 + m1 + 1) >> 1);
    }
    else
    {
        out = (((flag_0?m0:zero) + (flag_1?m1:zero)));
    }
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



/*
    full luma sample interpolation:
    假设整数像素分布如下：G 是当前运动矢量的整数值指向的值

    A1    A0    [A]   [B]   B0    B1

    C1    C0    [C]   [D]   D0    D1
                ----------------------x
    [E]   [F] | [G]   [H]   [I]   [J] 
              | 
    [K]   [L] | [M]   [N]   [P]   [Q] 
              | 
    R1    R0  | [R]   [S]   S0    S1
              | 
    T1    T0  | [T]   [U]   U0    U1
              y
    fragtional luma sample interpolation:
    则分数像素分布如下，花括号是整数，方括号是1/2 像素 圆括号是 1/4
    1/2 都是由 six_tap_fliter 得到的
    1/4 都是由周围最近的两个 1/2 或者 整数像素 求平均值

                    {A}     [aa]     {B}                



                    {C}     [bb]     {D}
       
       
                  ------------------ -----------------------
    {E}     {F}   | {G} (a) [b] (c)| {H}       {I}       {J}
                  | (d) (e) (f) (g)| 
    [cc]    [dd]  | [h] (i) [j] (k)| [m]       [ee]      [ff]
                  | (n) (p) (q) (r)|
                  ------------------
    {K}     {L}   | {M}     [s]      {N}       {P}       {Q}
                  |
                  |
                  |
                  | {R}     [gg]     {S}
                  |
                  |
                  |
                  | {T}     [hh]     {U}
*/


// f-full, h-half, q-qualter
static int
A_1 , A_0 , A_f ,   aa_h,    B_f , B_0 , B_1 , 
C_1 , C_0 , C_f ,   bb_h,    D_f , D_0 , D_1 , 
E_f , F_f , G_f,a_q,b_h,c_q, H_f , I_f , J_f , 
            d_q,e_q,f_q,g_q, 
cc_h, dd_h, h_h,i_q,j_h,k_q, m_h , ee_h, ff_h, 
            n_q,p_q,q_q,r_q, 
K_f , L_f , M_f,    s_h,     N_f , P_f , Q_f , 
R_1 , R_0 , R_f ,   gg_h,    S_f , S_0 , S_1 ,
T_1 , T_0 , T_f ,   hh_h,    U_f , U_0 , U_1 ;

#define calc_h h_h = Clip1Y((six_tap_filter(A_f, C_f, G_f, M_f, R_f, T_f) + 16) >> 5, BitDepthY)
#define calc_s s_h = Clip1Y((six_tap_filter(K_f, L_f, M_f, N_f, P_f, Q_f) + 16) >> 5, BitDepthY)
#define calc_m m_h = Clip1Y((six_tap_filter(B_f, D_f, H_f, N_f, S_f, U_f) + 16) >> 5, BitDepthY)
#define calc_b b_h = Clip1Y((six_tap_filter(E_f, F_f, G_f, H_f, I_f, J_f) + 16) >> 5, BitDepthY)
#define calc_j j_h = Clip1Y((six_tap_filter(aa_h, bb_h, six_tap_filter(E_f, F_f, G_f, H_f, I_f, J_f), six_tap_filter(K_f, L_f, M_f, N_f, P_f, Q_f), gg_h, hh_h) + 512) >> 10, BitDepthY)

//分数像素内插，大写字母都是整数像素，小写字母都是分数像素
int Prediction_Inter_LumaSampleInterpolation(\
    int xIntL,  int yIntL,\
    int xFracL, int yFracL,\
    int PicWidthInMbs, int PicHeightInMbs,\
    picture* ref_pic, int BitDepthY)
{
    int PicWidthInSample  = PicWidthInMbs  * 16;
    int PicHeightInSample = PicHeightInMbs * 16;
    auto func = [xIntL, yIntL, ref_pic, PicWidthInSample, PicHeightInSample](int xDZL, int yDZL)
    ->int
    {
        int sample = 0;
        int xZL = 0, yZL = 0;

        xZL = Clip3(0, PicWidthInSample -1,  xIntL + xDZL);
        yZL = Clip3(0, PicHeightInSample -1, yIntL + yDZL);
        return (int)ref_pic->cons[0][yZL][xZL];
    };
    A_1 = func(-2, -2);A_0 = func(-1, -2);A_f = func(0, -2);B_f = func(1, -2);B_0 = func(2, -2);B_1 = func(3, -2);
    C_1 = func(-2, -1);C_0 = func(-1, -1);C_f = func(0, -1);D_f = func(1, -1);D_0 = func(2, -1);D_1 = func(3, -1);
    E_f = func(-2,  0);F_f = func(-1,  0);G_f = func(0,  0);H_f = func(1,  0);I_f = func(2,  0);J_f = func(3,  0);
    K_f = func(-2,  1);L_f = func(-1,  1);M_f = func(0,  1);N_f = func(1,  1);P_f = func(2,  1);Q_f = func(3,  1);
    R_1 = func(-2,  2);R_0 = func(-1,  2);R_f = func(0,  2);S_f = func(1,  2);S_0 = func(2,  2);S_1 = func(3,  2);
    T_1 = func(-2,  3);T_0 = func(-1,  3);T_f = func(0,  3);U_f = func(1,  3);U_0 = func(2,  3);U_1 = func(3,  3);
    
    // printf(">>Inter: insert sample:\n");
    // printf("        int pos :xIntL  yIntL  (%3d, %3d)\n", xIntL, yIntL);
    // printf("        fra pos :xFracL yFracL (%3d, %3d)\n", xFracL, yFracL);
    // printf("        %d, %d,  %d, %d, %d, %d\n",  A_1, A_0, A_f, B_f, B_0, B_1);
    // printf("        %d, %d,  %d, %d, %d, %d\n",  C_1, C_0, C_f, D_f, D_0, D_1);
    // printf("        %d, %d, [%d, %d, %d, %d]\n", E_f, F_f, G_f, H_f, I_f, J_f);
    // printf("        %d, %d, [%d, %d, %d, %d]\n", K_f, L_f, M_f, N_f, P_f, Q_f);
    // printf("        %d, %d, [%d, %d, %d, %d]\n", R_1, R_0, R_f, S_f, S_0, S_1);
    // printf("        %d, %d, [%d, %d, %d, %d]\n", T_1, T_0, T_f, U_f, U_0, U_1);
    // printf("==Inter: \n");
    /*
        h_h = Clip1Y((six_tap_filter(A_f, C_f, G_f, M_f, R_f, T_f) + 16) >> 5, BitDepthY); 
        b_h = Clip1Y((six_tap_filter(E_f, F_f, G_f, H_f, I_f, J_f) + 16) >> 5, BitDepthY);
    
    */
    if(xFracL == 0)
    {
        // 需要 G
        if(yFracL == 0) return G_f;
        // 需要 A C G M R T
        calc_h;
        if(yFracL == 1)
        {
            // 需要 G h
            return d_q = two_tap_filter(G_f, h_h);
        }
        if(yFracL == 2) 
            // 需要 h
            return h_h;
        if(yFracL == 3) 
        {
            // 需要 M h
            return n_q = two_tap_filter(M_f, h_h);
        }
    }
    else if(xFracL == 1)
    {
        calc_b;
        if(yFracL == 0)
        {
            return a_q = two_tap_filter(G_f, b_h);// 需要 G b 
        }
        // h 需要 A C G M R T
        calc_h;
        if(yFracL == 1)
        {
            // b 需要 E F G H I J
            // 需要 b h
            return e_q = two_tap_filter(b_h, h_h);
        }
        calc_s;
        if(yFracL == 2)
        {
            // j 需要aa bb b s gg hh
            aa_h = six_tap_filter(A_1, A_0, A_f, B_f, B_0, B_1);
            bb_h = six_tap_filter(C_1, C_0, C_f, D_f, D_0, D_1);
            
            gg_h = six_tap_filter(R_1, R_0, R_f, S_f, S_0, S_1);
            hh_h = six_tap_filter(T_1, T_0, T_f, U_f, U_0, U_1);
            calc_j;
            // 需要 h j
            return i_q = two_tap_filter(h_h, j_h);
        }
        if(yFracL == 3)
        {
            // 需要 h s
            return p_q = two_tap_filter(h_h, s_h);
        }
    }
    else if(xFracL == 2)
    {
        calc_b;
        if(yFracL == 0)
        {
            return b_h;
        }
        // j 需要aa bb b s gg hh
        aa_h = six_tap_filter(A_1, A_0, A_f, B_f, B_0, B_1);
        bb_h = six_tap_filter(C_1, C_0, C_f, D_f, D_0, D_1);
        calc_s;
        gg_h = six_tap_filter(R_1, R_0, R_f, S_f, S_0, S_1);
        hh_h = six_tap_filter(T_1, T_0, T_f, U_f, U_0, U_1);
        calc_j;
        if(yFracL == 1)
        {
            // 需要 b j
            return f_q = two_tap_filter(b_h, j_h);
        }
        if(yFracL == 2)
        {
            return j_h;
        }
        if(yFracL == 3)
        {
            return two_tap_filter(j_h, s_h);
        }
    }
    else if(xFracL == 3)
    {
        calc_b;
        if(yFracL == 0)
        {
            return c_q = two_tap_filter(b_h, H_f);
        }
        calc_m;
        if(yFracL == 1)
        {
            return g_q = two_tap_filter(b_h, m_h);
        }
        calc_s;
        if(yFracL == 2)
        {
            // j 需要aa bb b s gg hh
            aa_h = six_tap_filter(A_1, A_0, A_f, B_f, B_0, B_1);
            bb_h = six_tap_filter(C_1, C_0, C_f, D_f, D_0, D_1);
            

            gg_h = six_tap_filter(R_1, R_0, R_f, S_f, S_0, S_1);
            hh_h = six_tap_filter(T_1, T_0, T_f, U_f, U_0, U_1);
            calc_j;
            
            return k_q = two_tap_filter(j_h, m_h);
        }
        if(yFracL == 3)
        {
            return r_q = two_tap_filter(s_h, m_h);
        }
    }
}
void Prediction_Inter(\
    macroblock* current, matrix& out, \
    int PicWidthInMbs, int PicHeightInMbs,\
    int BitDepthY,\
    uint8_t mbPartIdx, uint8_t subMbPartIdx,\
    uint8_t width_part, uint8_t height_part,\
    picture* ref_pic, MotionVector **mv_lx
)
{
    type_macroblock type = current->type;
    int part = current->num_mb_part;

    int mvLX[2] = {mv_lx[mbPartIdx][subMbPartIdx][0], mv_lx[mbPartIdx][subMbPartIdx][1]};
    //xAL 是子块、子子块的图片中的绝对起始坐标
    int yAL = current->pos.y * 16, yBL = 0;
    int xAL = current->pos.x * 16, xBL = 0;
    //xL yL 是在块中的坐标
    int xL = 0, yL = 0, xFracL = 0, yFracL = 0;
    int xIntL = 0, yIntL = 0;
    //得到子块的起始绝对坐标

    int mb_partWidth =  MbPartWidth(type);
    int mb_partHeigh = MbPartHeight(type);
    
    if(part == 4 && type != B_Direct_16x16 && type != B_Skip)
    {
        int subW =  SubMbPartWidth(current->inter->sub[mbPartIdx].type);
        int subH = SubMbPartHeight(current->inter->sub[mbPartIdx].type);
        
        xAL += (mbPartIdx % (16 / 8    )) * mb_partWidth + \
             subMbPartIdx % ( 8 / subW )  * subW ;
        yAL += (mbPartIdx / (16 / 8    )) * mb_partHeigh + \
             subMbPartIdx / ( 8 / subW )  * subH ;
    }
    else if(type == B_Direct_16x16 || type == B_Skip)
    {
        xAL += (mbPartIdx   % (16 / 8)) * 8 + \
               (subMbPartIdx% (8  / 4)) * 4;
        yAL += (mbPartIdx   / (16 / 8)) * 8 + \
               (subMbPartIdx/ (8  / 4)) * 4;
    }
    else
    {
        xAL += (mbPartIdx % (16 / mb_partWidth)) * mb_partWidth;
        yAL += (mbPartIdx / (16 / mb_partWidth)) * mb_partHeigh;
    }
    
    //下面这步是直接求并和移位（负数也是，不需要处理）
    // 不用求符号和无符号数来解，直接用负数做运算
    xFracL = (mvLX[0] & 3); 
    yFracL = (mvLX[1] & 3);/*Sig(mvLX[1]) */ 
    xBL = xAL + (mvLX[0] >> 2); 
    yBL = yAL +  /*Sig(mvLX[1]) **/ (mvLX[1] >> 2);
    //每一个样点都需要内插
    for(yL = 0; yL < height_part; yL++)
    {
        for(xL = 0; xL < width_part; xL++)
        {
            //加上运动矢量的整数坐标
            //对应在参考图片中的起始坐标加上块内的坐标分别得到整数绝对坐标
            xIntL = xBL + xL;
            yIntL = yBL + yL;
            //前向参考直接加到预测样点矩阵中，
            out[yL][xL] =\
             Prediction_Inter_LumaSampleInterpolation(xIntL, yIntL, xFracL, yFracL,PicWidthInMbs,PicHeightInMbs, ref_pic, BitDepthY );
            // printf("(%2d, %2d)value : %d\n", xL, yL, out[yL][xL]);
        }
    }
}

