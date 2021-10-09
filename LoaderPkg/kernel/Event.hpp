#pragma once 

struct Message
{
    enum EventType {
        k_InterruptXHCI,
        k_InterruptLAPICTimer,
        k_TimerTimeout,
    } Type;

    union {
        struct {
            int Value;
        } Timer;
    } Arg;

    Message() = default;
    ~Message() = default;
    
    Message( EventType value )
     : Type( value ) {}
};