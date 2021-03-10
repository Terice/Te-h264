#ifndef MACROBLOCK_H__
#define MACROBLOCK_H__

#include "gtype.h"
#include "genum.h"
#include "gmbinfo.h"

#include "array2d.h"

class residual;
class picture;
class parser;
class decoder;
class slice;
class pixmap;

class macroblock;
class matrix;





class macroblock
{
private:
    parser* pa;
    decoder* de;
    slice* sl;

    
    uint32 address;


    // 枚举类型转换为实值
    // 枚举用来称谓，实值用来读表
    int transtype(type_macroblock type);

    // 子块信息的读取
    void Parse_Sub(int*);
    // 解析Direct块
    void Parse_Direct();
    // 分别根据预测模式的不同来解析对应的帧内预测环境
    void Parse_Intra();
    // 解析普通的帧间块
    void Parse_Inter();
    // 解析 Skip 的运动矢量
    void Parse_Skip();

    // 帧内4x4解码模式解析
    void ParseIntra4x4PredMode();
    // 帧内4x4解码
    void Decode_Intra4x4();
    // 帧内8x8解码
    void Decode_Intra8x8();
    // 帧内16x16 解码
    void Decode_Intra16x16();
    // 帧间解码
    void Decode_Inter();

    // 重建图像
    void ConstructPicture();

    void Calc_residual();
public:
    // 宏块读取出来的索引
    int mb_type;
    // 宏块类型
    type_macroblock type;
    // 宏块分区
    uint8 num_mb_part;
    // 与宏块绑定的pic
    picture* pic;
    // 宏块的残差
    residual* re;
    // 旁路解码标志
    bool  TransformBypassModeFlag;

    //
    // 宏块的坐标
    // 意思是宏块在pic的坐标
    point pos;
    // 宏块起始样点的全局坐标
    point pos_sample_start;

    // 宏块在pic中的index
    uint16 idx_inpicture;
    // 每一个宏块都有一个slice的index
    uint8  idx_slice;
    // 宏块在slice内的idx
    uint16 idx_inslice;

    //syntax elements
    
    // 来自slice读取的句法元素，说明当前宏块是不是Skip宏块
    // 如果是的话，还需要做初始化空block的动作
    uint16 mb_skip_flag;
    // 场解码模式，应该用不上吧，，毕竟场解码函数都没写  
    uint16 mb_field_decoding_flag; 

    // 8x8变换系数解码，用来判断I_NxN究竟是4x4 还是 8x8
    // 但实际上这个句法元素还有别的用处
    // 因为是对于帧内和帧间都可能出现，所以放出来了
    uint8 transform_size_8x8_flag;

    // 说明当前所有像素块的编码模式
    uint8 coded_block_pattern;   
    // 说明当前亮度块的编码方式
    uint8 CodedBlockPatternLuma;
    // 说明当前色度块的编码方式
    uint8 CodedBlockPatternChroma;

    // 指明当前是帧内还是帧间预测
    // 帧内为 1 ，帧间为 0
    int intea_mode;
    bool is_interpred(){return intea_mode ? false : true;};
    bool is_intrapred(){return intea_mode ? true : false;};
    bool is_avaiable(macroblock *);

    // 宏块自己去找在pic中的位置
    // 然后把自己绑定到pic上，并且把数据连接起来
    void attach(picture* p);

    // part块的预测模式，用来判断predmode中究竟使用什么结构
    // 主要是用来区分Intra系列和Direct
    // 因为只取这个宏块的第一个块的预测模式，
    // 所以对于多块的其实没有意义
    predmode_mb_part predmode;

    // 帧间预测模式，
    Inter_pred* inter;
    // 帧内预测模式，预测模式在intra中已经有说明了
    Intra_pred* intra;
    
    // 还需要三个个对像素级操作的数据对象
    // 对这三个对象进行操作时，一定要时刻想着数据元的长度

    // 预测值 单位 uin16
    pixmap* pred;
    // 残差值 单位 int16
    // 残差由 residual 来计算
    pixmap* resi; 
    // 重建值 单位 uint8
    pixmap* cons;


    //解码残差要用的qp
    Qp_info qp;

    // 解码intra16x16 或者其他推测周围块是可能用到的周围信息
    MacroBlockNeigh neighbour;
    

    // 基本的deocde函数
    void decode();


    // 完成基本解码环境的分析
    void ParseHead();
    // 完成数据的解码
    void DecodeData();

    macroblock();
    macroblock(slice* s, parser* p);
    ~macroblock();



    uint8 get_PosByIndex(int index, int& r, int& c);
};






#endif