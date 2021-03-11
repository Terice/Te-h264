#include "picture.h"

#include "gchart.h"

#include "parser.h"
#include "decoder.h"

#include "slice.h"
#include "macroblock.h"

picture::picture(parser* p, decoder* d)
{
    this->pa = p;
    this->de = d;
    this->IdrPicFlag = false;
    
    // 如果开始解picture了，那么说明sps和pps一定都出现过了，
    // 所以这里可以使用这些参数
    
    mb = new array2d<macroblock*>(pa->pV->PicWidthInMbs ,pa->pV->PicHeightInMbs, NULL);
    size.w = pa->pV->PicWidthInMbs  * 16;
    size.h = pa->pV->PicHeightInMbs * 16;

    // pix16 zero = 0;
    // int16 zero_i = 0;
    // pred = new array2d<pix16>(size.w, size.h, 0);
    // resi = new array2d<int16>(size.w, size.h, 0);
    cons = new array2d <byte>(size.w, size.h);
}
picture::~picture()
{
    // 删除宏块组
    delete mb;
    // 删除slice
    delete sl;

    // delete pred;
    // delete resi;
    delete cons;
    
}

void picture::deocde()
{
    sl = new slice(pa, de, this);
    // 设置解析器当前的slice
    // 由于简化了，所以这里就只有一个slice
    pa->cur_slice = this->sl;

    sl->decode();
    // sl解码完毕不出问题，那么给到解码完毕队列中
    de->add_DecodedPic(this);
}

void detect(macroblock *current, MacroBlockNeighInfo *info, int x, int y, int dx, int dy, picture* pic)
{
    int pos_x = x + dx;
    int pos_y = y + dy;
    if(pos_x < 0 || pos_y < 0 || pos_x > pic->size.w || pos_y > pic->size.h)
    {
        info->avaiable = false;
        info->pointer = NULL;
        return ;
    }
    macroblock* target = (pic->mb->get(pos_x, pos_y));
    if(!target) 
    {
        info->avaiable = false;
        info->pointer = NULL;
        return ;
    }
    else
    {
        info->pointer = target;
        if(target->intea_mode != current->intea_mode) info->avaiable = false;
        else
        {
            if(current->predmode == Intra_16x16 && target->predmode != Intra_16x16) 
                info->avaiable = false;
        }
    }
    
}
void picture::takein(macroblock *m)
{
    int pos_x = 0;
    int pos_y = 0;
    int curpos_x = m->pos.x;
    int curpos_y = m->pos.y;

    MacroBlockNeigh *neigh = &m->neighbour;
    detect(m, &m->neighbour.A, curpos_x, curpos_y, -1, 0, this);
    detect(m, &m->neighbour.B, curpos_x, curpos_y,  0,-1, this);
    detect(m, &m->neighbour.C, curpos_x, curpos_y, +1,+1, this);
    detect(m, &m->neighbour.D, curpos_x, curpos_y, -1,-1, this);
    
}
void picture::neighbour_4x4block(macroblock *current, int index_current, char direction, macroblock **target, int *index_target, int *x = NULL, int * y = NULL)
// void picture::neighbour_4x4block(macroblock* mb_current, char direction,int index_current, uint32 mode,  int *indexResult, int *r, int *c)
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
    macroblock* R = NULL;

    int maxW = 16, maxH = 16;//全块的宽度和高度，以样点为单位
    sampleBlockSideHeight = sampleBlockSideWidth = 4;

    //确定索引块的宽，高
    indexBlockSideWidth  = maxW / sampleBlockSideWidth;
    indexBlockSideHeight = maxH / sampleBlockSideHeight;

    //确定在所取方向上的坐标偏移
    switch (direction)
    {
    case 'A':xD = -1;   yD =  0; break;
    case 'B':xD =  0;   yD = -1; break;
    case 'C':xD = sampleBlockSideWidth; yD = -1; break;
    case 'D':xD = -1;   yD = -1; break;
    default:break;
    }

    //如果是4x4块在16x16中的索引，那么需要逆扫描
    /*if(blockType == 1 && colorType == 1)*/
    index_current = block4x4Index[index_current];
    //索引换成样点块内样点的坐标并且加上偏移 得到 相对坐标
    xN = (index_current % indexBlockSideWidth ) * sampleBlockSideWidth  + xD;
    yN = (index_current / indexBlockSideHeight) * sampleBlockSideHeight + yD;
    
    //目标宏块为自己的情况只有三种，一种是求左边的A，x >=0 ,另一种是求上边B y >=0， 还有是 x >= 0 , y >= 0;
    //确定宏块是哪一个以及是不是当前宏块
    if     (xN >= 0 && yN >= 0 && xN < maxW)    R = current;
    else if(xN <  0 && yN <  0)                 R = neighbour_macroblock(current, 'D');
    else if(xN <  0 && yN >= 0)                 R = neighbour_macroblock(current, 'A');
    else if(xN >= 0 && yN <  0 && xN < maxW)    R = neighbour_macroblock(current, 'B');
    else{
        if (xN >= maxW && yN <  0)              R = neighbour_macroblock(current, 'C');
        else                                    R = NULL;
    }
    //相对坐标换成目标宏块内坐标
    xW = (xN + maxW) % maxW;
    yW = (yN + maxH) % maxH;
    //目标宏块内坐标换成索引并输出
    if(!current->is_avaiable(R))
    {
        *index_target = -1;
        *target =  R;
        if(!y && !x)
        {
            *y = -1;
            *x = -1;
        }
    }
    else
    {
        xP = xW;
        yP = yW;
        if(!y && !x)
        {
            *y = yP;
            *x = xP;
        }
        *index_target = indexBlockSideHeight * (yP / sampleBlockSideHeight) + (xP / sampleBlockSideWidth);
        //如果是luma4x4块，那么换到4x4索引
        *index_target = block4x4Index[*index_target];
    }
    *target =  R;
}