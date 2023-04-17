#pragma once
#include <JuceHeader.h>
using namespace std::chrono;
class MetronomeClock
{
public:
	MetronomeClock();
	using timepoint = time_point<system_clock>;
	// Fields
	//time_point<system_clock> tickLatest;
	time_point<system_clock> tickAtSynch;
	time_point<system_clock> tickAtPlay;
	time_point<system_clock> tickAtFirstSample;
	tm localtimeAtSynch;
	int msJuceAtSynch;
	int msMaxAtSynch;
	int msDifferenceJuceToMax;
	int msMaxAtPlay;
	int msJuceAtFirstSample;
	int msPlaybackDelay;
	int msMaxLatestOnset;

	// Methods
	static time_point<system_clock> tick();

	//void reset_common_clock();
	void synchWithMax(	int expectedHours, int expectedMinutes, 
					int expectedSeconds, int expectedMilliseconds, 
					int max_clock_time_at_synch, 
					time_point<system_clock> tick_at_synch_time);

	int getDurationSincePlayback(time_point<system_clock> tick);

	int getDurationSinceFirstSample(time_point<system_clock> tick);
	//int getDurationSinceFirstSample(MetronomeClock *clock,  time_point<system_clock> tick);

	void setStartOfPlayback(int ms_max);
	void setStartOfFirstSample(time_point<system_clock> tick);

	int getMsMaxSinceSynch(int ms_max);
	int msJuceSinceSynch(int ms_juce);

	void printMsMaxAsTime(int ms_max);
	time_point<system_clock> convertMsMaxToTick(int ms_max);
	tm msMaxToLocalTime(int ms_max);

	static int convertTickToMsJuce(time_point<system_clock> tick);
	static int getMsComponentFromTick(time_point<system_clock> tick);
	static juce::String tickToString(time_point<system_clock> tick);
	static tm tickToLocalTime(time_point<system_clock> tick);
};

