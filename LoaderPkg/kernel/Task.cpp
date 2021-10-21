//
// include files
//
#include "asmfunc.h"
#include "Task.hpp"
#include "Timer.hpp"
#include "Segment.hpp"

//
// constant
//

//
// static variables
//

namespace 
{
}

TaskManager* TaskManager::s_TaskManager = nullptr;


//
// static function declaration
// 

//
// funcion definitions
// 

Task::Task( uint64_t id )
    : m_ID( id )
{}

Task& Task::InitContext( TaskFunc* f, int64_t data )
{
    const size_t stack_size = k_DefaultStackSize / sizeof(m_Stack[0]);
    m_Stack.resize( stack_size );
    uint64_t stack_end = reinterpret_cast<uint64_t>(&m_Stack[stack_size]);


    memset( &m_Context, 0, sizeof(m_Context) );
    m_Context.rip = reinterpret_cast<uint64_t>(f);
    m_Context.rdi = m_ID;
    m_Context.rsi = data;

    m_Context.cr3 = GetCR3();
    m_Context.rflags = 0x202;
    m_Context.cs = k_KernelCS;
    m_Context.ss = k_KernelSS;
    m_Context.rsp = (stack_end & ~0xFlu) - 8;

    // MXCSR のすべての例外をマスクする
    *reinterpret_cast<uint32_t*>(&m_Context.fxsave_area[24]) = 0x1F80;

    return *this;
}

TaskContext& Task::Context()
{
    return m_Context;
}

uint64_t Task::ID() const
{
    return m_ID;
}

Task& Task::Sleep()
{
    TaskManager::Instance().Sleep( this );
    return *this;
}

Task& Task::Wakeup()
{
    TaskManager::Instance().Wakeup( this );
    return *this;
}

void Task::SendMessage( const Message& msg )
{
    m_MsgQueue.push_back( msg );
    Wakeup();
}

std::optional<Message> Task::ReceiveMessage()
{
    if( m_MsgQueue.empty() ){
        return std::nullopt;
    }

    auto m = m_MsgQueue.front();
    m_MsgQueue.pop_front();

    return m;
}

TaskManager::TaskManager()
    : m_Tasks(),
      m_LatestID( 0 ),
      m_Running()

{
    m_Running.push_back( &NewTask() );
}

TaskManager& TaskManager::Instance()
{
    if( s_TaskManager == nullptr ){
        s_TaskManager = new TaskManager();
    }

    return *s_TaskManager;
}

Task& TaskManager::CurrentTask()
{
    return *m_Running.front();
}

Task& TaskManager::NewTask()
{
    ++m_LatestID;
    return *(m_Tasks.emplace_back(new Task(m_LatestID)));
}

void TaskManager::SwitchTask( bool current_sleep )
{
    Task* current_task = m_Running.front();
    m_Running.pop_front();

    if( !current_sleep ){
        m_Running.push_back( current_task );
    }

    Task* next_task = m_Running.front();

    SwitchContext( &next_task->Context(), &current_task->Context() );
}

void TaskManager::Sleep( Task* task )
{
    auto itr = std::find( m_Running.begin(), m_Running.end(), task );

    // 実行中ならタスクスイッチしてsleep
    if( itr == m_Running.begin() ){
        SwitchTask( true );
        return;
    }

    if( itr == m_Running.end() ){
        return;
    }

    m_Running.erase( itr );
}

Error TaskManager::Sleep( uint64_t id )
{
    auto itr = std::find_if( m_Tasks.begin(), m_Tasks.end(),
                             [id]( const auto& t ) { return t->ID() == id; });
    if( itr == m_Tasks.end() ){
        return MAKE_ERROR( Error::kNoSuchTask );
    }

    Sleep( itr->get() );
    return MAKE_ERROR( Error::kSuccess );
}

void TaskManager::Wakeup( Task* task )
{
    auto itr = std::find( m_Running.begin(), m_Running.end(), task );
    if( itr == m_Running.end() ){
        m_Running.push_back( task );
    }
}

Error TaskManager::Wakeup( uint64_t id )
{
    auto itr = std::find_if( m_Tasks.begin(), m_Tasks.end(),
                             [id]( const auto& t ) { return t->ID() == id; } );
    
    if( itr == m_Tasks.end() ){
        return MAKE_ERROR( Error::kNoSuchTask );
    }

    Wakeup( itr->get() );
    return MAKE_ERROR( Error::kSuccess );
}

Error TaskManager::SendMessage( uint64_t id, const Message& msg )
{
    auto itr = std::find_if( m_Tasks.begin(), m_Tasks.end(), 
                             [id]( const auto& t ) { return t->ID() == id; } );
    
    if( itr == m_Tasks.end() ){
        return MAKE_ERROR( Error::kNoSuchTask );
    }

    (*itr)->SendMessage( msg );
    return MAKE_ERROR( Error::kSuccess );
}

void InitializeTask()
{
    __asm__("cli");
    TimerManager::Instance().AddTimer( 
        Timer( k_TaskTimerPeriod, k_TaskTimerValue )
    );
    __asm__("sti");
}
