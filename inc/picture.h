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
    /**
     * 必须没有子块分区，也就是说所有的最小分区尺寸都必须相同
        @param current 当前宏块
        @param index 索引，逆扫描索引
        @param direction 方向
        @param width_part 当前part的宽，
        @param height_part 当前part的高
        @param width_all 当前宏块的宽
        @param height_all 当前宏块的高
        @param result 返回的宏块指针
    */
    void neighbour_position_nonsub(\
        macroblock *current, int index, \
        char direction, \
        int width_part, int height_part,\
        int width_all , int height_all, \
        int * x_result, int *y_result,\
        macroblock **result
    );
    void neighbour_position_sub(\
        macroblock *current,\
        int mbPartIdx, int subPartIdx,\
        char direction, \
        int width_all , int height_all, \
        int * x_result, int *y_result,\
        macroblock **result
    );
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

    // 因为只有picture才能完成相邻宏块之间的交流
    // 所以这些函数需要放在这里

    // 得到4x4块的周围情况
    // 宏块大小为 16x16 
    // 输入的idx_4x4 必须是zig-zag扫描模式
    void        neighbour_luma4x4(macroblock *current, int idx_4x4, char direction, macroblock **info, int *index_target);
    // 得到4x4块的周围情况
    // 宏块大小为 16x16 
    // 输入的idx_4x4 必须是zig-zag扫描模式
    void        neighbour_4x4block(macroblock *current, int idx_4x4, char direction, macroblock **info, int *index_target, int *r, int *c);
    // 得到相邻宏块的情况
    macroblock* neighbour_macroblock(macroblock* cur, char direction);
    // 根据周围的运动矢量来预测当前part(subpart)的情况
    void        neighbour_motionvector(macroblock *current, uint8 mbPartIdx, uint8 subPartIdx, uint8 direction, MotionVector **mv_lx);

    void        neighbout_subpart(macroblock *current, int mbPartIdx, int subPartIdx, char direction, int *partIdx_result, int *subIdx_result);

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