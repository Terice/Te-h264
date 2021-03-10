#include "sps.h"
#include "parser.h"

SPS::SPS(parser* p)
{
    this->pa = p;

    profile_idc                          = 0;//
    constraint_set0_flag                 = 0;//
    constraint_set1_flag                 = 0;//
    constraint_set2_flag                 = 0;//
    constraint_set3_flag                 = 0;//
    constraint_set4_flag                 = 0;//
    constraint_set5_flag                 = 0;//
    reserved_zero_2bits                  = 0;//
    level_idc                            = 0;//
    seq_parameter_set_id                 = 0;//
    chroma_format_idc                    = 1;//
    separate_colour_plane_flag           = 0;//
    bit_depth_luma_minus8                = 0;//
    bit_depth_chroma_minus8              = 0;//
    qpprime_y_zero_transform_bypass_flag = 0;//
    seq_scaling_matrix_present           = 0;//
    seq_scaling_list_present_flag        = 0;//
    log2_max_frame_num_minus4            = 0;//
    pic_order_cnt_type                   = 0;//
    log2_max_pic_order_cnt_lsb_minus4    = 0;//
    delta_pic_order_always_zero_flag     = 0;//
    offset_for_non_ref_pic               = 0;//
    ffset_for_top_to_bottom_field        = 0;//
    num_ref_frames_in_pic_order_cnt_cycle= 0;//
    offset_for_ref_frame                 = NULL;//
    max_num_ref_frames                   = 0;//
    gaps_in_frame_num_value_allowe_flag  = 0;//
    pic_width_in_mbs_minus1              = 0;//
    pic_height_in_map_units_minus1       = 0;//
    frame_mbs_only_flag                  = 0;//
    mb_adaptive_frame_field_flag         = 0;//
    direct_8x8_inference_flag            = 0;//
    frame_cropping_flag                  = 0;//
    frame_crop_left_offset               = 0;//
    frame_crop_right_offset              = 0;//
    frame_crop_top_offset                = 0;//
    frame_crop_bottom_offset             = 0;//
    vui_parameters_present_flag          = 0;//
}
SPS::~SPS()
{
    if(offset_for_ref_frame) delete[] offset_for_ref_frame;
}


void SPS::decode()
{
        
    profile_idc                             = pa->read_un(8);//
    constraint_set0_flag                    = pa->read_un(1);//
    constraint_set1_flag                    = pa->read_un(1);//
    constraint_set2_flag                    = pa->read_un(1);//
    constraint_set3_flag                    = pa->read_un(1);//
    constraint_set4_flag                    = pa->read_un(1);//
    constraint_set5_flag                    = pa->read_un(1);//
    reserved_zero_2bits                     = pa->read_un(2);//
    level_idc                               = pa->read_un(8);//
    seq_parameter_set_id                    = pa->read_ue();//
    if(profile_idc==100||profile_idc==110||profile_idc==122\
     ||profile_idc==244||profile_idc==44 ||profile_idc==83\
     ||profile_idc==86 ||profile_idc==118||profile_idc==128\
     ||profile_idc==138||profile_idc==139||profile_idc==134)
    {
        chroma_format_idc                   = pa->read_ue();//
        separate_colour_plane_flag          = pa->read_ue();//
        bit_depth_luma_minus8               = pa->read_ue();//
        bit_depth_chroma_minus8             = pa->read_un(1);//
        qpprime_y_zero_transform_bypass_flag= pa->read_un(1);//
        seq_scaling_matrix_present          = pa->read_un(1);//
        seq_scaling_list_present_flag       = 0;//
    }
    log2_max_frame_num_minus4               = pa->read_ue();//
    pic_order_cnt_type                      = pa->read_ue();//
    if(pic_order_cnt_type == 0) 
    {
        log2_max_pic_order_cnt_lsb_minus4   = pa->read_ue();//
    }
    else if(pic_order_cnt_type == 1)
    {
        delta_pic_order_always_zero_flag      = pa->read_un(1);//
        offset_for_non_ref_pic                = pa->read_se();//
        ffset_for_top_to_bottom_field         = pa->read_se();//
        num_ref_frames_in_pic_order_cnt_cycle = pa->read_ue();//
        
        if(offset_for_ref_frame) delete[] offset_for_ref_frame;
        offset_for_ref_frame = new uint32[num_ref_frames_in_pic_order_cnt_cycle];
        for (size_t i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
        {
            offset_for_ref_frame[i]         = pa->read_ue();//
        }
        
    }
    max_num_ref_frames                      = pa->read_ue();//
    gaps_in_frame_num_value_allowe_flag     = pa->read_un(1);//
    pic_width_in_mbs_minus1                 = pa->read_ue();//
    pic_height_in_map_units_minus1          = pa->read_ue();//
    frame_mbs_only_flag                     = pa->read_un(1);//
    if(!frame_mbs_only_flag) 
    {
        mb_adaptive_frame_field_flag        = pa->read_un(1);//
    }
    direct_8x8_inference_flag               = pa->read_un(1);//
    frame_cropping_flag                     = pa->read_un(1);//
    if(frame_cropping_flag)
    {
        frame_crop_left_offset              = pa->read_ue();//
        frame_crop_right_offset             = pa->read_ue();//
        frame_crop_top_offset               = pa->read_ue();//
        frame_crop_bottom_offset            = pa->read_ue();//
    }
    vui_parameters_present_flag             = pa->read_un(1);//
}