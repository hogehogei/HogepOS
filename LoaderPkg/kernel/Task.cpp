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

TaskManager::TaskManager()
    : m_Tasks(),
      m_LatestID( 0 ),
      m_CurrentTaskIndex( 0 )

{
    NewTask();
}

TaskManager& TaskManager::Instance()
{
    if( s_TaskManager == nullptr ){
        s_TaskManager = new TaskManager();
    }

    return *s_TaskManager;
}

Task& TaskManager::NewTask()
{
    ++m_LatestID;
    return *(m_Tasks.emplace_back(new Task(m_LatestID)));
}

void TaskManager::SwitchTask()
{
    size_t next_task_index = (m_CurrentTaskIndex + 1) % m_Tasks.size();

    Task& current_task = *m_Tasks[m_CurrentTaskIndex];
    Task& next_task    = *m_Tasks[next_task_index];
    m_CurrentTaskIndex = next_task_index;

    SwitchContext( &next_task.Context(), &current_task.Context() );
}

void InitializeTask()
{
    __asm__("cli");
    TimerManager::Instance().AddTimer( 
        Timer( k_TaskTimerPeriod, k_TaskTimerValue )
    );
    __asm__("sti");
}
