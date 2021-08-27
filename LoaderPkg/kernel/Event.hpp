#pragma once 

struct Message
{
    enum Type {
        k_InterruptXHCI,
    } type;

    Message( Message::Type value )
     : type(value) {}
};