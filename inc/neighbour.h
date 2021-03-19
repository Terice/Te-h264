#ifndef NEIGHBOUR_H__
#define NEIGHBOUR_H__

class macroblock;

void neighbour_macroblock(
    macroblock *current, char direction, 
    macroblock **result
);

void neighbour_motionvector(
    macroblock *current, int mbPartIdx, int subPartIdx,\
    int direction, MotionVector **mv_lx
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