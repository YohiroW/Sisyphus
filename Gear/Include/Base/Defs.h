#pragma once

#define BEGIN_NAMESPACE_GEAR namespace Gear {
#define END_NAMESPACE  }

#if _DEBUG

#define ENABLE_LOGGING 1

#define LOG_TO_FILE 1

#endif

#define FORCEINLINE __forceinline
#define INLINE __inline

// To avoid warning
#define assertf(exp, msg) assert(((void)msg, exp))