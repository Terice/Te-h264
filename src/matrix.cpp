#include "matrix.h"
#include <string.h>
#include <ostream>
#include "terror.h"
#include "gvars.h"
#include <stdlib.h>

#define exiterr(str) {std::cout << str << std::endl;exit(-1);}   

static unsigned long int datapkg_cout = 0;

// 采用的是类似于文件硬链接的方式来处理数据包
// 从而来解决内存泄漏的问题

static void increase(DataPkg* pkg)
{
    if(!pkg) return;
    pkg->user++;
}
static DataPkg* newpkg(int length)
{

    DataPkg *pkg = new DataPkg;
    pkg->pointer = new int[length];
    // std::cout << " new pkg : cur: " << ++datapkg_cout << std::endl;
    pkg->user = 0;
    pkg->length = length;

    return pkg;
}
static void delpkg(DataPkg* pkg)
{
    if(pkg->user == 0) 
    {
        delete[] pkg->pointer;
        delete pkg;
        // std::cout << " del pkg : cur: " << --datapkg_cout << std::endl;
    }
}
static void decrease(DataPkg* pkg)
{
    if(!pkg) return;

    pkg->user--;
    delpkg(pkg);
}


std::ostream&  operator<<(std::ostream& out ,const matrix& ma)
{
    for (size_t r = 0; r < ma.h; r++)
    {
        if(r % 4 == 0) 
        {
            printf("%-2zu", r);
            for(int i = 0; i < ma.h; i+=4) 
                printf("---------------------------%-2lu", i/4 + 1);
            printf("\n");
        }
        
        for (int c = 0; c < ma.w; c++)
        {
            if(c % 4 == 0) out << "|";
            printf("%6d ", ma[r][c]);
        }

        printf("|\n");
    }
    printf("%-2d", ma.w);
    for(int i = 0; i < ma.w; i+=4) 
        printf("---------------------------%-2lu", i/4 + 1);
    return out;
}
matrix::matrix(const matrix& r)
{
    decrease(data);// 丢掉之前自己的包
    data = r.data;
    increase(data);// 这个包的使用者加一
    this->w = r.w;
    this->h = r.h;
    
}
matrix::matrix(int w, int h, int* res)
{
    this->w = w;
    this->h = h;

    data = newpkg(w*h);
    increase(data);
     
    memcpy(data->pointer, res, data->length * sizeof(int));
}
matrix::matrix(int w, int h, int v)
{
    this->w = w;
    this->h = h;

    data = newpkg(w*h);
    increase(data);

    if(v == -1 || v == 0)
        memset(data->pointer, v, data->length * sizeof(int));
    else
        for (int i = 0; i < data->length; i++)
        {
            data->pointer[i] = v;
        }
}
matrix::matrix(int w, int h)
{
    data = NULL;
    this->w = w;
    this->h = h;
}
matrix::~matrix()
{
    decrease(data);
}



    
void matrix::operator=(int value)
{
    for (int i = 0; i < data->length; i++)
    {
        data->pointer[i] = value;
    }
    
}
inline int* matrix::operator[](int i) const
{
    return i >= h ? NULL : data->pointer + i * w;
}

matrix matrix::operator<<(int right)
{
    if(right < 0) return (*this);
    matrix result(w, h, 0);

    for (int i = 0; i < data->length; i++)
    {
        result.data->pointer[i] = this->data->pointer[i] >> right;
    }
    
    return result;
}
matrix matrix::operator>>(int right)
{
    if(right < 0) return (*this);
    matrix result(w, h, 0);

    for (int i = 0; i < data->length; i++)
    {
        result.data->pointer[i] = this->data->pointer[i] >> right;
    }
    
    return result;
}
void matrix::operator=(const matrix& r)
{
    // 因为自己马上不会用到自己的数据包了
    decrease(data);

    this->w = r.w;
    this->h = r.h;

    this->data = r.data;
    increase(data);
}
matrix matrix::operator+(int right)
{
    matrix result(this->w, this->h, 0);
    for (int i = 0; i < data->length; i++)
    {
        result.data->pointer[i] = this->data->pointer[i] + right;
    }
    return result;
}
matrix matrix::operator+(const matrix& r)
{
    if(this->w != r.w || this->h != r.h)      //抛出一个错误
    exiterr(">> matrix: add size no compitiable");

    matrix result(r.w, r.h, 0);


    for (uint i = 0; i < this->h; i++)
    {
        for (uint j = 0; j < this->w; j++)
        {result[i][j] = (*this)[i][j] + r[i][j];}
    }
    return result;
}
matrix matrix::operator*(int right)
{
    matrix result(this->w, this->h, 0);
    for (int i = 0; i < data->length; i++)
    {
        result.data->pointer[i] = this->data->pointer[i] * right;
    }
    return result;
}
matrix matrix::operator*(const matrix& right)
{
    if(this->w != right.h || this->h != right.w)      //抛出一个错误
    exiterr(">> matrix: nul size no compitiable");

    matrix result(right.w, this->h, 0);

    int r = result.h, c = result.w;
    int result_cur = 0;
    for(size_t r_cur = 0; r_cur < r; r_cur++)
    {
        for(size_t c_cur = 0; c_cur < c; c_cur++)
        {
            // 对于 result 的每一个坐标上的元素，做如下操作
            
            result_cur = 0;
            for (uint8_t cur = 0; cur < right.h; cur++)
            {
                result_cur += (*this)[r_cur][cur] * right[cur][c_cur];
            }
            result[r_cur][c_cur] = result_cur;
        }
    }

    return result;
}



int  matrix::get(int i)
{
    return data->pointer[i];
    return true;
}
bool matrix::set(int index, int value)
{
    data->pointer[index] = value;
    return true;
}
bool matrix::set(int x, int y, int i)
{
    (*this)[x][y] = i;
    return true;
}
bool matrix::setr(int row, int value)
{
    for (int c = 0; c < this->w; c++)
    {
        (*this)[row][c] = value;
    }
}
void matrix::from(int *data, int length)
{
    memcpy(this->data->pointer, data, length * sizeof(int));
    this->data->length = length;
}
void matrix::from(int *data, int start, int length)
{
    memcpy(this->data->pointer + start, data, length * sizeof(int));
    this->data->length = start + length;
}