#include "decoder.h"


#include "picture.h"
#include "slice.h"
#include <vector>
#include <algorithm>

#include "terror.h"
#include "gvars.h"
#include "gfunc.h"

void decoder::print_list()
{
    
    printf(">>deder:    \n");//(direction:----->)  [Frame_num]\n");//(dec_num)
    printf("         deocded list :");
    for (uint8 i = 0; i < list_Decoded.size(); i++)
    {
        printf("%d, ",list_Decoded[i]->POC);//, list_Decoded[i]->DecNum);
    }
    printf("\n");
    printf("         reflist   list :");
    for (uint8 i = 0; i < list_Ref.size(); i++)
    {
        printf("%d, ",list_Ref[i]->POC);
    }
    printf("\n");
    printf("         reflist_0 list :");
    for (uint8 i = 0; i < list_Ref0.size(); i++)
    {
        printf("%d, ",list_Ref0[i]->POC);//, list_Ref0[i]->DecNum);
    }
    printf("\n");
    printf("         reflist_1 list :");
    for (uint8 i = 0; i < list_Ref1.size(); i++)
    {
        printf("%d, ",list_Ref1[i]->POC);//, list_Ref1[i]->DecNum);
    }
    printf("\n");
}
bool decoder::clear_DecodedPic()
{
    // 输出所有缓冲的图像

    // 释放解码队列的所有缓存数据
    for(uint8 i = 0; i < list_Decoded.size(); i++)
    {
        delete list_Decoded[i];
    }
    // 清空所有的队列 
    list_Decoded.clear(); 

    list_Ref.clear(); 
    list_Ref_short.clear();
    list_Ref_long.clear();

    list_Ref0.clear(); 
    list_Ref1.clear(); 

    // 清空输出栈
    while(!list_Out.empty())
    {
        list_Out.pop();
    }
    
    
    return true;
};

//这个函数用来控制内存，在每次开始解码下一个帧的时候都把之前无用的帧删掉，
//标记在之前完成，
bool decoder::ctrl_Memory()
{
    std::vector<picture*>::iterator it;
    it = list_Decoded.begin();
    while(it != list_Decoded.end())
    {
        //释放 已经输出的 并且 不用于参考的 pic 或者 标记为不存在的 pic
        if(!(*it)->is_UsedForRef() && (*it)->state_out)
        {
            delete (*it);
            it = list_Decoded.erase(it);
        }
        else it++;
    }
    return true;
}

bool decoder::add_ReferenPic(picture* pic_toadd)
{
    if(pic_toadd == NULL) return false;

    //先按照解码顺序进入总的参考队列
    list_Ref.push_back(pic_toadd);

    //然后按照参考类型分别进入相应的参考队列
    if(pic_toadd->is_UsedForLong()) list_Ref_long.push_back(pic_toadd);
    else list_Ref_short.push_back(pic_toadd);

    return true;
}

//刷新参考表
//是指刷新所有的资源参考表
//从解码队列中拿取参考pic 来初始化参考资源表
//不直接修改这三个表的原因是：防止已经 delete 的 pic 出现在表中
bool decoder::flsh_ListRef()
{
    // auto flsh_func = [](std::vector<picture*>&  list_toflsh){
    //     std::vector<picture*>::iterator it;
    //     it = list_toflsh.begin();
    //     while (it != list_toflsh.end())
    //     {
    //         if((*it) && !(*it)->is_UsedForRef())
    //         {
    //             list_toflsh.erase(it);
    //         }
    //         else it++;
    //     }
    // };
    list_Ref.clear();
    list_Ref_short.clear();
    list_Ref_long.clear();
    std::vector<picture*>::iterator it;
    for(it = list_Decoded.begin();it != list_Decoded.end();it++)
    {
        if((*it)->is_UsedForRef()) 
        {
            list_Ref.push_back((*it));
            if((*it)->is_UsedForShort())
                list_Ref_short.push_back(*it);
            else //if((*it)->is_UsedForLong())
                list_Ref_long.push_back(*it);
        }
    }
    return true;
}
bool decoder::init_RefPicList()
{
    //初始化全部参考帧，长期参考帧，短期参考帧
    //这个函数在解码pic之前调用，解码pic加入解码队列是在解码pic之后，
    //所以可以直接从解码队列中获取所需的参考帧
    flsh_ListRef();

    //以下的表是指 参考表0 和 参考表1
    //建表包含 初始化，排序两个工作,
    type_slice type = curSLI->type;
    //清空参考队列0
    list_Ref0.clear();
    if(type == P || type == SP)
    {
        std::vector<picture*>::iterator it;
        int i_ref0 = 0;

        //插入short，先插入short，然后 long ，这样保证 short 自然在 long 的前面
        for(it = list_Ref_short.begin(); it != list_Ref_short.end(); it++)
        {
            list_Ref0.push_back(*it);
        }
        i_ref0 = list_Ref0.size();
        //从开头排序到结束的地方
        std::sort(list_Ref0.begin(), list_Ref0.end(), \
        [](picture* l, picture* r)->bool{return l->PicNum > r->PicNum;}\
        );
        //插入long
        for(it = list_Ref_long.begin(); it != list_Ref_long.end(); it++)
        {
            list_Ref0.push_back(*it);
        }
        //从短期结束的地方开始排序到结束的地方
        std::sort(list_Ref0.begin()+i_ref0, list_Ref0.end(), \
        [](picture* l, picture* r)->bool{return l->LongTermPicNum < r->LongTermPicNum;}\
        );

        return true;
    }
    else //if(type == B)
    {
        //B帧还需要清空参考列表1
        list_Ref1.clear();
        //P 帧中是直接插入参考帧，B帧需要一些处理
        std::vector<picture*>::iterator it;
        int i_ref = 0;//一个中间变量，用来存储需要排序的节点位置

        //插入short,插入的时候做第一步排序，也就是所有的大于当前POC的在末尾追加，小于的在整个小于队列的末尾追加
        for(it = list_Ref_short.begin(), i_ref = 0; it != list_Ref_short.end(); it++)
        {
            if((*it)->POC > curPIC->POC)
            {
                list_Ref0.push_back(*it);//大于当前POC的在末尾追加
            }
            else
            {   //i_ref 用来指明小于当前POC的pic的结束位置，插入一个就增加一次
                list_Ref0.insert(list_Ref0.begin()+i_ref, *it);//小于的在整个小于队列的末尾追加
                i_ref++;
            }
        }
        //小于当前POC的按照降序排列
        std::sort(list_Ref0.begin(), list_Ref0.begin()+i_ref, \
        [](picture* l, picture* r)->bool{return l->POC > r->POC;}\
        );
        //大于当前POC的按照升排列
        std::sort(list_Ref0.begin()+i_ref, list_Ref0.end(),\
        [](picture* l, picture* r)->bool{return l->POC < r->POC;}\
        );
        i_ref = list_Ref0.size();
        //插入long
        for(it = list_Ref_long.begin(); it != list_Ref_long.end(); it++)
        {
            list_Ref0.push_back(*it);
        }
        //从短期结束的地方开始排序到结束的地方
        std::sort(list_Ref0.begin()+i_ref, list_Ref0.end(), \
        [](picture* l, picture* r)->bool{return l->LongTermPicNum < r->LongTermPicNum;}\
        );
        
        //list1和list0方法相同，只不过排序的序列不同
        for(it = list_Ref_short.begin(), i_ref = 0; it != list_Ref_short.end(); it++)
        {
            if((*it)->POC < curPIC->POC)
            {
                list_Ref1.push_back(*it);//小于当前POC的在末尾追加
            }
            else
            {   //i_ref 用来指明大于当前POC的pic的结束位置，插入一个就增加一次
                list_Ref1.insert(list_Ref1.begin()+i_ref, *it);//大于的在整个大于队列的末尾追加
                i_ref++;
            }
        }
        //大于当前POC的按照升序排列
        std::sort(list_Ref1.begin(), list_Ref1.begin()+i_ref, \
        [](picture* l, picture* r)->bool{return l->POC < r->POC;}\
        );
        //小于当前POC的按照降序排列
        std::sort(list_Ref1.begin()+i_ref, list_Ref1.end(),\
        [](picture* l, picture* r)->bool{return l->POC > r->POC;}\
        );
        i_ref = list_Ref1.size();
        //插入long
        for(it = list_Ref_long.begin(); it != list_Ref_long.end(); it++)
        {
            list_Ref1.push_back(*it);
        }
        //从短期结束的地方开始排序到结束的地方
        std::sort(list_Ref1.begin()+i_ref, list_Ref1.end(), \
        [](picture* l, picture* r)->bool{return l->LongTermPicNum < r->LongTermPicNum;}\
        );
        //如果两表相同，list1 的前两项交换
        if(list_Ref1.size() > 1 && list_Ref0.size() == list_Ref1.size())
        {
            int size = list_Ref0.size();
            bool re = true;
            for (int i = 0; i < size; i++)
            {
                if(list_Ref0[i] != list_Ref1[i])
                {
                    re = false;
                    break;
                }
            }
            if(re)
            {
                picture* tmp = list_Ref1[0];
                list_Ref1[0] = list_Ref1[1];
                list_Ref1[1] = tmp;
            }
        }
        return false;
    }
}
int PicNumF(picture* pic)
{
    if(pic && pic->is_UsedForShort()){return pic->PicNum;}
    else                             {return -1;}
}
//重排序
bool decoder::opra_RefModfication(int MaxPicNum, int  CurrPicNum, int num_ref_idx_lX_active_minus1, char X)
{

    int i = 0, opra = 0;
    int picNumLXPred = CurrPicNum;
    int picNumLXNoWrap = 0;
    int picNumLX = 0;
    std::vector<picture*>& RefPicListX = !X?list_Ref0:list_Ref1;
    int refIdxLX = 0;

    if(opra_ModS.size() > 0)
    {
        //在末尾加上一位监视位
        RefPicListX.push_back(NULL);
        while ((opra = opra_ModS[i++]) != 3)
        {
            if(opra == 0 || opra == 1)
            {
                //短期重排
                int sig = opra == 0?-1:1;
                int diff_pic_num = (opra_ModS[i++] + 1)*sig;
                if(picNumLXPred + diff_pic_num < 0 || picNumLXPred + diff_pic_num > MaxPicNum) picNumLXNoWrap = picNumLXPred + diff_pic_num + -1*sig*MaxPicNum;
                else picNumLXNoWrap = picNumLXPred + diff_pic_num;
                picNumLXPred = picNumLXNoWrap;

                if(picNumLXNoWrap > CurrPicNum) picNumLX = picNumLXNoWrap - MaxPicNum;
                else picNumLX = picNumLXNoWrap;

                picture* tomod = NULL;
                int nIdx = 0, cIdx = 0;
                //如果最后一位不是NULL，那么说明位置不够了，需要补上一位
                if(RefPicListX[RefPicListX.size()-1] != NULL)
                {RefPicListX.push_back(NULL);}
                //整个列表向后移动一位（从refIdxLX开始的地方）
                for(cIdx = RefPicListX.size()-1; cIdx > refIdxLX; cIdx--)
                {//元素后移一位
                    RefPicListX[cIdx] = RefPicListX[cIdx - 1];
                }
                //查找满足short-term reference picture with PicNum equal to picNumLX 的pic
                std::vector<picture*>::iterator it;
                it = std::find_if(list_Ref.begin(), list_Ref.end(), \
                [picNumLX](picture* i)->bool{return i && i->is_UsedForShort() && i->PicNum == picNumLX;}\
                );
                tomod = it==RefPicListX.end()?NULL:(*it);
                //0索引赋值给 PicNum == PicNumLX 的短期参考图片，然后索引到1
                RefPicListX[refIdxLX++] = tomod;//tomod填入第一位
                nIdx = refIdxLX;
                //如果RefPicListX[cIdx]的PicNum不等于
                for(cIdx = refIdxLX; cIdx < RefPicListX.size(); cIdx++)
                    if(PicNumF(RefPicListX[cIdx]) != picNumLX)
                        RefPicListX[nIdx++] = RefPicListX[cIdx];
            }
            else if(opra == 2)
            {//长期重排
            }
            else
            {
                // 输出一个错误说明当前不存在的排序操作符号
            }
            
        }
        //删掉用过的操作符
        opra_ModS.erase(opra_ModS.begin(), opra_ModS.begin()+i);
    }
    //去掉多余的元素，一是超出长度的，一是NULL元素
    while(RefPicListX.size() > num_ref_idx_lX_active_minus1 + 1 \
      || (RefPicListX.size() > 0 && (RefPicListX.back()) == NULL))
    {
        RefPicListX.pop_back();
    }
    return true;
}

bool decoder::calc_PictureNum()
{
    int frame_num = curSLI->ps->frame_num;
    int MaxFrameNum = curSLI->ps->MaxPicNum;
    int field_pic_flag = curSLI->ps->field_pic_flag;

    //给每个pic都计算其 相应的变量
    std::vector<picture*>::iterator it;
    for(it = list_Ref.begin(); it != list_Ref.end(); it++)
    {
        if((*it)->is_UsedForShort())
        {
            if((*it)->FrameNum > frame_num)
                (*it)->FrameNumWrap = (*it)->FrameNum - MaxFrameNum;
            else (*it)->FrameNumWrap = (*it)->FrameNum;
        }
        if(!field_pic_flag)
        {
            if((*it)->is_UsedForShort())
            {(*it)->PicNum = (*it)->FrameNumWrap;}
            if((*it)->is_UsedForLong())
            {(*it)->LongTermPicNum = (*it)->LongTermFrameIdx;}
        }
    }
    return true;
}
bool decoder::ctrl_MMOC()
{
    /*
    1 - 一个 短期    标记为 不用于参考
    2 - 一个 长期    标记为 不用于参考
    3 - 一个 短期    标记为 长期
    4 - 大于 最大长期索引的 长期参考图像 标记为 不用于参考
    5 - 全部 参考图像 标记为 不用于参考  最大长期索引 设置为 无
    6 - 分配一个 长期参考索引 给 当前图像
    */
    opra_MMOC;
    int opration = 0;
    int i = 0;
    while (opration = opra_MMOC[i++] != 0)
    {
        switch (opration)
        {
            case 1:ctrl_MMOC_1(opra_MMOC[i++]);break;
            case 2:ctrl_MMOC_2(opra_MMOC[i++]);break;
            case 3:ctrl_MMOC_3(opra_MMOC[i++], opra_MMOC[i++]);break;
            case 4:ctrl_MMOC_4(opra_MMOC[i++]);break;
            case 5:ctrl_MMOC_5();break;
            case 6:ctrl_MMOC_6(opra_MMOC[i++]);break;
            default:;break;
        }
    }
    opra_MMOC.clear();
    return true;
}

bool decoder::ctrl_MMOC_1(int difference_of_pic_nums_minus1)
{
    int picNumX  = curSLI->ps->CurrPicNum - (difference_of_pic_nums_minus1 + 1);
    std::vector<picture*>::iterator it;
    it = std::find_if(list_Ref.begin(), list_Ref.end(),\
    [picNumX](picture* i)->bool{return i && i->is_UsedForShort() && i->PicNum == picNumX;}\
    );
    if(*it == NULL) return false;
    else (*it)->set_NotUseForRef();
    return true;
}
bool decoder::ctrl_MMOC_2(int long_term_pic_num)
{
    std::vector<picture*>::iterator it;
    it = std::find_if(list_Ref.begin(), list_Ref.end(),\
    [long_term_pic_num](picture* i)->bool{return i && i->is_UsedForLong() && i->LongTermPicNum == long_term_pic_num;}\
    );
    if(*it == NULL) return false;
    else 
    {
        (*it)->set_NotUseForRef();
        return true;
    }
}
bool decoder::ctrl_MMOC_3(int difference_of_pic_nums_minus1,int long_term_frame_idx)
{
    int picNumX = curSLI->ps->CurrPicNum - (difference_of_pic_nums_minus1 + 1);
    std::vector<picture*>::iterator it;
    it = std::find_if(list_Ref.begin(), list_Ref.end(),\
    [picNumX](picture* i)->bool{return i && i->is_UsedForShort() && i->PicNum == picNumX;}\
    );
    if(*it == NULL) return false;
    else 
    {
        (*it)->set_UseForLong(long_term_frame_idx);
        return true;
    }
}
bool decoder::ctrl_MMOC_4(int max_long_term_frame_idx_plus1)
{
    if(max_long_term_frame_idx_plus1 == 0)  MaxLongTermFrameIdx = -1;
    else MaxLongTermFrameIdx = max_long_term_frame_idx_plus1 - 1;
    return true;
}
bool decoder::ctrl_MMOC_5()   
{
    MaxLongTermFrameIdx = -1;
    for (int i = 0; i < list_Ref.size(); i++)
    {
        list_Ref[i]->set_NotUseForRef();
    }
    list_Ref.clear();
    list_Ref0.clear();
    list_Ref1.clear();
    return true;
}
bool decoder::ctrl_MMOC_6(int long_term_frame_idx)
{
    curPIC->set_UseForLong(long_term_frame_idx);
    curPIC->memory_management_control_operation = 6;
    // LongTermFrameIdx 在下一次初始化的时候会计算到
    return true;
}
bool decoder::ctrl_FIFO(int max_num_ref_frames)
{//如果短期参考帧的数量达到最大参考帧数量减去长期帧的那么移除最小的 FrameNumWrap 的 pic 标记为不用于参考
    //先进先出，所以从前往后遍历到第一个短期参考帧就行
    if(list_Ref_short.size() == max_num_ref_frames - list_Ref_long.size())
    {
        std::vector<picture*>::iterator it;
        it = std::find_if(list_Ref.begin(), list_Ref.end(), [](picture* i)->bool{return i->is_UsedForShort();});
        (*it)->set_NotUseForRef();
    }
    else
    {
        int a = 0;
    }
    return true;
}

void decoder::out_DecodedBuf()
{

}

#include <opencv2/opencv.hpp>
using namespace cv;
void decoder::out_DecodedPic()
{
    //由栈来管理和输出缓冲的pic
    //按照 POC 的逆序出栈
    //按照 解码顺序 入栈(入栈操作在添加解码pic函数中)
    //这样就能保证 小POC 先输出 大POC 后输出

    // 遇到IDR，则计数器归零
    if(curPIC->IdrPicFlag)
    {
        count_Out = 0;
    }
    while(!list_Out.empty() && list_Out.top()->POC == count_Out)
    {
        // 输出图像

        if(terr.pic_terminalchar())//是否输出字符画的控制
        {
            // list_Out.top()->drawpic();      //pic字符化
            picture* pic = list_Out.top();
            printf(">>decder: current out pic : POC:%4d\n", list_Out.top()->POC);
            Mat frame(pic->cons->h, pic->cons->w, CV_8UC1, pic->cons->data);
            imshow("out", frame);
            waitKey(1);
        }
        
        // 输出完毕
        list_Out.top()->state_out = true;//输出之后的 pic 输出状态全部变为 已经输出
        list_Out.pop();                  //已经输出的pic出栈
        
        count_Out+=2;                    //输出计数自增2（帧模式是两场，所以递增步长是2）
    }
}
decoder::decoder()
{

    namedWindow("out", cv::WINDOW_NORMAL);

    count_Out = 0;
    curPIC = NULL;
    curSLI = NULL;
    MaxLongTermFrameIdx = 100;
}
decoder::~decoder()
{
}