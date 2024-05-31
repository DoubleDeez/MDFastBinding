// 1047 Custom - Enhance MDFastBinding profiling readability
#pragma once

#include "ProfilingDebugging/CpuProfilerTrace.h"

#if defined(MDFASTBINDING_CONDENSED_PROFILING) && MDFASTBINDING_CONDENSED_PROFILING

// Trace a scoped cpu timing event providing the current function along with specified text appended to it
// Note: This macro has a larger overhead compared to TRACE_CPUPROFILER_EVENT_SCOPE_TEXT()
#define MD_TRACE_CPUPROFILER_EVENT_SCOPE_FUNCTION_TEXT(Text) TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*FString::Printf(TEXT("%s - %s"), ANSI_TO_TCHAR(__FUNCTION__), Text));

#endif