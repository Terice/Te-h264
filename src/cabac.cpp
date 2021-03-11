#include "cabac.h"

#include "genum.h"
#include "gchart.h"
#include "gvars.h"
#include "gfunc.h"

#include "terror.h"
#include "nal.h"
#include "picture.h"
#include <cmath>
#include "macroblock.h"
#include "block.h"
#include "residual.h"



#include "array2d.h"
#include "macroblock.h"
#include "slice.h"
#include "parser.h"
#include <iostream>
#include "slice.h"




int cabac::cread_ae(uint32 syntax)
{
    //result意思是计算出来的值
    uint32 result;
    //初始化
    if(state == 0)
    {
        init_variable();
        init_engine();
        state = 1;
        if(terr.cabac_state_running()) printf(">>cabac: INIT!!!!!!\n");
    }
    result = Decode(syntax);
    //后处理
    if(syntax == 0x00022000U && (type_macroblock)result == I_PCM) {init_engine();}

    int a = syntax >> 12 & 0xFF;
    if(terr.cabac_result_ae())
    {
        // if((a >= 0x61 && a <= 0x64 ) || (a == 0x41 || a == 0x43))
        printf(">>cabac: result of |%8x| is : |%5d|\n", syntax, result);
        // else
        // printf(">>cabac: result of |%8d| is : |%5d|\n", syntax, result);
    }

    return result;
}
//syntaxelements:
//
uint32 cabac::Decode(uint32 syntaxelements)
{ 
    uint32 result = 0;
    uint32 syntax;
    syntax = ((syntaxelements & 0x000FF000U)>> 12);

    if     (syntax == 0x00U) return 0;
    else if(syntax == 0x11U) result = read_mb_skip_flag();
    else if(syntax == 0x21U) result = read_mb_type();
    else if(syntax == 0x22U) result = read_transform_8x8_size_flag();
    else if(syntax == 0x23U) result = read_coded_block_pattern();
    else if(syntax == 0x25U) result = read_mb_qp_delta();
    else if(syntax == 0x31U) result = read_prev_intra4x4_pred_mode_flag();
    else if(syntax == 0x32U) result = read_rem_intra4x4_pred_mode();
    else if(syntax == 0x35U) result = read_intra_chroma_pred_mod();
    else if(syntax == 0x51U) result = read_sub_mb_type();
    else if(syntax == 0x41U) result = read_ref_idx                     (syntaxelements);
    else if(syntax == 0x43U) result = read_mvd_lx                      (syntaxelements);
    else if(syntax == 0x61U) result = read_coded_block_flag            (syntaxelements);
    else if(syntax == 0x62U) result = read_significant_coeff_flag      (syntaxelements);
    else if(syntax == 0x63U) result = read_last_significant_coeff_flag (syntaxelements);
    else if(syntax == 0x64U) result = read_coeff_abs_level_minus1      (syntaxelements);
    else if(syntax == 0x65U) result = DecodeValueUsingCtxIdx           (0, 1);
    else if(syntax == 0x76U) result = DecodeValueUsingCtxIdx           (276, 0);
    else ;

    return result;
}
bool cabac::DecodeValueUsingCtxIdx(uint16 ctxIdx_value, uint8 bypassFlag_value)
{
    pStateIdx = (*ctxIdxOfInitVariables)[ctxIdx_value][0];
    valMPS    = (*ctxIdxOfInitVariables)[ctxIdx_value][1];
    bool binVal;
    if(bypassFlag_value)//decode bypass
    {
        codIOffset = codIOffset << 1;
        codIOffset = codIOffset | p->read_bi();
        if(codIOffset >= codIRange) {binVal = 1; codIOffset = codIOffset - codIRange;}
        else binVal = 0;
        if(terr.cabac_state_running())     
        {
            printf(">>cabac: bypass decode\n");
            printf("         codIOffset: %5d\t", codIOffset);
            printf("          -------------------result : %d\n", binVal);
        }
    }
    else 
    {
        if(ctxIdx_value == 276)//decode terminate
        {
            codIRange = codIRange - 2;
            if(codIOffset >= codIRange) binVal = 1;
            else {binVal = 0;RenormD();}
            // if(terr.cabac_state_running())     
            // {
            //     printf(">>cabac: terminate decode\n");
            //     printf("         codIOffset: %5d\t", codIOffset);
            //     printf("          -------------------result : %d\n", binVal);
            // }
        }
        else//decode decision
        {
            uint16 qCodIRangeIdx = (codIRange >> 6) & 3; // codIRange是一个9位的值，去掉低6位，高3位用来计算小区间的长度。
            uint16 codIRangeLPS = rangeTabLPS[pStateIdx][qCodIRangeIdx];
            if(terr.cabac_state_running())     
            {
                printf("--cabac: Running State: ");
                printf("ctxIdx: %4d  ", ctxIdx_value);
                printf("codIOffset: %5d\t", codIOffset);
                printf("state: %5d\t", pStateIdx);
                printf("codIRang: %5d\t", codIRange);
                printf("codIRangeLPS: %5d\n", rangeTabLPS[pStateIdx][qCodIRangeIdx]);
                // printf("         state(%3d)----->", pStateIdx);
                // if(codIOffset >= codIRange)
                // printf("MPS(%3d)\n", transIdxMPS[pStateIdx]);
                // else
                // printf("LPS(%3d)\n", transIdxLPS[pStateIdx]);

            }
            codIRange = codIRange - codIRangeLPS;// 得到小概率区间的起始值， 也就是得到当前大小概率区间的分界值
            // 解码区间[[大概率区间],[小概率区间]]
            if(codIOffset >= codIRange) // 当前的数值落在小区间内（大于大概率符号区间的长度），说明是LPS
            {
                binVal = !(bool)valMPS  ; // 大概率字符取反就是小概率字符
                codIOffset -= codIRange ; // 当前数值减去小区间的起始值，得到在小区间中的数值

                codIRange = codIRangeLPS; // 同时把解码全区间（注意是大小一起的区间）设置为下一个小区间的范围，因为下一次需要在这个区间运算
                // 注意大概率区间是在这个if之前那一步运算的，所以这里计算全区间就可以了。
                
                // 如果当前的概率索引是0，也就是说状态不断转换的过程中小概率字符出现的几率已经到了0.5,而这个if是用来处理小概率字符的
                // 也就是说现在小概率字符几率到了0.5,并且马上要处理的这个字符也是小概率字符，那么小概率字符就会变成大概率字符，所以有了下一步
                if(pStateIdx == 0) {(*ctxIdxOfInitVariables)[ctxIdx_value][1] = 1 - valMPS;};
                (*ctxIdxOfInitVariables)[ctxIdx_value][0] = transIdxLPS[pStateIdx];// 状态往小概率转
            }
            else // 否则说明当前是大概率字符，取出valMPS，并且当前ctxIdx的状态往右边走一步（大概率方向）
            {
                // 大概率字符的区间始终都是从0开始的，所以不需要进行区间的变换，
                // 下一次直接把小区间的长度减去就得到整个解码区间了
                binVal = (bool)valMPS;
                (*ctxIdxOfInitVariables)[ctxIdx_value][0] = transIdxMPS[pStateIdx];
            }
            // 这里无论是if还是else 最后得到的codIRange的值都一定是代表全区间
            RenormD();
        }
    }
    if(terr.cabac_result_bin()) printf(">>cabac: cur bin: %d\n", binVal);
    return binVal;
}
void cabac::RenormD()
{
    // 归一化把编码区间的放大到9位
    // 同时读取offset，来确定当前编码值所在的位置
    while(codIRange < 256)
    {
        codIRange  <<= 1;
        codIOffset <<= 1;
        codIOffset |= (uint8_t)(p->read_bi());
    }
}
//上下文变量的初始化
uint8 cabac::init_variable()
{
    uint8 ctxRangeCol = 0, mncol = 0;
    uint8 preCtxState = 0;
    int m = 0, n = 0;
    ctxIdxOfInitVariables = new array2d<uint8_t>(1024, 2, 0);
    
    type_slice lifeTimetype_slice = lifeTimeSlice->type;
    if(lifeTimetype_slice == I) {mncol = 0;}
    else {mncol = lifeTimeSlice->ps->cabac_init_idc + 1;}
 
    pStateIdx = 63; valMPS = 0;
    //每个上下文模型是由大概率字符valMPS(Most Probable Symbol，MPS)和概率状态索引(pStateldx)两个部分组成
    (*ctxIdxOfInitVariables)[276][0] = pStateIdx;
    (*ctxIdxOfInitVariables)[276][1] = valMPS   ;

    // 句法元素表一共有43行，要一一对应初始化
    // 这里的43不是指句法元素的数量，也不是ctxIdx的数量
    // 因为一个句法元素可能有多组ctxIdx，而ctxIdx是1024个，所以这里的ctxIdx的变量值是1024行
    for (int i = 0; i < 43; i++)     //i is ctx  Idx range of current syntax element, init all 43 rows
    {
        // ctxRangeCol确定这一slice对应的是哪一行元素
        if(lifeTimetype_slice == SI) {ctxRangeCol = 0;}
        else if(lifeTimetype_slice == I){ctxRangeCol = 1;}
        else if(lifeTimetype_slice == P || lifeTimetype_slice == SP) {ctxRangeCol = 2;}
        else {ctxRangeCol = 3;}
        // 从每一个句法元素的下限开始，一直初始化到上限
        for (int j = ctxIdxRangOfSyntaxElements[i][ctxRangeCol][0]; j <= ctxIdxRangOfSyntaxElements[i][ctxRangeCol][1]; j++)
        {   //j is ctxIdx
            // m和n代表了两个变量值，这个值是用来初始化ctxIdx的变量（也就是pState和MPS）用的
            m = mnValToCtxIdex[j][mncol][0];
            n = mnValToCtxIdex[j][mncol][1];

            // 上下限都是0说明是na或者udf，此时不需要管（因为不会用到）
            if(!(ctxIdxRangOfSyntaxElements[i][ctxRangeCol][0] == 0 && ctxIdxRangOfSyntaxElements[i][ctxRangeCol][1] == 0))
            {
                preCtxState = (uint8_t)Clip3(\
                    1,\
                    126,\
                    ((m *  Clip3(0, 51, (int)lifeTimeSlice->ps->SliceQPY)) >> 4) + n\
                );
                if(preCtxState <= 63)
                { 
                    pStateIdx = 63 - preCtxState;
                    valMPS = 0;
                }
                else 
                {
                    pStateIdx = preCtxState - 64;
                    valMPS = 1;
                }
                (*ctxIdxOfInitVariables)[j][0] = pStateIdx;
                (*ctxIdxOfInitVariables)[j][1] = valMPS   ;
            }
        }
    }
    return 1;
}
uint8 cabac::init_engine()
{
    codIRange = 510;
    codIOffset = p->read_un(9);
    return 1;
}

// 是否是mbtype的二值化串
// 返回的是值的索引，
int InBinarization(uint16 result, int binIdx, const uint8 binarization_chart[][2])
{
    int re = -1;
    uint8 chart_length = 0;

    if(binarization_chart == binarization_mbtype_in_I) chart_length = 26;
    else if(binarization_chart == binarization_mbtype_in_PandSP) chart_length = 6;
    else /*mbtype B*/chart_length = 24;

    for(int i = 0; i < chart_length; i++)
        if(result == binarization_chart[i][0] && binarization_chart[i][1] == binIdx + 1){re = i; break;}
    return re;
}
// 是不是submbtype的二值化串
int InBinarizationSub(uint16 result, int binIdx, const uint8 binarization_chart[][2])
{
    int re = -1;
    uint8 chart_length = 0;

    if(binarization_chart == binarization_submbtype_in_PandSP) chart_length = 4;
    else /*if(binarization_chart == binarization_submbtype_in_B)*/ chart_length = 13;

    for(int i = 0; i < chart_length; i++)
        if(result == binarization_chart[i][0] && binarization_chart[i][1] == binIdx + 1){re = i; break;}
    return re;
}


bool cabac::Binarization_mbtype_submbtype(uint16 &maxBinIdxCtx, int &ctxIdxOffset, uint8 &bypassFlag)
{
    if(lifeTimeSlice->type == I){maxBinIdxCtx = 6; ctxIdxOffset = 3; bypassFlag = 0;return true;}
    else if(lifeTimeSlice->type == SI){return true;}
    else return false;
}
uint16 cabac::read_mb_type()
{
    uint16 result = 0;

    uint16 ctxIdx_cur = 0;
    uint16 result_cur = 0;
    uint16 ctxIdxInc = 0;

    type_slice slice_type = lifeTimeSlice->type;
    macroblock* cur = lifeTimeSlice->curMB;
    picture* p = pic;
    uint8 ctxIdxOffset = 0;

    auto f = [p](macroblock* cur, char direction, uint16 ctxIdxOffset)->uint8_t{
        macroblock* N = p->neighbour_macroblock(cur, direction);
        uint8 condTermFlagN = 0;
        if(!N || !cur->is_avaiable(N)) condTermFlagN = 0;
        else
        {
            if( (ctxIdxOffset == 0 && N->type == SI_M) ||\
                (ctxIdxOffset == 3 && N->type == I_NxN)||\
                (ctxIdxOffset == 27 && (N->type == B_Skip|| N->type == B_Direct_16x16))
            ) condTermFlagN  = 0;
            else condTermFlagN  = 1;
        }
        return condTermFlagN;
    };
    if(slice_type == I)
    {
        // ctxIdxOffset = 3;
        // int binIdx = -1;
        // do
        // {
        //     binIdx++;// sumi

        //     if(binIdx == 0) ctxIdxInc = f(cur, 'A', ctxIdxOffset) + f(cur, 'B', ctxIdxOffset);
        //     else if(binIdx == 1) {ctxIdx = 276;}
        //     else if(binIdx == 2 || binIdx == 3) ctxIdxInc = binIdx + 1;
        //     else if(binIdx == 4) ctxIdxInc = (b3 != 0) ? 5 : 6;
        //     else if(binIdx == 5) ctxIdxInc = (b3 != 0) ? 6 : 7;
        //     else ctxIdxInc = 7;
        //     if(binIdx != 1) ctxIdx = ctxIdxInc + ctxIdxOffset;
        //     result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        //     if(binIdx == 3) b3 = result_cur;
        //     result_cur <<= binIdx;
        //     result += result_cur;
        // } while((InBinarization(result, binIdx, binarization_mbtype_in_I)) == -1);
        // result = InBinarization(result, binIdx, binarization_mbtype_in_I);

        // return result;

        // I 帧， ctxIdxOffset = 3
        // I 帧的上下文索引 [3,10]
        int condTermFlagA, condTermFlagB;
        int binIdx; int predmode;

        ctxIdxOffset = 3;
        ctxIdx = condTermFlagA + condTermFlagB + ctxIdxOffset;
        if(decode_binary(ctxIdx))// 解第 0 位 看是不是I_NxN
            return 0;
        else
        {
            if(decode_finaly()) // 第1位， 是不是 I_PCM
                return 25;//I_PCM
            else // Intra_16x16
            {         // bin + 1
                result = 1;// 16x16 从1开始 0 号是 I_NxN
                ctxIdx = 2 + 1 + ctxIdxOffset;
                if(decode_binary(ctxIdx)) // 第2位 有没有 AC 系数 ，有则往下偏移 12
                    result += 12;
                if(decode_binary(++ctxIdx))  //第3位 色彩编码模式 如果！= 0 那么后面还有一位
                {
                    result += 4; // 第一位是 1 往下偏移 4 
                    if(decode_binary(++ctxIdx)) // 色彩编码模式的第二位
                        result += 4;
                }
                // predmode 解码预测模式，两位，上下文索引顺延上面的值 
                predmode = decode_binary(++ctxIdx);
                predmode <<= 1;
                predmode |= decode_binary(++ctxIdx);
                result += predmode;// 解出来就是值 向下偏移对应位数

                return result;
            }
        }
    }
    else if(slice_type == P || slice_type == SP)
    {
        // uint8 prefix_ctxIdxOffset = 14;
        // uint8 suffix_ctxIdxOffset = 17;
        // uint8 prefix_result = 0;
        // uint8 suffix_result = 0;
        // //求前缀
        // int binIdx = -1;
        // do
        // {
        //     binIdx++;// sumi
        //     if(binIdx == 0 || binIdx == 1) ctxIdxInc = binIdx;
        //     else ctxIdxInc = (b1 != 1) ? 2 : 3;
        //     ctxIdx = ctxIdxInc + prefix_ctxIdxOffset;
        //     result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        //     if(binIdx == 1) b1 = result_cur;
        //     result_cur <<= binIdx;
        //     prefix_result += result_cur;
        // } while((InBinarization(prefix_result, binIdx, binarization_mbtype_in_PandSP)) == -1);
        // prefix_result = InBinarization(prefix_result, binIdx, binarization_mbtype_in_PandSP);
        // //如果前缀值不为5，那么说明是Pslcie中的P宏块
        // if(prefix_result != 5){return prefix_result + 40;}
        // //否则前缀值为5，说明是P片中的I宏块
        // binIdx = -1;
        // do
        // {
        //     binIdx++;// sumi
        //      if(binIdx == 0) ctxIdxInc = 0;
        //     else if(binIdx == 1) {ctxIdx = 276;}
        //     else if(binIdx == 2 || binIdx == 3) ctxIdxInc = binIdx - 1;
        //     else if(binIdx == 4) ctxIdxInc = (b3 != 0) ? 2 : 3;
        //     else /*if(binIdx >= 5)*/ ctxIdxInc = 3;
        //     if(binIdx != 1) ctxIdx = ctxIdxInc + suffix_ctxIdxOffset;
        //     result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        //     if(binIdx == 3) b3 = result_cur;
        //     result_cur <<= binIdx;
        //     suffix_result += result_cur;
        // } while((InBinarization(suffix_result, binIdx, binarization_mbtype_in_I)) == -1);
        // suffix_result = InBinarization(suffix_result, binIdx, binarization_mbtype_in_I);
        // return suffix_result;

        // 还是和上面一样，先求前缀

        uint8 prefix_ctxIdxOffset = 14; // 写在这里怕忘记了
        uint8 suffix_ctxIdxOffset = 17;
        result = 0;
        ctxIdx = 0 + prefix_ctxIdxOffset;//
        if(decode_binary(ctxIdx))// 第一位是 1, 那么是帧内块
        {
            result += 5; // 前缀
            // 0 号 和 1号 的ctxIdx 顺延
            if(decode_binary(++ctxIdx)) // 是 I_NxN
                result += 0;
            else
                result += 1;           // 是 I_PCM 或者 I_16x16
        }
        else // 否则是 P 块
        {
            ++ctxIdx; // 顺延
            if(decode_binary(ctxIdx)) // 1 号是 1, 换ctxIdx
            {
                ctxIdx = 3 + prefix_ctxIdxOffset;
                if(decode_binary(ctxIdx))
                    result = 1;
                else
                    result = 2;
            }
            else // 否则 ctxIdx 顺延
                if(decode_binary(++ctxIdx))
                    result = 3; // P_8x8
                else
                    result = 0; // P_L0_16x16
        }
        // 这里能写 等于 是因为 I_NxN 如果能出来也已经出来了
        if(result <= 5) return result;
        // 否则是帧内块，换 ctxIdx
        ctxIdx = 0 + suffix_ctxIdxOffset;
        if(decode_finaly()) // 快进到休止解码判断 I_PCM
            return 30;
        else
        {
            int predmode;
            ctxIdx = 2 - 1 + suffix_ctxIdxOffset;
            if(decode_binary(ctxIdx)) // 第2位 有没有 AC 系数 ，有则往下偏移 12
                result += 12;
            if(decode_binary(++ctxIdx))  //第3位 色彩编码模式 如果！= 0 那么后面还有一位
            {
                result += 4; // 第一位是 1 往下偏移 4 
                if(decode_binary(++ctxIdx)) // 色彩编码模式的第二位
                    result += 4;
            }
            // predmode 解码预测模式，两位，上下文索引顺延上面的值 
            predmode = decode_binary(++ctxIdx);
            predmode <<= 1;
            predmode |= decode_binary(++ctxIdx);
            result += predmode;// 解出来就是值 向下偏移对应位数

            return result;
        }
        
        
    }
    else if(slice_type == SI)
    {
        uint8 prefix_ctxIdxOffset = 0;
        uint8 suffix_ctxIdxOffset = 3;

    }
    else /*if(slice_type == B)*/
    {
    //     uint8 prefix_result = 0;
    //     uint8 suffix_result = 0;
    //     int binIdx = -1;
    //     do
    //     {
    //         binIdx++;// sumi
    //         if(binIdx == 0) ctxIdxInc = f(cur, 'A', prefix_ctxIdxOffset) + f(cur, 'B', prefix_ctxIdxOffset);
    //         else if(binIdx == 1) ctxIdxInc = 3;
    //         else if(binIdx == 2) ctxIdxInc = (b1 != 0) ? 4: 5;
    //         else ctxIdxInc = 5;
    //         ctxIdx = ctxIdxInc + prefix_ctxIdxOffset;
    //         result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
    //         if(binIdx == 1) b1 = result_cur;
    //         result_cur <<= binIdx;
    //         prefix_result += result_cur;
    //         if(binIdx > 7) exit(-1);
    //     } while((InBinarization(prefix_result, binIdx, binarization_mbtype_in_B)) == -1);
    //     prefix_result = InBinarization(prefix_result, binIdx, binarization_mbtype_in_B);
    //     //如果前缀值不为23，那么说明是Bslcie中的B宏块
    //     if(prefix_result != 23){return prefix_result + 50;}
    //     //否则前缀值为23，说明是B片中的I宏块
    //     binIdx = -1;
    //     do
    //     {
    //         binIdx++;// sumi
    //         if(binIdx == 0) ctxIdxInc = 0;
    //         else if(binIdx == 1) {ctxIdx = 276;}
    //         else if(binIdx == 2 || binIdx == 3) ctxIdxInc = binIdx - 1;
    //         else if(binIdx == 4) ctxIdxInc = (b3 != 0) ? 2 : 3;
    //         else /*if(binIdx >= 5)*/ ctxIdxInc = 3;
    //         if(binIdx != 1) ctxIdx = ctxIdxInc + suffix_ctxIdxOffset;
    //         result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
    //         if(binIdx == 3) b3 = result_cur;
    //         result_cur <<= binIdx;
    //         suffix_result += result_cur;
    //     } while((InBinarization(suffix_result, binIdx, binarization_mbtype_in_I)) == -1);
    //     suffix_result = InBinarization(suffix_result, binIdx, binarization_mbtype_in_I);
    //     return suffix_result;


        uint8 prefix_ctxIdxOffset = 27;
        uint8 suffix_ctxIdxOffset = 32;
        int condTermFlagA, condTermFlagB;
        int tmp = 0;
        result = 0;
        ctxIdx = condTermFlagA + condTermFlagA + prefix_ctxIdxOffset;
        if(decode_binary(ctxIdx)) // 0
        {
            ctxIdx = 3 + prefix_ctxIdxOffset;
            if(decode_binary(ctxIdx))// 1
            {
                // 下面这个 if 和 else 段的代码要结合附件中的图来看和分析
                ctxIdx = 4 + prefix_ctxIdxOffset;
                if(decode_binary(ctxIdx))// 2  从这个if开始 ，ctxIdx 不再变化
                {
                    ctxIdx = 5 + prefix_ctxIdxOffset;
                    result += 8; // 向下 8 位
                               tmp |= decode_binary(ctxIdx);
                    tmp <<= 1; tmp |= decode_binary(ctxIdx);
                    tmp <<= 1; tmp |= decode_binary(ctxIdx);
                    if(tmp < 5)
                    {
                        tmp <<= 1; tmp = decode_binary(ctxIdx); 
                        result += tmp;
                    }
                    else
                    {
                        if(tmp == 5)      result = 23;
                        else if(tmp == 6) result = 11;
                        else result = 22;
                    }
                }
                else
                {
                    result = 3;
                    ctxIdx = 5 + prefix_ctxIdxOffset;
                               tmp |= decode_binary(ctxIdx);
                    tmp <<= 1; tmp |= decode_binary(ctxIdx);
                    tmp <<= 1; tmp |= decode_binary(ctxIdx);
                    result += tmp;
                }
            }
            else
            {
                result += 1; // 向下 1 位
                ctxIdx = 5 + prefix_ctxIdxOffset;
                if(decode_binary(ctxIdx))
                    result += 1;// (B_L1_16x16)
                else
                    result += 0;//(B_L0_16x16)
            }
        }
        else
            result = 0;
        // 23 号就是 I_NxN 
        if(result <= 23) return result;
        ctxIdx = 0 + suffix_ctxIdxOffset;
        if(decode_finaly()) // 快进到休止解码判断 I_PCM
            return 48;
        else // 和求 P 中的 I块一样的步骤
        {
            int predmode;
            ctxIdx = 2 - 1 + suffix_ctxIdxOffset;
            if(decode_binary(ctxIdx)) // 第2位 有没有 AC 系数 ，有则往下偏移 12
                result += 12;
            if(decode_binary(++ctxIdx))  //第3位 色彩编码模式 如果！= 0 那么后面还有一位
            {
                result += 4; // 第一位是 1 往下偏移 4 
                if(decode_binary(++ctxIdx)) // 色彩编码模式的第二位
                    result += 4;
            }
            // predmode 解码预测模式，两位，上下文索引顺延上面的值 
            predmode = decode_binary(++ctxIdx);
            predmode <<= 1;
            predmode |= decode_binary(++ctxIdx);
            result += predmode;// 解出来就是值 向下偏移对应位数

            return result;
        }
    }
    
    return result;
}
uint8 cabac::read_sub_mb_type()
{
    uint8 result = 0;

    uint16 ctxIdx_cur = 0;
    uint16 result_cur = 0;
    uint16 ctxIdxInc = 0;

    type_slice slice_type = lifeTimeSlice->type;
    int ctxIdxOffset = 0;
    if(slice_type == P || slice_type == SP)
    {// offset = 21
        ctxIdxOffset = 21;
        // int binIdx = -1;
        // do
        // {
        //     binIdx++;// sumi
        //     ctxIdxInc = binIdx;
        //     ctxIdx_cur = ctxIdxInc + ctxIdxOffset;
        //     result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx_cur, 0);
        //     result_cur <<= binIdx;
        //     result += result_cur;
        // } while((InBinarizationSub(result, binIdx, binarization_submbtype_in_PandSP)) == -1);
        // result = InBinarizationSub(result, binIdx, binarization_submbtype_in_PandSP);

        ctxIdx = 0 + ctxIdxOffset;
        if(decode_binary(ctxIdx))
            result = 0;
        else
        {
            if(decode_binary(++ctxIdx))
                if(decode_binary(++ctxIdx))
                    result = 2;
                else
                    result = 3;
            else
                result = 1;
        }
    }
    else// if(slice_type == B)
    {// offset = 36
        ctxIdxOffset = 36;
        // int binIdx = -1;
        // do
        // {
        //     binIdx++;// sumi
        //     if(binIdx == 0){ctxIdxInc = 0;}
        //     else if(binIdx == 1) {ctxIdxInc = 1;}
        //     else if(binIdx == 2) ctxIdxInc = (b1 != 0) ? 2 : 3;
        //     else ctxIdxInc = 3;

            
        //     ctxIdx_cur = ctxIdxInc + ctxIdxOffset;
        //     result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx_cur, 0);
        //     if(binIdx == 1) b1 = result_cur;
        //     result_cur <<= binIdx;
        //     result += result_cur; 
        // } while((InBinarizationSub(result, binIdx, binarization_submbtype_in_B)) == -1);
        // result = InBinarizationSub(result, binIdx, binarization_submbtype_in_B);
        ctxIdx = 0 + ctxIdxOffset;
        int tmp = 0;
        if(decode_binary(ctxIdx)) // 这里的ctxIdx 只有到了 3 + offset 才不会变
            if(decode_binary(++ctxIdx))
            {
                ctxIdx += 1;// 2 + offset
                if(decode_binary(ctxIdx))// 2 + offset
                {
                    if(decode_binary(++ctxIdx))
                        result = 11 + decode_binary(ctxIdx);
                    else
                    {
                        tmp  = decode_binary(ctxIdx);
                        tmp <<= 1; 
                        tmp |= decode_binary(ctxIdx);
                        result = 7 + tmp;
                    }
                }
                else
                {
                    result = 3;
                    tmp  = decode_binary(++ctxIdx);
                    tmp <<= 1;
                    tmp |= decode_binary(ctxIdx);
                    result += tmp;
                }
            }
            else
            {
                ctxIdx += 2;
                result = decode_binary(ctxIdx) + 1;
            }
        else
            result = 0;
    }
    return result;
}
uint8 cabac::read_transform_8x8_size_flag()
{
    uint8 result_cur = 0;
    uint16 result = 0;
    
    macroblock* currentMB = lifeTimeSlice->curMB;
    macroblock* A = NULL, * B = NULL;
    A = pic->neighbour_macroblock(currentMB, 'A');
    B = pic->neighbour_macroblock(currentMB, 'B');
    uint8  condTermFlagA = 0,  condTermFlagB = 0;

    if(!A || A->transform_size_8x8_flag == 0) condTermFlagA = 0;
    else condTermFlagA = 1;
    if(!B || B->transform_size_8x8_flag == 0) condTermFlagB = 0;
    else condTermFlagB = 1;

    ctxIdxInc = condTermFlagA + condTermFlagB;
    ctxIdx = ctxIdxInc + 399;


    result = decode_binary(ctxIdx);

    return result;
}

uint16 cabac::read_coded_block_pattern()
{
    //prefix  and suffix
    uint8 prefix_ctxIdxOffset = 73, suffix_ctxIdxOffset = 77;
    //maxBin:                     3                          1
    //prefix: FL cMax = 15
    
    
    uint8 result_cur = 0;
    uint16 result = 0;
    macroblock* currentMB = lifeTimeSlice->curMB;
    macroblock* A = NULL, * B = NULL;
    //prefix
    uint16 prefix_ctxIdxInc = 0;
    uint16 prefix_result = 0;
    int binIdx = -1;
    int tmp;

    uint8  condTermFlagA = 0,  condTermFlagB = 0;
    tmp = 0;
    do
    {
        binIdx++;// sumi
        A = pic->get_BLneighbour(currentMB, 'A', binIdx, 0x012, tmp);
        if(!A || A->type == I_PCM ||\
          (A != currentMB && (A->type != P_Skip || A->type != B_Skip) && ((A->CodedBlockPatternLuma >> tmp) & 1) != 0)||\
          (A == currentMB && (((prefix_result >> tmp) & 0x1) != 0)))
            condTermFlagA = 0;
        else condTermFlagA = 1;
        B = pic->get_BLneighbour(currentMB, 'B', binIdx, 0x012, tmp);
        if(!B || B->type == I_PCM ||\
          (B != currentMB && (B->type != P_Skip || B->type != B_Skip) && ((B->CodedBlockPatternLuma >> tmp) & 1) != 0)||\
          (B == currentMB && (((prefix_result >> tmp) & 0x1) != 0)))
            condTermFlagB = 0;
        else condTermFlagB = 1;
        prefix_ctxIdxInc = condTermFlagA + 2 * condTermFlagB;
        ctxIdx = prefix_ctxIdxInc + prefix_ctxIdxOffset;
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        result_cur = result_cur << binIdx;
        prefix_result += result_cur;
    } while(binIdx < 3);

    uint16 suffix_ctxIdxInc = 0;
    uint16 suffix_result = 0;
    tmp = 0;
    binIdx = -1;
    do
    {
        binIdx++;// sumi
        A = pic->neighbour_macroblock(currentMB, 'A');
        if(A && A->type == I_PCM) condTermFlagA = 1;
        else if((!A || A->type == P_Skip || A->type == B_Skip) ||\
                (binIdx == 0 && A->CodedBlockPatternChroma == 0) ||\
                (binIdx == 1 && A->CodedBlockPatternChroma != 2))
            condTermFlagA = 0;
        else condTermFlagA = 1;
        B = pic->neighbour_macroblock(currentMB, 'B');
        if(B && B->type == I_PCM) condTermFlagB = 1;
        else if((!B || B->type == P_Skip || B->type == B_Skip) ||\
                (binIdx == 0 && B->CodedBlockPatternChroma == 0) ||\
                (binIdx == 1 && B->CodedBlockPatternChroma != 2))
            condTermFlagB = 0;
        else condTermFlagB = 1;
        suffix_ctxIdxInc = condTermFlagA + 2 * condTermFlagB + ((binIdx == 1) ? 4 : 0);
        ctxIdx = suffix_ctxIdxInc + suffix_ctxIdxOffset;
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        result_cur = result_cur << binIdx;
        suffix_result += result_cur;
    } while(IsIn_TU_binarization(suffix_result, 2, binIdx) == -1);
    suffix_result = IsIn_TU_binarization(suffix_result, 2, binIdx);
    suffix_result <<= 4;
    result = prefix_result + suffix_result;


    return result;

    //suffix: TU cMax = 2;
    
}
int8 cabac::read_mb_qp_delta()
{
    int syntaxRequest = 0;
    uint16  ctxIdx_cur = 0;
    uint64 result_cur = 0;
    uint64 result = 0;
    int isResult = 0;
    int binIdx = -1;
    do
    {
        binIdx++;// sumi
        ctxIdx_cur = DecodeCtxIdxUsingBinIdx(binIdx, 2, 60, 0);
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx_cur, 0);
        // printf(">>cabac: mb_qp_delta_cur :%d\n", result_cur);
        result_cur <<= binIdx;
        result += result_cur;
    } while((isResult = IsIn_U_binarization(result, binIdx)) == -1);
    result = isResult;
    result = pow((-1), (result + 1)) * (ceil((double)result / 2));
    
    return result;
    
}
uint8 cabac::read_significant_coeff_flag(int syntaxelement)
{
    //get ctxBlockCat and maxNumCoeff
    uint8 ctxBlockCat = (syntaxelement >> 8) & 0xF;
    uint8 maxNumCoeff = coded_blokc_flag_maxNumCoeff[ctxBlockCat];
    uint8 levelListIdx = syntaxelement >> 20;

    //get ctxIdxOffset
    uint16 ctxIdxOffset = 0;
    if(ctxBlockCat < 5) ctxIdxOffset = 105;
    else if(ctxBlockCat == 5) ctxIdxOffset = 402;
    else if(ctxBlockCat > 5 && ctxBlockCat < 9) ctxIdxOffset = 484;
    else if(ctxBlockCat == 9) ctxIdxOffset = 660;
    else if(ctxBlockCat > 9 && ctxBlockCat < 13) ctxIdxOffset = 528;
    else if(ctxBlockCat == 13) ctxIdxOffset = 718;
    else if(ctxBlockCat == 5 || ctxBlockCat == 9 || ctxBlockCat == 13) ctxIdxOffset = 1012;

    if(ctxBlockCat != 3 && ctxBlockCat != 5 && ctxBlockCat != 9 && ctxBlockCat != 13) ctxIdxInc = levelListIdx;
    else if(ctxBlockCat == 3) ctxIdxInc = Min(levelListIdx / 1 , 2);
    else ctxIdxInc = ctxIncForCtxBlockCat[levelListIdx][0];

    ctxIdx = (int)ctxIdxInc + (int)ctxIdxBlockCatOffsetOfctxBlockCat[1][ctxBlockCat] + (int)ctxIdxOffset;
    uint8 result = DecodeValueUsingCtxIdx(ctxIdx, 0);
    return result;

}
uint8 cabac::read_last_significant_coeff_flag(int syntaxelement)
{
 

    //get ctxBlockCat and maxNumCoeff
    uint8 ctxBlockCat = (syntaxelement >> 8) & 0xF;
    uint8 maxNumCoeff = coded_blokc_flag_maxNumCoeff[ctxBlockCat];
    uint8 levelListIdx = syntaxelement >> 20;

    //get ctxIdxOffset
    uint16 ctxIdxOffset = 0;
    if(ctxBlockCat < 5) ctxIdxOffset = 166;
    else if(ctxBlockCat == 5) ctxIdxOffset = 417;
    else if(ctxBlockCat > 5 && ctxBlockCat < 9) ctxIdxOffset = 572;
    else if(ctxBlockCat == 9) ctxIdxOffset = 690;
    else if(ctxBlockCat > 9 && ctxBlockCat < 13) ctxIdxOffset = 616;
    else if(ctxBlockCat == 13) ctxIdxOffset = 748;
    else if(ctxBlockCat == 5 || ctxBlockCat == 9 || ctxBlockCat == 13) ctxIdxOffset = 1012;

    if(ctxBlockCat != 3 && ctxBlockCat != 5 && ctxBlockCat != 9 && ctxBlockCat != 13) ctxIdxInc = levelListIdx;
    else if(ctxBlockCat == 3) ctxIdxInc = Min(levelListIdx / 1 , 2);
    else ctxIdxInc = ctxIncForCtxBlockCat[levelListIdx][2];
    
    ctxIdx = (int)ctxIdxInc + (int)ctxIdxBlockCatOffsetOfctxBlockCat[2][ctxBlockCat] + (int)ctxIdxOffset;

    uint8 result = decode_binary(ctxIdx);

    return result;
}
uint8 cabac::read_coded_block_flag(int syntaxelement)
{
    //get ctxBlockCat and maxNumCoeff

    uint8 ctxBlockCat = (syntaxelement >> 8) & 0xF;
    uint8 maxNumCoeff = coded_blokc_flag_maxNumCoeff[ctxBlockCat];
    uint8 iCbCr = (syntaxelement >> 4) & 0xF;
    uint8 index = (syntaxelement) & 0xF;

    uint8 colorType = 0;
    if(iCbCr == 3) colorType = 1;
    else colorType = iCbCr + 2;
    colorType <<= 4;

    uint8 blockType = 0;
    switch (ctxBlockCat)
    {
    case 0:blockType = 0x003;break;
    case 1:blockType = 0x001;break;
    case 2:blockType = 0x001;break;
    case 3:blockType = 0x001;break;
    case 4:blockType = 0x001;break;
    case 5:blockType = 0x002;break;
    default:break;
    }
    
    //get ctxIdxOffset
    uint16 ctxIdxOffset = 0;
    if(ctxBlockCat < 5) ctxIdxOffset = 85;
    else if(ctxBlockCat > 5 && ctxBlockCat < 9) ctxIdxOffset = 460;
    else if(ctxBlockCat > 9 && ctxBlockCat < 13) ctxIdxOffset = 472;
    else if(ctxBlockCat == 5 || ctxBlockCat == 9 || ctxBlockCat == 13) ctxIdxOffset = 1012;
    
    //get tranblock
    macroblock* currentMB = lifeTimeSlice->curMB;
    int index_A = 0;
    macroblock* mbAddrA = pic->get_BLneighbour(currentMB, 'A', index, colorType + blockType, index_A);
    block*  transBlockA;    
    int index_B = 0;
    macroblock* mbAddrB = pic->get_BLneighbour(currentMB, 'B', index, colorType + blockType, index_B);
    block*  transBlockB;

    
    if(ctxBlockCat == 0 || ctxBlockCat == 6 || ctxBlockCat == 10)
    {  
        auto f = [iCbCr, ctxBlockCat](macroblock* cur, macroblock* N , int index_N)->block*{
            block* transBlockN = NULL;
            if((N && cur->is_avaiable(N)) && N->predmode == Intra_16x16)
                {
                    if(ctxBlockCat == 0) transBlockN = N->re->luma[1];
                    else if(ctxBlockCat == 3) transBlockN = N->re->chroma[0]->get_childBlock(iCbCr);
                    else if(ctxBlockCat == 4) transBlockN = N->re->chroma[1]->get_childBlock(iCbCr)->get_childBlock(index_N);
                }
            else transBlockN = NULL;
            return transBlockN;
        };
        transBlockA = f(currentMB, mbAddrA, index_A);
        transBlockB = f(currentMB, mbAddrB, index_B);
    }
    else if(ctxBlockCat == 1 || ctxBlockCat == 2)
    {
        auto f = [iCbCr](macroblock* cur, macroblock* N, int index_N)->block*{
            block* transBlockN = NULL;
            if(cur->is_avaiable(N) && (N->type != P_Skip && N->type != B_Skip && N->type != I_PCM) && ((N->CodedBlockPatternLuma >>(index_N >> 2)) & 1) != 0 && N->transform_size_8x8_flag == 0)
            transBlockN = N->re->luma[0]->get_childBlock(index_N/4)->get_childBlock(index_N%4);
            else if(cur->is_avaiable(N) && (N->type != P_Skip && N->type != B_Skip && N->type != I_PCM) && ((N->CodedBlockPatternLuma >>(index_N >> 2)) & 1) != 0 && N->transform_size_8x8_flag == 1)
                transBlockN = NULL;//这里没有认真写，因为fox.264不会出现8x8变换解码
            else transBlockN = NULL;
            return transBlockN;
        };
        transBlockA = f(currentMB, mbAddrA, index_A);
        transBlockB = f(currentMB, mbAddrB, index_B);      
    }
    else if(ctxBlockCat == 3)
    {
        mbAddrA = pic->neighbour_macroblock(currentMB, 'A');
        mbAddrB = pic->neighbour_macroblock(currentMB, 'B');
        auto f = [iCbCr](macroblock* cur, macroblock* mbAddrN)->block*{
            block* transBlockN = NULL;
            if(mbAddrN && (mbAddrN->type != P_Skip && mbAddrN->type != B_Skip && mbAddrN->type != I_PCM) && mbAddrN->CodedBlockPatternChroma != 0)
                transBlockN = mbAddrN->re->chroma[0]->get_childBlock(iCbCr);
            else transBlockN = NULL;
            return transBlockN;
        };
        transBlockA = f(currentMB, mbAddrA);
        transBlockB = f(currentMB, mbAddrB);
    }
    else if(ctxBlockCat == 4)
    {
        if(mbAddrA && (mbAddrA->type != P_Skip && mbAddrA->type != B_Skip && mbAddrA->type != I_PCM) && mbAddrA->CodedBlockPatternChroma == 2)
            transBlockA = mbAddrA->re->chroma[1]->get_childBlock(iCbCr)->get_childBlock(index_A);
        else transBlockA = NULL;

        if(mbAddrB && (mbAddrB->type != P_Skip && mbAddrB->type != B_Skip && mbAddrB->type != I_PCM) && mbAddrB->CodedBlockPatternChroma == 2)
            transBlockB = mbAddrB->re->chroma[1]->get_childBlock(iCbCr)->get_childBlock(index_B);
        else transBlockB = NULL;
    }
    //后面的ctxBlockCat都没有写，因为从5开始都是用于444的

    //get condTermFlagA and condTermFlagB
    uint8 condTermFlagA, condTermFlagB;
    slice* sl = lifeTimeSlice;
    auto f_condTermFlagN = [sl](macroblock* cur, macroblock* N, block* transBlockN)->uint8_t{
        uint8 condTermFlagN = 0;

        if((!cur->is_avaiable(N) && cur->is_interpred())||\
        (cur->is_avaiable(N) && transBlockN == NULL && N && N->type != I_PCM)||\
        ((cur->is_intrapred() && N && N->is_interpred()) && (sl->curNAL->nal_unit_type >= 2 && sl->curNAL->nal_unit_type <= 4))
        ) 
        condTermFlagN = 0;
        else if((!cur->is_avaiable(N) && cur->is_intrapred())||\
                (N && N->type == I_PCM)) 
                condTermFlagN = 1;
        else condTermFlagN = transBlockN->coded_block_flag;

        return condTermFlagN;
    };
    //calc ctxIdxInc
    uint8 ctxIdxInc;
    ctxIdxInc = f_condTermFlagN(currentMB, mbAddrA, transBlockA) + 2 * f_condTermFlagN(currentMB, mbAddrB, transBlockB);

    //get ctxIdxOffsetCat
    uint8 ctxIdxOffsetCat = ctxIdxBlockCatOffsetOfctxBlockCat[0][ctxBlockCat];
    //calc ctxIdx
    ctxIdx = ctxIdxInc + ctxIdxOffsetCat + ctxIdxOffset;

    //calc bin;
    uint8 result =  decode_binary(ctxIdx);

    return result;
}
uint8 cabac::read_coeff_sign_flag(int syntaxelement)
{
    uint8 result = decode_bypass();
    return result;
}
uint32 cabac::read_coeff_abs_level_minus1(int syntaxelement)
{
    uint8 ctxBlockCat = (syntaxelement >> 8) & 0xF;

    int binIdx = -1;
    //当前的binIdx解出来的ctxIdx
    uint16 result = 0;
    //当前位上的数据
    uint32 result_cur = 0;
    //当前解出来的二进制串的数据
    uint32 result_str = 0;
    //当前的结果
    int64 prefix_result;
    int64 suffix_result;
    //总的offset
    uint8 ctxIdxOffset = 0;

    //求前缀

    //求总offset的前缀   最大只有两位
    uint16 prefix_ctxIdxOffset = 0;
    if(ctxBlockCat < 5) prefix_ctxIdxOffset = 227;
    else if(ctxBlockCat == 5) prefix_ctxIdxOffset = 417;
    else if(ctxBlockCat > 5 && ctxBlockCat < 9) prefix_ctxIdxOffset = 864;
    else if(ctxBlockCat == 9) prefix_ctxIdxOffset = 690;
    else if(ctxBlockCat > 9 && ctxBlockCat < 13) prefix_ctxIdxOffset = 908;
    else if(ctxBlockCat == 13) prefix_ctxIdxOffset = 748;


    int numDecodAbsLevelEq1 = (syntaxelement >> 24) & 0xF;
    int numDecodAbsLevelGt1 = (syntaxelement >> 20) & 0xF;
    //求前缀二值串
    binIdx = -1;
    do
    {
        binIdx++;// sumi
        //求cat索引的ctxIdxInc
        uint8 ctxIdxInc = 0;
        if(binIdx == 0) ctxIdxInc = ((numDecodAbsLevelGt1 != 0) ? 0: Min(4, (1 + numDecodAbsLevelEq1)));
        else ctxIdxInc = 5 + Min(4 - ((ctxBlockCat == 3)?1 : 0), numDecodAbsLevelGt1);

        ctxIdx = ctxIdxInc + prefix_ctxIdxOffset + ctxIdxBlockCatOffsetOfctxBlockCat[3][ctxBlockCat];
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        result_cur = result_cur << binIdx;
        result_str += result_cur;
    } while( ((prefix_result = IsIn_TU_binarization(result_str, 14, binIdx)) == -1) && (binIdx < 13) );
    
    if(terr.cabac_result_bin()) std::cout << ">>cabac: break line -------------------------------for prefix & suffix" << std::endl;
    //求后缀二值串，旁路解码
    uint8 binIdx2 = -1;
    result_cur = 0;
    result_str = 0;
    //如果前缀不足14位或者是一个14位的截断数，那么没有后缀
    if(binIdx < 13 || (binIdx == 13 && prefix_result == 13)){suffix_result = 0;}
    else if(prefix_result == 14)
    {
        do
        {
            binIdx2++;// sumi
            result_cur = decode_bypass();
            result_str <<= 1;
            result_str += result_cur;
        } while((suffix_result = IsIn_UEGk_binarization(result_str, binIdx2, 0, 0, 0)) == -1);
    }
    result = suffix_result + prefix_result;

    return result;
}

uint16 cabac::read_intra_chroma_pred_mod()
{
    int syntaxRequest;
    uint16 ctxIdxInc;
    uint16 result_cur = 0;
    uint16 result_all = 0;
    uint16 result = 0;

    uint8 condTermFlagA = 0;
    uint8 condTermFlagB = 0;
    macroblock* cur   = lifeTimeSlice->curMB;
    macroblock* A     = pic->neighbour_macroblock(cur, 'A');
    macroblock* B     = pic->neighbour_macroblock(cur, 'B');
    auto f = [](macroblock* cur, macroblock* N)->uint8_t{
        uint8 condTermFlagN;
        if(!cur->is_avaiable(N) ||\
        N->is_interpred() || \
        N->type == I_PCM || \
        N->intra->intra_chroma_pred_mode  == 0
        )  condTermFlagN = 0;
        else condTermFlagN = 1;
        return condTermFlagN;
    };
    int binIdx = -1;
    do
    {
        binIdx++;// sumi
        if(binIdx == 0)
        {
            
            condTermFlagA = f(cur, A);
            condTermFlagB = f(cur, B);
            ctxIdxInc = condTermFlagA + condTermFlagB;   
        }
        else ctxIdxInc = 3;
        ctxIdx =  ctxIdxInc + 64;
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        result_cur = result_cur << binIdx;
        result_all += result_cur;
    } while(IsIn_TU_binarization(result_all, 3, binIdx) == -1);
    result = IsIn_TU_binarization(result_all, 3, binIdx);
    return result;
}

uint8 cabac::read_prev_intra4x4_pred_mode_flag()
{
    //FL cMax = 1   offset = 68
    uint16 ctxIdx_cur = 0 + 68;
    uint8 result = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx_cur, 0);
    return result;
}
uint8 cabac::read_rem_intra4x4_pred_mode()
{
    //FL cMax = 7   offset = 69
    uint16 ctxIdx = 0 + 69;
    uint8 result = 0;
    uint8 result_cur = 0;
    int binIdx = -1;
    do
    {
        binIdx++;
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        result_cur <<= binIdx;
        result += result_cur;
    } while (binIdx < 2);
    return result;
}

uint8 cabac::read_mb_skip_flag()
{
    uint8 result = 0;
    
    ctxIdxOffset = 0;

    type_slice slice_type = lifeTimeSlice->type;

    ctxIdxOffset = slice_type == B ? 24 : 11;
    macroblock* cur = lifeTimeSlice->curMB;

    int condTermFlagA = cur->neighbour.A.avaiable ? (cur->neighbour.A.pointer->mb_skip_flag == 0) : 0;
    int condTermFlagB = cur->neighbour.B.avaiable ? (cur->neighbour.B.pointer->mb_skip_flag == 0) : 0;

    // macroblock* A = pic->neighbour_macroblock(cur, 'A');
    // macroblock* B = pic->neighbour_macroblock(cur, 'B');
    // auto f = [](macroblock* cur,  macroblock* N)->uint8_t{
    //     uint8 condTermFlagN = 0; 
    //     if(!cur->is_avaiable(N) || N->mb_skip_flag == 1) 
    //         condTermFlagN = 0;
    //     else condTermFlagN = 1;
    //     return condTermFlagN;
    // };

    ctxIdx = condTermFlagA + condTermFlagB + ctxIdxOffset;
    result = decode_binary(ctxIdx);

    return result;
}



//句法元素结构：    0xAVVI
//最低位是预测的方向(最低位的1上是时间，2是空间)，上面的两位是句法元素标志值，再往上一位是宏块分区的索引，再往上一位是句法元素MbaffFrameFlag的值
uint8 cabac::read_ref_idx(int syntaxelements)
{
    //U offset 54    
    uint16 result_cur = 0;
    uint16 result_all = 0;
    uint16 result = 0;

    uint8 temporalDirection = (syntaxelements) & 0x1;
    uint8 spatialDirection  = (syntaxelements >> 1) & 0x1;
    uint8 mbPartIdx         = (syntaxelements >> 8) & 0xF;
    //subIdx 左移4位去0xF的并
    uint8 MbaffFrameFlag    = (syntaxelements >> 20) & 0xF;
    
    uint16 ctxIdxInc = 0;
    uint16 ctxIdxOffset = 54;
    picture* p = this->pic;

    macroblock* cur   = lifeTimeSlice->curMB;
    predmode_mb_part Pred_L = temporalDirection ? Pred_L1 : Pred_L0;

    auto f = [p,mbPartIdx,Pred_L,spatialDirection, temporalDirection,MbaffFrameFlag](macroblock* cur, char direction)->uint8_t{
        int tmp = 0;
        int mbPartIdxN = 0;
        uint8 fieldflag = 0;//if(MbaffFrameFlag && cur是帧宏块 && N 是场宏块)
        macroblock* N = p->get_PartNeighbour(cur, direction, 0x010, mbPartIdx, 0, mbPartIdxN, tmp);
        uint8 predModeEqualFlagN = 0;
        uint8 refIdxZeroFlagN = 0;
        if(N)
        {
            if(N->is_interpred())
            {
                if(temporalDirection)refIdxZeroFlagN = (N->ref_idx_l1[mbPartIdxN] > fieldflag)?0:1;
                else refIdxZeroFlagN = (N->ref_idx_l0[mbPartIdxN] > fieldflag)?0:1;
            }
            
            if(N->type == B_Direct_16x16 || N->type == B_Skip) predModeEqualFlagN = 0;
            else if(N->type == P_8x8 || N->type == B_8x8)
            {
                predmode_mb_part partpremode = SubMbPartPredMode(N->sub_mb_type[mbPartIdxN]);
                if(partpremode != Pred_L && partpremode != BiPred) predModeEqualFlagN = 0;
                else predModeEqualFlagN = 1;
            }
            else
            {
                predmode_mb_part partpremode = MbPartPredMode(N, N->type, mbPartIdxN);
                if(partpremode != Pred_L && partpremode != BiPred) predModeEqualFlagN = 0;
                else predModeEqualFlagN = 1;
            }
        }
        uint8 condTermFlagN = 0;
        if(!cur->is_avaiable(N) || \
           (N->type == P_Skip && N->type == B_Skip) || \
           N->is_intrapred() || \
           predModeEqualFlagN == 0 || \
           refIdxZeroFlagN == 1
        ) condTermFlagN = 0;
        else condTermFlagN = 1;
        return condTermFlagN;
    };
    int binIdx = -1;
    do
    {
        binIdx++;// sumi

        if(binIdx == 0)
        {
            uint8 condTermFlagA = 0, condTermFlagB = 0;
            condTermFlagA = f(cur, 'A');
            condTermFlagB = f(cur, 'B');
            ctxIdxInc = condTermFlagA + 2 * condTermFlagB;   
        }
        else if(binIdx == 1) ctxIdxInc = 4;
        else ctxIdxInc = 5;

        ctxIdx =  ctxIdxInc + ctxIdxOffset;
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        result_cur = result_cur << binIdx;
        result_all += result_cur;
    } while(IsIn_U_binarization(result_all, binIdx) == -1);
    result = IsIn_U_binarization(result_all, binIdx);
    return result;
}
//子块索引1 + 子宏块子块索引1  + 句法2 + 方向1
int cabac::read_mvd_lx(int syntaxelements)
{
    //编码模式UEG3
    //signedValFlag=1, uCoff=9  pre offset 40
    uint8 signedValFlag = 1;
    uint16 result_cur = 0;
    uint16 result_all = 0;
    int result = 0;
    
    uint8 temporalDirection = (syntaxelements) & 0x1;
    uint8 spatialDirection  = (syntaxelements >> 1) & 0x1;
    uint8 subMbPartIdx      = (syntaxelements >> 4) & 0xF;
    uint8 mbPartIdx         = (syntaxelements >> 8) & 0xF;
    uint8 MbaffFrameFlag    = (syntaxelements >> 20) & 0xF;
    
    macroblock* cur   = lifeTimeSlice->curMB;
    predmode_mb_part Pred_L = temporalDirection ? Pred_L1 : Pred_L0;
    picture* p = this->pic;

    //求前缀
    uint8 prefix_ctxIdxOffset = 0;

    int64 prefix_result = 0;
    int64 suffix_result = 0;
    //如果是水平预测，偏移是40，否则47
    if(spatialDirection == 0) prefix_ctxIdxOffset = 40;
    else prefix_ctxIdxOffset = 47;


    auto f = [p,mbPartIdx,subMbPartIdx,Pred_L, temporalDirection, spatialDirection, MbaffFrameFlag](macroblock* cur,int mbPartIdxN, char direction)->uint16_t{
        int subMbPartIdxN = 0;
        uint8 fieldflag = 0;//if(MbaffFrameFlag && cur是帧宏块 && N 是场宏块)
        macroblock* N = p->get_PartNeighbour(cur, direction, 0x010, mbPartIdx, subMbPartIdx, mbPartIdxN, subMbPartIdxN);
        matrix* mvd_lX = NULL;
        if(N) mvd_lX = temporalDirection ? N->mvd_l1 : N->mvd_l0; 

        uint8 predModeEqualFlagN = 0;
        if(N)
        {
            if(N->type == B_Direct_16x16 || N->type == B_Skip) predModeEqualFlagN = 0;
            else if(N->type == P_8x8 || N->type == B_8x8)
            {
                predmode_mb_part subpremode = (MbPartPredMode)SubMbPartPredMode(N->sub_mb_type[mbPartIdxN]);
                if(subpremode != Pred_L && subpremode != BiPred) predModeEqualFlagN = 0;
                else predModeEqualFlagN = 1;
            }
            else
            {
                predmode_mb_part partpremode = MbPartPredMode(N, N->type, mbPartIdxN);
                if(partpremode != Pred_L && partpremode != BiPred)predModeEqualFlagN = 0;
                else predModeEqualFlagN = 1;
            }
        }
        uint16 absMvdCompN = 0;
        if( !cur->is_avaiable(N) || \
            (N->type == P_Skip || N->type == B_Skip) || \
            N->is_intrapred() || \
            predModeEqualFlagN == 0  )
        {absMvdCompN = 0;}
        else
        {
            //这俩还有两个帧场的条件
            absMvdCompN = Abs(mvd_lX[mbPartIdxN][subMbPartIdxN][spatialDirection]);
        }
        return absMvdCompN;
    };
    uint8 ctxIdxInc = 0;
    int binIdx = -1;
    do
    {
        binIdx++;// sumi
        if(binIdx == 0)
        {
            uint16 absMvdCompA = f(cur,mbPartIdx, 'A');
            uint16 absMvdCompB = f(cur,mbPartIdx, 'B');
            uint16 absMvdComp = absMvdCompA + absMvdCompB;   
            if(absMvdComp < 3) ctxIdxInc = 0;
            else if(absMvdComp > 32) ctxIdxInc = 2;
            else ctxIdxInc = 1;
        }
        else if(binIdx >= 1 && binIdx <= 3) ctxIdxInc = binIdx + 2;
        else ctxIdxInc = 6;
        // if(cur->id_slice == 3 && cur->position_x == 10 && cur->position_y == 28 && mbPartIdx == 1)
        //     ctxIdxInc = 2;
        ctxIdx =  ctxIdxInc + prefix_ctxIdxOffset;
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        result_cur = result_cur << binIdx;
        prefix_result += result_cur;
    } while(IsIn_TU_binarization(prefix_result, 9 , binIdx) == -1);
    prefix_result = IsIn_TU_binarization(prefix_result, 9 , binIdx);
    //求后缀，旁路解码 
    uint8 binIdx2 = -1;
    result_cur = 0;
    uint32 result_str = 0;
    //如果前缀不足9位或者是一94位的截断数，那么没有后缀
    if((binIdx < 8 || (binIdx == 8 && prefix_result == 8))){suffix_result = 0;}
    else if(prefix_result == 9 )
    {
        do
        {
            binIdx2++;// sumi
            result_cur = (uint8_t)DecodeValueUsingCtxIdx(0, 1);
            result_str <<= 1;
            result_str += result_cur;
        } while((suffix_result = IsIn_UEGk_binarization(result_str, binIdx2, 0, 0, 3)) == -1);
    }
    int sig = 1;
    if(prefix_result != 0 && signedValFlag)
        sig = (DecodeValueUsingCtxIdx(0,1) == 1) ? -1 : 1;
    result = sig * (suffix_result + prefix_result);
    return result;
}


int cabac::IsIn_U_binarization(uint64 value, uint8 binIdx)
{
    uint32 a = 0;
    while((value & 0x1) == 1)
    {
        value >>= 1;
        a++;
    }
    if(binIdx == a) return binIdx; 
    else return -1;
}
int cabac::IsIn_TU_binarization(uint32 value, uint8 cMax, uint8 binIdx)
{
    int a = 0;
    if(binIdx + 1 == cMax) 
    {
        while ((value & 0x1) == 1)
        {
            value >>= 1;
            a++;
        }
        if(a == cMax || (a + 1 == cMax)) return a;
        else return -1;
    }
    else return IsIn_U_binarization(value, binIdx);
}
int cabac::IsIn_UEGk_binarization(uint32 value, uint8 binIdx, uint8  signedValFlag, uint8 uCoeff, uint8 k)
{
    uint8 length  = binIdx + 1;
    //求前缀的长度：
    uint8 prefix_length = uCoeff;
    uint8 prefix = 0;
    //求前缀
    if(prefix_length != 0)
    {
        for(uint8 i = 0; i < prefix_length; i++)
        {(prefix <<= 1) |= (value & (0x1 << i));}
        if(IsIn_TU_binarization(prefix, uCoeff, prefix_length) != -1)
        {
            prefix = IsIn_TU_binarization(prefix, uCoeff, prefix_length);
            if(prefix < uCoeff) return prefix;
        }
        else return -1;
    }

    //calc suffix of UEGK
    uint32 suffix = value >> prefix_length;
    //calc suffix length
    uint8 suffix_length = length - prefix_length;
    
    //data structure:  v1 + 0 + v2
    uint8 start_1_count = 0;
    uint32 count_1_pointer = 1 << (suffix_length - 1);
    uint8 sigLength = signedValFlag;
    uint32 v1 = 0;
    uint32 v2 = 0;
    for (uint8 i = 0; i < suffix_length; i++)
    {
        if(suffix & count_1_pointer)
        {
            count_1_pointer >>= 1;
            start_1_count++;
        }
        else break;
    }
    if(start_1_count + 1 + (start_1_count + k) + sigLength != suffix_length) return -1;
    else
    {
        v1 = (1 << (start_1_count)) - 1;
        v2 = suffix - (v1 << (1 + start_1_count + k));
        v1 <<= k;
        suffix = v1 + v2;
    }
    if(signedValFlag) suffix *= -1;
    else suffix *= 1;
    return prefix + suffix;
}


//binIdx maxBinIdxCtx ctxIdxOffset syntaxRequest
uint16 cabac::DecodeCtxIdxUsingBinIdx(uint16 binIdx, uint16 maxBinIdxCtx, int ctxIdxOffset, int syntaxRequest)
{
    uint16 ctxIdx_result, ctxIdxInc;
    uint8 condTermFlagA = 0;
    uint8 condTermFlagB = 0;
    macroblock* mb_cur = lifeTimeSlice->curMB;
    macroblock* mb_neiA = pic->neighbour_macroblock(mb_cur, 'A');
    macroblock* mb_neiB = pic->neighbour_macroblock(mb_cur, 'B');


    if(ctxIdxOffset == 70)
    { //upper
        if(!mb_neiB || ((mb_neiB && mb_neiB->is_avaiable(mb_cur)) || mb_neiB->mb_skip_flag == 0 )) condTermFlagB = 0;
        else
        {
            if(mb_neiB->mb_field_decoding_flag == 0) condTermFlagB = 0;
            else condTermFlagB  = 1;
        }
        //left
        if(!mb_neiA || ((mb_neiA && mb_neiA->is_avaiable(mb_cur)) || mb_neiA->mb_skip_flag == 0 )) condTermFlagA = 0;
        else
        {
            if(mb_neiA->mb_field_decoding_flag == 0) condTermFlagA = 0;
            else condTermFlagA  = 1;
        }
        ctxIdxInc = condTermFlagA + condTermFlagB;
    }
    //mb_type for i slice only
    else if(ctxIdxOffset == 3)
    {
        if(binIdx == 0)
        {
            if(!mb_neiB || ( mb_neiB && mb_neiB->is_avaiable(mb_cur) ) ) condTermFlagB = 0;
            else
            {
                if( (ctxIdxOffset == 0 && mb_neiB->type == SI_M) ||\
                    (ctxIdxOffset == 3 && mb_neiB->type == I_NxN)||\
                    (ctxIdxOffset == 27 && (mb_neiB->type == B_Skip|| mb_neiB->type == B_Direct_16x16))
                ) condTermFlagB  = 0;
                else condTermFlagB  = 1;
            }
            //left
            if(!mb_neiA || ( mb_neiA && mb_neiA->is_avaiable(mb_cur) ) ) condTermFlagA = 0;
            else
            {
                if( (ctxIdxOffset == 0 && mb_neiA->type == SI_M) ||\
                    (ctxIdxOffset == 3 && mb_neiA->type == I_NxN)||\
                    (ctxIdxOffset == 17 && (mb_neiA->type == B_Skip|| mb_neiA->type == B_Direct_16x16))
                ) condTermFlagA  = 0;
                else condTermFlagA  = 1;
            }
            ctxIdxInc = condTermFlagA + condTermFlagB;
        }
        else if(binIdx == 1) {return ctxIdx_result = 276;}
        else if(binIdx == 2 || binIdx == 3) ctxIdxInc = binIdx + 1;
        else if(binIdx == 4) ctxIdxInc = (b3 != 0) ? 5 : 6;
        else if(binIdx == 5) ctxIdxInc = (b3 != 0) ? 6 : 7;
        else ctxIdxInc = 7;
    }
    //coded_block_pattern------prefix(luma)
    else if(binIdx >= 0 && binIdx <= 3 && ctxIdxOffset == 73)
    {
        ctxIdxInc = condTermFlagA + 2 * condTermFlagB + ((binIdx == 1) ? 4 : 0);
    }
    //coded_block_pattern------suffix(Chroma)

    //mb_qp_delta
    else if(ctxIdxOffset == 60)
    {
        if(binIdx == 0)
        {
            macroblock* currentMB = lifeTimeSlice->curMB;
            macroblock*  prevMB = lifeTimeSlice->get_mbUsingIdInSlice(currentMB->idx_inslice - 1);
            if((prevMB == NULL || (prevMB->type == P_Skip || prevMB->type == B_Skip)) ||\
                prevMB->type == I_PCM||\
            (prevMB->premode != Intra_16x16 && (prevMB->CodedBlockPatternChroma == 0 && prevMB->CodedBlockPatternLuma == 0)) ||\
            prevMB->mb_qp_delta == 0
            )  ctxIdxInc = 0;
            else ctxIdxInc = 1;
        }
        else if(binIdx == 1) {ctxIdxInc = 2;}
        else {ctxIdxInc = 3;}
    }
    //ref idx l0 and l1
    else if(binIdx == 0 && ctxIdxOffset == 54)
    {

    }
    //intra_chroma_pred_mode 
    else if(ctxIdxOffset == 64)
    {

    }
    //coded_block_flag 
    //else if()
    // transform_size_8x8_flag 
    else if (ctxIdxOffset == 399)
    {
        if((mb_neiB == NULL || mb_neiB->is_avaiable(mb_cur))||\
            mb_neiB->transform_size_8x8_flag == 0\
          )condTermFlagB = 0;
        else condTermFlagB = 1;
        if((mb_neiA == NULL || mb_neiA->is_avaiable(mb_cur))||\
           mb_neiA->transform_size_8x8_flag == 0\
          )condTermFlagA = 0;
        else condTermFlagA = 1;
        ctxIdxInc = condTermFlagA + condTermFlagB;

    }

    ctxIdx_result = ctxIdxInc + ctxIdxOffset;
    return ctxIdx_result;
}








bool cabac::slice_end()
{
    delete ctxIdxOfInitVariables;
    if(state == 1) state = 0;

    return true;
}
bool cabac::set_pic(picture* pic1){this->pic = pic1; return true;}
bool cabac::set_slice(slice* sl){this->lifeTimeSlice = sl; return true;}

cabac::cabac(parser* parser)
{
    b3 = 0;
    b1 = 0;
    state = 0;
    this->p = parser;
}
cabac::~cabac()
{
    delete ctxIdxOfInitVariables;
}