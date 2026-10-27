// WorldLink.h: the client's connection to the World server (CLIENT-021).
//
// One hb::net::client from the shared/ submodule, driven by poll() from the game loop (no threads of its
// own). It connects to the address the Login server gave in enter_game_response, sends world_enter_request
// (ticket and character) as its first frame, and reads world_enter_response. After ok every frame is one
// legacy 2.20 game message (SHARED-003 envelope, no cipher): iSendMsg() sends one, and each received one
// is queued as an event. The Win32 game code only calls this; nothing here includes a Windows header.
//
// The members m_bIsAvailable and iSendMsg() keep the legacy XSocket call shapes so the ~70 send sites in
// the game code stay as they are.
#pragma once

#include "../shared/protocol/common_types.h"
#include "../shared/protocol/world_messages.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class WorldLink
{
public:
	enum class event_kind : std::uint8_t
	{
		entered,  // world_enter_response ok: the game messages may flow
		refused,  // world_enter_response was not ok; result says why. The link is closing.
		message,  // one legacy game message (DWORD msg id, WORD msg type, payload) in body
		closed,   // connect failed or the connection ended; text is for the log
	};

	struct event
	{
		event_kind kind = event_kind::closed;
		hb::net::world::world_enter_result result = hb::net::world::world_enter_result::ok;
		std::vector<char> body;
		std::string text;
	};

	// iSendMsg() results that are not a byte count; they equal the DEF_XSOCKEVENT_* values the game code switches on.
	static constexpr int send_socket_mismatch = -121;
	static constexpr int send_socket_closed = -127;
	static constexpr int send_msg_too_large = -132;

	// Starts connecting at once. The character text must fit the wire field (16, the game uses 10).
	WorldLink(std::string host, std::uint16_t port, const hb::net::session_ticket& ticket, std::string_view character);
	~WorldLink();

	WorldLink(const WorldLink&) = delete;
	WorldLink& operator=(const WorldLink&) = delete;

	// Runs the ready network work on the calling thread; call once per frame.
	void poll();

	// Pops the oldest queued event; false when none. Events are only queued by poll(), so the owner may
	// delete this link while handling one.
	bool next_event(event& out);

	// The legacy XSocket::iSendMsg shape: the whole message in data[0, size), the key is ignored (no cipher).
	// Returns size, or a send_* code when nothing was sent.
	int iSendMsg(char* data, std::uint32_t size, char key = 0);

	// True between world_enter_response ok and the close.
	bool m_bIsAvailable = false;

private:
	struct impl;
	std::unique_ptr<impl> m_impl;
};
