#ifndef GFUNC_H__
#define GFUNC_H__

#include "gtype.h"
#include "genum.h"

class macroblock;

// 这里声明即将全局使用的函数

inline int Min(int a, int b){return a <= b ? a : b;}
inline int Max(int a, int b){return a >= b ? a : b;}
inline unsigned int Abs(int x){return x>0?x:(unsigned int)-x;};
inline int Sig(int v){return v >= 0 ? 1 : -1;};

inline int MinPositive(int x, int y){return (x > 0 && y > 0) ? Min(x, y) : Max(x, y);}
// Six tap 滤波器
inline int SixTapFliter(int a, int b, int c, int d, int e, int f){return a - 5 * b + 20 * c + 20 * d - 5 * e + f;}
// zig-zag扫描
inline int InverseRasterScan(int a, int b, int c, int d, int e){ return e == 0?((a%(d/b))*b):((a/(d/b))*c);}

//Note: range: [lowerLimit, upperLimit] not:()
inline int Clip3(int lower, int upper, int value)
{    
    if(value >= upper) return upper;
    else if(value <= lower) return lower;
    else return value;
};
inline int Clip1Y(int x, int BitDepthY){return Clip3(0, (1 << BitDepthY) - 1, x);};
inline int Clip1C(int x, int BitDepthC){return Clip3(0, (1 << BitDepthC) - 1, x);};

predmode_mb_part    MbPartPredMode(type_macroblock type, uint8 mbPartIdx);
predmode_mb_part SubMbPartPredMode(type_submacroblock type);
// MbPartPredMode();
bool PredFlag(macroblock* m, uint8 MbPartIdx, uint8 flag);
uint8 SubNumMbPart(type_submacroblock type);

uint8 MbPartWidth(type_macroblock);
uint8 MbPartHeight(type_macroblock);

uint8 SubMbPartWidth (type_submacroblock);
uint8 SubMbPartHeight(type_submacroblock);


#endif