## Enter the World with the Login ticket

### Client

**Added**
- `WorldLink`, a shared `hb::net::client` World connection: it sends the Login ticket first, then carries game messages as SHARED-003 envelopes, keeping the legacy `iSendMsg` shape. The ticket is wiped from memory after it is sent.
- A "ticket expired" message screen (code 3W).

**Changed**
- Entering the game goes through `enter_game_request` on the login session instead of the legacy log socket; each failure shows a message screen.
- `MSGID_REQUEST_INITPLAYER` is no longer sent; the World's INITPLAYER confirm starts the init sequence.
- `shared/` moves to SHARED-003.

Summary: Client enters the World with its Login ticket over the shared network layer (2026-10-27)
