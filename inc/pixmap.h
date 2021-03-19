#ifndef PIXMAP_H__
#define PIXMAP_H__
#include "gtype.h"
class matrix;
#include <ostream>

// pixmap 用来构造像素级的操作，表示对 像素 的操作
// 以选择的区域来构造一个pixmap，表示对这个选择的区域的操作
// 这个对象利用 void* 重新实现对数据的地址级访问
// 数据必须是一个一维连续的，并且必须指明格式化长度和宽度
// 还必须指明数据的长度
// 只适用于数据的存储和转存，而不是适合用来进行计算
// ！
// 由于这个对象对于数据的长度要求十分严格，
// 所以使用之前一定要对齐数据的格式
// ！

typedef struct DataRes__
{
    // 源数据来源指针
    byte *pointer;
    // 源数据的长度
    int length;
    // 源数据的每一行的长度
    int w;
    int h;
}DataRes;


// 不采用上面的描述了，
// 一律使用 Byte 数据类型
// 只表示对最终的图像的操作
// 指针的一层封装而已
class pixmap
{
private:
    DataRes res;

    // 表明当前数据是不是来源于源一位数组的数据
    pixmap *pix;

    bool selected;

    // void* 数组的起始地址
    byte **data;


    // x,y 坐标变换为 地址
    byte* transpos(int x, int y);
    // 检查x,y这个地址是否是pixmap内有效的
    bool  checkpos(int x, int y);
public:
    // 区域的宽 高
    int h;
    int w;
    // 以 data 作为数据来源
    // 还需要说明源数据的格式化宽高
    pixmap(byte *data, int w, int h);
    // 以 pixmap 作为数据来源
    pixmap(pixmap *p);
    ~pixmap();

    // 选择合适的区域
    /// @param x  样点起始的位置
    /// @param y  样点起始的位置
    /// @param w  所需要选择的区域的宽
    /// @param h  所需要选择的区域的高
    /// @return 所选择的区域是否正确，不正确则会致命错误退出
    bool select(int x, int y, int w, int h);

    // 因为这个类只需要指针，而不需要对数据复杂的运算
    // 所以重载的符号比较少

    // 与matrix保持读取一致的符号重载
    // 不过注意这里的是uint8类型的
    byte* operator[](int i) const;
    // 全区赋值
    void operator=(byte i);

    friend std::ostream&  operator<<(std::ostream& out ,const pixmap& ma);
    
    // 由于pixmap不再使用一维数组，所以没有i这个概念了
    // 由于原data固定为行优先的方式，所以这里采用行优先

    // 从 matrix 中格式化复制进自己
    // 二者的宽高一定要相等，
    // 数据按值复制
    bool from(matrix* m);

    void print(bool sig);
};


#endif