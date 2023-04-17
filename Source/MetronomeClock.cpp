#include "MetronomeClock.h"
#include <JuceHeader.h>
using timepoint = time_point<system_clock>;

MetronomeClock::MetronomeClock()
{
    tickAtSynch = system_clock::now();
    tickAtPlay = system_clock::now();
    localtimeAtSynch = tickToLocalTime(tickAtSynch);

}

time_point<system_clock> MetronomeClock::tick()
{
    return system_clock::now();
}

void MetronomeClock::synchWithMax( int expectedHours, int expectedMinutes, 
                                int expectedSeconds, int expectedMilliseconds, 
                                int maxCpuclockFromOSC, 
                                time_point<system_clock> tickAtTimeOfSynch)
{
    localtimeAtSynch = tickToLocalTime(tickAtTimeOfSynch);

    tickAtSynch = tickAtTimeOfSynch;
    msJuceAtSynch = convertTickToMsJuce(tickAtSynch);
    msMaxAtSynch = maxCpuclockFromOSC;

    msDifferenceJuceToMax = msJuceAtSynch - msMaxAtSynch;

    DBG("Synched at - " << tickToString(tickAtSynch));
}

int MetronomeClock::getDurationSincePlayback(time_point<system_clock> tick) {
    auto durationSinceStart = tick - tickAtPlay;
    auto msSinceStart = duration_cast<milliseconds>(durationSinceStart);
    return msSinceStart.count();
}
int MetronomeClock::getDurationSinceFirstSample(time_point<system_clock> tick) {
    auto durationSinceStart = tick - tickAtFirstSample;
    auto msSinceStart = duration_cast<milliseconds>(durationSinceStart);
    return msSinceStart.count();
}

void MetronomeClock::setStartOfPlayback(int msMax) {
    msMaxAtPlay = msMax;
    tickAtPlay = convertMsMaxToTick(msMax);
}

void MetronomeClock::setStartOfFirstSample(time_point<system_clock> tick) {
    tickAtFirstSample = tick;
    msJuceAtFirstSample = convertTickToMsJuce(tickAtFirstSample);
    auto tickPlaybackDelay = tickAtFirstSample - tickAtPlay;
    msPlaybackDelay = duration_cast<milliseconds>(tickPlaybackDelay).count();
    DBG("Playback delay - " << msPlaybackDelay << "ms");
}

int MetronomeClock::convertTickToMsJuce(time_point<system_clock> tick)
{
    return duration_cast<milliseconds>(tick.time_since_epoch()).count();;
}

tm MetronomeClock::tickToLocalTime(time_point<system_clock> tick)
{
    auto time_t_AtSynch = system_clock::to_time_t(tick);
    return *std::localtime(&time_t_AtSynch);
}


void MetronomeClock::printMsMaxAsTime(int msMax)
{
    auto localTick = tick();
    DBG("MAX Time - " << tickToString(convertMsMaxToTick(msMax)));
    DBG("JUCE Time - " << tickToString(localTick));
}

time_point<system_clock> MetronomeClock::convertMsMaxToTick(int msMax)
{
    auto msMaxSinceSynch = getMsMaxSinceSynch(msMax);
    auto durationSinceMsMax = milliseconds(msMaxSinceSynch);
    //DBG("MAX CALC: " << tickToString(tickAtSynch + durationSinceMsMax));

    return tickAtSynch + durationSinceMsMax;
}

int MetronomeClock::getMsComponentFromTick(time_point<system_clock> tick)
{
    auto now_ms = time_point_cast<milliseconds>(tick);
    return now_ms.time_since_epoch().count() % 1000;
}

tm MetronomeClock::msMaxToLocalTime(int msMax)
{
    auto tick_since_synch = convertMsMaxToTick(msMax);

    return tickToLocalTime(tick_since_synch);
}

juce::String MetronomeClock::tickToString(time_point<system_clock> tick)
{
    tm local = MetronomeClock::tickToLocalTime(tick);
    int msRemainder = getMsComponentFromTick(tick);
    char time[13];
    int cx;

    cx = snprintf(time, 13, "%02d:%02d:%02d:%03d", local.tm_hour, local.tm_min, local.tm_sec, msRemainder);

    //auto hours = juce::String(local.tm_hour);
    //auto min = juce::String(local.tm_min);
    //auto sec = juce::String(local.tm_sec);
    //DBG(time);
    return juce::String(time);
    //return hours + ":" + min + ":" + sec + "." + ms;
}

int MetronomeClock::getMsMaxSinceSynch(int msMax)
{
    return msMax - msMaxAtSynch;
}

int MetronomeClock::msJuceSinceSynch(int msJuce)
{
    return msJuce - msJuceAtSynch;
}





