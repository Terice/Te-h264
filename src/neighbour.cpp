
#include "macroblock.h"
#include "picture.h"
#include "gfunc.h"
#include "gmbinfo.h"
#include "neighbour.h"
#include "decoder.h"


// 换成目标宏块内的坐标
// 目标宏块 在 420 nombaddf 一定是一样的
void target_position(int width, int height, int xN, int yN, int *xW, int *yW)
{
    *xW = (xN + width ) % width;
    *yW = (yN + height) % height;
}
// 计算目标宏块是谁
void target_macroblock(int maxW,  int xN, int yN, char *R)
{
    //目标宏块为自己的情况只有三种，一种是求左边的A，x >=0 ,另一种是求上边B y >=0， 还有是 x >= 0 , y >= 0;
    //确定宏块是哪一个以及是不是当前宏块
    if     (xN >= 0 && yN >= 0 && xN < maxW)    *R = 'N';// 是自己
    else if(xN <  0 && yN <  0)                 *R = 'D';
    else if(xN <  0 && yN >= 0)                 *R = 'A';
    else if(xN >= 0 && yN <  0 && xN < maxW)    *R = 'B';
    else{if (xN >= maxW && yN <  0)             *R = 'C';//
        else                                    *R =  0 ;// 不存在
    }
}
// 当前的相对坐标 加上偏移得到偏移坐标
/// @param width 是预测的宽度
void trans_adddelta(char direction, int xP, int yP, int width, int *xN, int *yN)
{
    //确定在所取方向上的坐标偏移
    int xD = 0, yD = 0;
    switch (direction)
    {
    case 'A':xD = -1;    yD =  0; break;
    case 'B':xD =  0;    yD = -1; break;
    case 'C':xD = width; yD = -1; break;
    case 'D':xD = -1;    yD = -1; break;
    }
    *xN = xP + xD;
    *yN = yP + yD;
}
// 目标方向转换成目标宏块
macroblock * trans_macroblock(macroblock * current, char direction)
{
    switch (direction)
    {
    case 'A': return current->neighbour.A.avaiable ? current->neighbour.A.pointer : NULL; break;
    case 'B': return current->neighbour.B.avaiable ? current->neighbour.B.pointer : NULL; break;
    case 'C': return current->neighbour.C.avaiable ? current->neighbour.C.pointer : NULL; break;
    case 'D': return current->neighbour.D.avaiable ? current->neighbour.D.pointer : NULL; break;
    case 'N': return current; break;
    case  0 : return NULL; break;
    default: return NULL;break;
    }
}

// 有sub，需要 PartIdx， subPartIdx
void neighbour_position_sub(\
    type_macroblock type_cur,\
    type_submacroblock type_cursub,\
    int mbPartIdx, int subPartIdx,\
    char direction, \
    int width_all , int height_all, \
    int *x_result, int *y_result,\
    char *target
)
{
    int x,    y;
    int xS,   yS;
    int xN,   yN;
    int xW,   yW;
    int xD,   yD;
    // 计算 part 起始坐标
    InverseRasterScan_part(mbPartIdx, type_cur, &x, &y);
    // 计算 subpart 起始坐标
    if(type_cur == P_8x8 || type_cur == P_8x8ref0 || type_cur == B_8x8)
        InverseRasterScan_subpart_8x8(subPartIdx, &xS, &yS);
    else xS = yS = 0;
    // 计算预测宽
    int predPartWidth;
    if(type_cur == P_Skip || type_cur == B_Skip || type_cur == B_Direct_16x16) predPartWidth = 16;
    else if(type_cur == B_8x8)
    {
        if(type_cursub == B_Direct_8x8) predPartWidth = 16;// B_Direct_8x8 的预测就是全块的预测
        else predPartWidth = SubMbPartHeight(type_cursub);
    }
    else if(type_cur == P_8x8 || type_cur == P_8x8ref0)
        predPartWidth = SubMbPartHeight(type_cursub);
    else
    {
        predPartWidth = MbPartWidth(type_cur);
    }
    switch (direction)
    {
    case 'A':xD = -1;            yD = 0 ; break;
    case 'B':xD = 0 ;            yD = -1; break;
    case 'C':xD = predPartWidth; yD = -1; break;
    case 'D':xD = -1;            yD = -1; break;
    default: break;
    }
    xN = x + xS + xD;
    yN = y + yS + yD;
    target_macroblock(16, xN, yN, target);
    target_position(width_all, height_all, xN, yN, &xW, &yW);
    *x_result = xW;
    *y_result = yW;
}

// 计算目标宏块内起始坐标在目标宏块内的 两级 索引
// 在其中已经处理了 I 、 P_8x8 B_8x8 、 B_Skip B_Direct_16x16
void neighbour_indice_sub(int xP, int yP, macroblock *N, int *mbPartIdx, int *subPartIdx)
{
    if(!N) return;// 如果是NULL直接返回表示不可用
    type_macroblock type = N->type;
    // 如果是 I 宏块，
    if(type < TYPE_MACROBLOCK_START_INDEX_P) *mbPartIdx = 0;
    else *mbPartIdx = (16/MbPartWidth(type)) * (yP/MbPartHeight(type)) +(xP/MbPartWidth(type));

    // 如果有子块
    if(type == P_8x8 || type == B_8x8 || type == P_8x8ref0)
    {
        type_submacroblock sub = N->inter->sub[*mbPartIdx].type;
        *subPartIdx = (8/SubMbPartWidth(sub))*((yP%8)/SubMbPartHeight(sub))+(xP%8)/SubMbPartWidth(sub);
    }
    // B_Skip 块 和 B_Direct_16x16 都是 4 分然后再 4分，一共 16 块
    else if(type == B_Skip || type == B_Direct_16x16)
    {
        *subPartIdx = 2*((yP % 8)/4)+((xP % 8)/4);
    }
    // 否则说明没有子块，这个直接赋值为0
    else *subPartIdx = 0;
}
void neighbour_indice_luma4x4(int xP, int yP, int *luma4x4BlkIdx)
{
    *luma4x4BlkIdx = 8*(yP/8)+4*(xP/8)+2*((yP%8)/4)+((xP%8)/4);
}
void neighbour_indice_luma8x8(int xP, int yP, int *luma8x8BlkIdx)
{
    *luma8x8BlkIdx = 2*(yP/8)+(xP/8);
}
void neighbour_macroblock(macroblock *current, char direction, macroblock **result)
{
    *result = trans_macroblock(current, direction);
}






// 计算并置的4x4子块
void col_located_4x4_sub_Partions(\
    macroblock *current, int mbPartIdx, int subPartIdx,\
    bool direct_8x8_inference_flag, decoder *de, \
    picture **picCol, macroblock **mbCol, MotionVector mvCol, int *refIdxCol \
)
{
    int x, y;
    macroblock *m;
    picture *p = de->list_Ref1[0];
    x = current->pos.x; y = current->pos.y;
    m = (*p->mb)[y][x];

    
    if(m->is_intrapred())
    {
        mvCol[0] = 0;
        mvCol[1] = 0;
        *refIdxCol = -1;
    }
    else
    {
        // 先简单设置为 0 0 0 
        mvCol[0] = m->inter->mv.mv_l0[0][0][0];
        mvCol[1] = m->inter->mv.mv_l0[0][0][1];
        *refIdxCol = m->inter->ref_idx_l0[0];
    }
    *picCol = p;
    *mbCol  = m;
}



void neighbour_motionver_normal(macroblock *current, int mbPartIdx, int subPartIdx, MotionVector mv_lx, int direction)
{
    
}
void neighbour_motionver_bdire(){}
void neighbour_motionver_pskip(){}

// 中值处理运动矢量
void MedianLumaMvPrediction()
{}
// 周围运动矢量数据
void NeighbourPartMotionData(
    macroblock *current, int mbPartIdx, int subPartIdx,\
    int listSuffixFlag, NeiMvData *A, NeiMvData *B, NeiMvData *C 
)
{
}




























// 计算周围的运动矢量，此时不需要参考索引
void neighbour_motionvector(
    macroblock *current, int mbPartIdx, int subPartIdx,\
    int listSuffixFlag, MotionVector mv_lX
)
{
    int tmp[3];
    neighbour_motionvector_data(current, mbPartIdx, subPartIdx, listSuffixFlag, mv_lX, tmp);
}
// 求解相邻运动矢量所在的位置以及参考索引
void neighbour_motionvector_data(
    macroblock *current, int mbPartIdx, int subPartIdx,\
    int listSuffixFlag, MotionVector mv_lX,\
    int *refIdxLXN
)
{
    macroblock* A = NULL;
    macroblock* B = NULL;
    macroblock* C = NULL;
    macroblock* D = NULL;

    int mbPartIdx_A = -1, subPartIdx_A = -1, refIdxLX_A = -1;MotionVector mv_lx_A;
    int mbPartIdx_B = -1, subPartIdx_B = -1, refIdxLX_B = -1;MotionVector mv_lx_B;
    int mbPartIdx_C = -1, subPartIdx_C = -1, refIdxLX_C = -1;MotionVector mv_lx_C;
    int mbPartIdx_D = -1, subPartIdx_D = -1, refIdxLX_D = -1;MotionVector mv_lx_D;

    predmode_mb_part Pred_LX = listSuffixFlag ? Pred_L1 : Pred_L0;
    int8* ref_idx_lx = listSuffixFlag == 1 ? current->inter->ref_idx_l1 : current->inter->ref_idx_l0;
    int refIdxLX = ref_idx_lx[mbPartIdx];

    // 这个函数是用来计算这个方向上有没有预测值，以及参考索引和预测标志
    auto f_1 = [&current, listSuffixFlag, mbPartIdx, subPartIdx, Pred_LX](char direction, int& mbPartIdx_N, int& subPartIdx_N, int& refIdxLX_N, int mv_lx_N[2])
    ->macroblock*
    {
        macroblock* N ;
        neighbour_part_16x16(current, mbPartIdx, subPartIdx, direction, &N, &mbPartIdx_N, &subPartIdx_N);
        int refIdx = -1;
        // 如果 N 不可用 || 是帧内 || 这个方向不预测
        // 那么就直接返回
        uint8_t predFlagLX = PredFlag(N, mbPartIdx_N, listSuffixFlag);
        if(!current->is_avaiable(N) || N->is_intrapred() || !predFlagLX)
        {
            refIdx = -1;
            mv_lx_N[0] = 0;
            mv_lx_N[1] = 0;
        }
        // predmode_mb_part premode_cur_sub = current->num_mb_part == 4 ? \
        //                                     SubMbPartPredMode(current->inter->sub[mbPartIdx].type) : Pred_NU;
        else
        {
            MotionVector **mv_lx = NULL;
            int8* ref_idx_lx = NULL;
            ref_idx_lx = listSuffixFlag ? N->inter->ref_idx_l1    : N->inter->ref_idx_l0;
            mv_lx      = listSuffixFlag ? N->inter->mv.mv_l1      : N->inter->mv.mv_l0;

            refIdx = ref_idx_lx[mbPartIdx_N];
            mv_lx_N[0] = mv_lx[mbPartIdx_N][subPartIdx_N][0];
            mv_lx_N[1] = mv_lx[mbPartIdx_N][subPartIdx_N][1];
        }

        refIdxLX_N = refIdx;
        return N;
    };
    //初始化 参考周围的 索引和运动矢量
    A = f_1('A', mbPartIdx_A, subPartIdx_A, refIdxLX_A, mv_lx_A);
    B = f_1('B', mbPartIdx_B, subPartIdx_B, refIdxLX_B, mv_lx_B);
    if(current->type == P_Skip && \
        (!current->is_avaiable(A) || (A->is_interpred() && (A->inter->ref_idx_l0[0] && A->inter->mv.mv_l0[0][0][0] == 0 && A->inter->mv.mv_l0[0][0][1] == 0)) ||\
         !current->is_avaiable(B) || (B->is_interpred() && (B->inter->ref_idx_l0[0] && B->inter->mv.mv_l0[0][0][0] == 0 && B->inter->mv.mv_l0[0][0][1] == 0)) \
        )
    )
    {// Skip宏块这种情况直接置零返回
        mv_lX[0] = 0;
        mv_lX[1] = 0;
        return ;
    }
    C = f_1('C', mbPartIdx_C, subPartIdx_C, refIdxLX_C, mv_lx_C);
    D = f_1('D', mbPartIdx_D, subPartIdx_D, refIdxLX_D, mv_lx_D);
    uint8_t partWidth = MbPartWidth(current->type);
    uint8_t partHeight= MbPartHeight(current->type);

    if(current->pos.x == 33 && current->pos.y == 7 && current->idx_slice == 2)
        int a = 0;

    if(!current->is_avaiable(C) || mbPartIdx_C == -1 || subPartIdx_C == -1)
    {
        C = D;
        mbPartIdx_C = mbPartIdx_D;
        subPartIdx_C = subPartIdx_D;
        mv_lx_C[0] = mv_lx_D[0];
        mv_lx_C[1] = mv_lx_D[1];
        refIdxLX_C = refIdxLX_D;
    }
    // 这里只是计算 prediction 的值，所以传入的是mvp
    // 计算周围的运动矢量
    if(partWidth == 16 && partHeight == 8 && mbPartIdx == 0 && refIdxLX_B == refIdxLX) 
    {
        // 当前分上下两块，第一块从 B 预测而来 P170页，以下除了else里面的情况皆是
        mv_lX[0] = mv_lx_B[0];
        mv_lX[1] = mv_lx_B[1];
    }
    else if((partWidth == 16 && partHeight == 8 && mbPartIdx == 1 && refIdxLX_A == refIdxLX) || \
            (partWidth == 8 && partHeight == 16 && mbPartIdx == 0 && refIdxLX_A == refIdxLX)
    )
    {
        // 当前分上下两块，第二块由 A 预测而来
        // 当前分左右两块，第一块由 A 预测而来
        mv_lX[0] = mv_lx_A[0];
        mv_lX[1] = mv_lx_A[1];
    }
    else if(partWidth == 8 && partHeight == 16 && mbPartIdx == 1 && refIdxLX_C == refIdxLX)
    {
        // 当前分为左右两块，第二块由 C 预测而来
        mv_lX[0] = mv_lx_C[0];
        mv_lX[1] = mv_lx_C[1];
    }
    else //中值处理
    {
        if(!current->is_avaiable(B) && !current->is_avaiable(C))
        {
            mv_lx_B[0] = mv_lx_C[0] = mv_lx_A[0];
            mv_lx_B[1] = mv_lx_C[1] = mv_lx_A[1];
            refIdxLX_B = refIdxLX_C = refIdxLX_A;
        }
        if     (refIdxLX_A == refIdxLX && refIdxLX_B != refIdxLX && refIdxLX_C != refIdxLX)
        {
            mv_lX[0] = mv_lx_A[0];
            mv_lX[1] = mv_lx_A[1];
        }
        else if(refIdxLX_A != refIdxLX && refIdxLX_B == refIdxLX && refIdxLX_C != refIdxLX)
        {
            mv_lX[0] = mv_lx_B[0];
            mv_lX[1] = mv_lx_B[1];
        }
        else if(refIdxLX_A != refIdxLX && refIdxLX_B != refIdxLX && refIdxLX_C == refIdxLX)
        {
            mv_lX[0] = mv_lx_C[0];
            mv_lX[1] = mv_lx_C[1];
        }
        else 
        {
            mv_lX[0] = Median(mv_lx_A[0], mv_lx_B[0], mv_lx_C[0]);
            mv_lX[1] = Median(mv_lx_A[1], mv_lx_B[1], mv_lx_C[1]);
        }
    }
    // 这三个是留给 Direct 预测模式来预测参考索引的
    refIdxLXN[0] = refIdxLX_A;
    refIdxLXN[1] = refIdxLX_B;
    refIdxLXN[2] = refIdxLX_C;
    
    return ;
}





// 计算 16x16 宏块中的相邻 part
void neighbour_part_16x16(\
macroblock *current, int mbPartIdx, int subPartIdx,\
char direction,\
macroblock **result, int *rePartIdx, int *reSubIdx
)
{
    int xP, yP;
    char direction_result;
    type_macroblock type_cur = current->type;
    type_submacroblock type_cur_sub;
    // 如果当前宏块有子块的话，那么计算这个子块的类型
    if(current->is_interpred() && current->num_mb_part == 4 && !current->mb_skip_flag)
        type_cur_sub = current->inter->sub[mbPartIdx].type; 
    // 如果没有的话，那么这个值是不会用到的，所以即便是未分配的值也不要紧
    neighbour_position_sub(type_cur, type_cur_sub, mbPartIdx, subPartIdx, direction, 16,16, &xP, &yP, &direction_result);
    *result = trans_macroblock(current, direction_result);
    // 计算目标宏块内的 块格式索引
    neighbour_indice_sub(xP, yP, *result, rePartIdx, reSubIdx);
}
void neighbour_chroma_4x4(
macroblock *current, int index,\
char direction,\
macroblock **result, int *index_result
)
{
    int xP, yP, xN, yN, xW, yW;
    char direction_result;
    InverseRasterScan_chroma4x4(index, &xP, &yP);
    // 起始坐标偏移
    trans_adddelta(direction, xP, yP, 16, &xN, &yN);
    // 偏移坐标转目标宏块代表值
    target_macroblock(8, xN, yN, &direction_result);
    // 偏移坐标转目标宏块内起始坐标
    target_position(8,8, xN, yN, &xW, &yW);
    // 目标宏块内起始坐标转目标宏块内的 4x4 索引
    neighbour_indice_luma4x4(xW, yW, index_result);
    // 目标宏块代表值转真实宏块指针
    *result = trans_macroblock(current, direction_result);
    
}


// luma 4x4 相邻块的推导， 包括目标中的坐标，和目标中的 4x4 索引

// 坐标和索引都会返回
void neighbour_luma_4x4(\
macroblock *current, int index,\
char direction,\
macroblock **result,\
int *index_result, int *x, int *y
)
{
    
    int xP, yP, xN, yN, xW, yW;
    char direction_result;
    // 4x4 索引转 起始坐标
    InverseRasterScan_luma4x4(index, &xP, &yP);
    // 起始坐标偏移
    trans_adddelta(direction, xP, yP, 16, &xN, &yN);
    // 偏移坐标转目标宏块代表值
    target_macroblock(16, xN, yN, &direction_result);
    // 偏移坐标转目标宏块内起始坐标
    target_position(16,16, xN, yN, &xW, &yW);
    // 目标宏块内起始坐标转目标宏块内的 4x4 索引
    neighbour_indice_luma4x4(xW, xW, index_result);
    // 目标宏块代表值转真实宏块指针
    *result = trans_macroblock(current, direction_result);
    *x = xW;
    *y = yW;
}
// 只计算 4x4 索引
void neighbour_luma_4x4_indice(\
macroblock *current, int index,\
char direction,\
macroblock **result,\
int *index_result
)
{
    int xP, yP, xN, yN,xW, yW;
    char direction_result;
    // 4x4 索引转 起始坐标
    InverseRasterScan_luma4x4(index, &xP, &yP);
    // 起始坐标偏移
    trans_adddelta(direction, xP, yP, 16, &xN, &yN);
    // 偏移坐标转目标宏块代表值
    target_macroblock(16, xN, yN, &direction_result);
    // 偏移坐标转目标宏块内起始坐标
    target_position(16,16, xN, yN, &xW, &yW);
    // 目标宏块内起始坐标转目标宏块内的 4x4 索引
    neighbour_indice_luma4x4(xW, yW, index_result);
    // 目标宏块代表值转真实宏块指针
    *result = trans_macroblock(current, direction_result);
}
// 只计算 4x4 坐标
void neighbour_luma_4x4_position(\
macroblock *current, int index,\
char direction,\
macroblock **result,\
int *x, int *y
)
{
    int xP, yP, xN, yN,xW, yW;
    char direction_result;
    InverseRasterScan_luma4x4(index, &xP, &yP);
    trans_adddelta(direction, xP, yP, 4, &xN, &yN);
    target_macroblock(16, xN, yN, &direction_result);
    target_position(16,16, xN, yN, &xW, &yW);
    *result = trans_macroblock(current, direction_result);
    *x = xW;
    *y = yW;
}

void neighbour_luma_8x8();
