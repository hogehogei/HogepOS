#include "Terminal.hpp"
#include "Global.hpp"
#include "Setting.hpp"
#include "Graphic.hpp"
#include "Font.hpp"
#include "Task.hpp"

TerminalMessageDispacher& TerminalMessageDispacher::Instance()
{
    static TerminalMessageDispacher s_Instance;
    return s_Instance;
}

void TerminalMessageDispacher::Register( LayerID layer_id, uint64_t task_id )
{
    m_LayerTaskMap.insert( std::make_pair(layer_id, task_id) );
}

void TerminalMessageDispacher::Unregister( LayerID layer_id )
{
    __asm__("cli");
    auto itr = m_LayerTaskMap.find( layer_id );
    __asm__("sti");
    if( itr != m_LayerTaskMap.end() ){
        __asm__("cli");
        m_LayerTaskMap.erase( itr );
        __asm__("sti");
    }
}

void TerminalMessageDispacher::Dispatch( LayerID layer_id, const Message& msg )
{
    __asm__("cli");
    auto itr = m_LayerTaskMap.find( layer_id );
    __asm__("sti");
    if( itr != m_LayerTaskMap.end() ){
        __asm__("cli");
        TaskManager::Instance().SendMessage( itr->second, msg );
        __asm__("sti");
    }
}


Terminal::Terminal()
    : m_Window(),
      m_LayerID(0),
      m_Cursor{0, 0},
      m_IsCursorVisible(false),
      m_LineBuf(),
      m_LineBufIndex(0)
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
    m_TaskID = TaskManager::Instance().
        NewTask().
        InitContext( TaskTerminal, reinterpret_cast<uint64_t>(this) ).
        Wakeup().
        ID();

    TerminalMessageDispacher::Instance().Register( m_LayerID, m_TaskID );
}

Terminal::~Terminal()
{
    TerminalMessageDispacher::Instance().Unregister( m_LayerID );
}

void Terminal::DrawCursor( bool is_visible )
{
    const auto color = is_visible ? k_TermCursorColor : k_TermBackColor;
    const auto pos = Vector2<int>{4 + 8*m_Cursor.x, 5 + 16*m_Cursor.y};
    FillRectAngle(*(m_Window->InnerWriter()), pos, {7, 15}, color);
}

RectAngle<int> Terminal::InputKey(
    uint8_t modifier, uint8_t keycode, char ascii
)
{
    DrawCursor(false);

    RectAngle<int> draw_area{ CalcCursorPos(), {8*2, 16} };

    // 改行
    if( ascii == '\n' ){
        m_LineBuf[m_LineBufIndex] = 0;
        m_Cursor.x = 0;
        m_LineBufIndex = 0;
        if( m_Cursor.y < (k_Rows - 1) ){
            ++m_Cursor.y;
        }
        else {
            ScrollLine();
        }
        draw_area.pos  = TopLevelWindow::k_TopLeftMargin;
        draw_area.size = m_Window->InnerSize(); 
    }
    else if( ascii == '\b' ){
        if( m_Cursor.x > 0 ){
            --m_Cursor.x;
            FillRectAngle(*(m_Window->Writer()), CalcCursorPos(), {8, 16}, k_TermBackColor );
            // カーソル位置再計算
            draw_area.pos = CalcCursorPos();
            if( m_LineBufIndex > 0 ){
                --m_LineBufIndex;
            }
        }
    }
    else if( ascii != 0 ){
        if( m_Cursor.x < (k_Columns - 1) ){
            m_LineBuf[m_LineBufIndex] = ascii;
            ++m_LineBufIndex;
            auto pos = CalcCursorPos();
            WriteAscii(*(m_Window->Writer()), pos.x, pos.y, ascii, k_TermCursorColor );
            ++m_Cursor.x;
        }
    }

    DrawCursor(true);

    return draw_area;
}

Vector2<int> Terminal::CalcCursorPos()
{
    return TopLevelWindow::k_TopLeftMargin + Vector2<int>{ 4 + 8*m_Cursor.x, 4 + 16*m_Cursor.y };
}

void Terminal::ScrollLine()
{
    RectAngle<int> move_src{
        TopLevelWindow::k_TopLeftMargin + Vector2<int>{4, 4 + 16},
        {8 * k_Columns, 16 * (k_Rows - 1)}
    };
    
    m_Window->Move( TopLevelWindow::k_TopLeftMargin + Vector2<int>{4, 4}, move_src );
    FillRectAngle(*(m_Window->InnerWriter()), {4, 4+(16*m_Cursor.y)}, {8*k_Columns, 16}, k_TermBackColor);
}

void TaskTerminal( uint64_t task_id, int64_t data )
{
    __asm__("cli");
    Task& task = TaskManager::Instance().CurrentTask();
    Terminal* term = reinterpret_cast<Terminal*>(data);
    g_LayerManager->Move(term->GetLayerID(), {100, 200});
    g_ActiveLayer->Activate(term->GetLayerID());
    __asm__("sti");


    while(1){
        __asm__("cli");

        auto msg = task.ReceiveMessage();
        if( !msg ){
            task.Sleep();
            __asm__("sti");
            continue;
        }
        __asm__("sti");

        switch( msg->Type ){
        case Message::k_KeyPush:
        {
            const auto area = term->InputKey( msg->Arg.Keyboard.Key.Modifier(), 
                                              msg->Arg.Keyboard.Key.KeyCode(),
                                              msg->Arg.Keyboard.Key.Ascii() );
            
            Message msg = MakeLayerMessage(
                task.ID(), term->GetLayerID(), LayerOperation::DrawArea, area
            );
            __asm__("cli");
            TaskManager::Instance().SendMessage( 1, msg );
            __asm__("sti");
        }
            break;
        default:
            break;
        }
    }
  
}
