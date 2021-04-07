#pragma once
#include "windows.h"
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <glm.hpp>
#include "Timer.hpp"
#include <chrono>
#include "CatmullRom.h"
#include <QFileDialog>
#include <qtextcodec.h>
//#include <GL/freeglut.h>

using namespace std;

#define max(a,b) (a > b ? a : b)
#define min(a,b) (a < b ? a : b)

const float PI = 3.141592653589793;

inline float norm(glm::vec3 a) {
	return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
 }

inline float clamp(float val, float min_v, float max_v) {
	return val > max_v ? max_v : val < min_v ? min_v : val;
}


class Util {
public:
	static string ChooseFile();

private:
	static string Lpcwstr2String(LPCWSTR lps);
};