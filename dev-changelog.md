## Character select through LoginLink

### Client

**Added**
- `LoginLink` reads the `character_list` that follows login ok and fills the character-select screen from it.
- Create Character and Delete send the shared `create_character_request` and `delete_character_request` over the login session, then refresh the list from the reply.
- Message texts for invalid character name, invalid stats, no free slot, level too high, character not found and delete failed.

**Changed**
- The Login connection stays open after login ok for the character requests.

**Removed**
- The legacy `XSocket` create-character and delete-character send paths and their reply handlers.

Summary: Client lists, creates and deletes characters through the Login server (2026-10-27)
