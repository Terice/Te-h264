#ifndef GENUMS_H__
#define GENUMS_H__

// 这里声明将全局使用的枚举类型
// type_macroblock

enum type_slice
{
    P  = 0,
    B  = 1,
    I  = 2,
    SP = 3,
    SI = 4,
};

#define TYPE_MACROBLOCK_START_INDEX_I 0   // I_NxN 开始
#define TYPE_MACROBLOCK_START_INDEX_P 27  // P_L0_16x16 开始
#define TYPE_MACROBLOCK_START_INDEX_B 33  // B_Direct_16x16 开始
// 每一种类型之内一定要按照解码的顺序来排布
// 至于真实值，单写方法判断吧，，，
enum type_macroblock
{
    I_NxN         = 0 ,
    I_16x16_0_0_0 = 1 ,
    I_16x16_1_0_0 = 2 ,
    I_16x16_2_0_0 = 3 ,
    I_16x16_3_0_0 = 4 ,
    I_16x16_0_1_0 = 5 ,
    I_16x16_1_1_0 = 6 ,
    I_16x16_2_1_0 = 7 ,
    I_16x16_3_1_0 = 8 ,
    I_16x16_0_2_0 = 9 ,
    I_16x16_1_2_0 = 10,
    I_16x16_2_2_0 = 11,
    I_16x16_3_2_0 = 12,
    I_16x16_0_0_1 = 13,
    I_16x16_1_0_1 = 14,
    I_16x16_2_0_1 = 15,
    I_16x16_3_0_1 = 16,
    I_16x16_0_1_1 = 17,
    I_16x16_1_1_1 = 18,
    I_16x16_2_1_1 = 19,
    I_16x16_3_1_1 = 20,
    I_16x16_0_2_1 = 21,
    I_16x16_1_2_1 = 22,
    I_16x16_2_2_1 = 23,
    I_16x16_3_2_1 = 24,
    I_PCM         = 25,
    SI_M          = 26,// SI 宏块
    P_L0_16x16    = 27,// P  开始
    P_L0_L0_16x8  = 28,
    P_L0_L0_8x16  = 29,
    P_8x8         = 30,
    P_8x8ref0     = 31,
    P_Skip        = 32,
    B_Direct_16x16= 33,// B 开始
    B_L0_16x16    = 34,
    B_L1_16x16    = 35,
    B_Bi_16x16    = 36,
    B_L0_L0_16x8  = 37,
    B_L0_L0_8x16  = 38,
    B_L1_L1_16x8  = 39,
    B_L1_L1_8x16  = 40,
    B_L0_L1_16x8  = 40,
    B_L0_L1_8x16  = 41,
    B_L1_L0_16x8  = 42,
    B_L1_L0_8x16  = 43,
    B_L0_Bi_16x8  = 44,
    B_L0_Bi_8x16  = 45,
    B_L1_Bi_16x8  = 46,
    B_L1_Bi_8x16  = 47,
    B_Bi_L0_16x8  = 48,
    B_Bi_L0_8x16  = 49,
    B_Bi_L1_16x8  = 51,
    B_Bi_L1_8x16  = 52,
    B_Bi_Bi_16x8  = 53,
    B_Bi_Bi_8x16  = 54,
    B_8x8         = 55,
    B_Skip        = 56
};

enum predmode_mb_part
{
    Intra_4x4      ,
    Intra_8x8      ,
    Intra_16x16    ,
    na_predmode    ,
    Pred_L0        ,
    Direct         ,
    BiPred         ,
    Pred_L1        ,

    Skip,
    PCM,

    Pred_NU = 127
};

#define TYPE_SUBMB_START_INDEX_P 0 // P_L0_8x8 开始
#define TYPE_SUBMB_START_INDEX_B 5 // B_Direct_8x8 开始
enum type_submacroblock
{
    //P宏块中的子宏块
    P_L0_8x8          = 0,//
    P_L0_8x4          = 1,//
    P_L0_4x8          = 2,//
    P_L0_4x4          = 3,//
    inferred_mb_type  = 4, //    
    B_Direct_8x8      = 5,//B宏块中的子宏块类型
    B_L0_8x8          = 6,//
    B_L1_8x8          = 7,//
    B_Bi_8x8          = 8,//
    B_L0_8x4          = 9,//
    B_L0_4x8          = 10,//
    B_L1_8x4          = 11,//
    B_L1_4x8          = 12,//
    B_Bi_8x4          = 13,//
    B_Bi_4x8          = 14,//
    B_L0_4x4          = 15,//
    B_L1_4x4          = 16,//
    B_Bi_4x4          = 17,//


    sub_type_NULL    = 127
};

enum reference
{
    Ref_short,
    Ref_long ,
    Nun_ref  ,
    Nul_exist,
};


#endif