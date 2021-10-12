#pragma once

// 
// include headers
//
#include <cstdint>
#include <queue>

#include "Event.hpp"


//
// constants
// 
constexpr int k_TimerFreq = 100;

class Timer
{
public:
    
    Timer( uint32_t timeout, int value );

    uint64_t Timeout() const;
    int Value() const;

    bool operator<( const Timer& rhs ) const;

    static Timer InfiniteTimer();

private:

    Timer();

    uint64_t m_Timeout;
    int m_Value;
};

class TimerManager
{
public:

    ~TimerManager() = default;

    TimerManager( TimerManager& ) = delete;
    TimerManager& operator=( TimerManager& ) = delete;

    static TimerManager& Instance();

    void Tick();
    uint64_t CurrentTick() const;
    void AddTimer( const Timer& timer );

private:

    TimerManager();

    static TimerManager* s_Instance;
    
    volatile uint64_t m_Tick;
    std::priority_queue<Timer> m_Timers;
};

void InitializeLAPICTimer();
void StartLAPICTimer();
uint32_t LAPICTimerElapsed();
void StopLAPICTimer();

void LAPICTimerOnInterrupt();

