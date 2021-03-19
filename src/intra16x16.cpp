#include "macroblock.h"

#include "gfunc.h"

#include "parser.h"
#include "pixmap.h"
#include <string.h>
#include "matrix.h"

#include "prediction.h"

void Prediction_Intra16x16_V(macroblock *current)
{

    macroblock* B = current->neighbour.B.pointer;

    if(!current->neighbour.B.avaiable) return;

    int data[16] ;
    // 得到 最后一行的数据指针
    uint8* res = B->cons[0][15];
    for (int i = 0; i < 16; i++)
    {// 格式转换
        data[i] = res[i];
    }
    // 复制
    for (int i = 0; i < 16; i++)
    {
        memcpy(current->pred[0][i], data, 16 * sizeof(int));
    }
    
}
void Prediction_Intra16x16_H(macroblock *current)
{
    macroblock* A = current->neighbour.A.pointer;


    uint8 data;
    int temp;
    for (size_t r = 0; r < 16; r++)
    {
        current->pred->setr(r, A->cons[0][r][15]);
    }
}
void Prediction_Intra16x16_DC(macroblock *current, int BitDepthY)
{
    MacroBlockNeighInfo* A = &current->neighbour.A;
    MacroBlockNeighInfo* B = &current->neighbour.B;
    int sum_a = 0, sum_b = 0;
    uint8 data;
    if(A->avaiable)
    {
        for (size_t i = 0; i < 16; i++)
        {
            // A->pointer->cons[0][15][i] = data;
            sum_a += A->pointer->cons[0][i][15];
        }
    }
    if(B->avaiable)
    {
        for (size_t i = 0; i < 16; i++)
        {
            // B->pointer->cons[0][15][i] = data;
            // sum_b += (int)data;

            sum_b += B->pointer->cons[0][15][i];
        }
    }
    int data_in;
    if( A->avaiable && B->avaiable)
        data_in = (int)((sum_a + 8 + sum_b + 8 ) >> 5) ;
    else if(A->avaiable) 
        data_in = (int)((sum_a + 8) >> 4) ;
    else if(B->avaiable) 
        data_in = (int)((sum_b + 8) >> 4) ;
    else 
        data_in = (int)(1 << (BitDepthY - 1));
    //

    current->pred[0] = data_in;
}
void Prediction_Intra16x16_Plane(macroblock *current, int BitDepthY)
{
    int16 H_samples[17] ; memset(H_samples, 0, 17 * sizeof(int16));
    int16 V_samples[17] ; memset(V_samples, 0, 17 * sizeof(int16));
    MacroBlockNeighInfo* A = &current->neighbour.A;
    MacroBlockNeighInfo* B = &current->neighbour.B;
    MacroBlockNeighInfo* D = &current->neighbour.D;

    int M;


    if(!D->avaiable || !A->avaiable || !B->avaiable) return;// 应该直接退出

    M = D->pointer->cons[0][15][15];
    int i;
    /*
    
    | M         H
    -    ----------------
    |   |
    | V |   Cur16x16
    |   |
    |   |
    
    */


    for (i = 1; i < 17; i++) V_samples[i] = A->pointer->cons[0][i-1][15];
    for (i = 1; i < 17; i++) H_samples[i] = B->pointer->cons[0][15][i-1];
    H_samples[0] = V_samples[0] = M;

    int H = 0, V = 0;
    for (i = 0; i < 8; i++) 
        H += (i + 1) * (H_samples[i + 9] - H_samples[7 - i]);
    for (i = 0; i < 8; i++) 
        V += (i + 1) * (V_samples[i + 9] - V_samples[7 - i]);
    //
    int a = 16 * (H_samples[16] + V_samples[16]);
    int b = (5 * H + 32) >> 6;
    int c = (5 * V + 32) >> 6;

    int data_in[16][16];
    for (int row = 0; row < 16; row++)
    {
        int ibb = a +  c * (row - 7) + 16;
        for (int col = 0; col < 16; col++)
        {
            data_in[row][col] = (int)Clip1Y((((ibb + b * (col - 7)) ) >> 5), BitDepthY);
            // fprintf(stderr, "%3d ", data_in[row][col]);
            // if(col%4 == 3) 
            // fprintf(stderr,"|");
        }
        // printf("\n");
    }
    for (int i = 0; i < 16; i++)
    {
        memcpy(current->pred[0][i], data_in[i], 16 * sizeof(int));
    }
    
}