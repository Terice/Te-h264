#ifndef MATRIX_H__
#define MATRIX_H__
#include "array2d.h"
#include <iostream>

typedef int matint;

typedef struct DataPkg__
{
    int *pointer;
    int length;
    int user;
}DataPkg;
// 专门用来计算的类
// 重写了数据的实现方式
// 坐标系是图像坐标系
class matrix
{
private:
    DataPkg *data;

    // // 数组运算检查
    // int comp(const matrix&);
    // // 边界检查
    // int edge(int i);
    // // 
    // int edge(int x, int y);
public:

    int w;
    int h;

    matrix operator<<(int right);
    matrix operator>>(int right);
    
    // 赋值等于会把一个局部变量变成固定的变量
    void operator=(const matrix&);
    // 给一个矩阵赋值为一个int值
    void operator=(int value);
    matrix operator+(int i);
    matrix operator+(const matrix&);
    // 相乘会返回一个新的 matrix，
    matrix operator*(const matrix&);
    matrix operator*(int i);
    // 矩阵的重载符号是按照[row][col]来访问的
    int* operator[](int i) const;


    
    
    friend std::ostream&  operator<<(std::ostream& out ,const matrix& ma);
    // bool Set_r(int row, int value);
    // int Sum_r(int row);
    // bool Set_c(int column, int value);
    // int Sum_c(int column);

    int  get(int i);
    bool set(int index, int i);
    bool set(int x, int y, int i);

    // 数组自己进行inverse4x4变换
    void inverse4x4();

    // 从 data 中复制 length 个数据到自己的空间中
    void from(int *data, int length);
    // 从 data 中复制 length 个数据到自己的空间中，但是从 start 开始
    void from(int *data, int start, int length);

    // 格式化这个数组
    // 返回的值是格式化是否成功
    // bool format(int w, int h);

    matrix(int width, int height, int value);
    matrix(int width, int height, int* res);
    matrix(int width, int height);
    matrix(const matrix& r);
    matrix();
    ~matrix();
};
#endif