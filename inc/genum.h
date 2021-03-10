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
//
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
    I_NxN_8x8     = 26,

    SI_M          = 30,

    P_L0_16x16    = 40,
    P_L0_L0_16x8  = 41,
    P_L0_L0_8x16  = 42,
    P_8x8         = 43,
    P_8x8ref0     = 44,
    P_Skip        = 45,

    B_Direct_16x16= 50,
    B_L0_16x16    = 51,
    B_L1_16x16    = 52,
    B_Bi_16x16    = 53,
    B_L0_L0_16x8  = 54,
    B_L0_L0_8x16  = 55,
    B_L1_L1_16x8  = 56,
    B_L1_L1_8x16  = 57,
    B_L0_L1_16x8  = 58,
    B_L0_L1_8x16  = 59,
    B_L1_L0_16x8  = 60,
    B_L1_L0_8x16  = 61,
    B_L0_Bi_16x8  = 62,
    B_L0_Bi_8x16  = 63,
    B_L1_Bi_16x8  = 64,
    B_L1_Bi_8x16  = 65,
    B_Bi_L0_16x8  = 66,
    B_Bi_L0_8x16  = 67,
    B_Bi_L1_16x8  = 68,
    B_Bi_L1_8x16  = 69,
    B_Bi_Bi_16x8  = 70,
    B_Bi_Bi_8x16  = 71,
    B_8x8         = 72,
    B_Skip        = 73,

    I_8x8         ,
    I_4x4
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

    Pred_NU = 127
};

enum type_submacroblock
{
    //P宏块中的子宏块
    P_L0_8x8          = 0,//
    P_L0_8x4          = 1,//
    P_L0_4x8          = 2,//
    P_L0_4x4          = 3,//
    inferred_mb_type  = 4, //

    //B宏块中的子宏块类型
    B_Direct_8x8     = 10,//
    B_L0_8x8         = 11,//
    B_L1_8x8         = 12,//
    B_Bi_8x8         = 13,//
    B_L0_8x4         = 14,//
    B_L0_4x8         = 15,//
    B_L1_8x4         = 16,//
    B_L1_4x8         = 17,//
    B_Bi_8x4         = 18,//
    B_Bi_4x8         = 19,//
    B_L0_4x4         = 20,//
    B_L1_4x4         = 21,//
    B_Bi_4x4         = 22,//


    sub_type_NULL    = 127
};

enum reference
{
    Ref_short,
    Ref_long,
    Nun_ref,
    Nul_exist,
};


#endif