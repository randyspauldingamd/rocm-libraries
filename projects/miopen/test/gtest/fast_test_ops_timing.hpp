#pragma once

#include <chrono>
#include <iomanip>

#define FTO_TIMING 1

namespace { using sc = std::chrono::steady_clock; }
#undef tomillis
#define tomillis(__DUR) (0.001 * std::chrono::duration_cast<std::chrono::microseconds>(__DUR).count())

#undef FTO_MS_TOKEN
#define FTO_MS_TOKEN mS_sTaRt
#undef FTO_MS_START
#undef coutmsstart
#undef coutmsbase
#undef coutmsnow
#undef coutms
#undef coutms2
#undef coutmsreset
#undef coutmsreset2
#if FTO_TIMING
#define coutmsstart(__TP) auto __TP = sc::now()
#define FTO_MS_START() coutmsstart(FTO_MS_TOKEN)
#define coutmsbase(__TOK, __TP, __PRE) std::cout << __PRE << "ms[ " << std::setw(19) << __TOK << "]: " << std::setw(12) << tomillis(sc::now() - __TP) << std::endl
#define coutmsnow(__TP) __TP = sc::now()
#else
#define coutmsstart(__TP) ((void)0)
#define FTO_MS_START() ((void)0)
#define coutmsbase(__TOK, __TP, __PRE) ((void)0)
#define coutmsnow(__TP) ((void)0)
#endif
#define FTO_MS_NOW() coutmsnow(FTO_MS_TOKEN)
#define coutms(__TOK, __TP) coutmsbase(__TOK, __TP, "")
#define FTO_MS_COUT(__TOK) coutms(__TOK, FTO_MS_TOKEN)
#define coutms2(__TOK, __TP) coutmsbase(__TOK, __TP, "            ")
#define FTO_MS_COUT2(__TOK) coutms2(__TOK, FTO_MS_TOKEN)
#define coutmsreset(__TOK, __TP) {coutms(__TOK, __TP); FTO_MS_NOW();}
#define FTO_MS_RESTART(__TOK) coutmsreset(__TOK, FTO_MS_TOKEN)
#define coutmsreset2(__TOK, __TP) {coutms2(__TOK, __TP); FTO_MS_NOW();}
#define FTO_MS_RESTART2(__TOK) coutmsreset2(__TOK, FTO_MS_TOKEN)
