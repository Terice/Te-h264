#include "slice.h"

#include <cmath>
#include "parser.h"
#include "decoder.h"
#include "nal.h"
#include "cabac.h"
#include "picture.h"
#include "macroblock.h"
#include "pps.h"
#include "sps.h"
#include <vector>
#include "gfunc.h"

#include "gvars.h"
#include "terror.h"

uint32 NextMbAddress(uint32 add){return 0;}
uint32 more_rbsp_data(){return 1;}

static bool free_pw(\
PredWeight* pw, \
int num_ref_idx_l0_active_minus1, \
int num_ref_idx_l1_active_minus1);

slice::slice(parser* p, decoder* d, picture* pic)
{
    next = NULL;
    hedMB = NULL;
    
    pw = NULL;
    ps = new ParameterSets;
    this->pic = pic;
    this->de = d;
    this->pa = p;
    ps->pps = (pa->pS->pps);
    ps->sps = (pa->pS->sps);
    index = pa->index_cur_slice;
    curNAL = pa->cur_nal;
    last_mb_qp_delta = 0;
}
slice::~slice()
{
    // 分别释放片头参数集和加权参数集
    if(pw) free_pw(pw, \
    ps->num_ref_idx_l0_active_minus1, \
    ps->num_ref_idx_l1_active_minus1);
    if(ps) delete ps;
}

void slice::decode()
{
    // 解码头部
    ParseSliceHead();
    // 解码完头部，slice需要告诉decoder需要建立参考帧列表了
    ConsRefList();
    // 解码数据部分
    ParseSliceData();
}

void slice::ParseSliceHead()
{
    int v;
    ps->first_mb_in_slice                    = pa->read_ue();
    ps->slice_type                           = pa->read_ue();
    slice_type = ps->slice_type;
    // 如果type类型数值大于5,那么减去5
    if(ps->slice_type >= 5) type = (type_slice)(ps->slice_type - 5);
    ps->pic_parameter_set_id                 = pa->read_ue();

    if(ps->sps->separate_colour_plane_flag == 1)
    ps->colour_plane_id                      = pa->read_un(2);

    v= ps->sps->log2_max_frame_num_minus4 + 4;
    ps->frame_num                            = pa->read_un(v);
    
    if(!ps->sps->frame_mbs_only_flag){
        ps->field_pic_flag                   = pa->read_un(1);
        if(ps->field_pic_flag){
            ps->bottom_field_flag            = pa->read_un(1);
        }
    }
    //POC
    if(pic->IdrPicFlag){
        ps->idr_pic_id                       = pa->read_ue();
    }
    if(ps->sps->pic_order_cnt_type == 0){
        v = ps->sps->log2_max_pic_order_cnt_lsb_minus4 + 4;
        ps->pic_order_cnt_lsb                = pa->read_un(v);
        if(ps->pps->bottom_field_pic_order_in_frame_present_flag && !ps->field_pic_flag)
            ps->delta_pic_order_cnt_bottom   = pa->read_se();
        else ps->delta_pic_order_cnt_bottom  = 0;
    }
    if(ps->sps->pic_order_cnt_type == 1 && !ps->sps->delta_pic_order_always_zero_flag){
        ps->delta_pic_order_cnt[0]           = pa->read_se();
        if(ps->pps->bottom_field_pic_order_in_frame_present_flag && !ps->field_pic_flag)
            ps->delta_pic_order_cnt[1]       = pa->read_se();
    }

    if(ps->pps->redundant_pic_cnt_present_flag){
        ps->redundant_pic_cnt                = pa->read_ue();
    }
    //B片的时间空间直接预测模式
    if(type == B){
        ps->direct_spatial_mv_pred_flag      = pa->read_un(1);
    }
    //重载的参考帧索引数量
    if(type == P||type == SP||type== B){
        ps->num_ref_idx_active_override_flag = pa->read_un(1);
        if(ps->num_ref_idx_active_override_flag)
        {
            ps->num_ref_idx_l0_active_minus1 = pa->read_ue();
            if(type == B)
                ps->num_ref_idx_l1_active_minus1 = pa->read_ue();
        }
        else //不重载的时候使用默认的值
        {
            ps->num_ref_idx_l0_active_minus1 = pa->pS->pps->num_ref_idx_l0_default_active_minus1;
            ps->num_ref_idx_l1_active_minus1 = pa->pS->pps->num_ref_idx_l1_default_active_minus1;
        }
    }
    //参考帧重排序
    //P和SP的重排序
    if(ps->slice_type % 5 != 2 && ps->slice_type % 5 != 4 ){ 
        ps->ref_pic_list_modification_flag_l0= pa->read_un(1);
        if(ps->ref_pic_list_modification_flag_l0) 
        do {
            std::vector<int>& mods = de->opra_ModS;
            ps->modification_of_pic_nums_idc = pa->read_ue();
            mods.push_back(ps->modification_of_pic_nums_idc);
            if(ps->modification_of_pic_nums_idc == 0 || ps->modification_of_pic_nums_idc == 1 )
            {
                ps->abs_diff_pic_num_minus1  = pa->read_ue();
                mods.push_back(ps->abs_diff_pic_num_minus1);
            }
            else if(ps->modification_of_pic_nums_idc == 2 ) 
            {
                ps->long_term_pic_num        = pa->read_ue();
                mods.push_back(ps->long_term_pic_num);
            }
        } while(ps->modification_of_pic_nums_idc != 3 );
    }
    //B的重排序
    if(ps->slice_type % 5 == 1){ 
        ps->ref_pic_list_modification_flag_l1= pa->read_un(1);
        if( ps->ref_pic_list_modification_flag_l1 ) 
        do {
            std::vector<int>& mods = de->opra_ModS;
            ps->modification_of_pic_nums_idc = pa->read_ue();
            mods.push_back(ps->modification_of_pic_nums_idc);
            if(ps->modification_of_pic_nums_idc == 0 || ps->modification_of_pic_nums_idc == 1 )
            {
                ps->abs_diff_pic_num_minus1  = pa->read_ue();
                mods.push_back(ps->abs_diff_pic_num_minus1);
            }
            else if(ps->modification_of_pic_nums_idc == 2 ) 
            {
                ps->long_term_pic_num        = pa->read_ue();
                mods.push_back(ps->long_term_pic_num);
            }
        } while(ps->modification_of_pic_nums_idc != 3 );
    }
    //加权预测
    if((ps->pps->weighted_pred_flag && (type == P || type == SP))||\
       (ps->pps->weighted_bipred_idc == 1 && type == B))
    {
        pw = new PredWeight();
        pw->luma_log2_weight_denom                    = pa->read_ue();
        if(pa->pV->ChromaArrayType != 0) 
            pw->chroma_log2_weight_denom              = pa->read_ue();
        pw->luma_weight_l0 = new int16_t[ps->num_ref_idx_l0_active_minus1 + 1]();
        pw->luma_offset_l0 = new int16_t[ps->num_ref_idx_l0_active_minus1 + 1]();
        pw->chroma_weight_l0 = new int16_t*[ps->num_ref_idx_l0_active_minus1 + 1]();
        pw->chroma_offset_l0 = new int16_t*[ps->num_ref_idx_l0_active_minus1 + 1]();
        for (uint8 i = 0; i <= ps->num_ref_idx_l0_active_minus1; i++)
        {
            pw->luma_weight_l0_flag                   = pa->read_un(1);
            if(pw->luma_weight_l0_flag)
            {
                pw->luma_weight_l0[i]                 = pa->read_se();
                pw->luma_offset_l0[i]                 = pa->read_se();
            }
            else
            {
                pw->luma_weight_l0[i]                 = (int16_t)powl(2, pw->luma_log2_weight_denom);
                pw->luma_offset_l0[i]                 = 0;
            }
            if(pa->pV->ChromaArrayType != 0) 
            pw->chroma_weight_l0_flag                 = pa->read_un(1);
            if(pw->chroma_weight_l0_flag)
            {
                pw->chroma_weight_l0[i] = new int16_t[2];
                pw->chroma_offset_l0[i] = new int16_t[2];
                for (uint8 j = 0; j < 2; j++)
                {
                    pw->chroma_weight_l0[i][j]        = pa->read_se();
                    pw->chroma_offset_l0[i][j]        = pa->read_se();
                }
                
            }
        }
        if(ps->slice_type % 5 == 1)
        {
            pw->luma_weight_l1 = new int16_t[ps->num_ref_idx_l1_active_minus1 + 1];
            pw->luma_offset_l1 = new int16_t[ps->num_ref_idx_l1_active_minus1 + 1];
            pw->chroma_weight_l1 = new int16_t*[ps->num_ref_idx_l1_active_minus1 + 1];
            pw->chroma_offset_l1 = new int16_t*[ps->num_ref_idx_l1_active_minus1 + 1];
            for (uint8 i = 0; i <= ps->num_ref_idx_l1_active_minus1; i++)
            {
                pw->luma_weight_l1_flag               = pa->read_un(1);
                if(pw->luma_weight_l1_flag)
                {
                    pw->luma_weight_l1[i]             = pa->read_se();
                    pw->luma_offset_l1[i]             = pa->read_se();
                }
                if(pa->pV->ChromaArrayType != 0) 
                pw->chroma_weight_l1_flag             = pa->read_un(1);
                if(pw->chroma_weight_l1_flag)
                {
                    //因为色度组件有两个，所以需要2个数组
                    pw->chroma_weight_l1[i] = new int16_t[2];
                    pw->chroma_offset_l1[i] = new int16_t[2];
                    for (uint8 j = 0; j < 2; j++)
                    {
                        pw->chroma_weight_l1[i][j]    = pa->read_se();
                        pw->chroma_offset_l1[i][j]    = pa->read_se();
                    }
                }
            }
        }
    }
    //参考标记
    if(curNAL->nal_ref_idc != 0)//标记
    {
        if(pic->IdrPicFlag){ 
            ps->no_output_of_prior_pics_flag  = pa->read_un(1);
            ps->long_term_reference_flag      = pa->read_un(1);
        }
        else{ 
            ps->adaptive_ref_pic_marking_mode_flag = pa->read_un(1);
            //如果是自适应控制标记
            if(ps->adaptive_ref_pic_marking_mode_flag)
            {
                //自适应控制标记是一系列操作，而不是某一个操作
                int i = 0;
                do {
                    std::vector<int>& MMOC = de->opra_MMOC;
                    ps->memory_management_control_operation                 = pa->read_ue();
                    MMOC.push_back(ps->memory_management_control_operation);
                    //
                    if(ps->memory_management_control_operation == 1 || \
                       ps->memory_management_control_operation == 3)        
                        //和这个slice的pic相差的PicNum
                    {
                        ps->difference_of_pic_nums_minus1                   = pa->read_ue();
                        MMOC.push_back(ps->difference_of_pic_nums_minus1);
                    }
                    if(ps->memory_management_control_operation == 2) 
                        //需要移除的长期参考帧索引
                    {
                        ps->long_term_pic_num                               = pa->read_ue();  
                        MMOC.push_back(ps->long_term_pic_num);
                    }
                    if(ps->memory_management_control_operation == 3 || \
                       ps->memory_management_control_operation == 6) 
                        //需要注册的长期参考帧索引
                    {
                        ps->long_term_frame_idx                             = pa->read_ue();
                        MMOC.push_back(ps->long_term_frame_idx);
                    }
                    if(ps->memory_management_control_operation == 4)
                        //最大的长期参考索引
                    {
                        ps->max_long_term_frame_idx_plus1                   = pa->read_ue();
                        MMOC.push_back(ps->max_long_term_frame_idx_plus1);
                    }
                }while(ps->memory_management_control_operation != 0);
            }
            //否则就是滑窗标记
        }
    }
    //CABAC 初始化init参数
    if(ps->pps->entropy_coding_mode_flag && type != I && type != SI){
        ps->cabac_init_idc                   = pa->read_ue();
    }
    else
        ps->cabac_init_idc = 0;
    ps->slice_qp_delta                       = pa->read_se();
    if(type == SP || type == SI){
        //switch解码
        if(type == SP){
        ps->sp_for_switch_flag               = pa->read_un(1);
        }
        ps->slice_qs_delta                   = pa->read_se();
    }
    //去方块滤波
    if(ps->pps->deblocking_fliter_control_present_flag)
    {
        ps->disable_deblocking_fliter_idc    = pa->read_ue();
        if(ps->disable_deblocking_fliter_idc!=1)
        {
            ps->slice_alpha_c0_offset_dic2   = pa->read_se();  
            ps->slice_beta_offset_div2       = pa->read_se();
        }
    }
    //片组循环
    v = ceil(log2( pa->pV->PicSizeInMapUnits / (ps->pps->slice_group_change_rate_minus1 + 1 + 1 )));
    if(ps->pps->num_slice_groups_minus1 > 0 &&\
       ps->pps->slice_group_map_type >= 3 &&\
       ps->pps->slice_group_map_type <= 5){
        ps->slice_group_change_cycle          = pa->read_un(v);
    }

    ps->MaxPicOrderCntLsb = (uint32)pow(2, ps->sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
    ps->SliceQPY = 26 + pa->pS->pps->pic_init_qp_minus26 + ps->slice_qp_delta;
    ps->MbaffFrameFlag = (ps->sps->mb_adaptive_frame_field_flag && !ps->field_pic_flag );
    if(ps->field_pic_flag == 0) ps->MaxPicNum = pa->pV->MaxFrameNum;
    else ps->MaxPicNum = 2 * pa->pV->MaxFrameNum; 
    if(ps->field_pic_flag == 0) ps->CurrPicNum = ps->frame_num; 
    else ps->CurrPicNum = 2 * ps->frame_num - 1; 
    
    //每次解完头，把frame_num这个句法赋值给pic，然后用 decoder 去解 picture numbers
    pic->FrameNum = ps->frame_num;
    //解码 picture numbers
    de->calc_PictureNum();

    Calc_POC();
};
void slice::ConsRefList()
{
    pic->FrameNum = ps->frame_num;
    //解码picture numbers
    de->calc_PictureNum();

    //初始化参考列表，把 pic_current 指针加入到相应的参考列表中去
    //解I帧不需要参考，只需要标记
    //初始化参考表(根据上一次的标记)
    if(type == P || type == SP || type == B)
        de->init_RefPicList();

    //修改
    //操作符在 decoder 中已经指定和如何移除，所以这里不用做是否启用这个函数的判定
    de->opra_RefModfication(ps->MaxPicNum, ps->CurrPicNum, ps->num_ref_idx_l0_active_minus1, 0);
    de->opra_RefModfication(ps->MaxPicNum, ps->CurrPicNum, ps->num_ref_idx_l1_active_minus1, 1);

    //至此参考列表建立完成
}
void slice::ParseSliceData()
{

    //初始化变量
    int x_cur = 0;
    int y_cur = 0;
    int CurrMbAddr = ps->first_mb_in_slice * (1 + ps->MbaffFrameFlag);
    int moreDataFlag = 1;
    int prevMbSkipped = 0;
    int index_MbInSlcie = 0;
    int index_MbInPicture = 0;

    // 本来是应该用到第一个句法元素的时候才init的
    // 但是这里 已经知道了 entropy_coding_mode_flag 这个句法元素
    // 而且目前处理的都是ae算子，马上就会要用到，所以写在这里
    // cabac_init_idc 在头部中就已经读取到了，所以不用担心
    pa->set_cabac_slice_new(pic, this);

    //片数据对齐
    if(ps->pps->entropy_coding_mode_flag) pa->read_al();


    // if(index == 43)
    //     terr.de.cabac_state_running = true;
        

    int width_mb = pa->pV->PicWidthInMbs;
    int heigh_mb = pa->pV->PicHeightInMbs;
    do{

        //跳过编码只是不解析而已，但是同样需要数据块、解码
        macroblock* mb = new macroblock(this, pa, de);
        
        // slice的统计由parser完成
        mb->idx_slice = pa->index_cur_slice;

        mb->idx_inslice = index_MbInSlcie++;

        
        // 宏块坐标的统计必须由slice来完成
        // 而不是由picture来完成，
        // 因为图片无法知道下一个宏块的坐标是什么，只有slice能推导出回来
        mb->pos.x = x_cur;
        mb->pos.y = y_cur;
        if(++x_cur >= width_mb) {x_cur = 0; ++y_cur;}

        mb->idx_inpicture = index_MbInPicture++;//index_MbInPicture这个变量不应该设置在slice里面，现在还没有改过来
    


        // 宏块加入pic同时找到属于自己的数据并且连接起来
        // 在这里找到了他的pic，
        // 宏块应该自己去寻找自己的内存，而不是由picture带着找
        // this->pic->connect_mb(mb);
        mb->attach(pic);

        // 需要一个头宏块来放初始参数
        //add macroblock into slice tree and set the init MBQPY
        //slice里面的宏块树的建立，量化参数的初始化，
        if(this->hedMB == NULL) 
        {
            hedMB = mb;
            // QPY prev 指的是slice中上一个宏块的量化参数 QPY 
            // 如果是第一个宏块，那么QPY_prev就是SliceQPY的值
            mb->qp.QPY_prev = this->ps->SliceQPY;
        }
        else
        {
            // 现在的curMB还是上一个解码完毕的宏块
            mb->qp.QPY_prev = curMB->qp.QPY;
        }
        //设置curMB为当前宏块
        curMB = mb;

        mb->mb_skip_flag = 0;
        if(type != I && type != SI)
        {
            // 这个if还没有具体实现，因为默认用ae算子
            if(!ps->pps->entropy_coding_mode_flag)
            {
                        ps->mb_skip_run = pa->read_ue();
                        prevMbSkipped = (ps->mb_skip_run > 0);
                        for(size_t i = 0; i < ps->mb_skip_run; i++){
                            CurrMbAddr = NextMbAddress(CurrMbAddr);
                        }
                        if(ps->mb_skip_run > 0)
                            moreDataFlag = more_rbsp_data( );
            }
            else
            {
                ps->mb_skip_flag = pa->read_ae(0x00011000U);
                moreDataFlag = !ps->mb_skip_flag;
                // 设置这个句法元素给宏块，让其自己判断解码方式
                mb->mb_skip_flag = ps->mb_skip_flag;
            }
        }
        // skipflag标识了这个宏块后面还有没有多余的数据
        // 也就是说说明了是不是Skip宏块
        if(moreDataFlag)
        {
            //意思是最后的宏块对中必须有一个宏块要被解码，要么都被跳过或者被帧解码，如果解码一个的话就要用场解码
            if(ps->MbaffFrameFlag && (CurrMbAddr % 2 == 0 || (CurrMbAddr % 2 == 1 && prevMbSkipped)))
                ps->mb_field_decoding_flag = pa->read_ae(0x00012000U);//1a
            else ps->mb_field_decoding_flag = ps->field_pic_flag;
        }
        
        //

        
        // 宏块解码，宏块应该能根据自己的句法元素推断出来自己的解码方式
        // 这一过程代替了就在上面的那一堆注释掉的函数
        mb->decode();
        //打印所有宏块的信息
        // mb->Info();

        // 判断后面还是否是到了slice的结尾
        if(!ps->pps->entropy_coding_mode_flag)       //if not CABAC 编码 , use more_rbsp_data() to confirm end
            moreDataFlag = more_rbsp_data();
        else
        {
            if(type != I && type != SI)
                prevMbSkipped = ps->mb_skip_flag;
            // 如果是不是frame模式，并且当前地址不是偶数，那么后面一定还有数据
            if(ps->MbaffFrameFlag && CurrMbAddr % 2 == 0)
                moreDataFlag = 1;                     //
            else
            {
                // 否则读取结束标志看是不是结束了（片结束标志）
                ps->end_of_slice_flag = pa->read_ae(0x00276000U);            //片结束标记
                moreDataFlag = !ps->end_of_slice_flag;  //如果片结束，那么moreDataFlag为0，此循环之后停止运行
                //if(ps->end_of_slice_flag == 1)  printf(">>slice: end mb: (%d,%d)\n", mb->position_x, mb->position_y);
            }
        }
        if(y_cur == heigh_mb && !ps->end_of_slice_flag) 
            break;
    }while(moreDataFlag);
    pa->index_cur_slice += 1;

    // 片结束了，调用cabac的结束标志，
    // 因为cabac的运行周期是片的生命周期
    pa->set_cabac_slice_end();
};




void slice::Calc_POC()
{
    uint8 pic_order_cnt_type = pa->pS->sps->pic_order_cnt_type;
    uint8 pic_order_cnt_lsb = ps->pic_order_cnt_lsb;
    
    picture* cur = de->get_CurrentPic();
    cur->pic_order_cnt_lsb = pic_order_cnt_lsb;
    picture* pre = de->get_LastRef();

    if(pic_order_cnt_type == 0)
    {
        int prevPicOrderCntMsb = 0,  prevPicOrderCntLsb = 0;
        if(cur->IdrPicFlag)
        {
            prevPicOrderCntMsb = prevPicOrderCntLsb = 0;
        }
        else
        {
            if(pre->memory_management_control_operation == 5)
            {
                //if(!pre->is_bottom())
                prevPicOrderCntMsb = 0;
                prevPicOrderCntLsb = pre->TopFieldOrderCnt;
                //else
                // prevPicOrderCntMsb = 0;
                // prevPicOrderCntLsb = 0;
            }
            else
            {
                prevPicOrderCntMsb = pre->PicOrderCntMsb;
                prevPicOrderCntLsb = pre->pic_order_cnt_lsb;
            }
            
        }
        //calc PicOrderCntMsb
        if((pic_order_cnt_lsb < prevPicOrderCntLsb) && (prevPicOrderCntLsb - pic_order_cnt_lsb >= (ps->MaxPicOrderCntLsb / 2)))
        {
            cur->PicOrderCntMsb = prevPicOrderCntMsb + ps->MaxPicOrderCntLsb;
        }
        else if((pic_order_cnt_lsb > prevPicOrderCntLsb) && (pic_order_cnt_lsb - prevPicOrderCntLsb > (ps->MaxPicOrderCntLsb / 2)))
        {
            cur->PicOrderCntMsb = prevPicOrderCntMsb - ps->MaxPicOrderCntLsb;
        }
        else cur->PicOrderCntMsb = prevPicOrderCntMsb;
        //如果不是底场 TopFieldOrderCnt 由下式给出
        cur->TopFieldOrderCnt = cur->PicOrderCntMsb + pic_order_cnt_lsb;
        //如果不是顶场  BottomFieldOrderCnt  由下式给出
        if(!ps->field_pic_flag) cur->BottomFieldOrderCnt = cur->TopFieldOrderCnt + ps->delta_pic_order_cnt_bottom;
        else cur->BottomFieldOrderCnt = cur->PicOrderCntMsb + ps->pic_order_cnt_lsb;
    }
    cur->POC = Min(cur->TopFieldOrderCnt, cur->BottomFieldOrderCnt);
    
}










//这里只是做了frame的返回，还有其他的情况没有写：自适应 和 场
// macroblock* slice::get_curMB(){return cur_macroblcok;}

static bool free_pw(PredWeight* pw, int num_ref_idx_l0_active_minus1, int num_ref_idx_l1_active_minus1)
{
    if(pw)
    {
        delete[] pw->luma_weight_l0;
        delete[] pw->luma_offset_l0;
        if(pw->chroma_weight_l0)
        {
            for (uint8 i = 0; i < num_ref_idx_l0_active_minus1 + 1; i++)
            delete[] pw->chroma_weight_l0[i];

            delete[] pw->chroma_weight_l0;
        }
        if(pw->chroma_offset_l0)
        {
            for (uint8 i = 0; i < num_ref_idx_l0_active_minus1 + 1; i++)
            delete[] pw->chroma_offset_l0[i];

            delete[] pw->chroma_offset_l0;
        }
        if(pw->chroma_weight_l1)
        {            
            for (uint8 i = 0; i < num_ref_idx_l1_active_minus1 + 1; i++)
            delete[] pw->chroma_weight_l1[i];

            delete[] pw->chroma_weight_l1;
        }
        if(pw->chroma_offset_l1)
        {
            for (uint8 i = 0; i < num_ref_idx_l1_active_minus1 + 1; i++)
            delete[] pw->chroma_offset_l1[i];
            
            delete[] pw->chroma_offset_l1;
        }
        delete pw;
    };
    return true;
}