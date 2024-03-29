//
// include files
//
#include "Layer.hpp"
#include "Global.hpp"
#include "Graphic.hpp"
#include "MouseCursor.hpp"

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

    //! @brief 指定されたIDを持つレイヤーを生成する
Layer::Layer( LayerID id )
    : m_ID(id),
      m_Pos{0, 0},
      m_Draggable( false ),
      m_Window()
{}

LayerID Layer::ID() const
{
    return m_ID;
}


Layer& Layer::SetDraggable( bool draggable )
{
    m_Draggable = draggable;
    return *this;
}

bool Layer::IsDraggable() const
{
    return m_Draggable;
}

Layer& Layer::SetWindow( const std::shared_ptr<Window>& window )
{
    m_Window = window;
    return *this;
}

std::shared_ptr<Window> Layer::GetWindow() const
{
    return m_Window;
}

Layer& Layer::Move( Vector2<int> pos )
{
    m_Pos = pos;
    return *this;
}

Layer& Layer::MoveRelative( Vector2<int> pos_diff )
{
    m_Pos += pos_diff;
    return *this;
}

void Layer::DrawTo( FrameBuffer& fb, const RectAngle<int>& area) const
{
    if( m_Window ){
        m_Window->DrawTo( fb, m_Pos, area );
    }
}

Vector2<int> Layer::GetPosition() const
{
    return m_Pos;
}

LayerManager::LayerManager()
    : m_Screen( nullptr ),
      m_BackBuffer(),
      m_Layers(),
      m_LayerStack(),
      m_LatestID( 0 )
{}

void LayerManager::SetWriter( FrameBuffer* screen )
{
    m_Screen = screen;

    FrameBufferConfig back_config = screen->Config();
    back_config.FrameBuffer = nullptr;
    m_BackBuffer.Initialize( back_config );
}

Layer& LayerManager::NewLayer()
{
    ++m_LatestID;
    return *(m_Layers.emplace_back( std::make_unique<Layer>(m_LatestID) ));
}

void LayerManager::Draw( const RectAngle<int>& area ) const
{
    for( auto layer : m_LayerStack ){
        layer->DrawTo( m_BackBuffer, area );
    }
    m_Screen->Copy( area.pos, m_BackBuffer, area );
}

void LayerManager::Draw( unsigned int id) const
{
    Draw( id, {{0, 0}, {-1, -1}} );
}

void LayerManager::Draw( unsigned int id, RectAngle<int> area ) const
{
    bool draw = false;
    RectAngle<int> window_area;

    for( auto layer : m_LayerStack ){
        if( layer->ID() == id ){
            window_area.size = layer->GetWindow()->Size();
            window_area.pos  = layer->GetPosition();
            if( area.size.x >= 0 || area.size.y >= 0 ){
                area.pos = area.pos + window_area.pos;
                window_area = window_area.Intersection( area );
            }
            draw = true;
        }

        if( draw ){
            layer->DrawTo( m_BackBuffer, window_area );
        }
    }

    m_Screen->Copy( window_area.pos, m_BackBuffer, window_area );
}

void LayerManager::Move( LayerID id, Vector2<int> new_pos )
{
    auto layer = FindLayer(id);
    const auto window_size = layer->GetWindow()->Size();
    const auto old_pos = layer->GetPosition();
    layer->Move( new_pos );
    
    Draw( {old_pos, window_size} );
    Draw( id );
}

void LayerManager::MoveRelative( LayerID id, Vector2<int> pos_diff )
{
    auto layer = FindLayer(id);
    const auto window_size = layer->GetWindow()->Size();
    const auto old_pos = layer->GetPosition();

    layer->MoveRelative( pos_diff );
    Draw( {old_pos, window_size} );
    Draw( id );
}

void LayerManager::UpDown( LayerID id, int new_height )
{
    if( new_height < 0 ){
        Hide( id );
        return;
    }
    if( new_height > m_LayerStack.size() ){
        new_height = m_LayerStack.size();
    }

    auto layer   = FindLayer(id);
    auto old_pos = std::find( m_LayerStack.begin(), m_LayerStack.end(), layer );
    auto new_pos = m_LayerStack.begin() + new_height;

    // 非表示の場合、新しい位置にレイヤを挿入して終了
    if( old_pos == m_LayerStack.end() ){
        m_LayerStack.insert( new_pos, layer );
        return;
    }

    // 一番最後を指している場合、eraseすると end() の向こう側に行ってしまうので一つ戻す
    if( new_pos == m_LayerStack.end() ){
        --new_pos;
    }

    m_LayerStack.erase( old_pos );
    m_LayerStack.insert( new_pos, layer );
}

void LayerManager::Hide( LayerID id )
{
    auto layer = FindLayer(id);
    auto pos   = std::find( m_LayerStack.begin(), m_LayerStack.end(), layer );
    if( pos != m_LayerStack.end() ){
        m_LayerStack.erase(pos);
    }
}

Layer* LayerManager::FindLayerByPosition( Vector2<int> pos, unsigned int exclude_id ) const
{
    auto pred = [pos, exclude_id]( Layer* layer ){
        if( layer->ID() == exclude_id ){
            return false;
        }
        const auto& window = layer->GetWindow();
        if( !window ){
            return false;
        }

        const auto window_pos = layer->GetPosition();
        const auto window_end_pos = window_pos + window->Size();

        return window_pos.x <= pos.x && pos.x < window_end_pos.x &&
               window_pos.y <= pos.y && pos.y < window_end_pos.y;
    };

    auto itr = std::find_if( m_LayerStack.rbegin(), m_LayerStack.rend(), pred );
    if( itr == m_LayerStack.rend() ){
        return nullptr;
    }

    return *itr;
}

Layer* LayerManager::FindLayer( LayerID id )
{
    auto pred = [id]( const LayerPtr& elem ){
        return elem->ID() == id;
    };

    auto itr = std::find_if( m_Layers.begin(), m_Layers.end(), pred );
    if( itr == m_Layers.end() ){
        return nullptr;
    }

    return itr->get();
}

int LayerManager::GetHeight( unsigned int id )
{
    for( int h = 0; h < m_LayerStack.size(); ++h ){
        if( m_LayerStack[h]->ID() == id ){
            return h;
        }
    }
    return -1;
}

ActiveLayer::ActiveLayer( LayerManager& manager )
    : m_Manager(manager),
      m_ActiveLayer(0),
      m_MouseLayer(0)
{}

void ActiveLayer::SetMouseLayer( LayerID mouse_layer )
{
    m_MouseLayer = mouse_layer;
}

void ActiveLayer::Activate( LayerID layer_id )
{
    if( m_ActiveLayer == layer_id ){
        return;
    }

    if( m_ActiveLayer > 0 ){
        Layer* layer = g_LayerManager->FindLayer( m_ActiveLayer );
        layer->GetWindow()->Deactivate();
        g_LayerManager->Draw( m_ActiveLayer );
    }

    m_ActiveLayer = layer_id;
    if( m_ActiveLayer > 0 ){
        Layer* layer = g_LayerManager->FindLayer( m_ActiveLayer );
        layer->GetWindow()->Activate();
        // マウスの一個下のレイヤに表示
        g_LayerManager->UpDown( m_ActiveLayer, g_LayerManager->GetHeight(m_MouseLayer) - 1 );
        g_LayerManager->Draw( m_ActiveLayer );
    }
}


void CreateLayer( const FrameBufferConfig& config, FrameBuffer* screen )
{
    const int k_FrameWidth = config.HorizontalResolution;
    const int k_FrameHeight = config.VerticalResolution;

    auto bg_window = std::make_shared<Window>( k_FrameWidth, k_FrameHeight, config.PixelFormat );
    auto bg_writer = bg_window->Writer();

    DrawDesktop( *bg_writer );

    auto mouse_window = std::make_shared<Window>( k_MouseCursorWidth, k_MouseCursorHeight, config.PixelFormat );
    mouse_window->SetTransparentColor( k_MouseTransparentColor );
    DrawMouseCursor( *mouse_window->Writer(), {0, 0} );

    g_MainWindow = std::make_shared<Window>(
        160, 52, config.PixelFormat
    );
    DrawWindow( *g_MainWindow->Writer(), "Hello window" );

    auto console_window = std::make_shared<Window>(
        Console::sk_Columns * 8, Console::sk_Rows * 16, config.PixelFormat
    );
    g_Console->SetWindow( console_window );

    // Create text box
    g_TextBoxWindow = std::make_shared<TopLevelWindow>(
        8*20,  100, config.PixelFormat, "TextBox"
    );
    DrawTextBox( *g_TextBoxWindow->InnerWriter(), {0, 0}, g_TextBoxWindow->InnerSize() );

    g_LayerManager = new LayerManager();
    g_LayerManager->SetWriter( screen );

    g_ActiveLayer = new ActiveLayer(*g_LayerManager);

    auto bglayer_id = g_LayerManager->NewLayer()
        .SetWindow( bg_window )
        .Move( {0, 0} )
        .ID();
    auto mouse_layer_id = g_LayerManager->NewLayer()
        .SetWindow( mouse_window )
        .Move( g_MousePosition )
        .ID();
    auto main_window_layer_id = g_LayerManager->NewLayer()
        .SetWindow( g_MainWindow )
        .SetDraggable( true )
        .Move( {300, 100} )
        .ID();
    g_Console->SetLayerID( g_LayerManager->NewLayer()
        .SetWindow( console_window )
        .Move( {0, 0} )
        .ID()
    );
    g_TextBoxWindowID = g_LayerManager->NewLayer()
        .SetWindow( g_TextBoxWindow )
        .SetDraggable( true )
        .Move( {500, 100} )
        .ID();

    g_LayerManager->UpDown( bglayer_id, 0 );
    g_LayerManager->UpDown( g_Console->LayerID(), 1 );
    g_LayerManager->UpDown( main_window_layer_id, 2 );
    g_LayerManager->UpDown( g_TextBoxWindowID, 3 );
    g_LayerManager->UpDown( mouse_layer_id, 4 );
    g_LayerManager->Draw( bglayer_id );

    g_MouseLayerID = mouse_layer_id;
    g_MainWindowLayerID = main_window_layer_id;
}

void ProcessLayerMessage( const Message& msg )
{
    const auto arg = msg.Arg.Layer;
    switch(arg.op){
    case LayerOperation::Move:
        g_LayerManager->Move( arg.LayerId, {arg.x, arg.y} );
        break;
    case LayerOperation::MoveRelative:
        g_LayerManager->MoveRelative( arg.LayerId, {arg.x, arg.y} );
        break;
    case LayerOperation::Draw:
        g_LayerManager->Draw( arg.LayerId );
        break;
    case LayerOperation::DrawArea:
        g_LayerManager->Draw( arg.LayerId, {{arg.x, arg.y}, {arg.w, arg.h}} );
        break;
    default:
        break;
    }
}