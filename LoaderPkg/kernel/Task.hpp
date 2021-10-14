#pragma once

// 
// include headers
//
#include <cstdint>
#include <array>
#include <vector>

struct TaskContext
{
    uint64_t cr3, rip, rflags, reserved1;       // offset 0x00
    uint64_t cs, ss, fs, gs;                    // offset 0x20
    uint64_t rax, rbx, rcx, rdx, rdi, rsi, rsp, rbp;    // offset 0x40
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;      // offset 0x80
    std::array<uint8_t, 512> fxsave_area;       // offset 0xc0
} __attribute__((packed));

using TaskFunc = void( uint64_t, int64_t );

class Task
{
public:

    static constexpr size_t k_DefaultStackSize = 4096;

    Task( uint64_t id );
    Task& InitContext( TaskFunc* f, int64_t data );
    TaskContext& Context();

private:

    uint64_t m_ID;
    std::vector<uint64_t> m_Stack;
    alignas(16) TaskContext m_Context;
};

class TaskManager
{
public:
    
    TaskManager( TaskManager& ) = delete;
    TaskManager& operator=( TaskManager& ) = delete;

    static TaskManager& Instance();

    Task& NewTask();
    void SwitchTask();

private:

    TaskManager();

    static TaskManager* s_TaskManager;
    
    std::vector<std::unique_ptr<Task>> m_Tasks;
    uint64_t m_LatestID;
    size_t m_CurrentTaskIndex;
};

void InitializeTask();