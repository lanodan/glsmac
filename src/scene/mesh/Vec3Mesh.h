#pragma once

#include "Mesh.h"

#include "types/Vec3.h"

namespace scene {
namespace mesh {

MINOR_CLASS( Vec3, Mesh )
	typedef types::Vec3 vec3_t;
	typedef types::Vec2<Mesh::coord_t> vec2_t;

	Vec3Mesh() : Mesh( 5 ) {};
	
	index_t AddVertex( const vec3_t &coord ) {
		AddCoord( coord.x );
		AddCoord( coord.y );
		AddCoord( coord.z );
		AddCoord( 0 );
		return AddCoord( 0 );
	}
	
	index_t AddVertex( const vec3_t &coord, const vec2_t &tex_coord ) {
		AddCoord( coord.x );
		AddCoord( coord.y );
		AddCoord( coord.z );
		AddCoord( tex_coord.x );
		return AddCoord( tex_coord.y );
	}
	
	index_t AddVertex( const float tex_tint, const vec3_t &coord, const vec2_t &tex_coord ) {
		AddCoord( coord.x );
		AddCoord( coord.y );
		AddCoord( coord.z );
		AddCoord( tex_coord.x );
		return AddCoord( tex_coord.y );
	}

};

} /* namespace mesh */
} /* namespace scene */