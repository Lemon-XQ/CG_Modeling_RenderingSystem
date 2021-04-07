#include "CatmullRom.h"

CatmullRom::CatmullRom()
{
	controlPoints.resize(MAX_NUM_CONTROL_POINTS);
	for (int i = 0; i < controlPoints.size(); i++)
		controlPoints[i] = glm::vec3(500 + 20 + i * 10 + i % 2 * 5 - floor(i / 3) * 30, i * 30 + 210, 0);

	initializeGeometryVectors();
	initializeBasisMatrix();
	initializeSegmentLengths();
	initializeArcLengthTable();
}

void CatmullRom::getVisibleControlPoints(vector<glm::vec3>& output)
{
	output.resize(MAX_NUM_CONTROL_POINTS);
	for (int i = 0; i < output.size(); i++)
		output[i] = controlPoints[i];
}

void CatmullRom::updateControlPoint(int index,glm::vec3 pos) 
{
	controlPoints[index] = pos;

	initializeGeometryVectors();
	initializeSegmentLengths();
	initializeArcLengthTable();
}

void CatmullRom::updateControlPoints(float dx,float dy) 
{
	for (auto& p : controlPoints) {
		p.x += dx;
		p.y += dy;
	}

	initializeGeometryVectors();
	initializeSegmentLengths();
	initializeArcLengthTable();
}

void CatmullRom::initializeGeometryVectors() 
{
	geometryVectorsX.clear();
	geometryVectorsY.clear();
	geometryVectorsZ.clear();

	// calc the tangent vector
	glm::vec3 tangents[MAX_NUM_CONTROL_POINTS - 2];
	for (int i = 1; i < controlPoints.size() - 1; i++) {
		glm::vec3 p1 = controlPoints[i - 1];
		glm::vec3 p2 = controlPoints[i + 1];

		float dx = 0.5f * (p2.x - p1.x);
		float dy = 0.5f * (p2.y - p1.y);
		float dz = 0.5f * (p2.z - p1.z);
		tangents[i - 1] = glm::vec3(dx, dy, dz);
	}

	// Construct vectors to store each fragment control point and tangent vector information
	for (int i = 0; i < controlPoints.size() - 3; i++) {
		glm::vec3 p1 = controlPoints[i + 1];
		glm::vec3 p2 = controlPoints[i + 2];
		glm::vec3 t1 = tangents[i];
		glm::vec3 t2 = tangents[i + 1];
		geometryVectorsX.push_back(glm::vec4(p1.x, p2.x, t1.x, t2.x));
		geometryVectorsY.push_back(glm::vec4(p1.y, p2.y, t1.y, t2.y));
		geometryVectorsZ.push_back(glm::vec4(p1.z, p2.z, t1.z, t2.z));
	}
}

void CatmullRom::initializeBasisMatrix() 
{
	// Eq: ax^3 + bx^2 + cx + d
	// x(0) =  p0x = [0 0 0 1]*Cx  // x component of p0
	// x(1) =  p3x = [1 1 1 1]*Cx  // x component of p3
	// x'(0) = t0x = [0 0 1 0]*Cx  // tangent x component to p0
	// x'(1) = t3x = [3 2 1 1]*Cx  // tangent x component to p3

	glm::mat4 invBasisMatrix = glm::mat4(
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
		glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(3.0f, 2.0f, 1.0f, 0.0f));

	// invert and transpose to get basis matrix
	basisMatrix = glm::transpose(glm::inverse(invBasisMatrix));
}

// get the point on the segment
glm::vec3 CatmullRom::evaluatePointOnSegment(float t, int segmentIdx) {

	glm::vec4 paramVect = glm::vec4(t*t*t, t*t, t, 1.0f);
	glm::vec4 geomX = geometryVectorsX[segmentIdx];
	glm::vec4 geomY = geometryVectorsY[segmentIdx];
	glm::vec4 geomZ = geometryVectorsZ[segmentIdx];

	// point = paramVect * basisMatrix * geometryVector
	float x = glm::dot(paramVect, basisMatrix * geomX);
	float y = glm::dot(paramVect, basisMatrix * geomY);
	float z = glm::dot(paramVect, basisMatrix * geomZ);

	return glm::vec3(x, y, z);
}

void CatmullRom::initializeSegmentLengths() {
	segmentLengths.clear();

	float totalLength = 0.0f;
	float step = 1.0f / (POINTS_PER_SEGMENT - 1);
	for (int segIdx = 0; segIdx < controlPoints.size() - 3; segIdx++) {
		// calc segment length
		float t = step;
		float segLen = 0.0f;
		glm::vec3 lastPoint = evaluatePointOnSegment(0.0f, segIdx);
		for (int i = 1; i < POINTS_PER_SEGMENT; i++) {
			glm::vec3 p = evaluatePointOnSegment(t, segIdx);
			float stepDist = glm::distance(p, lastPoint);
			segLen += stepDist;

			lastPoint = p;
			t += step;
		}

		totalLength += segLen;
		segmentLengths.push_back(segLen);
	}
	this->totalLength = totalLength;
}

void CatmullRom::initializeArcLengthTable() 
{
	float step = 1.0f / (POINTS_PER_SEGMENT - 1);
	float currentLength = 0.0f;

	arcLengthTable.clear();
	for (int segIdx = 0; segIdx < controlPoints.size() - 3; segIdx++) {
		float t = step;
		float segLen = 0.0f;
		glm::vec3 lastPoint = evaluatePointOnSegment(0.0f, segIdx);
		arcLengthTable.push_back(glm::vec3(currentLength, segIdx, 0.0f));
		for (int i = 1; i < POINTS_PER_SEGMENT; i++) {
			glm::vec3 p = evaluatePointOnSegment(t, segIdx);
			float stepDist = glm::distance(p, lastPoint);
			segLen += stepDist;
			currentLength += stepDist;

			arcLengthTable.push_back(glm::vec3(currentLength, segIdx, t));

			lastPoint = p;
			t += step;
		}
	}
}

int CatmullRom::getSegmentIndexAtArcLength(float len) {
	float currentLength = 0.0f;
	int i = 0;
	for (i = 0; i < segmentLengths.size(); i++) {
		currentLength += segmentLengths[i];
		if (len <= currentLength) {
			return i;
		}
	}
	return i - 1;
}

glm::vec3 CatmullRom::getPointAtArcLength(float len) 
{
	int segIdx = getSegmentIndexAtArcLength(len);
	int min = segIdx * POINTS_PER_SEGMENT;
	int max = min + POINTS_PER_SEGMENT - 1;
	for (int i = min; i < max; i++) {
		glm::vec3 data2 = arcLengthTable[i + 1];
		if (len <= data2.x) {
			glm::vec3 data1 = arcLengthTable[i];

			float minLen = data1.x;
			float maxLen = data2.x;
			float minT = data1.z;
			float maxT = data2.z;
			float ratio = (len - minLen) / (maxLen - minLen);
			float t = minT + ratio * (maxT - minT);
			return evaluatePointOnSegment(t, segIdx);
		}
	}

	// last control point
	glm::vec3 p = controlPoints[controlPoints.size() - 2];
	return p;
}

glm::vec3 CatmullRom::getCurvePoint(float t) {
	if (t < 0.0f) {
		t = 0.0f;
	}
	else if (t > 1.0f) {
		t = 1.0f;
	}
	return getPointAtArcLength(t * totalLength);
}