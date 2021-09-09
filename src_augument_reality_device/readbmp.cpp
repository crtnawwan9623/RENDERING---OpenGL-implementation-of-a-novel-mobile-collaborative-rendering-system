#include <GL/glut.h>
#include <GL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <Windows.h>
#include "worker.h"

#define FileName "merged_real-time-grab.bmp"
#define SourceFileName1 "real-time-grab1.bmp"
#define SourceFileName2 "real-time-grab2.bmp"

using namespace std;

Worker * worker1=NULL;
Worker * worker2=NULL;
Worker * worker3=NULL;
int active=0;
GLint MergedImageWidth=640;
GLint MergedImageHeight=750;
GLint MergedPixelLength;
GLubyte* MergedPixelData;


// Rotation interface
float xrot = 0;
float yrot = 0;
float xdiff = 0;
float ydiff = 0;
bool mouseDown = false;

int alpha=0;

int mergebitmap(const char* mergedFileName, const char* fileName1,const char* fileName2);
int initgrabpixelinfo(void);
int InitMergedata(void);
int MergeBmpData(int x0, int y0, int w, int h);

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
      else if (active == 3){
        worker3->yrot = x - xdiff;
        worker3->xrot = y + ydiff;
        worker3->SendCommand();
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
        if (worker1->InWindow(x,y)){
            xdiff = x - worker1->yrot;
            ydiff = -y + worker1->xrot;
            active = 1;
        }
        else if (worker2->InWindow(x,y)){
            xdiff = x - worker2->yrot;
            ydiff = -y + worker2->xrot;
            active = 2;
        }
        else if  (worker3->InWindow(x,y)){
            xdiff = x - worker3->yrot;
            ydiff = -y + worker3->xrot;
            active = 3;
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

    
    //try to load newly updated rendering result from shared memeory for subimage1

    glDrawPixels(MergedImageWidth, MergedImageHeight,
        GL_BGR_EXT, GL_UNSIGNED_BYTE, MergedPixelData);
    // draw the screen
    glutSwapBuffers();
}
void OnTimer(int value)
{
   alpha++;
   alpha=(alpha%256);
 //  glutPostRedisplay();
    worker1->ReadBmp(MergedPixelData);
    worker2->ReadBmp(MergedPixelData);
    worker3->ReadBmp(MergedPixelData);
   glutTimerFunc(100, OnTimer, 1);
}

int main(int argc, char* argv[])
{
 //   initgrabpixelinfo();
    InitMergedata();
    worker1=new Worker(1,0,0,320,240);
    worker2=new Worker(2,320,0,320,240);
    worker3=new Worker(3,0,250,640,480);

    // Init GLUT and run 
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(MergedImageWidth, MergedImageHeight);
    glutCreateWindow("GPU requester");
    glutDisplayFunc(&display);
    glutTimerFunc(100, OnTimer, 1);
    glutMouseFunc(mouseCB);
    glutMotionFunc(mouseMotionCB);
    glutMainLoop();

    free (MergedPixelData);
    delete worker1;
    delete worker2;
    delete worker3;
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
    i = width * 3;      // get the pixel of a row 
    while (i % 4 != 0)  // the data has to be mod of 4 
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

    // open file
    FILE* pFile = fopen(FileName, "rb");
    if (pFile == 0){
        printf("fail open merged file_2\n");
        return 0;
    }
        
    // get the image size
    fseek(pFile, 0x0012, SEEK_SET);
    fread(&MergedImageWidth, sizeof(MergedImageWidth), 1, pFile);
    fread(&MergedImageHeight, sizeof(MergedImageHeight), 1, pFile);
    // calculate the number of pixels 
    MergedPixelLength = MergedImageWidth * 3;
    while (MergedPixelLength % 4 != 0)
        ++MergedPixelLength;
    MergedPixelLength *= MergedImageHeight;
    // read pixel 

    static bool flag_1st=false;
    while (!flag_1st){
        MergedPixelData = (GLubyte*)malloc(MergedPixelLength);
        if (MergedPixelData == 0)
        exit(0);
        flag_1st=true;
    }
 

    fseek(pFile, 54, SEEK_SET);
    fread(MergedPixelData, MergedPixelLength, 1, pFile);
    // close file
    fclose(pFile);
    return 1;
}

/* init the mergedpixeldata*/
int InitMergedata(void)
{
    MergedPixelLength = MergedImageWidth * 3;
    while (MergedPixelLength % 4 != 0)
        ++MergedPixelLength;
    MergedPixelLength *= MergedImageHeight;
    // read pixel data 

    MergedPixelData = (GLubyte*)malloc(MergedPixelLength);
    if (MergedPixelData == 0)
        exit(0);
    memset(MergedPixelData,0,MergedPixelLength);

//    MergeBmpData(320, 720, 320, 240);
    return 1;
}

int MergeBmpData(int x0, int y0, int w, int h)
{
    FILE *file=fopen("real-time-grab1.bmp","rb");
    if (file==0){
        printf("fail open real-time-grab1\n");
        exit(0);
    }
    int Widthdatalength=w*3;
    while (Widthdatalength%4 !=0){
        Widthdatalength++;
    }

    int MergedWidthdatalength= MergedImageWidth * 3;
    while (MergedWidthdatalength%4 !=0){
        MergedWidthdatalength++;
    }

    int Totaldatalength = Widthdatalength*h;
    GLubyte* PixelData_tmp = (GLubyte*)malloc (Totaldatalength);
    if (PixelData_tmp == 0){
        printf("fail get memory PixelData_tmp\n");
        exit(0);
    }
        

    fseek(file,54,SEEK_SET);
    fread(PixelData_tmp,Totaldatalength,1,file);
    


    int heightgap=MergedImageHeight-y0-h;
    for (int i=0;i<h;i++){
        memcpy(&MergedPixelData[heightgap*MergedWidthdatalength+x0*3],&PixelData_tmp[i*Widthdatalength],w*3);
        heightgap+=1;
    }

    fclose(file);
    free(PixelData_tmp);
    return 1;
}

