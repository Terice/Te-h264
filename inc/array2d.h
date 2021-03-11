#ifndef ARRAT2D_H__
#define ARRAT2D_H__
#include <iostream>


// 还是把传入和返回的指针改成了值，总觉得指针很反人类
/// 用的时候不要放类的实例就行了，取值运算传一个指针可能不比赋值传递快多少

/*
 * 注意这个array的坐标系，
    0----> x
    |
    V y

 * 使用这个模板时，需要有赋值等于函数的重载
 * 如果没有，就用类的指针而不是直接用类
*/
template <class T>
class array2d
{
private:
public:
    T* data;
    // 宽
    int w;
    // 高
    int h;
    // 总长
    int l;


    /// @param x 横向的坐标
    /// @param y 纵向的坐标
    T   get(int x,  int y);
    
    /// @param i 一维数据上的坐标
    T   get(int i);

    // 设置[x,y]处的值
    // 因为值传递会引起参数值传递的问题
    // 所以这里用指针传递，要想好
    /// @param x 横向的坐标
    /// @param y 纵向的坐标
    /// @param value 要设置的值的->指针<-
    bool set(int x,  int y, T value);
    // 因为值传递会引起参数值传递的问题 
    // 所以这里用指针传递，要想好 
    // 设置一位长度i上的值 
    /// @param i 坐标
    /// @param value 要设置的值的->指针<-
    bool set(int i, T value);
    // 循环遍历设置值
    bool set(T value);

    bool is_avaiable(int x,  int y);
    
    bool operator=(array2d<T>& right);
    T* operator[](int i);
    
    array2d(int xinitlength,  int yinitlength, const T initvalue);
    array2d(int xinitlength,  int yinitlength);
    array2d();
    ~array2d();
};

template <class T>
T array2d<T>::get(int i)
{
    return data + i;
}
template <class T>
T array2d<T>::get(int x,  int y)
{
    return data + y * w + x;
}
template <class T>
bool array2d<T>::set(int x,  int y, T value)
{
    if(x >= w || y >= y || x < 0 || y < 0) return false;
    else {data[y * w + x] = *value; return true;}
}
template <class T>
bool array2d<T>::set(int i, T value)
{
    if(i >= w * h || i < 0) return false;
    else {data[i] = value; return true;}
}
template <class T>
bool array2d<T>::set(T value)
{
    for(int i = 0; i < l; i++) 
    {data[i] = value;}

    return true;
}

template <class T>
// 按照宽高来初始化一个array2d
array2d<T>::array2d(int width,  int height)
{
    w = width;
    h = height;
    l = w * h;
    data = new T[l];
}
template <class T>
// 按照宽高来初始化，并且赋初始值
array2d<T>::array2d(int widht,  int height, const T initvalue)
{
    w = widht;
    h = height;
    l = w * h;
    data = new T[l];
    for(int i = 0; i < w * h; i++) 
    {data[i] = initvalue;}
}
// 初始化一个空的array2d
template <class T>
array2d<T>::array2d()
{
    w = 0;
    h = 0;
    data = NULL;
}
// 释放数据
template <class T>
array2d<T>::~array2d()
{
    if(data)
    {
        delete[] data;
        data = NULL; 
    }
}


#endif