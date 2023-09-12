#pragma once

#include <cstdint>
#include <memory>
#include <map>
#include "Window.hpp"
#include "Type.hpp"
#include "Event.hpp"

class TerminalMessageDispacher
{
public:

    TerminalMessageDispacher() = default;
    ~TerminalMessageDispacher() = default;
    TerminalMessageDispacher( TerminalMessageDispacher& ) = delete;
    TerminalMessageDispacher& operator=( TerminalMessageDispacher& ) = delete;

    static TerminalMessageDispacher& Instance();

    void Register( LayerID layer_id, uint64_t task_id );
    void Unregister( LayerID layer_id );
    void Dispatch( LayerID layer_id, const Message& msg );

private:

    std::map<LayerID, uint64_t> m_LayerTaskMap;
};

class Terminal
{
public:

    static constexpr int k_Rows = 15;
    static constexpr int k_Columns = 60;
    static constexpr int k_LineMax = 128;

    Terminal();
    ~Terminal();
    LayerID GetLayerID() const { return m_LayerID; }
    RectAngle<int> InputKey( uint8_t modifier, uint8_t keycode, char ascii );

    void Print( const char* s );
private:

    void DrawCursor( bool is_visible );
    Vector2<int> CalcCursorPos();
    void ScrollLine();
    void ExecuteLine();

    std::shared_ptr<TopLevelWindow> m_Window;
    LayerID m_LayerID;

    Vector2<int> m_Cursor;
    bool m_IsCursorVisible;
    std::array<char, k_LineMax> m_LineBuf;
    int m_LineBufIndex;

    uint64_t m_TaskID;
};

void TaskTerminal( uint64_t task_id, int64_t data );
