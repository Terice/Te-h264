#ifndef PIXMAP_H__
#define PIXMAP_H__
#include "gtype.h"
class matrix;

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


// 不采用上面的描述了，
// 一律使用 Byte 数据类型
// 只表示对最终的图像的操作
// 指针的一层封装而已
class pixmap
{
private:
    // 每一个像素的数据宽 
    // 也就是数据区中每个元数据的长度
    // 以字节作为单位
    int step;
    // 源数据来源指针
    void *res;
    // 源数组的格式化形式
    area all;

    // void* 数组的起始地址
    byte **data;
    // 区域的宽 高
    int h;
    int w;
    // x,y 坐标变换为 地址
    byte* transpos(int x, int y);
    // 检查x,y这个地址是否是pixmap内有效的
    bool  checkpos(int x, int y);
public:
    // 用来格式化数据区内的格式
    /// @param data 数据起始地址的指针
    /// @param data_unit_lenght 必须指明每个数据的的长度
    /// @param all  一定要指明在原来的结构中整个数据是如何分割的，这个是用来做边界检查的
    pixmap(void *data, int data_unit_length, area all);
    // 选择子区域
    /// @param p 从已有的pixmap中继续选择子区域
    /// @param start  样点起始的位置
    /// @param select 所需要选择的区域的宽和高
    /// @return 所选择的区域是否正确，不正确则会致命错误退出
    pixmap(pixmap* p, point start, area select);
    pixmap();
    ~pixmap();

    // 因为这个类只需要指针，而不需要对数据复杂的运算
    // 所以重载的符号比较少

    // 与matrix保持读取一致的符号重载
    // 不过注意这里的是uint8类型的
    byte* operator[](int i) const;

    void operator=(int i);

    
    // 选择合适的区域
    /// @param start  样点起始的位置
    /// @param select 所需要选择的区域的宽和高
    /// @return 所选择的区域是否正确，不正确则会致命错误退出
    bool select(point start, area select);

    // 由于pixmap不再使用一维数组，所以没有i这个概念了
    // 由于原data固定为行优先的方式，所以这里采用行优先



    // 下面可能选择废弃



    // 获取第 i 行的起始地址
    void *get(int row);
    // 获取(x,y)处的值并且赋给r所指的地址
    // 注意数据长度对齐！
    bool get(int x, int y, void *r);
    // 将v所指的值赋给(x,y)处
    // 注意数据长度对齐！
    bool set(int x, int y, void *v);
    // 从res 中读入 length 个data，输入 pixmap 的第y行中
    // setl 永远都是按行复制，注意
    // 注意数据长度对齐！
    bool setl(void* res, int y, int length);
    
    // 注意数据长度对齐！
    bool setr(void *v, int row);

    // 注意数据长度对齐！
    bool setc(void *v, int col);

    // 将整个区域的值都设置为v所指的值
    // 注意数据长度对齐！
    bool seta(void *v);

    // 从 matrix 中格式化复制进自己
    // 二者的宽高一定要相等，
    // 注意数据长度对齐！
    // 数据按值复制
    bool from(matrix* m);

    void print(bool sig);
};


#endif