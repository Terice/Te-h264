#ifndef PREDCHART_H__
#define PREDCHART_H__
#include "genum.h"
#include "gtype.h"


#define na_mb 127
#define infer_mb 125

#define MACROBLOCK_CHART_INDEX_PART_N 1
#define MACROBLOCK_CHART_INDEX_PRED_0 2
#define MACROBLOCK_CHART_INDEX_PRED_1 3
#define MACROBLOCK_CHART_INDEX_PART_W 4
#define MACROBLOCK_CHART_INDEX_PART_H 5
// 重新制作了一下info表
// 第一条索引就是 宏块类型的枚举的值
// 第二条索引是对应的 数值
// 首先 I 宏块是没有分块的，但是这里为了一致保持了这条，值为1
// 对于的分块 宽高都是0
// 对于intra16x16, 色彩编码模式能够从type的值读出来，也不再查表了
// 把所有的宏块类型都考虑在内了，也就不用区分slice来判断了
static const unsigned char MacroBlockChart[][6]=
{
/*  |name          |Num Part|premode      |premode |MB   |MB   |
    |              |        |   0         |    1   |Part |Part |
    |0             |1       |2            |3       |width|heigh|*/
    { I_NxN         , 1     , na_mb       ,infer_mb,    0,     0},
    { I_16x16_0_0_0 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_1_0_0 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_2_0_0 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_3_0_0 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_0_1_0 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_1_1_0 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_2_1_0 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_3_1_0 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_0_2_0 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_1_2_0 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_2_2_0 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_3_2_0 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_0_0_1 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_1_0_1 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_2_0_1 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_3_0_1 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_0_1_1 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_1_1_1 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_2_1_1 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_3_1_1 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_0_2_1 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_1_2_1 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_2_2_1 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_16x16_3_2_1 , 1     , Intra_16x16 , Pred_NU,    0,     0},
    { I_PCM         , 1     , na_mb       , na_mb  ,    0,     0},
    { SI_M          , 1     , na_mb       , na_mb  ,   16,    16},
    { P_L0_16x16    , 1     , Pred_L0     , Pred_NU,   16,    16},
    { P_L0_L0_16x8  , 2     , Pred_L0     , Pred_L0,   16,     8},
    { P_L0_L0_8x16  , 2     , Pred_L0     , Pred_L0,    8,    16},
    { P_8x8         , 4     , Pred_NU     , Pred_NU,    8,     8},
    { P_8x8ref0     , 4     , Pred_NU     , Pred_NU,    8,     8},
    { P_Skip        , 1     , Pred_L0     , Pred_NU,   16,    16},
    { B_Direct_16x16, na_mb , Direct      , Pred_NU,    8,     8},
    { B_L0_16x16    , 1     , Pred_L0     , Pred_NU,   16,    16},
    { B_L1_16x16    , 1     , Pred_L1     , Pred_NU,   16,    16},
    { B_Bi_16x16    , 1     , BiPred      , Pred_NU,   16,    16},
    { B_L0_L0_16x8  , 2     , Pred_L0     , Pred_L0,   16,     8},
    { B_L0_L0_8x16  , 2     , Pred_L0     , Pred_L0,    8,    16},
    { B_L1_L1_16x8  , 2     , Pred_L1     , Pred_L1,   16,     8},
    { B_L1_L1_8x16  , 2     , Pred_L1     , Pred_L1,    8,    16},
    { B_L0_L1_16x8  , 2     , Pred_L0     , Pred_L1,   16,     8},
    { B_L0_L1_8x16  , 2     , Pred_L0     , Pred_L1,    8,    16},
    { B_L1_L0_16x8  , 2     , Pred_L1     , Pred_L0,   16,     8},
    { B_L1_L0_8x16  , 2     , Pred_L1     , Pred_L0,    8,    16},
    { B_L0_Bi_16x8  , 2     , Pred_L0     , BiPred ,   16,     8},
    { B_L0_Bi_8x16  , 2     , Pred_L0     , BiPred ,    8,    16},
    { B_L1_Bi_16x8  , 2     , Pred_L1     , BiPred ,   16,     8},
    { B_L1_Bi_8x16  , 2     , Pred_L1     , BiPred ,    8,    16},
    { B_Bi_L0_16x8  , 2     , BiPred      , Pred_L0,   16,     8},
    { B_Bi_L0_8x16  , 2     , BiPred      , Pred_L0,    8,    16},
    { B_Bi_L1_16x8  , 2     , BiPred      , Pred_L1,   16,     8},
    { B_Bi_L1_8x16  , 2     , BiPred      , Pred_L1,    8,    16},
    { B_Bi_Bi_16x8  , 2     , BiPred      , BiPred ,   16,     8},
    { B_Bi_Bi_8x16  , 2     , BiPred      , BiPred ,    8,    16},
    { B_8x8         , 4     , Pred_NU     , Pred_NU,    8,     8},
    { B_Skip        , 1     , Direct      , Pred_NU,    8,     8}
};
#define SUBMB_CHART_INDEX_PART_N   1
#define SUBMB_CHART_INDEX_PREDMODE 2
#define SUBMB_CHART_INDEX_PART_W   3
#define SUBMB_CHART_INDEX_PART_H   4

static const uint8 SubMacroBlockChart[18][5] =
{
/*  |sub_type          |Num |premode     |width |height|
    |                  |Sub |            |      |      |
    |0                 |Part|2           |3     |4     |*/
    { P_L0_8x8         ,   1,Pred_L0,    8,     8},// na_mb}
    { P_L0_8x4         ,   2,Pred_L0,    8,     4},// na_mb}
    { P_L0_4x8         ,   2,Pred_L0,    4,     8},// na_mb}
    { P_L0_4x4         ,   4,Pred_L0,    4,     4},// na_mb}
    { inferred_mb_type ,   4,Direct ,    4,     4},// na_mb}
    { B_Direct_8x8     ,   4,Direct ,    4,     4},// na_mb} 
    { B_L0_8x8         ,   1,Pred_L0,    8,     8},// na_mb} 
    { B_L1_8x8         ,   1,Pred_L1,    8,     8},// na_mb} 
    { B_Bi_8x8         ,   1,BiPred ,    8,     8},// na_mb} 
    { B_L0_8x4         ,   2,Pred_L0,    8,     4},// na_mb} 
    { B_L0_4x8         ,   2,Pred_L0,    4,     8},// na_mb} 
    { B_L1_8x4         ,   2,Pred_L1,    8,     4},// na_mb} 
    { B_L1_4x8         ,   2,Pred_L1,    4,     8},// na_mb} 
    { B_Bi_8x4         ,   2,BiPred ,    8,     4},// na_mb} 
    { B_Bi_4x8         ,   2,BiPred ,    4,     8},// na_mb} 
    { B_L0_4x4         ,   4,Pred_L0,    4,     4},// na_mb} 
    { B_L1_4x4         ,   4,Pred_L1,    4,     4},// na_mb} 
    { B_Bi_4x4         ,   4,BiPred ,    4,     4} // na_mb}  
};
#endif