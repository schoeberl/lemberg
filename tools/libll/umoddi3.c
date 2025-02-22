/* ===-- umoddi3.c - Implement __umoddi3 -----------------------------------===
 *
 *                    The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 *
 * This file implements __umoddi3 for the compiler_rt library.
 *
 * ===----------------------------------------------------------------------===
 */

#include "int_lib.h"

du_int __udivmoddi4(du_int a, du_int b, du_int* rem);

/* Returns: a % b */

du_int
__umoddi3(du_int a, du_int b)
{
    du_int r;
    __udivmoddi4(a, b, &r);
    return r;
}
