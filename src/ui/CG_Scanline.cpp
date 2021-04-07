#include "CG_Scanline.h"

CG_Scanline::CG_Scanline(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	glwidget = new GLWidget();
	glwidget->resize(win_width, win_height);
	ui.glWidgetArea->setWidget(glwidget); 	

	ui.methodBox->addItem("Scanline Zbuffer");
	ui.methodBox->addItem("Interval Scanline");

	ui.modeBox->addItem("Color");
	ui.modeBox->addItem("Depth");
}

void CG_Scanline::on_loadBtn_clicked()
{
	glwidget->setEditFlag(false);
	glwidget->setDesignFlag(false);
	glwidget->load();
	glwidget->updateGL();
}

void CG_Scanline::on_clearBtn_clicked()
{
	glwidget->clear();
	glwidget->updateGL();
}

void CG_Scanline::on_methodBox_currentIndexChanged(int index)
{
	glwidget->setMethod(index);
	glwidget->updateGL();
}

void CG_Scanline::on_modeBox_currentIndexChanged(int index)
{
	glwidget->setMode(index);
	glwidget->updateGL();
}

void CG_Scanline::on_editBtn_clicked()
{
	glwidget->setEditFlag(true);
	glwidget->setDesignFlag(false);
	glwidget->updateGL();
}

void CG_Scanline::on_designBtn_clicked()
{
	glwidget->clear();
	glwidget->setEditFlag(false);	
	glwidget->loadDesignModel();
	glwidget->setDesignFlag(true);
	glwidget->updateGL();
}

void CG_Scanline::on_saveBtn_clicked()
{
	vector<Model> models = glwidget->getModels();

	// choose file
	QString strPath = QFileDialog::getSaveFileName(NULL, QObject::tr("Save Model"), "../models/output.obj", QObject::tr("obj(*.obj)"));
	QTextCodec* code = QTextCodec::codecForName("GB2312");
	std::string fileName = code->fromUnicode(strPath).data();
	if (fileName.empty()) return;

	ofstream file(fileName);
	if (!file.is_open()) {
		cout << "Cannot open file" << endl;
		return;
	}

	// save model
	int vertex_offset = 1; // face's vertex id start from 1(in .obj format)
	for (auto model : models) {
		vector<Face> faces = model.get_faces();
		for (int i = 0; i < faces.size(); i++) {
			for (auto vertex : faces[i].vertices) {
				file << "v " << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
			}
			file << "f ";
			for (int index = vertex_offset; index < vertex_offset + faces[i].vertices.size(); index++)
				file << index << " ";
			file << "\n";
			vertex_offset += faces[i].vertices.size();
		}
	}
	file.close();
}



