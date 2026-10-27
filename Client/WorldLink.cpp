// WorldLink.cpp: see WorldLink.h.
#include "WorldLink.h"

#include "../shared/log/log.h"
#include "../shared/net/client.h"
#include "../shared/net/connection.h"
#include "../shared/protocol/message_codec.h"

#include <chrono>
#include <cstddef>
#include <deque>
#include <cstring>
#include <format>
#include <span>
#include <utility>

using namespace hb::net;

namespace
{
	// The World closes a link whose first frame is late, and the game message body is at most this big.
	constexpr std::uint32_t max_body_bytes = 65536;
	// Time the World has to answer world_enter_request.
	constexpr std::chrono::milliseconds enter_timeout{ 15000 };
	// A legacy game message holds at least its DWORD id and WORD type.
	constexpr std::size_t min_game_message = 6;

	// Portable secure zero: the volatile writes cannot be optimised away (same as LoginLink).
	void scrub(void* data, std::size_t size)
	{
		volatile unsigned char* p = static_cast<volatile unsigned char*>(data);
		for (std::size_t i = 0; i < size; ++i)
			p[i] = 0;
	}
}

struct WorldLink::impl
{
	enum class phase
	{
		connecting,  // resolving or connecting
		entering,    // world_enter_request sent, waiting for world_enter_response
		active,      // ok: game messages flow
		done,        // closed or refused; nothing more is delivered
	};

	impl(std::string host_name, std::uint16_t port_number, const session_ticket& ticket, std::string_view character, bool& available_flag)
		: host(std::move(host_name))
		, port(port_number)
		, available(available_flag)
	{
		request.ticket = ticket;
		valid = request.character.assign(character);
	}

	// Declared first so it is destroyed last: handlers still queued in it hold the client and connection.
	asio::io_context io;
	std::string host;
	std::uint16_t port;
	bool& available;
	world::world_enter_request request{};
	bool valid = false;
	std::shared_ptr<client> link;
	std::shared_ptr<connection> conn;
	phase state = phase::connecting;
	std::deque<event> events;
	std::uint32_t generation = 0;

	void start()
	{
		if (!valid)
		{
			finish("character name does not fit the wire field");
			return;
		}
		const std::uint32_t gen = generation;
		client_options options;
		options.host = host;
		options.port = port;
		options.connection.max_body_bytes = max_body_bytes;
		options.reconnects = false;
		link = client::create(io, options);
		hb::log::info("world link: connecting to {}:{}", host, port);
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
				opened->set_deadline(enter_timeout);
				opened->send(request);  // the send queue holds an encoded copy
				scrub(&request, sizeof(request));  // the ticket does not outlive the send
				state = phase::entering;
				hb::log::info("world link: connected, world_enter_request sent");
			},
			[this, gen](std::string_view error) {
				if (gen == generation)
					finish(std::string(error));
			});
	}

	void push_closed(std::string text)
	{
		event e;
		e.kind = event_kind::closed;
		e.text = std::move(text);
		events.push_back(std::move(e));
	}

	// The connection is gone or never opened: one closed event, once.
	void finish(std::string text)
	{
		if (state == phase::done)
			return;
		state = phase::done;
		available = false;
		hb::log::warning("world link: {}", text);
		push_closed(std::move(text));
	}

	void drop(close_reason reason, std::string_view detail)
	{
		if (conn)
			conn->close(reason, detail);
	}

	void on_closed(close_reason reason, std::string_view detail)
	{
		conn.reset();
		if (state == phase::done)
			return;  // refused, or already reported
		finish(std::format("connection closed: {} {}", to_string(reason), detail));
	}

	void on_frame(const frame& received)
	{
		switch (state)
		{
		case phase::entering:
		{
			world::world_enter_response reply{};
			if (!decode(received, reply))
			{
				drop(close_reason::protocol_error, "expected world_enter_response");
				return finish("unexpected frame while waiting for world_enter_response");
			}
			hb::log::info("world link: enter reply ({})", static_cast<int>(reply.result));
			event e;
			e.result = reply.result;
			if (reply.result == world::world_enter_result::ok)
			{
				conn->set_deadline(std::chrono::milliseconds{ 0 });
				state = phase::active;
				available = true;
				e.kind = event_kind::entered;
				events.push_back(std::move(e));
				return;
			}
			state = phase::done;
			available = false;
			e.kind = event_kind::refused;
			events.push_back(std::move(e));
			drop(close_reason::local_close, "refused");
			return;
		}
		case phase::active:
		{
			if (received.header.message_id != to_wire(message_id::client_game_message) || received.body.size() < min_game_message)
			{
				drop(close_reason::protocol_error, "bad game frame");
				return finish("malformed game frame");
			}
			event e;
			e.kind = event_kind::message;
			e.body.resize(received.body.size());
			std::memcpy(e.body.data(), received.body.data(), received.body.size());
			events.push_back(std::move(e));
			return;
		}
		case phase::connecting:
		case phase::done:
			return;
		}
	}

	void reset()
	{
		++generation;
		scrub(&request, sizeof(request));  // also covers a link dropped before it connected
		state = phase::done;
		available = false;
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
};

WorldLink::WorldLink(std::string host, std::uint16_t port, const session_ticket& ticket, std::string_view character)
	: m_impl(std::make_unique<impl>(std::move(host), port, ticket, character, m_bIsAvailable))
{
	m_impl->start();
}

WorldLink::~WorldLink()
{
	m_impl->reset();
	// Let the close and stop posted above run while the handlers still have an owner.
	m_impl->io.restart();
	for (int pass = 0; pass < 50 && m_impl->io.poll() > 0; ++pass)
		m_impl->io.restart();
}

void WorldLink::poll()
{
	m_impl->io.restart();
	m_impl->io.poll();
}

bool WorldLink::next_event(event& out)
{
	if (m_impl->events.empty())
		return false;
	out = std::move(m_impl->events.front());
	m_impl->events.pop_front();
	return true;
}

int WorldLink::iSendMsg(char* data, std::uint32_t size, char)
{
	if (m_impl->state == impl::phase::done || !m_impl->conn)
		return send_socket_closed;
	if (m_impl->state != impl::phase::active)
		return send_socket_mismatch;
	if (size < min_game_message || size > max_body_bytes)
		return send_msg_too_large;
	std::vector<std::byte> bytes;
	const std::span<const std::byte> body = std::as_bytes(std::span<const char>(data, size));
	if (!encode_frame(to_wire(message_id::world_game_message), body, max_body_bytes, bytes))
		return send_msg_too_large;
	m_impl->conn->send_encoded(std::move(bytes));
	return static_cast<int>(size);
}
