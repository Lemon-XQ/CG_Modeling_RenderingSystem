#include "Util.h"

string Util::Lpcwstr2String(LPCWSTR lps) 
{
	int len = WideCharToMultiByte(CP_ACP, 0, lps, -1, NULL, 0, NULL, NULL);
	if (len <= 0) {
		return "";
	}
	else {
		char *dest = new char[len];
		WideCharToMultiByte(CP_ACP, 0, lps, -1, dest, len, NULL, NULL);
		dest[len - 1] = 0;
		string str(dest);
		delete[] dest;
		return str;
	}
}

string Util::ChooseFile()
{
	QString strPath = QFileDialog::getOpenFileName(NULL, QObject::tr("Load Model"), "../models/", QObject::tr("obj(*.obj)"));
	QTextCodec* code = QTextCodec::codecForName("GB2312"); // Solve the Chinese path problem
	std::string fileName = code->fromUnicode(strPath).data();
	if (fileName.empty()) return "";

	return fileName;
}
