#ifndef PREDICTION_H__
#define PREDOCTION_H__

class pixmap;
class macroblock;
class matrix;

#include "gmbinfo.h"

/*   
    for the Intra_4x4
     0 1 2 3 4 5 6 7 8
    +--------------------> 
   0|M|A|B|C|D|E|F|G|H
   1|I|0 1 4 5|      x
   2|J|2 3 6 7|
   3|K|8 9 C D|
   5|L|A B E F|  /-- this is a sample in 4x4
             ^  /    (not a macroblock)
             \-/
    v 
*/
/*
    宏块方向的选择
    
    |  D  |  B  |  C  |
    |  A  | Cur |xxxxx|
    |xxxxx|xxxxx|xxxxx|

*/
// 宏块自己已经携带了必要的信息
void Prediction_Intra4x4_V                   (macroblock*);
void Prediction_Intra4x4_H                   (macroblock*);
void Prediction_Intra4x4_DC                  (macroblock*);
void Prediction_Intra4x4_Diagonal_Down_Left  (macroblock*);
void Prediction_Intra4x4_Diagonal_Down_Right (macroblock*);
void Prediction_Intra4x4_V_Right             (macroblock*);
void Prediction_Intra4x4_H_Down              (macroblock*);
void Prediction_Intra4x4_V_Left              (macroblock*);
void Prediction_Intra4x4_H_Up                (macroblock*);

void Prediction_Intra8x8                     ();//

void Prediction_Intra16x16_DC                (macroblock*);
void Prediction_Intra16x16_Plane             (macroblock*);
void Prediction_Intra16x16_H                 (macroblock*);
void Prediction_Intra16x16_V                 (macroblock*);

//帧间预测
//块索引，子块索引， 块宽，块高，参考图片索引，运动矢量，是否预测标志
//直接修改宏块的预测值，
void Prediction_Inter(matrix& out, uint8 mbPartIdx, uint8 subMbPartIdx, uint8 width_part, uint8 height_part, picture* idx_ref, MotionVector **mv_lx, bool predFlag);
// 像素内插
int  Prediction_Inter_LumaSampleInterpolation(int xIntL, int yIntL, int xFracL, int yFracL, picture* ref_pic);
// 帧间预测direct模式
int  Prediction_Inter_Direct(int ,int);
void Weight_CoefficWeight(bool is_explicit, matrix& m0, matrix& m1, matrix& out, int , int, bool flag_0, bool falg_1);
void Weight_defaultWeight(                  matrix& m0, matrix& m1, matrix& out,            bool flag_0, bool falg_1);
#endif