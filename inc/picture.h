#ifndef PICTURE_H__
#define PICTURE_H__


#include "gtype.h"
#include "genum.h"
#include "gmbinfo.h"

#include "array2d.h"

class parser;
class decoder;
class macroblock;
class pixel;
class slice;

// picture不是图像，是用来管理图像的对象
// 主要针对的对象是macroblock
// 即描述如何组织macroblock，以及如何操作、读取这些macroblock
// 还包括macroblock之间的沟通，相邻的问题等
class picture
{
private:

    //
    parser *pa;
    decoder *de;

    pixel *picpix;
    // 当前的slice
    slice *sl;

    // 宏块计数器
    int count_mb;
    
public:
    picture(parser*, decoder*);
    ~picture();

    // 按照样点为单位来计算得到的pic尺寸
    area size;

    // 宏块
    array2d<macroblock*> *mb;
    // -- 预测值和残差存储在宏块内
    
    // 预测值 
    // array2d<pix16> *pred;
    // 残差值
    // array2d<int16> *resi;
    // 重建图像值
    array2d<byte>  *cons;
    // 获得一个宏块，并且把他放到正确的位置
    // 同时告诉这个宏块周围的可用情况
    void takein(macroblock* m);
    // 根据自身的信息来解码
    void deocde();
    // 根据 nal_ref_idc 来完成相应的动作
    void refidc(int nal_ref_idc);

    void print();

    void drawpic();

    // 因为只有picture才能完成相邻宏块之间的交流
    // 所以这些函数需要放在这里

    // 指明当前图像是不是IDR图像
    bool IdrPicFlag;

    // 指明当前pic所处的参考状态
    reference state_ref;
    // 指明当前pic是否的输出状态
    bool state_out;

    // 解码的Num，表示当前pic的解码顺序
    int DecNum;

    //FrameNum相关变量
    int PicNum;
    int FrameNum;
    int LongTermIdx;
    int LongTermPicNum;
    int FrameNumWrap;
    //POC相关变量
    int POC;
    int TopFieldOrderCnt;
    int BottomFieldOrderCnt;
    
    int pic_order_cnt_lsb;
    int PicOrderCntMsb;
    
    //内存控制标志
    uint8 memory_management_control_operation;

    //参考帧的相关属性
    int LongTermFrameIdx;
    bool is_UsedForShort()      {return state_ref == Ref_short? true : false;};
    bool is_UsedForLong ()      {return state_ref == Ref_long? true : false;};
    bool is_UsedForRef  ()      {return(state_ref == Ref_long || state_ref == Ref_short) ? true : false;};
    bool is_Exist()             {return state_ref == Nul_exist? false : true;};
    void set_UseForShort()      {state_ref =  Ref_short ;};
    void set_UseForLong(int i)  {state_ref =  Ref_long  ;LongTermPicNum = i;};
    void set_NotUseForRef()     {state_ref =  Nun_ref   ;};
    void set_NotExist()         {state_ref =  Nul_exist ;};
};


#endif