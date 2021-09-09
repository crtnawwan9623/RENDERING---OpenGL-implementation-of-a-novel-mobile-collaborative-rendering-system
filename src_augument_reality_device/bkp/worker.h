#ifndef __MODEL_H__
#define __MODEL_H__

#include <GL/glut.h>
#include <GL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <Windows.h>
#include <string.h>

using namespace std;

class Worker {
private:
    void initSharedMemory();
    void closeSharedMemory();
    HANDLE H_Mutex = NULL;    // lock for accessing the shared memory
    HANDLE H_BmpRdyEvent = NULL;    // event for new tmp file updates in the shared memeory; sent from worker, received by owner
    HANDLE H_MouseEvent=NULL;  // event for mouse drag updates in the shared memeory; received by worker, sent from owner
    HANDLE H_sharedfile=NULL;  // handle for shared memeory for this worker 
    LPVOID lpBUF=NULL;
    int BMP_Header_Length;
    int BUF_SIZE;
    string Name_sharedmemory;
    string Name_Mutex;
    string Name_BmpRdyEvent;
    string Name_MouseEvent;
public:
    Worker(int id);
    ~Worker();

    int workerID;
    int imagewidth;
    int imageheight;
    float xrot;
    float yrot;
    
    void SendCommand();
    void ReadBmp(GLubyte* MergedPixelData_w);
    

};
#endif //__MODEL_H__

