/*
 * CLI tracer wrappers
 * Written by Michael Barabanov (baraban@fsmlabs.com)
 * Copyright (C) FSMLabs, 2000
 * Released under the terms of the GNU GPL.
 */

#ifndef __RTL_TRACEWRAP_H__
#define __RTL_TRACEWRAP_H__

#undef rtl_hard_savef_and_cli
#define rtl_hard_savef_and_cli(x) do { __rtl_hard_savef_and_cli(x); rtl_trace2(RTL_TRACE_HARD_SAVEF_AND_CLI, x); } while (0)

#undef rtl_hard_save_flags
#define rtl_hard_save_flags(x) do { __rtl_hard_save_flags(x); rtl_trace2(RTL_TRACE_HARD_SAVE_FLAGS, x); } while (0)

#undef rtl_hard_restore_flags
#define rtl_hard_restore_flags(x) do { rtl_trace2(RTL_TRACE_HARD_RESTORE_FLAGS, x); __rtl_hard_restore_flags(x); } while (0)

#undef rtl_hard_cli
#define rtl_hard_cli() do { rtl_trace2(RTL_TRACE_HARD_CLI, 0); __rtl_hard_cli(); } while (0)

#undef rtl_hard_sti
#define rtl_hard_sti() do { rtl_trace2(RTL_TRACE_HARD_STI, 0); __rtl_hard_sti(); } while (0)

#undef rtl_spin_lock
#define rtl_spin_lock(x) do { spin_lock(x); rtl_trace2(RTL_TRACE_SPIN_LOCK, (long) x); } while (0)

#undef rtl_spin_unlock
#define rtl_spin_unlock(x) do { rtl_trace2(RTL_TRACE_SPIN_UNLOCK, (long) x); spin_unlock(x); } while (0)

#endif
