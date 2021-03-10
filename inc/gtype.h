#ifndef GTYPE_H__
#define GTYPE_H__
#include "stdio.h"

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

// pix16 是给计算过程用的，保证超过255时不会溢出
typedef unsigned short pix16;
// pix8 是给最终的结果用的
typedef unsigned char  pix8;

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