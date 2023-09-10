#pragma once

#include "LobbySection.h"

#include "ui/object/Input.h"
#include "ui/object/TextView.h"

namespace task {
namespace mainmenu {
namespace lobby {

CLASS( ChatSection, LobbySection )

	ChatSection( Lobby* lobby );

	void Create();
	void Align();
	void Destroy();

	void AddMessage( const std::string& message );

private:
	::ui::object::TextView* m_log = nullptr;
	::ui::object::Input* m_input = nullptr;

};

}
}
}
