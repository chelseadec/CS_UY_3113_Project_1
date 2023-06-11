/**
* Author: Chelsea DeCambre
* Assignment: Simple 2D Scene
* Date due: 2023-06-11, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const char PLAYER_SPRITE_FILEPATH[] = "final.png";
const char PLAYER_SPRITE_FILEPATH2[] = "haunter.png";
const char PLAYER_SPRITE_FILEPATH3[] = "gengar.png";

GLuint player_texture_id, player_texture_id2, player_texture_id3;


enum Coordinate
{
    x_coordinate,
    y_coordinate
};

#define LOG(argument) std::cout << argument << '\n'

const int WINDOW_WIDTH = 640,
          WINDOW_HEIGHT = 480;

const float BG_RED = 0.247f,
            BG_BLUE = 0.231f,
            BG_GREEN = 0.256f,
            BG_OPACITY = 0.5f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const float MILLISECONDS_IN_SECOND = 1000.0;
const float DEGREES_PER_SECOND = 90.0f;

const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero


SDL_Window* display_window;
bool game_is_running = true;
bool is_growing = true;

ShaderProgram program;
glm::mat4 view_matrix, model_matrix, projection_matrix, trans_matrix;
glm::mat4 view_matrix2, model_matrix2, projection_matrix2, trans_matrix2;
glm::mat4 view_matrix3, model_matrix3, projection_matrix3, trans_matrix3;

float previous_ticks = 0.0f;

SDL_Joystick *player_one_controller;

// overall position
glm::vec3 player_position = glm::vec3(0.0f, 0.0f, 0.0f);

// movement tracker
glm::vec3 player_movement = glm::vec3(0.0f, 0.0f, 0.0f);

float get_screen_to_ortho(float coordinate, Coordinate axis)
{
    switch (axis) {
        case x_coordinate:
            return ((coordinate / WINDOW_WIDTH) * 10.0f ) - (10.0f / 2.0f);
        case y_coordinate:
            return (((WINDOW_HEIGHT - coordinate) / WINDOW_HEIGHT) * 7.5f) - (7.5f / 2.0);
        default:
            return 0.0f;
    }
}

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
//    if (image == NULL)
//    {
//        LOG("Unable to load image. Make sure the path is correct.");
//        assert(false);
//    }
    
    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);
    
    return textureID;
}

const float TRIANGLE_INIT_ANGLE = glm::radians(45.0);

void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    
    // Open the first controller found. Returns null on error
    player_one_controller = SDL_JoystickOpen(0);
    
    display_window = SDL_CreateWindow("Project 1! ",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(display_window);
    SDL_GL_MakeCurrent(display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    program.Load(V_SHADER_PATH, F_SHADER_PATH);
    
    player_texture_id = load_texture(PLAYER_SPRITE_FILEPATH);
    player_texture_id2 = load_texture(PLAYER_SPRITE_FILEPATH2);
    player_texture_id3 = load_texture(PLAYER_SPRITE_FILEPATH3);
    
    model_matrix = glm::mat4(1.0f); // gastly
    model_matrix2 = glm::mat4(1.0f); // haunter
    
    model_matrix3 = glm::mat4(1.0f); // gengar
    
    view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.
    trans_matrix = model_matrix;
    
    program.SetProjectionMatrix(projection_matrix);
    program.SetViewMatrix(view_matrix);
    // Notice we haven't set our model matrix yet!
    
    glUseProgram(program.programID);
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    player_movement = glm::vec3(0.0f);
    
    SDL_Event event;
    
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_WINDOWEVENT_CLOSE:
            case SDL_QUIT:
                game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT:
                        player_movement.x = 1.0f;
                        break;
                    case SDLK_LEFT:
                        player_movement.x = -1.0f;
                        break;
                    case SDLK_q:
                        game_is_running = false;
                        break;
                    default:
                        break;
                }
            default:
                break;
        }
    }
    
    const Uint8 *key_states = SDL_GetKeyboardState(NULL); // array of key states [0, 0, 1, 0, 0, ...]
    
    if (key_states[SDL_SCANCODE_LEFT])
    {
        player_movement.x = -1.0f;
    } else if (key_states[SDL_SCANCODE_RIGHT])
    {
        player_movement.x = 1.0f;
    }
    
    if (key_states[SDL_SCANCODE_UP])
    {
        player_movement.y = 1.0f;
    } else if (key_states[SDL_SCANCODE_DOWN])
    {
        player_movement.y = -1.0f;
    }
    
    if (glm::length(player_movement) > 1.0f)
    {
        player_movement = glm::normalize(player_movement);
    }
}
float TRAN_VALUE = 0.010f;
const float GROWTH_FACTOR = 1.10f;  // growth rate of 10.0% per frame
const float SHRINK_FACTOR = 0.90f;  // growth rate of -10.0% per frame
const float ROT_ANGLE = glm::radians(0.5f);
const int MAX_FRAME = 200;           // this value is, of course, up to you

int g_frame_counter = 0;
bool g_is_growing = true;
bool reverse = false;

void update()
{
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - previous_ticks; // the delta time is the difference from the last frame
    previous_ticks = ticks;


    // Add             direction       * elapsed time * units per second
    player_position += player_movement * delta_time * 1.0f;
    
    
    model_matrix = glm::mat4(1.0f);
    
    // STEP 1
    glm::vec3 scale_vector;
    g_frame_counter += 1;
    
    
//    model_matrix = glm::translate(model_matrix, glm::vec3(TRAN_VALUE, 0.0f, 0.0f));
    model_matrix2 = glm::rotate(model_matrix2, ROT_ANGLE, glm::vec3(0.0f, 0.0f, 1.0f));
//    model_matrix3 = glm::translate(model_matrix3, glm::vec3(TRAN_VALUE, 0.0f, 0.0f));
    // STEP 2
    if (g_frame_counter >= MAX_FRAME)
    {
        reverse = !reverse;
        
        g_is_growing = !g_is_growing;
        g_frame_counter = 0;
    }
    
    
    model_matrix2 = glm::translate(model_matrix2, glm::vec3(reverse ? -(TRAN_VALUE) : TRAN_VALUE, 0.0f, 0.0f)); // makes haunter travel in a cresent-like fashion.
    
    // STEP 3
    scale_vector = glm::vec3(g_is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
                             g_is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
                             1.0f);
    
    // STEP 4 ; Transformations to gastly
    model_matrix = glm::scale(model_matrix, scale_vector);
    model_matrix = glm::translate(model_matrix, glm::vec3(TRAN_VALUE, 0.0f, 0.0f));
    
    
    // Transformations to gengar
    
//    model_matrix2 = glm::scale(model_matrix2, (scale_vector *= .80));
    model_matrix3 = glm::rotate(model_matrix3, ROT_ANGLE, glm::vec3(0.0f, 0.0f, 1.0f));
   
}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    program.SetModelMatrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    program.SetModelMatrix(model_matrix);
    
    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
       };

    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    
    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };
    
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(program.texCoordAttribute);
    
    // Bind texture
    glBindTexture(GL_TEXTURE_2D, player_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // We disable two attribute arrays now
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    
    draw_object(model_matrix, player_texture_id);
    
    // Second Object
    
    program.SetModelMatrix(model_matrix2);
    
    // Vertices
    float vertices2[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
       };

    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
    glEnableVertexAttribArray(program.positionAttribute);
    
    // Textures
    float texture_coordinates2[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };
    
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates2);
    glEnableVertexAttribArray(program.texCoordAttribute);
    
    // Bind texture
    glBindTexture(GL_TEXTURE_2D, player_texture_id2);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // We disable two attribute arrays now
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    
    draw_object(model_matrix2, player_texture_id2);
    
    // Third Object
    
    program.SetModelMatrix(model_matrix3);
    
    // Vertices
    float vertices3[] = {
        -1.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,  // triangle 1
        -1.5f, -0.5f, -0.5f, 0.5f, -1.5f, 0.5f   // triangle 2
    };

    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices3);
    glEnableVertexAttribArray(program.positionAttribute);
    
    // Textures
    float texture_coordinates3[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };
    
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates3);
    glEnableVertexAttribArray(program.texCoordAttribute);
    
    // Bind texture
    glBindTexture(GL_TEXTURE_2D, player_texture_id3);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // We disable two attribute arrays now
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    
    draw_object(model_matrix3, player_texture_id3);
    
    SDL_GL_SwapWindow(display_window);
    
}

void shutdown()
{
    SDL_JoystickClose(player_one_controller);
    SDL_Quit();
}

/**
 Start here—we can see the general structure of a game loop without worrying too much about the details yet.
 */
int main(int argc, char* argv[])
{
    initialise();
    
    while (game_is_running)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
