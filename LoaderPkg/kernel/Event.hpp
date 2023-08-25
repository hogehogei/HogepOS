#pragma once 

#include <cstdint>

#include "Keyboard.hpp"


enum LayerOperation {
    Move, 
    MoveRelative,
    Draw,
};

struct Message
{
    enum EventType {
        k_InterruptXHCI,
        k_InterruptLAPICTimer,
        k_TimerTimeout,
        k_KeyPush,
        k_Layer,
        k_LayerFinish,
    } Type;

    uint64_t SrcTask;

    union {
        struct {
            int Value;
        } Timer;
        
        struct {
            Keyboard::Key Key;
        } Keyboard;
        
        struct {
            LayerOperation op;
            uint32_t LayerID;
            int x, y;
        } Layer;
    } Arg;

    Message() = default;
    ~Message() = default;
    
    Message( EventType type, uint64_t src_task )
     : Type(type), SrcTask(src_task) {}
};
