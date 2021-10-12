#pragma once 

#include <cstdint>

#include "Keyboard.hpp"

struct Message
{
    enum EventType {
        k_InterruptXHCI,
        k_InterruptLAPICTimer,
        k_TimerTimeout,
        k_KeyPush,
    } Type;

    union {
        struct {
            int Value;
        } Timer;
        
        struct {
            Keyboard::Key Key;
        } Keyboard;
        
    } Arg;

    Message() = default;
    ~Message() = default;
    
    Message( EventType value )
     : Type( value ) {}
};