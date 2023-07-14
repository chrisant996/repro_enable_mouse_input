# Issue

Unless some `WriteConsoleW()` call happens between two `ReadConsoleInputW()` calls, using
`SetConsoleMode()` to set/clear the `ENABLE_MOUSE_INPUT` mode flag doesn't take effect.

- This is a regression in Terminal 1.18.1462.0.
- The problem does not exist in Terminal 1.17.11461.0.

## Repro Steps

The repo includes a pre-built code-signed copy of the Repro.exe executable.

1. Start Windows Terminal 1.18
2. Run the `Repro.exe` program from this repo
3. Move the mouse over the terminal window; you should see no output
4. Click the primary mouse button on the terminal window; you should see no output
5. Hold <kbd>Ctrl</kbd>+<kbd>Alt</kbd> and move the mouse or click the mouse button

EXPECTED:  You should see output acknowledging that ReadConsoleInputW received mouse input records.
ACTUAL:
- Terminal 1.18 -- there is not output (incorrect behavior).
- Terminal 1.17 -- the output reports received mouse input records (correct behavior).

## Observe The Impact of `WriteConsoleW()`

If in step 2 you instead run `Repro.exe --verbose` then the program prints verbose output
acknowleding when <kbd>Ctrl</kbd>+<kbd>Alt</kbd> are pressed or released and indicating the new
console mode being used.

The additional verbose output line eliminates the repro, and makes the `ENABLE_MOUSE_INPUT` change
properly affect the `WaitForSingleObject(in, INFINITE)` call.

So, in Terminal 1.18, unless some `WriteConsoleW()` call happens between two `ReadConsoleInputW()`
calls, using `SetConsoleMode()` to set/clear `ENABLE_MOUSE_INPUT` doesn't take effect.
