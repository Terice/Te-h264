#include "macroblock.h"

#include "gvars.h"
#include "gchart.h"
#include "gfunc.h"

#include "terror.h"
#include "slice.h"
#include "parser.h"
#include "decoder.h"
#include "picture.h"
#include "pixmap.h"
#include "sps.h"
#include "pps.h"
#include "prediction.h"
#include "residual.h"
#include "matrix.h"

// 给所有要用的 mv 分配空间
// 如果没有 SubPart 的话， 传入NULL
extern int MallocMotionVectorPkg(MotionVector_info *m, int MbPart, SubMacroBlock* SubPart);
// 释放所分配的 mv 空间
// 如果没有 SubPart 的话， 传入NULL
extern int   FreeMotionVectorPkg(MotionVector_info *m, int MbPart, SubMacroBlock* SubPart);



macroblock::macroblock(slice* parent, parser* parser)
{
    this->sl = parent;
    this->pa = parser;
    this->pic = NULL;
    pred = NULL;
    resi = NULL;
    cons = NULL;
    inter = NULL;
    intra = NULL;


    //宏块的分区默认设置为只有一个，也就是整个宏块
    num_mb_part = 1;
}
macroblock::~macroblock()
{
    delete pred;
    delete resi;
    delete cons;
    delete re;
    if(intea_mode)
    {
        if(predmode == Intra_4x4) delete intra->predinfo.intra4x4;
        else if(predmode == Intra_8x8) delete intra->predinfo.intra8x8; 
    }
    else
    {
        FreeMotionVectorPkg(&inter->mv, num_mb_part,inter->sub);
        if(inter->sub && num_mb_part == 4) delete[] inter->sub;
        delete inter;
    }
}
void macroblock::attach(picture* p)
{
    // 链接到pic
    this->pic = p;
    // 链接之后pic根据其位置引导这个宏块, 找到其周围的环境
    pic->takein(this);
    // 链接到pic里面的数据, 同时设置数据元的宽度，以及原图的格式化方式
    pred = new matrix(16,16,0);
    resi = new matrix(16,16,0);
    cons = new pixmap(pic->cons->data, sizeof(byte) , pic->size);
    // 选择区域，这样就可以通过访问pixmap对象来修改picture中的数据了
    // pred->select(pos_sample_start, MACROBLOCK_SIZE);
    // resi->select(pos_sample_start, MACROBLOCK_SIZE);
    cons->select(pos_sample_start, MACROBLOCK_SIZE);

    // 这样宏块说他找到了
    return;
}

// 在宏块开始解码之前，已经获得了： pos（坐标） mb_skip_flag
// 此时应该已经链接到了pic上了
void macroblock::decode()
{
    ParseHead();
    re = new residual(this, pa);
    re->decode();
    // 解析残差的时候就已经可以解码出来残差(所有的残差)了，
    // 所以说解码 4x4 时重建图像拿到小块就可以重建图像了
    DecodeData();
    // 解码到这里就结束了
    // 呼～～，结束了
}

void macroblock::Parse_Sub(int* noSubMbPartSizeLessThan8x8Flag)
{
    //----------------------------------------------------子宏块预测

    /*  Part 长这样
        macroblock:
            |-------|
            | 1 | 2 |
            |---+---|
            | 3 | 4 |
            |-------|
        每一个Part还可以继续分，
        比如上面的宏块放大，可以分成这样
        Part
            -----------------
            |   |   |   1   |
            | 1 | 2 |-------|<----这是 有两个8x4subpart 的 part
            |   |   |   2   |
            -----------------
            | 1 | 2 |       |
            |---|---|   1   |<----这是一个 8x8 的part
            | 3 | 4 |       |
            -----------------
                ^
                \
                 \----------这是有4个4x4subpart 的 part
        但是对于每一个part分出来的subpart，只是运动矢量不一样，
        而 ref_idx_lx 和 predFlag 都是一样的
    */

    uint8_t mbPartIdx, subMbPartIdx, compIdx, subMbPartIdexInchart_curIdx;
    uint8_t i;

    inter = new Inter_pred;
    // 宏块数据初始化
    inter->sub = new SubMacroBlock[4];

    // 把帧间需要的信息链接起来，
    SubMacroBlock *sub = inter->sub;
    bool *predFlagL0 = inter->predFlagL0;
    bool *predFlagL1 = inter->predFlagL1;
    int8 *ref_idx_l0 = inter->ref_idx_l0;
    int8 *ref_idx_l1 = inter->ref_idx_l1;


    // 读取part的类型，
    // 然后计算出来每个part的预测模式
    // 然后读取part的 分块数
    uint8_t sub_type = 0;
    for(mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
    {
        sub_type = pa->read_ae(0x00051000U);

        if(type >= 50 && type <= 74) sub_type += 10;//+= 10是在枚举类型中加上B块的偏移

        sub[mbPartIdx].type = (type_submacroblock)sub_type;
        sub[mbPartIdx].predmode = SubMbPartPredMode(sub[mbPartIdx].type); // 直接计算出来预测模式
        sub[mbPartIdx].part = SubNumMbPart(sub[mbPartIdx].type);          // 计算part的分块数
    }
    
    // 读取预测标志位
    for(mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
    {
        predFlagL0[mbPartIdx] = PredFlag(this, mbPartIdx, 0);
        if(sl->type == B)
            predFlagL1[mbPartIdx] = PredFlag(this, mbPartIdx, 1);
    }

    // 读取参考帧索引
    for(mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
        if((sl->ps->num_ref_idx_l0_active_minus1 > 0 || sl->ps->mb_field_decoding_flag != sl->ps->field_pic_flag) &&\
        sub[mbPartIdx].type != B_Direct_8x8 && \
        type != P_8x8ref0 && \
        sub[mbPartIdx].predmode != Pred_L1)
            ref_idx_l0[mbPartIdx]                               = pa->read_ae(0x00041000U | ((uint32)mbPartIdx << 8) | (0U << 4) | 0U);
        else ref_idx_l0[mbPartIdx] = 0;
    for(mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
        if((sl->ps->num_ref_idx_l1_active_minus1 > 0 || sl->ps->mb_field_decoding_flag != sl->ps->field_pic_flag) &&\
        sub[mbPartIdx].type != B_Direct_8x8 &&\
        sub[mbPartIdx].predmode != Pred_L0)
            ref_idx_l1[mbPartIdx]                               = pa->read_ae(0x00041000U | ((uint32)mbPartIdx << 8) | (0U << 4) | 1U);
        else ref_idx_l1[mbPartIdx] = 0;
    //
    
    // 运动矢量的读取
    MallocMotionVectorPkg(&this->inter->mv, 4, inter->sub);
    MotionVector **mvp_l0 = inter->mv.mvp_l0;
    MotionVector **mvp_l1 = inter->mv.mvp_l1;
    MotionVector **mvd_l0 = inter->mv.mvd_l0;
    MotionVector **mvd_l1 = inter->mv.mvd_l1;
    MotionVector **mv_l0  = inter->mv.mv_l0 ;
    MotionVector **mv_l1  = inter->mv.mv_l1 ;
    for(mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
    {
        subMbPartIdexInchart_curIdx = sub[mbPartIdx].part;
        if(sub[mbPartIdx].type != B_Direct_8x8 && sub->predmode!= Pred_L1) 
            for(subMbPartIdx = 0; subMbPartIdx < subMbPartIdexInchart_curIdx/*(subMbPartCount = subMbInfo[subMbPartIdexInchart_curIdx][1])*/; subMbPartIdx++)
            {
                // for(compIdx = 0; compIdx < 2; compIdx++)
                // {
                    mvd_l1[mbPartIdx][subMbPartIdx][0] =  pa->read_ae(0x00043000U | ((uint32)mbPartIdx << 8) | (subMbPartIdx << 4)  | (0U << 1) | 0U);
                    mvd_l1[mbPartIdx][subMbPartIdx][1] =  pa->read_ae(0x00043000U | ((uint32)mbPartIdx << 8) | (subMbPartIdx << 4)  | (1U << 1) | 0U);
                // }
                // pic->get_MvNeighbour(this, mbPartIdx, subMbPartIdx, 0, mvp_l0);
                if(terr.inter_movevector())
                {
                    printf(">>mvd_l0: (%d, %d) in 4x4sub\n", mvd_l0[mbPartIdx][subMbPartIdx][0], mvd_l0[mbPartIdx][subMbPartIdx][1]);
                    printf(">>mv_l0 : (%d, %d) in 4x4sub\n", mv_l0[mbPartIdx][subMbPartIdx][0], mv_l0[mbPartIdx][subMbPartIdx][1]);
                }

                // 读取到mvp和mvd就可以计算出来mv了
                mv_l0[mbPartIdx][subMbPartIdx][0] += mvd_l0[mbPartIdx][subMbPartIdx][0];
                mv_l0[mbPartIdx][subMbPartIdx][1] += mvd_l0[mbPartIdx][subMbPartIdx][1];
            }
        else if(sub[mbPartIdx].type == B_Direct_8x8)
        {
            for(uint8_t subMbPartIdx = 0; subMbPartIdx < 4; subMbPartIdx++)
            {
                Prediction_Inter_Direct(mbPartIdx, subMbPartIdx);
            }
        }
        
    }
    if(sl->type == B)
        for(mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
        {
            subMbPartIdexInchart_curIdx = sub[mbPartIdx].part;
            if(sub[mbPartIdx].type != B_Direct_8x8 && sub[mbPartIdx].predmode != Pred_L0)
                for(subMbPartIdx = 0; subMbPartIdx < subMbPartIdexInchart_curIdx/*(subMbPartCount = subMbInfo[subMbPartIdexInchart_curIdx][1])*/; subMbPartIdx++)
                {
                    
                    // for(compIdx = 0; compIdx < 2; compIdx++) 
                    // {
                        mvd_l1[mbPartIdx][subMbPartIdx][0] =  pa->read_ae(0x00043000U | ((uint32)mbPartIdx << 8) | (subMbPartIdx << 4)  | (0U << 1) | 1U);
                        mvd_l1[mbPartIdx][subMbPartIdx][1] =  pa->read_ae(0x00043000U | ((uint32)mbPartIdx << 8) | (subMbPartIdx << 4)  | (1U << 1) | 1U);
                    // }
                    if(terr.inter_movevector())
                    {
                        printf(">>mvd_l1: (%d, %d) in 4x4sub\n", mvd_l1[mbPartIdx][subMbPartIdx][0], mvd_l1[mbPartIdx][subMbPartIdx][1]);
                        printf(">>mv_l1 : (%d, %d) in 4x4sub\n", mv_l1[mbPartIdx][subMbPartIdx][0], mv_l1[mbPartIdx][subMbPartIdx][1]);
                    }

                    // 读取到mvp和mvd就可以计算出来mv了
                    mv_l1[mbPartIdx][subMbPartIdx][0] += mvd_l1[mbPartIdx][subMbPartIdx][0];
                    mv_l1[mbPartIdx][subMbPartIdx][1] += mvd_l1[mbPartIdx][subMbPartIdx][1];

                    // pic->get_MvNeighbour(this, mbPartIdx, subMbPartIdx, 1, mvp_l1);
                }
            else if(sub[mbPartIdx].type == B_Direct_8x8)
            {
                for(uint8_t subMbPartIdx = 0; subMbPartIdx < 4; subMbPartIdx++)
                {
                    Prediction_Inter_Direct(mbPartIdx, subMbPartIdx);
                } 
            }
    }
    //--------------------------------------------------------子宏块预测结束
    // 检查是最小分区是不是8x8
    for(uint8_t mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
    {
        if(sub[mbPartIdx].type != B_Direct_8x8)
        { 
            if(subMbInfo[sub[mbPartIdx].type][1] > 1) 
                *noSubMbPartSizeLessThan8x8Flag = 0;
        }
        else if(!pa->pS->sps->direct_8x8_inference_flag) *noSubMbPartSizeLessThan8x8Flag = 0;
    }
}
void macroblock::Parse_Intra()
{
    intra = new Intra_pred;
    // 4x4 每块的预测模式的初始推导变量
    if(predmode == Intra_4x4)
    {
        Intra4x4_pred_info *i = new Intra4x4_pred_info;
        intra->predinfo.intra4x4 = i;

        i->index = 0; // 初始化第一个要解码的块为0
        uint8 *rem_intra4x4_pred_mode       = i->rem_intra4x4_pred_mode;
        uint8 *prev_intra4x4_pred_mode_flag = i->prev_intra4x4_pred_mode_flag;
        for(int8 luma4x4BlkIdx=0; luma4x4BlkIdx<16; luma4x4BlkIdx++) 
        { 
            prev_intra4x4_pred_mode_flag[luma4x4BlkIdx] = pa->read_ae(0x00031000U);
            if(!prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]) 
                rem_intra4x4_pred_mode[luma4x4BlkIdx] = pa->read_ae(0x00032000U);
        }
    }
    // 8x8 每块的预测模式的初始推导变量
    if(predmode == Intra_8x8)
    {
        Intra8x8_pred_info *i = new Intra8x8_pred_info;
        intra->predinfo.intra8x8 = i;

        uint8_t* prev_intra8x8_pred_mode_flag = i->prev_intra8x8_pred_mode_flag;
        uint8_t* rem_intra8x8_pred_mode       = i->rem_intra8x8_pred_mode;
        for(int8 luma8x8BlkIdx=0; luma8x8BlkIdx < 4; luma8x8BlkIdx++)
        { 
            prev_intra8x8_pred_mode_flag[luma8x8BlkIdx];
            if(!prev_intra8x8_pred_mode_flag[luma8x8BlkIdx]){
                rem_intra8x8_pred_mode[luma8x8BlkIdx];
            }
        }
    }
    // 16x16 根据表直接推算出来预测方式
    if(predmode == Intra_16x16)
        intra->predinfo.intra16x16 = macroBlockInfo_I_slice[mb_type][2];
    //
}
void macroblock::Parse_Skip()
{
    // 之前就已经推测出来mb_type了
    // P_Skip 宏块的运动矢量来自周围的宏块
    
    // B_Skip 宏块的运动矢量来自Direct预测（没错就是 Direct 推测）

    num_mb_part = 4; // Skip宏块必然是4分
    // 因为运动矢量一定是四块

    // Skip 宏块必定是帧间宏块
    inter = new Inter_pred;

    return ;
}
void macroblock::Parse_Direct()
{

}
void macroblock::Parse_Inter()
{

    inter = new Inter_pred;
    inter->sub = NULL;

    int8 *ref_idx_l0 = inter->ref_idx_l0;
    int8 *ref_idx_l1 = inter->ref_idx_l1;
    bool *predFlagL0 = inter->predFlagL0;
    bool *predFlagL1  = inter->predFlagL1;

    MallocMotionVectorPkg(&inter->mv, num_mb_part, NULL);

    MotionVector **mvd_l0       = inter->mv.mvd_l0;
    MotionVector **mvd_l1       = inter->mv.mvd_l1;
    MotionVector **mvp_l0       = inter->mv.mvp_l0;
    MotionVector **mvp_l1       = inter->mv.mvp_l1;
    MotionVector **mv_l0        = inter->mv.mv_l0 ;
    MotionVector **mv_l1        = inter->mv.mv_l1 ;
        

    uint8_t mbPartIdx;
    //读取预测标志位
    for(mbPartIdx = 0; mbPartIdx < num_mb_part; mbPartIdx++)
    {
        predFlagL0[mbPartIdx] = PredFlag(this, mbPartIdx, 0);
        if(sl->type == B)
        predFlagL1[mbPartIdx] = PredFlag(this, mbPartIdx, 1);
    }
    for(mbPartIdx = 0; mbPartIdx < num_mb_part; mbPartIdx++) 
        if((sl->ps->num_ref_idx_l0_active_minus1 > 0 || \
        sl->ps->mb_field_decoding_flag != sl->ps->field_pic_flag) && \
        MbPartPredMode(type, mbPartIdx) != Pred_L1)
            ref_idx_l0[mbPartIdx]  = pa->read_ae(0x00041000U | ((uint32)mbPartIdx << 8) | (0U << 4) + 0U); 
        else ref_idx_l0[mbPartIdx] = 0;
    for(mbPartIdx = 0; mbPartIdx < num_mb_part; mbPartIdx++) 
        if((sl->ps->num_ref_idx_l1_active_minus1 > 0 || \
        sl->ps->mb_field_decoding_flag != sl->ps->field_pic_flag) && 
        MbPartPredMode(type, mbPartIdx) != Pred_L0)
            ref_idx_l1[mbPartIdx]  = pa->read_ae(0x00041000U | ((uint32)mbPartIdx << 8) | (0U << 4) + 1U);
        else ref_idx_l1[mbPartIdx] = 0;
    for(mbPartIdx = 0; mbPartIdx < num_mb_part; mbPartIdx++)
    {
        predmode = MbPartPredMode(type, mbPartIdx);
        if(predmode != Pred_L1 && predmode != Pred_NU)
        {
            mvd_l0[mbPartIdx][0][0] = pa->read_ae(0x00043000U | ((uint32)mbPartIdx << 8) | (0U << 4)  | (0U << 1) | 0U); 
            mvd_l0[mbPartIdx][0][1] = pa->read_ae(0x00043000U | ((uint32)mbPartIdx << 8) | (0U << 4)  | (1U << 1) | 0U); 
            pic->neighbour_motionvector(this, mbPartIdx, 0, 0, mvp_l0);
            mv_l0[mbPartIdx][0][0] = mvp_l0[mbPartIdx][0][0] +  mvd_l0[mbPartIdx][0][0];
            mv_l0[mbPartIdx][0][1] = mvp_l0[mbPartIdx][0][1] +  mvd_l0[mbPartIdx][0][1];
            if(terr.inter_movevector())
            {
                printf(">>mvd_l0: (%d, %d)\n", mvd_l0[mbPartIdx][0][0], mvd_l0[mbPartIdx][0][1]);
                printf(">>mvp_l0: (%d, %d)\n", mvp_l0[mbPartIdx][0][0], mvp_l0[mbPartIdx][0][1]);
                printf(">>mv_l0 : (%d, %d)\n",  mv_l0[mbPartIdx][0][0],  mv_l0[mbPartIdx][0][1]);
            }
        }
    }
    if(sl->type == B)
    for(mbPartIdx = 0; mbPartIdx < num_mb_part; mbPartIdx++)
    {
        predmode = MbPartPredMode( type, mbPartIdx);
        if(predmode != Pred_L0 && predmode != Pred_NU)
        {
            mvd_l1[mbPartIdx][0][0] = pa->read_ae(0x00043000U | ((uint32)mbPartIdx << 8) | (0U << 4)  | (0U << 1) | 1);
            mvd_l1[mbPartIdx][0][1] = pa->read_ae(0x00043000U | ((uint32)mbPartIdx << 8) | (0U << 4)  | (1U << 1) | 1);
            pic->neighbour_motionvector(this, mbPartIdx, 0, 1, mvp_l1);
            mv_l1[mbPartIdx][0][0] = mvp_l1[mbPartIdx][0][0] + mvd_l1[mbPartIdx][0][0];
            mv_l1[mbPartIdx][0][1] = mvp_l1[mbPartIdx][0][1] + mvd_l1[mbPartIdx][0][1];
            if(terr.inter_movevector())
            {
                printf(">>mvd_l1: (%d, %d)\n", mvd_l1[mbPartIdx][0][0], mvd_l1[mbPartIdx][0][1]);
                printf(">>mvp_l1: (%d, %d)\n", mvp_l1[mbPartIdx][0][0], mvp_l1[mbPartIdx][0][1]);
                printf(">> mv_l1: (%d, %d)\n",  mv_l1[mbPartIdx][0][0],  mv_l1[mbPartIdx][0][1]);
            }
        }
    }
    
}
void macroblock::ParseHead()
{
    // I SI 宏块没有分块这一概念（表中就没有part这个列），所以4分块全是帧间预测
    // 
    // Skip宏块
    // I_PCM宏块
    // 非I_PCM 宏块
        // 4分块(帧间)
        // 小于4分的块
            // 帧内 (因为帧内没有分块的概念)
                // 8x8变换 flag 区分 I_NxN 
                // 4x4 预测模式读取 一共16个
                // 8x8 预测模式读取 一共4个
                // 16x16 的预测模式由mb_type 推导
                // 帧内色度块预测模式
            // 帧间 但不是 Direct
                // 参考索引
                // 预测flag
                // 运动矢量前缀
                // 运动矢量偏移
        // Direct块（B_Direct_16x16）
            // Direct 预测(也是帧间)
        
        // 色彩编码模式（是否编码）
        // 非intra16x16块用句法元素读取
        // intra16x16 块用色彩系数查表得到
    //

    // 需要注意的是 Skip 宏块的分块都是 4，但是却不能用num_mb_part == 4 （在下面） 的方法
    // 因为 Skip 从 slice 读到 mb_skip_flag 开始就没有数据了，
    // 数据都是推测出来的结果，所以在读取之前就必须要做判断，
    // 之前是单独分开了一个 Init0 函数单独处理 Skip，但是总觉得不妥，所以还是放到这里来了
    if(mb_skip_flag)  // Skip宏块的模式推导（本来就是需要推导的）
    {
        if(sl->type == B) 
        {
            type = B_Skip;
            predmode = Direct;
        }
        else /*if(sl->type == P)*/ 
        {
            type = P_Skip;
            predmode = Pred_L0;
        }
        // 注意 Skip 宏块是没有残差的（全部都是0）
        // 色度组件的解码模式还没有学-，-
        Parse_Skip();// Skip 运动矢量的计算
        // 由于之后已经没有数据了，这里执行完毕就返回
        return;// Skip 宏块结束 到这里就已经没有数据了，所以直接返回
    }


    mb_type = pa->read_ae(0x00021000U);
    // 为了要知道当前宏块的类型，需要在 ae 算子中加上偏移值来表明当前宏块的具体类型
    // ae 算子在计算这个句法的时候是能够知道具体的类型的，
    // 所以需要一次转化， 把枚举值转换出来
    type = transtype(mb_type);

    predmode = Pred_NU;// 帧内的 predmode 在下面计算
    intea_mode = 1; // 应该根据type来判断这个宏块是帧内还是帧间
    num_mb_part = MbPartNum(type);
    
    if(type == I_PCM) // PCM 宏块直接按照 固定的方法读取
    {
        for(unsigned i = 0; i < 256; i++) // 16 * 16
            pa->read_un(pa->pV->BitDepthY);
        for(unsigned i = 0; i < 128; i++) // 2 * 8 * 8
            pa->read_un(pa->pV->BitDepthC);
        return ;// PCM 宏块结束 返回
    }

    int noSubMbPartSizeLessThan8x8Flag = 1;
    // 前面已经说过，如果分块为4,那么一定是帧间，
    // 而帧内对于分块没有概念，所以对于帧内，分块的值一律设定为1
    if(num_mb_part == 4) // 所有含有sub块的的帧间块
    {
        Parse_Sub(&noSubMbPartSizeLessThan8x8Flag);

    }
    else if(num_mb_part < 4) // 包括所有的帧内宏块， 没有sub块的帧间块 
    {
        // 读取 transform_size_8x8_flag 并且判断 I_NxN 的类型
        if(type == I_NxN && pa->pS->pps->transform_8x8_mode_flag)
            transform_size_8x8_flag = pa->read_ae(0x00022000U);
        else //transform_size_8x8_flag 默认是设置为 0 的
            transform_size_8x8_flag = 0;
        if(type == I_NxN) // I_NxN 计算 predmode
            predmode = transform_size_8x8_flag ? Intra_4x4 : Intra_8x8;
        else // 不是I_NxN
        {
            // 却又是帧内，那么只能是Intra_16x16了
            if(is_intrapred()) predmode = Intra_16x16; 
            // 
        }
        if(is_intrapred())
        {
            // 在进行这个函数之前，要先解析完成predmode这个变量
            Parse_Intra();
            // 除了要完成初始化之外，还要读取色度的预测模式
            if(pa->pV->ChromaArrayType == 1 || pa->pV->ChromaArrayType == 2) 
                intra->intra_chroma_pred_mode = pa->read_ae(0x00035000U);//ue|ae
        }
        else // 不是帧内
        {
            if(predmode != Direct) // 不是 Direct
            {
                // 也就是普通的帧间宏块（非direct和非skip, 没有子分块）
                // 需要读取的变量：mvp， mvd， ref_idx_lx, predFlag
                Parse_Inter();
            }
        }
    }
    else // Direct 预测 Direct没有 mvd，没有ref_idx_Lx，但是可以有残差
    {
        // 8x8Direct在上面的num_mb_part == 4里面而不是这里
        // B_Skip P_Skip 早就已经返回了

        // 所以这里就只有 B_Direct_16x16
        Parse_Direct();
    }

    // 读取色彩编码模式（不是预测模式）
    if(predmode != Intra_16x16)
    {
        coded_block_pattern = pa->read_ae(0x00023000U);
        CodedBlockPatternLuma = coded_block_pattern % 16;
        CodedBlockPatternChroma = coded_block_pattern / 16;
        
        // 对于一些帧间的宏块，也可能有 transform_size_8x8_flag 这个句法
        if(CodedBlockPatternLuma > 0 &&\
        pa->pS->pps->transform_8x8_mode_flag && type != I_NxN &&\
        noSubMbPartSizeLessThan8x8Flag &&\
        (type != B_Direct_16x16 || pa->pS->sps->direct_8x8_inference_flag))
        {
            transform_size_8x8_flag = pa->read_ae(0x00022000U);
        }
    }
    else        //帧内16x16的色彩系数编码模式由查表得到，
    {
        coded_block_pattern = 1;
        CodedBlockPatternChroma = macroBlockInfo_I_slice[mb_type][3];
        CodedBlockPatternLuma   = macroBlockInfo_I_slice[mb_type][4];
    }
    
    //残差读不读是由 code_block_pattern 来决定的
}
void macroblock::DecodeData()
{
    // 准备把这里这里写的函数改成直接到最终的图像
    if(is_intrapred())
    {
        switch (predmode)
        {
        case Intra_16x16:Decode_Intra16x16(); break;
        case Intra_8x8  :Decode_Intra8x8()  ; break;
        case Intra_4x4  :Decode_Intra4x4()  ; break;
        default:terr.error("macroblock, no such intra decode"); break;
        }
    }
    else
    {
        Decode_Inter();
    }
    return ;    
}

void macroblock::Decode_Intra4x4()
{
    int8 *index = &(intra->predinfo.intra4x4->index);
    
    int luma4x4Index = block4x4Index[*index];
    //如果是第一个4x4块，那么先解析整个宏块的预测模式
    ParseIntra4x4PredMode();
    while(*index < 16)
    {
        switch (intra->predinfo.intra4x4->Intra4x4PredMode[*index])
        {
            case 0:Prediction_Intra4x4_V                   (this); break;
            case 1:Prediction_Intra4x4_H                   (this); break;
            case 2:Prediction_Intra4x4_DC                  (this); break;
            case 3:Prediction_Intra4x4_Diagonal_Down_Left  (this); break;
            case 4:Prediction_Intra4x4_Diagonal_Down_Right (this); break;
            case 5:Prediction_Intra4x4_V_Right             (this); break;
            case 6:Prediction_Intra4x4_H_Down              (this); break;
            case 7:Prediction_Intra4x4_V_Left              (this); break;
            case 8:Prediction_Intra4x4_H_Up                (this); break;
            default:terr.error("no such intra4x4 func");           break;
        }
        (*index)++;
    }
}
void macroblock::Decode_Intra8x8(){}
void macroblock::Decode_Intra16x16()
{
    switch (intra->predinfo.intra16x16)
    {
        case 0:Prediction_Intra16x16_V         (this); break;
        case 1:Prediction_Intra16x16_H         (this); break;
        case 2:Prediction_Intra16x16_DC        (this); break;
        case 3:Prediction_Intra16x16_Plane     (this); break;
        default:terr.error("no such intra16x16 func"); break;
    }
}
void macroblock::Decode_Inter()
{
    //如果不是16x16直接预测 并且 不是B_Skip宏块
    type_slice slice_type = sl->type;
    // if(type != B_Direct_16x16 && type != B_Skip) <- 这句话其实是用来说明运动矢量的来源的，解码里面其实可以直接运算
    {
        //获取宏块的子块数量
        uint8 mbPart = 0;
        uint8 width  = 0;
        uint8 height = 0;
        if(type != B_Direct_16x16 && type != B_Skip)
        {
            mbPart = num_mb_part;
            width  = MbPartWidth (type);
            height = MbPartHeight(type);
        }
        else // 本来 B_Skip 和 
        {
            mbPart = 4;
            width = 8;
            height = 8;
        }
        
        int8     *ref_idx_l0 = inter->ref_idx_l0;
        int8     *ref_idx_l1 = inter->ref_idx_l1;
        bool     *predFlagL0 = inter->predFlagL0;
        bool     *predFlagL1 = inter->predFlagL1;
        MotionVector **mv_l0 = inter->mv.mv_l0;
        MotionVector **mv_l1 = inter->mv.mv_l1;

        //子块 的 循环，
        for (uint8_t mbPartIdx = 0; mbPartIdx < mbPart; mbPartIdx++)
        {
            int refidx_l0 = ref_idx_l0?ref_idx_l0[mbPartIdx]:0;
            int refidx_l1 = ref_idx_l1?ref_idx_l1[mbPartIdx]:0;

            bool predFlag_0 = predFlagL0[mbPartIdx];
            bool predFlag_1 = slice_type == B ? predFlagL1[mbPartIdx]:false;

            //如果有子子块，那么循环的最大值增加为子子块的数量，否则为1（当前子块）
            uint8_t subMbPart;
            if(type != B_Direct_16x16 && type != B_Skip)
            {
                if(num_mb_part == 4)
                {
                    if(inter->sub[mbPartIdx].type == B_Direct_8x8)
                        subMbPart = 4;
                    else
                        subMbPart = SubNumMbPart(inter->sub[mbPartIdx].type);
                }
                else
                {
                    subMbPart = 1;
                }
                
            }
            else//B_Direct_16x16 和 B_Skip 不会有子子块类型这个属性，子子块都是直接推导出本身的数据
            // 因为 B_Direct_16x16 和 B_Skip 是分成四块， 每一块都不再分
                subMbPart = 1;

            for (uint8_t subPartIdx = 0; subPartIdx < subMbPart; subPartIdx++)
            {
                //如果是4分块，那么把宽高都 更新 为子子块的宽和高
                if(num_mb_part == 4 && type != B_Direct_16x16 && type != B_Skip)
                {
                    width  = SubMbPartWidth(inter->sub[mbPartIdx].type);
                    height = SubMbPartHeight(inter->sub[mbPartIdx].type);
                }
                matrix tmp_0(height, width, 0), tmp_1(height, width, 0);
                //如果有前向预测，
                if(predFlag_0)
                {
                    if(!de->list_Ref0[ref_idx_l0[mbPartIdx]]) 
                    {
                        printf("slice :%d, macro:%d, %d, refidx:%d\n", sl->index, pos.x, pos.y, ref_idx_l0[mbPartIdx]);
                        de->print_list();
                        terr.error("ref index is NULL");
                    }
                    //选择参考帧，
                    picture* ref_pic_0 = de->list_Ref0[ref_idx_l0[mbPartIdx]];
                    //帧内预测
                    Prediction_Inter(tmp_0 ,mbPartIdx, subPartIdx, width, height, ref_pic_0, mv_l0,1);
                    
                }
                //如果有后向预测
                if(predFlag_1)
                {
                    if(!de->list_Ref0[ref_idx_l1[mbPartIdx]]) 
                    {
                        printf("slice :%d, macro:%d, %d, refidx:%d\n", sl->index, pos.x, pos.y, ref_idx_l1[mbPartIdx]);
                        de->print_list();
                        terr.error("ref index is NULL");
                    }
                    picture* ref_pic_1 = de->list_Ref1[ref_idx_l1[mbPartIdx]];
                    Prediction_Inter(tmp_1 ,mbPartIdx, subPartIdx, width, height, ref_pic_1, mv_l1,1);
                }

                //加权预测
                uint8_t weighted_pred_flag  = sl->ps->pps->weighted_pred_flag;
                uint8_t weighted_bipred_idc = sl->ps->pps->weighted_bipred_idc;

                matrix out(height, width, 0);
                type_slice sltype = sl->type;
                if(sltype == P || sltype == SP)
                {
                    if(weighted_pred_flag) 
                    {
                        //显式加权
                        Weight_CoefficWeight(true, tmp_0, tmp_1, out, refidx_l0, refidx_l1, predFlag_0, predFlag_1);
                    }
                    else
                    {
                        //默认加权
                        Weight_defaultWeight(tmp_0, tmp_1, out, predFlag_0, predFlag_1);
                    }
                    
                }
                else //if(sltype == B)
                {
                    if(weighted_bipred_idc == 0)
                    {
                        //默认加权
                        Weight_defaultWeight(tmp_0, tmp_1, out, predFlag_0, predFlag_1);
                    }
                    else if(weighted_bipred_idc == 1)
                    {
                        //显式加权
                        Weight_CoefficWeight(true, tmp_0, tmp_1, out, refidx_l0, refidx_l1, predFlag_0, predFlag_1);
                    }
                    //B片的额外的加权预测
                    else //if(weighted_bipred_idc == 2)
                    {
                        if(predFlag_1 && predFlag_0)
                        {
                            //隐式加权
                            Weight_CoefficWeight(false, tmp_0, tmp_1, out, refidx_l0, refidx_l1, predFlag_0, predFlag_1);
                        }
                        else
                        {
                            //默认加权
                            Weight_defaultWeight(tmp_0, tmp_1, out, predFlag_0, predFlag_1);
                        }
                    }
                }
                //赋值
                int mb_partWidth = MbPartWidth(type);
                int mb_partHeigh = MbPartHeight(type);
                int sub_mb_partWidth = 8;
                int sub_mb_partHeigh = 8;
                int xS = 0;
                int yS = 0;
                if(type != B_Direct_16x16 && type != B_Skip)
                {
                    if(this->num_mb_part == 4)
                    {
                        sub_mb_partWidth = SubMbPartWidth (inter->sub[mbPartIdx].type);
                        sub_mb_partHeigh = SubMbPartHeight(inter->sub[mbPartIdx].type);
                    }
                    xS = (mbPartIdx % (16 / mb_partWidth)) * mb_partWidth + (num_mb_part == 4 ? \
                            (subPartIdx% (8  / sub_mb_partWidth) * sub_mb_partWidth) : 0);
                    yS = (mbPartIdx / (16 / mb_partWidth)) * mb_partHeigh + (num_mb_part == 4 ?\
                            (subPartIdx/ (8  / sub_mb_partWidth) * sub_mb_partWidth) : 0);
                }
                else
                {
                    xS = (mbPartIdx % (16 / mb_partWidth)) * mb_partWidth;                                
                    yS = (mbPartIdx / (16 / mb_partWidth)) * mb_partHeigh;
                }
                if(type == B_8x8)
                    int a = 0;
                for(uint8_t yL = 0; yL < height; yL++)
                {
                    for(uint8_t xL = 0; xL < width; xL++)
                    {
                        //到预测样点矩阵中，
                        // (*pred_Y)[yS + yL][xS + xL] += out[yL][xL];
                        // 注意矩阵的运算，y 对应的才是 row
                        pred[0][yS + yL][xS + xL] = out[yL][xL];
                        
                    }
                }
            }
        }
    }
    // else //if(type == B_Direct_16x16 || type == B_Skip)
    // {
        
    // }
    
    // }
}
bool macroblock::is_avaiable(macroblock *mb_N)
{
    if(mb_N == NULL) return false;
    if(this->idx_inpicture < mb_N->idx_inpicture ||\
       this->idx_slice != mb_N->idx_slice) return false;
    else return true;
}

type_macroblock macroblock::transtype(int mb_type)
{
    //去掉枚举类型的偏移得到真实的值
    int type = 0;
    if(sl->type == I) type = mb_type;
    else if(sl->type == P)
    {
        // 是 P Slice 中的 P 宏块
        if(mb_type < 5) type = TYPE_MACROBLOCK_START_INDEX_P + mb_type;
        else type -= 5; // 是 P slice 中的 I 宏块， 减去 I 在 P 中 的前缀
    }
    else// if(sl->type === B) // 是 B slice
    {
        if(mb_type < 23) type = TYPE_MACROBLOCK_START_INDEX_B + mb_type;
        else type -= 23; // 是 B slice 中的 I 宏块， 减去 I 在 B 中 的前缀
    }
    return (type_macroblock)type;
}
uint8_t macroblock::get_PosByIndex(int index, int& r, int& c)
{
    uint8_t index_Raster = 0;
    index_Raster = block4x4Index[index];
    r = (index_Raster / 4) * 4;
    c = (index_Raster % 4) * 4;
    return index_Raster;
}

void macroblock::ParseIntra4x4PredMode()
{
    int  luma4x4BlkIdxA,  luma4x4BlkIdxB;
    uint8  intraMxMPredModeA,  intraMxMPredModeB;
    macroblock* A, * B;
    uint8  dcPredModePredictedFlag  = 0;
    uint8 *Intra4x4PredMode = intra->predinfo.intra4x4->Intra4x4PredMode;
    uint8 *rem_intra4x4_pred_mode = intra->predinfo.intra4x4->rem_intra4x4_pred_mode;
    uint8 *prev_intra4x4_pred_mode_flag = intra->predinfo.intra4x4->prev_intra4x4_pred_mode_flag;
    for (uint8_t i = 0; i < 16; i++)
    {
        luma4x4BlkIdxA = 0;
        pic->neighbour_4x4block(this, i, 'A', &A, &luma4x4BlkIdxA);
        luma4x4BlkIdxB = 0;
        pic->neighbour_4x4block(this, i, 'B', &B, &luma4x4BlkIdxB);
        if(!this->is_avaiable(A) || !this->is_avaiable(B) ||\
           (A->is_interpred() && 1) ||\
           (B->is_interpred() && 1))
            dcPredModePredictedFlag = 1;
        else dcPredModePredictedFlag = 0;

        if(dcPredModePredictedFlag == 1 || (A->predmode != Intra_8x8 && A->predmode != Intra_4x4)) intraMxMPredModeA = 2;
        else 
        {
            if(A->predmode == Intra_4x4) intraMxMPredModeA = A->intra->predinfo.intra4x4->Intra4x4PredMode[luma4x4BlkIdxA];
            else  intraMxMPredModeA = A->intra->predinfo.intra8x8->Intra8x8PredMode[luma4x4BlkIdxA >> 2];
        }
        if(dcPredModePredictedFlag == 1 || (B->predmode != Intra_8x8 && B->predmode != Intra_4x4)) intraMxMPredModeB = 2;
        else 
        {
            if(B->predmode == Intra_4x4) intraMxMPredModeB = B->intra->predinfo.intra4x4->Intra4x4PredMode[luma4x4BlkIdxB];
            else  intraMxMPredModeB = B->intra->predinfo.intra8x8->Intra8x8PredMode[luma4x4BlkIdxB >> 2];
        }

        uint8_t predIntra4x4PredMode;
        predIntra4x4PredMode = Min(intraMxMPredModeA, intraMxMPredModeB);
        if(prev_intra4x4_pred_mode_flag[i]) Intra4x4PredMode[i] = predIntra4x4PredMode;
        else 
        {
            if(rem_intra4x4_pred_mode[i] < predIntra4x4PredMode) Intra4x4PredMode[i] = rem_intra4x4_pred_mode[i];
            else Intra4x4PredMode[i] = rem_intra4x4_pred_mode[i] + 1;
        }
    }
    // Sdelete_l(rem_intra4x4_pred_mode);
    // Sdelete_l(prev_intra4x4_pred_mode_flag);
}