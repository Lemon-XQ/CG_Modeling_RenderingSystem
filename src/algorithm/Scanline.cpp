#include "Scanline.h"

void Scanline::init(int height, int width) 
{
	_height = height;
	_width = width;

	if (_z_buffer != NULL) delete[] _z_buffer;
	if (_depth_buffer != NULL) delete[] _depth_buffer;
	if (_frame_buffer != NULL) delete[] _frame_buffer;

	_z_buffer = (float*)malloc(sizeof(float)*width);
	_depth_buffer = (glm::vec3*)malloc(sizeof(glm::vec3)*height*width);
	_frame_buffer = (glm::vec3*)malloc(sizeof(glm::vec3)*height*width);

	PT.resize(_height);
	ET.resize(_height);
}

void Scanline::build_tables(Model model) 
{
	vector<Face> faces = model.get_faces();
	
//#pragma omp parallel for private(_poly_cnt) /*schedule(dynamic)*/
	for (int i = 0; i < faces.size(); i++) {
		// back culling
		glm::vec3 viewpoint(_width / 2, _height / 2, 10000);	
		glm::vec3 a = faces[i].vertices[0], b = faces[i].vertices[1], c = faces[i].vertices[2];
		glm::vec3 face_center = (a + b + c)*(1 / 3.0f);
		float normal_product = glm::dot(faces[i].normal, viewpoint - face_center);
		// Determine vertex order of facets (clockwise/counterclockwise)
		bool clockwise = true; // Default is clockwise
		float mD = -glm::dot(faces[i].normal, faces[i].vertices[0]);// get another parameter to the equation of the plane
		float temp = glm::dot(faces[i].normal,model.get_center()) + mD;
		if (temp < 0)	clockwise = false;
		// discard faces whose dot product is <0 and whose vertex order is counterclockwise
		if (normal_product < 0 && !clockwise) continue;

		int min_y = INT_MAX, max_y = -INT_MAX, min_x = INT_MAX, max_x = -INT_MAX;
		map<int, int> valid_edge_map;
		Poly poly;
		vector<float> border_left_pos, border_right_pos;
		for (int j = 0; j < faces[i].vertices.size(); j++) {
			Edge edge;
			edge.poly_id = _poly_cnt;
			// The upper and lower endpoints of the edge (the origin is in the lower left corner, the larger y is, the higher y is)
			glm::vec3 top_point, bottom_point;
			glm::vec3 cur_point = faces[i].vertices[j], next_point;
			if (j == faces[i].vertices.size() - 1)// The last point and the first point will also form an edge!
				next_point = faces[i].vertices[0];
			else
				next_point = faces[i].vertices[j + 1];
			if (cur_point.y > next_point.y) {
				top_point = cur_point;
				bottom_point = next_point;
			}
			else {
				top_point = next_point;
				bottom_point = cur_point;
			}

			// calc dx=-1/k
			edge.dx = -1 * (bottom_point.x - top_point.x) / (bottom_point.y - top_point.y);

			// clip (remove invisible edges)
			if (top_point.y > _height - 1) {// Updates the intersection point when the upper endpoint exceeds the upper boundary
				if (bottom_point.y >= _height - 1) continue;// The edge goes beyond the upper boundary
				top_point.y = _height - 1;
				top_point.x = bottom_point.x + edge.dx*(bottom_point.y - top_point.y);
			}
			else if (top_point.y < 0) {// An upper endpoint exceeds the lower boundary, that is, an edge exceeds the lower boundary
				continue;
			}
			else if (bottom_point.y < 0) {// An upper endpoint exceeds the lower boundary, that is, an edge exceeds the lower boundary	
				bottom_point.x = bottom_point.x + edge.dx*bottom_point.y;
				bottom_point.y = 0;
			}

			edge.x = top_point.x;

			// Calculate the number of scan lines crossed by EDGE (dy = 0 indicates coincidence with scan lines, skip)
			edge.dy = round(top_point.y) - round(bottom_point.y);
			if (edge.dy <= 0) continue;

//#pragma omp critical 
			// Insert edge table (according to ymax)
			ET[round(top_point.y)].push_back(edge);

			min_y = min(min_y, round(bottom_point.y));
			max_y = max(max_y, round(top_point.y));
			min_x = min(min_x, min(round(bottom_point.x), round(top_point.x)));
			max_x = max(max_x, max(round(bottom_point.x), round(top_point.x)));

			// Record the position and number of insert edge table
			if (valid_edge_map.count(round(top_point.y)) == 0) valid_edge_map[round(top_point.y)] = 0;
			valid_edge_map[round(top_point.y)]++;
		}

		// Polygon not visible (remove corresponding edge)
		if (max_y<0 || min_y>_height - 1 || min_x > _width - 1 || max_x < 0) {
//#pragma omp critical
			for (auto& pair : valid_edge_map) {
				ET[pair.first]._Pop_back_n(pair.second);
			}
			continue;
		}
		poly.id = _poly_cnt;
		poly.dy = max_y - min_y;
		poly.color = Model::CalcColor(faces[i]);
		// ax+by+cz+d=0
		poly.coeff.a = faces[i].normal.x;
		poly.coeff.b = faces[i].normal.y;
		poly.coeff.c = faces[i].normal.z;
		poly.coeff.d = -(glm::dot(faces[i].normal, faces[i].vertices[0])); // d = -(ax+by+cz)

//#pragma omp critical
		{
			// Insert polygon table (according to ymax)
			//PT[max_y].push_back(poly);

			// Insert polygon table (according to poly_id)
			PT_by_id.push_back(poly);
			_poly_cnt++;
		}
	}
	//cout << PT_by_id.size() << endl;
}

void Scanline::build_tables(vector<Model> models)
{
	// clear
	//for (int i = 0; i < PT.size(); i++) {
	//	PT[i].clear();
	//}
	for (int i = 0; i < ET.size(); i++) {
		ET[i].clear();
	}
	PT_by_id.clear();

	_poly_cnt = 0;

	for (Model model : models)
		build_tables(model);	
}

float Scanline::scan(vector<Model> models, Method method)
{
	if (method == 0)
		cout << "************Scan method: SCAN_LINE_ZBUFFER************" << endl;
	else
		cout << "************Scan method: INTERVAL_SCAN_LINE***********" << endl;

	Timer timer, total_timer, scan_timer;
	total_timer.TimeStart();
	timer.TimeStart();
	build_tables(models);
	timer.TimeEnd();
	cout << "-----------Build ET/PT time:" << timer.TimeGap_in_ms() << " ms----------" << endl;

	scan_timer.TimeStart();
	switch (method) {
	case Method::SCAN_LINE_ZBUFFER:
		scan_line_zbuffer();
		break;

	case Method::INTERVAL_SCAN_LINE:
		interval_scan_line();
		break;
	}
	scan_timer.TimeEnd();
	total_timer.TimeEnd();
	cout << "-----------Scan time:" << scan_timer.TimeGap_in_ms() << " ms----------" << endl;
	cout << "-----------Total time:" << total_timer.TimeGap_in_ms() << " ms---------" << endl;

	return total_timer.TimeGap_in_ms();
}


// The edges of the same polygon sorted by x (if x is same, by dx)
bool cmp_x(Edge a, Edge b) 
{
	if (a.poly_id == b.poly_id) {
		if (a.x != b.x)
			return a.x < b.x;
		else
			return a.dx < b.dx;
	}
	else {
		return a.poly_id < b.poly_id;
	}
}

// Sort by x (if x is same, by dx)
bool cmp_x_2(Edge a, Edge b)
{
	if (a.x != b.x)
		return a.x < b.x;
	else
		return a.dx < b.dx;
}

float get_Z(Poly& poly, float x, float y) {
	if (abs(poly.coeff.c) < 1e-5) // special case
		return -FLT_MAX;
	return -(poly.coeff.a*round(x) + poly.coeff.b*y + poly.coeff.d) / poly.coeff.c;
}

float get_dZ(Poly& poly) {
	if (abs(poly.coeff.c) < 1e-5) // special case
		return 0;
	return -poly.coeff.a / poly.coeff.c;
}

void Scanline::scan_line_zbuffer()
{
	memset(_depth_buffer, 0, sizeof(glm::vec3)*_height*_width);
	memset(_frame_buffer, 0, sizeof(glm::vec3)*_height*_width);
	AET.clear();
	//APT.clear();
	for (int y = _height - 1; y >= 0; y--) {

		// clear zbuffer
		fill(_z_buffer, _z_buffer + _width, -FLT_MAX);

		// update AET£¬APT (insert new edge, new polygon)
		for (int i = 0; i < ET[y].size(); i++) {
			AET.push_back(ET[y][i]);
		}
		//for (int i = 0; i < PT[y].size(); i++) {
		//	APT.push_back(PT[y][i]);
		//}

		if (AET.size() == 0) continue;

		// Pair of edges of the same polygon sorted according to the x coordinate of the edges (dx if the x is the same)
		sort(AET.begin(), AET.end(), cmp_x);

		// update depth of each side pair of AET
//#pragma omp parallel for schedule(dynamic)
		for (int i = 1; i < AET.size(); i += 2) {
			Edge& cur_edge_left = AET[i - 1];
			Edge& cur_edge_right = AET[i];
			Poly poly = PT_by_id[cur_edge_left.poly_id];
			
			if (round(cur_edge_right.x) - round(cur_edge_left.x) <= 0) continue;

			// calc depth
			float zx = get_Z(poly, cur_edge_left.x, y);
			float dzx = get_dZ(poly);

			// Outbounds of the window are not displayed
			float x_start, x_end;
			x_start = clamp(round(cur_edge_left.x), 0, _width - 1);
			x_end = clamp(round(cur_edge_right.x), 0, _width - 1);			
//#pragma omp parallel for		
			// Fill each pixel in the edge pair
			for (int x = x_start; x < x_end; x++) {
				if (zx > _z_buffer[x]) {
					_z_buffer[x] = zx;
					_frame_buffer[(_height - 1 - y)*_width + x] = poly.color;
					_depth_buffer[(_height - 1 - y)*_width + x] = Model::CalcColorByDepth(zx);
				}
				zx = zx + dzx;
			}
		}
		
		// Update AET (remove edge at end of scan)
		int cnt = 0;
		for (int i = 0; i < AET.size(); i++) {
			AET[i].dy -= 1;
			AET[i].x += AET[i].dx;
			if (AET[i].dy != 0) {
				AET[cnt] = AET[i];
				cnt++;
			}
		}
		AET.resize(cnt);

		//// Update APT (remove polygons after scan)
		//cnt = 0;
		//for (int i = 0; i < APT.size(); i++) {
		//	APT[i].dy -= 1;
		//	if (APT[i].dy != 0) {
		//		APT[cnt] = APT[i];
		//		cnt++;
		//	}
		//}
		//APT.resize(cnt);		
	}
}

void Scanline::interval_scan_line() 
{
	vector<int> in_poly_ids,in_poly_ids_last;
	bool update_in_flag = false;
	memset(_depth_buffer, 0, sizeof(glm::vec3)*_height*_width);
	memset(_frame_buffer, 0, sizeof(glm::vec3)*_height*_width);
	AET.clear();
	//APT.clear();
	for (int y = _height - 1; y >= 0; y--) {
		// Update AET, APT (insert new edge, new polygon)
		//for (auto i = 0; i < PT[y].size(); i++) {
		//	APT.push_back(PT[y][i]);
		//}
		for (auto i = 0; i < ET[y].size(); i++) {
			AET.push_back(ET[y][i]);
		}

		if (!ET[y].empty() || !AET.empty()) {
			update_in_flag = true;
		}

		if (update_in_flag) {
			in_poly_ids.clear();
			in_poly_ids_last.clear();
		}

		if (AET.size() == 0) continue;

		// Sort by the x coordinates of the edges (dx if the x is the same)
		sort(AET.begin(), AET.end(), cmp_x_2);

		// update depth of each side pair of AET
//#pragma omp parallel for schedule(dynamic)
		for (int i = 0; i < AET.size(); i++) {
			Edge& cur_edge_left = AET[i];
			if (i + 1 == AET.size()) break;
			Edge& cur_edge_right = AET[i+1];
			Poly poly;
			float cur_edge_middle = (cur_edge_left.x + cur_edge_right.x) / 2;

			// update in_id
			if (update_in_flag) {
				auto pos = find(in_poly_ids.begin(), in_poly_ids.end(), cur_edge_left.poly_id);
				if (pos == in_poly_ids.end()) {
					in_poly_ids.push_back(cur_edge_left.poly_id);
				}
				else {
					in_poly_ids.erase(pos);
				}
				in_poly_ids_last = in_poly_ids;
			}
			else {
				in_poly_ids = in_poly_ids_last;
			}

			// Important!!! If in_ids is empty, indicating that the background is between the edge pairs
			if (in_poly_ids.empty()) continue;

			// check valid
			if (round(cur_edge_right.x) - round(cur_edge_left.x) <= 0) continue;

			// process throughout cases
			float edge_through_x = cur_edge_right.x;
			// Find the deepest polygon on the left and right
			int top_id_left = -1, top_id_right = -1;
			float max_z_left = -FLT_MAX, max_z_right = -FLT_MAX;
			for (int poly_id : in_poly_ids) {
				poly = PT_by_id[poly_id];
				float z_left = get_Z(poly, cur_edge_left.x, y);
				float z_right = get_Z(poly, cur_edge_right.x, y);
				if (z_left > max_z_left) {
					top_id_left = poly_id;
					max_z_left = z_left;
				}
				if (z_right > max_z_right) {
					top_id_right = poly_id;
					max_z_right = z_right;
				}
			}
			if (top_id_left == -1 || top_id_right == -1) continue;

			// Find crossover points
			float dzx_1, dzx_2;
			if (top_id_left != top_id_right) {
				poly = PT_by_id[top_id_left];
				float z_1 = get_Z(poly, cur_edge_left.x, y);
				dzx_1 = get_dZ(poly);
				poly = PT_by_id[top_id_right];
				float z_2 = get_Z(poly, cur_edge_left.x, y);
				dzx_2 = get_dZ(poly);
				if (dzx_1 != dzx_2)
					edge_through_x = cur_edge_left.x + (z_2 - z_1) / (dzx_1 - dzx_2);
				else
					edge_through_x = cur_edge_left.x;
			}
			else {
				poly = PT_by_id[top_id_left];
				dzx_1 = dzx_2 = get_dZ(poly);
			}

			edge_through_x = clamp(edge_through_x, cur_edge_left.x, cur_edge_right.x);

			// Fill each pixel in the edge pair (before the crossover point)
			int x_start, x_end, offset_start, offset_end;
			x_start = clamp(round(cur_edge_left.x), 0, _width - 1);// Outbounds of the window are not displayed
			x_end = clamp(round(edge_through_x), 0, _width - 1);
			offset_start = (_height - 1 - y)*_width + x_start;
			offset_end = (_height - 1 - y)*_width + x_end;
			poly = PT_by_id[top_id_left];
			float zx = get_Z(poly, cur_edge_left.x, y);
			for (int x = x_start; x < x_end; x++) {
				_frame_buffer[(_height - 1 - y)*_width + x] = poly.color;
				_depth_buffer[(_height - 1 - y)*_width + x] = Model::CalcColorByDepth(zx);
				zx += dzx_1;
			}

			// Fill each pixel in the edge pair (after the crossover point)
			poly = PT_by_id[top_id_right];
			x_start = clamp(round(edge_through_x), 0, _width - 1);;
			x_end = clamp(round(cur_edge_right.x), 0, _width - 1);
			offset_start = (_height - 1 - y)*_width + x_start;
			offset_end = (_height - 1 - y)*_width + x_end;	
			zx = get_Z(poly, edge_through_x, y);
			for (int x = x_start; x < x_end; x++) {
				_frame_buffer[(_height - 1 - y)*_width + x] = poly.color;
				_depth_buffer[(_height - 1 - y)*_width + x] = Model::CalcColorByDepth(zx);
				zx += dzx_2;
			}
		}		
		
		// Update AET (remove edge at end of scan)
		int cnt = 0;
		for (int i = 0; i < AET.size(); i++) {
			AET[i].dy -= 1;
			AET[i].x += AET[i].dx;
			if (AET[i].dy != 0) {
				AET[cnt] = AET[i];
				cnt++;
			}
		}
		// Y coherence (update in_ids only when AET changes)
		if (cnt != AET.size()) 
			update_in_flag = true;
		else 
			update_in_flag = false;
		AET.resize(cnt);

		// Update APT (remove polygons after scan)
/*		cnt = 0;
		for (int i = 0; i < APT.size(); i++) {
			APT[i].dy -= 1;
			if (APT[i].dy != 0) {
				APT[cnt] = APT[i];
				cnt++;
			}
		}
		APT.resize(cnt);*/		
	}
}