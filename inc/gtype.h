#ifndef GTYPE_H__
#define GTYPE_H__


// 这里用来定义全局使用的数据类型
typedef char  int8;
typedef short int16;
typedef int   int32;
typedef long  int64;

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef unsigned long  uint64;

typedef unsigned char  byte;

typedef struct point__
{
    int x;
    int y;
}point;
typedef struct area__
{
    int w;
    int h;
}area;

#endif