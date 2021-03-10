#include "pps.h"
#include "parser.h"
PPS::PPS(parser* p)
{
    this->pa = p;
    
    pic_parameter_set_id                            = 0;//
    seq_parameter_set_id                            = 0;//
    entropy_coding_mode_flag                        = 0;//
    bottom_field_pic_order_in_frame_present_flag    = 0;//
    num_slice_groups_minus1                         = 0;//
    slice_group_map_type                            = 0;//
    run_length_minus1                               = NULL;//
    top_left                                        = NULL;//
    bottom_right                                    = NULL;//
    slice_group_change_direction_flag               = 0;//
    slice_group_change_rate_minus1                  = 0;//
    pic_size_in_map_units_minus1                    = 0;//
    slice_group_id                                  = NULL;//
    num_ref_idx_l0_default_active_minus1            = 0;//
    num_ref_idx_l1_default_active_minus1            = 0;//
    weighted_pred_flag                              = 0;//
    weighted_bipred_idc                             = 0;//
    pic_init_qp_minus26                             = 0;//
    pic_init_qs_minus26                             = 0;//
    chroma_qp_index_offset                          = 0;//
    deblocking_fliter_control_present_flag          = 0;//
    constrained_intra_pred_flag                     = 0;//
    redundant_pic_cnt_present_flag                  = 0;//
 
    transform_8x8_mode_flag                         = 0;//
}
PPS::~PPS()
{

    if(run_length_minus1) delete[] run_length_minus1;
    if(top_left) delete[] top_left;
    if(bottom_right) delete[] bottom_right;
    if(slice_group_id); delete[] slice_group_id;
}

void PPS::decode()
{
    
    pic_parameter_set_id                                   = pa->read_ue();//
    seq_parameter_set_id                                   = pa->read_ue();//
    entropy_coding_mode_flag                               = pa->read_un(1);//
    bottom_field_pic_order_in_frame_present_flag           = pa->read_un(1);
    num_slice_groups_minus1                                = pa->read_ue();//
    // 以下是用来处理一个pic有多个slice时用的函数，这里就简化了，
    // 因为一般一个pic只有一个slice，也就不存在所谓的多个slice的问题了。
    if(num_slice_groups_minus1 > 0)
    {
        slice_group_map_type                               = pa->read_ue();//
        if(slice_group_map_type == 0)
        {
            
            if(run_length_minus1) delete[] run_length_minus1;
            run_length_minus1 = new uint32[num_slice_groups_minus1];
            for (size_t iGroup = 0; iGroup < num_slice_groups_minus1; iGroup++)
            {
                run_length_minus1[iGroup]                  = pa->read_ue();//
            }
        }
        else if(slice_group_map_type ==2)
        {
            if(top_left) delete[] top_left;
            if(bottom_right) delete[] bottom_right;

            top_left     = new uint32[num_slice_groups_minus1];
            bottom_right = new uint32[num_slice_groups_minus1];
            for(size_t iGroup=0;iGroup < num_slice_groups_minus1;iGroup++)
            {
                top_left[iGroup]                           = pa->read_ue();//
                bottom_right[iGroup]                       = pa->read_ue();//
            }
        }
        else if(slice_group_map_type==3||slice_group_map_type==4||slice_group_map_type==5)
        {
            slice_group_change_direction_flag              = pa->read_un(1);//
            slice_group_change_rate_minus1                 = pa->read_ue();//
        }
        else if(slice_group_map_type == 6)
        {
            pic_size_in_map_units_minus1                   = pa->read_ue();//
            if(slice_group_id); delete[] slice_group_id;
            slice_group_id = new uint32[pic_size_in_map_units_minus1];
            for (size_t i = 0; i < pic_size_in_map_units_minus1 ; i++)
            {
                slice_group_id[i]                          = pa->read_un(4);//
            }
        }
    }
    num_ref_idx_l0_default_active_minus1                   = pa->read_ue();//
    num_ref_idx_l1_default_active_minus1                   = pa->read_ue();//
    weighted_pred_flag                                     = pa->read_un(1);//
    weighted_bipred_idc                                    = pa->read_un(2);//
    pic_init_qp_minus26                                    = pa->read_se();//
    pic_init_qs_minus26                                    = pa->read_se();//
    chroma_qp_index_offset                                 = pa->read_se();//
    deblocking_fliter_control_present_flag                 = pa->read_un(1);//
    constrained_intra_pred_flag                            = pa->read_un(1);//
    redundant_pic_cnt_present_flag                         = pa->read_un(1);//
    if(0)
    {
        transform_8x8_mode_flag                            = pa->read_un(1);//
    }

}