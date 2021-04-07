#pragma once
#include "glm.hpp"
#include <vector>
using namespace std;

#define MAX_NUM_CONTROL_POINTS   12
#define LINES_PER_SEGMENT        50   // Number of line segments divided into each curve segment (for display)
#define POINTS_PER_SEGMENT       25   // Number of points sampled from each curve segment
#define VISIBLE_CONTROL_POINTS   12   // Points that the user can control

class CatmullRom
{
public:
	CatmullRom();

public:
	void getVisibleControlPoints(vector<glm::vec3>& output);

	// Update a control point location
	void updateControlPoint(int index, glm::vec3 pos);

	// Update all control point locations
	void updateControlPoints(float dx, float dy);
	
	// t:0-1
	glm::vec3 getCurvePoint(float t);

private:
	vector<glm::vec3>  controlPoints; 
	
	// Store control point and tangent vector information
	vector<glm::vec4>  geometryVectorsX;
	vector<glm::vec4>  geometryVectorsY;
	vector<glm::vec4>  geometryVectorsZ;

	vector<float>  segmentLengths;        // Record the arc length of all segments
	vector<glm::vec3> arcLengthTable;     // Record information for each segment (arc length, segment id, t)
	float totalLength; // Total length of curve

	glm::mat4 basisMatrix;  // basis function

	// init
	void initializeGeometryVectors();
	void initializeBasisMatrix();
	void initializeSegmentLengths();
	void initializeArcLengthTable();

	glm::vec3 evaluatePointOnSegment(float t, int segmentIdx);

	int getSegmentIndexAtArcLength(float len);

	glm::vec3 getPointAtArcLength(float len);

};