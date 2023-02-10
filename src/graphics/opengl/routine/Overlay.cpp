#include "Overlay.h"

namespace graphics {
namespace opengl {
namespace routine {

void Overlay::Start() {

}

void Overlay::Stop() {

}

void Overlay::Iterate() {

	for ( auto it = m_gl_scenes.begin() ; it < m_gl_scenes.end() ; ++it )
		(*it)->Update();

	glClear( GL_DEPTH_BUFFER_BIT ); // overlay must be always on top
	
	for ( auto it = m_gl_scenes.begin() ; it < m_gl_scenes.end() ; ++it ) {
		(*it)->Draw( m_shader_program );
	}

}

bool Overlay::SceneBelongs( const scene::Scene *scene ) const {
	return scene->GetType() == scene::SCENE_TYPE_SIMPLE2D;
}

} /* namespace routine */
} /* namespace opengl */
} /* namespace graphics */
