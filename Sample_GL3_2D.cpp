#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <time.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <ao/ao.h>
#include <mpg123.h>

#define BITS 8

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

GLFWwindow* window;
float SCREEN_ZOOM_STEP = 0.03;
int score=0,numberOfBlack=0,numberOfMismatch=0;
float redx=1.5,greenx=-1.5,turret_angle=1,turrety=0;
float screen_left,screen_right,screen_top,screen_bottom;
float camera_rotation_angle = 90,laser_x,laser_y;
float turret_rectangle_rotation = 0,rectangle_rotation=0,laser_rotation;
float screen_x=0,screen_y=0,blockSpeed=0.010,zoom=1.0,CURSOR_X=0,CURSOR_Y=0;
//int draw_flag = 0, number_of_blocks = 0, i;
vector <float> block_x;
vector <float> block_y;
vector <int> block_color;
bool redbucket_clicked = false, greenbucket_clicked = false, turret_clicked = false;
float mirror_y=0.2;
int laserFlag=0;
struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
    glm::mat4 projection;
    glm::mat4 model;
    glm::mat4 view;
    GLuint MatrixID;
} Matrices;

GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if(VertexShaderStream.is_open())
    {
        std::string Line = "";
        while(getline(VertexShaderStream, Line))
            VertexShaderCode += "\n" + Line;
        VertexShaderStream.close();
    }

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if(FragmentShaderStream.is_open()){
        std::string Line = "";
        while(getline(FragmentShaderStream, Line))
            FragmentShaderCode += "\n" + Line;
        FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    printf("Compiling shader : %s\n", vertex_file_path);
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> VertexShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
    fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

    // Compile Fragment Shader
    printf("Compiling shader : %s\n", fragment_file_path);
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
    fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

    // Link the program
    fprintf(stdout, "Linking program\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
    fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    //    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                0,                  // attribute 0. Vertices
                3,                  // size (x,y,z)
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)0            // array buffer offset
                );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                1,                  // attribute 1. Color
                3,                  // size (r,g,b)
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)0            // array buffer offset
                );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

//float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
//bool triangle_rot_status = true;
bool rectangle_rot_status = true;

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */

/*void updateZoomAndPan () {
    float top = (screen_y + 4) / zoom;
    float bottom = (screen_y - 4) / zoom;
    float left = (screen_x - 4) / zoom;
    float right = (screen_x + 4) / zoom;

    Matrices.projection = glm::ortho(left, right, bottom, top, 0.1f, 500.0f);
}*/

void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Function is called first on GLFW_PRESS.
    if (action == GLFW_RELEASE) {
        switch (key) {
       // case GLFW_KEY_C:
         //   rectangle_rot_status = !rectangle_rot_status;
            break;
            //  case GLFW_KEY_:
            //    triangle_rot_status = !triangle_rot_status;
            //  break;
        //case GLFW_KEY_X:
            // do something ..
          //  break;
        default:
            break;
        }
    }
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_SPACE:
            laserFlag=1;
            laser_x=-4;
            laser_y=turrety;
            laser_rotation=turret_rectangle_rotation;
            break;
        case GLFW_KEY_ESCAPE:
            if(score > 100)
                cout << "YOU WON" << endl;
            if(score < 100)
                cout << "YOU LOST" << endl;
            cout << score << endl;

            quit(window);
            break;
        case GLFW_KEY_M:
            blockSpeed+=0.001;
            if(blockSpeed>=0.020)
                blockSpeed=0.020;
            break;
        case GLFW_KEY_N:
            blockSpeed-=0.001;
            if(blockSpeed<=0.005)
                blockSpeed=0.005;
            break;
        case GLFW_KEY_LEFT:
            if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
            {
                redx=redx-0.1;
                if(redx<-2.5)
                    redx=-2.5;
                break;
            }
            else if(glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL))
            {
                redx=redx-0.1;
                if(redx<-2.5)
                    redx=-2.5;
                break;
            }
            else if(glfwGetKey(window, GLFW_KEY_RIGHT_ALT))
            {
                greenx=greenx-0.1;
                if(greenx<-2.5)
                    greenx=-2.5;
                break;
            }
            else if(glfwGetKey(window, GLFW_KEY_LEFT_ALT))
            {
                greenx=greenx-0.1;
                if(greenx<-2.5)
                    greenx=-2.5;
                break;
            }
            else
            {
                screen_x-=0.1;
                if(screen_x<-4)
                    screen_x=-4;
                break;
            }
        case GLFW_KEY_RIGHT:
            if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
            {
                redx=redx+0.1;
                if(redx>2.5)
                    redx=2.5;
                break;
            }
            else if(glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL))
            {
                redx=redx+0.1;
                if(redx>2.5)
                    redx=2.5;
                break;
            }
            else if(glfwGetKey(window, GLFW_KEY_RIGHT_ALT))
            {
                greenx=greenx+0.1;
                if(greenx>2.5)
                    greenx=2.5;
                break;
            }
            else if(glfwGetKey(window, GLFW_KEY_LEFT_ALT))
            {
                greenx=greenx+0.1;
                if(greenx>2.5)
                    greenx=2.5;
                break;
            }
            else
            {
                screen_x+=0.1;
                if(screen_x>=3.0)
                    screen_x=3.0;
                break;
            }
        case GLFW_KEY_S:
            turrety=turrety+0.1;
            if(turrety>3.5)
                turrety=3.5;
            break;
        case GLFW_KEY_F:
            turrety=turrety-0.1;
            if(turrety<-3.5)
                turrety=-3.5;
            break;
        case GLFW_KEY_A:
            turret_rectangle_rotation+=3;
            if(turret_rectangle_rotation>=90)
                turret_rectangle_rotation=90;
            break;
        case GLFW_KEY_D:
            turret_rectangle_rotation-=3;
            if(turret_rectangle_rotation<=-90)
                turret_rectangle_rotation=-90;
            break;
        case GLFW_KEY_UP:
           zoom+=0.1;
            if(zoom>=2.0)
                zoom=2.0;
            break;
        case GLFW_KEY_DOWN:
            zoom-=0.1;
            if(zoom<=1.0)
                zoom=1.0;
            break;
        default:
            break;
        }
    }
    if (action== GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_SPACE:
            laserFlag=1;
            laser_x=-4;
            laser_y=turrety;
            laser_rotation=turret_rectangle_rotation;
            break;
        case GLFW_KEY_ESCAPE:
            if(score > 100)
                cout << "YOU WON" << endl;
            if(score < 100)
                cout << "YOU LOST" << endl;
            cout << score << endl;
            quit(window);
            break;
        case GLFW_KEY_M:
            blockSpeed+=0.001;
            if(blockSpeed>=0.020)
                blockSpeed=0.020;
            break;
        case GLFW_KEY_N:
            blockSpeed-=0.001;
            if(blockSpeed<=0.005)
                blockSpeed=0.005;
            break;
        case GLFW_KEY_LEFT:
            if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
            {
                redx=redx-0.2;
                if(redx<-2.5)
                    redx=-2.5;
                break;
            }
            else if(glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL))
            {
                redx=redx-0.2;
                if(redx<-2.5)
                    redx=-2.5;
                break;
            }
            else if(glfwGetKey(window, GLFW_KEY_RIGHT_ALT))
            {
                greenx=greenx-0.2;
                if(greenx<-2.5)
                    greenx=-2.5;
                break;
            }
            else if(glfwGetKey(window, GLFW_KEY_LEFT_ALT))
            {
                greenx=greenx-0.2;
                if(greenx<-2.5)
                    greenx=-2.5;
                break;
            }
            else
            {
                screen_x-=0.2;
                if(screen_x<=-4.0)
                    screen_x=-4.0;
                break;
            }
        case GLFW_KEY_RIGHT:
            if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
            {
                redx=redx+0.2;
                if(redx>2.5)
                    redx=2.5;
                break;
            }
            else if(glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL))
            {
                redx=redx+0.2;
                if(redx>2.5)
                    redx=2.5;
                break;
            }
            else if(glfwGetKey(window, GLFW_KEY_RIGHT_ALT))
            {
                greenx=greenx+0.2;
                if(greenx>2.5)
                    greenx=2.5;
                break;
            }
            else if(glfwGetKey(window, GLFW_KEY_LEFT_ALT))
            {
                greenx=greenx+0.2;
                if(greenx>2.5)
                    greenx=2.5;
                break;
            }
            else
            {
                screen_x+=0.2;
                if(screen_x>=3.0)
                    screen_x=3.0;
                break;
            }
        case GLFW_KEY_S:
            turrety=turrety+0.2;
            if(turrety>3.5)
                turrety=3.5;
            break;
        case GLFW_KEY_F:
            turrety=turrety-0.2;
            if(turrety<-3.5)
                turrety=-3.5;
            break;
        case GLFW_KEY_A:
            turret_rectangle_rotation+=5;
            if(turret_rectangle_rotation>=90)
                turret_rectangle_rotation=90;
            break;
        case GLFW_KEY_D:
            turret_rectangle_rotation-=5;
            if(turret_rectangle_rotation<=-90)
                turret_rectangle_rotation=-90;
            break;
        case GLFW_KEY_UP:
           zoom+=0.1;
            if(zoom>=2.0)
                zoom=2.0;
            break;
        case GLFW_KEY_DOWN:
            zoom-=0.1;
            if(zoom<=1.0)
                zoom=1.0;
            break;
        default:
            break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
    switch (key) {
    case 'Q':
    case 'q':
        if(score > 100)
            cout << "YOU WON" << endl;
        if(score < 100)
            cout << "YOU LOST" << endl;
        cout << score << endl;
        quit(window);
        break;
    default:
        break;
    }
}

pair<float, float> toRelativeCoords (float screen_x, float screen_y) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    return make_pair((8.0 * screen_x / width) - 4, 4 - (8.0 * screen_y / height));
}

void cursorMove (GLFWwindow *window, double scree_x, double scree_y) {

    // Convert screen coords to world coords
    pair<float, float> p = toRelativeCoords((float) scree_x, (float) scree_y);
    float x = p.first, y = p.second;

    // Update global cursor position variables
    CURSOR_X = x, CURSOR_Y = y;

    // Orient the cannon appropriately
    turret_angle = atan( (y - turrety) / (x + 4)) * 180.0f / M_PI;
    if (laserFlag==0) laser_rotation = turret_angle;

    // Handle clicking on the buckets and cannon
    if (redbucket_clicked)
        redx = CURSOR_X;
    if (greenbucket_clicked)
        greenx = CURSOR_X;

    if (turret_clicked== true )
        turrety = CURSOR_Y;
}

/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

    GLfloat fov = 90.0f;

    // sets the viewport of openGL renderer
    glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

    // set the projection matrix as perspective
    /* glMatrixMode (GL_PROJECTION);
       glLoadIdentity ();
       gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
    // Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}

void scroll_callback (GLFWwindow *window, double xoffset, double yoffset) {
    if (yoffset <= 0 && zoom <= 1) return;
    else {
        zoom += yoffset * SCREEN_ZOOM_STEP;
        //updateZoomAndPan();
        screen_left=(screen_x-4.0)/zoom;
        screen_right=(4.0+screen_x)/zoom;
        screen_top=-(screen_y-4.0)/zoom;
        screen_bottom=-(4+screen_y)/zoom;
    }
}


void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            //if (action == GLFW_RELEASE)
            //    triangle_rot_dir *= -1;
            if (action == GLFW_PRESS)
            {
                float temp_angle= atan((CURSOR_Y-turrety)/(CURSOR_X+4))*180.0f/M_PI;
                if(laserFlag==0)
                    laserFlag=1;
                turret_rectangle_rotation=temp_angle;
                laser_rotation=temp_angle;
                laser_x=-4;
                laser_y=turrety;
            }
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_PRESS) {
                // Check if buckets are being clicked
                if (CURSOR_X <= redx + 1.6 / 2.0 && CURSOR_X >= redx - 1.6 / 2.0 && CURSOR_Y >= -4.0 && CURSOR_Y <= -(4.0 - 0.8))
                    redbucket_clicked = true;
                else if (CURSOR_X <= greenx + 1.6 / 2.0 && CURSOR_X >= greenx - 1.6 / 2.0 && CURSOR_Y >= -4.0 && CURSOR_Y <= -(4.0 - 0.8))
                    greenbucket_clicked = true;

                // Check if cannon is being clicked
                else if (CURSOR_X >= -4 && CURSOR_X <= 0.5 && CURSOR_Y >= turrety - 0.3 && CURSOR_Y <= turrety + 0.3)
                    turret_clicked = true;
            }
            if (action == GLFW_RELEASE) {
//                rectangle_rot_dir *= -1;

                // Reset everything clickable as unclicked
                redbucket_clicked = false;
                greenbucket_clicked = false;
                turret_clicked = false;
            }
            break;
        default:
            break;
    }
}

//VAO *triangle, *rectangle;
VAO  *triangle, *red_rectangle, *green_rectangle, *turret_rectangle,*mirror1, *mirror2, *mirror3, *mirror4, *rectangle[3],*laser;
//vector <VAO*> rectangle;
// Creates the triangle object used in this sample code
//void createTriangle ()
//{
/* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

/* Define vertex array as used in glBegin (GL_TRIANGLES) */
//  static const GLfloat vertex_buffer_data [] = {
//    0, 1,0, // vertex 0
//    -1,-1,0, // vertex 1
//    1,-1,0, // vertex 2
//  };

//  static const GLfloat color_buffer_data [] = {
//    1,0,0, // color 0
//    0,1,0, // color 1
//    0,0,1, // color 2
//  };

// create3DObject creates and returns a handle to a VAO that can be used later
//  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
//}

// Creates the rectangle object used in this sample code
void createRedRectangle ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data [] = {
        -0.8,-0.25,0, // vertex 1
        0.8,-0.25,0, // vertex 2
        0.8, 0.25,0, // vertex 3

        0.8, 0.25,0, // vertex 3
        -0.8, 0.25,0, // vertex 4
        -0.8,-0.25,0,  // vertex 1

    };

    static const GLfloat color_buffer_data [] = {
        1,0,0, // color 1
        1,0,0, // color 2
        1,0,0, // color 3

        1,0,0, // color 3
        1,0,0, // color 4
        1,0,0,  // color 1

    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    red_rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createLaser()
{
    static const GLfloat vertex_buffer_data [] = {
        -0.2,-0.01,0, // vertex 1
        0.2,-0.01,0, // vertex 2
        0.2, 0.01,0, // vertex 3

        0.2, 0.01,0, // vertex 3
        -0.2, 0.01,0, // vertex 4
        -0.2,-0.01,0,  // vertex 1

    };

    static const GLfloat color_buffer_data [] = {
        0.6,0.2,0.9, // color 1
        0.6,0.2,0.9,
        0.6,0.2,0.9,
        0.6,0.2,0.9,
        0.6,0.2,0.9,
        0.6,0.2,0.9,
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    laser = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createMirror1 ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data [] = {
        -0.4,-0.05,0, // vertex 1
        0.4,-0.05,0, // vertex 2
        0.4, 0.05,0, // vertex 3

        0.4, 0.05,0, // vertex 3
        -0.4, 0.05,0, // vertex 4
        -0.4,-0.05,0,  // vertex 1

    };

    static const GLfloat color_buffer_data [] = {
        0,0,0, // color 1
        0,0,0, // color 2
        0,0,0, // color 3

        0,0,0, // color 3
        0,0,0, // color 4
        0,0,0,  // color 1

    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    mirror1 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createMirror2 ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data [] = {
        -0.4,-0.05,0, // vertex 1
        0.4,-0.05,0, // vertex 2
        0.4, 0.05,0, // vertex 3

        0.4, 0.05,0, // vertex 3
        -0.4, 0.05,0, // vertex 4
        -0.4,-0.05,0,  // vertex 1

    };

    static const GLfloat color_buffer_data [] = {
        0,0,0, // color 1
        0,0,0, // color 2
        0,0,0, // color 3

        0,0,0, // color 3
        0,0,0, // color 4
        0,0,0,  // color 1

    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    mirror2 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createMirror3 ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data [] = {
        -0.4,-0.05,0, // vertex 1
        0.4,-0.05,0, // vertex 2
        0.4, 0.05,0, // vertex 3

        0.4, 0.05,0, // vertex 3
        -0.4, 0.05,0, // vertex 4
        -0.4,-0.05,0,  // vertex 1

    };

    static const GLfloat color_buffer_data [] = {
        0,0,0, // color 1
        0,0,0, // color 2
        0,0,0, // color 3

        0,0,0, // color 3
        0,0,0, // color 4
        0,0,0,  // color 1

    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    mirror3 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createMirror4 ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data [] = {
        -0.4,-0.05,0, // vertex 1
        0.4,-0.05,0, // vertex 2
        0.4, 0.05,0, // vertex 3

        0.4, 0.05,0, // vertex 3
        -0.4, 0.05,0, // vertex 4
        -0.4,-0.05,0,  // vertex 1

    };

    static const GLfloat color_buffer_data [] = {
        0,0,0, // color 1
        0,0,0, // color 2
        0,0,0, // color 3

        0,0,0, // color 3
        0,0,0, // color 4
        0,0,0,  // color 1

    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    mirror4 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createGreenRectangle ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data [] = {

        -0.8,-0.25,0, // vertex 1
        0.8,-0.25,0, // vertex 2
        0.8, 0.25,0, // vertex 3

        0.8, 0.25,0, // vertex 3
        -0.8, 0.25,0, // vertex 4
        -0.8,-0.25,0,  // vertex 1
    };

    static const GLfloat color_buffer_data [] = {

        0,1,0, // color 1
        0,1,0, // color 2
        0,1,0, // color 3

        0,1,0, // color 3
        0,1,0, // color 4
        0,1,0  // color 1
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    green_rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void CreateTurret()
{
    static const GLfloat vertex_buffer_data [] = {

        -0.3,0.05,0, // vertex 1
        0.3,0.05,0, // vertex 2
        0.3,-0.05,0, // vertex 3

        0.3,-0.05,0, // vertex 3
        -0.3,-0.05,0, // vertex 4
        -0.3,0.05,0,  // vertex 1

    };

    static const GLfloat color_buffer_data [] = {

        0.23,0.23,1.23, // color 1
        0.23,0.23,1.23, // color 2
        0.23,0.23,1.23, // color 3

        0.23,0.23,1.23, // color 1
        0.23,0.23,1.23, // color 2
        0.23,0.23,1.23,//color 1
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    turret_rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);

}

void CreateRedBlock()
{
    static const GLfloat vertex_buffer_data [] = {

        -0.3,0.2,0, // vertex 1
        0.3,0.2,0, // vertex 2
        0.3,-0.2,0, // vertex 3

        0.3,-0.2,0, // vertex 3
        -0.3,-0.2,0, // vertex 4
        -0.3,0.2,0,  // vertex 1

    };

    static const GLfloat color_buffer_data [] = {

        1,0,0, // color 1
        1,0,0, // color 1
        1,0,0, // color 1

        1,0,0, // color 1
        1,0,0, // color 1
        1,0,0, // color 1
    };
    // create3DObject creates and returns a handle to a VAO that can be used later
    rectangle[0] = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createGreenBlock()
{
    static const GLfloat vertex_buffer_data [] = {

        -0.3,0.2,0, // vertex 1
        0.3,0.2,0, // vertex 2
        0.3,-0.2,0, // vertex 3

        0.3,-0.2,0, // vertex 3
        -0.3,-0.2,0, // vertex 4
        -0.3,0.2,0,  // vertex 1

    };

    static const GLfloat color_buffer_data [] = {

        0,1,0, // color 1
        0,1,0, // color 1
        0,1,0, // color 1
        0,1,0, // color 1
        0,1,0, // color 1
        0,1,0, // color 1

    };
    // create3DObject creates and returns a handle to a VAO that can be used later
    rectangle[1] = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createBlackBlock()
{
    static const GLfloat vertex_buffer_data [] = {

        -0.3,0.2,0, // vertex 1
        0.3,0.2,0, // vertex 2
        0.3,-0.2,0, // vertex 3

        0.3,-0.2,0, // vertex 3
        -0.3,-0.2,0, // vertex 4
        -0.3,0.2,0,  // vertex 1

    };

    static const GLfloat color_buffer_data [] = {

        0,0,0, // color 1
        0,0,0, // color 1
        0,0,0, // color 1
        0,0,0, // color 1
        0,0,0, // color 1
        0,0,0, // color 1

    };
    // create3DObject creates and returns a handle to a VAO that can be used later
    rectangle[2] = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

//float camera_rotation_angle = 90;
//float turret_rectangle_rotation = 0,rectangle_rotation=0;
//float triangle_rotation = 0;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
/*void draw_block()
{

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    // For each model you render, since the MVP will be different (at least the M part)
    //  Don't change unless you are sure!!
    glm::mat4 MVP;	// MVP = Projection * View * Model

    int i;
    for(i=0;i<block_x.size();i++)
    {
        Matrices.model = glm::mat4(1.0f);

        glm::mat4 translateRectangle = glm::translate (glm::vec3(block_x[i], block_y[i], 0));        // glTranslatef
        glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
        Matrices.model *= (translateRectangle * rotateRectangle);
        MVP = VP * Matrices.model;
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

        // draw3DObject draws the VAO given to it using current MVP matrix
        draw3DObject(rectangle[block_color[i]]);
    }
}*/

void draw ()
{
    glClearColor(0.3,0.1,0.2,0.7);
    // clear the color and depth in the frame buffer
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use the loaded shader program
    // Don't change unless you know what you are doing
    glUseProgram (programID);

    // Eye - Location of camera. Don't change unless you are sure!!
    glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 target (0, 0, 0);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (0, 1, 0);

    // Compute Camera matrix (view)
    // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
    //  Don't change unless you are sure!!
    Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

    // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
    //  Don't change unless you are sure!!
    glm::mat4 VP = Matrices.projection * Matrices.view;

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    // For each model you render, since the MVP will be different (at least the M part)
    //  Don't change unless you are sure!!
    glm::mat4 MVP;	// MVP = Projection * View * Model

    // Load identity to model matrix
    //Matrices.model = glm::mat4(1.0f);

    /* Render your scene */

    //glm::mat4 translateTriangle = glm::translate (glm::vec3(-2.0f, 0.0f, 0.0f)); // glTranslatef
    //glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
    //glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
    //Matrices.model *= triangleTransform;
    //MVP = VP * Matrices.model; // MVP = p * V * M

    //  Don't change unless you are sure!!
    //glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    //draw3DObject(triangle);

    // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
    // glPopMatrix ();
    //float screen_left,screen_right,screen_top,screen_bottom;
    screen_left=(screen_x-4.0)/zoom;
    screen_right=(4.0+screen_x)/zoom;
    screen_top=-(screen_y-4.0)/zoom;
    screen_bottom=-(4+screen_y)/zoom;
    Matrices.projection = glm::ortho(screen_left, screen_right, screen_bottom, screen_top, 0.1f, 500.0f);

    if(laserFlag==1)
    {
        Matrices.model = glm::mat4(1.0f);

        glm::mat4 translateLaser = glm::translate (glm::vec3(laser_x, laser_y, 0));        // glTranslatef
        glm::mat4 rotateLaser = glm::rotate((float)(laser_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
        Matrices.model *= (translateLaser * rotateLaser);
        MVP = VP * Matrices.model;
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

        draw3DObject(laser);
    }

    Matrices.model = glm::mat4(1.0f);

    glm::mat4 translateRedRectangle = glm::translate (glm::vec3(redx, -3.45, 0));        // glTranslatef
    glm::mat4 rotateRedRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translateRedRectangle * rotateRedRectangle);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(red_rectangle);

    Matrices.model = glm::mat4(1.0f);

    glm::mat4 translateGreenRectangle = glm::translate (glm::vec3(greenx, -3.45, 0));        // glTranslatef
    glm::mat4 rotateGreenRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translateGreenRectangle * rotateGreenRectangle);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix

    draw3DObject(green_rectangle);

    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateTurretRectangle1 = glm::translate (glm::vec3(0.3, 0, 0));
    glm::mat4 translateTurretRectangle = glm::translate (glm::vec3(-4.0, turrety, 0));        // glTranslatef
    glm::mat4 rotateTurretRectangle = glm::rotate((float)(turret_rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translateTurretRectangle * rotateTurretRectangle * translateTurretRectangle1);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(turret_rectangle);

    Matrices.model = glm::mat4(1.0f);

    glm::mat4 translateMirror1 = glm::translate (glm::vec3(-1.7, 2, 0));        // glTranslatef
    glm::mat4 rotateMirror1 = glm::rotate((float)(135*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translateMirror1 * rotateMirror1);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(mirror1);

    Matrices.model = glm::mat4(1.0f);

    glm::mat4 translateMirror2 = glm::translate (glm::vec3(-0.3, -1, 0));        // glTranslatef
    glm::mat4 rotateMirror2 = glm::rotate((float)(45*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translateMirror2 * rotateMirror2);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(mirror2);

    Matrices.model = glm::mat4(1.0f);

    glm::mat4 translateMirror3 = glm::translate (glm::vec3(1.7, 3.0, 0));        // glTranslatef
    glm::mat4 rotateMirror3 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translateMirror3 * rotateMirror3);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(mirror3);

    Matrices.model = glm::mat4(1.0f);

    glm::mat4 translateMirror4 = glm::translate (glm::vec3(2.7, mirror_y, 0));        // glTranslatef
    glm::mat4 rotateMirror4 = glm::rotate((float)(90*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translateMirror4 * rotateMirror4);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    draw3DObject(mirror4);

    Matrices.model = glm::mat4(1.0f);

    int i;
    for(i=0;i<block_x.size();i++)
    {
        Matrices.model = glm::mat4(1.0f);

        glm::mat4 translateRectangle = glm::translate (glm::vec3(block_x[i], block_y[i], 0));        // glTranslatef
        glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
        Matrices.model *= (translateRectangle * rotateRectangle);
        MVP = VP * Matrices.model;
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

        // draw3DObject draws the VAO given to it using current MVP matrix
        draw3DObject(rectangle[block_color[i]]);
    }

    // Increment angles
    float increments = 1;

    //camera_rotation_angle++; // Simulating camera rotation
    //triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
    //rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        //        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
        //        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */

    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    // Register function to handle cursor movment
    glfwSetCursorPosCallback(window, cursorMove);

    // Register function to handle mouse scroll
    glfwSetScrollCallback(window, scroll_callback);


    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
    // Create the models
    //createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
    createRedRectangle ();
    createGreenRectangle();
    CreateTurret();
    createMirror1();
    createMirror2();
    createMirror3();
    createMirror4();
    createBlackBlock();
    createGreenBlock();
    CreateRedBlock();
    createLaser();
    // Create and compile our GLSL program from the shaders
    programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
    // Get a handle for our "MVP" uniform
    Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


    reshapeWindow (window, width, height);

    // Background color of the scene
    glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
    glClearDepth (1.0f);

    glEnable (GL_DEPTH_TEST);
    glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{

    mpg123_handle *mh;
    unsigned char *buffer;
    size_t buffer_size;
    size_t done;
    int err;

    int driver;
    ao_device *dev;

    ao_sample_format format;
    int channels, encoding;
    long rate;

    // if(argc < 2)
    //   exit(0);

    /* initializations */
    ao_initialize();
    driver = ao_default_driver_id();
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    buffer_size = 3200;
    buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

    /* open the file and get the decoding format */
    mpg123_open(mh, "q.mp3");
    mpg123_getformat(mh, &rate, &channels, &encoding);

    /* set the output format and open the output device */
    format.bits = mpg123_encsize(encoding) * BITS;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    dev = ao_open_live(driver, &format, NULL);


    int width = 600;
    int height = 600;

    window = initGLFW(width, height);
    srand(time(NULL));

    initGL (window, width, height);

    laser_rotation=rectangle_rotation;

    double last_update_time = glfwGetTime(), current_time;

    laser_rotation=rectangle_rotation;
    /* Draw in loop */
    while (!glfwWindowShouldClose(window))
    {
        if(laser_x<-5.0 || laser_x>5.0 || laser_y>5.0 || laser_y<-5.0)
            laserFlag=0;
        mirror_y+=0.01;
        if(mirror_y>2.5)
            mirror_y=-2.5;
        if (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
            ao_play(dev, (char*) buffer, done);
        else mpg123_seek(mh, 0, SEEK_SET);

        int i;
        float x_tmp=0,head_x,head_y;
        for(i=0;i<block_x.size();i++)
        {
            x_tmp=0;
            if(laserFlag==1)
            {
                while(x_tmp>=-0.2)
                {
                    head_y=laser_y+(0.01*sin(laser_rotation*M_PI/180));
                    head_x=laser_x+(0.2*cos(laser_rotation*M_PI/180))+x_tmp;
                    if(head_x<=block_x[i]+0.31 && head_x>=block_x[i]-0.31 && head_y<=block_y[i]+0.21 && head_y>=block_y[i]-0.21)
                    {
                        block_x.erase(block_x.begin()+i);
                        block_color.erase(block_color.begin()+i);
                        block_y.erase(block_y.begin()+i);
                        score+=30;
                        laserFlag=0;
                        break;
                    }
                    x_tmp-=0.02;
                }
            }
        }
        if(laserFlag==1)
        {
            double leading_point_x = (double)laser_x + ((double)0.4 / 2.0) * cos((double)laser_rotation*M_PI/180.0f);
            double leading_point_y = (double)laser_y + ((double)0.4 / 2.0) * sin((double)laser_rotation*M_PI/180.0f);


            double mirror_ax = -1.7 + (0.8 / 2.0) * cos(135*M_PI/180.0f);
            double mirror_ay = 2 + (0.8 / 2.0) * sin(135*M_PI/180.0f);
            double mirror_bx = -1.7 - (0.8 / 2.0) * cos(135*M_PI/180.0f);
            double mirror_by = 2 - (0.8 / 2.0) * sin(135*M_PI/180.0f);

            double d1 = sqrt(pow(mirror_ax - leading_point_x, 2) + pow(mirror_ay - leading_point_y, 2));
            double d2 = sqrt(pow(mirror_bx - leading_point_x, 2) + pow(mirror_by - leading_point_y, 2));

            if (fabs(d1 + d2-0.8)<=0.02)
            {
                //            resetBeam();
                laser_rotation = 2.0 * 135 - laser_rotation;
                //cout << laser_rotation << endl;
                laser_x = leading_point_x + (0.4 / 2.0) * cos((double)laser_rotation*M_PI/180.0f);
                laser_y = leading_point_y + (0.4/ 2.0) * sin((double)laser_rotation*M_PI/180.0f);
            }
        }
        if(laserFlag==1)
        {
            double leading_point_x = (double)laser_x + ((double)0.4 / 2.0) * cos((double)laser_rotation*M_PI/180.0f);
            double leading_point_y = (double)laser_y + ((double)0.4 / 2.0) * sin((double)laser_rotation*M_PI/180.0f);


            double mirror_ax = -0.3 + (0.8 / 2.0) * cos(45*M_PI/180.0f);
            double mirror_ay = -1 + (0.8 / 2.0) * sin(45*M_PI/180.0f);
            double mirror_bx = -0.3 - (0.8 / 2.0) * cos(45*M_PI/180.0f);
            double mirror_by = -1 - (0.8 / 2.0) * sin(45*M_PI/180.0f);

            double d1 = sqrt(pow(mirror_ax - leading_point_x, 2) + pow(mirror_ay - leading_point_y, 2));
            double d2 = sqrt(pow(mirror_bx - leading_point_x, 2) + pow(mirror_by - leading_point_y, 2));

            if (fabs(d1 + d2-0.8)<=0.02)
            {
                //            resetBeam();
                laser_rotation = 2.0 * 45 - laser_rotation;
               // cout << laser_rotation << endl;
                laser_x = leading_point_x + (0.4 / 2.0) * cos((double)laser_rotation*M_PI/180.0f);
                laser_y = leading_point_y + (0.4/ 2.0) * sin((double)laser_rotation*M_PI/180.0f);
            }
        }
        if(laserFlag==1)
        {
            double leading_point_x = (double)laser_x + ((double)0.4 / 2.0) * cos((double)laser_rotation*M_PI/180.0f);
            double leading_point_y = (double)laser_y + ((double)0.4 / 2.0) * sin((double)laser_rotation*M_PI/180.0f);


            double mirror_ax = 1.7 + (0.8 / 2.0) * cos(0*M_PI/180.0f);
            double mirror_ay = 3 + (0.8 / 2.0) * sin(0*M_PI/180.0f);
            double mirror_bx = 1.7 - (0.8 / 2.0) * cos(0*M_PI/180.0f);
            double mirror_by = 3 - (0.8 / 2.0) * sin(0*M_PI/180.0f);

            double d1 = sqrt(pow(mirror_ax - leading_point_x, 2) + pow(mirror_ay - leading_point_y, 2));
            double d2 = sqrt(pow(mirror_bx - leading_point_x, 2) + pow(mirror_by - leading_point_y, 2));

            if (fabs(d1 + d2-0.8)<=0.02)
            {
                //            resetBeam();
                laser_rotation = 2.0 * 0 - laser_rotation;
              //  cout << laser_rotation << endl;
                laser_x = leading_point_x + (0.4 / 2.0) * cos((double)laser_rotation*M_PI/180.0f);
                laser_y = leading_point_y + (0.4/ 2.0) * sin((double)laser_rotation*M_PI/180.0f);
            }
        }
        if(laserFlag==1)
        {
            double leading_point_x = (double)laser_x + ((double)0.4 / 2.0) * cos((double)laser_rotation*M_PI/180.0f);
            double leading_point_y = (double)laser_y + ((double)0.4 / 2.0) * sin((double)laser_rotation*M_PI/180.0f);


            double mirror_ax = 2.7 + (0.8 / 2.0) * cos(90*M_PI/180.0f);
            double mirror_ay = mirror_y + (0.8 / 2.0) * sin(90*M_PI/180.0f);
            double mirror_bx = 2.7 - (0.8 / 2.0) * cos(90*M_PI/180.0f);
            double mirror_by = mirror_y - (0.8 / 2.0) * sin(90*M_PI/180.0f);

            double d1 = sqrt(pow(mirror_ax - leading_point_x, 2) + pow(mirror_ay - leading_point_y, 2));
            double d2 = sqrt(pow(mirror_bx - leading_point_x, 2) + pow(mirror_by - leading_point_y, 2));

            if (fabs(d1 + d2-0.8)<=0.02)
            {
                //            resetBeam();
                laser_rotation = 2.0 * 90 - laser_rotation;
         //       cout << laser_rotation << endl;
                laser_x = leading_point_x + (0.4 / 2.0) * cos((double)laser_rotation*M_PI/180.0f);
                laser_y = leading_point_y + (0.4/ 2.0) * sin((double)laser_rotation*M_PI/180.0f);
            }
        }
        for(i=0;i<block_x.size();i++)
        {
            block_y[i]-=blockSpeed;
            int greenFlag=0,redFlag=0;
            if(block_y[i]<=-3.1 && block_y[i]>=-3.9)
            {
                if(block_color[i]==1)
                {
                    if(block_x[i]<=greenx+0.8 && block_x[i]>=greenx-0.8)
                    {
                        greenFlag=1;
                        score+=20;
                        block_x.erase(block_x.begin()+i);
                        block_color.erase(block_color.begin()+i);
                        block_y.erase(block_y.begin()+i);
                    }
                    if(block_x[i]<=redx+0.8 && block_x[i]>=redx-0.8)
                    {
                        if(greenFlag==1)
                            score-=20;
                        else
                            score-=30;
                        block_x.erase(block_x.begin()+i);
                        block_color.erase(block_color.begin()+i);
                        block_y.erase(block_y.begin()+i);
                    }
                }
                else if(block_color[i]==0)
                {
                    if(block_x[i]<=redx+0.8 && block_x[i]>=redx-0.8)
                    {
                        redFlag=1;
                        score+=20;
                        block_x.erase(block_x.begin()+i);
                        block_color.erase(block_color.begin()+i);
                        block_y.erase(block_y.begin()+i);
                    }
                    else if(block_x[i]<=greenx+0.8 && block_x[i]>=greenx-0.8)
                    {
                        if(redFlag==1)
                            score-=20;
                        else
                            score=-30;
                        block_x.erase(block_x.begin()+i);
                        block_color.erase(block_color.begin()+i);
                        block_y.erase(block_y.begin()+i);
                    }
                }
                else
                {
                    if(block_x[i]<=greenx+0.8 && block_x[i]>=greenx-0.8)
                    {
                        score-=50;
                        block_x.erase(block_x.begin()+i);
                        block_color.erase(block_color.begin()+i);
                        block_y.erase(block_y.begin()+i);
                        numberOfBlack+=1;
                    }
                    else if(block_x[i]<=redx+0.8 && block_x[i]>=redx-0.8)
                    {
                        score-=50;
                        block_x.erase(block_x.begin()+i);
                        block_color.erase(block_color.begin()+i);
                        block_y.erase(block_y.begin()+i);
                        numberOfBlack+=1;
                    }
                    else
                    {
                        score-=10;
                        block_x.erase(block_x.begin()+i);
                        block_color.erase(block_color.begin()+i);
                        block_y.erase(block_y.begin()+i);
                        //numberOfBlack+=1;
                    }
                }
            }
        }


        //cout << score << endl;
        // OpenGL Draw commands
        draw();
        //draw_block();
        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 3.0)
        { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            block_x.push_back(float((float)(rand()%550-350)/(float)100));
            block_y.push_back(4.5);
            block_color.push_back(rand()%3);
            //    draw_flag = 1;
            last_update_time = current_time;
            //number_of_blocks++;
        }
        if(score< -20)
            break;
        if(laserFlag==1)
        {   //printf("%f\n", laser_rotation);
            laser_x+=cos(laser_rotation*M_PI/180)/10;
            laser_y+=sin(laser_rotation*M_PI/180)/10;
        }
        // cout << laser_rotation*M_PI << endl;
        // cout << laser_x << " " << laser_y << endl;
        //else
        //{
        //  draw_flag = 0;
        //}
    }
    if(score > 100)
        cout << "YOU WON" << endl;
    if(score < 100)
        cout << "YOU LOST" << endl;
    cout << score << endl;
    free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();
    glfwTerminate();

    return 0;
    //    exit(EXIT_SUCCESS);
}
