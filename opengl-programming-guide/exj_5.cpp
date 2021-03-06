
/*  exj_5.cpp
 * James A. Feister - thegreatpissant@gmail.com
 * DONE - Break out different model types.
 * DONE - Add a simple render system, yes it is very simple
 * DONE - Add an actor a subclass of an entity
 * PROOF - Use std library to load shaders
 * PROOF - Rendering function in renderer only
 * PROOF - Independent model movement
 * PROOF - very simple scene graph of entities to render
 * TODO - Move the shaders out of here
 * TODO - Move any other OpenGL stuff out of here.
 * Proposed exj_5 - Physics engine
 * Proposed exj_5 - Selection
 * Proposed exj_5 - Display class, Oculus and traditional
 * Proposed exj_5 - Fix input system to be more fluent
 */

#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <cstdlib>

using namespace std;

//  OpenGL
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

// 3rd Party
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform2.hpp>

//  Engine parts
#include "common/Shader.h"
#include "common/Render.h"
#include "common/Model.h"
#include "common/Display.h"
#include "common/Actor.h"
#include "common/Camera.h"
#include "common/Model_vbotorus.h"

enum class queue_events {
    STRAFE_LEFT,
    STRAFE_RIGHT,
    MOVE_FORWARD,
    MOVE_BACKWARD,
    YAW_LEFT,
    YAW_RIGHT,
    MOVE_UP,
    MOVE_DOWN,
    PITCH_UP,
    PITCH_DOWN,
    COLOR_CHANGE,
    MODEL_CHANGE,
    APPLICATION_QUIT
};

queue<queue_events> gqueue;
shared_ptr<Display> display { new Display };
shared_ptr<Renderer> renderer;
shared_ptr<Camera> camera;
shared_ptr<Entity> selected;
vector<shared_ptr<Actor>> scene_graph;

//  Constants and Vars
//  @@TODO Should move into a variable system
Shader vertex_shader(GL_VERTEX_SHADER), fragment_shader(GL_FRAGMENT_SHADER);
Shader ads_vertex_shader(GL_VERTEX_SHADER), ads_fragment_shader(GL_FRAGMENT_SHADER);
ShaderProgram diffuse_shading;
ShaderProgram ads_shading;
ShaderProgram * global_shader;
glm::mat4 MVP;
glm::mat4 camera_matrix;
glm::mat3 NormalMatrix;
glm::mat4 ModelViewMatrix;
glm::vec3 Ka = glm::vec3(0.3f, 0.5f, 0.3f);
glm::vec3 Kd = glm::vec3(0.4f, 0.1f, 0.3f);
glm::vec3 Ks = glm::vec3(0.1f, 0.4f, 0.2f);
float Shine = 0.5f;
glm::vec3 La = glm::vec3(1.0f, 1.0f, 1.0f);
glm::vec3 Ld = glm::vec3(0.3f, 0.5f, 0.1f);
glm::vec3 Ls = glm::vec3(0.7f, 0.2f, 2.8f);
glm::vec4 LightPosition;


//  Function Declarations
void Init( );
void GlutIdle( );
void GlutReshape( int newWidth, int newHeight );
void GlutDisplay( );
void GlutKeyboard( unsigned char key, int x, int y );
void CleanupAndExit( );
//  Models
void GenerateModels( );
//  Entities
void GenerateEntities( );
//  Shaders
void GenerateShaders( );

//  Globalized user vars
GLfloat strafe{ 1.0f }, height{ 0.0f }, depth{ -15.0f }, rotate{ 0.0f };

float dir = 1.0f;
float xpos = 2.0f;
float ypos = 0.0f;

// MAIN //
int main( int argc, char **argv ) {
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA );
    glutInitWindowSize( display->getWidth(), display->getHeight() );

    glutCreateWindow( argv[0] );
    if ( glewInit( ) ) {
        cerr << "Unable to initialize GLEW ... exiting " << endl;
        exit( EXIT_FAILURE );
    }

    //  Initialize common systems

    //  Camera
    camera = shared_ptr<Camera>{ new Camera( strafe, height, depth, 0.0f, 0.0f,
                                             0.0f ) };
    renderer = display->getRenderer();
    display->setCamera (camera);

    //  Load our Application Items
    GenerateModels( );
    GenerateShaders( );

    //  This scene specific items
    GenerateEntities( );

    //  Boiler Plate
    glutIdleFunc( GlutIdle );
    glutReshapeFunc( GlutReshape );
    glutDisplayFunc( GlutDisplay );
    glutKeyboardFunc( GlutKeyboard );

    //  Go forth and loop
    glutMainLoop( );
}

void GenerateShaders( ) {
    //  Shaders
    try {
        vertex_shader.SourceFile("../shaders/diffuse_shading.vert");
        fragment_shader.SourceFile("../shaders/diffuse_shading.frag");
        vertex_shader.Compile();
        fragment_shader.Compile();
        diffuse_shading.addShader(vertex_shader.GetHandle());
        diffuse_shading.addShader(fragment_shader.GetHandle());
        diffuse_shading.link();
        diffuse_shading.unuse();
    }
    catch (ShaderProgramException excp) {
        cerr << excp.what () << endl;
        exit (EXIT_FAILURE);
    }

    try {
        ads_vertex_shader.SourceFile("../shaders/ads_shading.vert");
        ads_fragment_shader.SourceFile("../shaders/ads_shading.frag");
        ads_vertex_shader.Compile();
        ads_fragment_shader.Compile();
        ads_shading.addShader(ads_vertex_shader.GetHandle());
        ads_shading.addShader(ads_fragment_shader.GetHandle());
        ads_shading.link();
        ads_shading.unuse();
        ads_shading.printActiveUniforms();
    }
    catch (ShaderProgramException excp) {
        cerr << excp.what() << endl;
        exit (EXIT_FAILURE);
    }

//    global_shader = &diffuse_shading;
    global_shader = &ads_shading;
}

void GlutReshape( int newWidth, int newHeight )
{
    display->Reshape(newWidth, newHeight);
}


void GlutDisplay( )
{
    glm::mat4 r_matrix =
            glm::rotate( glm::mat4 (), camera->getOrientation()[0], glm::vec3( 1.0f, 0.0f, 0.0f ) );
    r_matrix =
            glm::rotate( r_matrix, camera->getOrientation()[1], glm::vec3( 0.0f, 1.0f, 0.0f ) );
    r_matrix =
            glm::rotate( r_matrix, camera->getOrientation()[2], glm::vec3( 0.0f, 0.0f, 1.0f ) );
    glm::vec4 cr = r_matrix * glm::vec4( 0.0f, 0.0f, 1.0f, 1.0f );
    camera_matrix = glm::lookAt(
                camera->getPosition(),
                camera->getPosition() + glm::vec3( cr.x, cr.y, cr.z ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
    glm::mat4 model = glm::mat4(1.0f);
    model *= glm::rotate(-35.0f, glm::vec3(1.0f,0.0f,0.0f));
    model *= glm::rotate(35.0f, glm::vec3(0.0f,1.0f,0.0f));
    ModelViewMatrix = camera_matrix *  model;
    NormalMatrix = glm::mat3 (glm::vec3( ModelViewMatrix[0]), glm::vec3( ModelViewMatrix[1]), glm::vec3( ModelViewMatrix[2]));
    MVP = display->getPerspective() * ModelViewMatrix;

    //	Light Movement
    static float bounce = 0.0f;
    static bool bounce_lr = true;  //  True = left; False = right;
    const float bounce_distance = 10.0f;
    if (bounce_lr) {
        bounce -= 0.1f;
        if (bounce <  (-1.0f*bounce_distance)) {
            bounce_lr = false;
        }
    } else {
        bounce += 0.1f;
        if (bounce > bounce_distance ) {
            bounce_lr = true;
        }
    }
    LightPosition = camera_matrix * glm::vec4(bounce, 0.0f, -5.0f, 1.0f);

    //  Set values in the shader
    global_shader->use();
    global_shader->setUniform("NormalMatrix", NormalMatrix);
    //global_shader->setUniform("ProjectionMatrix", Projection);
    global_shader->setUniform("ModelViewMatrix", ModelViewMatrix);
    global_shader->setUniform("MVP", MVP );
    global_shader->setUniform("Material.Ka", Ka);
    global_shader->setUniform("Material.Kd", Kd);
    global_shader->setUniform("Material.Ks", Ks);
    global_shader->setUniform("Light.La", La);
    global_shader->setUniform("Light.Ld", Ld);
    global_shader->setUniform("Light.Ls", Ls);
    global_shader->setUniform("Light.Position", LightPosition);
    global_shader->setUniform("Material.Shininess", Shine);

    display->Render( scene_graph );
    global_shader->unuse();

    glFinish( );
    glutSwapBuffers( );
}

void GlutKeyboard( unsigned char key, int x, int y )
{
    switch ( key ) {
    default:
        break;
    case 27:
    case 'q':
    case 'Q':
        gqueue.push( queue_events::APPLICATION_QUIT );
        break;
    case 'h':
    case 'H':
        gqueue.push( queue_events::YAW_LEFT );
        break;
    case 'l':
    case 'L':
        gqueue.push( queue_events::YAW_RIGHT );
        break;
    case 'a':
    case 'A':
        gqueue.push( queue_events::STRAFE_LEFT );
        break;
    case 'd':
    case 'D':
        gqueue.push( queue_events::STRAFE_RIGHT );
        break;
    case 's':
    case 'S':
        gqueue.push( queue_events::MOVE_BACKWARD );
        break;
    case 'w':
    case 'W':
        gqueue.push( queue_events::MOVE_FORWARD );
        break;
    case 'k':
    case 'K':
        gqueue.push( queue_events::MOVE_UP );
        break;
    case '-':
        gqueue.push( queue_events::PITCH_UP );
        break;
    case '+':
        gqueue.push( queue_events::PITCH_DOWN );
        break;
    case 'j':
    case 'J':
        gqueue.push( queue_events::MOVE_DOWN );
        break;
    case 'c':
    case 'C':
        gqueue.push( queue_events::COLOR_CHANGE );
        break;
    case 'm':
    case 'M':
        gqueue.push( queue_events::MODEL_CHANGE );
        break;
    }
}

const glm::vec3 back_movement(0.0f, 0.0f, 1.0f);
const glm::vec3 forward_movement(0.0f, 0.0f, -1.0f);
const glm::vec3 left_movement( -0.3f, 0.0f, 0.0f);
const glm::vec3 right_movement( 0.3f, 0.0f, 0.0f);
const glm::vec3 up_movement(0.0f, 1.0f, 0.0f);
const glm::vec3 down_movement(0.0f, -1.0f, 0.0f);

void GlutIdle( )
{
    //  Pump the events loop
    while ( !gqueue.empty( ) ) {
        switch ( gqueue.front( ) ) {
        case queue_events::MOVE_FORWARD:
            selected->move (forward_movement);
            break;
        case queue_events::MOVE_BACKWARD:
            selected->move (back_movement);
            break;
        case queue_events::STRAFE_RIGHT:
            selected->move (right_movement);
            break;
        case queue_events::STRAFE_LEFT:
            selected->move(left_movement);
            break;
        case queue_events::YAW_RIGHT:
            selected->orient(up_movement);
            break;
        case queue_events::YAW_LEFT:
            selected->orient(down_movement);
            break;
        case queue_events::MOVE_UP:
            selected->move(up_movement);
            break;
        case queue_events::MOVE_DOWN:
            selected->move(down_movement);
            break;
        case queue_events::PITCH_UP:
            selected->orient(right_movement);
            break;
        case queue_events::PITCH_DOWN:
            selected->orient(left_movement);
            break;
        case queue_events::COLOR_CHANGE:
//            color = ( color >= 4 ? 1 : color + 1 );
            break;
        case queue_events::MODEL_CHANGE:
            break;
        case queue_events::APPLICATION_QUIT:
            CleanupAndExit( );
        }
        gqueue.pop( );
    }

    glutPostRedisplay( );
}

void CleanupAndExit( )
{
    exit( EXIT_SUCCESS );
}

void GenerateModels( ) {
    int ext = 0;
    shared_ptr<Simple_equation_model_t> tmp;
    shared_ptr<VBOTorus> tmpt;

    //  Generate Torus
    tmpt = shared_ptr<VBOTorus> { new VBOTorus (0.7f, 0.3f, 50, 50) };
    tmpt->name = "vbo_torus";
    renderer->add_model( tmpt );

    //  Generate Some equation model
    for ( auto power_to :
    { 1.0f, 1.2f, 1.4f, 1.6f, 1.8f, 2.1f, 2.2f, 2.3f, 3.5f, 4.0f } ) {
        tmp =
                shared_ptr<Simple_equation_model_t>{ new Simple_equation_model_t };
        float x = 0.0f;
        float z = 0.0f;
        tmp->numVertices = 600;
        tmp->vertices.resize( tmp->numVertices * 3 );
        for ( int i = 0; i < tmp->numVertices; i++, x += 0.1f, z += 0.05f ) {
            tmp->vertices[i * 3] = x;
            tmp->vertices[i * 3 + 1] = powf( x, power_to );
            tmp->vertices[i * 3 + 2] = 0.0f; // z;
            if ( z >= -1.0f )
                z = 0.0f;
        }
        tmp->name = "ex15_" + to_string( ext++ );
        tmp->renderPrimitive = GL_POINTS;
        tmp->setup_render_model( );
        renderer->add_model( tmp );
    }

}

void GenerateEntities( ) {
   //  Actors
    GLfloat a = 0.0f;
    for ( int i = 0; i < 1; i++, a += 10.0f ) {
        scene_graph.push_back( shared_ptr<Actor>{ new Actor(
                                                  a, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, i ) } );
    }
    //  Selected Entity
    selected = camera;
}
