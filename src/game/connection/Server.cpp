#include "Server.h"

using namespace network;

namespace game {
namespace connection {

Server::Server( LocalSettings* const settings )
	: Connection( network::CM_SERVER, settings ) {
	//
}

void Server::ProcessEvent( const network::Event& event ) {
	Connection::ProcessEvent( event );

	ASSERT( event.cid || event.type == Event::ET_LISTEN, "server connection received event without cid" );

	switch ( event.type ) {
		case Event::ET_LISTEN: {
			ASSERT( !m_player, "player already set" );
			Log( "Listening" );
			m_state->m_settings.global.Initialize();
			m_state->m_slots.Resize( 7 ); // TODO: make dynamic?
			const auto& rules = m_state->m_settings.global.game_rules;
			NEW( m_player, ::game::Player, {
				m_state->m_settings.local.player_name,
				::game::Player::PR_HOST,
				rules.GetDefaultFaction(),
				rules.GetDefaultDifficultyLevel(),
			} );
			m_state->AddPlayer( m_player );
			m_slot = 0; // host always has slot 0
			m_state->AddCIDSlot( 0, m_slot );
			auto& slot = m_state->m_slots.GetSlot( m_slot );
			slot.SetPlayer( m_player, 0, event.data.remote_address ); // host always has cid 0
			if ( m_on_listen ) {
				m_on_listen();
			}
			if ( m_on_global_settings_update ) {
				m_on_global_settings_update();
			}
			if ( m_on_player_join ) {
				m_on_player_join( m_slot, &slot, m_player );
			}
			break;
		}
		case Event::ET_CLIENT_CONNECT: {
			Log( "Client " + std::to_string( event.cid ) + " connected" );
			ASSERT( m_state->GetCidSlots().find( event.cid ) == m_state->GetCidSlots().end(), "player cid already in slots" );

			const auto& banned = m_settings->banned_addresses;
			if ( banned.find( event.data.remote_address ) != banned.end() ) {
				Kick( event.cid, "You are banned" );
				break;
			}

			{
				Packet packet( Packet::PT_REQUEST_AUTH ); // ask to authenticate
				m_network->MT_SendPacket( packet, event.cid );
			}
			break;
		}
		case Event::ET_CLIENT_DISCONNECT: {
			Log( "Client " + std::to_string( event.cid ) + " disconnected" );
			auto it = m_state->GetCidSlots().find( event.cid );
			if ( it != m_state->GetCidSlots().end() ) {
				const auto slot_num = it->second;
				m_state->RemoveCIDSlot( event.cid );
				auto& slot = m_state->m_slots.GetSlot( slot_num );
				auto* player = slot.GetPlayerAndClose();
				ASSERT( player, "player in slot is null" );
				m_state->RemovePlayer( player );
				if ( m_on_player_leave ) {
					m_on_player_leave( slot_num, &slot, player );
				}
			}
			break;
		}
		case Event::ET_PACKET: {
			try {
				Packet packet( Packet::PT_NONE );
				packet.Unserialize( Buffer( event.data.packet_data ) );
				switch ( packet.type ) {
					case Packet::PT_AUTH: {
						if ( packet.data.str.empty() ) {
							Log( "Authentication from " + std::to_string( event.cid ) + " failed, disconnecting" );
							m_network->MT_DisconnectClient( event.cid );
							break;
						}

						Log( "Got authentication from " + std::to_string( event.cid ) + ": " + packet.data.str );

						if ( m_state->GetCidSlots().find( event.cid ) != m_state->GetCidSlots().end() ) {
							Log( "Duplicate uthentication from " + std::to_string( event.cid ) + ", disconnecting" );
							m_network->MT_DisconnectClient( event.cid );
							break;
						}

						// find free slot
						size_t slot_num = 0; // 0 = 'not found'
						for ( auto& slot : m_state->m_slots.GetSlots() ) {
							if ( slot.GetState() == ::game::Slot::SS_OPEN ) {
								break;
							}
							slot_num++;
						}
						if ( slot_num >= m_state->m_slots.GetCount() ) { // no available slots left
							Log( "No free slots for player " + std::to_string( event.cid ) + " (" + packet.data.str + "), dropping" );
							Kick( event.cid, "Server is full!" );
							break;
						}

						const auto& rules = m_state->m_settings.global.game_rules;
						NEWV( player, ::game::Player, {
							packet.data.str,
							::game::Player::PR_PLAYER,
							rules.GetDefaultFaction(),
							rules.GetDefaultDifficultyLevel(),
						} );
						m_state->AddPlayer( player );

						m_state->AddCIDSlot( event.cid, slot_num );
						auto& slot = m_state->m_slots.GetSlot( slot_num );
						slot.SetPlayer( player, event.cid, event.data.remote_address );

						{
							Log( "Sending players list to " + std::to_string( event.cid ) );
							Packet p( Packet::PT_PLAYERS );
							p.data.num = slot_num;
							p.data.str = m_state->m_slots.Serialize().ToString();
							g_engine->GetNetwork()->MT_SendPacket( p, event.cid );
						}
						SendGlobalSettings( event.cid );

						if ( m_on_player_join ) {
							m_on_player_join( slot_num, &slot, player );
						}

						break;
					}
					case Packet::PT_UPDATE_SLOT: {
						Log( "Got slot update from " + std::to_string( event.cid ) );
						const auto& slots = m_state->GetCidSlots();
						const auto& it = slots.find( event.cid );
						if ( it == slots.end() ) {
							Error( event.cid, "slot index mismatch" );
							break;
						}
						auto& slot = m_state->m_slots.GetSlot( it->second );
						slot.Unserialize( packet.data.str );
						if ( m_on_slot_update ) {
							m_on_slot_update( it->second, &slot );
						}
						break;
					}
					case Packet::PT_MESSAGE: {
						Log( "Got chat message from " + std::to_string( event.cid ) + ": " + packet.data.str );
						const auto& slots = m_state->GetCidSlots();
						const auto& it = slots.find( event.cid );
						if ( it == slots.end() ) {
							Error( event.cid, "slot index mismatch" );
							break;
						}
						GlobalMessage( FormatChatMessage( m_state->m_slots.GetSlot( it->second ).GetPlayer(), packet.data.str ) );
						break;
					}
					default: {
						Log( "WARNING: invalid packet type from client " + std::to_string( event.cid ) + " : " + std::to_string( packet.type ) );
					}
				}
			}
			catch ( std::runtime_error& err ) {
				Error( event.cid, err.what() );
			}
			break;
		}
		case Event::ET_ERROR: {
			Error( event.cid, event.data.packet_data );
			break;
		}
		default: {
			Log( "WARNING: invalid event type from client " + std::to_string( event.cid ) + " : " + std::to_string( event.type ) );
		}
	}

}

void Server::Broadcast( std::function< void( const size_t cid ) > callback ) {
	for ( const auto& slot : m_state->m_slots.GetSlots() ) {
		if ( slot.GetState() == Slot::SS_PLAYER ) {
			const auto cid = slot.GetCid();
			if ( cid != 0 ) { // don't send to self
				callback( cid );
			}
		}
	}
}

void Server::KickFromSlot( const size_t slot_num, const std::string& reason ) {
	ASSERT( slot_num < m_state->m_slots.GetSlots().size(), "slot index overflow" );
	auto& slot = m_state->m_slots.GetSlot( slot_num );
	ASSERT( slot.GetState() == Slot::SS_PLAYER, "kick on non-player slot" );
	KickFromSlot( slot, reason );
}

void Server::BanFromSlot( const size_t slot_num, const std::string& reason ) {
	ASSERT( slot_num < m_state->m_slots.GetSlots().size(), "slot index overflow" );
	auto& slot = m_state->m_slots.GetSlot( slot_num );
	ASSERT( slot.GetState() == Slot::SS_PLAYER, "ban on non-player slot" );
	Log( "Banning address: " + slot.GetRemoteAddress() );
	m_settings->banned_addresses.insert( slot.GetRemoteAddress() );
	KickFromSlot( slot, reason );
}

void Server::ChangeGameState( const game_state_t game_state ) {
	Broadcast(
		[ this, game_state ]( const size_t cid ) -> void {
			Log( "Sending game state change (" + std::to_string( game_state ) + ") to " + std::to_string( cid ) );
			Packet p( Packet::PT_GAME_STATE_CHANGE );
			p.data.num = game_state;
			m_network->MT_SendPacket( p, cid );
		}
	);
}

void Server::SendMapGenerationPercentage( const size_t percentage ) {
	Broadcast(
		[ this, percentage ]( const size_t cid ) -> void {
			Log( "Sending map generation percentage (" + std::to_string( percentage ) + "%) to " + std::to_string( cid ) );
			Packet p( Packet::PT_TILES );
			p.data.boolean = false;
			p.data.num = percentage;
			m_network->MT_SendPacket( p, cid );
		}
	);
}

void Server::UpdateSlot( const size_t slot_num, const Slot* slot ) {
	Broadcast(
		[ this, slot_num, slot ]( const size_t cid ) -> void {
			Log( "Sending slot update to " + std::to_string( cid ) );
			Packet p( Packet::PT_SLOT_UPDATE );
			p.data.num = slot_num;
			p.data.str = slot->Serialize().ToString();
			m_network->MT_SendPacket( p, cid );
		}
	);
}

void Server::Message( const std::string& message ) {
	GlobalMessage( FormatChatMessage( GetPlayer(), message ) );
}

void Server::ResetHandlers() {
	Connection::ResetHandlers();
	m_on_listen = nullptr;
}

void Server::UpdateGameSettings() {
	Broadcast(
		[ this ]( const size_t cid ) -> void {
			SendGlobalSettings( cid );
		}
	);
}

void Server::GlobalMessage( const std::string& message ) {
	if ( m_on_message ) {
		m_on_message( message );
	}
	Broadcast(
		[ this, message ]( const size_t cid ) -> void {
			Packet p( Packet::PT_MESSAGE );
			p.data.str = message;
			m_network->MT_SendPacket( p, cid );
		}
	);
}

void Server::Kick( const size_t cid, const std::string& reason = "" ) {
	Log(
		"Kicking " + std::to_string( cid ) + ( !reason.empty()
			? " (reason: " + reason + ")"
			: ""
		)
	);
	Packet p( Packet::PT_KICK );
	p.data.str = reason;
	m_network->MT_SendPacket( p, cid );
	m_network->MT_DisconnectClient( cid );
}

void Server::KickFromSlot( Slot& slot, const std::string& reason ) {
	slot.SetCloseAfterClear();
	Kick( slot.GetCid(), reason );
}

void Server::Error( const size_t cid, const std::string& reason ) {
	Log( "Network protocol error (cid: " + std::to_string( cid ) + "): " + reason );
	Kick( cid, "Network protocol error" );
}

void Server::SendGlobalSettings( size_t cid ) {
	Log( "Sending global settings to " + std::to_string( cid ) );
	Packet p( Packet::PT_GLOBAL_SETTINGS );
	p.data.str = m_state->m_settings.global.Serialize().ToString();
	m_network->MT_SendPacket( p, cid );
}

const std::string Server::FormatChatMessage( const Player* player, const std::string& message ) const {
	return "<" + player->GetPlayerName() + "> " + message;
}

}

}
