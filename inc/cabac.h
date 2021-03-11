#ifndef CABAC_H__
#define CABAC_H__

#include "gtype.h"
#include "array2d.h"

class cabac
{
private:

    uint8 b3, b1;
    class slice* lifeTimeSlice;
    array2d<uint8_t>* ctxIdxOfInitVariables;
    uint8 state;// 0 - sleep,   1 - active;
    class parser* p;
    class picture* pic;
    uint16 codIRange;
    uint16 codIOffset;
    uint8  bypassFlag;
    uint16 ctxIdx;
    uint16 ctxIdxInc;
    uint16 ctxIdxOffset;

    uint8 decode_binary(uint16 ctx);
    uint8 decode_bypass();
    uint8 decode_finaly();

    uint8 init_variable();
    uint8 init_engine();
    uint16 DecodeCtxIdxUsingBinIdx(uint16 binIdx, uint16 maxBinIdxCtx, int ctxIdxOffset, int syntaxRequest);
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

    // 二值化的方法
    // 基于查表的方法
    bool Binarization_mbtype_submbtype(uint16 &maxBinIdxCtx, int &ctxIdxOffset, uint8 &bypassFlag);
    // 一元二值化
    int IsIn_U_binarization(uint64 value, uint8 length);    //if is, return value, if not, return -1
    // 截断一元二值化
    int IsIn_TU_binarization(uint32 value, uint8 cMax, uint8 length);//if is, return value, if not, return -1
    // k阶指数哥伦布二值化
    //判断是不是在UEGk二值串中，如果是，返回这个二值串的值，否则返回-1
    //五个参数分别是：当前的值，当前的长度，signedValFlag uCoeff k由句法表得到
    int IsIn_UEGk_binarization(uint32 value, uint8 length, uint8  signedValFlag, uint8 uCoeff, uint8 k);

    uint16 pStateIdx;
    uint16 valMPS   ;
    uint32 Decode(uint32);
public:
    uint8 get_state(){return state;};
    int   cread_ae(uint32);
    bool  slice_end();
    bool  set_pic  (picture* p);
    bool  set_slice(slice* sl);
    cabac(parser* parser);
    ~cabac();
};

#endif