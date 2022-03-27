static int t4[16] = 
{
    1,  1,  1,  1,
    1,  1, -1, -1, 
    1, -1, -1,  1,
    1, -1,  1, -1
};

static int t2[4] =
{
    1, 1,
    1,-1
};

#include "parser.h"
#include "cabac.h"
#include "terror.h"
#include "decoder.h"
#include "reader.h"
#include "sps.h"
#include "pps.h"
#include <math.h>

// 刷新序列参数集
bool parser::update_ps(SPS* s)
{
    if(pS->sps) 
        delete pS->sps;
    pS->sps = s;


    // 注意以下过程需要改进放在上面的update_ps函数中去
    // 把一些SPS已经能够解析的传送给parser
    this->pV->ChromaArrayType     = s->separate_colour_plane_flag == 0 ? s->chroma_format_idc : 0;
    this->pV->BitDepthY           = s->bit_depth_luma_minus8 + 8;
    this->pV->QpBdOffsetY         = s->bit_depth_luma_minus8 * 6;
    this->pV->BitDepthC           = s->bit_depth_chroma_minus8 + 8;
    this->pV->QpBdOffsetC         = s->bit_depth_chroma_minus8 * 6;
    this->pV->MaxFrameNum         = (uint32_t)pow(2, s->log2_max_frame_num_minus4 + 4);
    this->pV->PicWidthInMbs       = s->pic_width_in_mbs_minus1 + 1;
    this->pV->PicHeightInMapUnits = s->pic_height_in_map_units_minus1 + 1;
    this->pV->PicHeightInMbs      = s->pic_height_in_map_units_minus1 + 1;
    this->pV->FrameHeightInMbs    = ( 2 - s->frame_mbs_only_flag ) * this->pV->PicHeightInMapUnits;
    this->pV->PicSizeInMapUnits   = this->pV->PicWidthInMbs * this->pV->PicHeightInMapUnits;
    this->pV->SubWidthC           = (s->chroma_format_idc == 1 || s->chroma_format_idc == 2) ? 2 : 1;
    this->pV->SubHeightC          = (s->chroma_format_idc == 1) ? 2 : 1;


    return true;
}
// 刷新图像参数集
bool parser::update_ps(PPS* p)
{
    if(pS->pps) 
        delete pS->pps;
    pS->pps = p;

    return true;
}
void parser::set_cabac_slice_new(picture* pic, slice *sl){cabac_core->slice_new(pic,sl);}
void parser::set_cabac_slice_end(){cabac_core->slice_end();}

// bool parser::algi()   {return bitstream->balgi();}

// uint64 parser::next(int size){return bitstream->bnext(size);}
bool   parser::read_bi()             {return bitstream->bread_bi();}
short  parser::read_ch()             {return bitstream->bread_ch();};
bool   parser::read_al()             {return bitstream->bforc_ne();}
uint64 parser::read_un(int size)     {return bitstream->bread_bn(size);}
int64  parser::read_sn(int size)     {return bitstream->bread_bn(size);}
uint64 parser::read_ue()             {return bitstream->bread_ue();}
int64  parser::read_se()             {return bitstream->bread_se();}
uint64 parser::read_me(int mb_type)  {return bitstream->bread_me(this->pV->ChromaArrayType, mb_type);}
uint64 parser::read_te(int range)    {return bitstream->bread_te(range);}
uint64 parser::read_ce()             {return 0;}
uint64 parser::read_ae(uint32 syntax){return cabac_core->cread_ae(syntax);}
uint16 parser::read_12()             {return 0;}
bool   parser::find_nextNAL()
{
    // nal 的起始码 ： 0x 00 00 01
    uint8 state = 0;
    //强制对齐到当前未对齐索引后面第一个对齐位
    bitstream->bforc_ne();
    while(state != 3)
    {
        //这里只是找数据，所以可以用short类型
        short tmp = read_ch();
        if(tmp == -1) return false;
        /*  
                                    0                    .__________.
                                   |                    |  finded  |
            [0] --0--> [1] --0--> [2] --1--> [3] ------>|__________|
              |   |          |          |          |             |
              |<--/ other    |          |          |             |
              |<-------------/ other    |          |             |
              |<------------------------/ other    |             |
              |<-----------------------------------/other        |
              \<-------------------------------------------------/
        */
        switch (state)
        {
            case 0:if(tmp == 0) state = 1; break; // 第一个 00 
            case 1:if(tmp == 0) state = 2; else state = 0; break; // 第二个 00 
            case 2:\
                if(tmp == 1) state = 3;\
                else if(tmp == 0) state = 2;\
                else state = 0; break;
            default:state = 0;break;
        }
    }
    return true;
}
// void parser::rfsh(){bitstream->bfrsh();}

parser::parser(FILE* fp)
{
    
    bitstream = new reader(fp);
    cabac_core = new cabac(this);
    pS = new ParametersS();
    pV = new ParametersV();
    pS->pps = NULL;
    pS->sps = NULL;
    matrix_4x4Trans = new matrix(4,4,0);
    matrix_2x2Trans = new matrix(2,2,0);
    matrix_4x4Trans->from(t4, 16);
    matrix_2x2Trans->from(t2, 4);
    index_cur_slice = 0;
}
parser::~parser()
{    
    if(pS->pps)    delete pS->pps;
    if(pS->sps)    delete pS->sps;
    delete cabac_core;
    delete pS;
    delete pV;
}
