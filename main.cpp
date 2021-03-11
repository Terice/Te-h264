#include "terror.h"
#include "gvars.h"
#include "matrix.h"
#include <iostream>
int main(int argc, char* argv[])
{
    int i =1, j =0, k =3;
    int value = (--i || ++j && k++);
    std::cout << value << std::endl;

    // 5e(1+4);
    int x = 0, y = 1;
    int a = 0, b = 1;
    ++x||++y;
    std::cout << x << " " << y << std::endl;
    
	return 0;
}