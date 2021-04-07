#include "ui/CG_Scanline.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	CG_Scanline w;
	w.show();
	return a.exec();
}
