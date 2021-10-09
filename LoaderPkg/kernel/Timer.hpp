#pragma once

// 
// include headers
//
#include <cstdint>
#include <queue>

#include "Event.hpp"

class Timer
{
public:
    
    Timer( uint32_t timeout, int value );

    uint64_t Timeout() const;
    int Value() const;

    bool operator<( const Timer& rhs ) const;

    static Timer InfiniteTimer();

private:

    uint64_t m_Timeout;
    int m_Value;
};

class TimerManagerImpl
{
    friend class TimerManager;
private:

    TimerManagerImpl();

    void Tick();
    uint64_t CurrentTick() const;
    void AddTimer( const Timer& timer );

    volatile uint64_t m_Tick;
    std::priority_queue<Timer> m_Timers;
};

class TimerManagerImpl;
class TimerManager
{
public:

    TimerManager();
    ~TimerManager() = default;

    TimerManager( TimerManager& ) = delete;
    TimerManager& operator=( TimerManager& ) = delete;

    bool Initialize();
    static TimerManager& Instance();

    void Tick();
    uint64_t CurrentTick() const;
    void AddTimer( const Timer& timer );

private:

    TimerManagerImpl* m_Impl;
};

void InitializeLAPICTimer();
void StartLAPICTimer();
void StopLAPICTimer();

void LAPICTimerOnInterrupt();

