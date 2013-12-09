/* Authors: Bucky Frost
 * 	    Paul Gentemann
 * CS 381
 * File Name: splinepatch.cpp
 * Last Modified: Mon Dec  9 04:50:40 AKST 2013
 * Description: A pair of spline plains, patched together, that will
 *     make a wave upon user input that are textured.
 */

// OpenGL/GLUT includes - DO THESE FIRST
#include <cstdlib>       // Do this before GL/GLUT includes
using std::exit;
#ifndef __APPLE__
# include <GL/glew.h>
# include <GL/glut.h>    // Includes OpenGL headers as well
#else
# include <GLEW/glew.h>
# include <GLUT/glut.h>  // Apple puts glut.h in a different place
#endif
#ifdef _MSC_VER          // Tell MS-Visual Studio about GLEW lib
# pragma comment(lib, "glew32.lib")
#endif

#include "lib381/bitmapprinter.h" // For in app doc
#include "lib381/glslprog.h"      // For GLSL code-handling functions
#include "lib381/globj.h" 	  // For class Tex2D
#include "lib381/tshapes.h"	  // For shape drawing funcs

#include <string>
using std::string;
#include <iostream>
using std::cerr; using std::endl; using std::cout;
#include <sstream>
using std::ostringstream;
#include <iomanip>
using std::setprecision; using std::fixed;
#include <cmath>
using std::sin; using std::exp;

// Function prototypes
void documentation();
void waveFun(GLdouble *, int, int);
void drawBezierPatch(int, GLdouble *);
void myDisplay();
void myIdle();
void resetZoom();
void fixShaderFloat(GLfloat *);
void myKeyboard(unsigned char, int, int);
void myPassiveMotion(int, int);
void init();
void myReshape(int, int);

// Global variables
const int ESCKEY = 27;         // ASCII value of Escape
const int startwinsize = 600;  // Start window width & height (pixels)
int winw = 1, winh = 1;        // Window width, height (pixels)
bool help = false;
bool wireFrame;

// Shaders
bool shaderbool1 = true;
string vshader1fname;          // Filename for vertex shader source
string fshader1fname;          // Filename for fragment shader source
GLhandleARB prog1;             // GLSL Program Object
GLfloat shaderfloat1 = 1.;

// Textures
Tex2D tex0, tex1;
const int IMG_WIDTH = 256, IMG_HEIGHT = IMG_WIDTH;
GLubyte teximage1[IMG_HEIGHT*IMG_WIDTH][3];  // Temp storage for texture

int min_nonmip; // 0=NEAREST, 1=LINEAR
int min_mip;	// 0=NEAREST, 1=LINEAR, 2=NON

// Camera 
GLdouble viewmatrix[16];       
int zoom;

// Wave/Spline Patches
int numsubdivs;                // Number of subdivisions for object
const int minsubdivs = 1;      //  Minumum of above
const int maxsubdivs = 50;     //  Maximum of above
const int PTS = 4;             // Number of control points in a patch
const int EDGES = 4;           // Sides in a patch
const int SIZE = PTS*EDGES*3;  // Size of Patch Arrays
bool wave;                     // Starts the wave propagation.
GLdouble modd;                 // Value for the waves in the Bezier patches

// Bezier Patches
GLdouble b1[SIZE] = {
    3.5,-3.0, 0.0,   2.5,-2.0, 0.0,   1.5,-1.0, 0.0,   1.5, 0.0, 0.0,
    2.5,-3.0, 0.0,   0.5,-2.0, 0.0,   0.5,-1.0, 0.0,   0.5, 0.0, 0.0,
   -2.5,-3.0, 0.0,  -0.5,-2.0, 0.0,  -0.5,-1.0, 0.0,  -0.5, 0.0, 0.0,
   -3.5,-3.0, 0.0,  -2.5,-2.0, 0.0,  -1.5,-1.0, 0.0,  -1.5, 0.0, 0.0};
GLdouble b2[SIZE] = {
    1.5, 0.0, 0.0,   1.5, 1.0, 0.0,   2.5, 2.0, 0.0,   3.5, 3.0, 0.0,
    0.5, 0.0, 0.0,   0.5, 1.0, 0.0,   0.5, 2.0, 0.0,   0.5, 3.0, 0.0,
   -0.5, 0.0, 0.0,  -0.5, 1.0, 0.0,  -0.5, 2.0, 0.0,  -0.5, 3.0, 0.0,
   -1.5, 0.0, 0.0,  -1.5, 1.0, 0.0,  -2.5, 2.0, 0.0,  -3.5, 3.0, 0.0};

// waveFun
// Makes a wave through one of the axes of a spline patch. 
void waveFun(GLdouble *arr, int column, int axis)
{
    for (int i = 0; i < PTS; ++i)
    {
        int pos = 3 * PTS * i + (3 * column + axis);
        arr[pos] = exp(1-modd) * sin(2 * 3.14 * modd); // Dampened wave.
    }
}

// drawBezierPatch
// Draws a number of control points for a bezier patch. The z coordinates 
// of all the points are translated by the sine of the mod parameter.
void drawBezierPatch(int subdivs, GLdouble *cpts)
{
    glColor3d(0.,0.,0.5);
    glMap2d(GL_MAP2_VERTEX_3, 0., 1., 3, 4, 0., 1., 3*4, 4, cpts);
    glEnable(GL_MAP2_VERTEX_3);

    glMapGrid2d(subdivs, 0., 1., subdivs, 0., 1.);
    glEnable(GL_AUTO_NORMAL);

    glFrontFace(GL_CW);  // Normals are evidently backwards here :-(
    wireFrame ? glEvalMesh2(GL_LINE, 0, subdivs, 0, subdivs) 
    	      : glEvalMesh2(GL_FILL, 0, subdivs, 0, subdivs);
    glFrontFace(GL_CCW);
}

// myDisplay
// The GLUT display function
void myDisplay()
{
    GLenum minfilters[] = 
    { GL_NEAREST_MIPMAP_NEAREST,
      GL_LINEAR_MIPMAP_NEAREST,
      GL_NEAREST_MIPMAP_LINEAR,
      GL_LINEAR_MIPMAP_LINEAR,
      GL_NEAREST,
      GL_LINEAR };

    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLhandleARB theprog;  // CURRENTLY-used program object or 0 if none
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Activate shaders
    theprog = prog1;

    glEnable(GL_DEPTH_TEST);    // Set up 3D
    glLoadIdentity();           // Start with camera.
    glMultMatrixd(viewmatrix);

    // Position light source 0 & draw ball there
    glPushMatrix();
        glTranslated(-1., 1., 2.);  // Starting left, up, behind camera
        GLfloat origin[] = { 0.f, 0.f, 0.f, 1.f };
        glLightfv(GL_LIGHT0, GL_POSITION, origin);
        glUseProgramObjectARB(0);
        glColor3d(1., 1., 1.);          // white ball
        glutSolidSphere(.5, 20, 15);   // obj for light source
    glPopMatrix();

    glUseProgramObjectARB(theprog);

    //Send info to shader
    if (theprog)
    {
        GLint loc;  // Location for shader vars
        loc = glGetUniformLocationARB(theprog, "myb1");
        if (loc != -1)
        {
            glUniform1i(loc, shaderbool1);
        }
        loc = glGetUniformLocationARB(theprog, "myf1");
        if (loc != -1)
        {
            glUniform1f(loc, shaderfloat1);
        }
    }

    // Draw Objects
    glTranslated(0,0, -zoom);   // Position objects out at distance
    if(wave)
    {
        modd += 0.1;
        waveFun(b1, 3, 2);  // Last set of points, using z-coords
        waveFun(b1, 2, 2);  // Second to last set of points.
        waveFun(b2, 0, 2);  // First points to second patch.
        waveFun(b2, 1, 2);  // Second set of points.
    }

    drawBezierPatch(numsubdivs, b1);
    drawBezierPatch(numsubdivs, b2);
    documentation();    

    glutSwapBuffers();
}

// myIdle
// The GLUT idle function
void myIdle()
{
    glutPostRedisplay();
    // Print OpenGL errors, if there are any (for debugging)
    static int error_count = 0;
    if (GLenum err = glGetError())
    {
        ++error_count;
        cerr << "OpenGL ERROR " << error_count << ": "
             << gluErrorString(err) << endl;
    }
}

// fixShaderFloat
// Makes sure that the shader float does not exceed the bounds [0,1].
void fixShaderFloat(GLfloat *f)
{
    if (*f < 0.) *f = 0.;
    if (*f > 1.) *f = 1.;
}

// resetZoom
// Resets the initial zoom value (use everywhere for DRY)
void resetZoom()
{
    zoom = 5;
    glLoadIdentity();
    glGetDoublev(GL_MODELVIEW_MATRIX, viewmatrix);
}

// myKeyboard
// The GLUT keyboard function
void myKeyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case ESCKEY:  // Esc: quit
        exit(0);
        break;
    case '+':
        --zoom;
        break;
    case '-':
        ++zoom;
        break;
    case '.':   // Insert a splash at point
        
        break;
    case 'R':
    case 'r':
        resetZoom();
        break;
    case ' ':
        wave = true;
        modd = 0.;
        break;
    case '(':
        if(numsubdivs > minsubdivs)
            --numsubdivs;
        break;
    case ')':
        if(numsubdivs < maxsubdivs)
            ++numsubdivs;
        break;
    case 'w':
    case 'W':
    	wireFrame = !wireFrame;
	break;
    case 'h':
    case 'H':
        help = !help;
        break;
    case 'f':     // Space: toggle shader bool
        shaderbool1 = !shaderbool1;
        break;
    case '[':     // [: Decrease shader float
        shaderfloat1 -= 0.02;
        fixShaderFloat(&shaderfloat1);
        break;
    case ']':     // ]: Increase shader float
        shaderfloat1 += 0.02;
        fixShaderFloat(&shaderfloat1);
        break;
    }
    glutPostRedisplay();
}

// mySpecial
// The GLUT special function
void mySpecial(int key, int x, int y)
{
    glTranslated(0., 0., -zoom);
    switch (key)
    {
        case GLUT_KEY_LEFT:
            glRotated(-5, 0., 1., 0.);
            break;
        case GLUT_KEY_RIGHT:
            glRotated(5, 0., 1., 0.);
            break;
        case GLUT_KEY_UP:
            glRotated(-5, 1., 0., 0.);
            break;
        case GLUT_KEY_DOWN:
            glRotated(5, 1., 0., 0.);
            break;
    }
    glTranslated(0., 0., zoom);
    glMultMatrixd(viewmatrix);
    glGetDoublev(GL_MODELVIEW_MATRIX, viewmatrix);

    glutPostRedisplay();
}

// myReshape
// The GLUT reshape function
void myReshape(int w, int h)
{
    // Set viewport & save window dimensions in globals
    glViewport(0, 0, w, h);
    winw = w;
    winh = h;
    // Set up projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60., double(w)/h, 0.01, 100.);
    glMatrixMode(GL_MODELVIEW);  // Always go back to model/view mode
}

// init
// Initialize GL states & global data
void init()
{
    resetZoom();    // Reset camera position to default.
    glMatrixMode(GL_MODELVIEW);
    modd = 0.0;
    wave = false;
    numsubdivs = 10;
    wireFrame = false;

    // Shaders
    prog1 = makeProgramObjectFromFiles(vshader1fname, fshader1fname);
}

// The main
int main(int argc, char ** argv)
{
    // Initilization of OpenGL/GLUT
    glutInit(&argc, argv);
    // Set shader source filenames. Done here, as opposed to init() so we can 
    // use command line arguments.
    getShaderFilenames(vshader1fname, fshader1fname, argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    // Creating the view window
    glutInitWindowSize(startwinsize, startwinsize);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("CS 381 - Shaders, Normals, Lighting, and Splines Oh My!");

    // Init GLEW & check status - exit on failure
    if (glewInit() != GLEW_OK)
    {
        cerr << "glewInit failed" << endl;
        exit(1);
    }

    init();
    glutDisplayFunc(myDisplay);
    glutIdleFunc(myIdle);
    glutReshapeFunc(myReshape);
    glutKeyboardFunc(myKeyboard);
    glutSpecialFunc(mySpecial);

    // Start GLUT event handling loop
    glutMainLoop();

    return 0;
}

void documentation()
{
    // Draw documentation
    glDisable(GL_DEPTH_TEST);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);  // Set up simple ortho projection
    glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0., double(winw), 0., double(winh));
        glColor3d(0., 0., 0.);        // Black text
        BitmapPrinter p(20., winh - 20., 20.);
        if(help)
        {
            ostringstream os1;
            ostringstream os2;
            os1 << fixed << setprecision(2) << shaderfloat1;
            os2 << fixed << setprecision(2) << numsubdivs;
            p.print("Arrows         Rotate Scene");
            p.print("[ ]            Change Lighting (" + os1.str() + ")");
            p.print("( )            Change Subdivisions (" + os2.str() + ")");
            p.print("+/-            Zoom in/out");
            p.print("r              Reset Camera");
            p.print("space          Distort Spline");
            p.print(string("w              Wire-Frame (" )
	    		+ (wireFrame ? "true" : "false") + ")");
            p.print(string("f              Lighting (" )
                        + (shaderbool1 ? "true" : "false") + ")");
        }
        else
        {
            p.print("h              help");
        }
        p.print("Esc            Quit");
    glPopMatrix();                // Restore prev projection
    glMatrixMode(GL_MODELVIEW);
}
