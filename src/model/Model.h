#pragma once
#include "utils/Util.h"

struct Face {
	glm::vec3 normal;
	vector<glm::vec3> vertices;
};

class Model {
public:
	Model();
	
	bool load(string model_path="");
	void scale(int width, int height, float ratio = 0.5f);
	void rotate(float yaw, float pitch, bool updateMin = true);
	void move(float dx, float dy);
	// Model deformation caused by model editing
	void deform(float dx, float dy, glm::vec2 deform_range_min, glm::vec2 deform_range_max);
	// Model deformation caused by model design
	void deformBySpline(CatmullRom* curve);

	static glm::vec3 CalcColor(Face face);
	static glm::vec3 CalcColorByDepth(float z);

	vector<Face> get_faces() { return _faces; }
	int get_vertex_cnt() { return _vertex_cnt; }
	glm::vec2 get_min_pos() { return glm::vec2(_min_x, _min_y); }
	glm::vec2 get_max_pos() { return glm::vec2(_max_x, _max_y); }
	glm::vec3 get_center() { return _center; }

	void addDepth(int coeff) {
		for (auto& face : _faces) {
			for (auto& vertex : face.vertices) {
				vertex.z -= coeff;
				vertex.x += coeff;
			}
		}
	}
	
private:
	vector<Face> _faces;
	vector<glm::vec3> _vertices, _normals;

	// bounding box
	float _max_x = -1, _max_y = -1, _min_x = FLT_MAX, _min_y = FLT_MAX, _max_z = -1, _min_z = FLT_MAX;
	static int _width, _height;
	glm::vec3 _center = glm::vec3(0, 0, 0); // model center
	int _vertex_cnt = 0;
	bool init_flag = false;

	const int layers_horizontal = 72;
	const int layers_vertical = 72;
};