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

struct Vertex
{
    unsigned int vertices_length;
    unsigned int number_of_points;
    float primitive_size;

    Vertex() : vertices_length(0), number_of_points(0), primitive_size(sizeof(float)) {}

    void add_vertices(float *vertices, int num_new_vertices, float *new_vertices)
    {
        unsigned int num_new_points = num_new_vertices / INFO_PER_POINT;
        for (int i = 0; i < num_new_vertices; i++)
        {
            vertices[i] = new_vertices[i];
        }
        this->vertices_length += num_new_vertices;
        this->number_of_points += num_new_points;
    }

    void add_vertices_update_buffer(unsigned int &VBO, int num_new_vertices, float *new_vertices)
    {
        unsigned int num_new_points = num_new_vertices / INFO_PER_POINT;

        glad_glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, vertices_length * primitive_size, num_new_vertices * primitive_size, new_vertices);

        this->vertices_length += num_new_vertices;
        this->number_of_points += num_new_points;
    }
};

struct Index
{
    unsigned int indices_length;
    unsigned int number_of_groups;
    float primitive_size;

    Index() : indices_length(0), number_of_groups(0), primitive_size(sizeof(unsigned int)) {}

    void add_indices(unsigned int *indices, int num_new_indices, unsigned int *new_indices)
    {
        unsigned int new_number_of_groups = num_new_indices / INFO_PER_POINT;
        for (int i = 0; i < num_new_indices; i++)
        {
            indices[i] = new_indices[i];
        }
        this->indices_length += num_new_indices;
        this->number_of_groups += new_number_of_groups;
    }

    void add_indices_update_buffer(unsigned int &EBO, int num_new_indices, unsigned int *new_indices)
    {
        unsigned int new_number_of_groups = num_new_indices / INFO_PER_POINT;

        glBindBuffer(GL_ARRAY_BUFFER, EBO);
        glBufferSubData(GL_ARRAY_BUFFER, indices_length * primitive_size, sizeof(new_indices), new_indices);

        this->indices_length += num_new_indices;
        this->number_of_groups += new_number_of_groups;
    }
};

struct Points
{
    struct Point
    {
        glm::vec3 position;
        glm::vec2 tex_coord;
        static Points *points;

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
};

Points::Point *Points::points = new Points::Point[MAX_NO_POINTS];
int Points::num_points = 0;
const int Points::num_info_per_point = 5;
vector<int> Points::info_length_per_point = {3, 2};
Vertex *Points::vertex = new Vertex;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);
glm::vec3 quadratic(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t);
glm::vec2 quadratic(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, float t);
Points::Point quadratic(Points::Point p1, Points::Point p2, Points::Point p3, float t);
glm::vec3 lerp(glm::vec3 p1, glm::vec3 p2, float t);
glm::vec2 lerp(glm::vec2 p1, glm::vec2 p2, float t);
Points::Point lerp(Points::Point p1, Points::Point p2, float t);

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
    Index index = Index();
    Points points = Points();

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

    points.add_point(Points::Point(glm::vec3(0.0f, 0.3f, 0.0f), glm::vec2(1.0f, 1.0f)));
    points.add_point(Points::Point(glm::vec3(0.1f, 0.1f, 0.0f), glm::vec2(0.0f, 1.0f)));
    points.add_point(Points::Point(glm::vec3(0.4f, 0.4f, 0.0f), glm::vec2(0.0f, 0.0f)));
    points.add_point(Points::Point(glm::vec3(0.6f, 0.3f, 0.0f), glm::vec2(1.0f, 0.0f)));

    points.write_all_points_to_buffer(VBO);

    int patch = 0;
    for (float t = 0; t <= 1.1f; t += 0.05f)
    {
        Points::Point p1 = points.points[patch];
        Points::Point p2 = points.points[patch + 1];
        Points::Point p3 = points.points[patch + 2];
        Points::Point p4 = points.points[patch + 3];

        Points::Point lerp_point = lerp(quadratic(p1, p2, p3, t), quadratic(p2, p3, p4, t), t);
        points.add_point(lerp_point);
        vertex.add_vertices_update_buffer(VBO, Points::num_info_per_point, lerp_point.get_properties_as_array());
    }

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

        glad_glPointSize(7);
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