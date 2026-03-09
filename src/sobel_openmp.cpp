#include "../include/sobel.h"
#include <omp.h>
#include <cmath>
#include <algorithm>

void sobel_openmp(const std::vector<uint8_t>& input,
                  std::vector<uint8_t>& output,
                  int width, int height)
{
    const int Gx[3][3] = {{-1,0,1},{-2,0,2},{-1,0,1}};
    const int Gy[3][3] = {{1,2,1},{0,0,0},{-1,-2,-1}};

    #pragma omp parallel for
    for(int y=1;y<height-1;y++)
    {
        for(int x=1;x<width-1;x++)
        {
            int sumX=0;
            int sumY=0;

            for(int j=-1;j<=1;j++)
            {
                for(int i=-1;i<=1;i++)
                {
                    int pixel=input[(y+j)*width+(x+i)];
                    sumX+=pixel*Gx[j+1][i+1];
                    sumY+=pixel*Gy[j+1][i+1];
                }
            }

            int mag=sqrt(sumX*sumX+sumY*sumY);
            output[y*width+x]=std::min(255,mag);
        }
    }
}