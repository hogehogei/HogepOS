
//
// include files
//
#include "MouseCursor.hpp"
#include "Global.hpp"

//
// constant
//


//
// static variables
//

//
// static function declaration
// 

//
// funcion definitions
// 

MouseCursor::MouseCursor( IPixelWriter* writer, PixelColor back_color, Vector2<int> initial_position )
    : m_PixelWriter( writer ),
      m_BackColor( back_color ),
      m_Position( initial_position )
{
    DrawMouseCursor( *m_PixelWriter, m_Position );
}

void MouseCursor::MoveRelative( Vector2<int> displacement )
{
    EraseMouseCursor( *m_PixelWriter, m_Position, m_BackColor );
    m_Position += displacement;
    DrawMouseCursor( *m_PixelWriter, m_Position );
}

void MouseObserver( uint8_t buttons, int8_t displacement_x, int8_t displacement_y )
{
    static unsigned int s_MouseDragLayerID = 0;
    static uint8_t s_PreviousButtons = 0;

    const auto oldpos = g_MousePosition;

    auto newpos = g_MousePosition + Vector2<int>( displacement_x, displacement_y );
    newpos = ElementMin( newpos, g_ScreenSize + Vector2<int>(-1, -1) );
    g_MousePosition = ElementMax( newpos, Vector2<int>(0, 0) );

    const auto posdiff = g_MousePosition - oldpos;

    g_LayerManager->Move( g_MouseLayerID, g_MousePosition );

    const bool previous_left_pressed = (s_PreviousButtons & 0x01);
    const bool left_pressed = (buttons & 0x01);

    if( !previous_left_pressed && left_pressed ){
        auto layer = g_LayerManager->FindLayerByPosition( g_MousePosition, g_MouseLayerID );
        if( layer && layer->IsDraggable() ){
            s_MouseDragLayerID = layer->ID();
        }
    }
    else if( previous_left_pressed && left_pressed ){
        if( s_MouseDragLayerID > 0 ){
            g_LayerManager->MoveRelative( s_MouseDragLayerID, posdiff );
        }
    }
    else if( previous_left_pressed && !left_pressed ){
        s_MouseDragLayerID = 0;
    }

    s_PreviousButtons = buttons;
}