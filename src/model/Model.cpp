#include "Model.h"

int Model::_width, Model::_height;

Model::Model() 
{
}

bool Model::load(string model_path) 
{
	// let user to select the model when no path is specified
	if (model_path == "") 
		model_path = Util::ChooseFile();

	ifstream file(model_path);
	if (!file.is_open()) {
		cout << "Cannot open file" << endl;
		return false;
	}

	Timer timer;
	timer.TimeStart();

	vector<int> vertexIndices;
	string type;
	int v_idx, n_idx, t_idx;
	_normals.clear();
	_vertices.clear();
	_faces.clear();
	_vertex_cnt = 0;
	_center = glm::vec3(0, 0, 0);
	_min_x = FLT_MAX; _max_x = -FLT_MAX; _min_y = FLT_MAX; _max_y = -FLT_MAX; _min_z = FLT_MAX; _max_z = -FLT_MAX;

	// read obj
	while (file >> type) {
		if (type=="v") { // vertex
			glm::vec3 vertex;
			file >> vertex.x >> vertex.y >> vertex.z;
			_max_x = max(_max_x, vertex.x);
			_min_x = min(_min_x, vertex.x);
			_max_y = max(_max_y, vertex.y);
			_min_y = min(_min_y, vertex.y);
			_max_z = max(_max_z, vertex.z);
			_min_z = min(_min_z, vertex.z);
			_center += vertex;
			_vertices.push_back(vertex);
		}
		else if(type == "vn") { // vertex normal
			glm::vec3 normal;
			file >> normal.x >> normal.y >> normal.z;
			_normals.push_back(normal);
		}
		else if(type == "f") { // face: vertex_idx/texture_idx/normal_idx
			vertexIndices.clear();
			while (true) {
				char c = file.get();
				if (c == ' ') 
					continue;
				else if (c == '\n' || c == EOF) 
					break;
				else 
					file.putback(c);

				file >> v_idx;

				char splitter = file.get();
				n_idx = 0;

				if (splitter == '/') {
					splitter = file.get();
					if (splitter == '/') { // skip texture_idx
						file >> n_idx;
					}
					else {
						file.putback(splitter);
						file >> t_idx;
						splitter = file.get();
						if (splitter == '/') {
							file >> n_idx;
						}
						else {
							file.putback(splitter);
						}
					}
				}
				else {
					file.putback(splitter);
				}
				vertexIndices.push_back(v_idx - 1);
			}
			if(vertexIndices.size() >= 3) {
				Face face;
				for (int i = 0; i < vertexIndices.size(); i++) {
					_vertex_cnt++;
					face.vertices.emplace_back(_vertices[vertexIndices[i]]);
				}
				glm::vec3 a = _vertices[vertexIndices[0]], b = _vertices[vertexIndices[1]], c = _vertices[vertexIndices[2]];
				face.normal = glm::normalize(glm::cross(b - a, c - b));
				_faces.push_back(face);
			}
		}
	}
	file.close();
	_center /= _vertex_cnt;

	timer.TimeEnd();
	cout << "Load time:" << timer.TimeGap_in_ms() << " ms" << endl;
	cout << "Vertex num:" << _vertex_cnt << endl;
	cout << "Face num:" << _faces.size() << endl;

	if (_vertex_cnt == 0) return false;
	return true;
}

void Model::scale(int width, int height,float ratio)
{
	Timer timer;
	timer.TimeStart();

	// window size
	_width = width;
	_height = height;
	// model size
	float model_width = _max_x - _min_x, model_height = _max_y - _min_y;
	float scale = ratio * min(_height, _width) / max(model_width, model_height);
	glm::vec3 center = _center;
	_vertex_cnt = 0;

	// Update model size, model center and vertex position of each face
	_min_x = FLT_MAX; _max_x = -FLT_MAX; _min_y = FLT_MAX; _max_y = -FLT_MAX; _min_z = FLT_MAX; _max_z = -FLT_MAX;
	_center = glm::vec3(0, 0, 0);
	for (int i = 0; i < _faces.size(); i++)
	{
		for (int j = 0; j < _faces[i].vertices.size(); j++) {
			_faces[i].vertices[j] = (_faces[i].vertices[j] - center) * scale;
			// move to window center
			_faces[i].vertices[j].x = _faces[i].vertices[j].x + width / 2;
			_faces[i].vertices[j].y = _faces[i].vertices[j].y + height / 2;
			_min_x = min(_min_x, _faces[i].vertices[j].x);
			_max_x = max(_max_x, _faces[i].vertices[j].x);
			_min_y = min(_min_y, _faces[i].vertices[j].y);
			_max_y = max(_max_y, _faces[i].vertices[j].y);
			_min_z = min(_min_z, _faces[i].vertices[j].z);
			_max_z = max(_max_z, _faces[i].vertices[j].z);
			_center += _faces[i].vertices[j];
			_vertex_cnt++;
		}
	}
	_center /= _vertex_cnt;

	timer.TimeEnd();
	//cout << "Scale time:" << timer.TimeGap_in_ms() << " ms" << endl;
}

void Model::rotate(float yaw, float pitch, bool updateMin) 
{
	Timer timer;
	timer.TimeStart();

	// calc rotation matrix
	float theta_x = PI / 180 * pitch, theta_y= PI / 180 * yaw, theta_z = 0;
	glm::mat3 Rx, Ry, Rz;
	Rx = glm::mat3(1, 0, 0,
				   0, cos(theta_x), -sin(theta_x),
				   0, sin(theta_x), cos(theta_x));
	Ry = glm::mat3(cos(theta_y), 0, sin(theta_y),
				   0, 1, 0,
				   -sin(theta_y), 0, cos(theta_y));
	Rz = glm::mat3(cos(theta_z), -sin(theta_z), 0,
				   sin(theta_z), cos(theta_z), 0,
				   0, 0, 1);
	glm::mat3 R = Rz*Ry*Rx;

	if (updateMin) {
		_min_x = FLT_MAX; _max_x = -FLT_MAX; _min_y = FLT_MAX; _max_y = -FLT_MAX; _min_z = FLT_MAX; _max_z = -FLT_MAX;
	}

	// rotate every face
	for (int i = 0; i < _faces.size(); i++){
		Face& face = _faces[i];
		// update normal
		_faces[i].normal = R*_faces[i].normal;
		glm::vec3 win_center(_width / 2, _height / 2, 0);
		// update vertex
		for (int j = 0; j < _faces[i].vertices.size(); j++) {
			// rotate around its center
			_faces[i].vertices[j] = R*(_faces[i].vertices[j] - _center);
			_faces[i].vertices[j] += _center;
			// rotate around window center
			//_faces[i].vertices[j] = R*(_faces[i].vertices[j] - win_center);
			//_faces[i].vertices[j] += win_center;

			if (updateMin) {
				_min_x = min(_min_x, _faces[i].vertices[j].x);
				_max_x = max(_max_x, _faces[i].vertices[j].x);
				_min_y = min(_min_y, _faces[i].vertices[j].y);
				_max_y = max(_max_y, _faces[i].vertices[j].y);
				_min_z = min(_min_z, _faces[i].vertices[j].z);
				_max_z = max(_max_z, _faces[i].vertices[j].z);
			}
		}
	}

	timer.TimeEnd();
	//cout << "Rotate time:" << timer.TimeGap_in_ms() << " ms" << endl;
}

void Model::move(float dx, float dy)
{
	_min_x = FLT_MAX; _max_x = -FLT_MAX; _min_y = FLT_MAX; _max_y = -FLT_MAX; _min_z = FLT_MAX; _max_z = -FLT_MAX;

	// move every face
	for (int i = 0; i < _faces.size(); i++) {
		Face& face = _faces[i];
		glm::vec3 win_center(_width / 2, _height / 2, 0);
		// update vertex
		for (int j = 0; j < face.vertices.size(); j++) {
			// move in the x direction, then move in the y direction
			_faces[i].vertices[j].x += dx;
			_faces[i].vertices[j].y += dy;

			_min_x = min(_min_x, _faces[i].vertices[j].x);
			_max_x = max(_max_x, _faces[i].vertices[j].x);
			_min_y = min(_min_y, _faces[i].vertices[j].y);
			_max_y = max(_max_y, _faces[i].vertices[j].y);
			_min_z = min(_min_z, _faces[i].vertices[j].z);
			_max_z = max(_max_z, _faces[i].vertices[j].z);
		}
	}
	// update model center
	_center.x += dx;
	_center.y += dy;
}

void Model::deform(float dx, float dy, glm::vec2 deform_range_min, glm::vec2 deform_range_max)
{
	_min_x = FLT_MAX; _max_x = -FLT_MAX; _min_y = FLT_MAX; _max_y = -FLT_MAX; _min_z = FLT_MAX; _max_z = -FLT_MAX;
	_center = glm::vec3(0, 0, 0);
	_vertex_cnt = 0;

	// Move the faces near the anchor
	for (int i = 0; i < _faces.size(); i++) {
		Face& face = _faces[i];

		// update vertex
		for (int j = 0; j < face.vertices.size(); j++) {
			glm::vec3& v = face.vertices[j];

			// only update vertex in the range
			if (v.x<deform_range_min.x || v.x>deform_range_max.x || v.y<deform_range_min.y || v.y>deform_range_max.y) {
				_min_x = min(_min_x, v.x);
				_max_x = max(_max_x, v.x);
				_min_y = min(_min_y, v.y);
				_max_y = max(_max_y, v.y);
				_min_z = min(_min_z, v.z);
				_max_z = max(_max_z, v.z);
				_center += v;
				_vertex_cnt++;
				continue;
			}

			v.x += dx;
			v.y += dy;

			_min_x = min(_min_x, v.x);
			_max_x = max(_max_x, v.x);
			_min_y = min(_min_y, v.y);
			_max_y = max(_max_y, v.y);
			_min_z = min(_min_z, v.z);
			_max_z = max(_max_z, v.z);
			_center += v;
			_vertex_cnt++;
		}
		// update normal
		if (face.vertices.size() > 2) {
			glm::vec3 a = face.vertices[0], b = face.vertices[1], c = face.vertices[2];
			face.normal = glm::normalize(glm::cross(b - a, c - b));
		}
	}
	// update model center
	_center /= _vertex_cnt;
}


void Model::deformBySpline(CatmullRom* curve)
{
	float model_height = _max_y - _min_y, model_width = _max_x - _min_x, model_depth = _max_z - _min_z;
	float min_y = _min_y, min_x = _min_x, middle_x = (_min_x + _max_x) / 2, middle_y = (_min_y + _max_y) / 2, min_z = _min_z;

	_min_x = FLT_MAX; _max_x = -FLT_MAX; _min_y = FLT_MAX; _max_y = -FLT_MAX; _min_z = FLT_MAX; _max_z = -FLT_MAX;
	_center = glm::vec3(0, 0, 0);
	glm::vec3 offset; // Generated model offsets

	if (!init_flag) 
		offset = glm::vec3(_width / 2, _height / 2, 0);
	else 
		offset = glm::vec3(middle_x, middle_y, 0);

	// Generate the model according to the spline curve
	vector<glm::vec3> vs; // Model vertex set
	vs.resize((layers_horizontal + 1)*layers_vertical);
	_vertex_cnt = 0;
	// generate layer by layer
	for (int layer = 0; layer <= layers_horizontal; layer++) {
		for (int i = 0; i < layers_vertical; i++) {
			float m = i * 2 * PI / layers_vertical;

			glm::vec3 curvePoint = curve->getCurvePoint(layer / (float)layers_horizontal);
			
			float radius;
			if (!init_flag) 
				radius = curvePoint.x - _width/2;
			else 
				radius = curvePoint.x - middle_x;

			glm::vec3 v;
			v.x = radius*cos(m);
			v.y = radius*sin(m);
			v.z = curvePoint.y;

			v += offset; // origin v range is -radius~radius, need to normalize to 1

			vs[layer*layers_vertical + i] = v;

			_min_x = min(_min_x, v.x);
			_max_x = max(_max_x, v.x);
			_min_y = min(_min_y, v.y);
			_max_y = max(_max_y, v.y);
			_min_z = min(_min_z, v.z);
			_max_z = max(_max_z, v.z);
			_center += v;
			_vertex_cnt++;
		}
	}

	// generate face according to vertex
	_faces.clear();
	_faces.resize(2 * (_vertex_cnt - 2));
	for (int layer = 0; layer < layers_horizontal; layer++) {
		for (int i = 0; i < layers_vertical; i++) {
			Face& f = _faces[(layer*layers_vertical + i) * 2];
			Face& g = _faces[(layer*layers_vertical + i) * 2 + 1];
			f.vertices.push_back(vs[layer*layers_vertical + (i%layers_vertical)]);
			f.vertices.push_back(vs[layer*layers_vertical + ((i+1)%layers_vertical)]);
			f.vertices.push_back(vs[(layer+1)*layers_vertical + ((i+1)%layers_vertical)]);
			g.vertices.push_back(vs[layer*layers_vertical + (i%layers_vertical)]);
			g.vertices.push_back(vs[(layer+1)*layers_vertical + ((i+1)%layers_vertical)]);
			g.vertices.push_back(vs[(layer+1)*layers_vertical + (i%layers_vertical)]);

			// calc face normal
			if (f.vertices.size() > 2) {
				glm::vec3 a = f.vertices[0], b = f.vertices[1], c = f.vertices[2];
				f.normal = glm::normalize(glm::cross(b - a, c - b));
			}
			if (g.vertices.size() > 2) {
				glm::vec3 a = g.vertices[0], b = g.vertices[1], c = g.vertices[2];
				g.normal = glm::normalize(glm::cross(b - a, c - b));
			}
		}
	}
	
	// Generate faces for the top and bottom sides of the model
	int faceId = layers_vertical*layers_horizontal * 2;
	// top
	int vertex_first = layers_horizontal*layers_vertical;
	for (int i = vertex_first; i < vertex_first + layers_vertical - 2; i++) {
		int a = vertex_first; 
		int	b = (i + 1);
		int	c = (i + 2);
		glm::vec3 va = vs[b] - vs[a];
		glm::vec3 vb = vs[c] - vs[b];
		glm::vec3 vcross = glm::cross(va, vb);

		Face& f = _faces[faceId++];
		f.vertices.clear();
		f.vertices.push_back(vs[a]);
		f.vertices.push_back(vs[b]);
		f.vertices.push_back(vs[c]);
		// calc normal
		f.normal = glm::normalize(vcross);
	}

	// botttom
	vertex_first = 0;
	for (int i = vertex_first; i < vertex_first + layers_vertical - 2; i++) {
		int a = vertex_first; 
		int	b = (i + 1);
		int	c = (i + 2);
		glm::vec3 va = vs[b] - vs[a];
		glm::vec3 vb = vs[c] - vs[b];
		glm::vec3 vcross = glm::cross(vb, va);

		Face& f = _faces[faceId++];
		f.vertices.clear();
		f.vertices.push_back(vs[c]);
		f.vertices.push_back(vs[b]);
		f.vertices.push_back(vs[a]);
		// calc normal
		f.normal = glm::normalize(vcross);
	}
	
	// update model center
	_center /= _vertex_cnt;
	init_flag = true;
	rotate(0, 90, false);
}

// Generate model color (according to normal direction, with light)
glm::vec3 Model::CalcColor(Face face)
{
	glm::vec3 light_color = glm::vec3(0.4, 0.4, 0.0);
	glm::vec3 ambient_color = glm::vec3(0.5, 0.5, 0.5);
	glm::vec3 light_position = glm::vec3(_width / 2, _height / 2, 10000.0f);
	glm::vec3 color;

	for (auto vertex : face.vertices) {
		glm::vec3 ray_direction = glm::normalize(light_position - vertex);
		glm::vec3 &normal = face.normal;
		float cosine = std::abs(glm::dot(ray_direction, normal));
		color += cosine * light_color;
		color += ambient_color;
	}

	color /= face.vertices.size();
	color = glm::clamp(color, 0.0f, 1.0f);
	color *= 255;

	return color;
}

glm::vec3 Model::CalcColorByDepth(float z) 
{
	glm::vec3 color;
	float depth = ((z + _width / 2) / (_width * 1.3)) * 255;
	if (depth > 255) {
		depth = 255;
	}
	else if (depth < 0) {
		depth = 0;
	}
	color.x = depth;
	color.y = depth;
	color.z = depth;
	return color;
}
