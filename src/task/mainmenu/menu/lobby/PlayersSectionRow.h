#pragma once

#include "ui/object/UIContainer.h"
#include "ui/object/Dropdown.h"

#include "game/Slot.h"
#include "game/Player.h"

using namespace ui::object;

namespace task {
namespace mainmenu {
namespace lobby {

class PlayersSection;
	
CLASS( PlayersSectionRow, UIContainer )

	PlayersSectionRow( PlayersSection* parent, const ::game::Slot* slot, const std::string& class_name = "" );

	void Create();
	void Destroy();

	//void Update( const ::game::Player& player );
	
private:
	PlayersSection* m_parent;
	const ::game::Slot* m_slot;

	struct {
		Dropdown* actions;
		Dropdown* faction;
		Dropdown* difficulty_level;
	} m_elements;
	
};

}
}
}

#include "PlayersSection.h"
