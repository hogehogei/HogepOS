#pragma once

#include <cstdint>
#include <memory>
#include "Window.hpp"
#include "Type.hpp"

class Terminal
{
public:

    static constexpr int k_Rows = 15;
    static constexpr int k_Columns = 60;

    Terminal();
    LayerID GetLayerID() const { return m_LayerID; }

private:

    std::shared_ptr<TopLevelWindow> m_Window;
    LayerID m_LayerID;

    Vector2<int> m_Cursor;
    bool m_IsCursorVisible;

    void DrawCursor();
};

void TaskTerminal( uint64_t task_id, int64_t data );
