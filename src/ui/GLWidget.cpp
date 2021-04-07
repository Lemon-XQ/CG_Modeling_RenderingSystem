#include "GLWidget.h"

GLWidget::GLWidget(QWidget *parent) :
	QGLWidget(parent)
{
}

void GLWidget::initializeGL()
{
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);

	scan_line.init(win_height, win_width);
	Model model;
	if (model.load("../models/bunny.obj")) { // default load bunny model
		model.scale(win_width, win_height);
		models.push_back(model);
		_total_face += model.get_faces().size();
		_total_vertex += model.get_vertex_cnt();
	}
}

void GLWidget::resizeGL(int w, int h) {
	glViewport(0, 0, w, h);

	win_height = h;
	win_width = w;
	if (h != 0 && w != 0)
		scan_line.init(h, w);
}

void GLWidget::mouseMoveEvent(QMouseEvent *ev)
{
	int x = ev->pos().x();
	int y = win_height - ev->pos().y();

	// scale
	if (zoom_flag) {
		if (y > last_mouse_y) scale_ratio -= 0.1f;
		else  scale_ratio += 0.1f;
		scale_ratio = scale_ratio > 1.5f ? 1.5f : scale_ratio < 0.1 ? 0.1 : scale_ratio;// control scale ratio
		models[models.size() - 1].scale(win_width, win_height, scale_ratio);
	}

	// translate
	if (move_flag) {
		int dx = x - last_mouse_x;
		int dy = y - last_mouse_y;
		models[models.size() - 1].move(dx, dy);
		// Move control points simultaneously in design mode
		if (designFlag)
			curve.updateControlPoints(dx, dy);
	}

	// Select the anchor point (not rotatable when selected)
	if (click_anchor_flag) {
		int dx = x - last_mouse_x;
		int dy = y - last_mouse_y;
		// Updates the selected anchor location
		anchorPoints[clicked_anchor_index].x += dx;
		anchorPoints[clicked_anchor_index].y += dy;

		if (editFlag) {
			// update range
			int i = clicked_anchor_index;
			glm::vec2 min_pos = models[models.size() - 1].get_min_pos();
			glm::vec2 max_pos = models[models.size() - 1].get_max_pos();
			glm::vec2 middle_pos = min_pos + (max_pos - min_pos) / 2.0f;
			if (i == 0 || i == 1 || i == 3)
				model_deform_range_min = min_pos;
			else if (i == 2 || i == 4)
				model_deform_range_min = glm::vec2(middle_pos.x, min_pos.y);
			else if (i == 5 || i == 6)
				model_deform_range_min = glm::vec2(min_pos.x, middle_pos.y);
			else if (i == 7)
				model_deform_range_min = middle_pos;

			if (i == 0)
				model_deform_range_max = middle_pos;
			else if (i == 1 || i == 2)
				model_deform_range_max = glm::vec2(max_pos.x, middle_pos.y);
			else if (i == 4 || i == 6 || i == 7)
				model_deform_range_max = max_pos;
			else if (i == 3 || i == 5)
				model_deform_range_max = glm::vec2(middle_pos.x, max_pos.y);

			// update model
			models[models.size() - 1].deform(dx, dy, model_deform_range_min, model_deform_range_max);
		}
		else if (designFlag) {
			// update control points
			curve.updateControlPoint(clicked_anchor_index, anchorPoints[clicked_anchor_index]);
			// update model
			models[models.size() - 1].deformBySpline(&curve);
		}
	}
	// rotate
	else if (rotate_flag) {
		camera_yaw = -(x - last_mouse_x) * 0.5;// Need inverse (the rotation Angle about the X axis is negative when the mouse is moved to the right)
		if (camera_yaw < 0.0)
			camera_yaw += 360.0;
		else if (camera_yaw > 360.0)
			camera_yaw -= 360.0;

		camera_pitch = (y - last_mouse_y) * 0.5;
		if (camera_pitch < -90.0)
			camera_pitch = -90.0;
		else if (camera_pitch > 90.0)
			camera_pitch = 90.0;
		models[models.size() - 1].rotate(camera_yaw, camera_pitch);
	}

	last_mouse_x = x;
	last_mouse_y = y;

	updateGL();
}

void GLWidget::mousePressEvent(QMouseEvent *ev)
{
	int x = ev->pos().x();
	int y = win_height - ev->pos().y();

	// Left-click to rotate/select anchor points
	if (ev->button() == Qt::LeftButton) {
		rotate_flag = 1;

		// record the selected index of editable anchor points
		if (editFlag || designFlag) {
			for (int i = 0; i < anchorPoints.size();i++) {
				auto p = anchorPoints[i];
				//cout << abs(x - p.x) << " " << abs(y - p.y) << endl;
				if (abs(x - p.x) < 5.0f && abs(y - p.y) < 5.0f) {
					click_anchor_flag = 1;
					rotate_flag = 0;
					clicked_anchor_index = i;
					//cout << "clicked " << i << endl;
					break;
				}
			}
		}		
	}

	// Right click to zoom
	if (ev->button() == Qt::RightButton) {
		zoom_flag = 1;
	}

	// middle click to move
	if (ev->button() == Qt::MiddleButton) {
		move_flag = 1;
	}

	last_mouse_x = x;
	last_mouse_y = y;
}

void GLWidget::mouseReleaseEvent(QMouseEvent *ev)
{
	if (ev->button() == Qt::LeftButton) {
		rotate_flag = 0;
		click_anchor_flag = 0;
		clicked_anchor_index = -1;
	}
	if (ev->button() == Qt::RightButton) {
		zoom_flag = 0;
	}
	if (ev->button() == Qt::MiddleButton) {
		move_flag = 0;
	}
}

void GLWidget::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, win_width, 0, win_height);

	// Update editable anchor points in edit mode (8 points in the model bounding box)
	if (editFlag && clicked_anchor_index == -1) {
		anchorPoints.clear();		
		glm::vec2 min_pos = models[models.size() - 1].get_min_pos();
		glm::vec2 max_pos = models[models.size() - 1].get_max_pos();

		anchorPoints.resize(8);
		anchorPoints[0] = glm::vec3(min_pos, 0); // left_top
		anchorPoints[1] = glm::vec3(min_pos.x + (max_pos.x - min_pos.x) / 2, min_pos.y, 0);// middle_top 
		anchorPoints[2] = glm::vec3(max_pos.x, min_pos.y,0); // right_top
		anchorPoints[3] = glm::vec3(min_pos.x, min_pos.y + (max_pos.y - min_pos.y) / 2, 0); // left_middle
		anchorPoints[4] = glm::vec3(anchorPoints[2].x, anchorPoints[3].y, 0); // right_middle
		anchorPoints[5] = glm::vec3(min_pos.x, max_pos.y, 0); // left_bottom 
		anchorPoints[6] = glm::vec3(anchorPoints[1].x, max_pos.y, 0); // middle_bottom 
		anchorPoints[7] = glm::vec3(max_pos, 0); // right_bottom		
	}

	// scan conversion
	float scan_time = scan_line.scan(models, _scan_method);

	// get result
	if (_display_mode == Mode::COLOR)
		buffer = scan_line.get_frame_buffer();
	else
		buffer = scan_line.get_depth_buffer();

	// render model
	glBegin(GL_POINTS);
	for (int y = 0; y < win_height; y++) {
		for (int x = 0; x < win_width; x++) {
			int index = (win_height - 1 - y)*win_width + x;
			glColor3ub(buffer[index].x, buffer[index].y, buffer[index].z);
			glVertex2i(x, y);
		}
	}
	glEnd();

	// render text
	string hint_str[3] = { "Scan Time: " + to_string(scan_time) + " ms",
		"Faces: " + to_string(_total_face),
		"Vertices: " + to_string(_total_vertex)
	};
	glColor3ub(255, 255, 255);
	for (int i = 0; i < 3; i++) {
		glRasterPos2i(10, win_height - 20 * (i + 1));
		for (int j = 0; j < hint_str[i].length(); j++)
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, hint_str[i][j]);
	}


	// render anchor points
	if (editFlag || designFlag) {
		glColor3ub(255, 0, 0);
		glPointSize(10);
		glBegin(GL_POINTS);
		int offset = 0;
		if (designFlag) { // In the design mode, the anchor points are the control points, and the first and last two points are removed when rendering
			curve.getVisibleControlPoints(anchorPoints);
			offset = 1;
		}
		for (int i = offset; i < anchorPoints.size() - offset;i++) {
			glm::vec3 point = anchorPoints[i];
			glVertex2i(point.x, point.y);
		}
		glEnd();
	}

	// render spline curve
	if (designFlag) {
		int n = (anchorPoints.size() - 3) * LINES_PER_SEGMENT;
		glColor3ub(255, 0, 0);
		glLineWidth(2.0f);
		glEnable(GL_LINE_SMOOTH);
		glBegin(GL_LINES);
		glm::vec3 p1 = curve.getCurvePoint(0.0f);
		for (int i = 1; i <= n; i++) {
			float t = (float)i / (float)n;
			glm::vec3 p2 = curve.getCurvePoint(t);
			glVertex3d(p1.x, p1.y, p1.z);
			glVertex3d(p2.x, p2.y, p2.z);
			p1 = p2;
		}
		glEnd();
	}
	
	glFinish();
}