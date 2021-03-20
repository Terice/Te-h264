#ifndef PREDICTION_H__
#define PREDOCTION_H__

class pixmap;
class macroblock;
class matrix;

#include "gmbinfo.h"
#include "slice.h"
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
void Prediction_Intra4x4_V                   (macroblock*, matrix*);
void Prediction_Intra4x4_H                   (macroblock*, matrix*);
void Prediction_Intra4x4_DC                  (macroblock*, matrix*, int);
void Prediction_Intra4x4_Diagonal_Down_Left  (macroblock*, matrix*);
void Prediction_Intra4x4_Diagonal_Down_Right (macroblock*, matrix*);
void Prediction_Intra4x4_V_Right             (macroblock*, matrix*);
void Prediction_Intra4x4_H_Down              (macroblock*, matrix*);
void Prediction_Intra4x4_V_Left              (macroblock*, matrix*);
void Prediction_Intra4x4_H_Up                (macroblock*, matrix*);

void Prediction_Intra8x8                     ();//

void Prediction_Intra16x16_DC                (macroblock*, int BitDepthY);
void Prediction_Intra16x16_Plane             (macroblock*, int BitDepthY);
void Prediction_Intra16x16_H                 (macroblock*);
void Prediction_Intra16x16_V                 (macroblock*);

//帧间预测
//块索引，子块索引， 块宽，块高，参考图片索引，运动矢量，是否预测标志
//直接修改宏块的预测值，
void Prediction_Inter(\
    macroblock* current, matrix& out, \
    int PicWidthInMbs, int PicHeightInMbs,\
    int BitDepthY,\
    uint8_t mbPartIdx, uint8_t subMbPartIdx,\
    uint8_t width_part, uint8_t height_part,\
    picture* ref_pic, MotionVector **mv_lx
);
// 像素内插
int  Prediction_Inter_LumaSampleInterpolation(\
    int xIntL, int yIntL, \
    int xFracL, int yFracL, \
    picture* ref_pic
);
// 帧间预测direct模式
int Prediction_Inter_Direct(\
    macroblock* current,\
    int mbPartIdx,\
    parser* pa, decoder *de
);
void Weight_CoefficWeight(
    bool is_explicit,\
    matrix& m0, matrix& m1, matrix& out, \
    int refidx_l0, int refidx_l1, \
    bool   flag_0, bool   flag_1,\
    int BitDepth_Y,\
    PredWeight* pw\
);
void Weight_defaultWeight(
    matrix& m0, matrix& m1, matrix& out, \
    bool flag_0, bool flag_1
);
int Prediction_Inter_LumaSampleInterpolation(\
    int xIntL,  int yIntL,\
    int xFracL, int yFracL,\
    int PicWidthInMbs, int PicHeightInMbs,\
    picture* ref_pic, int BitDepthY
);


#endif