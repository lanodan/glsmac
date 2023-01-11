#include "UI.h"

#include "engine/Engine.h"

using namespace scene;
using namespace types;

namespace ui {

using namespace object;

void UI::Start() {
	Log( "Creating UI" );

	m_shape_scene = new Scene( "UIShape", SCENE_TYPE_ORTHO );
	g_engine->GetRenderer()->AddScene( m_shape_scene );

	m_text_scene = new Scene( "UIText", SCENE_TYPE_TEXT );
	g_engine->GetRenderer()->AddScene( m_text_scene );

	m_root_object.Create();
	m_root_object.UpdateObjectArea();

	m_clamp.x.SetRange( 0.0, g_engine->GetRenderer()->GetWindowWidth(), -1.0, 1.0 );
	m_clamp.y.SetRange( 0.0, g_engine->GetRenderer()->GetWindowHeight(), -1.0, 1.0 );
	m_clamp.y.SetInversed( true );
	
	m_debug_scene = new Scene( "UIDebug", SCENE_TYPE_ORTHO );
	g_engine->GetRenderer()->AddScene( m_debug_scene );
}

void UI::Stop() {
	Log( "Destroying UI" );

	g_engine->GetRenderer()->RemoveScene( m_debug_scene );
	delete m_debug_scene;
	
	m_root_object.Destroy();

	g_engine->GetRenderer()->RemoveScene( m_text_scene );
	delete m_text_scene;

	g_engine->GetRenderer()->RemoveScene( m_shape_scene );
	delete m_shape_scene;
}

void UI::AddObject( object::UIObject *object ) {
	m_root_object.AddChild( object );
}
void UI::RemoveObject( object::UIObject *object ) {
	m_root_object.RemoveChild( object );
}

Scene *UI::GetShapeScene() {
	return m_shape_scene;
}

Scene *UI::GetTextScene() {
	return m_text_scene;
}

const UI::coord_t UI::ClampX( const UI::coord_t value ) const {
	return m_clamp.x.Clamp( value );
}

const UI::coord_t UI::ClampY( const UI::coord_t value ) const {
	return m_clamp.y.Clamp( value );
}

void UI::Resize() {
	m_clamp.x.SetSrcRange( 0.0, g_engine->GetRenderer()->GetWindowWidth() );
	m_clamp.y.SetSrcRange( 0.0, g_engine->GetRenderer()->GetWindowHeight() );
	m_root_object.Realign();
	
	for (auto& it : m_debug_frames) {
		ResizeDebugFrame( it.first, &it.second );
	}
}

void UI::Iterate() {

}

void UI::SendEvent( const event::UIEvent* event ) {
	//Log("Got event: " + event->GetName());
	m_root_object.SendEvent( event );
}

void UI::ShowDebugFrame( const UIObject* object ) {
	auto it = m_debug_frames.find( object );
	if ( it == m_debug_frames.end() ) {
		Log("Showing debug frame for " + object->GetName());
		debug_frame_data_t data;
		
		// semi-transparent 1x1 texture with random color for every frame
		data.texture = new Texture( "DebugTexture", 1, 1 );
		data.texture->SetPixel( 0, 0, Color::RGBA( rand() % 256, rand() % 256, rand() % 256, 160 ) );
		
		data.mesh = new mesh::vec2::Rectangle();
		//data.mesh = new mesh::vec2::Rectangle( { -0.4, -0.2 }, { 0.4, 0.6 } );
		
		data.actor = new actor::MeshActor( "DebugFrame", data.mesh );
		data.actor->SetTexture( data.texture );
		data.actor->SetPosition( { 0.0, 0.0, -0.9 } );
		
		m_debug_scene->AddActor( data.actor );
		
		ResizeDebugFrame( object, &data );
		
		m_debug_frames[object] = data;
	}
}

void UI::HideDebugFrame( const UIObject* object ) {
	auto it = m_debug_frames.find( object );
	if ( it != m_debug_frames.end() ) {
		Log("Hiding debug frame for " + object->GetName());
		m_debug_scene->RemoveActor( it->second.actor );
		delete it->second.actor;
		delete it->second.texture;
		m_debug_frames.erase( it );
	}
}

void UI::ResizeDebugFrame( const UIObject* object, const debug_frame_data_t* data ) {
	auto geom = object->GetAreaGeometry();
	data->mesh->SetCoords( {
		ClampX(geom.first.x),
		ClampY(geom.first.y)
	}, {
		ClampX(geom.second.x),
		ClampY(geom.second.y)
	});
}

} /* namespace ui */