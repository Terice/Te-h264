#include "macroblock.h"
#include "picture.h"
#include "pixmap.h"
#include "matrix.h"
#include "parser.h"
#include <string.h>
#include "neighbour.h"

// 尽量都采用matrix进行运算

#define GET_NEIGHT_4x4_A() \
neighbour_luma_4x4_position(current, current->intra->predinfo.intra4x4->index, \
          'A',\
    &mbAddrA,\
        &c_A,\
        &r_A);\
if  (mbAddrA)\
{   I = mbAddrA->cons[0][r_A + 0][c_A];\
    J = mbAddrA->cons[0][r_A + 1][c_A];\
    K = mbAddrA->cons[0][r_A + 2][c_A];\
    L = mbAddrA->cons[0][r_A + 3][c_A];\
};
#define GET_NEIGHT_4x4_B() \
neighbour_luma_4x4_position(current, current->intra->predinfo.intra4x4->index, \
          'B',\
    &mbAddrB,\
        &c_B,\
        &r_B);\
if  (mbAddrB)\
{   A = mbAddrB->cons[0][r_B][c_B + 0];\
    B = mbAddrB->cons[0][r_B][c_B + 1];\
    C = mbAddrB->cons[0][r_B][c_B + 2];\
    D = mbAddrB->cons[0][r_B][c_B + 3];\
};
#define GET_NEIGHT_4x4_C() \
neighbour_luma_4x4_position(current, current->intra->predinfo.intra4x4->index, \
          'C',\
    &mbAddrC,\
        &c_C,\
        &r_C);\
if(current->is_avaiable(mbAddrC))\
{\
    if(current->intra->predinfo.intra4x4->index == 3||\
       current->intra->predinfo.intra4x4->index == 11)\
        E=F=G=H=D;\
    else\
    {\
        E = mbAddrC->cons[0][r_C][c_C + 0];\
        F = mbAddrC->cons[0][r_C][c_C + 1];\
        G = mbAddrC->cons[0][r_C][c_C + 2];\
        H = mbAddrC->cons[0][r_C][c_C + 3];\
    }\
}\
else \
    E=F=G=H=D;\
;

#define GET_NEIGHT_4x4_D() \
neighbour_luma_4x4_position(current, current->intra->predinfo.intra4x4->index, \
          'D',\
    &mbAddrD,\
        &c_D,\
        &r_D);\
if  (mbAddrD)\
{    M = mbAddrD->cons[0][r_D][c_D];\
};

#define get_neighbour_4x4_A GET_NEIGHT_4x4_A
#define get_neighbour_4x4_B GET_NEIGHT_4x4_B
#define get_neighbour_4x4_C GET_NEIGHT_4x4_C
#define get_neighbour_4x4_D GET_NEIGHT_4x4_D

// 从 2 个已有的像素来预测当前像素
#define two_tap_filter(a, b)    ((a + b + 1) >> 1)
// 从 3 个已有的像素来预测当前像素
#define thr_tap_filter(a, b, c) ((a + 2 * b + c + 2) >> 2)

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



// 4x4 一共就16个像素点，每一个都写出来工作量也不是很大
// 用算法去算的话，反而显得复杂
// 更改之后是先算出来预测值，然后对应填入表中，重复的值就不会再计算
// 还是画图比较好理解吧，从代码真的看不出来所以然



/*   
     0 1 2 3 4 5 6 7 8
    +--------------------> 
   0|M|A|B|C|D|E|F|G|H
   1|I|A B C D|     
   2|J|A B C D|  
   3|K|A B C D|
   5|L|A B C D|  
*/
void Prediction_Intra4x4_V                   (macroblock* current, matrix *pred)
{    
    int predIndex = 0;

    // 将要读取的数值， 注意数据对齐
    uint8     A = 0,     B = 0,     C = 0,     D = 0;
    int     r_B = 0,     c_B = 0;
    macroblock *mbAddrB;
    picture* pic = current->pic;

    get_neighbour_4x4_B();
    
    int re[4] = {
        (int)A,(int)B,(int)C,(int)D
    };

    // 按列修改为按行复制
    memcpy(pred[0][0], re, 4 * sizeof(int));
    memcpy(pred[0][1], re, 4 * sizeof(int));
    memcpy(pred[0][2], re, 4 * sizeof(int));
    memcpy(pred[0][3], re, 4 * sizeof(int));

    // current->pred->setl(re, 0, 4);
    // current->pred->setl(re, 1, 4);
    // current->pred->setl(re, 2, 4);
    // current->pred->setl(re, 3, 4);

    // current->pred->setc(re + 0, 0);
    // current->pred->setc(re + 1, 1);
    // current->pred->setc(re + 2, 2);
    // current->pred->setc(re + 3, 3);
    
}
/*   
     0 1 2 3 4 5 6 7 8
    +--------------------> 
   0|M|A|B|C|D|E|F|G|H
   1|I|I I I I|     
   2|J|J J J J|  
   3|K|K K K K|
   5|L|L L L L|  
*/
void Prediction_Intra4x4_H                   (macroblock* current, matrix *pred)
{    
    int predIndex = 0;

    // 将要读取的数值， 注意数据对齐
    uint8     I = 0,     J = 0,     K = 0,     L = 0;
    int     r_A = 0,     c_A = 0;
    
    picture* pic = current->pic;
    macroblock *mbAddrA;

    get_neighbour_4x4_A();
    
    int re[4] = {
        (int)I, 
        (int)J, 
        (int)K, 
        (int)L
    };
    for (int c = 0; c < 4; c++) pred[0][0][c] = I;
    for (int c = 0; c < 4; c++) pred[0][1][c] = J;
    for (int c = 0; c < 4; c++) pred[0][2][c] = K;
    for (int c = 0; c < 4; c++) pred[0][3][c] = L;
    
    // current->pred->setr(re + 0, 0);
    // current->pred->setr(re + 1, 1);
    // current->pred->setr(re + 2, 2);
    // current->pred->setr(re + 3, 3);
}

/*   
    for the Intra_4x4
     0 1 2 3 4 5 6 7 8
    +--------------------> 
   0|M|A|B|C|D|E|F|G|H
   1|I|       |     
   2|J|  DC   |
   3|K|       |
   5|L|       |  
*/
void Prediction_Intra4x4_DC                  (macroblock* current, matrix *pred, int BitDepthY)
{    

    int predIndex = 0;
    uint8     A = 0,     B = 0,     C = 0,     D = 0;
    int     r_B = 0,     c_B = 0;
    uint8     I = 0,     J = 0,     K = 0,     L = 0;
    int     r_A = 0,     c_A = 0;
    picture* pic = current->pic;
    macroblock *mbAddrA;
    macroblock *mbAddrB;

    get_neighbour_4x4_A();
    get_neighbour_4x4_B();
    
    int value = 0;
    if(mbAddrB && mbAddrA)
        value = (int)(((A + B + C + D + I + J + K + L + 4)) >> 3);
    else if(!mbAddrB && mbAddrA)
        value = (int)(((I + J + K + L + 2)) >> 2);
    else if(mbAddrB && !mbAddrA)
        value = (int)(((A + B + C + D + 2)) >> 2);
    else 
        value = (int)(1 << (BitDepthY - 1));

    pred[0] = value;
}
/*   
    for the Intra_4x4
     0 1 2 3 4 5 6 7 8
    +--------------------> 
   0|M|A|B|C|D|E|F|G|H
   1|I|/ / / /|/ / /     
   2|J|/ / / /|/ /
   3|K|/ / / /|/          | /
   5|L|/ / / /|  45 degree|/___
*/
void Prediction_Intra4x4_Diagonal_Down_Left  (macroblock* current, matrix *pred)
{    

    int predIndex = 0;
    uint8     A = 0,     B = 0,     C = 0,     D = 0;
    int     r_B = 0,     c_B = 0;
    uint8     E = 0,     F = 0,     G = 0,     H = 0;
    int     r_C = 0,     c_C = 0;
    picture* pic = current->pic;
    macroblock *mbAddrC;
    macroblock *mbAddrB;

    get_neighbour_4x4_B();
    get_neighbour_4x4_C();
    
    int pr[8] = {A, B, C, D, E, F, G, H};
    int p[7];
    for (int8 i = 0; i < 6; i++)
    {
        p[i] = (int)thr_tap_filter(pr[i], pr[i + 1], pr[i + 2]);
    }
    p[6] = (int)((pr[6] + 3 * pr[7]) >> 2);

    memcpy(pred[0][0], p + 0, 4 * sizeof(int));
    memcpy(pred[0][1], p + 1, 4 * sizeof(int));
    memcpy(pred[0][2], p + 2, 4 * sizeof(int));
    memcpy(pred[0][3], p + 3, 4 * sizeof(int));
}
/*
    for the Intra_4x4
     0 1 2 3 4 5 6 7 8
    +--------------------> 
   0|M|A|B|C|D|E|F|G|H
   1|I|\ \ \ \|\ \ \     
   2|J|\ \ \ \|\ \
   3|K|\ \ \ \|\          |\
   5|L|\ \ \ \|  45 degree|_\__
*/
void Prediction_Intra4x4_Diagonal_Down_Right (macroblock* current, matrix *pred)
{
    int predIndex = 0;
    uint8     A = 0,     B = 0,     C = 0,     D = 0;
    int     r_B = 0,     c_B = 0;
    

    uint8     I = 0,     J = 0,     K = 0,     L = 0;
    int     r_A = 0,     c_A = 0;   
    uint8     M = 0;
    int     r_D = 0,     c_D = 0; 
    picture* pic = current->pic;
    macroblock *mbAddrA;
    macroblock *mbAddrB;
    macroblock *mbAddrD;
    get_neighbour_4x4_A();
    get_neighbour_4x4_B();
    get_neighbour_4x4_D();



    int pr[9] = {L, K, J, I, M, A, B, C, D};
    int p[7];
    for (int8 i = 0; i < 7; i++)
    {
        p[i] = (int)thr_tap_filter(pr[i], pr[i + 1], pr[i + 2]);
    }
    
    memcpy(pred[0][0], p + 3, 4 * sizeof(int));
    memcpy(pred[0][1], p + 2, 4 * sizeof(int));
    memcpy(pred[0][2], p + 1, 4 * sizeof(int));
    memcpy(pred[0][3], p + 0, 4 * sizeof(int));
 
}
void Prediction_Intra4x4_V_Right             (macroblock* current, matrix *pred)
{    
    int predIndex = 0;
    uint8     A = 0,     B = 0,     C = 0,     D = 0;
    int     r_B = 0,     c_B = 0;
    

    uint8     I = 0,     J = 0,     K = 0,     L = 0;
    int     r_A = 0,     c_A = 0;   
    uint8     M = 0;
    int     r_D = 0,     c_D = 0; 
    picture* pic = current->pic;
    macroblock *mbAddrA;
    macroblock *mbAddrB;
    macroblock *mbAddrD;
    get_neighbour_4x4_A();
    get_neighbour_4x4_B();
    get_neighbour_4x4_D();

    int pr[9] = {L, K, J, I, M, A, B, C, D};
    int p[4][4];
    int8 i;
    // p[0]
    for (i = 0; i < 4; i++)
    {
        p[0][i] = (int)two_tap_filter(pr[i + 4], pr[i + 5]);
    }
    // p[1]
    p[1][0] = (int)((pr[3] + 2 * pr[4] + pr[5] + 2) >> 2) ;
    for (i = 0; i < 3; i++)
    {
        p[1][i + 1] = (int)thr_tap_filter(pr[i + 4], pr[i + 5], pr[i + 6]);
        
    }
    // p[2]
    p[2][0] = (int)thr_tap_filter(pr[2], pr[3], pr[4]);
    for (i = 0; i < 3; i++)
    {
        p[2][i + 1] = p[0][i];
    }
    // p[3]
    p[3][0] = (int)thr_tap_filter(pr[1], pr[2], pr[3]) ;
    p[3][1] = p[1][0];
    p[3][2] = p[1][1];
    p[3][3] = p[1][2];

    memcpy(pred[0][0], p[0], 4 * sizeof(int));
    memcpy(pred[0][1], p[1], 4 * sizeof(int));
    memcpy(pred[0][2], p[2], 4 * sizeof(int));
    memcpy(pred[0][3], p[3], 4 * sizeof(int));
}
void Prediction_Intra4x4_H_Down              (macroblock* current, matrix *pred)
{
    int predIndex = 0;
    uint8     A = 0,     B = 0,     C = 0,     D = 0;
    int     r_B = 0,     c_B = 0;
    uint8     E = 0,     F = 0,     G = 0,     H = 0;
    int     r_C = 0,     c_C = 0;
    uint8     I = 0,     J = 0,     K = 0,     L = 0;
    int     r_A = 0,     c_A = 0;   
    uint8     M = 0;
    int     r_D = 0,     c_D = 0; 
    picture* pic = current->pic;
    macroblock *mbAddrA;
    macroblock *mbAddrB;
    macroblock *mbAddrD;
    get_neighbour_4x4_A();
    get_neighbour_4x4_B();
    get_neighbour_4x4_D();

    int pr[9] = {L, K, J, I, M, A, B, C, D};
    int p[4][4];
    int8 i;
    // 
    p[0][0] = (int)two_tap_filter(pr[3], pr[4]);
    for (i = 0; i < 3; i++)
    {
        p[0][i + 1] = (int)thr_tap_filter(pr[i + 3], pr[i + 4], pr[i + 5]);
    }
    
    p[1][0] = (int)two_tap_filter(pr[2], pr[3]);
    p[1][1] = (int)thr_tap_filter(pr[2], pr[3], pr[4]);
    p[1][2] = p[0][0];
    p[1][3] = p[0][1];

    p[2][0] = (int)two_tap_filter(pr[1], pr[2]);
    p[2][1] = (int)thr_tap_filter(pr[1], pr[2], pr[3]);
    p[2][2] = p[1][0];
    p[2][3] = p[1][1];

    p[3][0] = (int)two_tap_filter(pr[0], pr[1]);
    p[3][1] = (int)thr_tap_filter(pr[0], pr[1], pr[2]);
    p[3][2] = p[2][0];
    p[3][3] = p[2][1];

    memcpy(pred[0][0], p[0], 4 * sizeof(int));
    memcpy(pred[0][1], p[1], 4 * sizeof(int));
    memcpy(pred[0][2], p[2], 4 * sizeof(int));
    memcpy(pred[0][3], p[3], 4 * sizeof(int));
}
void Prediction_Intra4x4_V_Left              (macroblock* current, matrix *pred)
{
    int predIndex = 0;
    uint8     A = 0,     B = 0,     C = 0,     D = 0;
    int     r_B = 0,     c_B = 0;
    uint8     E = 0,     F = 0,     G = 0,     H = 0;
    int     r_C = 0,     c_C = 0;
    picture* pic = current->pic;
    macroblock *mbAddrB;
    macroblock *mbAddrC;
    get_neighbour_4x4_B();
    get_neighbour_4x4_C();

    int pr[8] = {A, B, C, D, E, F, G, H};
    int pe_02[5];
    int pe_13[5];

    int8 i;
    for (i = 0; i < 5; i++)
    {
        pe_02[i] = (int)two_tap_filter(pr[i], pr[i + 1]);
        pe_13[i] = (int)thr_tap_filter(pr[i], pr[i + 1], pr[i + 2]);
    }
    
    memcpy(pred[0][0], pe_02 + 0, 4 * sizeof(int));
    memcpy(pred[0][1], pe_13 + 0, 4 * sizeof(int));
    memcpy(pred[0][2], pe_02 + 1, 4 * sizeof(int));
    memcpy(pred[0][3], pe_13 + 1, 4 * sizeof(int));
}
void Prediction_Intra4x4_H_Up                (macroblock* current, matrix *pred)
{
    int predIndex = 0;
    uint8     I = 0,     J = 0,     K = 0,     L = 0;
    int     r_A = 0,     c_A = 0;   
    picture* pic = current->pic;
    macroblock *mbAddrA;
    get_neighbour_4x4_A();

    int pr[4] = {L, K, J, I};
    int p[4][4];
    int8 i;

    p[0][0] = (int)two_tap_filter(pr[2], pr[3]);
    p[0][1] = (int)thr_tap_filter(pr[1], pr[2], pr[3]);
    p[0][2] = (int)two_tap_filter(pr[1], pr[2]);
    p[0][3] = (int)thr_tap_filter(pr[0], pr[1], pr[2]);

    p[1][0] = p[0][2];
    p[1][1] = p[0][3];
    p[1][2] = (int)two_tap_filter(pr[0], pr[1]);
    p[1][3] = (int)((3 * pr[0] + pr[1] + 2) >> 2);

    p[2][0] = p[1][2];
    p[2][1] = p[1][3];
    p[2][2] = pr[0];
    p[2][3] = pr[0];

    for (i = 0; i < 4; i++)
    {
        p[3][i] = pr[0];
    }
    
    
    memcpy(pred[0][0], p[0], 4 * sizeof(int));
    memcpy(pred[0][1], p[1], 4 * sizeof(int));
    memcpy(pred[0][2], p[2], 4 * sizeof(int));
    memcpy(pred[0][3], p[3], 4 * sizeof(int));
}
