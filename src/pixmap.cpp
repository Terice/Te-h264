#include "pixmap.h"
#include "terror.h"
#include "gvars.h"
#include <string.h>
pixmap::pixmap(void *data_res, int data_unit_length, area a)
{
    step = data_unit_length;
    res = data_res;
    all = a;
}
pixmap::~pixmap()
{
    // 注意 pixmap 是直接在原来的数组上进行操作的，所以 res 并不需要释放
    // 而data指针并不是指向数据区的，而是指向那些指向数据区指针的
    delete[] data;
}

bool  pixmap::checkpos(int x, int y)
{
    if(x < 0 || x > w || y < 0 || y > h) return false;
    else return true;
}
byte* pixmap::transpos(int x, int y)
{
    return data[y] + x * step;
}
bool pixmap::select(point start, area select)
{
    int x = start.x;
    int y = start.y;
    data = new byte*[select.w];
    h = select.h;
    w = select.w;

    // 宽度超过
    if(start.x + select.w > all.w) terr.error("[pixmap](width  overflow)");
    // 高度超过
    if(start.y + select.h > all.h) terr.error("[pixmap](height overflow)");

    // 得到选择部分的起始地址
    void *select_start = res + (y * all.w + x) * step;

    for (size_t i = 0; i < h; i++)
    {
        // 每次往前加上一整行的长度就是下一行的开始
        data[i] = (byte*)select_start + (i * all.w) * step;
    }
    return true;
}

bool pixmap::get(int x, int y, void *r)
{
    memcpy(r, transpos(x, y), step);
}
bool pixmap::set(int x, int y, void *v)
{
    memcpy(transpos(x, y), v, step);
}