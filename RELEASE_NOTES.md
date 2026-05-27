# RAMMY v3.1

- Fixed Windows startup by registering RAMMY with a scheduled task, so elevated startup works after reboot.
- Fixed tray right-click menu language sync when switching between English and Spanish.
- Localized tray cleanup status text for cleaning, errors, and freed memory.
- Kept the tray clean action available without opening the main window.
- Kept the dynamic tray icon showing current RAM usage.
- Included fresh setup and portable release builds.

# RAMMY v2.0.0

- Rebuilt RAMMY from the old C++ project into a Tauri v2 desktop app.
- Added a React, TypeScript, Vite, and TailwindCSS frontend.
- Added native Windows RAM cleanup through Rust commands.
- Added manual memory optimization from the app UI.
- Added automatic cleanup by timer.
- Added automatic cleanup by RAM usage threshold.
- Added Windows startup support with hidden tray launch.
- Added system tray support for showing, hiding, quitting, and cleaning RAM.
- Added a dynamic tray icon that displays current RAM usage.
- Added native notification after automatic cleanup.
- Added two independent visual themes: Glass and Flanishy.
- Added Glass transparency control.
- Added bilingual UI support for English and Spanish.
- Added installer and portable builds.
- Added FOSS metadata, README, MIT license, and reproducible build instructions.
