<p align="center"><img src="icon.png" alt="RAMMY icon" width="170"></p>
<h1 align="center">RAMMY</h1>
<p align="center">Aggressive Windows RAM optimization in one clean portable app.</p>
<p align="center"><strong>One click. One executable. More control over memory pressure.</strong></p>

## Meet RAMMY
RAMMY is a lightweight Windows memory cleaner for people who want fast results without opening Task Manager, chasing processes, or installing a heavy system suite. It gives you live RAM visibility, instant cleanup, automatic threshold cleanup, and timer cleanup in a compact cybercore interface.

## Inspiration
RAMMY was inspired by two excellent Windows memory projects:
- [IgorMundstein/WinMemoryCleaner](https://github.com/IgorMundstein/WinMemoryCleaner)
- [henrypp/memreduct](https://github.com/henrypp/memreduct)

The idea behind RAMMY is simple: keep the portable simplicity, make the interface sharper, and push cleanup harder. RAMMY targets more than a basic working-set trim by combining process cleanup, Windows memory-list cleanup, file-cache cleanup, registry-cache reconciliation, page combining, volume flushing, and repeated purge cycles.

## What It Does
- Shows live RAM usage, available memory, and cache memory.
- Cleans memory manually with the `CLEAN` button.
- Automatically cleans when RAM usage reaches your selected threshold.
- Runs timer-based cleanup from 1 to 180 minutes.
- Can start with Windows and minimize to the system tray.
- Ships as a single portable `.exe` with its visual assets embedded.

## RAMMY Memory Engine
RAMMY uses a layered cleanup engine. Each pass targets a different kind of memory pressure, then the final sweep repeats the most aggressive commands after Windows has started moving pages between internal lists.

| Function | Target | Method | Purpose |
| --- | --- | --- | --- |
| `cleanProcessesUltraAggressive()` | Process working sets | `EmptyWorkingSet`, `SetProcessWorkingSetSize`, `SetProcessWorkingSetSizeEx` | Trims accessible processes so unused pages can return to Windows. |
| `cleanWindowsMemoryLists()` | Internal memory lists | `NtSetSystemInformation(SystemMemoryListInformation)` | Flushes modified pages and purges standby / low-priority standby lists. |
| `cleanSystemFileCache()` | System file cache | `NtSetSystemInformation(SystemFileCacheInformationEx)` | Asks the NT cache manager to reduce system file-cache pressure. |
| `cleanFileCacheApi()` | File cache | `SetSystemFileCacheSize` | Uses the documented Win32 cache route as an extra cache-trimming path. |
| `cleanRegistryCache()` | Registry cache | `NtSetSystemInformation(SystemRegistryReconciliationInformation)` | Requests registry-cache reconciliation where Windows supports it. |
| `combinePhysicalMemory()` | Duplicate physical pages | `NtSetSystemInformation(SystemCombinePhysicalMemoryInformation)` | Lets Windows merge identical memory pages to reduce duplication. |
| `flushVolumeCaches()` | Drive buffers | `FlushFileBuffers` | Commits pending volume buffers so cached data can settle faster. |
| `aggressiveFlushCycle()` | Repeated purge loop | Modified-list flush + standby purges + working-set empty | Repeats cleanup commands so newly flushed pages can be released. |
| `finalIntensiveCycle()` | Final sweep | 12 interleaved NT cleanup calls | Catches memory that returns while Windows is reconciling state. |
| `measureStableRam()` | Result reading | `GlobalMemoryStatusEx` | Samples RAM after cleanup so the displayed result is less noisy. |

RAMMY is more aggressive because it stacks these strategies instead of depending on one cleanup button behind the scenes. That makes it useful after games, browsers, editors, launchers, and other workloads that leave large caches or idle working sets behind.

## Portable By Design
Download `RAMMY.exe` from Releases, place it anywhere, and run it. No installer. No extra assets. No setup folder.

## Best For
- Freeing unused memory after heavy apps or games.
- Reducing memory pressure before launching another workload.
- Keeping a small cleanup tool in the tray.
- Users who want simple controls with advanced cleanup behavior.

## Notes
Some cleanup actions may request administrator permission so RAMMY can access the Windows memory cleanup APIs properly. RAMMY does not replace Windows memory management; it gives you a direct, aggressive cleanup tool for moments when you want more control, more visibility, and a faster way to recover available RAM.
