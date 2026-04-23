# Parse

You are looking at something on screen. You are not sure what it is. Maybe it is a cryptic error message, maybe it is a chat your coworker sent that reads like a ransom note, maybe it is a dashboard with six overlapping labels. Parse is a Windows tray app that turns "what the hell is this" into a small popup that actually answers the question.

Press a hotkey, drag a rectangle, read the explanation. Ask a followup if the first pass was unsatisfying. Close the popup when you are done. That is the whole app.

## What it does

- Lives in the system tray. No main window, no splash screen, no "welcome back" dialog.
- Binds a global hotkey (default `Ctrl+Shift+/`, falls back to `Ctrl+Shift+Alt+P` if something else stole the first one).
- Dims the screen across all monitors and gives you a crosshair to drag a rectangle with.
- Captures the selected region via good old `BitBlt`, encodes it as PNG, and streams it to OpenAI's vision-capable chat API.
- Shows the response in a small dark popup anchored near what you captured. Streams tokens as they arrive.
- Lets you ask followup questions in the same conversation - the image stays in context.
- Quietly drops a copy of every screenshot into `~/projects/Reliquary/inbox/files/parse-<timestamp>.png`, because that is what the author wanted and nobody else is ever going to run this.

Press `Esc` to dismiss. Double-click the tray icon to capture without touching the keyboard. Right-click the tray icon for About / Quit.

## Requirements

- Windows 10 1809+ or Windows 11, x64
- Visual Studio 2022 with the "Desktop development with C++" workload
- Qt 6.7 or newer for MSVC 2022 64-bit (developed against 6.8.3)
- CMake 3.24 or newer
- Windows 11 SDK
- An OpenAI API key with access to `gpt-4o` (or whatever vision model you swap in by editing `kModel` in `OpenAIClient.cpp`)

## Build

Point CMake at your Qt install:

```
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64"

cmake --build build --config Release
```

Then stage the Qt runtime DLLs next to the binary, because Windows still does not know what a rpath is:

```
C:\Qt\6.8.3\msvc2022_64\bin\windeployqt.exe ^
    --release --no-translations --no-system-d3d-compiler ^
    --no-opengl-sw build\Release\parse.exe
```

Run `build\Release\parse.exe`. A tray icon appears. That is the success state.

## Configuration

The API key is read from the `OPENAI_API_KEY` environment variable on startup. Yes, env vars for desktop apps are unfashionable. This is a one-person tool and the author wrote the user themselves, so the usability critique is under advisement.

```
setx OPENAI_API_KEY sk-...
```

Open a new shell after `setx` or the old one will not see the change. If the variable is missing when Parse launches you will get a blunt error dialog and the app will exit.

To rotate the key: update the env var, restart Parse. No UI for this by design.

## Usage

1. Press `Ctrl+Shift+/`. The screen dims and your cursor becomes a crosshair.
2. Drag a rectangle around the thing you want explained. Release.
3. A popup appears near the selection. Tokens start streaming in within a second or two.
4. Type a followup and press Enter if you have one. Press `Esc` to close.

That is the entire UX. If you are expecting more, you are using the wrong app.

## Known limitations

- **Hardware-overlay content captures as black.** Protected video, some Chromium windows with hardware compositing, certain fullscreen games. The fix is the Windows Graphics Capture API, which is not implemented because shipping beats perfection.
- **Mixed-DPI monitors.** The rect-to-physical-pixel math assumes uniform DPR. If your laptop is at 150% and your external is at 100% and your selection crosses the boundary, the capture will be off. Do not do that.
- **No offline mode.** Requires internet and a valid OpenAI API key. If the API is down, so is the app.
- **The hotkey is hardcoded.** Change it by editing `AppController::initialize` and rebuilding. A preferences UI is not on the roadmap.
- **Rate limits are not handled gracefully.** You get a blunt "rate limited" error message. Wait and retry.

## Layout

```
src/
  main.cpp            - entry point, DPI awareness, Qt setup
  SingleInstance.*    - named-pipe lock so only one copy runs per user
  HotkeyManager.*     - Win32 RegisterHotKey behind a Qt signal
  RegionSelector.*    - the dimmed overlay with the drag rectangle
  Capturer.*          - BitBlt, DPR math, PNG encoding
  OpenAIClient.*      - streaming chat completions via SSE
  ResultPopup.*       - the dark popup with transcript and input
  AppController.*     - wiring: tray, hotkey, capture, popup, conversation
resources/
  parse.qrc           - Qt resource file
  icons/tray.png      - 32x32 tray icon
app.manifest          - per-monitor DPI v2 + Win10/11 compatibility
CMakeLists.txt
```

## License

MIT. See `LICENSE.txt`. Copyright Locke Werks, 2026.
