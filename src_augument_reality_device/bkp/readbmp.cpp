#include <GL/glut.h>
#include <GL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <Windows.h>
#include "worker.h"

#define FileName "merged_real-time-grab.bmp"
#define SourceFileName1 "../../src/build/real-time-grab1.bmp"
#define SourceFileName2 "../../src/build/real-time-grab2.bmp"

using namespace std;

Worker * worker1=NULL;
Worker * worker2=NULL;
int active=0;
static GLint MergedImageWidth;
static GLint MergedImageHeight;
static GLint MergedPixelLength;
static GLubyte* MergedPixelData;


// Rotation interface
float xrot = 0;
float yrot = 0;
float xdiff = 0;
float ydiff = 0;
bool mouseDown = false;

int alpha=0;

int mergebitmap(const char* mergedFileName, const char* fileName1,const char* fileName2);
int initgrabpixelinfo(void);

void mouseMotionCB(int x, int y)
{
  if (mouseDown)
    {
      if (active == 1){
        worker1->yrot = x - xdiff;
        worker1->xrot = y + ydiff;
        worker1->SendCommand();
      }
      else if (active == 2){
        worker2->yrot = x - xdiff;
        worker2->xrot = y + ydiff;
        worker2->SendCommand();
      }
      cout<<"x="<<x<<"y="<<y<<endl;
    }
}

void mouseCB(int button, int state, int x, int y)
{
  switch(button){
  case GLUT_LEFT_BUTTON:
    switch(state)
      {
      case GLUT_DOWN:
	    mouseDown = true;
        if (y>480){
            xdiff = x - worker1->yrot;
            ydiff = -y + worker1->xrot;
            active = 1;
        }
        else{
            xdiff = x - worker2->yrot;
            ydiff = -y + worker2->xrot;
            active = 2;
        }
	    break;
      case GLUT_UP:
	    mouseDown = false;
        active = 0;
	    break;
      default:
	    break;
      }
    break;    
  case GLUT_MIDDLE_BUTTON:
    break;
  default:
    break;
  }
}

void display(void)
{
    // 清除屏幕并不必要
    // 每次绘制时，画面都覆盖整个屏幕
    // 因此无论是否清除屏幕，结果都一样
    // glClear(GL_COLOR_BUFFER_BIT);
    // 绘制像素
    
    //try to load newly updated rendering result from shared memeory for subimage1

    glDrawPixels(MergedImageWidth, MergedImageHeight,
        GL_BGR_EXT, GL_UNSIGNED_BYTE, MergedPixelData);
    // 完成绘制
    glutSwapBuffers();
}
void OnTimer(int value)
{
   alpha++;
   alpha=(alpha%256);
 //  glutPostRedisplay();
    worker1->ReadBmp(MergedPixelData);
    worker2->ReadBmp(MergedPixelData);
   glutTimerFunc(100, OnTimer, 1);
}

int main(int argc, char* argv[])
{
    initgrabpixelinfo();
    worker1=new Worker(1);
    worker2=new Worker(2);

    // 初始化GLUT并运行
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(MergedImageWidth, MergedImageHeight);
    glutCreateWindow(FileName);
    glutDisplayFunc(&display);
    glutTimerFunc(100, OnTimer, 1);
    glutMouseFunc(mouseCB);
    glutMotionFunc(mouseMotionCB);
    glutMainLoop();

    free (MergedPixelData);
    delete worker1;
    delete worker2;
    return 0;
}



int mergebitmap(const char* mergedFileName, const char* fileName1,const char* fileName2)
{
    FILE* pDummyFile1;
    FILE* pDummyFile2;
    FILE* pWritingFile;
    GLint PixelDataLength;
    GLubyte* PixelData_tmp;
    GLubyte BMP_Header[54];
    GLint i, j;
    GLint width, height;
    // reading two source bmp file and create a merged bmp file
    pDummyFile1 = fopen(fileName1, "rb");
    if (pDummyFile1 == 0){
        printf("fail open source file1\n");
        return 0;
    }
    pDummyFile2 = fopen(fileName2, "rb");
    if (pDummyFile1 == 0){
        printf("fail open source file2\n");
        return 0;
    }
        
    pWritingFile = fopen(mergedFileName, "wb");
    if (pWritingFile == 0){
        printf("fail open merged file_1\n");
        return 0;
    }
        
    
    fseek(pDummyFile1, 0x0012, SEEK_SET);
    fread(&width, sizeof(width), 1, pDummyFile1);
    fread(&height, sizeof(height), 1, pDummyFile1);

    // set up the head information in the merged file
    // copy the whole head info. from a source file, and change the height parameter at address 0x12
    fseek(pDummyFile1, 0, SEEK_SET);
    fread(BMP_Header, sizeof(BMP_Header), 1, pDummyFile1);
    fseek(pWritingFile, 0, SEEK_SET);
    fwrite(BMP_Header, sizeof(BMP_Header), 1, pWritingFile);
    fseek(pWritingFile, 0x0012, SEEK_SET);
    i = width;
    j = height*2;
    fwrite(&i, sizeof(i), 1, pWritingFile);
    fwrite(&j, sizeof(j), 1, pWritingFile);

    // set up buffer for reading pixel info from source bmp file
    i = width * 3;      // 得到每一行的像素数据长度
    while (i % 4 != 0)  // 补充数据，直到 i 是的倍数
        ++i;
    PixelDataLength = i * height;
    PixelData_tmp = (GLubyte*)malloc(PixelDataLength);

    // read pixel info from each source bmp file and write it into merged file
    fseek(pDummyFile1, 54, SEEK_SET);
    fread(PixelData_tmp, PixelDataLength, 1, pDummyFile1);
    fseek(pWritingFile, 0, SEEK_END);
    fwrite(PixelData_tmp, PixelDataLength, 1, pWritingFile);
    fseek(pDummyFile2, 54, SEEK_SET);
    fread(PixelData_tmp, PixelDataLength, 1, pDummyFile2);
    fseek(pWritingFile, 0, SEEK_END);
    fwrite(PixelData_tmp, PixelDataLength, 1, pWritingFile);
    fclose(pDummyFile1);
    fclose(pDummyFile2);
    fclose(pWritingFile);
    free(PixelData_tmp);
    return 1;
}

int initgrabpixelinfo(void)
{
    // merge source bmp files
    if (!mergebitmap(FileName, SourceFileName1,SourceFileName2))
        return 0;

    // 打开文件
    FILE* pFile = fopen(FileName, "rb");
    if (pFile == 0){
        printf("fail open merged file_2\n");
        return 0;
    }
        
    // 读取图象的大小信息
    fseek(pFile, 0x0012, SEEK_SET);
    fread(&MergedImageWidth, sizeof(MergedImageWidth), 1, pFile);
    fread(&MergedImageHeight, sizeof(MergedImageHeight), 1, pFile);
    // 计算像素数据长度
    MergedPixelLength = MergedImageWidth * 3;
    while (MergedPixelLength % 4 != 0)
        ++MergedPixelLength;
    MergedPixelLength *= MergedImageHeight;
    // 读取像素数据

    static bool flag_1st=false;
    while (!flag_1st){
        MergedPixelData = (GLubyte*)malloc(MergedPixelLength);
        if (MergedPixelData == 0)
        exit(0);
        flag_1st=true;
    }
 

    fseek(pFile, 54, SEEK_SET);
    fread(MergedPixelData, MergedPixelLength, 1, pFile);
    // 关闭文件
    fclose(pFile);
    return 1;
}