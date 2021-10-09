//
// include files
//
#include <limits>

#include "Timer.hpp"
#include "Interrupt.hpp"
#include "Global.hpp"


//
// constant
//

//
// static variables
//
namespace
{
    constexpr uint32_t k_MaxcCount = 0xFFFFFFFFu;
    constexpr uint32_t k_InitialCount = 0x1000000u;
    volatile uint32_t* lvt_timer = reinterpret_cast<uint32_t*>(0xFEE00320);
    volatile uint32_t* initial_count = reinterpret_cast<uint32_t*>(0xFEE00380);
    volatile uint32_t* current_count = reinterpret_cast<uint32_t*>(0xFEE00390);
    volatile uint32_t* divide_config = reinterpret_cast<uint32_t*>(0xFEE003E0);
}

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
    Timer t( 0, 0 );
    t.m_Timeout = std::numeric_limits<uint64_t>::max();
    t.m_Value = 0;

    return t;
}

TimerManagerImpl::TimerManagerImpl()
    : m_Tick( 0 )
{
    m_Timers.push( Timer::InfiniteTimer() );
}

void TimerManagerImpl::Tick()
{
    ++m_Tick;

    while( 1 ){
        const auto& t = m_Timers.top();

        // 一番近いタイムアウト時間まで、まだ経過していなければ
        // それ以降は確認する必要なし
        if( t.Timeout() > m_Tick ){
            break;
        }

        Message m( Message::k_TimerTimeout );
        m.Arg.Timer.Value = t.Value();
        g_EventQueue.Push( m );

        m_Timers.pop();
    }
}

uint64_t TimerManagerImpl::CurrentTick() const
{
    uint64_t tick = 0;

    __asm__("cli");
    tick = m_Tick;
    __asm__("sti");

    return tick;
}

void TimerManagerImpl::AddTimer( const Timer& timer )
{
    m_Timers.push( timer );
}


TimerManager::TimerManager()
    : m_Impl( nullptr )
{}

bool TimerManager::Initialize()
{
    m_Impl = new TimerManagerImpl();

    return true;
}

TimerManager& TimerManager::Instance()
{
    static TimerManager s_Instance;
    return s_Instance;
}

void TimerManager::Tick()
{
    m_Impl->Tick();
}

uint64_t TimerManager::CurrentTick() const
{
    if( m_Impl ){
        return m_Impl->CurrentTick();
    }
    return 0;
}

void TimerManager::AddTimer( const Timer& timer )
{
    if( m_Impl ){
        m_Impl->AddTimer( timer );
    }
}

void InitializeLAPICTimer()
{
    *divide_config = 0b1011;                                      // divide 1:1
    *lvt_timer = (0b010 << 16) | InterruptVector::kAPICTimer;     // not-masked, periodic
    *initial_count = k_InitialCount;
}

void StartLAPICTimer()
{
    *initial_count = k_InitialCount;
}

void StopLAPICTimer()
{
    *initial_count = 0;
}

void LAPICTimerOnInterrupt()
{
    TimerManager::Instance().Tick();
}