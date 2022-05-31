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

#define INFO_PER_POINT 5
#define MAX_NO_POINTS 100
#define MAX_NO_PATCHES 15
#define MARKER_RADIUS 15

struct Vertex
{
    unsigned int vertices_length;
    unsigned int number_of_points;
    float primitive_size;

    Vertex() : vertices_length(0), number_of_points(0), primitive_size(sizeof(float)) {}

    void add_vertices_update_buffer(unsigned int &VBO, int num_new_vertices, float *new_vertices)
    {
        unsigned int num_new_points = num_new_vertices / INFO_PER_POINT;

        glad_glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, vertices_length * primitive_size, num_new_vertices * primitive_size, new_vertices);

        this->vertices_length += num_new_vertices;
        this->number_of_points += num_new_points;
    }

    void modify_point_position_in_buffer(unsigned int &VBO, float offset, float size, float *new_pos)
    {
        glad_glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, new_pos);
    }
};

struct Points
{
    struct Point
    {
        glm::vec3 position;
        glm::vec2 tex_coord;
        static Points *points;
        int index;

        Point()
        {
            this->points->num_points = 0;
        }

        Point(glm::vec3 pos, glm::vec2 tex) : position(pos), tex_coord(tex) {}

        float *get_properties_as_array()
        {
            float *properties_array = new float[this->points->num_info_per_point];
            int property_id = 0;
            for (int i = 0; i < this->points->info_length_per_point[property_id]; i++)
            {
                properties_array[i] = position[i];
            }
            property_id++;
            for (int i = 0; i < this->points->info_length_per_point[property_id]; i++)
            {
                properties_array[i + this->points->info_length_per_point[property_id - 1]] = tex_coord[i];
            }
            property_id++;
            return properties_array;
        }
    };

    static Point *points;
    static int num_points;
    static const int num_info_per_point;
    static vector<int> info_length_per_point;
    static Vertex *vertex;

    Points() {}

    void add_point(Point point)
    {
        point.index = num_points;
        this->points[num_points++] = point;
    }

    float *get_all_points_properties_as_array()
    {
        float *properties_array = new float[this->num_info_per_point * this->num_points];

        for (int i = 0; i < this->num_points; i++)
        {
            Point curr_point = this->points[i];
            float *point_properties = curr_point.get_properties_as_array();

            for (int j = 0; j < this->num_info_per_point; j++)
            {
                properties_array[i * this->num_info_per_point + j] = point_properties[j];
            }
        }
        return properties_array;
    }

    void write_all_points_to_buffer(unsigned int &VBO)
    {
        float *properties_array = new float[this->num_info_per_point * this->num_points];
        for (int i = 0; i < this->num_points; i++)
        {
            Point curr_point = this->points[i];
            float *point_properties = curr_point.get_properties_as_array();

            for (int j = 0; j < this->num_info_per_point; j++)
            {
                properties_array[i * this->num_info_per_point + j] = point_properties[j];
            }
        }
        this->vertex->add_vertices_update_buffer(VBO, this->num_points * INFO_PER_POINT, properties_array);
    }

    void write_point_to_buffer(unsigned int &VBO, Point point)
    {
        float *properties_array = new float[this->num_info_per_point];
        float *point_properties = point.get_properties_as_array();

        for (int i = 0; i < this->num_info_per_point; i++)
        {
            properties_array[i] = point_properties[i];
        }

        this->vertex->add_vertices_update_buffer(VBO, INFO_PER_POINT, properties_array);
    }

    void modity_point_position_in_buffer(unsigned int &VBO, int point_index, glm::vec3 new_position)
    {
        this->points[point_index].position = new_position;
        float offset = point_index * INFO_PER_POINT * this->vertex->primitive_size;
        float new_pos[3] = {new_position.x, new_position.y, new_position.z};
        float size = sizeof(new_pos);
        this->vertex->modify_point_position_in_buffer(VBO, offset, size, new_pos);
    }

} points;

Points::Point *Points::points = new Points::Point[MAX_NO_POINTS];
int Points::num_points = 0;
const int Points::num_info_per_point = 5;
vector<int> Points::info_length_per_point = {3, 2};
Vertex *Points::vertex = new Vertex;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, int button, int action, int mods);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);
Points::Point cubic(Points::Point p1, Points::Point p2, Points::Point p3, Points::Point p4, float t);
glm::vec3 quadratic(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t);
glm::vec2 quadratic(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, float t);
Points::Point quadratic(Points::Point p1, Points::Point p2, Points::Point p3, float t);
glm::vec3 lerp(glm::vec3 p1, glm::vec3 p2, float t);
glm::vec2 lerp(glm::vec2 p1, glm::vec2 p2, float t);
Points::Point lerp(Points::Point p1, Points::Point p2, float t);
glm::vec3 convert_mouse_coord_to_world(double x, double y);

bool mouse_l_down = false;
int selected = -1;

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
    unsigned int indices[1 * MAX_NO_PATCHES] = {};

    Vertex vertex = Vertex();

    points.vertex = &vertex;

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, INFO_PER_POINT * vertex.primitive_size, (void *)0);
    glEnableVertexAttribArray(0);
    // texture coord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, INFO_PER_POINT * vertex.primitive_size, (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int baseMap = loadTexture(BASE_COLOR_TEX_PATH);
    unsigned int emissionMap = loadTexture(EMISSIVE_TEX_PATH);

    lava_shader.use();
    lava_shader.setInt("material.base", 0);
    lava_shader.setInt("material.emission", 1);

    // create a function to add these points by starting from a coordinate and adding to it randomly
    points.add_point(Points::Point(glm::vec3(-0.6f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)));
    points.add_point(Points::Point(glm::vec3(-0.4f, 0.4f, 0.0f), glm::vec2(0.0f, 1.0f)));
    points.add_point(Points::Point(glm::vec3(-0.1f, 0.6f, 0.0f), glm::vec2(0.0f, 0.0f)));
    points.add_point(Points::Point(glm::vec3(0.1f, 0.2f, 0.0f), glm::vec2(1.0f, 0.0f)));
    points.add_point(Points::Point(glm::vec3(0.3f, 0.1f, 0.0f), glm::vec2(1.0f, 0.0f)));
    points.add_point(Points::Point(glm::vec3(0.5f, -0.3f, 0.0f), glm::vec2(1.0f, 0.0f)));
    points.add_point(Points::Point(glm::vec3(0.7f, 0.5f, 0.0f), glm::vec2(1.0f, 0.0f)));

    points.write_all_points_to_buffer(VBO);

    int patch = 1;
    for (int p = 0; p < patch; p++)
    {
        for (float t = 0; t <= 1.1f; t += 0.05f)
        {
            Points::Point p1 = points.points[p];
            Points::Point p2 = points.points[p + 1];
            Points::Point p3 = points.points[p + 2];
            Points::Point p4 = points.points[p + 3];

            Points::Point lerp_point_1 = cubic(p1, p2, p3, p4, t);
            points.add_point(lerp_point_1);
            points.write_point_to_buffer(VBO, lerp_point_1);

            Points::Point p5 = points.points[p + 4];
            Points::Point p6 = points.points[p + 5];
            Points::Point p7 = points.points[p + 6];
            Points::Point lerp_point_2 = cubic(p4, p5, p6, p7, t);
            points.add_point(lerp_point_2);
            points.write_point_to_buffer(VBO, lerp_point_2);
        }
    }

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (selected != -1)
        {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            Points::Point selected_p = points.points[selected];
            glm::vec3 old_position = selected_p.position;
            glm::vec3 new_position = convert_mouse_coord_to_world(x, y);
            glm::vec3 offset = new_position - old_position;
            points.modity_point_position_in_buffer(VBO, selected, new_position);
            int i = 7;
            for (float t = 0; t <= 1.1f; t += 0.05f)
            {
                Points::Point p_1 = points.points[i];
                Points::Point p_2 = points.points[i + 1];

                Points::Point p1 = points.points[0];
                Points::Point p2 = points.points[1];
                Points::Point p3 = points.points[2];
                Points::Point p4 = points.points[3];
                Points::Point p5 = points.points[4];
                Points::Point p6 = points.points[5];
                Points::Point p7 = points.points[6];

                glm::vec3 new_position_1 = cubic(p1, p2, p3, p4, t).position;
                points.modity_point_position_in_buffer(VBO, i, new_position_1);

                glm::vec3 new_position_2 = cubic(p4, p5, p6, p7, t).position;
                points.modity_point_position_in_buffer(VBO, i + 1, new_position_2);

                i += 2;
            }
        }

        glActiveTexture(GL_TEXTURE0);

        lava_shader.use();

        glBindVertexArray(VAO);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, baseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, emissionMap);

        lava_shader.setMat4("model", glm::mat4(1.0f));
        lava_shader.setMat4("view", glm::mat4(1.0f));
        lava_shader.setMat4("projection", glm::mat4(1.0f));

        glad_glPointSize(15);
        glDrawArrays(GL_POINTS, 0, vertex.number_of_points);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glfwTerminate();
    return 0;
}

Points::Point cubic(Points::Point p1, Points::Point p2, Points::Point p3, Points::Point p4, float t)
{
    return Points::Point(lerp(quadratic(p1, p2, p3, t), quadratic(p2, p3, p4, t), t));
}

glm::vec3 quadratic(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, float t)
{
    return glm::vec3(lerp(lerp(v1, v2, t), lerp(v2, v3, t), t));
}

glm::vec2 quadratic(glm::vec2 v1, glm::vec2 v2, glm::vec2 v3, float t)
{
    return glm::vec2(lerp(lerp(v1, v2, t), lerp(v2, v3, t), t));
}

Points::Point quadratic(Points::Point p1, Points::Point p2, Points::Point p3, float t)
{
    return Points::Point(lerp(lerp(p1, p2, t), lerp(p2, p3, t), t));
}

glm::vec3 lerp(glm::vec3 v1, glm::vec3 v2, float t)
{
    return glm::vec3(v1.x + t * (v2.x - v1.x), v1.y + t * (v2.y - v1.y), v1.z + t * (v2.z - v1.z));
}

glm::vec2 lerp(glm::vec2 v1, glm::vec2 v2, float t)
{
    return glm::vec2(v1.x + t * (v2.x - v1.x), v1.y + t * (v2.y - v1.y));
}

Points::Point lerp(Points::Point p1, Points::Point p2, float t)
{
    return Points::Point(lerp(p1.position, p2.position, t), lerp(p1.tex_coord, p2.tex_coord, t));
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

unsigned int loadTexture(char const *path)
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
            mouse_l_down = false;
        }
    }

    if (mouse_l_down)
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        for (int i = 0; i < points.num_points; i++)
        {
            Points::Point point = points.points[i];
            float distance = glm::distance(convert_mouse_coord_to_world(x, y), glm::vec3(point.position)) * 100;
            if (distance < MARKER_RADIUS)
            {
                selected = point.index;
                break;
            }
        }
    }
}

glm::vec3 convert_mouse_coord_to_world(double x, double y)
{
    return glm::vec3(2 * x / SCR_WIDTH - 1, 1 - 2 * y / SCR_HEIGHT, 0);
}