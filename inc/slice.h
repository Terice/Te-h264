#ifndef SLICE_H__
#define SLICE_H__

#define SLICE_TYPE_P  0
#define SLICE_TYPE_B  1
#define SLICE_TYPE_I  2
#define SLICE_TYPE_SP 3
#define SLICE_TYPE_SI 4

#include "gtype.h"
#include "array2d.h"
#include "genum.h"

class SPS;
class PPS;
class parser;
class decoder;

class picture;
class macroblock;

class nal;

typedef struct ParameterSets__
{
    
    SPS* sps;
    PPS* pps;
    uint32 first_mb_in_slice                    ;
    uint32 slice_type                           ;
    uint32 pic_parameter_set_id                 ;
    uint16 colour_plane_id                      ;
    uint32 frame_num                            ;
    uint16 field_pic_flag                    = 0;
    uint16 bottom_field_flag                    ;
    uint32 idr_pic_id                           ;
    uint16 pic_order_cnt_lsb                    ;
    int32  delta_pic_order_cnt_bottom           ;
    int32  delta_pic_order_cnt[2]               ;
    uint16 redundant_pic_cnt                    ;
    uint16 direct_spatial_mv_pred_flag          ;
    uint16 num_ref_idx_active_override_flag;    ;
    uint32 num_ref_idx_l0_active_minus1      = 0;
    uint32 num_ref_idx_l1_active_minus1      = 0;
    uint32 cabac_init_idc                       ;
    int32  slice_qp_delta                       ;
    uint16 sp_for_switch_flag                   ;
    uint32 slice_qs_delta                       ;
    uint32 disable_deblocking_fliter_idc     = 0;
    int32  slice_alpha_c0_offset_dic2           ;
    int32  slice_beta_offset_div2               ;
    uint32 slice_group_change_cycle             ;

    uint8 ref_pic_list_modification_flag_l0     ;
    uint8 ref_pic_list_modification_flag_l1     ;
    uint32 modification_of_pic_nums_idc         ;
    uint32 abs_diff_pic_num_minus1              ;

    uint8  no_output_of_prior_pics_flag         ;//
    uint8  long_term_reference_flag             ;//
    uint8  adaptive_ref_pic_marking_mode_flag   ;//
    uint32 memory_management_control_operation  ;//
    uint32 difference_of_pic_nums_minus1        ;//
    uint32 long_term_pic_num                    ;//
    uint32 long_term_frame_idx                  ;//
    uint32 max_long_term_frame_idx_plus1        ;//


    uint32 MbaffFrameFlag                    = 0;//
    uint32 MaxPicOrderCntLsb                 = 0;
    uint32 MaxPicNum                         = 0;//
    uint32 CurrPicNum                        = 0;//
    uint8  SliceQPY                          = 0;


    //slice data
    uint32 mb_skip_run                       = 0;//
    uint32 mb_skip_flag                      = 0;//
    uint32 mb_field_decoding_flag            = 0;//
    uint32 end_of_slice_flag                 = 0;//
    
}ParameterSets;
typedef struct PREDWEIGHT
{
    uint8 luma_log2_weight_denom             = 0;
    uint8 chroma_log2_weight_denom           = 0;
    uint8 luma_weight_l0_flag                = 0;
    int16 *luma_weight_l0                    = NULL;
    int16 *luma_offset_l0                    = NULL;
    uint8 chroma_weight_l0_flag              = 0;
    int16 **chroma_weight_l0                 = NULL;
    int16 **chroma_offset_l0                 = NULL;
    uint8 luma_weight_l1_flag                = 0;
    int16 *luma_weight_l1                    = NULL;
    int16 *luma_offset_l1                    = NULL;
    uint8 chroma_weight_l1_flag              = 0;
    int16 **chroma_weight_l1                 = NULL;
    int16 **chroma_offset_l1                 = NULL;

    int logWD_C                                = 0;
    int W_0c                                   = 0;
    int W_1C                                   = 0;
    int O_0C                                   = 0;
    int O_1c                                   = 0;
}PredWeight;

class slice
{
private:

    slice* next;
    parser*  pa;
    decoder* de;
    picture *pic;
    // 需要解码的每一个宏块都在此初始化，交回pic
    array2d<macroblock*> *mb_map;

    void Calc_POC();

public:
    slice(parser* p, decoder* d, picture* pic);
    ~slice();

    nal* curNAL;

    ParameterSets* ps;//contain SPS and PPS
    PredWeight* pw;

    int slice_type;
    type_slice type;
    int index;

    int last_mb_qp_delta;
    
    macroblock* curMB;
    macroblock* hedMB;

    // 其实就是把解码头部和解码数据部整合了一下
    // 分别调用另外3个函数也是可以的
    void decode();

    void ParseSliceHead();
    void ConsRefList();
    void ParseSliceData();
};










#endif