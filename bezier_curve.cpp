// command to compile on my environment (linux mint):
// g++ -c bezier_curve.cpp
// g++ -c Points.cpp

// command to link:
// g++ bezier_curve.o -o bezier_curve.exec -lGL -lGLU -lglfw3 -lX11 -lXxf86vm -lXrandr -lpthread -lXi -ldl

// execute:
// ./bezier_curve.exec

#include "./Points.cpp"
#include "./glad.h"
#include "./Shader_s.h"
#include "./stb_image.h"
#include "./stb_image.cpp"
#include "./glad.c"
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

using namespace std;

const char WINDOW_NAME[] = "beziér";
const char VERTEX_SHADER_NAME[] = "bezier_shader.vertex";
const char FRAGMENT_SHADER_NAME[] = "bezier_shader.fragment";
const char AMBIENT_OCCLUSION_TEX_PATH[] = "./textures/lava/ambientocclusion.png";
const char BASE_COLOR_TEX_PATH[] = "./textures/lava/basecolor.png";
const char EMISSIVE_TEX_PATH[] = "./textures/lava/emissive.png";
const char HEIGHT_TEX_PATH[] = "./textures/lava/height.png";
const char METALLIC_TEX_PATH[] = "./textures/lava/metallic.png";
const char NORMAL_TEX_PATH[] = "./textures/lava/normal.png";
const char ROUGHNESS_TEX_PATH[] = "./textures/lava/roughness.png";
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

#define MARKER_RADIUS 8
#define NI 4
#define NJ 5
#define RES_I NI * 10
#define RES_J NJ * 10

float CP[NI][NJ][3];
float outp[RES_I][RES_J][3];

bool mouse_l_down = false;
int selected = -1;
bool already_added = false;
bool draw_bezier_surface = false;

GLFWwindow *window;
float vertices[INFO_PER_POINT * MAX_NO_POINTS] = {};
unsigned int VBO, VAO;

//----------------------- FUNCTION DECLARATIONS -----------------------//

int setupGlfwAndGlad();
void setupGL();
unsigned int load_texture(const char *path);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, int button, int action, int mods);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow *window);
void handleMouseDown();
glm::vec3 convert_mouse_coord_to_world(float x, float y);
void quad(unsigned int &VBO, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);
float blend(int k, float mu, int n);
void bezier_surface(unsigned int &VBO, int NUMI, int NUMJ);
void generate_points(unsigned int &VBO, int NUMI, int NUMJ);

//
//
//----------------------- MAIN -----------------------//
//
//

int main()
{

    if (setupGlfwAndGlad() == -1)
    {
        return -1;
    }

    setupGL();

    Shader lava_shader(VERTEX_SHADER_NAME, FRAGMENT_SHADER_NAME);
    unsigned int baseMap = load_texture(BASE_COLOR_TEX_PATH);
    unsigned int emissionMap = load_texture(EMISSIVE_TEX_PATH);
    lava_shader.use();
    lava_shader.setInt("material.base", 0);
    lava_shader.setInt("material.emission", 1);

    generate_points(VBO, NI, NJ);

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (mouse_l_down)
        {
            handleMouseDown();
        }

        glActiveTexture(GL_TEXTURE0);
        lava_shader.use();
        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, baseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, emissionMap);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
        // view = glm::translate(view, glm::vec3(0.5f, 0.5f, 0.5f));
        // view = glm::rotate(view, (float)glfwGetTime() * glm::radians(20.0f), glm::vec3(1.0f, 1.0f, 0.1f));
        // projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        // model = glm::translate(model, cubePositions[i]);

        lava_shader.setMat4("model", model);
        lava_shader.setMat4("view", view);
        lava_shader.setMat4("projection", projection);

        glPointSize(8);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawArrays(GL_POINTS, 0, (NI + 1) * (NJ + 1));
        glDrawArrays(GL_TRIANGLES, (NI + 1) * (NJ + 1), points.num_points - ((NI + 1) * (NJ + 1)));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();
    return 0;
}

//
//
//----------------------- GLFW & GLAD SETUP -----------------------//
//
//

int setupGlfwAndGlad()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, WINDOW_NAME, NULL, NULL);

    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    return 0;
}

//
//
//----------------------- GL SETUP-----------------------//
//
//

void setupGL()
{
    glEnable(GL_DEPTH_TEST);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, INFO_PER_POINT * points.primitive_size, (void *)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, INFO_PER_POINT * points.primitive_size, (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coordinates
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, INFO_PER_POINT * points.primitive_size, (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

//
//
//----------------------- LOAD TEXTURE -----------------------//
//
//

unsigned int load_texture(char const *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

//
//
//----------------------- WINDOW AND INPUT RELATED FUNCTIONS -----------------------//
//
//

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, int button, int action, int mods)
{

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (GLFW_PRESS == action)
        {
            mouse_l_down = true;
        }
        else if (GLFW_RELEASE == action)
        {
            selected = -1;
            already_added = false;
            mouse_l_down = false;
            draw_bezier_surface = true;
        }
    }

    if (mouse_l_down && selected == -1)
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        for (int i = 0; i < points.num_points; i++)
        {
            Points::Point point = points.points[i];
            float distance = glm::distance(convert_mouse_coord_to_world(float(x), float(y)), glm::vec3(point.position)) * 100;
            if (distance < MARKER_RADIUS)
            {
                selected = point.index;
                break;
            }
        }
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(key == GLFW_KEY_LEFT){
        cout << "rotate me" << endl;
    }
    if(key == GLFW_KEY_RIGHT){
        cout << "em etator" << endl;
    }
    if(key == GLFW_KEY_UP){
        cout << "r\no\nt\na\nt\ne\n \nm\ne" << endl;
    }
    if(key == GLFW_KEY_DOWN){
        cout << "\ne\nm\n \ne\nt\na\nt\no\nr" << endl;
    }

}


void handleMouseDown()
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    glm::vec3 pos = convert_mouse_coord_to_world(float(x), float(y));
    if (!already_added && selected == -1)
    {
        points.add_point(VBO, Points::Point(glm::vec3(pos.x, pos.y, pos.z)));
        already_added = true;
    }
    else if (mouse_l_down && selected != -1 && selected < (NI + 1) * (NJ + 1))
    {
        Points::Point selected_p = points.points[selected];
        glm::vec3 old_position = selected_p.position;
        glm::vec3 new_position = convert_mouse_coord_to_world(x, y);
        glm::vec3 offset = new_position - old_position;
        points.modify_point_position_in_buffer(VBO, selected, new_position);
        int selected_i = selected_p.i_in_CP_array;
        int selected_j = selected_p.j_in_CP_array;
        CP[selected_i][selected_j][0] = new_position.x;
        CP[selected_i][selected_j][1] = new_position.y;
        CP[selected_i][selected_j][2] = new_position.z;
        points.num_points = (NI + 1) * (NJ + 1);
        bezier_surface(VBO, NI, NJ);
    }
}

glm::vec3 convert_mouse_coord_to_world(float x, float y)
{
    return glm::vec3(2 * x / SCR_WIDTH - 1, 1 - 2 * y / SCR_HEIGHT, 0);
}

//
//
//----------------------- BEZIER SURFACE GENERATING FUNCTIONS -----------------------//
//
//

void quad(unsigned int &VBO, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d)
{
    glm::vec3 u = b - a;
    glm::vec3 v = c - b;

    glm::vec3 normal = glm::normalize(glm::cross(u, v));

    points.add_point(VBO, Points::Point(a, normal, glm::vec2(-1.0, -1.0)));
    points.add_point(VBO, Points::Point(b, normal, glm::vec2(1.0, -1.0)));
    points.add_point(VBO, Points::Point(c, normal, glm::vec2(1.0, 1.0)));
    points.add_point(VBO, Points::Point(a, normal, glm::vec2(-1.0, -1.0)));
    points.add_point(VBO, Points::Point(c, normal, glm::vec2(1.0, 1.0)));
    points.add_point(VBO, Points::Point(d, normal, glm::vec2(-1.0, 1.0)));
}

float blend(int k, float mu, int n)
{
    int nn, kn, nkn;
    float blend = 1;

    nn = n;
    kn = k;
    nkn = n - k;

    while (nn >= 1)
    {
        blend *= float(nn);
        nn--;
        if (kn > 1)
        {
            blend /= float(kn);
            kn--;
        }
        if (nkn > 1)
        {
            blend /= float(nkn);
            nkn--;
        }
    }
    if (k > 0)
    {
        blend *= float(pow(mu, k));
    }
    if (n - k > 0)
    {
        blend *= float(pow(1 - mu, n - k));
    }
    return blend;
}

void bezier_surface(unsigned int &VBO, int NUMI, int NUMJ)
{
    int i, j, ki, kj;
    float mui, muj, bi, bj;

    for (i = 0; i < RES_I; i++)
    {
        mui = float(i) / (RES_I - 1);
        for (j = 0; j < RES_J; j++)
        {
            muj = float(j) / (RES_J - 1);

            outp[i][j][0] = 0;
            outp[i][j][1] = 0;
            outp[i][j][2] = 0;
            for (ki = 0; ki <= NUMI; ki++)
            {
                bi = blend(ki, mui, NUMI);
                for (kj = 0; kj <= NUMJ; kj++)
                {
                    bj = blend(kj, muj, NUMJ);
                    outp[i][j][0] += (CP[ki][kj][0] * bi * bj);
                    outp[i][j][1] += (CP[ki][kj][1] * bi * bj);
                    outp[i][j][2] += (CP[ki][kj][2] * bi * bj);
                }
            }
        }
    }

    for (i = 0; i < RES_I - 1; i++)
    {
        for (j = 0; j < RES_J - 1; j++)
        {
            glm::vec3 a = glm::vec3(outp[i][j][0] / NUMI - 0.5f, outp[i][j][1] / NUMJ - 0.5f, outp[i][j][2]);
            glm::vec3 b = glm::vec3(outp[i][j + 1][0] / NUMI - 0.5f, outp[i][j + 1][1] / NUMJ - 0.5f, outp[i][j + 1][2]);
            glm::vec3 c = glm::vec3(outp[i + 1][j][0] / NUMI - 0.5f, outp[i + 1][j][1] / NUMJ - 0.5f, outp[i + 1][j][2]);
            glm::vec3 d = glm::vec3(outp[i + 1][j + 1][0] / NUMI - 0.5f, outp[i + 1][j + 1][1] / NUMJ - 0.5f, outp[i + 1][j + 1][2]);
            quad(VBO, a, c, d, b);
        }
    }
}

void generate_points(unsigned int &VBO, int NUMI, int NUMJ)
{
    int i, j, ki, kj;
    float mui, muj, bi, bj;
    srand(time(0));
    for (i = 0; i <= NUMI; i++)
    {
        for (j = 0; j <= NUMJ; j++)
        {
            CP[i][j][0] = i;
            CP[i][j][1] = j;
            CP[i][j][2] = 0.0f;
        }
    }
    for (i = 0; i <= NUMI; i++)
    {
        for (j = 0; j <= NUMJ; j++)
        {
            points.add_point(VBO, Points::Point(glm::vec3(CP[i][j][0] / NUMI - 0.5f, CP[i][j][1] / NUMJ - 0.5f, CP[i][j][2]), true, i, j));
        }
    }
    bezier_surface(VBO, NUMI, NUMJ);
}
