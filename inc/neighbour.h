#ifndef NEIGHBOUR_H__
#define NEIGHBOUR_H__

class macroblock;



typedef struct Neigh16x16__
{
    bool avaiable;
    
    macroblock *mb;
    int mbPartIdx;
    int subPartIdx;
}Neigh16x16;

typedef struct NeiMvData__
{
    Neigh16x16 info;
    int refidx;
    MotionVector mv;
}NeiMvData;


void neighbour_macroblock(
    macroblock *current, char direction, 
    macroblock **result
);
void neighbour_motionvector_data(
    macroblock *current, int mbPartIdx, int subPartIdx,\
    int listSuffixFlag, MotionVector mv_lX,\
    int *refIdxLXN
);

void neighbour_motionver_normal();
void neighbour_motionver_bskip ();
void neighbour_motionver_pskip ();

/// 计算并置的 sub-macroblock
void col_located_4x4_sub_Partions(\
    macroblock *current, int mbPartIdx, int subPartIdx,\
    bool direct_8x8_inference_flag, decoder *de,\
    picture **picCol, macroblock **mbCol, MotionVector mvCol, int *refIdxCol \
);

/**
 * @param current 当前宏块
 * @param mbPartIdx 块索引
 * @param subPartIdx 子块索引
 * @param direction 运动矢量的方向，是mv_l1还是mv_l0
 * @param mv_lx 对应方向上的运动矢量
 */
void neighbour_motionvector(
    macroblock *current, int mbPartIdx, int subPartIdx,\
    int direction, MotionVector mv_lx
);


// 计算 16x16 宏块中的相邻 part
void neighbour_part_16x16(\
    macroblock *current, int mbPartIdx, int subPartIdx,\
    char direction,\
    macroblock **result, int *rePartIdx, int *reSubIdx
);
void neighbour_chroma_4x4(
    macroblock *current, int index,\
    char direction,\
    macroblock **result, int *index_result
);
// luma 4x4 相邻块的推导， 包括目标中的坐标，和目标中的 4x4 索引

// 坐标和索引都会返回
void neighbour_luma_4x4(\
    macroblock *current, int index,\
    char direction,\
    macroblock **result,\
    int *index_result, int *x, int *y
);
// 只计算 4x4 索引
void neighbour_luma_4x4_indice(\
    macroblock *current, int index,\
    char direction,\
    macroblock **result,\
    int *index_result
);
// 只计算 4x4 坐标
void neighbour_luma_4x4_position(\
    macroblock *current, int index,\
    char direction,\
    macroblock **result,\
    int *x, int *y
);

#endif
