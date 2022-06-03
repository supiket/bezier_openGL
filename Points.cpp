#include <glm/glm.hpp>
#include <vector>
#include "./glad.h"

using namespace std;

#define INFO_PER_POINT 8
#define MAX_NO_POINTS 100000

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
