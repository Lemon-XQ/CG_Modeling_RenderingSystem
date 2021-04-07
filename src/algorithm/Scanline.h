#pragma once
#include "model/Model.h"
#include <algorithm>

enum Method {
	SCAN_LINE_ZBUFFER,
	INTERVAL_SCAN_LINE
};

struct Edge {
	float x;
	float dx;
	int dy;
	int poly_id;
	//float zx;// z=-(ax+by+d)/c
	Edge(float _x, float _dx, int _dy, int _id) {
		x = _x; dx = _dx; dy = _dy; poly_id = _id;
	}
	Edge(){}
};

struct Poly {
	glm::vec4 coeff; // abcd: ax+by+cz+d=0
	int id;
	int dy;
	glm::vec3 color;
};

class Scanline {
public:
	void init(int height, int width);
	float scan(vector<Model> models, Method method = SCAN_LINE_ZBUFFER);

	//float* get_z_buffer() { return _z_buffer; }
	glm::vec3* get_depth_buffer() { return _depth_buffer; }
	glm::vec3* get_frame_buffer() { return _frame_buffer; }

private:
	void build_tables(Model model);
	void build_tables(vector<Model> model);

	void scan_line_zbuffer();
	void interval_scan_line();

	int _height, _width;
	vector<vector<Edge>> ET;
	vector<Edge> AET;
	vector<vector<Poly>> PT;
	vector<Poly> APT, PT_by_id;
	glm::vec3* _frame_buffer = NULL;
	glm::vec3* _depth_buffer = NULL;
	float* _z_buffer = NULL;

	int _poly_cnt = 0;
	int _min_y = INT_MAX, _max_y = -INT_MAX, _min_x = INT_MAX, _max_x = -INT_MAX;
};