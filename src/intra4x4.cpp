#include "macroblock.h"
#include "picture.h"
#include "pixmap.h"
#include "matrix.h"
#include "parser.h"
#define GET_NEIGHT_4x4_A() \
pic->neighbour_4x4block(current, current->intra->predinfo.intra4x4->index, \
          'A',\
    &mbAddrA,&predIndex,\
        &c_A,\
        &r_A);\
if  (mbAddrA)\
{    mbAddrA->cons->get(r_A + 0, c_A,  &I);\
     mbAddrA->cons->get(r_A + 1, c_A,  &J);\
     mbAddrA->cons->get(r_A + 2, c_A,  &K);\
     mbAddrA->cons->get(r_A + 3, c_A,  &L);\
};
#define GET_NEIGHT_4x4_B() \
pic->neighbour_4x4block(current, current->intra->predinfo.intra4x4->index, \
          'B',\
    &mbAddrB,&predIndex,\
        &c_B,\
        &r_B);\
if  (mbAddrB)\
{    mbAddrB->cons->get(r_B, c_B + 0,  &A);\
     mbAddrB->cons->get(r_B, c_B + 1,  &B);\
     mbAddrB->cons->get(r_B, c_B + 2,  &C);\
     mbAddrB->cons->get(r_B, c_B + 3,  &D);\
};
#define GET_NEIGHT_4x4_C() \
pic->neighbour_4x4block(current, current->intra->predinfo.intra4x4->index,\
          'C',\
    &mbAddrC,&predIndex,\
        &c_C,\
        &r_C);\
if(current->is_avaiable(mbAddrC))\
{\
    if(current->intra->predinfo.intra4x4->index == 3||\
       current->intra->predinfo.intra4x4->index == 11)\
        E=F=G=H=D;\
    else\
    {\
        mbAddrC->cons->get(r_C, c_C + 0,  &E);\
        mbAddrC->cons->get(r_C, c_C + 1,  &F);\
        mbAddrC->cons->get(r_C, c_C + 2,  &G);\
        mbAddrC->cons->get(r_C, c_C + 3,  &H);\
    }\
}\
else \
    E=F=G=H=D;\
;

#define GET_NEIGHT_4x4_D() \
pic->neighbour_4x4block(current, current->intra->predinfo.intra4x4->index,\
          'D',\
    &mbAddrD,&predIndex,\
        &c_D,\
        &r_D);\
if  (mbAddrD)\
{    mbAddrD->cons->get(r_D + 3, c_D + 3,  &M);\
};

#define get_neighbour_4x4_A GET_NEIGHT_4x4_A
#define get_neighbour_4x4_B GET_NEIGHT_4x4_B
#define get_neighbour_4x4_C GET_NEIGHT_4x4_C
#define get_neighbour_4x4_D GET_NEIGHT_4x4_D

// 从 2 个已有的像素来预测当前像素
#define two_tap_filter(a, b)    ((a + b) >> 2)
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
void Prediction_Intra4x4_V                   (macroblock* current)
{    
    int predIndex = 0;

    // 将要读取的数值， 注意数据对齐
    pix8     A = 0,     B = 0,     C = 0,     D = 0;
    int     r_B = 0,     c_B = 0;
    macroblock *mbAddrB;
    picture* pic = current->pic;

    get_neighbour_4x4_B();
    
    pix16 re[4] = {
        (pix16)A,(pix16)B,(pix16)C,(pix16)D
    };

    // 按列修改为按行复制

    current->pred->setl(re, 0, 4);
    current->pred->setl(re, 1, 4);
    current->pred->setl(re, 2, 4);
    current->pred->setl(re, 3, 4);

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
void Prediction_Intra4x4_H                   (macroblock* current)
{    
    int predIndex = 0;

    // 将要读取的数值， 注意数据对齐
    pix8     I = 0,     J = 0,     K = 0,     L = 0;
    int     r_A = 0,     c_A = 0;
    
    picture* pic = current->pic;
    macroblock *mbAddrA;

    get_neighbour_4x4_A();
    
    pix16 re[4] = {
        (pix16)I, 
        (pix16)J, 
        (pix16)K, 
        (pix16)L
    };
    current->pred->setr(re + 0, 0);
    current->pred->setr(re + 1, 1);
    current->pred->setr(re + 2, 2);
    current->pred->setr(re + 3, 3);
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
void Prediction_Intra4x4_DC                  (macroblock* current, parser *pa)
{    

    int predIndex = 0;
    pix8     A = 0,     B = 0,     C = 0,     D = 0;
    int     r_B = 0,     c_B = 0;
    pix8     I = 0,     J = 0,     K = 0,     L = 0;
    int     r_A = 0,     c_A = 0;
    picture* pic = current->pic;
    macroblock *mbAddrA;
    macroblock *mbAddrB;

    get_neighbour_4x4_A();
    get_neighbour_4x4_B();
    
    pix16 value = 0;
    if(!mbAddrB && !mbAddrA)
        value = (pix16)(((A + B + C + D + I + J + K + L + 4)) >> 3);
    else if(!mbAddrB && mbAddrA)
        value = (pix16)(((I + J + K + L + 2)) >> 2);
    else if(mbAddrB && !mbAddrA)
        value = (pix16)(((A + B + C + D + 2)) >> 2);
    else 
        value = (pix16)(1 << (pa->pV->BitDepthY - 1));
    current->pred->seta(&value);
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
void Prediction_Intra4x4_Diagonal_Down_Left  (macroblock* current)
{    
    pixmap res;
    int predIndex = 0;
    pix8     A = 0,     B = 0,     C = 0,     D = 0;
    int     r_B = 0,     c_B = 0;
    pix8     E = 0,     F = 0,     G = 0,     H = 0;
    int     r_C = 0,     c_C = 0;
    picture* pic = current->pic;
    macroblock *mbAddrC;
    macroblock *mbAddrB;

    get_neighbour_4x4_B();
    get_neighbour_4x4_C();
    
    int pr[8] = {A, B, C, D, E, F, G, H};
    pix16 p[7];
    for (int8 i = 0; i < 6; i++)
    {
        p[i] = (pix16)thr_tap_filter(pr[i], pr[i + 1], pr[i + 2]);
    }
    p[6] = (pix16)((pr[6] + 3 * pr[7]) >> 2);
    res.setl(p + 0, 0, 4);
    res.setl(p + 1, 1, 4);
    res.setl(p + 2, 2, 4);
    res.setl(p + 3, 3, 4);
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
void Prediction_Intra4x4_Diagonal_Down_Right (macroblock* current)
{
    pixmap res;
    int predIndex = 0;
    pix8     A = 0,     B = 0,     C = 0,     D = 0;
    int     r_B = 0,     c_B = 0;
    pix8     E = 0,     F = 0,     G = 0,     H = 0;
    int     r_C = 0,     c_C = 0;
    pix8     I = 0,     J = 0,     K = 0,     L = 0;
    int     r_A = 0,     c_A = 0;   
    pix8     M = 0;
    int     r_D = 0,     c_D = 0; 
    picture* pic = current->pic;
    macroblock *mbAddrA;
    macroblock *mbAddrB;
    macroblock *mbAddrD;
    get_neighbour_4x4_A();
    get_neighbour_4x4_B();
    get_neighbour_4x4_D();

    pix16 value;
    int pr[9] = {L, K, J, I, M, A, B, C, D};
    pix16 p[7];
    for (int8 i = 0; i < 7; i++)
    {
        p[i] = (pix16)(pr[i] + 2 * pr[i + 1] + pr[i + 2] + 2 ) >> 2;
    }
    res.setl(p + 0, 0, 4);
    res.setl(p + 1, 1, 4);
    res.setl(p + 2, 2, 4);
    res.setl(p + 3, 3, 4);
 
}
void Prediction_Intra4x4_V_Right             (macroblock* current)
{    
    pixmap res;
    int predIndex = 0;
    pix8     A = 0,     B = 0,     C = 0,     D = 0;
    int     r_B = 0,     c_B = 0;
    pix8     E = 0,     F = 0,     G = 0,     H = 0;
    int     r_C = 0,     c_C = 0;
    pix8     I = 0,     J = 0,     K = 0,     L = 0;
    int     r_A = 0,     c_A = 0;   
    pix8     M = 0;
    int     r_D = 0,     c_D = 0; 
    picture* pic = current->pic;
    macroblock *mbAddrA;
    macroblock *mbAddrB;
    macroblock *mbAddrD;
    get_neighbour_4x4_A();
    get_neighbour_4x4_B();
    get_neighbour_4x4_D();

    int pr[9] = {L, K, J, I, M, A, B, C, D};
    pix16 p[4][4];
    int8 i;
    // p[0]
    for (i = 0; i < 4; i++)
    {
        p[0][i] = (pix16)two_tap_filter(pr[i + 4], pr[i + 5]);
    }
    // p[1]
    p[1][0] = (pix16)((pr[3] + 2 * pr[4] + pr[5] + 2) >> 2) ;
    for (i = 0; i < 3; i++)
    {
        p[1][i + 1] = (pix16)thr_tap_filter(pr[i + 4], pr[i + 5], pr[i + 6]);
        
    }
    // p[2]
    p[2][0] = (pix16)thr_tap_filter(pr[2], pr[3], pr[4]);
    for (i = 0; i < 3; i++)
    {
        p[2][i + 1] = p[0][i];
    }
    // p[3]
    p[3][0] = (pix16)thr_tap_filter(pr[1], pr[2], pr[3]) ;
    p[3][1] = p[1][0];
    p[3][2] = p[1][1];
    p[3][3] = p[1][2];


    res.setl(p[0], 0, 4);
    res.setl(p[1], 1, 4);
    res.setl(p[2], 2, 4);
    res.setl(p[3], 3, 4);
}
void Prediction_Intra4x4_H_Down              (macroblock* current)
{
    pixmap res;
    int predIndex = 0;
    pix8     A = 0,     B = 0,     C = 0,     D = 0;
    int     r_B = 0,     c_B = 0;
    pix8     E = 0,     F = 0,     G = 0,     H = 0;
    int     r_C = 0,     c_C = 0;
    pix8     I = 0,     J = 0,     K = 0,     L = 0;
    int     r_A = 0,     c_A = 0;   
    pix8     M = 0;
    int     r_D = 0,     c_D = 0; 
    picture* pic = current->pic;
    macroblock *mbAddrA;
    macroblock *mbAddrB;
    macroblock *mbAddrD;
    get_neighbour_4x4_A();
    get_neighbour_4x4_B();
    get_neighbour_4x4_D();

    int pr[9] = {L, K, J, I, M, A, B, C, D};
    pix16 p[4][4];
    int8 i;
    // 
    p[0][0] = (pix16)two_tap_filter(pr[3], pr[4]);
    for (i = 0; i < 3; i++)
    {
        p[0][i + 1] = (pix16)thr_tap_filter(pr[i + 3], pr[i + 4], pr[i + 5]);
    }
    
    p[1][0] = (pix16)two_tap_filter(pr[2], pr[3]);
    p[1][1] = (pix16)thr_tap_filter(pr[2], pr[3], pr[4]);
    p[1][2] = p[0][0];
    p[1][3] = p[0][1];

    p[2][0] = (pix16)two_tap_filter(pr[1], pr[2]);
    p[2][1] = (pix16)thr_tap_filter(pr[1], pr[2], pr[3]);
    p[2][2] = p[1][0];
    p[2][3] = p[1][1];

    p[3][0] = (pix16)two_tap_filter(pr[0], pr[1]);
    p[3][1] = (pix16)thr_tap_filter(pr[0], pr[1], pr[2]);
    p[3][2] = p[2][0];
    p[3][3] = p[2][1];

    res.setl(p[0], 0, 4);
    res.setl(p[1], 1, 4);
    res.setl(p[2], 2, 4);
    res.setl(p[3], 3, 4);
}
void Prediction_Intra4x4_V_Left              (macroblock* current)
{
    pixmap res;
    int predIndex = 0;
    pix8     A = 0,     B = 0,     C = 0,     D = 0;
    int     r_B = 0,     c_B = 0;
    pix8     E = 0,     F = 0,     G = 0,     H = 0;
    int     r_C = 0,     c_C = 0;
    picture* pic = current->pic;
    macroblock *mbAddrB;
    macroblock *mbAddrC;
    get_neighbour_4x4_B();
    get_neighbour_4x4_C();

    int pr[8] = {A, B, C, D, E, F, G, H};
    pix16 pe_02[5];
    pix16 pe_13[5];

    int8 i;
    for (i = 0; i < 5; i++)
    {
        pe_02[i] = (pix16)two_tap_filter(pr[i], pr[i + 1]);
        pe_13[i] = (pix16)thr_tap_filter(pr[i], pr[i + 1], pr[i + 2]);
    }
    
    
    res.setl(pe_02 + 0, 0, 4);
    res.setl(pe_13 + 0, 1, 4);
    res.setl(pe_02 + 1, 2, 4);
    res.setl(pe_13 + 1, 3, 4);
}
void Prediction_Intra4x4_H_Up                (macroblock* current)
{
    pixmap res;
    int predIndex = 0;
    pix8     I = 0,     J = 0,     K = 0,     L = 0;
    int     r_A = 0,     c_A = 0;   
    picture* pic = current->pic;
    macroblock *mbAddrA;
    get_neighbour_4x4_A();

    int pr[4] = {L, K, J, I};
    int p[4][4];
    int8 i;

    p[0][0] = (pix16)two_tap_filter(pr[2], pr[3]);
    p[0][1] = (pix16)thr_tap_filter(pr[1], pr[2], pr[3]);
    p[0][3] = (pix16)two_tap_filter(pr[1], pr[2]);
    p[0][4] = (pix16)thr_tap_filter(pr[0], pr[1], pr[2]);

    p[1][0] = p[0][2];
    p[1][1] = p[0][3];
    p[1][2] = (pix16)two_tap_filter(pr[0], pr[1]);
    p[1][3] = (pix16)((3 * pr[0] + pr[1] + 2) >> 2);

    p[2][0] = p[1][2];
    p[2][1] = p[1][3];
    p[2][2] = pr[0];
    p[2][3] = pr[0];

    for (i = 0; i < 4; i++)
    {
        p[3][i] = pr[0];
    }
    
    
    res.setl(p[0], 0, 4);
    res.setl(p[1], 1, 4);
    res.setl(p[2], 2, 4);
    res.setl(p[3], 3, 4);
}