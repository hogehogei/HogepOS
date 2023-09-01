#include "Terminal.hpp"
#include "Global.hpp"
#include "Graphic.hpp"
#include "Task.hpp"

Terminal::Terminal()
    : m_Window(),
      m_LayerID(0),
      m_Cursor{0, 0},
      m_IsCursorVisible(false)
{
    m_Window = std::make_shared<TopLevelWindow>(
        k_Columns * 8 + 8 + TopLevelWindow::k_MarginX,
        k_Rows * 16 + 8 + TopLevelWindow::k_MarginY,
        g_MainScreen.Config().PixelFormat,
        "Terminal"
    );
    DrawTerminal( *(m_Window->InnerWriter()), {0, 0}, m_Window->InnerSize() );
    m_LayerID = g_LayerManager->NewLayer()
        .SetWindow(m_Window)
        .SetDraggable(true)
        .ID();
}

void Terminal::DrawCursor()
{
    const auto color = 0xFFFFFF;
    const auto pos = Vector2<int>{4 + 8*m_Cursor.x, 5 + 16*m_Cursor.y};
    FillRectAngle(*(m_Window->InnerWriter()), pos, {7, 15}, ToColor(color));
}

void TaskTerminal( uint64_t task_id, int64_t data )
{
    __asm__("cli");
    Task& task = TaskManager::Instance().CurrentTask();
    Terminal* term = new Terminal;
    g_LayerManager->Move(term->GetLayerID(), {100, 200});
    g_ActiveLayer->Activate(term->GetLayerID());
    __asm__("sti");

    while(1) {
        __asm__("cli");
        task.Sleep();
        __asm__("sti");
    }
}