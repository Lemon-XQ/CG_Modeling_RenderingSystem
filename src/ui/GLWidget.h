#pragma once
#include <QMouseEvent>
#include <qgl.h>
#include <glut.h>
#include "Model.h"
#include "Scanline.h"

// Display mode
enum Mode {
	DEPTH,
	COLOR
};

class GLWidget : public QGLWidget {
	Q_OBJECT

public:
	explicit GLWidget(QWidget *parent = 0);

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

public:
	// init model
	void load(string model_path="") {
		Model model;
		if (model.load(model_path)) {
			model.scale(win_width, win_height);
			models.push_back(model);
			_total_face += model.get_faces().size();
			_total_vertex += model.get_vertex_cnt();
		}
		else {
			cout << "Error while loading model...vertex_cnt = 0" << endl;
		}
	}

	// init base model£¨used for design£©
	void loadDesignModel() {
		Model model;
		model.deformBySpline(&curve);
		models.push_back(model);
		_total_face += model.get_faces().size();
		_total_vertex += model.get_vertex_cnt();
	}

	void setMethod(int index) {
		if (index == 0) _scan_method = Method::SCAN_LINE_ZBUFFER;
		else _scan_method = Method::INTERVAL_SCAN_LINE;
	}

	void setMode(int index) {
		if (index == 0) _display_mode = Mode::COLOR;
		else _display_mode = Mode::DEPTH;
	}

	void setEditFlag(bool flag) {
		editFlag = flag;
	}

	void setDesignFlag(bool flag) {
		designFlag = flag;
	}

	void clear() {
		anchorPoints.clear();
		models.clear();
		_total_face = 0;
		_total_vertex = 0;
		editFlag = false;
		designFlag = false;
		curve = CatmullRom();
	}

	vector<Model> getModels() { return models; }
	bool getDesignFlag() { return designFlag; }

private:
	int win_width = 1000;
	int win_height = 750;	
	
	int  last_mouse_x, last_mouse_y;

	// rotate/scale/translate
	float camera_yaw = 0.0f;
	float camera_pitch = 0.0f;
	float camera_distance = 5.0f;
	float scale_ratio = 0.5f;

	bool zoom_flag = 0;
	bool rotate_flag = 0;
	bool move_flag = 0;

	// model editing/design	
	bool editFlag = 0;
	bool designFlag = 0;
	bool click_anchor_flag = 0;
	vector<glm::vec3> anchorPoints;
	int  clicked_anchor_index = -1;
	glm::vec2 model_deform_range_min, model_deform_range_max;	
	CatmullRom curve;

	// scan
	vector<Model> models; // All Model objects in the current scene (default only operates on the current Model)
	Scanline scan_line;
	glm::vec3* buffer;
	Method _scan_method = Method::SCAN_LINE_ZBUFFER;
	Mode _display_mode = Mode::COLOR;

	int _total_face = 0, _total_vertex = 0;
};