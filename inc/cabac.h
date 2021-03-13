#ifndef CABAC_H__
#define CABAC_H__

#include "gtype.h"
#include "array2d.h"

class cabac
{
private:

    uint8 b3, b1;
    class slice* lifeTimeSlice;
    uint8 ctxIdxOfInitVariables[1024][2];
    uint8 state;// 0 - sleep,   1 - active;
    class parser* pa;
    class picture* pic;
    uint16 codIRange;
    uint16 codIOffset;
    uint8  bypassFlag;
    uint16 ctxIdx;
    uint16 ctxIdxInc;
    uint16 ctxIdxOffset;

    // 下面的三个方法只可能解码出来两种状态
    // 所以用了 bool

    bool decode_binary(uint16 ctx);
    bool decode_bypass();
    bool decode_finaly();
    int decode_unary(uint16 ctx, int offset);
    // UEGK 都是采用旁路解码
    // 这个函数不包括符号位和前缀的TU串
    int decode_uegk(int k);
    int decode_trunc();

    uint8 init_variable();
    uint8 init_engine();
    bool DecodeValueUsingCtxIdx(uint16 ctxIdx_value, uint8 bypassFlag_value);
    void RenormD();

    uint16 read_intra_chroma_pred_mod();
    uint8  read_prev_intra4x4_pred_mode_flag();
    uint8  read_rem_intra4x4_pred_mode();

    uint8  read_mb_skip_flag();

    uint16 read_mb_type();
    //子宏块预测中的值
    uint8  read_sub_mb_type();
    uint8  read_ref_idx(int);
    int    read_mvd_lx(int);

    uint16 read_coded_block_pattern();
    uint8  read_transform_8x8_size_flag();
    int8   read_mb_qp_delta();
    uint8  read_coded_block_flag(int);
    uint8  read_significant_coeff_flag(int);
    uint8  read_last_significant_coeff_flag(int);
    uint8  read_coeff_sign_flag(int);
    uint32 read_coeff_abs_level_minus1(int);

    uint16 pStateIdx;
    uint16 valMPS   ;
    uint32 Decode(uint32);
public:
    uint8 get_state(){return state;};
    int   cread_ae(uint32);
    bool  slice_new(picture *pic, slice *sl);
    bool  slice_end();
    bool  set_pic  (picture* p);
    bool  set_slice(slice* sl);
    cabac(parser* p);
    ~cabac();
};

#endif