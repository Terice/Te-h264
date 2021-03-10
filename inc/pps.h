#ifndef PPS_H__
#define PPS_H__
#include "gtype.h"

class parser;


class PPS
{
private:
    parser* pa;
public:
    uint32  pic_parameter_set_id                            ;//
    uint32  seq_parameter_set_id                            ;//
    uint32  entropy_coding_mode_flag                        ;//
    uint16  bottom_field_pic_order_in_frame_present_flag    ;//
    uint32  num_slice_groups_minus1                         ;//
    uint32  slice_group_map_type                            ;//
    uint32* run_length_minus1                               ;//
    uint32* top_left                                        ;//
    uint32* bottom_right                                    ;//
    uint32  slice_group_change_direction_flag               ;//
    uint32  slice_group_change_rate_minus1                  ;//
    uint32  pic_size_in_map_units_minus1                    ;//
    uint32* slice_group_id                                  ;//
    uint32  num_ref_idx_l0_default_active_minus1            ;//
    uint32  num_ref_idx_l1_default_active_minus1            ;//
    uint32  weighted_pred_flag                              ;//
    uint32  weighted_bipred_idc                             ;//
    int32   pic_init_qp_minus26                             ;//
    int32   pic_init_qs_minus26                             ;//
    int32   chroma_qp_index_offset                          ;//
    uint32  deblocking_fliter_control_present_flag          ;//
    uint32  constrained_intra_pred_flag                     ;//
    uint32  redundant_pic_cnt_present_flag                  ;//
 
    uint8   transform_8x8_mode_flag                         ;//

    void decode();
    PPS(parser*);
    ~PPS();
};
#endif

