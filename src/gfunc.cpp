#include "gfunc.h"
#include "gchart.h"
#include "gtype.h"
#include <math.h>

#include "matrix.h"
#include "slice.h"
#include "macroblock.h"
#include "picture.h"
#include "predchart.h"

int Min(int a, int b){return a <= b ? a : b;}
int Max(int a, int b){return a >= b ? a : b;}
unsigned int Abs(int x){return x>0?x:(unsigned int)-x;};
int Sig(int v){return v >= 0 ? 1 : -1;};
int Median(int x, int y, int z){return x + y + z - Min(x, Min(y, z)) - Max(x, Max(y , z));}

int MinPositive(int x, int y){return (x > 0 && y > 0) ? Min(x, y) : Max(x, y);}
// Six tap 滤波器
int SixTapFliter(int a, int b, int c, int d, int e, int f){return a - 5 * b + 20 * c + 20 * d - 5 * e + f;}
// zig-zag扫描
int InverseRasterScan(int a, int b, int c, int d, int e){ return e == 0?((a%(d/b))*b):((a/(d/b))*c);}

//Note: range: [lowerLimit, upperLimit] not:()
int Clip3(int lower, int upper, int value)
{    
    if(value >= upper) return upper;
    else if(value <= lower) return lower;
    else return value;
};
int Clip1Y(int x, int BitDepthY){return Clip3(0, (1 << BitDepthY) - 1, x);};
int Clip1C(int x, int BitDepthC){return Clip3(0, (1 << BitDepthC) - 1, x);};

static inline int MallocMotionVector(MotionVector ***mv, int MbPart, SubMacroBlock* SubPart)
{
    *mv = new MotionVector*[MbPart];
    if(SubPart) //r
        for (int i = 0; i < MbPart; i++)
            (*mv)[i] = new MotionVector[SubPart[i].part];
    else 
        for (int i = 0; i < MbPart; i++)
            (*mv)[i] = new MotionVector[1];
    return 0;
}
static inline int FreeMotionVector(MotionVector ***mv, int MbPart, SubMacroBlock* SubPart)
{
    if(SubPart) //r
        for (int i = 0; i < MbPart; i++)
            delete[] (*mv)[i];
    else 
        for (int i = 0; i < MbPart; i++)
            delete[] (*mv)[i];
    delete[] (*mv);
    *mv = NULL;
    return 0;
}
int MallocMotionVectorPkg_P(MotionVector_info *m, int MbPart, SubMacroBlock* SubPart)
{

}

int MallocMotionVectorPkg_B(MotionVector_info *m, int MbPart, SubMacroBlock* SubPart)
{
    
}

int MallocMotionVectorPkg(MotionVector_info *m, int MbPart, SubMacroBlock* SubPart)
{
    MallocMotionVector(&m->mv_l0,  MbPart, SubPart);
    MallocMotionVector(&m->mv_l1,  MbPart, SubPart);
    MallocMotionVector(&m->mvp_l0, MbPart, SubPart);
    MallocMotionVector(&m->mvp_l1, MbPart, SubPart);
    MallocMotionVector(&m->mvd_l0, MbPart, SubPart);
    MallocMotionVector(&m->mvd_l1, MbPart, SubPart);
    return 0;
}
int FreeMotionVectorPkg(MotionVector_info *m, int MbPart, SubMacroBlock* SubPart)
{
    FreeMotionVector(&m->mv_l0,  MbPart, SubPart);
    FreeMotionVector(&m->mv_l1,  MbPart, SubPart);
    FreeMotionVector(&m->mvp_l0, MbPart, SubPart);
    FreeMotionVector(&m->mvp_l1, MbPart, SubPart);
    FreeMotionVector(&m->mvd_l0, MbPart, SubPart);
    FreeMotionVector(&m->mvd_l1, MbPart, SubPart);
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
predmode_mb_part inline SubMbPartPredMode(type_submacroblock type)
{
    return (predmode_mb_part)SubMacroBlockChart[type][SUBMB_CHART_INDEX_PREDMODE];
}
int MbPartNum      (type_macroblock    type){return (int)   MacroBlockChart[type][MACROBLOCK_CHART_INDEX_PART_N];}
int MbPartWidth    (type_macroblock    type){return (int)   MacroBlockChart[type][MACROBLOCK_CHART_INDEX_PART_W];}
int MbPartHeight   (type_macroblock    type){return (int)   MacroBlockChart[type][MACROBLOCK_CHART_INDEX_PART_H];}
int SubNumMbPart   (type_submacroblock type){return (int)SubMacroBlockChart[type][SUBMB_CHART_INDEX_PART_N];}
int SubMbPartWidth (type_submacroblock type){return (int)SubMacroBlockChart[type][SUBMB_CHART_INDEX_PART_W];}
int SubMbPartHeight(type_submacroblock type){return (int)SubMacroBlockChart[type][SUBMB_CHART_INDEX_PART_H];}

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
