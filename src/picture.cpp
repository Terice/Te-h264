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