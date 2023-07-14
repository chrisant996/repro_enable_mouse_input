// See Readme.txt.

#include <stdio.h>
#include <windows.h>

static BOOL WINAPI CtrlBreakHandler(DWORD dwCtrlType);
static void OnKeyEvent(KEY_EVENT_RECORD& r);
static void OnMouseEvent(MOUSE_EVENT_RECORD& r);

static DWORD s_num = 0;

int _cdecl main(int argc, char const** argv)
{
    argv++, argc--;

    SetConsoleCtrlHandler(CtrlBreakHandler, true/*Add*/);

    // Get std handles.
    DWORD in_mode;
    DWORD out_mode;
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(in, &in_mode);
    GetConsoleMode(out, &out_mode);

    // Initialize default input mode.
    DWORD force_mode = in_mode;
    force_mode &= ~(ENABLE_PROCESSED_INPUT|ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT);
    force_mode |= (ENABLE_QUICK_EDIT_MODE);

    // Options.
    bool verbose = false;
    for (int i = 0; i < argc; ++i)
    {
        if (strcmp(argv[i], "--verbose") == 0)
            verbose = true;
        else if (argv[i][0] >= '0' && argv[i][0] <= '9')
            force_mode = atoi(argv[i]);
    }

    // Enable VT escape code processing.
    SetConsoleMode(out, out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    printf("Output mode 0x%x.\n", out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    // Apply input mode.
    SetConsoleMode(in, force_mode);
    printf(" Input mode 0x%x.\n", force_mode);

    // Verify input mode.
    DWORD verify_mode;
    GetConsoleMode(in, &verify_mode);
    if (force_mode != verify_mode)
        printf("Mode flags mismatch:  expected 0x%04.4x, actual 0x%04.4x.\n(0x0080 is ignorable, but is surprising.)\n", force_mode, verify_mode);

    // Report what mode is being used.
    static const struct { const char* name; DWORD flag; } c_modes[] =
    {
        { "ENABLE_PROCESSED_INPUT",         0x0001 },
        { "ENABLE_LINE_INPUT",              0x0002 },
        { "ENABLE_ECHO_INPUT",              0x0004 },
        { "ENABLE_WINDOW_INPUT",            0x0008 },
        { "ENABLE_MOUSE_INPUT",             0x0010 },
        { "ENABLE_INSERT_MODE",             0x0020 },
        { "ENABLE_QUICK_EDIT_MODE",         0x0040 },
        { "ENABLE_EXTENDED_FLAGS",          0x0080 },
        { "ENABLE_AUTO_POSITION",           0x0100 },
        { "ENABLE_VIRTUAL_TERMINAL_INPUT",  0x0200 },
    };
    for (const auto& entry : c_modes)
    {
        if (force_mode & entry.flag)
            printf("    %s\n", entry.name);
    }

    // Loop, reading one input record at a time.
    puts("Awaiting input...  (^C to end)");
    while (true)
    {
        // Hold Ctrl+Alt to disable Quick Edit and allow mouse input.
        const bool use_quick_edit = !(GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_MENU) < 0);
        const bool was_quick_edit = !!(force_mode & ENABLE_QUICK_EDIT_MODE);
        if (use_quick_edit != was_quick_edit)
        {
            if (was_quick_edit)
                force_mode &= ~ENABLE_QUICK_EDIT_MODE;
            if (use_quick_edit)
                force_mode |= ENABLE_QUICK_EDIT_MODE;
            if (verbose)
                printf("Updating console input mode to 0x%d.\n", force_mode);
        }

        // Set the console mode to control what input is accepted.
        SetConsoleMode(in, force_mode);

        // Wait for input matching the specified input mode.
        WaitForSingleObject(in, INFINITE);

        // Read input record.
        DWORD count;
        INPUT_RECORD record;
        ReadConsoleInputW(in, &record, 1, &count);

        // Process the input record.
        switch (record.EventType)
        {
        case KEY_EVENT:     OnKeyEvent(record.Event.KeyEvent); break;
        case MOUSE_EVENT:   OnMouseEvent(record.Event.MouseEvent); break;
        }

        // ^C breaks out.
        if (record.EventType == KEY_EVENT)
        {
            if (record.Event.KeyEvent.uChar.UnicodeChar == 3 && record.Event.KeyEvent.wVirtualKeyCode == 0x43)
            {
                puts("\n^C");
                break;
            }
        }
    }

    // Restore console modes.
    SetConsoleMode(out, out_mode);
    SetConsoleMode(in, in_mode);
    puts("Restored original console modes.");

    return 0;
}

static BOOL WINAPI CtrlBreakHandler(DWORD dwCtrlType)
{
    return (dwCtrlType == CTRL_C_EVENT);
}

static void OnKeyEvent(KEY_EVENT_RECORD& r)
{
    // Sometimes conhost can send through ALT codes, with the resulting Unicode
    // code point in the Alt key-up event.
    if (!r.bKeyDown && r.wVirtualKeyCode == VK_MENU && r.uChar.UnicodeChar)
    {
        r.bKeyDown = TRUE;
        r.dwControlKeyState = 0;
    }

    // Only interested in key down.
    if (!r.bKeyDown)
        return;

    const int key_char = r.uChar.UnicodeChar;
    const int key_vk = r.wVirtualKeyCode;
    const int key_sc = r.wVirtualScanCode;
    const int key_flags = r.dwControlKeyState;

    // Ignore unaccompanied Alt/Ctrl/Shift/Windows key presses.
    if (key_vk == VK_MENU || key_vk == VK_CONTROL || key_vk == VK_SHIFT || key_vk == VK_LWIN || key_vk == VK_RWIN)
        return;

    printf("%5u:  key event:  %c%c%c %c%c  ctrl=0x%08.8x  char=0x%04.4x  vk=0x%04.4x  scan=0x%04.4x",
            ++s_num,
            (key_flags & SHIFT_PRESSED) ? 'S' : '_',
            (key_flags & LEFT_CTRL_PRESSED) ? 'C' : '_',
            (key_flags & LEFT_ALT_PRESSED) ? 'A' : '_',
            (key_flags & RIGHT_ALT_PRESSED) ? 'A' : '_',
            (key_flags & RIGHT_CTRL_PRESSED) ? 'C' : '_',
            key_flags,
            key_char,
            key_vk,
            key_sc);
    if (key_char >= 0x20 && key_char <= 0x7e)
        printf("  \"%c\"", char(key_char));
    puts("");
}

static void OnMouseEvent(MOUSE_EVENT_RECORD& r)
{
    const int key_flags = r.dwControlKeyState;

    printf("%5u:  mouse event:  %u,%u  %c%c%c %c%c  ctrl=0x%08.8x  buttons=0x%04.4x  event=0x%04.4x\n",
            ++s_num,
            r.dwMousePosition.X,
            r.dwMousePosition.Y,
            (key_flags & SHIFT_PRESSED) ? 'S' : '_',
            (key_flags & LEFT_CTRL_PRESSED) ? 'C' : '_',
            (key_flags & LEFT_ALT_PRESSED) ? 'A' : '_',
            (key_flags & RIGHT_ALT_PRESSED) ? 'A' : '_',
            (key_flags & RIGHT_CTRL_PRESSED) ? 'C' : '_',
            key_flags,
            r.dwButtonState,
            r.dwEventFlags);
}
