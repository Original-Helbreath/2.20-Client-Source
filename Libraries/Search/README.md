# search.dll (documentation only)

search.dll is the client's anti-cheat helper. There is no source for it, and by project decision it is
**not recreated**. This folder only documents what the two known builds do. The client no longer loads it
(see *Client changes* below).

Tags: **[V]** seen in the decompile, **[I]** inferred, **[U]** unverified. Addresses are virtual addresses
in the DLL (image base `0x10000000`).

## Files

- `decompile/search_src.c`: Ghidra pseudo-C of the source-tree build's application code.
- `decompile/search_202.c`: the same for the retail 2.02 build.

Both files are Ghidra 12.1.4 headless output with `FUN_` names replaced by readable ones. The statically
linked MFC 4.2 and VC6 CRT code is left out.

## The two builds

| | Source-tree build | Retail 2.02 build |
|---|---|---|
| Path in 2.20-Reference | `binaries/client-source-dlls/search.dll` | `binaries/client-2.02/search.dll` |
| Size | 135,168 bytes | 143,360 bytes |
| PE timestamp | 2002-10-14 13:46:40 UTC [V] | 2001-06-11 03:50:26 UTC [V] |
| Export (ordinal 1) | `__FindHackingDll__`, takes `char *` [V] | `_FindHackingDll_`, takes no arguments [V] |
| Returns | 1 = clean, -1 = refused [V] | 1 = clean, -1 = detected [V] |

The source-tree build is the newer one, judging by its timestamp, and it is the build the 2.20 client was
written against: Wmain.cpp looks up `__FindHackingDll__` and passes `"CRCCHECK"`.

Both builds are MFC DLLs with an empty `CWinApp` subclass. Both contain the same helper class, which we
call `CProcessList` because the binary has no symbols. Apart from addresses, the helper's code is the same
in both builds [V]. Its outline:

- On Windows NT (`GetVersionEx` platform 2), it lists processes from the registry performance data (the
  "process" and "id process" counters, like Microsoft's TList sample) and lists a process's modules with
  psapi `EnumProcessModules`/`GetModuleFileNameExA`.
- On Windows 9x, it uses the ToolHelp32 snapshot functions.
- It fills an MFC `CMapStringToString` keyed by process id (`"%u"`) with the process name. On NT the
  name comes without an extension; on 9x it ends in `.EXE` or `.DLL`. Module paths are uppercased and
  kept in a sorted `CStringList`.

| Role | Source-tree build | 2.02 build |
|---|---|---|
| Exported entry point | `0x10001000` | `0x10001000` |
| Constructor / Init (loads the APIs dynamically) | `0x10001270` / `0x100012e0` | `0x10001560` / `0x100015d0` |
| NT process list / NT module list | `0x10001430` / `0x10001850` | `0x10001740` / `0x10001b60` |
| 9x process list / 9x module list | `0x10001a30` / `0x10001bf0` | `0x10001d40` / `0x10001f00` |
| File sample checksum | (none) | `0x10001360` |

### Source-tree build: `__FindHackingDll__(char *)`

1. It compares the first 8 bytes of the argument with `"CRCCHECK"` and returns -1 if they differ [V].
   Despite the name, no CRC is computed: the string only works as a password for the call [V].
2. It builds the process list and uppercases each name. If a name equals `NPMON`, `NPMON.EXE`,
   `TSEARCH` or `TSEARCH.EXE` (memory-search and packet-monitor tools), it returns -1 [V].
3. Otherwise it returns 1, and also returns 1 if the process list could not be built [V].

It never calls the module-list half of the helper class [V]. It terminates nothing itself:
`TerminateProcess` appears only in the bundled CRT's exit code [V]. The client quits when the result is not 1.

### 2.02 build: `_FindHackingDll_(void)`

1. It builds the same process list. It has no argument check and no process-name blacklist [V].
2. For a process whose name is exactly `EXPLORER.EXE`, it walks that process's module paths. When
   `path.Mid(8)` equals `HOOK.DLL` and the file's sample checksum is `0x58`, it shows a Korean message
   box ("the system is being shut down because an illegal program is in use") and returns -1 [V].
3. For every other process, it does the same with `path.Mid(10)` equal to `NTPORT.DLL` and checksum
   `0x7a`. The message names the process as a suspected speed hack ("speed bug") [V].
4. Otherwise it returns 1 [V].

Notes:
- The checksum adds together the low 7 bits of three file bytes, at 1/4, 1/2 and 3/4 of the file
  length [V]. It identifies one specific file version rather than any file with that name [I].
- `Mid(n)` keeps the string from position n to the end. `Right(n)` was probably intended [I].
- The `EXPLORER.EXE` branch can only match on Windows 9x, because NT process names have no
  extension [I].

### What changed between the builds

The 2.02 build hunted for two particular injected DLLs. The newer build drops that, and instead
blacklists two tool executables and adds the `"CRCCHECK"` argument check.

## How the client used it (before this branch)

- At start-up, `WinMain` decoded the name `"rd`qbg-ckk"` (each byte + 1 = `search.dll`), loaded it with
  `LoadLibrary`, and called `__FindHackingDll__("CRCCHECK")` through `GetProcAddress`. The client exited
  if the DLL or the export was missing, or the result was not 1.
- On every switch back to the game window (`WM_ACTIVATEAPP`), `WndProc` called the import-linked
  `__FindHackingDll__("CRCCHECK")` again and switched to the quit game mode if it did not return 1.

## Client changes

On branch `feature/client-libraries`, `Client/Wmain.cpp` has the `dllimport` declaration, the start-up
`LoadLibrary`/`GetProcAddress` block and the task-switch call removed. The client no longer loads, links
or needs search.dll. The file-checksum check on task switch (`bCheckImportantFile`) is unchanged.
