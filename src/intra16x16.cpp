#include "macroblock.h"

#include "gfunc.h"

#include "parser.h"
#include "pixmap.h"
#include <string.h>

void Prediction_Intra16x16_V(macroblock *current, parser* pa)
{

    macroblock* B = current->neighbour.B.pointer;

    if(!current->neighbour.B.avaiable) return;

    pix16 data[16] ;
    pix8* res = (pix8*)(B->cons->get(15));
    for (int8 i = 0; i < 16; i++)
    {
        data[i] = res[i];
    }
    for (int8 i = 0; i < 16; i++)
    {
        current->pred->setl(data, i, 16);
    }
    
}
void Prediction_Intra16x16_H(macroblock *current, parser* pa)
{
    macroblock* A = current->neighbour.A.pointer;
    if(!current->neighbour.B.avaiable) return;


    pix8 data;
    pix16 temp;
    for (size_t r = 0; r < 16; r++)
    {
        A->cons->get(r, 15, &data);
        temp = (pix16)data;
        current->pred->setr(&temp, r);
    }
}
void Prediction_Intra16x16_DC(macroblock *current, parser* pa)
{
    MacroBlockNeighInfo* A = &current->neighbour.A;
    MacroBlockNeighInfo* B = &current->neighbour.B;
    int sum_a = 0, sum_b = 0;
    pix8 data;
    if(A->avaiable)
    {
        for (size_t i = 0; i < 16; i++)
        {
            A->pointer->cons->get(15, i, &data);
            sum_a += (int)data;
        }
    }
    if(B->avaiable)
    {
        for (size_t i = 0; i < 16; i++)
        {
            B->pointer->cons->get(15, i, &data);
            sum_b += (int)data;
        }
    }
    pix16 data_in;
    if( A->avaiable && B->avaiable)
        data_in = (pix16)((sum_a + 8 + sum_b + 8 ) >> 5) ;
    else if(A->avaiable) 
        data_in = (pix16)((sum_a + 8) >> 4) ;
    else if(B->avaiable) 
        data_in = (pix16)((sum_b + 8) >> 4) ;
    else 
        data_in = (pix16)(1 << (pa->pV->BitDepthY - 1));
    //

    current->pred->seta(&data_in);
}
void Prediction_Intra16x16_Plane(macroblock *current, parser* pa)
{
    int16 H_samples[17] ; memset(H_samples, 0, 17 * sizeof(int16));
    int16 V_samples[17] ; memset(V_samples, 0, 17 * sizeof(int16));
    MacroBlockNeighInfo* A = &current->neighbour.A;
    MacroBlockNeighInfo* B = &current->neighbour.B;
    MacroBlockNeighInfo* D = &current->neighbour.D;

    pix8 data_out;
    if(D->avaiable) 
    {
        D->pointer->cons->get(15,15, &data_out);
        H_samples[0] = V_samples[0] = (int16)data_out;

    }
    if(A->avaiable)
    {
        for (uint8 i = 0; i < 16; i++)
        {
            A->pointer->cons->get(i, 15 , &data_out);
            H_samples[i + 1] = (int16)data_out;
        }
    }
    if(B->avaiable)
    {
        for (uint8 i = 0; i < 16; i++)
        {
            B->pointer->cons->get(i, 15 , &data_out);
            V_samples[i + 1] = (int16)data_out;
        }
    }

    int H = 0, V = 0;
    for (uint8 i = 9; i < 17; i++){H += (i - 8) * (H_samples[i] - H_samples[16 - i]);}
    for (uint8 i = 9; i < 17; i++){V += (i - 8) * (V_samples[i] - V_samples[16 - i]);}
    
    int a = 16 * (H_samples[16] + V_samples[16]);
    int b = (5 * H + 32) >> 6;
    int c = (5 * V + 32) >> 6;

    pix16 data_in;
    for (uint8 row = 0; row < 16; row++)
    {
        for (uint8 col = 0; col < 16; col++)
        {
            data_in = (pix16)Clip1Y(((a + b * (col - 7) + c * (row - 7) + 16) >> 5),pa->pV->BitDepthY);
            current->cons->set(col, row, &data_in);
        }
    }
}