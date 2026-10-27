## Log in through the shared network layer

### Client

**Added**
- `LoginLink`, the client's Login server connection on the shared `hb::net::client`, driven by `poll()` from the game loop; password copies are wiped after sending.
- The Log In screen sends the shared `login_request` and shows each `login_result`; the New Account screen (re-enabled for the English build) sends `create_account_request` with account, password and email and shows each `account_result`.
- Texts for account locked, service not available, account creation disabled and invalid account name.
- The shared `hb::log` (`Client.log`) and the shared net, protocol and log sources.

**Changed**
- `login.json` now sets the Login server address in English builds too; a missing file keeps the built-in default, and a malformed file or an out-of-range port stops start-up with the config error.

**Removed**
- The legacy `XSocket` login and create-account send paths.

Summary: Client creates accounts and logs in against the new Login server over the shared network layer (2026-10-27)
