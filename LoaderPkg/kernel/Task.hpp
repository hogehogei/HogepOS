#pragma once

// 
// include headers
//
#include <cstdint>
#include <array>
#include <vector>
#include <deque>
#include <optional>

#include "error.hpp"
#include "Event.hpp"

struct TaskContext
{
    uint64_t cr3, rip, rflags, reserved1;       // offset 0x00
    uint64_t cs, ss, fs, gs;                    // offset 0x20
    uint64_t rax, rbx, rcx, rdx, rdi, rsi, rsp, rbp;    // offset 0x40
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;      // offset 0x80
    std::array<uint8_t, 512> fxsave_area;       // offset 0xc0
} __attribute__((packed));

using TaskFunc = void( uint64_t, int64_t );

class TaskManager;
class Task
{
public:
    static constexpr int k_DefaultLevel = 1;
    static constexpr size_t k_DefaultStackSize = 4096;

    Task( uint64_t id );
    Task& InitContext( TaskFunc* f, int64_t data );
    TaskContext& Context();

    uint64_t ID() const;
    bool Running() const;
    unsigned int Level() const;
    Task& Sleep();
    Task& Wakeup();

    void SendMessage( const Message& msg );
    std::optional<Message> ReceiveMessage();

private:

    friend TaskManager;
    Task& SetLevel( int level );
    Task& SetRunning( bool running );

    uint64_t m_ID;
    std::vector<uint64_t>   m_Stack;
    alignas(16) TaskContext m_Context;
    std::deque<Message>     m_MsgQueue;

    unsigned int            m_Level;
    bool                    m_Running;
};

class TaskManager
{
public:

    static constexpr int k_MaxLevel = 3;
    static constexpr uint64_t k_MainTaskID = 1;
public:
    
    TaskManager( TaskManager& ) = delete;
    TaskManager& operator=( TaskManager& ) = delete;

    static TaskManager& Instance();

    Task& NewTask();
    Task& CurrentTask();
    void SwitchTask( bool current_sleep = false );

    void Sleep( Task* task );
    Error Sleep( uint64_t id );

    void Wakeup( Task* task, int level = -1 );
    Error Wakeup( uint64_t id, int level = -1 );

    Error SendMessage( uint64_t id, const Message& msg );

private:

    TaskManager();

    void ChangeLevelRunning( Task* task, int level );

    static TaskManager* s_TaskManager;
    
    std::vector<std::unique_ptr<Task>> m_Tasks;
    uint64_t m_LatestID;
    std::array<std::deque<Task*>, k_MaxLevel + 1> m_Running;

    unsigned int m_CurrentLevel;
    bool m_LevelChanged;
};

void InitializeTask();