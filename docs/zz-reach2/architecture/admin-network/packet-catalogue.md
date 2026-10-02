# Admin network packet catalogue

Ordinals are the values of `PacketAdminType` in `src/network/core/tcp_admin.h` lines 21–64. `ServerFull` is explicitly 100. Every following enumerator is one greater. The header forbids reordering (lines 18–19). Field lists below are the bytes the server writes, or the bytes the server reads, in that order. Where a layout comment in `tcp_admin.h` disagrees, the send or receive function wins, and the comment is named.

How this socket is opened, authenticated, and pumped is [archi-admin-network.md](archi-admin-network.md).

## Framing

| Item | Rule | Source |
| --- | --- | --- |
| Size | First two bytes, little-endian, length of the whole packet including those two bytes | `src/network/core/packet.cpp` lines 64–70 and 248–250 |
| Type | Next byte, this catalogue’s ordinal | `packet.cpp` lines 46–57, `tcp_admin.cpp` line 36 |
| Integers | Little-endian | `packet.h` lines 46–49 |
| Bool | `uint8` 0 or 1 | `packet.cpp` lines 109–111 |
| String | Bytes plus `'\0'`. No length prefix. The `Recv_string` argument is a maximum | `packet.cpp` lines 170–174 |
| Byte buffer | `uint16` length, then that many bytes (`Send_buffer`) | `packet.cpp` lines 181–185 |
| Limit | Senders use `COMPAT_MTU` (1460) unless noted | `src/network/core/config.h` line 44 |
| Encryption | After `ServerEnableEncryption` has been queued, `PrepareToSend` encrypts everything after the size prefix and inserts a 16-byte MAC | `packet.cpp` lines 46–53 and 72–76; `src/network/network_crypto_internal.h` line 20 |

String maxima, including the `'\0'`, from `config.h` lines 54–59:

| Constant | Bytes |
| --- | --- |
| `NETWORK_PASSWORD_LENGTH` | 33 |
| `NETWORK_CLIENT_NAME_LENGTH` | 25 |
| `NETWORK_REVISION_LENGTH` | 33 |
| `NETWORK_RCONCOMMAND_LENGTH` | 500 |
| `NETWORK_CHAT_LENGTH` | 900 |
| `NETWORK_GAMESCRIPT_JSON_LENGTH` | 9000 |

An ordinal this `HandlePacket` does not list, including the hole at 123 and anything from 11 through 99, closes the socket (`src/network/core/tcp_admin.cpp` lines 81–85).

## Bot to server

These are read by `ServerNetworkAdminSocketHandler`. The base class treats them as illegal if a different handler receives them (`tcp_admin.cpp` lines 117–127).

| Ordinal | Enumerator | When the server accepts it | Fields the server reads |
| --- | --- | --- | --- |
| 0 | `AdminJoin` | Status `Inactive`, and `allow_insecure_admin_login` | Password, name, version. Empty password on the server, mismatch, or empty name or version closes the socket |
| 1 | `AdminQuit` | Any status | No payload. The socket is closed |
| 2 | `AdminUpdateFrequency` | Status `Active` | `uint16` `AdminUpdateType`, `uint16` frequency bitmask |
| 3 | `AdminPoll` | Status `Active` | `uint8` `AdminUpdateType`, `uint32` id |
| 4 | `AdminChat` | Status `Active` | `uint8` `NetworkAction`, `uint8` `NetworkChatDestinationType`, `uint32` destination, chat string |
| 5 | `AdminRemoteConsoleCommand` | Status `Active` | Command string, max 500 |
| 6 | `AdminGameScript` | Status `Active` | JSON string, max 9000 |
| 7 | `AdminPing` | Status `Active` | `uint32` token |
| 8 | `AdminExternalChat` | Status `Active` | Source string, `uint16` colour, user string, message string. Each string max 900. An invalid colour is `IllegalPacket` |
| 9 | `AdminJoinSecure` | Status `Inactive` | Name, version, `uint16` method bitmask |
| 10 | `AdminAuthenticationResponse` | Status `Authenticate` | 32-byte public key, 16-byte MAC, 8-byte encrypted message |

Read sites: `src/network/network_admin.cpp` lines 516–550, 664–728, 786–857, 893–896. Response sizes: `src/network/network_crypto.cpp` lines 235–237 and 290–292.

`AdminUpdateFrequency` stores the bitmask when every set bit is allowed for that type. Otherwise `IllegalPacket`. See the mask table below. `AdminPoll` of a missing client or company id sends nothing and stays connected. A type that cannot be polled is `IllegalPacket` (`network_admin.cpp` lines 719–780).

`AdminChat` accepts only `NetworkAction` values 2, 3, 4, and 5 (`ServerMessage`, `ChatBroadcast`, `ChatTeam`, `ChatClient`). The call into the game chat path sets `from_admin` true, so this message is not echoed as `ServerChat`.

## Server to bot

`ServerFull` and `ServerBanned` are written by the listener with no admin object (`src/network/core/tcp_listen.h` lines 44–66). Everything else is queued on `ServerNetworkAdminSocketHandler`.

### Before and during authentication

| Ordinal | Enumerator | Payload the server writes |
| --- | --- | --- |
| 100 | `ServerFull` | Empty. Connection closed. Pool full, or `AllowConnection` is false |
| 101 | `ServerBanned` | Empty. Connection closed. Address matches `_network_ban_list` |
| 102 | `ServerError` | `uint8` `NetworkErrorCode`, then the socket is closed (`network_admin.cpp` lines 132–146) |
| 128 | `ServerAuthenticationRequest` | `uint8` `NetworkAuthenticationMethod`, then 32-byte public key, then 24-byte nonce (`network_crypto.cpp` lines 408–413 and 191–194) |
| 129 | `ServerEnableEncryption` | 24-byte nonce (`network_crypto.cpp` lines 254–256). Plaintext. Handlers are installed after this packet is queued |

### Session control

| Ordinal | Enumerator | Payload the server writes |
| --- | --- | --- |
| 103 | `ServerProtocol` | `uint8` version (`NETWORK_GAME_ADMIN_VERSION`, 3). Then, for each update type from 0 through 9: `bool` true, `uint16` type, `uint16` allowed mask. Then `bool` false. Status becomes `Active` before this packet is built, and `ServerWelcome` is sent next (`network_admin.cpp` lines 153–171) |
| 104 | `ServerWelcome` | Server name, OpenTTD revision, `bool` dedicated, empty string (the comment in the sender says this used to be the map name), `uint32` generation seed, `uint8` `LandscapeType`, `uint32` calendar date of 1 January of the configured starting year, `uint16` map width, `uint16` map height (`network_admin.cpp` lines 178–191) |
| 105 | `ServerNewGame` | Empty. Sent to active admins at the start of a server load, new game, or restart (`src/network/network.cpp` lines 1016–1018) |
| 106 | `ServerShutdown` | Empty. Sent when admins are being disconnected, which is the default of `NetworkDisconnect` (`network.cpp` lines 1039–1042) |

`LandscapeType` (`src/landscape_type.h` lines 16–21): 0 temperate, 1 arctic, 2 tropic, 3 toyland.

The welcome layout comment (`tcp_admin.h` lines 265–278) still calls the fourth string “Name of the Map”. The sender writes `""`.

### Pushed and polled game state

`ServerDate` through `ServerCompanyStatistics` are sent for a matching subscription or for a poll. `ServerCompanyRemove` is sent to every active admin whether or not `CompanyInfo` was requested (`network_admin.cpp` lines 1019–1023).

| Ordinal | Enumerator | Payload the server writes |
| --- | --- | --- |
| 107 | `ServerDate` | `uint32` `TimerGameCalendar::date.base()` (lines 224–228) |
| 108 | `ServerClientJoin` | `uint32` `ClientID`. On a new client this follows `ServerClientInfo` (lines 239–244 and 931–935) |
| 109 | `ServerClientInfo` | `uint32` client id, hostname (empty for the server client), name, `uint8` 0 (sender comment: used to be language), `uint32` join date, `uint8` company (`255` spectator). Skipped when the client-info pointer is null (lines 255–269) |
| 110 | `ServerClientUpdate` | `uint32` client id, name, `uint8` company (lines 280–286) |
| 111 | `ServerClientQuit` | `uint32` client id (lines 298–302) |
| 112 | `ServerClientError` | `uint32` client id, `uint8` `NetworkErrorCode` (lines 314–320) |
| 113 | `ServerCompanyNew` | `uint8` company id. On create this is sent before `ServerCompanyInfo` (lines 330–334 and 994–995) |
| 114 | `ServerCompanyInfo` | See below |
| 115 | `ServerCompanyUpdate` | See below |
| 116 | `ServerCompanyRemove` | `uint8` company id, `uint8` `AdminCompanyRemoveReason` (lines 391–397) |
| 117 | `ServerCompanyEconomy` | One packet per existing company. See below |
| 118 | `ServerCompanyStatistics` | One packet per existing company. See below |
| 126 | `ServerPong` | `uint32` token copied from `AdminPing` (lines 596–601) |

`ServerCompanyInfo` (`network_admin.cpp` lines 345–358):

| Order | Field |
| --- | --- |
| 1 | `uint8` company index |
| 2 | Company name string |
| 3 | President name string |
| 4 | `uint8` colour |
| 5 | `bool` protected. The sender always writes true |
| 6 | `uint32` inaugurated year |
| 7 | `bool` `is_ai` |
| 8 | `uint8` quarters of bankruptcy, `CeilDiv(months_of_bankruptcy, 3)` |

The layout comment at `tcp_admin.h` lines 359–370 stops after `is_ai`. The sender writes the quarters byte as well.

`ServerCompanyUpdate` (`network_admin.cpp` lines 369–380):

| Order | Field |
| --- | --- |
| 1 | `uint8` company index |
| 2 | Company name string |
| 3 | President name string |
| 4 | `uint8` colour |
| 5 | `bool` protected. The sender always writes true |
| 6 | `uint8` quarters of bankruptcy, same `CeilDiv` as info |

The layout comment at `tcp_admin.h` lines 373–387 lists four extra `uint8` share-owner fields after the quarters byte. `SendCompanyUpdate` does not write them.

`ServerCompanyEconomy` (`network_admin.cpp` lines 407–430), per company:

| Order | Field |
| --- | --- |
| 1 | `uint8` company index |
| 2 | `uint64` `company->money`, via `Send_uint64` |
| 3 | `uint64` `company->current_loan` |
| 4 | `uint64` income: the negation of the sum of `yearly_expenses[0]`, a `Money` (`OverflowSafeInt64`, `src/economy_type.h` line 17) passed to `Send_uint64`. The layout comment calls this `int64_t` (`tcp_admin.h` line 404). `Money` converts to `int64_t` (`src/core/overflowsafe_type.hpp` line 174), and that value is what `Send_uint64` receives |
| 5 | `uint16` delivered cargo this quarter, summed and clamped to `UINT16_MAX` |
| 6–8 | Last closed quarter, `old_economy[0]`: `uint64` company value, `uint16` performance, `uint16` delivered cargo clamped the same way |
| 9–11 | Previous quarter, `old_economy[1]`, same three fields |

`old_economy[0]` is the quarter just closed (`src/economy.cpp` lines 673–676). The layout comment’s “last quarter” then “previous quarter” matches index 0 then index 1.

`ServerCompanyStatistics` (`network_admin.cpp` lines 441–455). Vehicle counts, then station counts, each five `uint16`s in `NetworkVehicleType` order (`src/network/network_type.h` lines 22–30):

| Index | `NetworkVehicleType` | Vehicles field | Stations field |
| --- | --- | --- | --- |
| 0 | `Train` | trains | train stations |
| 1 | `Truck` | lorries | lorry stations |
| 2 | `Bus` | buses | bus stops |
| 3 | `Aircraft` | planes | airports and heliports |
| 4 | `Ship` | ships | harbours |

That order matches the layout comment at `tcp_admin.h` lines 417–432.

### Chat, console, commands, GameScript

| Ordinal | Enumerator | Payload the server writes |
| --- | --- | --- |
| 119 | `ServerChat` | `uint8` `NetworkAction`, `uint8` destination type, `uint32` origin client id, message, `uint64` data (`network_admin.cpp` lines 470–478). Data is the give-money amount for that action. An rcon `say` puts 0 or 1 here; see the architecture note |
| 120 | `ServerRemoteConsoleCommand` | `uint16` colour (`ExtendedTextColour::ToNetwork`), result string (lines 505–511). Emitted only while `_redirect_console_to_admin` is this socket |
| 121 | `ServerConsole` | Origin string, text string (lines 559–571). Dropped when `origin.size() + text.size() + 5 >= 1460` |
| 122 | `ServerCommandNames` | Repeated `bool` true, `uint16` `Commands` ordinal, name string. A `bool` false ends the packet. If the next name would not fit, the server ends this packet with false and starts another `ServerCommandNames` (lines 610–634) |
| 123 | `ServerCommandLoggingOld` | Never sent. Receiving it closes the socket |
| 124 | `ServerGameScript` | JSON string (lines 581–586). Sent when a script calls `ScriptAdmin::Send` and this socket’s `Gamescript` mask tests `Automatic` |
| 125 | `ServerRemoteConsoleCommandEnd` | The command string the bot sent (lines 489–494). Queued immediately after `IConsoleCmdExec` returns |
| 127 | `ServerCommandLogging` | `uint32` client id, `uint8` company, `uint16` command ordinal, then `Send_buffer` of `CommandPacket::data` (`uint16` length plus bytes), then `uint32` frame (lines 645–655) |

The command-logging layout comment (`tcp_admin.h` lines 493–498) describes the parameter blob as a bare variable buffer. The sender uses `Send_buffer`, so the blob is a `uint16` length followed by the bytes (`packet.cpp` lines 181–185). Command ids, names, and that blob are not stable across revisions (`tcp_admin.h` lines 470–473 and 488–491).

## Frequency masks announced in `ServerProtocol`

`AdminUpdateFrequency` is an `EnumBitSet`. Bit `n` is `1 <<` the enumerator’s ordinal (`src/core/base_bitset_type.hpp` lines 55–58). `SendProtocol` writes `_admin_update_type_frequencies[type].base()` as a `uint16` (`network_admin.cpp` lines 49–60 and 162–165).

| Bit | `AdminUpdateFrequency` | Value |
| --- | --- | --- |
| 0 | `Poll` | `0x0001` |
| 1 | `Daily` | `0x0002` |
| 2 | `Weekly` | `0x0004` |
| 3 | `Monthly` | `0x0008` |
| 4 | `Quarterly` | `0x0010` |
| 5 | `Annually` | `0x0020` |
| 6 | `Automatic` | `0x0040` |

| `uint16` type | Name | Mask written |
| --- | --- | --- |
| 0 | `Date` | `0x003F` (63): poll and every calendar-style frequency, driven by economy timers |
| 1 | `ClientInfo` | `0x0041` (65): poll, automatic |
| 2 | `CompanyInfo` | `0x0041` (65): poll, automatic |
| 3 | `CompanyEconomy` | `0x003D` (61): poll, weekly, monthly, quarterly, annually. No daily |
| 4 | `CompanyStats` | `0x003D` (61): same as economy |
| 5 | `Chat` | `0x0040` (64): automatic |
| 6 | `Console` | `0x0040` (64): automatic |
| 7 | `CmdNames` | `0x0001` (1): poll |
| 8 | `CmdLogging` | `0x0040` (64): automatic |
| 9 | `Gamescript` | `0x0040` (64): automatic |

`AdminUpdateType::End` is 10 and is not written (`tcp_admin.h` lines 79–91).

`NetworkAuthenticationMethod` uses the same bit rule (`src/network/network_crypto.h` lines 182–190). The bot sends the mask. The server clears bit 0 before choosing a method.

| Bit | Method | Value |
| --- | --- | --- |
| 0 | `X25519_KeyExchangeOnly` | `0x0001`. Cleared for admin joins. Not used |
| 1 | `X25519_PAKE` | `0x0002` |
| 2 | `X25519_AuthorizedKey` | `0x0004` |

## Enums carried inside packets

Values are the enumerator ordinals. Each of these headers says the values are part of the admin API and new entries may only be appended.

### `NetworkAction` (`src/network/network_type.h` lines 79–93)

| Value | Enumerator | On `AdminChat` |
| --- | --- | --- |
| 0 | `ClientJoin` | Rejected |
| 1 | `ClientLeave` | Rejected |
| 2 | `ServerMessage` | Accepted |
| 3 | `ChatBroadcast` | Accepted |
| 4 | `ChatTeam` | Accepted |
| 5 | `ChatClient` | Accepted |
| 6 | `GiveMoney` | Rejected. Can appear on `ServerChat`; the `uint64` is the amount |
| 7 | `ClientNameChange` | Rejected |
| 8 | `CompanySpectator` | Rejected |
| 9 | `CompanyJoin` | Rejected |
| 10 | `CompanyNew` | Rejected |
| 11 | `ClientKicked` | Rejected |
| 12 | `ChatExternal` | Rejected. The game uses this when displaying `AdminExternalChat` locally. That path does not emit `ServerChat` |

### `NetworkChatDestinationType` (`network_type.h` lines 69–73)

| Value | Enumerator |
| --- | --- |
| 0 | `Broadcast` |
| 1 | `Team` |
| 2 | `Client` |

### `NetworkErrorCode` (`network_type.h` lines 99–124)

`ServerError` and `ServerClientError` carry this as a `uint8`. Codes this admin path itself sends: `IllegalPacket` (4), `NotAuthorized` (6), `NotExpected` (7), `WrongPassword` (10), `NoAuthenticationMethodAvailable` (22). `ServerClientError` forwards whatever the game connection stored, so a bot can see the other values too.

| Value | Enumerator |
| --- | --- |
| 0 | `General` |
| 1 | `Desync` |
| 2 | `SavegameFailed` |
| 3 | `ConnectionLost` |
| 4 | `IllegalPacket` |
| 5 | `NewGRFMismatch` |
| 6 | `NotAuthorized` |
| 7 | `NotExpected` |
| 8 | `WrongRevision` |
| 9 | `NameInUse` |
| 10 | `WrongPassword` |
| 11 | `CompanyMismatch` |
| 12 | `Kicked` |
| 13 | `Cheater` |
| 14 | `ServerFull` |
| 15 | `TooManyCommands` |
| 16 | `TimeoutPassword` |
| 17 | `TimeoutComputer` |
| 18 | `TimeoutMap` |
| 19 | `TimeoutJoin` |
| 20 | `InvalidClientName` |
| 21 | `NotOnAllowList` |
| 22 | `NoAuthenticationMethodAvailable` |

### `AdminCompanyRemoveReason` (`tcp_admin.h` lines 106–110)

| Value | Enumerator |
| --- | --- |
| 0 | `Manual` |
| 1 | `Autoclean` |
| 2 | `Bankrupt` |

`End` is 3 and is not sent. `tcp_admin.cpp` lines 18–21 assert these ordinals match `CompanyRemoveReason`.

### `ClientID` and company id

| Value | Meaning | Source |
| --- | --- | --- |
| 0 | `ClientID::Invalid` | `network_type.h` line 44 |
| 1 | `ClientID::Server` | line 45 |
| 2 | `ClientID::First` | line 46 |
| 0–14 | Company index. `MAX_COMPANIES` is 15 | `src/company_type.h` lines 16 and 32 |
| 255 | `COMPANY_SPECTATOR` in the client play-as byte | line 21 |
| `0xFFFFFFFF` | Poll id meaning “every client” or “every company” | `network_admin.cpp` lines 734 and 752 |

### `AdminStatus` (`tcp_admin.h` lines 72–76)

Not a packet field. 0 `Inactive` on accept, 1 `Authenticate` while a secure handshake is in progress, 2 `Active` from the moment `ServerProtocol` is built.
