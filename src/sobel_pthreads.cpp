#include "../include/sobel.h"
#include <pthread.h>
#include <cmath>
#include <algorithm>

struct ThreadData
{
    const std::vector<uint8_t>* input;
    std::vector<uint8_t>* output;
    int width;
    int height;
    int start_row;
    int end_row;
};

void* sobel_thread(void* arg)
{
    ThreadData* data = (ThreadData*)arg;

    const int Gx[3][3]={{-1,0,1},{-2,0,2},{-1,0,1}};
    const int Gy[3][3]={{1,2,1},{0,0,0},{-1,-2,-1}};

    for(int y=data->start_row;y<data->end_row;y++)
    {
        for(int x=1;x<data->width-1;x++)
        {
            int sumX=0,sumY=0;

            for(int j=-1;j<=1;j++)
            {
                for(int i=-1;i<=1;i++)
                {
                    int pixel=(*data->input)[(y+j)*data->width+(x+i)];
                    sumX+=pixel*Gx[j+1][i+1];
                    sumY+=pixel*Gy[j+1][i+1];
                }
            }

            int mag=sqrt(sumX*sumX+sumY*sumY);
            (*data->output)[y*data->width+x]=std::min(255,mag);
        }
    }

    return NULL;
}

void sobel_pthreads(const std::vector<uint8_t>& input,
                    std::vector<uint8_t>& output,
                    int width,int height,
                    int num_threads)
{
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];

    int rows_per_thread=height/num_threads;

    for(int i=0;i<num_threads;i++)
    {
        thread_data[i].input=&input;
        thread_data[i].output=&output;
        thread_data[i].width=width;
        thread_data[i].height=height;
        thread_data[i].start_row=i*rows_per_thread;
        thread_data[i].end_row=(i==num_threads-1)?height:(i+1)*rows_per_thread;

        pthread_create(&threads[i],NULL,sobel_thread,&thread_data[i]);
    }

    for(int i=0;i<num_threads;i++)
        pthread_join(threads[i],NULL);
}