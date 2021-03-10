#ifndef GMBINFO_H__
#define GMBINFO_H__

#include "gtype.h"
#include "genum.h"

// 解帧内4x4需要的信息
typedef struct Intra4x4_pred_info__
{
    // 当前解码所在的4x4块的索引,当然最大值只有16了
    int8 index;
    // 4x4 预测模式
    uint8 Intra4x4PredMode            [16];
    // 4x4 预测方式，一共16个
    uint8 rem_intra4x4_pred_mode      [16];
    uint8 prev_intra4x4_pred_mode_flag[16];
}Intra4x4_pred_info;
// 解8x8需要的信息
typedef struct Intra8x8_pred_info__
{
    // 8x8 划分必然是划分成 4 块

    int8 index;
    uint8 Intra8x8PredMode            [4];

    uint8 prev_intra8x8_pred_mode_flag[4];
    uint8 rem_intra8x8_pred_mode      [4];
}Intra8x8_pred_info;

// 因为4x4 8x8 16x16 中只会出现一个，
// 所以采用union
// 4x4 8x8需要自己申请空间，这里只是指针
typedef union Inta_pred_info__
{
    // 4x4 需要对每一个4x4块都作出说明
    Intra4x4_pred_info* intra4x4;
    // 8x8 也需要对每一个块进行说明
    Intra8x8_pred_info* intra8x8;
    // 16x16 只有4种，DC、Plane、V、H
    // 为了对齐到边界 选择 int 类型
    int intra16x16;
}Inta_pred_info;


typedef struct Intra_pred__
{
    //帧内预测的限制条件
    uint8 constrained_intra_pred_flag;
    // 帧内色度预测模式
    uint16 intra_chroma_pred_mode;
    // predinfo 根据 predmode来判断使用哪个
    // 分配空间也是
    Inta_pred_info predinfo;
}Intra_pred;


// 解QP需要的信息
typedef struct Qp_info__
{
    int16 mb_qp_delta;
    int8  QPY_prev;
    int8  QPY;
    int8  QPY_;
    int8  QPC;
    int8  QPC_;
}Qp_info;

typedef point MotionVector;
typedef struct MotionVector_info__
{
    // 第一个[]是子块的索引，num_MBpart
    // 第二个[]是子子块的索引，看子块又分成了多少块
    // 第三个[]是组件的索引， 一个是x， 一个是y（运动矢量的两个方向）
    
    MotionVector **mvp_l0;
    MotionVector **mvp_l1;
    MotionVector **mvd_l0;
    MotionVector **mvd_l1;
    MotionVector **mv_l0;
    MotionVector **mv_l1;
}MotionVector_info;

typedef struct SubMacroBlock__
{
    // 子块类型
    // 每一个part的类型
    // 注意这个类型说明的是所有的子块的类型
    type_submacroblock type  ;
    // 每一个part分成多少个块
    // 这些块的类型都是一样的
    int part;
    // 
    predmode_mb_part predmode;
}SubMacroBlock;

typedef struct Inter_pred__
{
    // 预测标志只对于part来说，最大只有4个
    // 所以干脆直接写成固定的4个了

    bool predFlagL0[4];
    bool predFlagL1[4];
    // 每一个part的参考索引
    // 参考索引是以part为单位的
    // 如果参考超过了128,那我只能无语了
    int8 ref_idx_l0[4];
    int8 ref_idx_l1[4];
    // 运动矢量（对于子块-子子块都有）
    // 因为如果子块是4个的话，就可以继续分下去
    MotionVector_info mv;

    SubMacroBlock* sub;
}Inter_pred;
class macroblock;
typedef struct MacroBlockNeighInfo__
{
    // 这个info所指向的macroblock
    macroblock *pointer;
    // 对于当前 macroblock ，m 能不能用
    bool avaiable;
}MacroBlockNeighInfo;

/*
 * 方向
    |  D  |  B  |  C  |
    |  A  | Cur |xxxxx|
    |xxxxx|xxxxx|xxxxx|
*/
typedef struct MacroBlockNeigh__
{
    MacroBlockNeighInfo A;
    MacroBlockNeighInfo B;
    MacroBlockNeighInfo C;
    MacroBlockNeighInfo D;
}MacroBlockNeigh;


#endif