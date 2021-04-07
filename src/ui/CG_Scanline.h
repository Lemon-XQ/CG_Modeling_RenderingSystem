#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_CG_Scanline.h"
#include <glut.h>
#include "model/Model.h"
#include "algorithm/Scanline.h"
#include "ui/GLWidget.h"
#include <QFileDialog>

class CG_Scanline : public QMainWindow
{
	Q_OBJECT

public:
	CG_Scanline(QWidget *parent = Q_NULLPTR);

private:
	Ui::CG_ScanlineClass ui;
	GLWidget* glwidget;

	int win_width = 1000, win_height = 750;

	vector<Model> models;
	Scanline scan_line;
	glm::vec3* buffer;

private slots:
	void on_loadBtn_clicked();
	void on_clearBtn_clicked();
	void on_modeBox_currentIndexChanged(int index);
	void on_methodBox_currentIndexChanged(int index);
	void on_editBtn_clicked();
	void on_designBtn_clicked();
	void on_saveBtn_clicked();
};
