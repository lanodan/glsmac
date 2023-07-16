#pragma once

#include "types/Serializable.h"

#include "Player.h"

namespace game {

CLASS( Slot, types::Serializable )

	enum slot_state_t {
		SS_CLOSED,
		SS_OPEN,
		SS_PLAYER,
	};
	const slot_state_t GetState() const;

	void Open();
	Player* GetPlayer() const;
	Player* GetPlayerAndClose();
	void SetPlayer( Player* player );

	const types::Buffer Serialize() const;
	void Unserialize( types::Buffer buf );

private:
	
	slot_state_t m_slot_state = SS_OPEN;
	Player* m_player = nullptr;
	
};

}