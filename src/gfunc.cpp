#include "gfunc.h"
#include "gchart.h"
#include "gtype.h"
#include <math.h>

#include "matrix.h"
#include "slice.h"
#include "macroblock.h"
#include "picture.h"
#include "predchart.h"

static inline int MallocMotionVector(MotionVector **mv, int MbPart, SubMacroBlock* SubPart)
{
    mv = new MotionVector*[MbPart];
    if(SubPart) //r
        for (int i = 0; i < MbPart; i++)
            mv[i] = new MotionVector[SubPart[i].part];
    else 
        for (int i = 0; i < MbPart; i++)
            mv[i] = new MotionVector[1];
    return 0;
}
static inline int FreeMotionVector(MotionVector **mv, int MbPart, SubMacroBlock* SubPart)
{
    if(SubPart) //r
        for (int i = 0; i < MbPart; i++)
            delete[] mv[i];
    else 
        for (int i = 0; i < MbPart; i++)
            delete[] mv[i];
    delete[] mv;
    return 0;
}
int MallocMotionVectorPkg(MotionVector_info *m, int MbPart, SubMacroBlock* SubPart)
{
    MallocMotionVector(m->mv_l0,  MbPart, SubPart);
    MallocMotionVector(m->mv_l1,  MbPart, SubPart);
    MallocMotionVector(m->mvp_l0, MbPart, SubPart);
    MallocMotionVector(m->mvp_l0, MbPart, SubPart);
    MallocMotionVector(m->mvd_l0, MbPart, SubPart);
    MallocMotionVector(m->mvd_l0, MbPart, SubPart);
    return 0;
}
int FreeMotionVectorPkg(MotionVector_info *m, int MbPart, SubMacroBlock* SubPart)
{
    FreeMotionVector(m->mv_l0,  MbPart, SubPart);
    FreeMotionVector(m->mv_l1,  MbPart, SubPart);
    FreeMotionVector(m->mvp_l0, MbPart, SubPart);
    FreeMotionVector(m->mvp_l0, MbPart, SubPart);
    FreeMotionVector(m->mvd_l0, MbPart, SubPart);
    FreeMotionVector(m->mvd_l0, MbPart, SubPart);
    return 0;
}
void InverseRasterScan_luma4x4  (int luma4x4BlkIdx,  int *x, int *y)
{
    *x = InverseRasterScan( luma4x4BlkIdx / 4, 8, 8, 16, 0 ) + InverseRasterScan( luma4x4BlkIdx % 4, 4, 4, 8, 0);
    *y = InverseRasterScan( luma4x4BlkIdx / 4, 8, 8, 16, 1 ) + InverseRasterScan( luma4x4BlkIdx % 4, 4, 4, 8, 1 );
}
void InverseRasterScan_luma8x8  (int luma8x8BlkIdx,  int *x, int *y)
{
    *x = InverseRasterScan( luma8x8BlkIdx, 8, 8, 16, 0 );
    *y = InverseRasterScan( luma8x8BlkIdx, 8, 8, 16, 1 );
}
void InverseRasterScan_chroma4x4(int chroma4x4BlkIdx, int *x, int *y)
{
    *x = InverseRasterScan( chroma4x4BlkIdx, 4, 4, 8, 0 );
    *y = InverseRasterScan( chroma4x4BlkIdx, 4, 4, 8, 1 );
}

void InverseRasterScan_part          (int mbPartIdx, type_macroblock type, int *x, int *y)
{
    int mbPartW = MbPartWidth(type);
    int mbPartH = MbPartHeight(type);

    *x = InverseRasterScan(mbPartIdx, mbPartW, mbPartH, 16, 0);
    *y = InverseRasterScan(mbPartIdx, mbPartW, mbPartH, 16, 1);
}
void InverseRasterScan_subpart_non8x8(int mbPartIdx, int subPartIdx, SubMacroBlock *sub, int *x, int *y)
{
    int subPartW = SubMbPartWidth(sub[subPartIdx].type);
    int subPartH = SubMbPartHeight(sub[subPartIdx].type);

    *x = InverseRasterScan(subPartIdx, subPartW, subPartH, 8, 0);
    *y = InverseRasterScan(subPartIdx, subPartW, subPartH, 8, 1);
}
void InverseRasterScan_subpart_8x8   (               int subPartIdx,                     int *x, int *y)
{
    *x = InverseRasterScan(subPartIdx, 4, 4, 8, 0);
    *y = InverseRasterScan(subPartIdx, 4, 4, 8, 1);
}
predmode_mb_part    MbPartPredMode(type_macroblock type, uint8 mbPartIdx)
{
    int index = mbPartIdx == 0 ? MACROBLOCK_CHART_INDEX_PRED_0 : MACROBLOCK_CHART_INDEX_PRED_1;
    return (predmode_mb_part)MacroBlockChart[type][index];
}
int MbPartNum(type_macroblock type)
{
    return (int)MacroBlockChart[type][MACROBLOCK_CHART_INDEX_PART_N];
}
int inline MbPartWidth(type_macroblock type)
{
    return (int)MacroBlockChart[type][MACROBLOCK_CHART_INDEX_PART_W];
}
int inline MbPartHeight(type_macroblock type)
{
    return (int)MacroBlockChart[type][MACROBLOCK_CHART_INDEX_PART_H];
}
predmode_mb_part inline SubMbPartPredMode(type_submacroblock type)
{
    return (predmode_mb_part)SubMacroBlockChart[type][SUBMB_CHART_INDEX_PREDMODE];
}
int inline SubNumMbPart   (type_submacroblock type)
{
    return (int)SubMacroBlockChart[type][SUBMB_CHART_INDEX_PART_N];
}
int inline SubMbPartWidth (type_submacroblock type)
{
    return (int)SubMacroBlockChart[type][SUBMB_CHART_INDEX_PART_W];
}
int inline SubMbPartHeight(type_submacroblock type)
{
    return (int)SubMacroBlockChart[type][SUBMB_CHART_INDEX_PART_H];
}
bool PredFlag(macroblock* m, uint8 MbPartIdx, uint8 flag)
{
    if(!m) return false;

    auto Pred_LX = flag ? Pred_L1 : Pred_L0;
    predmode_mb_part premode = MbPartPredMode(m->type, MbPartIdx);
    if(m->num_mb_part == 4 && m->inter->sub) 
    {
        premode = SubMbPartPredMode(m->inter->sub[MbPartIdx].type);
        if(premode == Pred_LX || premode == BiPred) return 1;
        else return 0;
    }
    else 
    {
        if(premode == Pred_LX || premode == BiPred) return 1;
        else return 0;  
    }
}
