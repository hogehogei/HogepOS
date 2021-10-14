//
// include files
//
#include <limits>

#include "Timer.hpp"
#include "Interrupt.hpp"
#include "Global.hpp"
#include "Task.hpp"

//
// constant
//

//
// static variables
//
namespace
{
    constexpr uint32_t k_MaxCount = 0xFFFFFFFFu;
    constexpr uint32_t k_InitialCount = 0x1000000u;
    volatile uint32_t* lvt_timer = reinterpret_cast<uint32_t*>(0xFEE00320);
    volatile uint32_t* initial_count = reinterpret_cast<uint32_t*>(0xFEE00380);
    volatile uint32_t* current_count = reinterpret_cast<uint32_t*>(0xFEE00390);
    volatile uint32_t* divide_config = reinterpret_cast<uint32_t*>(0xFEE003E0);
}

TimerManager* TimerManager::s_Instance = nullptr;

//
// static function declaration
// 

//
// funcion definitions
// 

Timer::Timer( uint32_t timeout, int value )
    : m_Timeout( 0 ),
      m_Value( value )
{
    m_Timeout = TimerManager::Instance().CurrentTick() + timeout;
}

Timer::Timer()
    : m_Timeout( std::numeric_limits<uint64_t>::max() ),
      m_Value( 0 )
{}

uint64_t Timer::Timeout() const
{
    return m_Timeout;
}

int Timer::Value() const
{
    return m_Value;
}

bool Timer::operator<( const Timer& rhs ) const
{
    return this->Timeout() > rhs.Timeout();
}

Timer Timer::InfiniteTimer() 
{
    return Timer();
}

TimerManager::TimerManager()
    : m_Tick( 0 ),
      m_Timers()
{
    m_Timers.push( Timer::InfiniteTimer() );
}

TimerManager& TimerManager::Instance()
{
    if( !s_Instance ){
        s_Instance = new TimerManager();
    }

    return *s_Instance;
}

bool TimerManager::Tick()
{
    ++m_Tick;

    bool task_timer_timeout = false;
    while( 1 ){
        const auto& t = m_Timers.top();

        // 一番近いタイムアウト時間まで、まだ経過していなければ
        // それ以降は確認する必要なし
        if( t.Timeout() > m_Tick ){
            break;
        }
        
        // ContextSwitch 用タイマなら
        if( t.Value() == k_TaskTimerValue ){
            task_timer_timeout = true;
            m_Timers.pop();
            m_Timers.push( Timer( k_TaskTimerPeriod, k_TaskTimerValue ) );
            continue;
        }
        Message m( Message::k_TimerTimeout );
        m.Arg.Timer.Value = t.Value();
        g_EventQueue.Push( m );

        m_Timers.pop();
    }

    return task_timer_timeout;
}

uint64_t TimerManager::CurrentTick() const
{
    uint64_t tick = 0;

    __asm__("cli");
    tick = m_Tick;
    __asm__("sti");

    return tick;
}

void TimerManager::AddTimer( const Timer& timer )
{
    m_Timers.push( timer );
}


void InitializeLAPICTimer()
{
    *divide_config = 0b1011;    // divide 1:1
    *lvt_timer = 0b001 << 16;   // masked, one-shot

    StartLAPICTimer();
    acpi::WaitMillSeconds( 100 );
    const auto elapsed = LAPICTimerElapsed();    
    StopLAPICTimer();

    // 1秒経過時間を計算
    g_LApicTimerFreq = static_cast<unsigned long>(elapsed) * 10;

    // 10ms 毎に割り込み発生となるように設定
    *divide_config = 0b1011;    // divide 1:1
    *lvt_timer = (0b010 << 16) | InterruptVector::kAPICTimer;     // not-masked, periodic
    *initial_count = g_LApicTimerFreq / k_TimerFreq;
}

void StartLAPICTimer()
{
    *initial_count = k_MaxCount;
}

uint32_t LAPICTimerElapsed()
{
    return k_MaxCount - *current_count;
}

void StopLAPICTimer()
{
    *initial_count = 0;
}

void LAPICTimerOnInterrupt()
{
    bool task_timer_timeout = TimerManager::Instance().Tick();
    NotifyEndOfInterrupt();

    if( task_timer_timeout ){
        TaskManager::Instance().SwitchTask();
    }
}