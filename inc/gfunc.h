#ifndef GFUNC_H__
#define GFUNC_H__

#include "gtype.h"
#include "genum.h"

class macroblock;

// 这里声明即将全局使用的函数

int Min(int a, int b){return a <= b ? a : b;}
int Max(int a, int b){return a >= b ? a : b;}
unsigned int Abs(int x){return x>0?x:(unsigned int)-x;};
int Sig(int v){return v >= 0 ? 1 : -1;};

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

// MbPartPredMode();
bool PredFlag(macroblock* m, uint8 MbPartIdx, uint8 flag);

predmode_mb_part    MbPartPredMode(type_macroblock type, uint8 mbPartIdx);
int MbPartNum(type_macroblock type);
int MbPartWidth (type_macroblock);
int MbPartHeight(type_macroblock);

predmode_mb_part SubMbPartPredMode(type_submacroblock type);
int SubNumMbPart(type_submacroblock type);
int SubMbPartWidth (type_submacroblock);
int SubMbPartHeight(type_submacroblock);

void InverseRasterScan_luma4x4  (int luma4x4BlkIdx,  int *x, int *y);
void InverseRasterScan_luma8x8  (int luma8x8BlkIdx,  int *x, int *y);
void InverseRasterScan_chroma4x4(int chroma4x4BlkIdx, int *x, int *y);

void InverseRasterScan_part          (int mbPartIdx, type_macroblock type, int *x, int *y);
// 只用于 除 P_8x8 P_8x8ref0 B_8x8 之外的
void InverseRasterScan_subpart_non8x8(int mbPartIdx, int subPartIdx, SubMacroBlock *sub, int *x, int *y);
// 只用于 P_8x8 P_8x8ref0 B_8x8
void InverseRasterScan_subpart_8x8   (               int subPartIdx,                     int *x, int *y);

#endif