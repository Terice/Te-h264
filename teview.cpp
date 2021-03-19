#include <stdio.h>
#include <opencv2/opencv.hpp>
using namespace cv;
// 54 x 30 
unsigned char data[1048576];
int main(int argc, char* argv[])
{
    if(argc < 1) return -1;
    int w;//= atoi(argv[2]);
    int h;//= atoi(argv[3]);

    w = 54 * 16;
    h = 30 * 16;

    FILE* fp = fopen(argv[1], "r");
    fread(data, w * h, sizeof(char), fp);
    
    Mat m( h,w, CV_8UC1, data);
    imshow("res", m);
    waitKey(0);
}