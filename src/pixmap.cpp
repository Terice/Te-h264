#include "pixmap.h"
#include "terror.h"
#include "gvars.h"
#include <string.h>

pixmap::pixmap(byte *data, int w, int h)
{
    res.pointer = data;
    res.length = w * h;
    res.w = w;
    res.h = h;

    this->w = 16;
    this->h = 16;
    this->selected = false;
    
    this->pix = NULL;
}
pixmap::pixmap(pixmap *p)
{
    this->res = p->res;

    this->selected = false;

    this->pix = p;
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
    return data[y] + x ;
}


byte* pixmap::operator[](int i) const
{
    if(!selected)      terr.error("[pixmap]:area not select");
    return data[i];
}
bool pixmap::select(int x, int y, int width, int height)
{
    selected = true;
    if(x < 0 || y < 0) terr.error("[pixmap]:position < 0");
    if(!pix)
    {
        this->w = width;this->h = height;
        if(x+w > res.w || y+h > res.h) 
            terr.error("[pixmap]55:position overflow");
        else
        {
            data = new byte*[h];
            byte* start = res.pointer + y * res.w + x;
            for (int i = 0; i < h; i++)
            {
                data[i] = start + i * res.w;
            }
        }
    }
    else
    {
        this->w = width;this->h = height;
        // 上一个选区一定是合法的
        // 所以这里只需要在上一次选择的区域之中判断
        if(x+w > pix->w || y+h > pix->h) 
            terr.error("[pixmap]70:position overflow");
        else
        {
            data = new byte*[h];
            byte* tmp;
            for (int i = 0; i < h; i++)
            {
                data[i] = pix->data[i + y] + x;
            }
        }
    }
    return true;
}



std::ostream&  operator<<(std::ostream& out ,const pixmap& ma)
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
    