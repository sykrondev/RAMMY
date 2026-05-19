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

## Under The Hood

RAMMY uses several Windows memory cleanup strategies together instead of relying on only one.

It can trim process working sets with `EmptyWorkingSet`, `SetProcessWorkingSetSize`, and `SetProcessWorkingSetSizeEx`. It also calls Windows internal memory list commands through `NtSetSystemInformation`, including emptying working sets, flushing the modified page list, purging the standby list, and purging the low-priority standby list.

RAMMY also targets system-level memory pressure by cleaning the system file cache, calling the Win32 file cache API, reconciling registry cache data, combining physical memory pages where Windows allows it, flushing volume buffers, and running a final intensive purge cycle to catch memory that returns during cleanup.

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
