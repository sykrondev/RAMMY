# RAMMY

RAMMY is a lightweight Windows memory optimizer built with React, TypeScript, Vite, TailwindCSS, and Tauri v2.

## Features

- Manual RAM cleanup from the main window.
- Auto-clean by timer or RAM usage threshold.
- System tray support with hide/show behavior.
- Localized tray menu action to clean RAM without opening the app.
- Dynamic tray icon that shows current RAM usage.
- Native Windows startup option with hidden tray start through a scheduled task.
- Glass and Flanishy visual themes.
- Glass transparency control.
- English and Spanish UI.
- Notification after automatic cleanup with the freed memory amount.

## Development

Install dependencies:

```powershell
npm install
```

Run in development:

```powershell
npm run tauri dev
```

Build release binaries:

```powershell
npm run tauri build
```

## Project Structure

- `src/useRammyLogic.ts`: shared app logic, settings, RAM state, auto-clean, translations, and notifications.
- `src/themes/GlassTheme.tsx`: Glass UI.
- `src/themes/FlanishyTheme.tsx`: Flanishy UI.
- `src-tauri/src/lib.rs`: native Windows RAM cleanup, startup registration, tray menu, and dynamic tray icon.

## License

MIT
