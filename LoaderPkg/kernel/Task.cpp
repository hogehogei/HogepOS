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
namespace
{
    template <typename T, typename U>
    void Erase( T& c, const U& value )
    {
        auto itr = std::remove( c.begin(), c.end(), value );
        c.erase( itr, c.end() );
    }

    void TaskIdle( uint64_t id, int64_t data )
    {
        while( 1 ){
            __asm__( "hlt" );
        }
    }
}

Task::Task( uint64_t id )
    : m_ID( id ),
      m_Stack(),
      m_Context(),
      m_MsgQueue(),
      m_Level( k_DefaultLevel ),
      m_Running( false )
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

bool Task::Running() const
{
    return m_Running;
}

unsigned int Task::Level() const
{
    return m_Level;
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

Task& Task::SetLevel( int level )
{
    m_Level = level;
    return *this;
}

Task& Task::SetRunning( bool running )
{
    m_Running = running;
    return *this;
}


TaskManager::TaskManager()
    : m_Tasks(),
      m_LatestID( 0 ),
      m_Running(),
      m_CurrentLevel( k_MaxLevel ),
      m_LevelChanged( false )

{
    Task& task = NewTask()
        .SetLevel( m_CurrentLevel )
        .SetRunning( true );
    m_Running[m_CurrentLevel].push_back( &task );

    Task& idle = NewTask()
        .InitContext( TaskIdle, 0 )
        .SetLevel( 0 )
        .SetRunning( true );
    m_Running[0].push_back( &idle );
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
    return *(m_Running[m_CurrentLevel].front());
}

Task& TaskManager::NewTask()
{
    ++m_LatestID;
    return *(m_Tasks.emplace_back(new Task(m_LatestID)));
}

void TaskManager::SwitchTask( bool current_sleep )
{
    auto& level_queue = m_Running[m_CurrentLevel];
    Task* current_task = level_queue.front();
    level_queue.pop_front();

    if( !current_sleep ){
        level_queue.push_back( current_task );
    }
    if( level_queue.empty() ){
        m_LevelChanged = true;
    }

    // 現在レベルのランキューが空なら、最優先順にランキューを調べ
    // 実行待ちのタスクがあるランレベルに設定
    if( m_LevelChanged ){
        m_LevelChanged = false;

        for( int lv = k_MaxLevel; lv >= 0; --lv ){
            if( !m_Running[lv].empty() ){
                m_CurrentLevel = lv;
                break;
            }
        }
    }

    Task* next_task = m_Running[m_CurrentLevel].front();

    SwitchContext( &next_task->Context(), &current_task->Context() );
}

void TaskManager::Sleep( Task* task )
{
    if( !task->Running() ){
        return;
    }

    task->SetRunning( false );

    if( task == m_Running[m_CurrentLevel].front() ){
        SwitchTask( true );
        return;
    }

    Erase( m_Running[task->Level()], task );
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

void TaskManager::Wakeup( Task* task, int level )
{
    if( task->Running() ){
        ChangeLevelRunning( task, level );
        return;
    }

    if( level < 0 ){
        level = task->Level();
    }

    task->SetLevel( level );
    task->SetRunning( true );

    m_Running[level].push_back( task );
    if( level > m_CurrentLevel ){
        m_LevelChanged = true;
    }
}

Error TaskManager::Wakeup( uint64_t id, int level )
{
    auto itr = std::find_if( m_Tasks.begin(), m_Tasks.end(),
                             [id]( const auto& t ) { return t->ID() == id; } );
    
    if( itr == m_Tasks.end() ){
        return MAKE_ERROR( Error::kNoSuchTask );
    }

    Wakeup( itr->get(), level );
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

void TaskManager::ChangeLevelRunning( Task* task, int level )
{
    if( level < 0 || level == task->Level() ){
        return;
    }

    if( task != m_Running[m_CurrentLevel].front() ){
        // 現在実行中でなければ、違うランレベルに変更
        Erase( m_Running[task->Level()], task );
        m_Running[level].push_back( task );
        task->SetLevel( level );
        if( level > m_CurrentLevel ){
            m_LevelChanged = true;
        }

        return;
    }

    // 現在実行中なら、対象のランレベルキューの先頭に追加
    // m_CurrentLevel のランキューの先頭が現在実行中のタスクと認識するので
    m_Running[m_CurrentLevel].pop_front();
    m_Running[level].push_front( task );
    task->SetLevel( level );

    if( level < m_CurrentLevel ){
        m_LevelChanged = true;
    }
    m_CurrentLevel = level;
}

void InitializeTask()
{
    __asm__("cli");
    TimerManager::Instance().AddTimer( 
        Timer( k_TaskTimerPeriod, k_TaskTimerValue )
    );
    __asm__("sti");
}
