#include <windows.h>
/* Volatile absolute pointer deliberately requires DIR64 relocation. */
static const char message[] = "Prism canonical probe: PASS\n";
static const char *volatile relocated_message = message;
DWORD PrismEntry(void) {
    OutputDebugStringA(relocated_message);
    return 0x50415353u;
}
