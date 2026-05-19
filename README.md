<p align="center">
  <img src="icon.png" alt="RAMMY icon" width="170">
</p>

<h1 align="center">RAMMY</h1>

<p align="center">
  Aggressive Windows RAM optimization in one clean portable app.
</p>

<p align="center">
  <strong>One click. One executable. More control over memory pressure.</strong>
</p>

## Meet RAMMY

RAMMY is a lightweight Windows memory cleaner built for people who want fast results without opening Task Manager, hunting for processes, or installing a heavy system suite.

It gives you a live view of your RAM, lets you clean memory instantly, and can automatically trigger cleanup when your system reaches the threshold you choose. The interface is minimal, cybercore, and focused: see memory pressure, press `CLEAN`, keep moving.

## Why RAMMY Exists

RAMMY was inspired by two excellent Windows memory tools:

- [IgorMundstein/WinMemoryCleaner](https://github.com/IgorMundstein/WinMemoryCleaner)
- [henrypp/memreduct](https://github.com/henrypp/memreduct)

The goal with RAMMY was to take that idea further: a more aggressive cleaner, a sharper portable experience, and a modern UI that feels built for quick everyday use.

RAMMY is designed to push harder across multiple cleanup paths, targeting more memory areas than a basic working-set trim. It combines process cleanup, Windows memory list purging, file cache cleanup, registry cache reconciliation, physical page combining, and repeated purge cycles to optimize RAM more deeply.

## What It Does

- Shows live RAM usage, available memory, and cache memory.
- Cleans memory manually with the `CLEAN` button.
- Automatically cleans when RAM usage reaches your selected threshold.
- Runs timer-based cleanup from 1 to 180 minutes.
- Can start with Windows.
- Minimizes to the system tray.
- Ships as a single portable `.exe`.
- Embeds its icon, background, and visual assets directly inside the app.

## Technical Deep Dive: How It Works

RAMMY is built around a multi-pass cleanup pipeline. Instead of pressing one Windows API and calling it done, RAMMY chains several memory optimization paths together, then repeats the aggressive purge phase and measures memory again after Windows has had a moment to settle.

The main cleanup flow lives in `ejecutarLimpieza()`. It runs two full passes, then a final intensive purge cycle. During cleanup RAMMY lowers its own cleanup thread priority so the desktop stays responsive while memory pressure is being reduced.

| RAMMY Function | Target Area | Windows API / Method | What It Does |
| --- | --- | --- | --- |
| `limpiarProcesosUltraAgresivo()` | Process working sets | `EmptyWorkingSet`, `SetProcessWorkingSetSize`, `SetProcessWorkingSetSizeEx` | Trims memory held by running processes, asking Windows to release non-essential pages from each accessible process. |
| `limpiarListasMemoriaWindows()` | Windows memory lists | `NtSetSystemInformation(SystemMemoryListInformation)` | Runs core memory-list commands: empty working sets, flush modified pages, purge standby list, and purge low-priority standby list. |
| `limpiarSystemFileCache()` | System file cache | `NtSetSystemInformation(SystemFileCacheInformationEx)` | Reduces memory used by Windows file caching through the native NT cache control path. |
| `limpiarFileCacheApi()` | File cache | `SetSystemFileCacheSize` | Uses the documented Win32 file cache API as a second cache-cleaning path alongside the NT call. |
| `limpiarRegistryCache()` | Registry cache | `NtSetSystemInformation(SystemRegistryReconciliationInformation)` | Requests registry cache reconciliation so Windows can release registry-related cached memory where possible. |
| `combinarMemoriaFisica()` | Duplicate physical pages | `NtSetSystemInformation(SystemCombinePhysicalMemoryInformation)` | Asks Windows to combine identical physical memory pages, reducing duplicated RAM usage when supported. |
| `limpiarCacheVolumenes()` | Volume file buffers | `FlushFileBuffers` | Flushes fixed and removable drive buffers so pending file cache data can be committed and released more cleanly. |
| `cicloAgresivoFlush()` | Repeated purge cycle | Modified list flush, standby purge, low-priority standby purge, working set empty | Runs repeated NT cleanup calls in sequence so freshly flushed pages can be purged instead of staying cached. |
| `cicloFinalIntensivo()` | Final memory sweep | 12-command NT purge sequence | Performs a final interleaved purge after the main passes to catch memory that reappears while Windows is reconciling lists. |
| `medirRamEstable()` | Result measurement | `GlobalMemoryStatusEx` | Samples available RAM twice and reports the best stable reading after cleanup settles. |

### Memory Areas RAMMY Targets

- **Working Set:** RAM currently attached to running processes. RAMMY trims accessible processes so unused pages can return to the system.
- **Modified Page List:** Dirty pages waiting to be written back. RAMMY flushes this list so pages can move toward standby/free states.
- **Standby List:** Cached pages Windows keeps ready for reuse. RAMMY purges both normal and low-priority standby pages.
- **System File Cache:** Memory used by Windows to cache file data. RAMMY hits this through both NT and Win32 paths.
- **Registry Cache:** Cached registry hive data. RAMMY requests reconciliation through the native system information API.
- **Combined Page List:** Duplicate physical pages Windows can merge. RAMMY asks Windows to combine pages where the OS supports it.
- **Volume Buffers:** Pending filesystem buffers. RAMMY flushes drive buffers to help cache cleanup complete more effectively.

### Why The Cleanup Is More Aggressive

Many memory cleaners focus mainly on working sets or standby lists. RAMMY goes wider: it trims process memory, clears multiple Windows memory lists, targets file cache through two different API routes, touches registry cache, triggers page combining, flushes volume buffers, then runs repeated purge cycles.

That layered approach is why RAMMY is designed to feel stronger after heavy workloads, gaming sessions, browsers, editors, launchers, and apps that leave cached memory behind.

The result is a more aggressive optimization pass built for users who want RAM cleanup to feel immediate, visible, and easy to control.

## Portable By Design

RAMMY is made to be carried around as one file.

Download `RAMMY.exe` from Releases, place it anywhere, and run it. No installer. No extra assets. No setup folder.

## Best For

- Freeing unused memory after heavy apps or games.
- Quickly reducing memory pressure before launching another workload.
- Keeping a small cleanup tool in the tray.
- Users who want simple controls without losing advanced cleanup behavior.

## Notes

Some cleanup actions may request administrator permission so RAMMY can access the Windows memory cleanup APIs properly.

RAMMY does not replace Windows memory management. It gives you a direct, aggressive cleanup tool for moments when you want more control, more visibility, and a faster way to recover available RAM.
