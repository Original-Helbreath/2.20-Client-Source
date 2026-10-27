// LoginLink.h: the client's connection to the Login server (CLIENT-018).
//
// One hb::net::client from the shared/ submodule, driven by poll() from the game loop (no threads of its
// own). One request at a time: request_login() or request_create_account() connects, sends the typed
// struct, and delivers the typed reply through the handlers. Everything here is platform neutral; the
// Win32 game code only calls it. The reply-to-screen-code mapping lives here too so it can be tested
// without a window.
#pragma once

#include "../shared/protocol/login_messages.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class LoginLink
{
public:
	struct handlers
	{
		// The TCP connection is open and the request has been sent.
		std::function<void()> on_connected;
		// The login reply. characters holds the character_list tail when result is ok, else it is empty.
		std::function<void(const hb::net::login::login_response&, const std::vector<hb::net::character_summary>&)> on_login;
		// The create-account reply.
		std::function<void(hb::net::login::account_result)> on_account;
		// The create_character reply (CLIENT-019). characters holds the fresh character_list when the result
		// is ok, else it is empty and the session stays open.
		std::function<void(hb::net::login::character_result, const std::vector<hb::net::character_summary>&)> on_create;
		// The delete_character reply, shaped like on_create.
		std::function<void(hb::net::login::character_result, const std::vector<hb::net::character_summary>&)> on_delete;
		// No reply will come: resolve or connect failed, the connection closed first, or the reply was
		// malformed. The text is for the log.
		std::function<void(std::string_view)> on_failed;
	};

	LoginLink(std::string host, std::uint16_t port, handlers callbacks);
	~LoginLink();

	LoginLink(const LoginLink&) = delete;
	LoginLink& operator=(const LoginLink&) = delete;

	// False when a text does not fit its wire field (account 16, password 32, world 32, email 64) or
	// holds a NUL; nothing is sent then. A request in flight is dropped first.
	bool request_login(std::string_view account, std::string_view password, std::string_view world);
	bool request_create_account(std::string_view account, std::string_view password, std::string_view email);

	// The fields of a character to create; the text is 1..10 letters or digits (the Login server checks).
	struct new_character
	{
		std::string_view name;
		std::uint8_t sex = 1;
		std::uint8_t skin = 1;
		std::uint8_t hair_style = 0;
		std::uint8_t hair_colour = 0;
		std::uint8_t underwear_colour = 0;
		hb::net::character_stats stats{};
	};

	// Character requests (CLIENT-019) go over the session opened by a successful login. False, with nothing
	// sent, when there is no session, another request is pending, or the name does not fit the wire field.
	bool request_create_character(const new_character& fields);
	bool request_delete_character(std::string_view name);

	// True after a login ok (and its character list) until the connection closes or is cancelled.
	bool has_session() const;

	// Drops the request in flight and closes the connection; no handler is called for it.
	void cancel();

	// Runs the ready network work on the calling thread; call once per frame.
	void poll();

	// True from a request until its reply (or failure) has been delivered.
	bool awaiting_reply() const;

private:
	struct impl;
	std::unique_ptr<impl> m_impl;
};

// The legacy screen-message code (m_cMsg, read by UpdateScreen_OnLogResMsg) for a login or account
// reply. Null when no message screen follows: login ok goes to the character screen, and
// version_mismatch goes to the version screen.
const char* LoginResultCode(hb::net::login::login_result result);
const char* AccountResultCode(hb::net::login::account_result result);
// CLIENT-019: the message code for a create or delete character reply. The first character names the
// screen behind the message (2 create character, 3 character select), the second the text.
const char* CreateCharacterResultCode(hb::net::login::character_result result);
const char* DeleteCharacterResultCode(hb::net::login::character_result result);
