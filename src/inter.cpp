   
    // 加权预测

#include "matrix.h"
#include "picture.h"

void Weight_CoefficWeight(bool is_explicit, matrix& m0, matrix& m1, matrix& out, int , int, bool flag_0, bool falg_1);
void Weight_defaultWeight(                  matrix& m0, matrix& m1, matrix& out,            bool flag_0, bool falg_1);

int Parse_Inter_Direct(int mbPartIdx, int subMbPartIdx);
int Prediction_Inter_LumaSampleInterpolation(int xIntL, int yIntL, int xFracL, int yFracL, picture* ref_pic)
