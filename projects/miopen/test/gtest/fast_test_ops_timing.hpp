#pragma once

#include <chrono>
#include <iomanip>

#define FTO_TIMING 1
#define FTO_USE_DRIVE_CACHE 0

namespace { using sc = std::chrono::steady_clock; }
#undef tomillis
#define tomillis(__DUR) (0.001 * std::chrono::duration_cast<std::chrono::microseconds>(__DUR).count())

#undef MIOPEN_MS_TOKEN
#define MIOPEN_MS_TOKEN mS_sTaRt
#undef MIOPEN_MS_START
#undef coutmsstart
#undef coutmsbase
#undef coutmsnow
#undef coutms
#undef coutms2
#undef coutmsreset
#undef coutmsreset2
#if FTO_TIMING
#define coutmsstart(__TP) auto __TP = sc::now()
#define MIOPEN_MS_START() coutmsstart(MIOPEN_MS_TOKEN)
#define coutmsbase(__TOK, __TP, __PRE) std::cout << __PRE << "ms[" << std::setw(20) << __TOK << "]: " << std::setw(12) << tomillis(sc::now() - __TP) << std::endl
#define coutmsnow(__TP) __TP = sc::now()
#else
#define coutmsstart(__TP) ((void)0)
#define MIOPEN_MS_START() ((void)0)
#define coutmsbase(__TOK, __TP, __PRE) ((void)0)
#define coutmsnow(__TP) ((void)0)
#endif
#define MIOPEN_MS_NOW() coutmsnow(MIOPEN_MS_TOKEN)
#define coutms(__TOK, __TP) coutmsbase(__TOK, __TP, "")
#define MIOPEN_MS_COUT(__TOK) coutms(__TOK, MIOPEN_MS_TOKEN)
#define coutms2(__TOK, __TP) coutmsbase(__TOK, __TP, "            ")
#define MIOPEN_MS_COUT2(__TOK) coutms2(__TOK, MIOPEN_MS_TOKEN)
#define coutmsreset(__TOK, __TP) {coutms(__TOK, __TP); MIOPEN_MS_NOW();}
#define MIOPEN_MS_RESET(__TOK) coutmsreset(__TOK, MIOPEN_MS_TOKEN)
#define coutmsreset2(__TOK, __TP) {coutms2(__TOK, __TP); MIOPEN_MS_NOW();}
#define MIOPEN_MS_RESET2(__TOK) coutmsreset2(__TOK, MIOPEN_MS_TOKEN)
