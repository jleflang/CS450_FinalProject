#include "includes/common.h"
#include <set>

#define GLEW_STATIC
#include "includes/glew.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include "includes/freeglut.h"

#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/matrix_access.hpp"
#include "glm/glm/gtc/matrix_transform.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

// Provided Code
#include "includes/glslprogram.h"
#include "includes/loadobjfile.h"
#include "includes/vertexbufferobject.h"


// My code
#include "includes/loadmtlfile.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_PIC
#define STBI_NO_PNM
#include "includes/stb_image.h"
#endif // !STB_IMAGE_IMPLEMENTATION


//	This code is mostly taken from the sample OpenGL / GLUT program
//
//	The objective is to texture an object
//
//	The left mouse button does rotation
//	The middle mouse button does scaling
//	The user interface allows:
//		1. The axes to be turned on and off
//		2. The color of the axes to be changed
//		3. Debugging to be turned on and off
//		4. Depth cueing to be turned on and off
//		5. The projection to be changed
//      6. The viewpoint to be switched
//		7. The transformations to be reset
//		8. The program to quit
//
//	Author:			James Leflang

// title of these windows:

const char* WINDOWTITLE = { "Using Lighting -- James Leflang" };
const char* GLUITITLE = { "User Interface Window" };

// what the glui package defines as true and false:

const int GLUITRUE = { true };
const int GLUIFALSE = { false };

// the escape key:

#define ESCAPE		0x1b

// initial window size:

const int INIT_WINDOW_SIZE = { 768 };

// size of the 3d box:

// const float BOXSIZE = { 2.f };

// multiplication factors for input interaction:
//  (these are known from previous experience)

const float ANGFACT = { 1. };
const float SCLFACT = { 0.005f };

// minimum allowable scale factor:

const float MINSCALE = { 0.05f };

// scroll wheel button values:

const int SCROLL_WHEEL_UP = { 3 };
const int SCROLL_WHEEL_DOWN = { 4 };

// equivalent mouse movement when we click a the scroll wheel:

const float SCROLL_WHEEL_CLICK_FACTOR = { 5. };

// active mouse buttons (or them together):

const int LEFT = { 4 };
const int MIDDLE = { 2 };
const int RIGHT = { 1 };

float White[3] = { 1., 1., 1. };

// which projection:

enum Projections
{
    ORTHO,
    PERSP
};

// which button:

enum ButtonVals
{
    RESET,
    QUIT
};


// window background color (rgba):

const GLfloat BACKCOLOR[] = { 0., 0., 0., 1. };

// line width for the axes:

const GLfloat AXES_WIDTH = { 3. };

// the color numbers:
// this order must match the radio button order

enum Colors
{
    RED,
    YELLOW,
    GREEN,
    CYAN,
    BLUE,
    MAGENTA,
    WHITE,
    BLACK
};

char* ColorNames[] =
{
    (char*)"Red",
    (char*)"Yellow",
    (char*)"Green",
    (char*)"Cyan",
    (char*)"Blue",
    (char*)"Magenta",
    (char*)"White",
    (char*)"Black"
};

// the color definitions:
// this order must match the menu order

const GLfloat Colors[][3] =
{
    { 1., 0., 0. },		// red
    { 1., 1., 0. },		// yellow
    { 0., 1., 0. },		// green
    { 0., 1., 1. },		// cyan
    { 0., 0., 1. },		// blue
    { 1., 0., 1. },		// magenta
    { 1., 1., 1. },		// white
    { 0., 0., 0. },		// black
};

// fog parameters:

const GLfloat FOGCOLOR[4] = { .0, .0, .0, 1. };
const GLenum  FOGMODE = { GL_LINEAR };
const GLfloat FOGDENSITY = { 0.30f };
const GLfloat FOGSTART = { 1.5 };
const GLfloat FOGEND = { 4. };


// what options should we compile-in?
// in general, you don't need to worry about these
// i compile these in to show class examples of things going wrong

//#define DEMO_Z_FIGHTING
//#define DEMO_DEPTH_BUFFER

// should we turn the shadows on?

//#define ENABLE_SHADOWS

// non-constant global variables:

int		ActiveButton;			// current button that is down
GLuint	AxesList;				// list to hold the axes
int		AxesOn;					// != 0 means to draw the axes
int		DebugOn;				// != 0 means to print debugging info
int		DepthCueOn;				// != 0 means to use intensity depth cueing
int		DepthBufferOn;			// != 0 means to use the z-buffer
int		DepthFightingOn;		// != 0 means to force the creation of z-fighting
GLuint  envMapTexture;
GLuint  envCube;
GLuint  Tex0;
GLuint	SphereList;				// object display list
GLuint  ObjFileList;
int		MainWindow;				// window id for main graphics window
float	Scale;					// scaling factor
int		ShadowsOn;				// != 0 means to turn shadows on
int		WhichColor;				// index into Colors[ ]
int		WhichProjection;		// ORTHO or PERSP
int     WhichView;
int		Xmouse, Ymouse;			// mouse values
float	Xrot, Yrot;				// rotation angles in degrees

bool	Frozen;

int    envW, envH, nrComp;

unsigned char* Texture;
int           width, height;

GLSLProgram *Environment;
GLSLProgram *Back;
GLSLProgram *Iem;
GLSLProgram *Prefilter;
GLSLProgram *Brdf;
GLSLProgram *Uber;

GLSLProgram *GetDepth;
GLSLProgram* RenderWithShadows;

GLuint framebuf;
GLuint renderbuf;
GLuint iemMap;
GLuint prefilter;
GLuint brdf;

GLuint depthMap;
GLuint shadowMap;
GLuint shadowColorMap;

struct objtex_maps
{
    std::string name;
    GLuint diffuse, rough, reflect, norm;
};

std::vector<struct objtex_maps> objtextures;

VertexBufferObject* envCubeObj;
std::vector<VertexBufferObject*> telescopeObj;
VertexBufferObject* brdfQuad;

MaterialSet* materiallib;

const unsigned int shadows[2] = 
{
    2048,
    2048
};

GLfloat CubeVertices[][3] =
{
    { -1., -1., -1. },
    {  1., -1., -1. },
    { -1.,  1., -1. },
    {  1.,  1., -1. },
    { -1., -1.,  1. },
    {  1., -1.,  1. },
    { -1.,  1.,  1. },
    {  1.,  1.,  1. }
};

GLuint CubeIndices[][4] =
{
    //{ 0, 2, 3, 1 },
    { 1, 3, 2, 0 },
    //{ 4, 5, 7, 6 },
    { 6, 7, 5, 4 },
    //{ 1, 3, 7, 5 },
    { 5, 7, 3, 1 },
    //{ 0, 4, 6, 2 },
    { 2, 6, 4, 0 },
    //{ 2, 6, 7, 3 },
    { 3, 7, 6, 2 },
    //{ 0, 1, 5, 4 }
    { 4, 5, 1, 0 }
};

GLfloat CubeTextures[][2] =
{
    { 1., 0. },
    {-1., 0. },
    { 0., 1. },
    { 0.,-1. },
    { 1., 1. },
    {-1.,-1. }
};

GLfloat CubeNormals[][3] =
{
    { 0.,  0., -1. },
    { 0.,  0.,  1. },
    {-1.,  0.,  0. },
    { 1.,  0.,  0. },
    { 0., -1.,  0. },
    { 0.,  1.,  0. },
};

// function prototypes:

void	Animate();
void	Display();
void	DoAxesMenu(int);
void	DoColorMenu(int);
void	DoDepthBufferMenu(int);
void	DoDepthFightingMenu(int);
void	DoDepthMenu(int);
void	DoDebugMenu(int);
void	DoMainMenu(int);
void	DoProjectMenu(int);
//void	DoShadowMenu();
void	DoRasterString(float, float, float, char*);
void	DoStrokeString(float, float, float, float, char*);
float	ElapsedSeconds();
void	InitGraphics();
void	InitLists();
void	InitMenus();
void	Keyboard(unsigned char, int, int);
void	MouseButton(int, int, int, int);
void	MouseMotion(int, int);
void	Reset();
void	Resize(int, int);
void	Visibility(int);

void            OsuSphere(float, int, int);
void			Axes(float);
unsigned char* BmpToTexture(char*, int*, int*);
void			HsvRgb(float[3], float[3]);
int				ReadInt(FILE*);
short			ReadShort(FILE*);

void renderQuad();
void renderSphere();

// main program:

int
main(int argc, char* argv[])
{
    // turn on the glut package:
    // (do this before checking argc and argv since it might
    // pull some command line arguments out)

    glutInit(&argc, argv);

    // setup all the graphics stuff:

    InitGraphics();

    // create the display structures that will not change:

    InitLists();

    // init all the global variables used by Display( ):
    // this will also post a redisplay

    Reset();

    // setup all the user interface stuff:

    InitMenus();

    // draw the scene once and wait for some interaction:
    // (this will never return)

    glutSetWindow(MainWindow);
    glutMainLoop();

    // glutMainLoop( ) never returns
    // this line is here to make the compiler happy:

    return 0;
}


void GLAPIENTRY
DebugOutput(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
        type, severity, message);
}


// this is where one would put code that is to be called
// everytime the glut main loop has nothing to do
//
// this is typically where animation parameters are set
//
// do not call Display( ) from here -- let glutMainLoop( ) do it

float Time;
#define MS_IN_THE_ANIMATION_CYCLE	10000

void
Animate()
{
    // put animation stuff in here -- change some global variables
    // for Display( ) to find:

    int ms = glutGet(GLUT_ELAPSED_TIME);	// milliseconds
    ms %= MS_IN_THE_ANIMATION_CYCLE;
    Time = (float)ms / (float)MS_IN_THE_ANIMATION_CYCLE;        // [ 0., 1. )

    // force a call to Display( ) next time it is convenient:

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}


// draw the complete scene:

void
Display()
{
    if (DebugOn != 0)
    {
        fprintf(stderr, "Display\n");
    }

    // set which window we want to do the graphics into:

    glutSetWindow(MainWindow);

    float theta = (2.f * (float)M_PI) * Time;
    glm::mat4 model(1.f);

    glm::vec3 light_translate[] = {
        glm::vec3(5.f * (float)cos(2. * M_PI), 2.f, 5.f * (float)sin(2 * M_PI)),
        glm::vec3(4.9f * (float)sin(theta), 8.f, -3.f),
        glm::vec3(-7.f * (float)cos(theta), -7.f * (float)sin(theta), -6.f),
        glm::vec3(5.f * (float)sin(2. * M_PI), 2.f, 5.f * (float)cos(2 * M_PI)),
    };

    glm::vec3 light_color[] = {
        glm::vec3(600.,600.,600.),
        glm::vec3(600.,600.,600.),
        glm::vec3(600.,600.,600.),
        glm::vec3(600.,600.,600.),
    };

    glm::mat4 L0_td = glm::translate(model, light_translate[0]);
    L0_td = glm::scale(L0_td, glm::vec3(0.5f));
    glm::mat4 L1_td = glm::translate(model, light_translate[1]);
    L1_td = glm::scale(L1_td, glm::vec3(0.5f));
    glm::mat4 L2_td = glm::translate(model, light_translate[2]);
    L2_td = glm::scale(L2_td, glm::vec3(0.5f));
    glm::mat4 L3_td = glm::translate(model, light_translate[3]);
    L3_td = glm::scale(L3_td, glm::vec3(0.5f));

    GetDepth->Use();

    glm::mat4 lightProjection = glm::ortho(-500.0f, 500.0f, -500.0f, 500.0f, 0.f, 500.f);
    glm::mat4 lightView = glm::lookAt(light_translate[2], glm::vec3(0., 0., 0.), glm::vec3(0., 1., 0.));

    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    glCullFace(GL_FRONT);
    glViewport(0, 0, shadows[0], shadows[1]);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMap);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_FLAT);
    glDisable(GL_NORMALIZE);

    GetDepth->SetUniformVariable((char*)"uLightSpaceMatrix", lightSpaceMatrix);

    glm::mat4 objfile(1.f);

    //objfile = glm::rotate(objfile, glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
    //objfile = glm::rotate(objfile, glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f));
    //objfile = glm::translate(objfile, glm::vec3(0.f, 0.f, -9.f));
    //objfile = glm::scale(objfile, glm::vec3(0.1f, 0.1f, 0.1f));
   
    GetDepth->SetUniformVariable((char*)"uModel", objfile);
    for (auto obj : telescopeObj)
        obj->Draw();

    GetDepth->SetUniformVariable((char*)"uModel", L0_td);
    renderSphere();

    GetDepth->SetUniformVariable((char*)"uModel", L1_td);
    renderSphere();

    GetDepth->SetUniformVariable((char*)"uModel", L2_td);
    renderSphere();

    GetDepth->SetUniformVariable((char*)"uModel", L3_td);
    renderSphere();

    GetDepth->Use(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // erase the background:

    glDrawBuffer(GL_BACK);
    glClearColor(0.f, 0.f, 0.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);

    // set the viewport to a square centered in the window:

    GLsizei vx = glutGet(GLUT_WINDOW_WIDTH);
    GLsizei vy = glutGet(GLUT_WINDOW_HEIGHT);
    GLsizei v = vx < vy ? vx : vy;			// minimum dimension
    GLint xl = (vx - v) / 2;
    GLint yb = (vy - v) / 2;
    glViewport(xl, yb, v, v);

    glCullFace(GL_BACK);

    // set the viewing volume:
    // remember that the Z clipping  values are actually
    // given as DISTANCES IN FRONT OF THE EYE
    // USE gluOrtho2D( ) IF YOU ARE DOING 2D !

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glm::mat4 projection;

    if (WhichProjection == ORTHO)
        projection = glm::ortho(-3., 3., -3., 3., 0.1, 1000.);
    else
        projection = glm::perspective(glm::radians(90.), 1., 0.1, 1000.);

    RenderWithShadows->SetUniformVariable((char*)"uProj", projection);

    Uber->Use();
    Uber->SetUniformVariable((char*)"uProj", projection);
    Uber->SetUniformVariable((char*)"uLightSpaceMatrix", lightSpaceMatrix);

    //Back->Use();
    

    // place the objects into the scene:

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // set the eye position, look-at position, and up-vector:

    glm::mat4 modelview;
    glm::vec3 eye(0., 0, 11.);
    glm::vec3 look(0., 0., 0.);
    glm::vec3 up(0., 1., 0.);

    modelview = glm::lookAt(eye, look, up);

    // rotate the scene:

    modelview = glm::rotate(modelview, glm::radians(Yrot), glm::vec3(0., 1., 0.));
    modelview = glm::rotate(modelview, glm::radians(Xrot), glm::vec3(1., 0., 0.));

    // uniformly scale the scene:

    if (Scale < MINSCALE)
        Scale = MINSCALE;

    modelview = glm::scale(modelview, glm::vec3(Scale, Scale, Scale));

    glm::vec3 current_cam = glm::row(modelview, 0);

    // set the fog parameters:

    if (DepthCueOn != 0)
    {
        glFogi(GL_FOG_MODE, FOGMODE);
        glFogfv(GL_FOG_COLOR, FOGCOLOR);
        glFogf(GL_FOG_DENSITY, FOGDENSITY);
        glFogf(GL_FOG_START, FOGSTART);
        glFogf(GL_FOG_END, FOGEND);
        glEnable(GL_FOG);
    }
    else
    {
        glDisable(GL_FOG);
    }

    // possibly draw the axes:

    /*if (AxesOn != 0)
    {
        glm::mat4 axis(1.f);
        Uber->SetUniformVariable((char*)"uModel", axis);
        Uber->SetUniformVariable((char*)"albedo", 0.9f, 0.9f, 0.9f);
        Uber->SetUniformVariable((char*)"metallic", 0.05f);
        Uber->SetUniformVariable((char*)"roughness", 0.f);
        glCallList(AxesList);
    }*/

    Uber->SetUniformVariable((char*)"uView", modelview);
    Uber->SetUniformVariable((char*)"uCamPos", current_cam);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, iemMap);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, brdf);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, shadowColorMap);

    // draw the current object:

    //glm::mat4 objfile(1.f);

    //objfile = glm::rotate(objfile, glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
    //objfile = glm::rotate(objfile, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    //objfile = glm::translate(objfile, glm::vec3(0.f, 0.f, -9.f));
    //objfile = glm::scale(objfile, glm::vec3(0.1f, 0.1f, 0.1f));

    //RenderWithShadows->SetUniformVariable((char*)"uColor", objcolor);
    //RenderWithShadows->SetUniformVariable((char*)"uModel", objfile);
    
    for (auto obj : telescopeObj)
    {
        std::string cur_mat = obj->GetMaterial();
        GLuint dif = NULL, refl = NULL, rough = NULL, normal = NULL;

        for (objtex_maps &textures : objtextures)
        {
            if (textures.name == cur_mat)
            {
                dif = textures.diffuse;
                rough = textures.rough;
                refl = textures.reflect;
                normal = textures.norm;
            }

            continue;
        }

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, dif);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, rough);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, refl);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, normal);

        Uber->SetUniformVariable((char*)"uModel", objfile);
        obj->Draw();

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
        

    Uber->SetUniformVariable((char*)"lightPositions[0]", light_translate[0]);
    Uber->SetUniformVariable((char*)"lightColors[0]", light_color[0]);
    Uber->SetUniformVariable((char*)"uModel", L0_td);
    //glCallList(SphereList);
    renderSphere();

    Uber->SetUniformVariable((char*)"lightPositions[1]", light_translate[1]);
    Uber->SetUniformVariable((char*)"lightColors[1]", light_color[1]);
    Uber->SetUniformVariable((char*)"uModel", L1_td);
    //glCallList(SphereList);
    renderSphere();

    Uber->SetUniformVariable((char*)"lightPositions[2]", light_translate[2]);
    Uber->SetUniformVariable((char*)"lightColors[2]", light_color[2]);
    Uber->SetUniformVariable((char*)"uModel", L2_td);
    //glCallList(SphereList);
    renderSphere();

    Uber->SetUniformVariable((char*)"lightPositions[3]", light_translate[3]);
    Uber->SetUniformVariable((char*)"lightColors[3]", light_color[3]);
    Uber->SetUniformVariable((char*)"uModel", L3_td);
    //glCallList(SphereList);
    renderSphere();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, 0);

    Uber->Use(0);
    
    Back->Use();
    Back->SetUniformVariable((char*)"uProj", projection);
    Back->SetUniformVariable((char*)"uView", modelview);
    Back->SetUniformVariable((char*)"uenvMap", 0);
    Back->SetUniformVariable((char*)"uExpose", 2.6f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCube);
    envCubeObj->Draw();
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    Back->Use(0);

    /*Brdf->Use();
    renderQuad();
    Brdf->UnUse();*/


    // #ifdef DEMO_Z_FIGHTING
    //     if( DepthFightingOn != 0 )
    //     {
    //         glPushMatrix( );
    //             glRotatef( 90.,   0., 1., 0. );
    //             glCallList( ConeList );
    //         glPopMatrix( );
    //     }
    // #endif


        // draw some gratuitous text that just rotates on top of the scene:

        // glDisable( GL_DEPTH_TEST );
        // glColor3f( 0., 1., 1. );
        // DoRasterString( 0., 1., 0., (char *)"Text That Moves" );


        // draw some gratuitous text that is fixed on the screen:
        //
        // the projection matrix is reset to define a scene whose
        // world coordinate system goes from 0-100 in each axis
        //
        // this is called "percent units", and is just a convenience
        //
        // the modelview matrix is reset to identity as we don't
        // want to transform these coordinates

        // glDisable( GL_DEPTH_TEST );
        // glMatrixMode( GL_PROJECTION );
        // glLoadIdentity( );
        // gluOrtho2D( 0., 100.,     0., 100. );
        // glMatrixMode( GL_MODELVIEW );
        // glLoadIdentity( );
        // glColor3f( 1., 1., 1. );
        // DoRasterString( 5., 5., 0., (char *)"Text That Doesn't" );


        // swap the double-buffered framebuffers:

    glutSwapBuffers();


    // be sure the graphics buffer has been sent:
    // note: be sure to use glFlush( ) here, not glFinish( ) !

    glFlush();
}


void
DoAxesMenu(int id)
{
    AxesOn = id;

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}


void
DoColorMenu(int id)
{
    WhichColor = id - RED;

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}


void
DoDebugMenu(int id)
{
    DebugOn = id;

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}


void
DoDepthBufferMenu(int id)
{
    DepthBufferOn = id;

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}


void
DoDepthFightingMenu(int id)
{
    DepthFightingOn = id;

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}


void
DoDepthMenu(int id)
{
    DepthCueOn = id;

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}

// main menu callback:

void
DoMainMenu(int id)
{
    switch (id)
    {
    case RESET:
        Reset();
        break;

    case QUIT:
        // gracefully close out the graphics:
        // gracefully close the graphics window:
        // gracefully exit the program:
        glutSetWindow(MainWindow);
        glFinish();
        glutDestroyWindow(MainWindow);
        exit(0);
        break;

    default:
        fprintf(stderr, "Don't know what to do with Main Menu ID %d\n", id);
    }

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}


void
DoProjectMenu(int id)
{
    WhichProjection = id;

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}


void
DoShadowsMenu(int id)
{
    ShadowsOn = id;

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}

void
DoViewMenu(int id)
{
    WhichView = id;

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}


// use glut to display a string of characters using a raster font:

void
DoRasterString(float x, float y, float z, char* s)
{
    glRasterPos3f((GLfloat)x, (GLfloat)y, (GLfloat)z);

    char c;			// one character to print
    for (; (c = *s) != '\0'; s++)
    {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
    }
}


// use glut to display a string of characters using a stroke font:

void
DoStrokeString(float x, float y, float z, float ht, char* s)
{
    glPushMatrix();
    glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z);
    float sf = ht / (119.05f + 33.33f);
    glScalef((GLfloat)sf, (GLfloat)sf, (GLfloat)sf);
    char c;			// one character to print
    for (; (c = *s) != '\0'; s++)
    {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
    }
    glPopMatrix();
}


// return the number of seconds since the start of the program:

float
ElapsedSeconds()
{
    // get # of milliseconds since the start of the program:

    int ms = glutGet(GLUT_ELAPSED_TIME);

    // convert it to seconds:

    return (float)ms / 1000.f;
}


// initialize the glui window:

void
InitMenus()
{
    glutSetWindow(MainWindow);

    int numColors = sizeof(Colors) / (3 * sizeof(int));
    int colormenu = glutCreateMenu(DoColorMenu);
    for (int i = 0; i < numColors; i++)
    {
        glutAddMenuEntry(ColorNames[i], i);
    }

    int axesmenu = glutCreateMenu(DoAxesMenu);
    glutAddMenuEntry("Off", 0);
    glutAddMenuEntry("On", 1);

    int depthcuemenu = glutCreateMenu(DoDepthMenu);
    glutAddMenuEntry("Off", 0);
    glutAddMenuEntry("On", 1);

    int depthbuffermenu = glutCreateMenu(DoDepthBufferMenu);
    glutAddMenuEntry("Off", 0);
    glutAddMenuEntry("On", 1);

    int depthfightingmenu = glutCreateMenu(DoDepthFightingMenu);
    glutAddMenuEntry("Off", 0);
    glutAddMenuEntry("On", 1);

    int debugmenu = glutCreateMenu(DoDebugMenu);
    glutAddMenuEntry("Off", 0);
    glutAddMenuEntry("On", 1);

    int projmenu = glutCreateMenu(DoProjectMenu);
    glutAddMenuEntry("Orthographic", ORTHO);
    glutAddMenuEntry("Perspective", PERSP);

    int shadowsmenu = glutCreateMenu(DoShadowsMenu);
    glutAddMenuEntry("Off", 0);
    glutAddMenuEntry("On", 1);

    int mainmenu = glutCreateMenu(DoMainMenu);
    glutAddSubMenu("Axes", axesmenu);
    //glutAddSubMenu("Colors", colormenu);

#ifdef DEMO_DEPTH_BUFFER
    glutAddSubMenu("Depth Buffer", depthbuffermenu);
#endif

#ifdef DEMO_Z_FIGHTING
    glutAddSubMenu("Depth Fighting", depthfightingmenu);
#endif

    glutAddSubMenu("Depth Cue", depthcuemenu);
    glutAddSubMenu("Projection", projmenu);


#ifdef ENABLE_SHADOWS
    glutAddSubMenu("Shadows", shadowsmenu);
#endif

    glutAddMenuEntry("Reset", RESET);
#ifdef _DEBUG
    glutAddSubMenu("Debug", debugmenu);
#endif // DEBUG

    glutAddMenuEntry("Quit", QUIT);

    // attach the pop-up menu to the right mouse button:

    glutAttachMenu(GLUT_RIGHT_BUTTON);
}



// initialize the glut and OpenGL libraries:
//	also setup display lists and callback functions

void
InitGraphics()
{
    glutSetOption(GLUT_MULTISAMPLE, 8);
    glutInitContextVersion(4, 5);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
#ifdef _DEBUG
    glutInitContextFlags(GLUT_DEBUG);
#endif

    // request the display modes:
    // ask for red-green-blue-alpha color, double-buffering, and z-buffering:

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);

    // set the initial window configuration:

    glutInitWindowPosition(0, 0);
    glutInitWindowSize(INIT_WINDOW_SIZE, INIT_WINDOW_SIZE);

    // open the window and set its title:

    MainWindow = glutCreateWindow(WINDOWTITLE);
    glutSetWindowTitle(WINDOWTITLE);

    // set the framebuffer clear values:

    glClearColor(BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3]);

    // setup the callback functions:
    // DisplayFunc -- redraw the window
    // ReshapeFunc -- handle the user resizing the window
    // KeyboardFunc -- handle a keyboard input
    // MouseFunc -- handle the mouse button going down or up
    // MotionFunc -- handle the mouse moving with a button down
    // PassiveMotionFunc -- handle the mouse moving with a button up
    // VisibilityFunc -- handle a change in window visibility
    // EntryFunc	-- handle the cursor entering or leaving the window
    // SpecialFunc -- handle special keys on the keyboard
    // SpaceballMotionFunc -- handle spaceball translation
    // SpaceballRotateFunc -- handle spaceball rotation
    // SpaceballButtonFunc -- handle spaceball button hits
    // ButtonBoxFunc -- handle button box hits
    // DialsFunc -- handle dial rotations
    // TabletMotionFunc -- handle digitizing tablet motion
    // TabletButtonFunc -- handle digitizing tablet button hits
    // MenuStateFunc -- declare when a pop-up menu is in use
    // TimerFunc -- trigger something to happen a certain time from now
    // IdleFunc -- what to do when nothing else is going on

    glutSetWindow(MainWindow);
    glutDisplayFunc(Display);
    glutReshapeFunc(Resize);
    glutKeyboardFunc(Keyboard);
    glutMouseFunc(MouseButton);
    glutMotionFunc(MouseMotion);
    glutPassiveMotionFunc(MouseMotion);
    //glutPassiveMotionFunc( NULL );
    glutVisibilityFunc(Visibility);
    glutEntryFunc(NULL);
    glutSpecialFunc(NULL);
    glutSpaceballMotionFunc(NULL);
    glutSpaceballRotateFunc(NULL);
    glutSpaceballButtonFunc(NULL);
    glutButtonBoxFunc(NULL);
    glutDialsFunc(NULL);
    glutTabletMotionFunc(NULL);
    glutTabletButtonFunc(NULL);
    glutMenuStateFunc(NULL);
    glutTimerFunc(-1, NULL, 0);
    glutIdleFunc(Animate);

    // init glew (a window must be open to do this):

    GLenum err = glewInit();
#ifdef _DEBUG
    if (err != GLEW_OK)
    {
        fprintf(stderr, "glewInit Error\n");
    }
    else
        fprintf(stderr, "GLEW initialized OK\n");
    fprintf(stderr, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    //glEnable(GL_ARB_debug_output);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallback(DebugOutput, 0);
    glDebugMessageControl(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, GL_DEBUG_SEVERITY_HIGH, 0, NULL, GL_TRUE);
#endif

    glEnable(GL_MULTISAMPLE);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glEnable(GL_NORMALIZE);

    envCubeObj = new VertexBufferObject();
    envCubeObj->CollapseCommonVertices(false);
    envCubeObj->glBegin(GL_QUADS);
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            GLuint k = CubeIndices[i][j];
            envCubeObj->glVertex3fv(CubeVertices[k]);
            envCubeObj->glNormal3fv(CubeNormals[i]);
            envCubeObj->glTexCoord2fv(CubeTextures[k]);
        }
    }
    envCubeObj->glEnd();

#ifndef _DEBUG
    envCubeObj->SetVerbose(false);
#else
    envCubeObj->SetVerbose(true);
#endif // !_DEBUG

    //telescopeObj = new VertexBufferObject();
    //telescopeObj->CollapseCommonVertices(false);
    //telescopeObj->glBegin(GL_TRIANGLES);
    materiallib = new MaterialSet();
    LoadObjFile((char*)"assets\\skyscanner_100.obj", &telescopeObj, materiallib);
    //telescopeObj->glEnd();

    //glShadeModel(GL_FLAT);
    //glDisable(GL_NORMALIZE);

    brdfQuad = new VertexBufferObject();
    brdfQuad->CollapseCommonVertices(false);
    brdfQuad->glBegin(GL_TRIANGLE_STRIP);

    brdfQuad->glVertex3f(-1., 1., 0.);
    brdfQuad->glTexCoord2f(0., 1.);
    brdfQuad->glVertex3f(-1., -1., 0.);
    brdfQuad->glTexCoord2f(0., 0.);
    brdfQuad->glVertex3f(-1., 1., 0.);
    brdfQuad->glTexCoord2f(1., 1.);
    brdfQuad->glVertex3f(1., -1., 0.);
    brdfQuad->glTexCoord2f(1., 0.);

    brdfQuad->glEnd();

#ifndef _DEBUG
    brdfQuad->SetVerbose(false);
#else
    brdfQuad->SetVerbose(true);
#endif // !_DEBUG

    Back = new GLSLProgram();

    bool valid = Back->Create((char*)"shaders\\back.vert", (char*)"shaders\\back.frag");
#ifdef _DEBUG
    if (!valid)
    {
        fprintf(stderr, "Background Shader cannot be created!\n");
        DoMainMenu(QUIT);
    }
    else
    {
        fprintf(stderr, "Background Shader created.\n");
    }
#endif // _DEBUG
    Back->SetVerbose(false);

    Environment = new GLSLProgram();

    valid = Environment->Create((char*)"shaders\\env.vert", (char*)"shaders\\env.frag");
#ifdef _DEBUG
    if (!valid)
    {
        fprintf(stderr, "Environment Shader cannot be created!\n");
        DoMainMenu(QUIT);
    }
    else
    {
        fprintf(stderr, "Environment Shader created.\n");
    }
#endif // _DEBUG
    Environment->SetVerbose(false);

    Iem = new GLSLProgram();

    valid = Iem->Create((char*)"shaders\\env.vert", (char*)"shaders\\iem.frag");
#ifdef _DEBUG
    if (!valid)
    {
        fprintf(stderr, "IEM Shader cannot be created!\n");
        DoMainMenu(QUIT);
    }
    else
    {
        fprintf(stderr, "IEM Shader created.\n");
    }
#endif // _DEBUG
    Iem->SetVerbose(false);

    Prefilter = new GLSLProgram();

    valid = Prefilter->Create((char*)"shaders\\env.vert", (char*)"shaders\\prefilter.frag");
#ifdef _DEBUG
    if (!valid)
    {
        fprintf(stderr, "Prefilter Shader cannot be created!\n");
        DoMainMenu(QUIT);
    }
    else
    {
        fprintf(stderr, "Prefilter Shader created.\n");
    }
#endif // _DEBUG
    Prefilter->SetVerbose(false);

    Brdf = new GLSLProgram();

    valid = Brdf->Create((char*)"shaders\\brdfLUT.vert", (char*)"shaders\\brdfLUT.frag");
#ifdef _DEBUG
    if (!valid)
    {
        fprintf(stderr, "BRDF Shader cannot be created!\n");
        DoMainMenu(QUIT);
    }
    else
    {
        fprintf(stderr, "BRDF Shader created.\n");
    }
#endif // _DEBUG
    Brdf->SetVerbose(false);

    Uber = new GLSLProgram();

    valid = Uber->Create((char*)"shaders\\objshader.vert", (char*)"shaders\\objshader.frag");
#ifdef _DEBUG
    if (!valid)
    {
        fprintf(stderr, "Uber Shader cannot be created!\n");
        DoMainMenu(QUIT);
    }
    else
    {
        fprintf(stderr, "Uber Shader created.\n");
    }
#endif // _DEBUG
    Uber->SetVerbose(false);

    GetDepth = new GLSLProgram();
    valid = GetDepth->Create((char*)"shaders\\GetDepth.vert", (char*)"shaders\\GetDepth.frag");
#ifdef _DEBUG
    if (!valid)
    {
        fprintf(stderr, "GetDepth Shader cannot be created!\n");		//DoMainMenu(QUIT);
    }
    else
    {
        fprintf(stderr, "GetDepth Shader created successfully.\n");
    }
#endif // _DEBUG
    GetDepth->SetVerbose(false);

    RenderWithShadows = new GLSLProgram();
    valid = RenderWithShadows->Create((char*)"shaders\\RenderWithShadows.vert", (char*)"shaders\\RenderWithShadows.frag");
#ifdef _DEBUG
    if (!valid)
    {
        fprintf(stderr, "RenderWithShadows Shader cannot be created!\n");
    }
    else
    {
        fprintf(stderr, "RenderWithShadows Shader created successfully.\n");
    }
#endif // _DEBUG
    RenderWithShadows->SetVerbose(false);

    glGenFramebuffers(1, &depthMap);
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadows[0], shadows[1], 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &shadowColorMap);
    glBindTexture(GL_TEXTURE_2D, shadowColorMap);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, shadows[0], shadows[1], 0, GL_RGBA, GL_FLOAT, NULL);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMap);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowColorMap, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Uber->Use();
    Uber->SetUniformVariable((char*)"ao", 1.f);
    Uber->SetUniformVariable((char*)"uExpose", 2.3f);
    Uber->Use(0);

    glGenFramebuffers(1, &framebuf);
    glGenRenderbuffers(1, &renderbuf);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuf);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuf);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 2048, 2048);
    glFramebufferRenderbuffer(GL_RENDERBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuf);
    
    // Init the Env HDR
    // Set STBI to flip images for texture loading
    stbi_set_flip_vertically_on_load(1);
    float* envImage = stbi_loadf((char*)"assets\\LA_Downtown_Helipad_GoldenHour_3k.hdr", &envW, &envH, &nrComp, 0);
    if (envImage)
    {
        glGenTextures(1, &envMapTexture);
        glBindTexture(GL_TEXTURE_2D, envMapTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, envW, envH, 0, GL_RGB, GL_FLOAT, envImage);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        stbi_image_free(envImage);
    }

    glGenTextures(1, &envCube);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCube);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 2048, 2048, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    Environment->Use();
    Environment->SetUniformVariable((char*)"uenvMap", 0);
    Environment->SetUniformVariable((char*)"uProj", captureProjection);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, envMapTexture);
    glViewport(0, 0, 2048, 2048);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuf);
    for (int i = 0; i < 6; ++i)
    {
        Environment->SetUniformVariable((char*)"uView", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCube, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        envCubeObj->Draw();
    }

    Environment->UnUse();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, envCube);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glGenTextures(1, &iemMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, iemMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    glBindFramebuffer(GL_FRAMEBUFFER, framebuf);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 128, 128);

    // pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
    // -----------------------------------------------------------------------------
    Iem->Use();
    Iem->SetUniformVariable((char*)"uenvMap", 0);
    Iem->SetUniformVariable((char*)"uProj", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCube);

    glViewport(0, 0, 128, 128); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, framebuf);
    for (unsigned int i = 0; i < 6; ++i)
    {
        Iem->SetUniformVariable((char*)"uView", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, iemMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        envCubeObj->Draw();
    }

    Iem->UnUse();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenTextures(1, &prefilter);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
    // ----------------------------------------------------------------------------------------------------
    Prefilter->Use();
    Prefilter->SetUniformVariable((char*)"uenvMap", 0);
    Prefilter->SetUniformVariable((char*)"uProj", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCube);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuf);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        // reisze framebuffer according to mip-level size.
        unsigned int mipWidth = 512 * (unsigned int)std::pow(0.5f, mip);
        unsigned int mipHeight = 512 * (unsigned int)std::pow(0.5f, mip);
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuf);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        Prefilter->SetUniformVariable((char*)"roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            Prefilter->SetUniformVariable((char*)"uView", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilter, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            envCubeObj->Draw();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Prefilter->UnUse();

    // pbr: generate a 2D LUT from the BRDF equations used.
    // ----------------------------------------------------
    glGenTextures(1, &brdf);

    // pre-allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, brdf);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 1024, 1024, 0, GL_RG, GL_FLOAT, 0);
    // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
    glBindFramebuffer(GL_FRAMEBUFFER, framebuf);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1024, 1024);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdf, 0);

    glViewport(0, 0, 1024, 1024);
    Brdf->Use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad();
    //brdfQuad->Draw();
    Brdf->Use(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    for (struct mat matter : materiallib->obj_mats)
    {
        struct objtex_maps cur_maps = { };
        struct Texture curtex;

        cur_maps.name = matter.n;

        curtex = *matter.m->LoadKd();

        glGenTextures(1, &cur_maps.diffuse);
        glBindTexture(GL_TEXTURE_2D, cur_maps.diffuse);

        glTextureParameteri(cur_maps.diffuse, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(cur_maps.diffuse, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(cur_maps.diffuse, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(cur_maps.diffuse, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, curtex.textW, curtex.textH, 0, GL_RGB, GL_UNSIGNED_BYTE, curtex.img);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        curtex = *matter.m->LoadNs();

        glGenTextures(1, &cur_maps.rough);
        glBindTexture(GL_TEXTURE_2D, cur_maps.rough);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTextureParameteri(cur_maps.rough, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(cur_maps.rough, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(cur_maps.rough, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(cur_maps.rough, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, curtex.textW, curtex.textH, 0, GL_RED, GL_UNSIGNED_BYTE, curtex.img);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        curtex = *matter.m->LoadRefl();

        glGenTextures(1, &cur_maps.reflect);
        glBindTexture(GL_TEXTURE_2D, cur_maps.reflect);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTextureParameteri(cur_maps.reflect, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(cur_maps.reflect, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(cur_maps.reflect, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(cur_maps.reflect, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, curtex.textW, curtex.textH, 0, GL_RED, GL_UNSIGNED_BYTE, curtex.img);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        curtex = *matter.m->LoadNorm();

        glGenTextures(1, &cur_maps.norm);
        glBindTexture(GL_TEXTURE_2D, cur_maps.norm);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
        glTextureParameteri(cur_maps.norm, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(cur_maps.norm, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(cur_maps.norm, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(cur_maps.norm, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, curtex.textW, curtex.textH, 0, GL_RGB, GL_UNSIGNED_SHORT, curtex.img16);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        objtextures.push_back(cur_maps);

    }
}


// initialize the display lists that will not change:
// (a display list is a way to store opengl commands in
//  memory so that they can be played back efficiently at a later time
//  with a call to glCallList( )

int		NumLngs, NumLats;
struct point* Pts;

struct point
{
    float x, y, z;		// coordinates
    float nx, ny, nz;	// surface normal
    float s, t;		// texture coords
};

inline
struct point*
    PtsPointer(int lat, int lng)
{
    if (lat < 0)	lat += (NumLats - 1);
    if (lng < 0)	lng += (NumLngs - 0);
    if (lat > NumLats - 1)	lat -= (NumLats - 1);
    if (lng > NumLngs - 1)	lng -= (NumLngs - 0);
    return &Pts[NumLngs * lat + lng];
}

inline
void
DrawPoint(struct point* p)
{
    glNormal3fv(&p->nx);
    glTexCoord2fv(&p->s);
    glVertex3fv(&p->x);
}

void
OsuSphere(float radius, int slices, int stacks)
{
    // set the globals:

    NumLngs = slices;
    NumLats = stacks;
    if (NumLngs < 3)
        NumLngs = 3;
    if (NumLats < 3)
        NumLats = 3;

    // allocate the point data structure:

    Pts = new struct point[NumLngs * NumLats];

    // fill the Pts structure:

    for (int ilat = 0; ilat < NumLats; ilat++)
    {
        float lat = -(float)M_PI / 2.f + (float)M_PI * (float)ilat / (float)(NumLats - 1);	// ilat=0/lat=0. is the south pole
                                            // ilat=NumLats-1, lat=+M_PI/2. is the north pole
        float xz = cosf(lat);
        float  y = sinf(lat);
        for (int ilng = 0; ilng < NumLngs; ilng++)				// ilng=0, lng=-M_PI and
                                            // ilng=NumLngs-1, lng=+M_PI are the same meridian
        {
            float lng = -(float)M_PI + 2.f * (float)M_PI * (float)ilng / (float)(NumLngs - 1);
            float x = xz * cosf(lng);
            float z = -xz * sinf(lng);
            struct point* p = PtsPointer(ilat, ilng);
            p->x = radius * x;
            p->y = radius * y;
            p->z = radius * z;
            p->nx = x;
            p->ny = y;
            p->nz = z;
            p->s = (lng + (float)M_PI) / (2.f * (float)M_PI);
            p->t = (lat + (float)M_PI / 2.f) / (float)M_PI;
        }
    }

    struct point top, bot;		// top, bottom points

    top.x = 0.;		top.y = radius;	top.z = 0.;
    top.nx = 0.;		top.ny = 1.;		top.nz = 0.;
    top.s = 0.;		top.t = 1.;

    bot.x = 0.;		bot.y = -radius;	bot.z = 0.;
    bot.nx = 0.;		bot.ny = -1.;		bot.nz = 0.;
    bot.s = 0.;		bot.t = 0.;

    // connect the north pole to the latitude NumLats-2:

    glBegin(GL_TRIANGLE_STRIP);
    for (int ilng = 0; ilng < NumLngs; ilng++)
    {
        float lng = -(float)M_PI + 2.f * (float)M_PI * (float)ilng / (float)(NumLngs - 1);
        top.s = (lng + (float)M_PI) / (2.f * (float)M_PI);
        DrawPoint(&top);
        struct point* p = PtsPointer(NumLats - 2, ilng);	// ilat=NumLats-1 is the north pole
        DrawPoint(p);
    }
    glEnd();

    // connect the south pole to the latitude 1:

    glBegin(GL_TRIANGLE_STRIP);
    for (int ilng = NumLngs - 1; ilng >= 0; ilng--)
    {
        float lng = -(float)M_PI + 2.f * (float)M_PI * (float)ilng / (float)(NumLngs - 1);
        bot.s = (lng + (float)M_PI) / (2.f * (float)M_PI);
        DrawPoint(&bot);
        struct point* p = PtsPointer(1, ilng);					// ilat=0 is the south pole
        DrawPoint(p);
    }
    glEnd();

    // connect the horizontal strips:

    for (int ilat = 2; ilat < NumLats - 1; ilat++)
    {
        struct point* p;
        glBegin(GL_TRIANGLE_STRIP);
        for (int ilng = 0; ilng < NumLngs; ilng++)
        {
            p = PtsPointer(ilat, ilng);
            DrawPoint(p);
            p = PtsPointer(ilat - 1, ilng);
            DrawPoint(p);
        }
        glEnd();
    }

    // clean-up:

    delete[] Pts;
    Pts = NULL;
}

void
InitLists()
{

    glutSetWindow(MainWindow);

    SphereList = glGenLists(1);
    glNewList(SphereList, GL_COMPILE);
    OsuSphere(.5, 360, 48);
    glEndList();

    /*ObjFileList = glGenLists(1);
    glNewList(ObjFileList, GL_COMPILE);
    LoadObjFile((char*)"assets\\minicooper.obj");
    glEndList();*/

    // create the axes:

    AxesList = glGenLists(1);
    glNewList(AxesList, GL_COMPILE);
    glLineWidth(AXES_WIDTH);
    Axes(1.5);
    glLineWidth(1.);
    glEndList();
}


// the keyboard callback:

void
Keyboard(unsigned char c, int x, int y)
{
    if (DebugOn != 0)
        fprintf(stderr, "Keyboard: '%c' (0x%0x)\n", c, c);

    switch (c)
    {
    case 'o':
    case 'O':
        WhichProjection = ORTHO;
        break;

    case 'p':
    case 'P':
        WhichProjection = PERSP;
        break;

    case 'f':
    case 'F':
        Frozen = !Frozen;
        if (Frozen)
            glutIdleFunc(NULL);
        else
            glutIdleFunc(Animate);
        break;

    case 'q':
    case 'Q':
    case ESCAPE:
        DoMainMenu(QUIT);	// will not return here
        break;				// happy compiler

    default:
        fprintf(stderr, "Don't know what to do with keyboard hit: '%c' (0x%0x)\n", c, c);
    }

    // force a call to Display( ):

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}


// called when the mouse button transitions down or up:

void
MouseButton(int button, int state, int x, int y)
{
    int b = 0;			// LEFT, MIDDLE, or RIGHT

    if (DebugOn != 0)
        fprintf(stderr, "MouseButton: %d, %d, %d, %d\n", button, state, x, y);


    // get the proper button bit mask:

    switch (button)
    {
    case GLUT_LEFT_BUTTON:
        b = LEFT;		break;

    case GLUT_MIDDLE_BUTTON:
        b = MIDDLE;		break;

    case GLUT_RIGHT_BUTTON:
        b = RIGHT;		break;

    case SCROLL_WHEEL_UP:
        Scale += SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
        // keep object from turning inside-out or disappearing:
        if (Scale < MINSCALE)
            Scale = MINSCALE;
        break;

    case SCROLL_WHEEL_DOWN:
        Scale -= SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
        // keep object from turning inside-out or disappearing:
        if (Scale < MINSCALE)
            Scale = MINSCALE;
        break;

    default:
        b = 0;
        fprintf(stderr, "Unknown mouse button: %d\n", button);
    }

    // button down sets the bit, up clears the bit:

    if (state == GLUT_DOWN)
    {
        Xmouse = x;
        Ymouse = y;
        ActiveButton |= b;		// set the proper bit
    }
    else
    {
        ActiveButton &= ~b;		// clear the proper bit
    }

    glutSetWindow(MainWindow);
    glutPostRedisplay();

}


// called when the mouse moves while a button is down:

void
MouseMotion(int x, int y)
{
    // if( true )
    // 	fprintf( stderr, "MouseMotion: %d, %d\n", x, y );


    int dx = x - Xmouse;		// change in mouse coords
    int dy = y - Ymouse;

    if ((ActiveButton & LEFT) != 0)
    {
        Xrot += (ANGFACT * dy);
        Yrot += (ANGFACT * dx);
    }


    if ((ActiveButton & MIDDLE) != 0)
    {
        Scale += SCLFACT * (float)(dx - dy);

        // keep object from turning inside-out or disappearing:

        if (Scale < MINSCALE)
            Scale = MINSCALE;
    }

    Xmouse = x;			// new current position
    Ymouse = y;

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}


// reset the transformations and the colors:
// this only sets the global variables --
// the glut main loop is responsible for redrawing the scene

void
Reset()
{
    ActiveButton = 0;
    AxesOn = 0;
    DebugOn = 0;
    DepthBufferOn = 1;
    DepthFightingOn = 0;
    DepthCueOn = 0;
    Scale = 1.0;
    ShadowsOn = 0;
    WhichColor = WHITE;
    WhichProjection = PERSP;
    Xrot = Yrot = 0.;
    Frozen = false;
}


// called when user resizes the window:

void
Resize(int width, int height)
{
    if (DebugOn != 0)
        fprintf(stderr, "ReSize: %d, %d\n", width, height);

    // don't really need to do anything since window size is
    // checked each time in Display( ):

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}


// handle a change to the window's visibility:

void
Visibility(int state)
{
    if (DebugOn != 0)
        fprintf(stderr, "Visibility: %d\n", state);

    if (state == GLUT_VISIBLE)
    {
        glutSetWindow(MainWindow);
        glutPostRedisplay();
    }
    else
    {
        // could optimize by keeping track of the fact
        // that the window is not visible and avoid
        // animating or redrawing it ...
    }
}



///////////////////////////////////////   HANDY UTILITIES:  //////////////////////////


// the stroke characters 'X' 'Y' 'Z' :

static float xx[] = {
        0.f, 1.f, 0.f, 1.f
};

static float xy[] = {
        -.5f, .5f, .5f, -.5f
};

static int xorder[] = {
        1, 2, -3, 4
};

static float yx[] = {
        0.f, 0.f, -.5f, .5f
};

static float yy[] = {
        0.f, .6f, 1.f, 1.f
};

static int yorder[] = {
        1, 2, 3, -2, 4
};

static float zx[] = {
        1.f, 0.f, 1.f, 0.f, .25f, .75f
};

static float zy[] = {
        .5f, .5f, -.5f, -.5f, 0.f, 0.f
};

static int zorder[] = {
        1, 2, 3, 4, -5, 6
};

// fraction of the length to use as height of the characters:
const float LENFRAC = 0.10f;

// fraction of length to use as start location of the characters:
const float BASEFRAC = 1.10f;

//	Draw a set of 3D axes:
//	(length is the axis length in world coordinates)

void
Axes(float length)
{
    glBegin(GL_LINE_STRIP);
    glVertex3f(length, 0., 0.);
    glVertex3f(0., 0., 0.);
    glVertex3f(0., length, 0.);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f(0., 0., 0.);
    glVertex3f(0., 0., length);
    glEnd();

    float fact = LENFRAC * length;
    float base = BASEFRAC * length;

    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < 4; i++)
    {
        int j = xorder[i];
        if (j < 0)
        {

            glEnd();
            glBegin(GL_LINE_STRIP);
            j = -j;
        }
        j--;
        glVertex3f(base + fact * xx[j], fact * xy[j], 0.0);
    }
    glEnd();

    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < 5; i++)
    {
        int j = yorder[i];
        if (j < 0)
        {

            glEnd();
            glBegin(GL_LINE_STRIP);
            j = -j;
        }
        j--;
        glVertex3f(fact * yx[j], base + fact * yy[j], 0.0);
    }
    glEnd();

    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < 6; i++)
    {
        int j = zorder[i];
        if (j < 0)
        {

            glEnd();
            glBegin(GL_LINE_STRIP);
            j = -j;
        }
        j--;
        glVertex3f(0.0, fact * zy[j], base + fact * zx[j]);
    }
    glEnd();

}

// read a BMP file into a Texture:

#define VERBOSE		false
#define BMP_MAGIC_NUMBER	0x4d42
#ifndef BI_RGB
#define BI_RGB			0
#define BI_RLE8			1
#define BI_RLE4			2
#endif


// bmp file header:
struct bmfh
{
    short bfType;		// BMP_MAGIC_NUMBER = "BM"
    int bfSize;		// size of this file in bytes
    short bfReserved1;
    short bfReserved2;
    int bfOffBytes;		// # bytes to get to the start of the per-pixel data
} FileHeader;

// bmp info header:
struct bmih
{
    int biSize;		// info header size, should be 40
    int biWidth;		// image width
    int biHeight;		// image height
    short biPlanes;		// #color planes, should be 1
    short biBitCount;	// #bits/pixel, should be 1, 4, 8, 16, 24, 32
    int biCompression;	// BI_RGB, BI_RLE4, BI_RLE8
    int biSizeImage;
    int biXPixelsPerMeter;
    int biYPixelsPerMeter;
    int biClrUsed;		// # colors in the palette
    int biClrImportant;
} InfoHeader;



// read a BMP file into a Texture:

unsigned char*
BmpToTexture(char* filename, int* width, int* height)
{
    FILE* fp;
#ifdef _WIN32
    errno_t err = fopen_s(&fp, filename, "rb");
    if (err != 0)
    {
        fprintf(stderr, "Cannot open Bmp file '%s'\n", filename);
        return NULL;
    }
#else
    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "Cannot open Bmp file '%s'\n", filename);
        return NULL;
    }
#endif

    FileHeader.bfType = ReadShort(fp);


    // if bfType is not BMP_MAGIC_NUMBER, the file is not a bmp:

    if (VERBOSE) fprintf(stderr, "FileHeader.bfType = 0x%0x = \"%c%c\"\n",
        FileHeader.bfType, FileHeader.bfType & 0xff, (FileHeader.bfType >> 8) & 0xff);
    if (FileHeader.bfType != BMP_MAGIC_NUMBER)
    {
        fprintf(stderr, "Wrong type of file: 0x%0x\n", FileHeader.bfType);
        fclose(fp);
        return NULL;
    }


    FileHeader.bfSize = ReadInt(fp);
    if (VERBOSE)	fprintf(stderr, "FileHeader.bfSize = %d\n", FileHeader.bfSize);

    FileHeader.bfReserved1 = ReadShort(fp);
    FileHeader.bfReserved2 = ReadShort(fp);

    FileHeader.bfOffBytes = ReadInt(fp);
    if (VERBOSE)	fprintf(stderr, "FileHeader.bfOffBytes = %d\n", FileHeader.bfOffBytes);


    InfoHeader.biSize = ReadInt(fp);
    if (VERBOSE)	fprintf(stderr, "InfoHeader.biSize = %d\n", InfoHeader.biSize);
    InfoHeader.biWidth = ReadInt(fp);
    if (VERBOSE)	fprintf(stderr, "InfoHeader.biWidth = %d\n", InfoHeader.biWidth);
    InfoHeader.biHeight = ReadInt(fp);
    if (VERBOSE)	fprintf(stderr, "InfoHeader.biHeight = %d\n", InfoHeader.biHeight);

    const int nums = InfoHeader.biWidth;
    const int numt = InfoHeader.biHeight;

    InfoHeader.biPlanes = ReadShort(fp);
    if (VERBOSE)	fprintf(stderr, "InfoHeader.biPlanes = %d\n", InfoHeader.biPlanes);

    InfoHeader.biBitCount = ReadShort(fp);
    if (VERBOSE)	fprintf(stderr, "InfoHeader.biBitCount = %d\n", InfoHeader.biBitCount);

    InfoHeader.biCompression = ReadInt(fp);
    if (VERBOSE)	fprintf(stderr, "InfoHeader.biCompression = %d\n", InfoHeader.biCompression);

    InfoHeader.biSizeImage = ReadInt(fp);
    if (VERBOSE)	fprintf(stderr, "InfoHeader.biSizeImage = %d\n", InfoHeader.biSizeImage);

    InfoHeader.biXPixelsPerMeter = ReadInt(fp);
    InfoHeader.biYPixelsPerMeter = ReadInt(fp);

    InfoHeader.biClrUsed = ReadInt(fp);
    if (VERBOSE)	fprintf(stderr, "InfoHeader.biClrUsed = %d\n", InfoHeader.biClrUsed);

    InfoHeader.biClrImportant = ReadInt(fp);


    // fprintf( stderr, "Image size found: %d x %d\n", ImageWidth, ImageHeight );


    // pixels will be stored bottom-to-top, left-to-right:
    unsigned char* texture = new unsigned char[3 * nums * numt];
    if (texture == NULL)
    {
        fprintf(stderr, "Cannot allocate the texture array!\n");
        return NULL;
    }

    // extra padding bytes:

    int requiredRowSizeInBytes = 4 * ((InfoHeader.biBitCount * InfoHeader.biWidth + 31) / 32);
    if (VERBOSE)	fprintf(stderr, "requiredRowSizeInBytes = %d\n", requiredRowSizeInBytes);

    int myRowSizeInBytes = (InfoHeader.biBitCount * InfoHeader.biWidth + 7) / 8;
    if (VERBOSE)	fprintf(stderr, "myRowSizeInBytes = %d\n", myRowSizeInBytes);

    int oldNumExtra = 4 * (((3 * InfoHeader.biWidth) + 3) / 4) - 3 * InfoHeader.biWidth;
    if (VERBOSE)	fprintf(stderr, "Old NumExtra padding = %d\n", oldNumExtra);

    int numExtra = requiredRowSizeInBytes - myRowSizeInBytes;
    if (VERBOSE)	fprintf(stderr, "New NumExtra padding = %d\n", numExtra);


    // this function does not support compression:

    if (InfoHeader.biCompression != 0)
    {
        fprintf(stderr, "Wrong type of image compression: %d\n", InfoHeader.biCompression);
        fclose(fp);
        return NULL;
    }


    // we can handle 24 bits of direct color:
    if (InfoHeader.biBitCount == 24)
    {
        rewind(fp);
        fseek(fp, FileHeader.bfOffBytes, SEEK_SET);
        int t;
        unsigned char* tp;
        for (t = 0, tp = texture; t < numt; t++)
        {
            for (int s = 0; s < nums; s++, tp += 3)
            {
                *(tp + 2) = fgetc(fp);		// b
                *(tp + 1) = fgetc(fp);		// g
                *(tp + 0) = fgetc(fp);		// r
            }

            for (int e = 0; e < numExtra; e++)
            {
                fgetc(fp);
            }
        }
    }

    // we can also handle 8 bits of indirect color:
    if (InfoHeader.biBitCount == 8 && InfoHeader.biClrUsed == 256)
    {
        struct rgba32
        {
            unsigned char r, g, b, a;
        };
        struct rgba32* colorTable = new struct rgba32[InfoHeader.biClrUsed];

        rewind(fp);
        fseek(fp, sizeof(struct bmfh) + InfoHeader.biSize - 2, SEEK_SET);
        for (int c = 0; c < InfoHeader.biClrUsed; c++)
        {
            colorTable[c].r = fgetc(fp);
            colorTable[c].g = fgetc(fp);
            colorTable[c].b = fgetc(fp);
            colorTable[c].a = fgetc(fp);
            if (VERBOSE)	fprintf(stderr, "%4d:\t0x%02x\t0x%02x\t0x%02x\t0x%02x\n",
                c, colorTable[c].r, colorTable[c].g, colorTable[c].b, colorTable[c].a);
        }

        rewind(fp);
        fseek(fp, FileHeader.bfOffBytes, SEEK_SET);
        int t;
        unsigned char* tp;
        for (t = 0, tp = texture; t < numt; t++)
        {
            for (int s = 0; s < nums; s++, tp += 3)
            {
                int index = fgetc(fp);
                *(tp + 0) = colorTable[index].r;	// r
                *(tp + 1) = colorTable[index].g;	// g
                *(tp + 2) = colorTable[index].b;	// b
            }

            for (int e = 0; e < numExtra; e++)
            {
                fgetc(fp);
            }
        }

        delete[] colorTable;
    }

    fclose(fp);

    *width = nums;
    *height = numt;
    return texture;
}

int
ReadInt(FILE* fp)
{
    const unsigned char b0 = fgetc(fp);
    const unsigned char b1 = fgetc(fp);
    const unsigned char b2 = fgetc(fp);
    const unsigned char b3 = fgetc(fp);
    return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
}

short
ReadShort(FILE* fp)
{
    const unsigned char b0 = fgetc(fp);
    const unsigned char b1 = fgetc(fp);
    return (b1 << 8) | b0;
}


// function to convert HSV to RGB
// 0.  <=  s, v, r, g, b  <=  1.
// 0.  <= h  <=  360.
// when this returns, call:
//		glColor3fv( rgb );

void
HsvRgb(float hsv[3], float rgb[3])
{
    // guarantee valid input:

    float h = hsv[0] / 60.f;
    while (h >= 6.)	h -= 6.;
    while (h < 0.) 	h += 6.;

    float s = hsv[1];
    if (s < 0.)
        s = 0.;
    if (s > 1.)
        s = 1.;

    float v = hsv[2];
    if (v < 0.)
        v = 0.;
    if (v > 1.)
        v = 1.;

    // if sat==0, then is a gray:

    if (s == 0.0)
    {
        rgb[0] = rgb[1] = rgb[2] = v;
        return;
    }

    // get an rgb from the hue itself:

    float i = (float)floor(h);
    float f = h - i;
    float p = v * (1.f - s);
    float q = v * (1.f - s * f);
    float t = v * (1.f - (s * (1.f - f)));

    float r = 0., g = 0., b = 0.;			// red, green, blue
    switch ((int)i)
    {
    case 0:
        r = v;	g = t;	b = p;
        break;

    case 1:
        r = q;	g = v;	b = p;
        break;

    case 2:
        r = p;	g = v;	b = t;
        break;

    case 3:
        r = p;	g = q;	b = v;
        break;

    case 4:
        r = t;	g = p;	b = v;
        break;

    case 5:
        r = v;	g = p;	b = q;
        break;
    }


    rgb[0] = r;
    rgb[1] = g;
    rgb[2] = b;
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = indices.size();

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}