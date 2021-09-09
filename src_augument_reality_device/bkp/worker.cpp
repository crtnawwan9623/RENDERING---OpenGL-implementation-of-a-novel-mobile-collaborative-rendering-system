#include <GL/glut.h>
#include <GL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <Windows.h>
#include "worker.h"

using namespace std;

Worker::Worker(int id)
{
    workerID=id;
    imagewidth=640;
    imageheight=480;
    BMP_Header_Length=54;
    BUF_SIZE=4096*512;
    string id_str=to_string(id);

    xrot=0;
    yrot=0;

    Name_sharedmemory="ShareMemory";
    Name_sharedmemory += id_str;
    Name_Mutex="Mutex";
    Name_Mutex+=id_str;
    Name_BmpRdyEvent="BmpRdyEvent";
    Name_BmpRdyEvent+=id_str;
    Name_MouseEvent="MouseEvent";
    Name_MouseEvent+=id_str;
    initSharedMemory();
}

Worker::~Worker()
{
    closeSharedMemory();
}

void Worker::initSharedMemory()
{
      //步骤1：创建共享文件句柄

    H_sharedfile = CreateFileMapping(

    INVALID_HANDLE_VALUE,//物理文件句柄

    NULL,  //默认安全级别

    PAGE_READWRITE,      //PAGE_READWRITE表示可读可写，PAGE_READONLY表示只读，PAGE_WRITECOPY表示只写

    0,  //高位文件大小

    BUF_SIZE,  //低位文件大小

    Name_sharedmemory.c_str()  //共享内存名称

    );  

    if (H_sharedfile == NULL)

    {

    cout<<"Could not create file mapping object..."<<endl;

    return;

    }

    //步骤2：映射缓存区视图，得到指向共享内存的指针

    lpBUF = MapViewOfFile(

    H_sharedfile, //已创建的文件映射对象句柄

    FILE_MAP_ALL_ACCESS,//访问模式:可读写

    0, //文件偏移的高32位

    0, //文件偏移的低32位

    BUF_SIZE //映射视图的大小

    );

    if (lpBUF == NULL)

    {

    cout << "Could not create file mapping object..." << endl;

    CloseHandle(H_sharedfile);

    return;

    }

    H_Mutex = CreateMutex(NULL, FALSE, Name_Mutex.c_str());

    H_BmpRdyEvent = CreateEvent(NULL, FALSE, FALSE, Name_BmpRdyEvent.c_str());

    H_MouseEvent = CreateEvent(NULL, FALSE, FALSE, Name_MouseEvent.c_str());
}

void Worker::closeSharedMemory()
{
    CloseHandle(H_Mutex);

    CloseHandle(H_BmpRdyEvent);
    CloseHandle(H_MouseEvent);

//步骤4：解除映射和关闭句柄

    UnmapViewOfFile(lpBUF);

    CloseHandle(H_sharedfile);
}

void Worker::SendCommand()
{
    float MousePos[2];
    MousePos[0]=xrot;
    MousePos[1]=yrot;

    if(WAIT_OBJECT_0 == WaitForSingleObject(H_Mutex, 100))//使用互斥体加锁
    {
      memcpy((float*)lpBUF, MousePos, 2*sizeof(float));
      ReleaseMutex(H_Mutex); //放锁
      SetEvent(H_MouseEvent);
      cout << "MousePos[0]=" <<MousePos[0] << " " << "MousePos[1]=" <<MousePos[1]<<endl;
    } 
}

void Worker::ReadBmp(GLubyte* MergedPixelData_w)
{
    GLubyte* BmpData;
    GLint i = imagewidth * 3;      // 得到每一行的像素数据长度
    while (i % 4 != 0)  // 补充数据，直到 i 是的倍数
        ++i;
    GLint PixelDataLength = i * imageheight;
    BmpData = (GLubyte*)malloc(BMP_Header_Length+PixelDataLength);

    if(WAIT_OBJECT_0 == WaitForSingleObject(H_BmpRdyEvent, 0))
    {    
        if(WAIT_OBJECT_0 == WaitForSingleObject(H_Mutex, 100))//使用互斥体加锁
        {
            //load newly updated rendering result,200 is ofset of bmp info.
            memcpy(BmpData, (GLubyte*)lpBUF+200, BMP_Header_Length+PixelDataLength);
            ReleaseMutex(H_Mutex); //放锁
          //  cout<<int(BmpData[0x0012])<<" "<<int(BmpData[0x0013])<<" "<<int(BmpData[0x0014])<<" "<<int(BmpData[0x0015])<<endl;
          //  here we set up only for subimage 1, PixelDataLength should be added to MergedPixelData for subimage 2
            if (workerID == 1)
            memcpy(MergedPixelData_w, &BmpData[BMP_Header_Length], PixelDataLength);
            else if (workerID == 2)
            memcpy(MergedPixelData_w+PixelDataLength, &BmpData[BMP_Header_Length], PixelDataLength);
            glutPostRedisplay();
        } 
    }

    free(BmpData);
}