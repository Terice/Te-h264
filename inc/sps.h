#ifndef SPS_H__
#define SPS_H__
#include "gtype.h"
#include <stdio.h>


class parser;


class SPS
{
private:
    parser* pa;
public:
    uint32  profile_idc                          ;
    uint32  constraint_set0_flag                 ;
    uint32  constraint_set1_flag                 ;
    uint32  constraint_set2_flag                 ;
    uint32  constraint_set3_flag                 ;
    uint32  constraint_set4_flag                 ;
    uint32  constraint_set5_flag                 ;
    uint32  reserved_zero_2bits                  ;
    uint32  level_idc                            ;
    uint32  seq_parameter_set_id                 ;
    uint32  chroma_format_idc                    ;
    uint32  separate_colour_plane_flag           ;
    uint32  bit_depth_luma_minus8                ;
    uint32  bit_depth_chroma_minus8              ;
    uint32  qpprime_y_zero_transform_bypass_flag ;
    uint32  seq_scaling_matrix_present           ;
    uint32  seq_scaling_list_present_flag        ;
    uint32  log2_max_frame_num_minus4            ;
    uint32  pic_order_cnt_type                   ;
    uint32  log2_max_pic_order_cnt_lsb_minus4    ;
    uint32  delta_pic_order_always_zero_flag     ;
    uint32  offset_for_non_ref_pic               ;
    uint32  ffset_for_top_to_bottom_field        ;
    uint32  num_ref_frames_in_pic_order_cnt_cycle;
    uint32* offset_for_ref_frame                 ;
    uint32  max_num_ref_frames                   ;
    uint32  gaps_in_frame_num_value_allowe_flag  ;
    uint32  pic_width_in_mbs_minus1              ;
    uint32  pic_height_in_map_units_minus1       ;
    uint32  frame_mbs_only_flag                  ;
    uint32  mb_adaptive_frame_field_flag         ;
    uint32  direct_8x8_inference_flag            ;
    uint32  frame_cropping_flag                  ;
    uint32  frame_crop_left_offset               ;
    uint32  frame_crop_right_offset              ;
    uint32  frame_crop_top_offset                ;
    uint32  frame_crop_bottom_offset             ;
    uint32  vui_parameters_present_flag          ;
    

    void decode();
    SPS(parser*);
    ~SPS();
};


#endif