// Include standard headers
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>



// glm::vec3, glm::vec4, glm::ivec4, glm::mat4
#include <glm/glm.hpp>
// glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>


#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>

#include "shader.h"
#include "libobj.h"


#include <iostream>
#include <string.h>
#include <Windows.h>

using namespace std;


HANDLE shared_file=NULL;
LPVOID lpBUF=NULL;


HANDLE H_Mutex = NULL;    // lock for accessing the shared memory
HANDLE H_BmpRdyEvent = NULL;    // event for new tmp file updates in the shared memeory; sent from worker, received by owner
HANDLE H_MouseEvent=NULL;  // event for mouse drag updates in the shared memeory; received by worker, sent from owner
HANDLE H_sharedfile=NULL;  // handle for shared memeory for this worker 

string Name_sharedmemory="ShareMemory1";
string Name_Mutex="Mutex1";
string Name_BmpRdyEvent="BmpRdyEvent1";
string Name_MouseEvent="MouseEvent1";


GLMmodel* objModel;

GLuint programID;
GLuint modelID;
GLuint viewID;
GLuint projectionID;

glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;

GLuint vertexbuffer;
GLuint normalbuffer;

// Rotation interface
float xrot = 0;
float yrot = 0;
float xdiff = 0;
float ydiff = 0;
bool mouseDown = false;
const int BMP_Header_Length=54;
const int imagewidth=320;
const int imageheight=240;
static GLubyte* BmpData;
GLint PixelDataLength;

void saveFrameBuff();
void initSharedMemory();
void WriteSharedMemory();
void ReadSharedMemory();
void closeSharedMemory();

int alpha=0;
int NumRedraw=0;

//detect mouse motion 
void mouseMotionCB(int x, int y)
{
  if (mouseDown)
    {
      yrot = x - xdiff;
      xrot = y + ydiff;
      glutPostRedisplay();
    }
}

//detect mouse click 
void mouseCB(int button, int state, int x, int y)
{
  switch(button){
  case GLUT_LEFT_BUTTON:
    switch(state)
      {
      case GLUT_DOWN:
	mouseDown = true;
	xdiff = x - yrot;
	ydiff = -y + xrot;
	break;
      case GLUT_UP:
	mouseDown = false;
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

 // operateSharedMemory();
  // Clear the screne
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


  model = glm::mat4(1.0f);
  glm::mat4 xRotMat = glm::rotate(glm::mat4(1.0f), xrot, glm::normalize(glm::vec3(glm::inverse(model) * glm::vec4(1, 0, 0, 1))) );
  model = model * xRotMat;
  glm::mat4 yRotMat = glm::rotate(glm::mat4(1.0f), yrot, glm::normalize(glm::vec3(glm::inverse(model) * glm::vec4(0, 1, 0, 1))) );
  model = model * yRotMat;

  glm::mat4 modelViewMatrix = view * model;
  glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(modelViewMatrix)); // Normal Matrix

  // Use our shader
  glUseProgram(programID);
  // Send the model, view and projection matrices to the shader 
  glUniformMatrix4fv(modelID, 1, GL_FALSE, &model[0][0]);
  glUniformMatrix4fv(viewID, 1, GL_FALSE, &view[0][0]);
  glUniformMatrix4fv(projectionID, 1, GL_FALSE, &projection[0][0]);
  glUniformMatrix3fv( glGetUniformLocation(programID, "normalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);

  glmDrawVBO(objModel, programID);

  // Swap buffers
  glutSwapBuffers();
  saveFrameBuff();
}


void init(char* fname)
{

  objModel = glmReadOBJ(fname);
  if (!objModel) 
  {
    cout<<"read fname failed" <<endl;
    exit(0);
  }
  


  // Normilize vertices
  glmUnitize(objModel);
  // Compute facet normals
  glmFacetNormals(objModel);
  // Comput vertex normals
  glmVertexNormals(objModel, 90.0);
  // Load the model (vertices and normals) into a vertex buffer
  glmLoadInVBO(objModel);

  // Black background
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  // Accept fragment if it closer to the camera than the former one
  glDepthFunc(GL_LESS); 

  // TODO: Make sure that this is trully unnecessary
  //GLuint VertexArrayID;
  //glGenVertexArrays(1, &VertexArrayID);
  //glBindVertexArray(VertexArrayID);

  // Create and compile our GLSL program from the shaders
  programID = LoadShaders( "shaders/vertShader.glsl", "shaders/fragShader.glsl" );

  // Get a handle for our model, view and projection uniforms
  modelID = glGetUniformLocation(programID, "model");
  viewID = glGetUniformLocation(programID, "view");
  projectionID = glGetUniformLocation(programID, "projection");

  glm::vec4 light_ambient = glm::vec4( 0.1, 0.1, 0.1, 0.5 );
  glm::vec4 light_diffuse = glm::vec4 ( 0.8, 1.0, 1.0, 1.0 );
  glm::vec4 light_specular =glm::vec4( 0.8, 1.0, 1.0, 1.0 );

  glUseProgram(programID);
  glUniform4fv( glGetUniformLocation(programID, "light_ambient"),1,&light_ambient[0]);
  glUniform4fv( glGetUniformLocation(programID, "light_diffuse"),1, &light_diffuse[0]);
  glUniform4fv( glGetUniformLocation(programID, "light_specular"),1, &light_specular[0]);      

  // Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
  projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
  // Camera matrix
  view = glm::lookAt( glm::vec3(0,0,3), // Camera position in World Space
		      glm::vec3(0,0,0), // and looks at the origin
		      glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
		      );
  // Model matrix : an identity matrix (model will be at the origin)
  model      = glm::mat4(1.0f);

  // Initialize a light
  glm::vec4 lightPosition = glm::vec4(-20, -10, 0, 0);
  glUniform4fv( glGetUniformLocation(programID, "lightPos"),1, &lightPosition[0]); 

}


// Get OpenGL version information
void getGLinfo()
{
  std::cout << "GL Vendor   : " << glGetString(GL_VENDOR) << std::endl;
  std::cout << "GL Renderer : " << glGetString(GL_RENDERER) << std::endl;
  std::cout << "GL Version  : " << glGetString(GL_VERSION) << std::endl;
}
void reshape( int width, int height )
{
  glViewport(0, 0, width, height);
  GLfloat aspectRatio = GLfloat(width)/height;
  projection = glm::perspective(45.0f, aspectRatio, 0.1f, 100.0f);
  glutPostRedisplay();
}

void OnTimer(int value)
{
   alpha++;
   alpha=(alpha%256);

    if (NumRedraw>0)
    {
      glutPostRedisplay();
      NumRedraw--;
    }
   

   ReadSharedMemory();
   glutTimerFunc(100, OnTimer, 1);
}

int main(  int argc, char **argv  )
{

  // TODO: Command-line parsing and error checking
  char* fname;
  if (argc == 2)
    {
      fname = argv[1];
    }
  else
    {
      std::cerr << "You have to specify an OBJ file as argument." << std::endl;
      return 1;
    }
  initSharedMemory();

  glutInit( &argc, argv );
  glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
  glutInitWindowSize( 320, 240 );

  glutCreateWindow( "GPU provider1" );
  glewInit();
  getGLinfo();
  init(fname);
  glutDisplayFunc( display );
  glutReshapeFunc( reshape );
//  glutMouseFunc(mouseCB);
//  glutMotionFunc(mouseMotionCB);
  glutTimerFunc(100, OnTimer, 1);
  glutMainLoop();
  closeSharedMemory();
  free(BmpData);
  return 0;
}

void saveFrameBuff()
{

  GLint i;
  GLubyte* pPixelData;

  i = imagewidth * 3;      // get the length of each row 
    while (i % 4 != 0)  // length has to be mod of 4
        ++i;
  PixelDataLength = i * imageheight;

  //first time load tmp header from a tmp file and set up width and height
  static bool flag_1st=false;
  if (flag_1st == false)
    {
      // all data is stored in BmpData (head+pixel info)
      flag_1st=true;
      BmpData = (GLubyte*)malloc(BMP_Header_Length+PixelDataLength);
      if (BmpData == 0)
      {
        cout<<"allocate Bmpdata failed"<<endl;
        exit(0);
      }
      
      
      //load tmp header from a tmp file
      FILE* pDummyFile;
      pDummyFile = fopen("ground.bmp", "rb");
      if (pDummyFile == 0)
      {
        cout<<"open ground bmp failed"<<endl;
        exit(0);
      }
        
      fread(BmpData, BMP_Header_Length, 1, pDummyFile);
      fclose(pDummyFile);
      //set up width and height
      memcpy(&BmpData[0x0012], &imagewidth, sizeof(imagewidth));
      memcpy(&BmpData[0x0016], &imageheight, sizeof(imageheight));
    }


  //read screen pixel 
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glReadPixels(0, 0, imagewidth, imageheight, GL_BGR_EXT, GL_UNSIGNED_BYTE, &BmpData[BMP_Header_Length]);

  /*
  FILE* pWritingFile;
  pWritingFile = fopen("test.bmp", "wb");
  if (pWritingFile == 0)
        exit(0);
  fseek(pWritingFile, 0, SEEK_SET);
  fwrite(BmpData, BMP_Header_Length+PixelDataLength, 1, pWritingFile);
  fclose(pWritingFile);
  */
  WriteSharedMemory();
}

void initSharedMemory()
{
      //step 1：set memory sharing handle

    shared_file = OpenFileMapping(

    FILE_MAP_ALL_ACCESS,//access mode: read&write

    FALSE,

    Name_sharedmemory.c_str()  //name of shared memory

    );

    if (shared_file == NULL)

    {

    cout << "Could not open file mapping object..." << endl;

    return;

    }

    //step 2: set memory mapping and get pointer 

    lpBUF = MapViewOfFile(

    shared_file, // shared memory holder

    FILE_MAP_ALL_ACCESS,//access mode read and write

    0, //offset of file: high 32 bits

    0, //offset of file: low 32 bits

    0 //mapping view size

    );

    if (lpBUF == NULL)

    {

    cout << "Could not create file mapping object..." << endl;

    CloseHandle(shared_file);

    return;

    }

    

    H_Mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, Name_Mutex.c_str());

    if (H_Mutex == NULL)

    {

    cout << "open mutex failed..." <<endl;

    return;

    }

    H_BmpRdyEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, Name_BmpRdyEvent.c_str());

    if (H_BmpRdyEvent == NULL)

    {

    cout << "open mutex failed..." << endl;

    return;

    }

    H_MouseEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, Name_MouseEvent.c_str());

    if (H_MouseEvent == NULL)

    {

    cout << "open mutex failed..." << endl;

    return;

    }

}

// write newly updated rendering result (BmpData)
void WriteSharedMemory()
{

    if(WAIT_OBJECT_0 == WaitForSingleObject(H_Mutex, 100))//use mutex 
    {
      memcpy((GLubyte*)lpBUF+200, BmpData, BMP_Header_Length+PixelDataLength);

      ReleaseMutex(H_Mutex); //release mutex

      SetEvent(H_BmpRdyEvent);
    } 

}

void ReadSharedMemory()
{
    float MousePos[2];


    if(WAIT_OBJECT_0 == WaitForSingleObject(H_MouseEvent, 0))
    {    
        if(WAIT_OBJECT_0 == WaitForSingleObject(H_Mutex, 100))//use mutex
        {
            memcpy(MousePos, (float*)lpBUF, 2*sizeof(float));
            ReleaseMutex(H_Mutex); //release mutex
            xrot=MousePos[0];
            yrot=MousePos[1];
            cout << "MousePos[0]=" <<MousePos[0] << " " << "MousePos[1]=" <<MousePos[1]<<endl;
            NumRedraw=5;
            glutPostRedisplay();
        } 
    }
}

void closeSharedMemory()
{
    CloseHandle(H_Mutex);

    CloseHandle(H_BmpRdyEvent);
    CloseHandle(H_MouseEvent);

//step4：release memory mapping and close handle

    UnmapViewOfFile(lpBUF);

    CloseHandle(shared_file);
}