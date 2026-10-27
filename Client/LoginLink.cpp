// LoginLink.cpp: see LoginLink.h.
#include "LoginLink.h"

#include "../shared/log/log.h"
#include "../shared/net/client.h"
#include "../shared/net/connection.h"
#include "../shared/protocol/message_codec.h"

#include <cstddef>
#include <format>
#include <utility>

using namespace hb::net;

namespace
{
	// Portable secure zero: the volatile writes cannot be optimised away.
	void scrub(void* data, std::size_t size)
	{
		volatile unsigned char* p = static_cast<volatile unsigned char*>(data);
		for (std::size_t i = 0; i < size; ++i)
			p[i] = 0;
	}

	void scrub(std::vector<std::byte>& bytes)
	{
		if (!bytes.empty())
			scrub(bytes.data(), bytes.size());
	}

	// Zeroes what it watches when the scope ends, on every return path.
	class scrubber
	{
	public:
		scrubber(void* data, std::size_t size) : m_data(data), m_size(size) {}
		explicit scrubber(std::vector<std::byte>& bytes) : m_bytes(&bytes) {}
		~scrubber()
		{
			if (m_bytes)
				scrub(*m_bytes);
			else
				scrub(m_data, m_size);
		}
		scrubber(const scrubber&) = delete;
		scrubber& operator=(const scrubber&) = delete;

	private:
		void* m_data = nullptr;
		std::size_t m_size = 0;
		std::vector<std::byte>* m_bytes = nullptr;
	};
}

struct LoginLink::impl
{
	enum class phase
	{
		idle,
		login,           // sent login_request, waiting for login_response
		character_list,  // LOGIN-007: unused until the server sends a character_list after login ok
		account,         // sent create_account_request, waiting for create_account_response
	};

	impl(std::string host_name, std::uint16_t port_number, handlers callbacks)
		: cb(std::move(callbacks))
		, host(std::move(host_name))
		, port(port_number)
	{
	}

	// Declared first so it is destroyed last: handlers still queued in it hold the client and connection.
	asio::io_context io;
	handlers cb;
	std::string host;
	std::uint16_t port;
	std::shared_ptr<client> link;
	std::shared_ptr<connection> conn;
	std::vector<std::byte> pending;
	phase state = phase::idle;
	login::login_response held{};
	// Bumped whenever the connection is dropped or replaced; every queued callback carries the value it was
	// made under and does nothing if it is stale.
	std::uint32_t generation = 0;

	static constexpr std::uint32_t max_body_bytes = 65536;

	void reset_connection()
	{
		++generation;
		state = phase::idle;
		scrub(pending);
		pending.clear();
		if (link)
		{
			link->stop();
			link.reset();
		}
		if (conn)
		{
			conn->close(close_reason::local_close);
			conn.reset();
		}
	}

	void begin(const std::vector<std::byte>& bytes, phase next)
	{
		reset_connection();
		scrub(pending);
		pending = bytes;
		state = next;
		const std::uint32_t gen = generation;

		client_options options;
		options.host = host;
		options.port = port;
		options.connection.max_body_bytes = max_body_bytes;
		options.reconnects = false;
		link = client::create(io, options);
		hb::log::info("login link: connecting to {}:{}", host, port);
		link->start(
			[this, gen](const std::shared_ptr<connection>& opened) {
				if (gen != generation)
				{
					opened->close(close_reason::shutdown);
					return;
				}
				conn = opened;
				opened->start(
					[this, gen](connection&, const frame& received) {
						if (gen == generation)
							on_frame(received);
					},
					[this, gen](connection&, close_reason reason, std::string_view detail) {
						if (gen == generation)
							on_closed(reason, detail);
					});
				opened->send_encoded(pending);  // the send queue takes a copy; ours is zeroed
				scrub(pending);
				pending.clear();
				hb::log::info("login link: connected, request sent");
				if (cb.on_connected)
					cb.on_connected();
			},
			[this, gen](std::string_view error) {
				if (gen == generation)
					fail(std::string(error));
			});
	}

	// No reply will come. The connection is dropped before the handler runs, so the handler may start a
	// new request.
	void fail(const std::string& text)
	{
		hb::log::warning("login link: {}", text);
		const auto handler = cb.on_failed;
		reset_connection();
		if (handler)
			handler(text);
	}

	void on_closed(close_reason reason, std::string_view detail)
	{
		if (state == phase::idle)
		{
			hb::log::info("login link: closed ({})", to_string(reason));
			conn.reset();
			return;
		}
		fail(std::format("connection closed before the reply: {} {}", to_string(reason), detail));
	}

	void on_frame(const frame& received)
	{
		switch (state)
		{
		case phase::login:
		{
			login::login_response reply{};
			if (!decode(received, reply))
				return fail("unexpected frame while waiting for login_response");
			if (reply.result != login::login_result::ok)
			{
				hb::log::info("login link: login refused ({})", static_cast<int>(reply.result));
				const auto handler = cb.on_login;
				reset_connection();
				if (handler)
					handler(reply, {});
				return;
			}
			// LOGIN-006: no character_list follows the ok yet (LOGIN-007). Deliver now with an empty list and
			// keep the connection open for the session; a list that arrives later is logged, not shown.
			state = phase::idle;
			hb::log::info("login link: login ok");
			if (cb.on_login)
				cb.on_login(reply, {});
			return;
		}
		case phase::character_list:
		{
			login::character_list head{};
			std::vector<character_summary> characters;
			if (!decode_character_list(received, head, characters))
				return fail("unexpected frame while waiting for character_list");
			state = phase::idle;
			hb::log::info("login link: login ok, {} character(s)", characters.size());
			if (cb.on_login)
				cb.on_login(held, characters);
			return;
		}
		case phase::account:
		{
			login::create_account_response reply{};
			if (!decode(received, reply))
				return fail("unexpected frame while waiting for create_account_response");
			hb::log::info("login link: create account reply ({})", static_cast<int>(reply.result));
			const auto handler = cb.on_account;
			reset_connection();
			if (handler)
				handler(reply.result);
			return;
		}
		case phase::idle:
			hb::log::warning("login link: unexpected frame {:#06x} with no request in flight", received.header.message_id);
			return;
		}
	}
};

LoginLink::LoginLink(std::string host, std::uint16_t port, handlers callbacks)
	: m_impl(std::make_unique<impl>(std::move(host), port, std::move(callbacks)))
{
}

LoginLink::~LoginLink()
{
	m_impl->reset_connection();
	// Let the close and stop posted above run while the handlers still have an owner.
	m_impl->io.restart();
	for (int pass = 0; pass < 50 && m_impl->io.poll() > 0; ++pass)
		m_impl->io.restart();
}

bool LoginLink::request_login(std::string_view account, std::string_view password, std::string_view world)
{
	login::login_request request{};
	std::vector<std::byte> bytes;
	const scrubber scrub_request(&request, sizeof(request));  // the password does not outlive this call
	const scrubber scrub_bytes(bytes);
	if (!request.account.assign(account) || !request.password.assign(password) || !request.world.assign(world))
		return false;
	if (!encode(request, impl::max_body_bytes, bytes))
		return false;
	m_impl->begin(bytes, impl::phase::login);
	return true;
}

bool LoginLink::request_create_account(std::string_view account, std::string_view password, std::string_view email)
{
	login::create_account_request request{};
	std::vector<std::byte> bytes;
	const scrubber scrub_request(&request, sizeof(request));
	const scrubber scrub_bytes(bytes);
	if (!request.account.assign(account) || !request.password.assign(password) || !request.email.assign(email))
		return false;
	if (!encode(request, impl::max_body_bytes, bytes))
		return false;
	m_impl->begin(bytes, impl::phase::account);
	return true;
}

void LoginLink::cancel()
{
	scrub(m_impl->pending);
	m_impl->pending.clear();
	m_impl->reset_connection();
}

void LoginLink::poll()
{
	m_impl->io.restart();
	m_impl->io.poll();
}

bool LoginLink::awaiting_reply() const
{
	return m_impl->state != impl::phase::idle;
}

const char* LoginResultCode(login::login_result result)
{
	using r = login::login_result;
	switch (result)
	{
	case r::ok:
	case r::version_mismatch:
		return nullptr;
	case r::password_mismatch:
		return "11";  // Password is not correct!
	case r::no_such_account:
		return "12";  // Not existing account!
	case r::no_such_world:
	case r::world_unavailable:
		return "1Y";  // the world server is not running
	case r::account_blocked:
		return "7H";  // Connection Rejected (blocked until the date in the reply)
	case r::account_locked:
		return "7K";  // Account locked
	case r::service_unavailable:
		return "7L";  // Service not available
	}
	return "7L";
}

const char* AccountResultCode(login::account_result result)
{
	using r = login::account_result;
	switch (result)
	{
	case r::ok:
		return "54";  // New account created.
	case r::failed:
		return "05";  // Can not create new account!
	case r::name_taken:
		return "06";  // Already existing account name.
	case r::invalid_name:
		return "0O";  // Invalid account name
	case r::disabled:
		return "0N";  // Account creation is disabled
	}
	return "05";
}
