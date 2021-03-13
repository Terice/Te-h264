#include "nal.h"

#include <iostream>
#include <cmath>

#include "parser.h"
#include "decoder.h"
#include "pps.h"
#include "sps.h"
#include "picture.h"
#include "sei.h"

nal::nal(parser* p, decoder* d)
{
    this->pa = p;
    this->de = d;
}
nal::~nal()
{
    // 注意SPS， PPS， 以及每个图像pic 都不是由nal来管理的，所以这里不需要释放
    // 这里的void* data其实没有实际的意义，
    // 但是不搞一个 data 记得那些解过的东西，感觉 nal 存在感也太低了————
}
bool nal::decode()
{
    forbidden_zero_bit = pa->read_un(1);
    nal_ref_idc        = pa->read_un(2);
    nal_unit_type      = pa->read_un(5);
    
    pa->cur_nal = this;
    switch (nal_unit_type)
    {
        case NAL_TYPE_NONIDR:decode_PIC();break;
        case NAL_TYPE_IDR   :decode_IDR();break;
        case NAL_TYPE_PPS   :decode_PPS();break;
        case NAL_TYPE_SPS   :decode_SPS();break;
        case NAL_TYPE_SEI   :decode_SEI();break;
        default:std::cout << ">> nal: no fn to deocde type: [" << nal_unit_type << "]" << std::endl;
    }
    return true;
}
void nal::decode_SPS()
{
    SPS *s = new SPS(pa);
    this->data = s;
    // 先解码参数集，
    s->decode();
    // 然后再挂参数
    pa->update_ps(s);// 把解码参数指针挂上去

    // 注意以下过程需要改进放在上面的update_ps函数中去
    // 把一些SPS已经能够解析的传送给parser
    pa->pV->ChromaArrayType     = s->separate_colour_plane_flag == 0 ? s->chroma_format_idc : 0;
    pa->pV->BitDepthY           = s->bit_depth_luma_minus8 + 8;
    pa->pV->QpBdOffsetY         = s->bit_depth_luma_minus8 * 6;
    pa->pV->BitDepthC           = s->bit_depth_chroma_minus8 + 8;
    pa->pV->QpBdOffsetC         = s->bit_depth_chroma_minus8 * 6;
    pa->pV->MaxFrameNum         = (uint32_t)pow(2, s->log2_max_frame_num_minus4 + 4);
    pa->pV->PicWidthInMbs       = s->pic_width_in_mbs_minus1 + 1;
    pa->pV->PicHeightInMapUnits = s->pic_height_in_map_units_minus1 + 1;
    pa->pV->PicHeightInMbs      = s->pic_height_in_map_units_minus1 + 1;
    pa->pV->FrameHeightInMbs    = ( 2 - s->frame_mbs_only_flag ) * pa->pV->PicHeightInMapUnits;
    pa->pV->PicSizeInMapUnits   = pa->pV->PicWidthInMbs *pa->pV->PicHeightInMapUnits;
    
    pa->pV->SubWidthC           = (s->chroma_format_idc == 1 || s->chroma_format_idc == 2) ? 2 : 1;
    pa->pV->SubHeightC          = (s->chroma_format_idc == 1) ? 2 : 1;
}
void nal::decode_PPS()
{
    PPS *p = new PPS(pa);
    this->data = p;
    p->decode(); // 先解码，
    // 再更新
    pa->update_ps(p);// 把解码参数挂上去
}

void nal::decode_PIC()
{
    // 对于普通PIC来说需要控制一下内存，
    // 丢弃掉那些不需要的缓冲帧
    de->ctrl_Memory();
    
    // 图像解码不仅需要参数集，
    // 还需要参考帧控制、内存控制参数等，
    // 所以还需要一个decoder对象
    picture* pic = new picture(pa, de);
    this->data = pic;
    
    pic->deocde();


    //解码完毕需要对当前的pic进行标记，
    //在下一次解码的时候会初始化，重排序和修改，而不是这里
    // nal_ref_idc会在这里用到
    pic->refidc(nal_ref_idc);
}

void nal::decode_IDR()
{
    // 对于IDR图像来说
    // 这里首先有一步IDR控制
    // 由于遇到IDR，缓存空间是必定全部清除的
    // 所以这里不需要调用控制内存函数
    de->clear_DecodedPic();

    // 然后和普通的图像解码方式是一样的
    picture* pic = new picture(pa, de);
    this->data = pic;
    // 不过多一步设置IDR标志
    pic->IdrPicFlag = true;

    pic->deocde();

    pic->refidc(nal_ref_idc);
}

void nal::decode_SEI()
{
    sei* se = new sei(pa);

    se->decode();

    // this->data = se;
    delete se;
}