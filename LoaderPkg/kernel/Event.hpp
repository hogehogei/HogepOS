#pragma once 

#include <cstdint>
#include "Keyboard.hpp"

enum LayerOperation {
    Move, 
    MoveRelative,
    Draw,
    DrawArea,
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
            LayerID LayerId;
            int x, y;
            int w, h;
        } Layer;
    } Arg;

    Message() = default;
    ~Message() = default;
};
