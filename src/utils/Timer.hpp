#include <time.h>
#include <Windows.h>

class Timer {
public:
	Timer() {
		QueryPerformanceFrequency(&Freq);
		timegap_ms = 0;
	}
	float Get_TimeRes_in_ms() {
		return 1000.0f / Freq.QuadPart;
	}
	void TimeStart() {
		QueryPerformanceCounter(&StartTickCount);
	}
	void TimeEnd() {
		QueryPerformanceCounter(&EndTickCount);
	}
	float TimeGap_in_ms() {
		timegap_ms = (float)(EndTickCount.QuadPart - StartTickCount.QuadPart) * 1000.0f / Freq.QuadPart;
		return timegap_ms;
	}
private:
	LARGE_INTEGER Freq;
	LARGE_INTEGER StartTickCount;
	LARGE_INTEGER EndTickCount;
	float timegap_ms;
};