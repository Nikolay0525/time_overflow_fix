// time_overflow_fix: Kyubu port of coremaze's Cube-World-System-Time-Overflow-Fix.
//
// The game reads timeGetTime(), which wraps after ~49 days of uptime (the original README says 24 days: the game
// treats the value as signed) and breaks server connections and particles. This hook answers with a time that starts
// at the moment the game starts, so the bug appears after ~24 days of *game* time instead of PC uptime.
//
// The old mod overwrote the game's import slot; Kyubu's hook_import does the same thing and can remove it again.
#include "../common.hpp"

#include <windows.h>

namespace {
DWORD(WINAPI* g_next)() = nullptr;   // the real timeGetTime
CRITICAL_SECTION g_lock;
DWORD g_current = 0;
DWORD g_previous = 0;

DWORD WINAPI get_time() {
    EnterCriticalSection(&g_lock);
    const DWORD now = g_next();
    g_current += now - g_previous;   // unsigned subtraction works across the wrap
    g_previous = now;
    const DWORD result = g_current;
    LeaveCriticalSection(&g_lock);
    return result;
}
}  // namespace

struct TimeOverflowFix {
    explicit TimeOverflowFix(kyubu::Mod& m) {
        InitializeCriticalSection(&g_lock);
        KyubuId id = 0;
        const KyubuResult r = m.api()->hook_import(m.handle(), "winmm.dll", "timeGetTime", reinterpret_cast<void*>(&get_time), 0,
                                                   reinterpret_cast<void**>(&g_next), &id);
        if (r != KY_OK || !g_next) { m.error("time_overflow_fix: could not hook winmm!timeGetTime (result %d)", static_cast<int>(r)); return; }
        g_previous = g_next();
        m.info("time_overflow_fix: timeGetTime hooked");
    }
};

KYUBU_MOD("time_overflow_fix", KY_SIDE_CLIENT, TimeOverflowFix)
