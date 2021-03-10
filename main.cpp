#include "terror.h"
#include "gvars.h"
#include "matrix.h"
#include <iostream>
int main(int argc, char* argv[])
{
    int a[16];
    for (int i = 0; i < 16; i++)
    {
        a[i] = i;
    }
    matrix cur(4, 4, a);
    
    for (size_t i = 0; i < 1500000; i++)
    {
        matrix tmp(4,4, 10);
        std::cout << tmp * cur << std::endl;
        tmp = cur;
        tmp = (tmp * cur);
        cur = tmp;
        // std::cout << i << std::endl;
    }
    
	return 0;
}