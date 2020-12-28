//
// Created by XingfengYang on 2020/12/23.
//

#ifndef SYNESTIAOS_ASSERT_H
#define SYNESTIAOS_ASSERT_H

void __assert_fail(const char *file, int line, const char *failedExpr);

void __assert_fail_msg(const char *file, int line, const char *failedExpr, const char *msg);

#define DEBUG_ASSERT(x)                            \
    do {                                           \
        if (!(x)) {                                \
            __assert_fail(__FILE__, __LINE__, #x); \
        }                                          \
    } while (0)

#define DEBUG_ASSERT_WITH_MSG(x, msg)                       \
    do {                                                    \
        if (!(x)) {                                         \
            __assert_fail_msg(__FILE__, __LINE__, #x, msg); \
        }                                                   \
    } while (0)


#endif//SYNESTIAOS_ASSERT_H
