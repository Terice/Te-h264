
#include "macroblock.h"
#include "picture.h"
#include "gfunc.h"
#include "gmbinfo.h"

// 换成目标宏块内的坐标
// 目标宏块 在 420 nombaddf 一定是一样的
void target_position(int width, int height, int xN, int yN, int *xW, int *yW)
{
    *xW = (xN + width ) % width;
    *yW = (yN + height) % height;
}
// 计算目标宏块是谁
void target_macroblock(int maxW,  int xN, int yN, char *R)
{
    //目标宏块为自己的情况只有三种，一种是求左边的A，x >=0 ,另一种是求上边B y >=0， 还有是 x >= 0 , y >= 0;
    //确定宏块是哪一个以及是不是当前宏块
    if     (xN >= 0 && yN >= 0 && xN < maxW)    *R = 'N';// 是自己
    else if(xN <  0 && yN <  0)                 *R = 'D';
    else if(xN <  0 && yN >= 0)                 *R = 'A';
    else if(xN >= 0 && yN <  0 && xN < maxW)    *R = 'B';
    else{if (xN >= maxW && yN <  0)             *R = 'C';//
        else                                    *R =  0 ;// 不存在
    }
}

// 没有sub，并且所有的块都具有相同的尺寸
void neighbour_position_nonsub(\
    int index, \
    char direction, \
    int width_part, int height_part,\
    int width_all , int height_all, \
    int * x_result, int *y_result,\
    char *target
)
{    
    /*宏块的坐标轴
        0----> x
        |
        V y
    */
    int8 xD = 0, yD = 0;//相邻块坐标差
    int8 xN = 0, yN = 0;//计算的坐标相对于目标宏块的差
    int xW  = 0, yW = 0;//计算的坐标在宏块中的绝对位置
    int xP  = 0, yP = 0;//计算的坐标在宏块中的绝对位置

    uint8  indexBlockSideWidth = 0,  indexBlockSideHeight = 0;//索引块高和块宽
    uint8 sampleBlockSideWidth = 0, sampleBlockSideHeight = 0;//样点块高和宽

    //全块的宽度和高度，以样点为单位
    int maxW = width_all;
    int maxH = height_all;
    sampleBlockSideHeight = height_part;
    sampleBlockSideWidth = width_part;

    //确定索引块的宽，高
    indexBlockSideWidth  = maxW / sampleBlockSideWidth;
    indexBlockSideHeight = maxH / sampleBlockSideHeight;

    //确定在所取方向上的坐标偏移
    switch (direction)
    {
    case 'A':xD = -1;                   yD =  0; break;
    case 'B':xD =  0;                   yD = -1; break;
    case 'C':xD = sampleBlockSideWidth; yD = -1; break;
    case 'D':xD = -1;                   yD = -1; break;
    default:break;
    }

    //如果是4x4块在16x16中的索引，那么需要逆扫描
    /*if(blockType == 1 && colorType == 1)*/
    // index = block4x4Index[index];
    //索引换成样点块内样点的坐标并且加上偏移 得到 相对坐标
    xN = (index % indexBlockSideWidth ) * sampleBlockSideWidth  + xD;
    yN = (index / indexBlockSideHeight) * sampleBlockSideHeight + yD;
    // 检查目标的位置是哪个宏块，用target来记载
    target_macroblock(sampleBlockSideWidth, xN, yN, target);
    //相对坐标换成目标宏块内坐标
    target_position(maxW, maxH, xN, yN, &xW, &yW); 
    *x_result = xW;
    *y_result = yW;
}
// 有sub，需要 PartIdx， subPartIdx
void neighbour_position_sub(\
    type_macroblock type_cur,\
    type_submacroblock type_cursub,\
    int mbPartIdx, int subPartIdx,\
    char direction, \
    int width_all , int height_all, \
    int *x_result, int *y_result,\
    char *target
)
{
    int x,    y;
    int xS,   yS;
    int xN,   yN;
    int xW,   yW;
    int xD,   yD;
    // 计算 part 起始坐标
    InverseRasterScan_part(mbPartIdx, type_cur, &x, &y);
    // 计算 subpart 起始坐标
    if(type_cur == P_8x8 || type_cur == P_8x8ref0 || type_cur == B_8x8)
        InverseRasterScan_subpart_8x8(subPartIdx, &xS, &yS);
    else xS = yS = 0;
    // 计算预测宽
    int predPartWidth;
    if(type_cur == P_Skip || type_cur == B_Skip || type_cur == B_Direct_16x16) predPartWidth = 16;
    else if(type_cur == B_8x8)
    {
        if(type_cursub == B_Direct_8x8) predPartWidth = 16;// B_Direct_8x8 的预测就是全块的预测
        else predPartWidth = SubMbPartHeight(type_cursub);
    }
    else if(type_cur == P_8x8 || type_cur == P_8x8ref0)
        predPartWidth = SubMbPartHeight(type_cursub);
    else
    {
        predPartWidth = MbPartWidth(type_cur);
    }

    switch (direction)
    {
    case 'A':xD = -1;            yD = 0 ; break;
    case 'B':xD = 0 ;            yD = -1; break;
    case 'C':xD = predPartWidth; yD = -1; break;
    case 'D':xD = -1;            yD = -1; break;
    default: break;
    }
    xN = x + xS + xD;
    yN = y + yS + yD;
    target_macroblock(predPartWidth, xN, yN, target);
    target_position(width_all, height_all, xN, yN, &xW, &yW);
    *x_result = xW;
    *y_result = yW;
}

void neighbour_indice_sub(int xP, int yP, macroblock *N, int *mbPartIdx, int *subPartIdx)
{
    type_macroblock type = N->type;
    // 如果是 I 宏块，
    if(type < TYPE_MACROBLOCK_START_INDEX_P) *mbPartIdx = 0;
    else *mbPartIdx = (16/MbPartWidth(type)) * (yP/MbPartHeight(type)) +(xP/MbPartWidth(type));

    if(type == P_8x8 || type == B_8x8 || type == P_8x8ref0)
    {
        type_submacroblock sub = N->inter->sub[*mbPartIdx].type;
        *subPartIdx = (8/SubMbPartWidth(sub))*((yP%8)/SubMbPartHeight(sub))+(xP%8)/SubMbPartWidth(sub);
    }
    else if(type == B_Skip || type == B_Direct_16x16)
    {
        *subPartIdx = 2*((yP % 8)/4)+((xP % 8)/4);
    }
    else *subPartIdx = 0;
}
void neighbour_indice_luma4x4(int xP, int yP, int *luma4x4BlkIdx)
{
    *luma4x4BlkIdx = 8*(yP/8)+4*(xP/8)+2*((yP%8)/4)+((xP%8)/4);
}
void neighbour_indice_luma8x8(int xP, int yP, int *luma8x8BlkIdx)
{
    *luma8x8BlkIdx = 2*(yP/8)+(xP/8);
}



MacroBlockNeighInfo neighbour_part(macroblock *current, int mbPartIdx, int subPartIdx)
{
    MacroBlockNeighInfo result;
    result.avaiable = false;

    
}