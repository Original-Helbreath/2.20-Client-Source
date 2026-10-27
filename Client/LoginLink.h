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
