#include "SideMenu.h"

#include "engine/Engine.h"
#include "../../World.h"

namespace game {
namespace world {
namespace ui {

SideMenu::SideMenu( World* world, const uint8_t item_height, const uint8_t margin )
	: UI( world, "MapBottomBarLeftMenu" )
	, m_item_height( item_height )
	, m_margin( margin )
{
	Hide(); // closed by default
}

SideMenu::~SideMenu() {
	for ( auto& item : m_menu_items ) {
		if ( item.submenu ) {
			g_engine->GetUI()->RemoveObject( item.submenu );
		}
	}
}

void SideMenu::Create() {
	UI::Create();
	
	Log( "Creating side menu" );
	
	SetHeight( m_margin * 2 + m_item_height * m_menu_items.size() );
	
	NEW( m_background, ::ui::object::Surface, "MapBottomBarSideMenuBackground" );
	AddChild( m_background );
	
	size_t top = m_margin;
	for ( auto& item : m_menu_items ) {
		NEWV( button, ::ui::object::LabelButton, "MapBottomBarSideMenuButton" );
		button->SetHeight( m_item_height );
		button->SetTop( top );
		button->SetLabel( item.label );
		button->On( UIEvent::EV_BUTTON_CLICK, EH( this, item, button ) {
			if ( m_active_button && m_active_button != button ) {
				m_active_button->RemoveStyleModifier( Style::M_SELECTED );
				m_active_button = nullptr;
			}
			if ( m_active_submenu && m_active_submenu != item.submenu ) {
				m_active_submenu->Hide();
				m_active_submenu = nullptr;
			}
			if ( item.submenu ) {
				if ( button->HasStyleModifier( Style::M_SELECTED ) ) {
					button->RemoveStyleModifier( Style::M_SELECTED );
					item.submenu->Hide();
					m_active_button = nullptr;
					m_active_submenu = nullptr;
				}
				else {
					button->AddStyleModifier( Style::M_SELECTED );
					item.submenu->Show();
					m_active_button = button;
					m_active_submenu = item.submenu;
				}
			}
			else { 
				if ( item.on_click ) {
					if ( item.on_click( button, item ) ) {
						m_world->CloseMenus();
					}
				}
			}
			return true;
		});
		AddChild( button );
		m_buttons.push_back( button );
		if ( item.submenu ) {
			item.submenu->SetBottom( GetBottom() + GetHeight() - ( button->GetTop() + button->GetHeight() ) );
			item.submenu->SetLeft( GetWidth() + GetLeft() );
		}
		top += m_item_height;
	}
	
	On( UIEvent::EV_MOUSE_DOWN, EH() {
		return true; // prevent clickthrough
	});
}

void SideMenu::Destroy() {
	
	Log( "Destroying side menu" );
	
	Hide();
	
	RemoveChild( m_background );
	for ( auto& button : m_buttons ) {
		RemoveChild( button );
	}
	m_buttons.clear();
	
	UI::Destroy();
}

void SideMenu::Show() {
	UI::Show();
}

void SideMenu::Hide() {
	
	if ( m_active_button ) {
		m_active_button->RemoveStyleModifier( Style::M_SELECTED );
		m_active_button = nullptr;
	}
	if ( m_active_submenu ) {
		m_active_submenu->Hide();
		m_active_submenu = nullptr;
	}
	
	UI::Hide();
}

void SideMenu::AddItem( const std::string& label, std::function<bool( LabelButton* button, menu_item_t item )> on_click ) {
	ASSERT( !m_created, "don't add items to active menu" );
	m_menu_items.push_back({
		label,
		nullptr,
		on_click
	});
}

void SideMenu::AddSubMenu( const std::string& label, SideMenu* submenu ) {
	ASSERT( !m_created, "don't add submenus to active menu" );
	m_menu_items.push_back({
		label,
		submenu,
		0
	});
	g_engine->GetUI()->AddObject( submenu );
}

}
}
}
