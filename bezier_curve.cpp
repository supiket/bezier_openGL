// command to compile on my environment (linux mint):
// g++ -c bezier_curve.cpp

// command to link:
// g++ bezier_curve.o -o bezier_curve.exec -lGL -lGLU -lglfw3 -lX11 -lXxf86vm -lXrandr -lpthread -lXi -ldl

// execute:
// ./bezier_curve.exec

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

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

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

#define INFO_PER_POINT 8
#define MAX_NO_POINTS 100000
#define MARKER_RADIUS 8

struct Points
{
    struct Point
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 tex_coord;
        static Points *points;
        int index;
        bool is_CP;
        int i_in_CP_array, j_in_CP_array;

        Point()
        {
            this->points->num_points = 0;
        }

        Point(glm::vec3 pos, bool is_CP = false, int i_in_CP_array = -1, int j_in_CP_array = -1)
        {
            this->position = pos;
            this->normal = glm::vec3(0, 0, 0);
            this->tex_coord = glm::vec2(0, 0);
            this->is_CP = is_CP;
            if (is_CP)
            {
                this->i_in_CP_array = i_in_CP_array;
                this->j_in_CP_array = j_in_CP_array;
            }
        }

        Point(glm::vec3 pos, glm::vec3 normal, glm::vec2 tex, bool is_CP = false) : position(pos), normal(normal), tex_coord(tex), is_CP(is_CP), i_in_CP_array(-1), j_in_CP_array(-1) {}

        float *get_properties_as_array()
        {
            float *properties_array = new float[INFO_PER_POINT];
            properties_array[0] = this->position[0];
            properties_array[1] = this->position[1];
            properties_array[2] = this->position[2];
            properties_array[3] = this->normal[0];
            properties_array[4] = this->normal[1];
            properties_array[5] = this->normal[2];
            properties_array[6] = this->tex_coord[0];
            properties_array[7] = this->tex_coord[1];

            return properties_array;
        }
    };

    static Point *points;
    static int num_points;
    static vector<int> info_length_per_point;
    float primitive_size;

    Points() : primitive_size(sizeof(float)) {}

    void add_vertices_to_buffer(unsigned int &VBO, float offset, float size, float *new_vertices)
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, new_vertices);
    }

    void modify_point_position_in_buffer(unsigned int &VBO, float offset, float size, float *new_pos)
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, new_pos);
    }

    void add_point(unsigned int &VBO, Point point)
    {
        point.index = this->num_points;
        this->points[this->num_points++] = point;
        this->write_point_to_buffer(VBO, point);
    }

    float *get_all_points_properties_as_array()
    {
        float *properties_array = new float[INFO_PER_POINT * this->num_points];

        for (int i = 0; i < this->num_points; i++)
        {
            Point curr_point = this->points[i];
            float *point_properties = curr_point.get_properties_as_array();

            for (int j = 0; j < INFO_PER_POINT; j++)
            {
                properties_array[i * INFO_PER_POINT + j] = point_properties[j];
            }
        }
        return properties_array;
    }

    void write_all_points_to_buffer(unsigned int &VBO)
    {
        for (int i = 0; i < this->num_points; i++)
        {
            this->write_point_to_buffer(VBO, this->points[i]);
        }
    }

    void write_point_to_buffer(unsigned int &VBO, Point point)
    {
        float *properties_array = new float[INFO_PER_POINT];
        float *point_properties = point.get_properties_as_array();

        for (int i = 0; i < INFO_PER_POINT; i++)
        {
            properties_array[i] = point_properties[i];
        }
        float offset = (this->num_points - 1) * this->primitive_size * INFO_PER_POINT;
        float size = this->primitive_size * INFO_PER_POINT;
        this->add_vertices_to_buffer(VBO, offset, size, properties_array);
    }

    void modify_point_position_in_buffer(unsigned int &VBO, int point_index, glm::vec3 new_position)
    {
        this->points[point_index].position = new_position;
        float offset = point_index * INFO_PER_POINT * this->primitive_size;
        float new_pos[3] = {new_position.x, new_position.y, new_position.z};
        float size = sizeof(new_pos);
        this->modify_point_position_in_buffer(VBO, offset, size, new_pos);
    }

} points;

Points::Point *Points::points = new Points::Point[MAX_NO_POINTS];
int Points::num_points = 0;
vector<int> Points::info_length_per_point = {3, 3, 2};

#define NI 4
#define NJ 5
#define RES_I NI * 10
#define RES_J NJ * 10

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, int button, int action, int mods);
void processInput(GLFWwindow *window);
unsigned int load_texture(const char *path);
Points::Point cubic(Points::Point p1, Points::Point p2, Points::Point p3, Points::Point p4, float t);
glm::vec3 quadratic(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t);
glm::vec2 quadratic(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, float t);
Points::Point quadratic(Points::Point p1, Points::Point p2, Points::Point p3, float t);
glm::vec3 lerp(glm::vec3 p1, glm::vec3 p2, float t);
glm::vec2 lerp(glm::vec2 p1, glm::vec2 p2, float t);
Points::Point lerp(Points::Point p1, Points::Point p2, float t);
glm::vec3 convert_mouse_coord_to_world(float x, float y);
void quad(unsigned int &VBO, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);
float blend(int k, float mu, int n);
void bezier_surface(unsigned int &VBO, int NUMI, int NUMJ);
void generate_points(unsigned int &VBO, int NUMI, int NUMJ);

bool mouse_l_down = false;
int selected = -1;

float CP[NI][NJ][3];
float outp[RES_I][RES_J][3];

bool already_added = false;
bool draw_bezier_surface = false;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, WINDOW_NAME, NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader lava_shader(VERTEX_SHADER_NAME, FRAGMENT_SHADER_NAME);

    float vertices[INFO_PER_POINT * MAX_NO_POINTS] = {};

    unsigned int VBO, VAO;
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
    // texture coord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, INFO_PER_POINT * points.primitive_size, (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

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
                // int selected_index = points.num_points - (selected - points.num_points - (NI + 1) * (NJ + 1));
                // int selected_i = floor(selected_index / NI);
                // int selected_j = selected_index % NJ;
                // CP[selected_i][selected_j][0] = new_position.x;
                // CP[selected_i][selected_j][1] = new_position.y;
                // CP[selected_i][selected_j][2] = new_position.z;
                // glClearBuffe
                // bezier_surface(VBO, NI, NJ);
            }
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
        // glDrawArrays(GL_POINTS, 0, points.num_points);
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

Points::Point cubic(Points::Point p1, Points::Point p2, Points::Point p3, Points::Point p4, float t)
{
    return Points::Point(lerp(quadratic(p1, p2, p3, t), quadratic(p2, p3, p4, t), t));
}

Points::Point quadratic(Points::Point p1, Points::Point p2, Points::Point p3, float t)
{
    return Points::Point(lerp(lerp(p1, p2, t), lerp(p2, p3, t), t));
}

Points::Point lerp(Points::Point p1, Points::Point p2, float t)
{
    return Points::Point(lerp(p1.position, p2.position, t), lerp(p1.normal, p2.normal, t), lerp(p1.tex_coord, p2.tex_coord, t));
}

glm::vec3 quadratic(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, float t)
{
    return glm::vec3(lerp(lerp(v1, v2, t), lerp(v2, v3, t), t));
}

glm::vec2 quadratic(glm::vec2 v1, glm::vec2 v2, glm::vec2 v3, float t)
{
    return glm::vec2(lerp(lerp(v1, v2, t), lerp(v2, v3, t), t));
}

glm::vec3 lerp(glm::vec3 v1, glm::vec3 v2, float t)
{
    return glm::vec3(v1.x + t * (v2.x - v1.x), v1.y + t * (v2.y - v1.y), v1.z + t * (v2.z - v1.z));
}

glm::vec2 lerp(glm::vec2 v1, glm::vec2 v2, float t)
{
    return glm::vec2(v1.x + t * (v2.x - v1.x), v1.y + t * (v2.y - v1.y));
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

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

glm::vec3 convert_mouse_coord_to_world(float x, float y)
{
    return glm::vec3(2 * x / SCR_WIDTH - 1, 1 - 2 * y / SCR_HEIGHT, 0);
}

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
