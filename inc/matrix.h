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
    matrix operator+(int i);
    matrix operator+(const matrix&);
    // 相乘会返回一个新的 matrix，
    matrix operator*(const matrix&);
    matrix operator*(int i);
    int* operator[](int i) const;


    
    
    
    //带等号的运算符都直接在源对象上面直接赋值
    matrix operator+=(const matrix&);
    matrix operator+=(int);
    matrix operator*=(int);
    matrix operator*=(matrix& right);
    matrix operator/=(int);
    matrix operator>>=(int);
    matrix operator<<=(int);

    friend std::ostream&  operator<<(std::ostream& out ,const matrix& ma);
    // bool Set_r(int row, int value);
    // int Sum_r(int row);
    // bool Set_c(int column, int value);
    // int Sum_c(int column);

    int get(int i);
    bool set(int index, int i);
    bool set(int x, int y, int i);

    // 数组自己进行inverse4x4变换
    void inverse4x4();
    // 
    void from(int *data, int length);
    // 
    void from(int *data, int start, int length);

    matrix(int width, int height, int value);
    matrix(int width, int height, int* res);
    matrix(int width, int height);
    matrix(const matrix& r);
    matrix();
    ~matrix();
};
#endif