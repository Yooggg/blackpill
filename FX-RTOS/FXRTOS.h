#define FX_METADATA(data) 
#define FX_INTERFACE(file) <stdint.h> 
#ifndef _HAL_INIT_ARMv7M_LIB_HEADER_
#define _HAL_INIT_ARMv7M_LIB_HEADER_

/** 
  ******************************************************************************
  *  @file   CortexM/init/hal_init.h
  *  @brief  HAL initialization.
  *
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

//!
//! User application.
//! It is called by HAL after system fully initialized at SPL = LOW level in
//! context of first thread.
//!
void fx_app_init(void);

//!
//! Non-returning function used from user code in order to start scheduling. 
//!
void fx_kernel_entry(void);

FX_METADATA(({ interface: [HAL_INIT, ARMv7M_LIB] }))

FX_METADATA(({ options: [                                               
    HAL_INIT_INTR_STACK_SIZE: {                                                        
        type: int, range: [0x400, 0xffffffff], default: 0x1000,                     
        description: "Size of the interrupt stack (in bytes)."}]}))

#endif
#ifndef _CFG_OPTIONS_STD_CM4_HEADER_
#define _CFG_OPTIONS_STD_CM4_HEADER_

/** 
  ******************************************************************************
  *  @file   standard-cortex-m4-options.h
  *  @brief  Kernel options.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#define FX_SCHED_ALG_PRIO_NUM 32
#define FX_TIMER_THREAD_PRIO 1                              
#define FX_TIMER_THREAD_STACK_SIZE 0x400
#define HAL_INIT_INTR_STACK_SIZE 0x400
#define RTL_MEM_POOL_MAX_CHUNK 15
#define HW_PIT_TICK_PERIOD 100000

#ifndef __IAR_SYSTEMS_ASM__
FX_METADATA(({ interface: [CFG_OPTIONS, STANDARD_CORTEX_M4] }))
#endif

#endif
#ifndef _LANG_TYPES_OS_186_HEADER_
#define _LANG_TYPES_OS_186_HEADER_

/** 
  ******************************************************************************
  *  @file   lang_types.h
  *  @brief  Common language extensions, default headers and useful macros.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

//
// These standard headers are required for all FX-RTOS distributions.
//
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include FX_INTERFACE(CFG_OPTIONS)

#define FX_STATUS_OK 0

//!
//! Error checking is disabled.
//!
#if (!defined LANG_ASSERT_ERROR_CHECKING_TYPE) || (LANG_ASSERT_ERROR_CHECKING_TYPE == 0)
#define lang_param_assert(assert, errcode) 

//!
//! Classic error checking policy.
//!
#elif (LANG_ASSERT_ERROR_CHECKING_TYPE == 1)
#define lang_param_assert(assert, err_code) if (!(assert)) {return (err_code);}

//!
//! Centralized error checking policy: user function call on error.
//!
#elif (LANG_ASSERT_ERROR_CHECKING_TYPE == 2)

//!
//! User-supplied function for error handling.
//!
void fx_error_catch(const char* func, const char* err_code_str, int err_code);

//!
//! Helper macro to get error code as a string. 
//!
#define lang_param_assert_return_str(cond, err_code_str, err_code) \
    if (!(cond)) { fx_error_catch(__func__, err_code_str, err_code); }

//!
//! Parameter checking. 
//! @param assert Condition.
//! @param err_code Unique identifier of assert in module (i.e. err code).
//! @warning This macro should be used inside functions at top level only.
//!
#define lang_param_assert(assert, err_code) \
    lang_param_assert_return_str(assert, #err_code, err_code)

#else
#error Unknown error checking type!
#endif

#define lang_type_to_bits(type) (sizeof(type) * 8)
#define lang_bits_to_words(n) \
    (((n) + lang_type_to_bits(unsigned) - 1)/(lang_type_to_bits(unsigned)))

#define lang_containing_record(address, type, field) \
    ((type*)((char*)(address) - (size_t)(&((type*)0)->field)))

#if __STDC_VERSION__ > 201000L
#define lang_static_assert(cond) _Static_assert(cond, #cond)
#else
#define __lang_static_assert(cond, line) \
    typedef int static_assertion_##line[((!(!(cond))) * 2) - 1]
#define ___lang_static_assert(cond, line) __lang_static_assert(cond, line)
#define lang_static_assert(cond) ___lang_static_assert(cond, __LINE__)
#endif

#define lang_min(a, b) ((a) < (b) ? (a) : (b))
#define lang_max(a, b) ((a) > (b) ? (a) : (b))

FX_METADATA(({ interface: [LANG_TYPES, OS_186] }))

FX_METADATA(({ options: [                                               
    LANG_ASSERT_ERROR_CHECKING_TYPE: {                                            
        type: enum, values: [Off: 0, Classic: 1, Centralized: 2], default: 0,  
        description: "Default error checking policy." }]}))

#endif 
#ifndef _HAL_CPU_INTR_ARMv7M_FPU_HEADER_
#define _HAL_CPU_INTR_ARMv7M_FPU_HEADER_

/** 
  ******************************************************************************
  *  @file   CortexM/intr_v7m_fpu/hal_cpu_intr.h
  *  @brief  HAL interrupt implementation for CortexM with FPU module.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)

//!
//! Returns current interrupt vector.
//! @return Vector of current ISR.
//! @warning This function may be called only in interrupt handler environment.
//!
unsigned int hal_intr_get_current_vect(void);

//!
//! User-supplied function which is called on every OS-managed interrupt.
//!
void fx_intr_handler(void);

FX_METADATA(({ interface: [HAL_CPU_INTR, ARMv7M_FPU] }))

#endif
#ifndef _RTL_LIST_V1_HEADER_
#define _RTL_LIST_V1_HEADER_

/*
  * Copyright (c) 2005-2007, Kohsuke Ohtani
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions
  * are met:
  * 1. Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright
  *    notice, this list of conditions and the following disclaimer in the
  *    documentation and/or other materials provided with the distribution.
  * 3. Neither the name of the author nor the names of any co-contributors
  *    may be used to endorse or promote products derived from this software
  *    without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  * SUCH DAMAGE.
  */

#include <stddef.h>

typedef struct _rtl_list_t 
{
    struct _rtl_list_t  *next;
    struct _rtl_list_t  *prev;
}
rtl_list_t, rtl_list_linkage_t;

#define rtl_list_init(head) ((head)->next = (head)->prev = (head))
#define rtl_list_next(node) ((node)->next)
#define rtl_list_prev(node) ((node)->prev)
#define rtl_list_empty(head) ((head)->next == (head))
#define rtl_list_first(head) ((head)->next)
#define rtl_list_last(head) ((head)->prev)
#define rtl_list_end(head, node) ((node) == (head))
#define rtl_list_is_node_linked(node) (((node)->next) && ((node)->prev))
#define rtl_list_entry(p, type, member) \
    ((type*)((char*)(p) - (size_t)(&((type*)0)->member)))

static inline void
rtl_list_insert(rtl_list_t* prev, rtl_list_t* node)
{
    node->next = prev->next;
    node->prev = prev;
    prev->next->prev = node;
    prev->next = node;
}

static inline void
rtl_list_remove(rtl_list_t* node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = (rtl_list_linkage_t*)0;
    node->prev = (rtl_list_linkage_t*)0;
}

static inline void 
rtl_list_insert_range(rtl_list_t* dst, rtl_list_t* src)
{
    dst->prev->next = src->next;
    src->next->prev = dst->prev;
    dst->prev = src->prev;
    src->prev->next = dst;
}

FX_METADATA(({ interface: [RTL_LIST, V1] }))

#endif
#ifndef _HAL_CLOCK_ARMv7M_V1_HEADER_
#define _HAL_CLOCK_ARMv7M_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   CortexM/clock/hal_clock.h
  *  @brief  Clock ticks interface.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(CFG_OPTIONS)
  
FX_METADATA(({ interface: [HAL_CLOCK, ARMv7M_V1] }))

FX_METADATA(({ options: [                                               
    HAL_CLOCK_TICK_HOOK: {                                                    
        type: int, range: [0, 1], default: 0,                        
        description: "Call application function in HAL tick."}]}))
                
#endif
#ifndef _FX_TIMER_INTERNAL_SIMPLE_HEADER_
#define _FX_TIMER_INTERNAL_SIMPLE_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_timer_internal.h
  *  @brief  Internal timers interface. This implementation may be used ONLY in 
  *  uniprocessor systems with unified interrupt architecture since timer 
  *  callback may be called from interrupt handlers directly.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(RTL_LIST)
#include FX_INTERFACE(HAL_CLOCK)

//
// Error codes.
//
enum
{
    FX_TIMER_OK = FX_STATUS_OK,
    FX_TIMER_ALREADY_CANCELLED,
    FX_TIMER_CONCURRENT_USE,
    FX_TIMER_INTERNAL_ERR_MAX
};

#define FX_TIMER_MAX_RELATIVE_TIMEOUT UINT32_C(0x7FFFFFFF)

//!
//! Timer representation. 
//!
typedef struct
{
    uint32_t timeout;
    uint32_t period;
    int (*callback)(void*);
    void* callback_arg;
    rtl_list_linkage_t link;
} 
fx_timer_internal_t;

void fx_timer_ctor(void);
#define fx_app_timer_ctor()
#define fx_timer_time_after(a, b) (((int32_t)(b) - (int32_t)(a)) < 0)
#define fx_timer_time_after_or_eq(a, b) (((int32_t)(b) - (int32_t)(a)) <= 0)
uint32_t fx_timer_get_tick_count(void);
uint32_t fx_timer_set_tick_count(uint32_t);
int fx_timer_internal_init(fx_timer_internal_t* t, int (*f)(void*), void* arg);
int fx_timer_internal_cancel(fx_timer_internal_t* t);
int fx_timer_internal_set_rel(
    fx_timer_internal_t* t, 
    uint32_t delay, 
    uint32_t period
);
int fx_timer_internal_set_abs(
    fx_timer_internal_t* t, 
    uint32_t delay, 
    uint32_t period
);

FX_METADATA(({ interface: [FX_TIMER_INTERNAL, SIMPLE] }))

#endif
#ifndef _FX_APP_TIMER_PROXY_HEADER_
#define _FX_APP_TIMER_PROXY_HEADER_

/**
  ******************************************************************************
  *  @file   stubs/fx_app_timer.h
  *  @brief  Proxy header for non-threaded timers.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(FX_TIMER_INTERNAL)

FX_METADATA(({ interface: [FX_APP_TIMER, PROXY] }))

#endif
#ifndef _FX_TIMER_DIRECT_HEADER_
#define _FX_TIMER_DIRECT_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_timer.h
  *  @brief  Interface of application timers with disabled error checking.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(FX_APP_TIMER)

//!
//! Without error checking timer is directly mapped into internal timer. 
//!
typedef fx_timer_internal_t fx_timer_t;

#define fx_timer_init(timer, func, arg) fx_timer_internal_init(timer, func, arg)
#define fx_timer_deinit(timer)
#define fx_timer_cancel(timer) fx_timer_internal_cancel(timer)
#define fx_timer_set_rel(t, d, period) fx_timer_internal_set_rel(t, d, period)
#define fx_timer_set_abs(t, d, period) fx_timer_internal_set_abs(t, d, period)

FX_METADATA(({ interface: [FX_TIMER, DIRECT] }))

#endif
#ifndef _HAL_CPU_CONTEXT_KER_FRAME_BASED_HEADER_
#define _HAL_CPU_CONTEXT_KER_FRAME_BASED_HEADER_

/** 
  ******************************************************************************
  *  @file common/context/hal_cpu_context.h
  *  @brief CPU context management functions.
  *  This module may be used for any HAL without usermode threads support.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)

struct _hal_intr_frame_t;

//!
//! Hardware context contains only pointer to the current stack frame containing
//! full register state.
//!
typedef struct
{
    struct _hal_intr_frame_t* frame;  
}
hal_cpu_context_t;

void hal_context_ker_create(
    hal_cpu_context_t* context, 
    uintptr_t stk, 
    uintptr_t entry, 
    uintptr_t arg
);

#define hal_context_ker_delete(context)
void hal_context_switch(hal_cpu_context_t* new_ctx, hal_cpu_context_t* old_ctx);

FX_METADATA(({ interface: [HAL_CPU_CONTEXT, KER_FRAME_BASED] }))

#endif
#ifndef _FX_PROCESS_DISABLED_HEADER_
#define _FX_PROCESS_DISABLED_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_process.h
  *  @brief  Stub for disabling process subsystem. This module is tightly 
  *  coupled with threads impl.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)

#define fx_process_ctor()
#define fx_process_set_exception(id, h, old)
#define fx_process_self() (NULL)
#define fx_process_switch(newp, oldp)
typedef struct { int dummy; } fx_process_t;

//
// When process support is disabled the kernel does not handle hardware 
// exceptions, only TERM exception is supported for internal kernel use.
//
#define FX_EXCEPTION_TERM 0

//
// If only one function may be set as exception handler for single exception id 
// then "set" functions may be safely disabled and "get" function should always 
// return default handler.
//
typedef void (*fx_process_exception_handler_t)(void* t, unsigned id, void* arg);
extern void fx_thread_term_handler(void* target, unsigned id, void* arg);
#define fx_process_get_exception(exc_id) (&fx_thread_term_handler)

FX_METADATA(({ interface: [FX_PROCESS, DISABLED] }))

#endif
#ifndef _RTL_QUEUE_V1_HEADER_
#define _RTL_QUEUE_V1_HEADER_

/*
  * Copyright (c) 2005-2007, Kohsuke Ohtani
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions
  * are met:
  * 1. Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright
  *    notice, this list of conditions and the following disclaimer in the
  *    documentation and/or other materials provided with the distribution.
  * 3. Neither the name of the author nor the names of any co-contributors
  *    may be used to endorse or promote products derived from this software
  *    without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  * SUCH DAMAGE.
  */

#include <stddef.h>

typedef struct _rtl_queue 
{
    struct _rtl_queue* next;
    struct _rtl_queue* prev;
}
rtl_queue_t, rtl_queue_linkage_t;

#define RTL_QUEUE_INITIALIZER {NULL, NULL}

#define rtl_queue_init(head) ((head)->next = (head)->prev = (head))
#define rtl_queue_empty(head) ((head)->next == (head))
#define rtl_queue_next(q) ((q)->next)
#define rtl_queue_prev(q) ((q)->prev)
#define rtl_queue_first(head) ((head)->next)
#define rtl_queue_last(head) ((head)->prev)
#define rtl_queue_end(head,q) ((q) == (head))
#define rtl_queue_is_item_linked(q) (((q)->next) && ((q)->prev))
#define rtl_queue_item_init(item) ((item)->next = (item)->prev = NULL)
#define rtl_queue_entry(q, type, member) \
    ((type*)((char*)(q) - (size_t)(&((type*)0)->member)))

void rtl_enqueue(rtl_queue_t*, rtl_queue_t*);
rtl_queue_t* rtl_dequeue(rtl_queue_t*);
void rtl_queue_insert(rtl_queue_t*, rtl_queue_t*);
void rtl_queue_remove(rtl_queue_t*);
void rtl_queue_copy(rtl_queue_t* dst, rtl_queue_t* src);

FX_METADATA(({ interface: [RTL_QUEUE, V1] }))

#endif
#ifndef _FX_SCHED_ALG_MPQ_FIFO_HEADER_
#define _FX_SCHED_ALG_MPQ_FIFO_HEADER_

/** 
  ******************************************************************************
  *  @file   mpq/fx_sched_alg.h
  *  @brief  Interface of scheduler container.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(CFG_OPTIONS)
#include FX_INTERFACE(RTL_QUEUE)
#include FX_INTERFACE(LANG_TYPES)

#ifndef FX_SCHED_ALG_PRIO_NUM
#define FX_SCHED_ALG_PRIO_NUM 64
#endif

#if FX_SCHED_ALG_PRIO_NUM > 1024
#error Too many priority levels, it must be less than 1024!
#endif

//! 
//! Scheduler container representation. 
//! It contains N queues for each priority and bitmaps for fast search.
//!
typedef struct
{
    rtl_queue_t priority_queues[FX_SCHED_ALG_PRIO_NUM];
    unsigned map1;
    unsigned map2[lang_bits_to_words(FX_SCHED_ALG_PRIO_NUM)];
}
fx_sched_container_t;

//!
//! Scheduler container's item. 
//!
typedef struct
{
    unsigned int prio;
    rtl_queue_linkage_t link;
} 
fx_sched_params_t;

//!
//! Lowest priority has maximum numeric value starting from 0.
//!
#define FX_SCHED_ALG_PRIO_IDLE (FX_SCHED_ALG_PRIO_NUM - 1)

//!
//! Cast scheduling params as integer. 
//! It is also used for "priority visualization" by external tools.
//!
#define fx_sched_params_as_number(s) ((s)->prio)

//!
//! Checks whether sched params A preempts parameters B.
//!
#define fx_sched_params_is_preempt(a, b) ((a)->prio < (b)->prio)

//!
//! Check for params equality.
//!
#define fx_sched_params_is_equal(a, b) ((a)->prio == (b)->prio)

//!
//! Checks whether the item unique at given priority level. 
//! It means that no pending work exist.
//!
#define fx_sched_params_is_unique(a) \
    (rtl_queue_first(&((a)->link)) == rtl_queue_last(&((a)->link)))

//!
//! Copy constructor.
//! @param src Source item from which params will be copied to destination item.
//! @param dst Destination item.
//!
#define fx_sched_params_copy(src, dst) ((dst)->prio = (src)->prio)

//!
//! Schedulable item initializer. (priority-specific method).
//! @param [in] item Schedulable item to be initialized.
//! @param [in] priority Priority which to be set in schedulable item.
//!
#define fx_sched_params_init_prio(item, priority) ((item)->prio = (priority))

//! 
//! Default initializers for scheduler container's items. 
//!
typedef enum
{
    FX_SCHED_PARAMS_INIT_IDLE = 0,  //!< This value is used to init IDLE-entity.
    FX_SCHED_PARAMS_INIT_DEFAULT,   //!< Default priority.
    FX_SCHED_PARAMS_INIT_SPECIFIED  //!< Copy sched params from another item.
}
fx_sched_params_init_t;

void fx_sched_params_init(
    fx_sched_params_t* params, 
    fx_sched_params_init_t type, 
    const fx_sched_params_t* src
);

void fx_sched_container_init(fx_sched_container_t* c);
void fx_sched_container_add(fx_sched_container_t* c, fx_sched_params_t* p);
void fx_sched_container_remove(fx_sched_container_t* c, fx_sched_params_t* p);
fx_sched_params_t* fx_sched_container_get(fx_sched_container_t* c);

FX_METADATA(({ interface: [FX_SCHED_ALG, MPQ_FIFO] }))

FX_METADATA(({ options: [                                               
    FX_SCHED_ALG_PRIO_NUM: {                                                    
        type: int, range: [8, 1024], default: 64,                               
        description: "Number of scheduling priorities."}]}))

#endif
#ifndef _HAL_ASYNC_ARMv7M_UNIFIED_HEADER_
#define _HAL_ASYNC_ARMv7M_UNIFIED_HEADER_

/** 
  ******************************************************************************
  *  @file   CortexM/sync/unified/hal_async.h
  *  @brief  SPL management functions for unified interrupt architecture.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)

FX_METADATA(({ interface: [HAL_ASYNC, ARMv7M_UNIFIED] }))

//!
//! Constants for SPL levels. These constants are tightly coupled with PRIMASK 
//! register format, so, they must not be changed!
//!
typedef enum
{
    SPL_SYNC = 0x01,
    SPL_DISPATCH = SPL_SYNC,
    SPL_LOW = 0x00
}
spl_t;

#define hal_async_ctor()

spl_t hal_async_raise_spl(const spl_t new_spl); 
void hal_async_lower_spl(const spl_t new_spl);
spl_t hal_async_get_current_spl(void);

#define ICSR_ADDR ((volatile unsigned int*) 0xE000ED04)
#define hal_async_request_swi(spl) ((*ICSR_ADDR) = 0x10000000)
                
#endif
#ifndef _FX_DBG_V1_HEADER_
#define _FX_DBG_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_dbg.h
  *  @brief  Debug interface for assertions.
  *
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(CFG_OPTIONS)

//
// This function stops execution and optionally calls user-defined callback, 
// if debug is enabled. If debug is disabled it stops the system silently.
//
void fx_panic_internal(const char* msg, const char* funcname);

//
// Panic macro uses C99 feature __func__, which is defined in each function as 
// const char* __func__ = "<funcion name>";
//
#define fx_panic(msg) fx_panic_internal(msg, __func__)

//!
//! Debug assertion macro. It causes system panic if condition is not satisfied.
//! It should be an expression.
//!
#ifdef FX_DBG_ENABLED
#define fx_dbg_assert(cond) ((!(cond)) ? fx_panic("Debug assertion"), 1 : 0)
#else
#define fx_dbg_assert(cond) ((void)0)
#endif

FX_METADATA(({ interface: [FX_DBG, V1] }))

#endif 
  
#ifndef _TRACE_LOCKS_STUB_HEADER_
#define _TRACE_LOCKS_STUB_HEADER_

/** 
  ******************************************************************************
  *  @file   trace_locks.h
  *  @brief  Stub for trace subsystem. Disables spl tracing.
  *
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#define trace_intr_lock()         ((void)0)
#define trace_intr_unlock()       ((void)0)
#define trace_dispatch_lock()     ((void)0)
#define trace_dispatch_unlock()   ((void)0)
#define trace_lock_enter(lock)    ((void)0)
#define trace_lock_leave(lock)    ((void)0)

FX_METADATA(({ interface: [TRACE_LOCKS, STUB] }))

#endif 
#ifndef _HAL_MP_STUB_V1_HEADER_
#define _HAL_MP_STUB_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   common/mp/hal_mp.h
  *  @brief  Multiprocessor interface stub for uniprocessor systems.
  *  On uniprocessor systems IPIs may be sent only to itself.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(HAL_ASYNC)

#define HAL_MP_CPU_MAX 1
#define hal_mp_get_current_cpu() 0
#define hal_mp_get_cpu_count() 1
#define hal_mp_request_ipi(cpu, spl) ((void) (cpu), hal_async_request_swi(spl))

FX_METADATA(({ interface: [HAL_MP, STUB_V1] }))

#endif
#ifndef _FX_SPL_UNIFIED_UP_HEADER_
#define _FX_SPL_UNIFIED_UP_HEADER_

/** 
  ******************************************************************************
  *  @file   unified/fx_spl.h
  *  @brief  Definitions for unified synchronization model for uniprocessors.
  *  In this sync scheme OS kernel operates at sync level with intrs disabled.
  *  Spinlocks raise level to sync (disabling interrupts) from any level.
  *  Interrupts handlers run at level higher than DISPATCH and lower than SYNC, 
  *  therefore, OS services may be used from interrupts directly.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(HAL_ASYNC)
#include FX_INTERFACE(FX_DBG)
#include FX_INTERFACE(TRACE_LOCKS)
#include FX_INTERFACE(HAL_MP)

#define FX_SPL_SCHED_LEVEL SPL_SYNC

typedef struct { spl_t old_spl; } lock_t;
typedef spl_t fx_lock_intr_state_t;

static inline void
fx_spl_raise_to_sync_from_any(spl_t* old_state) 
{
    *old_state = hal_async_raise_spl(SPL_SYNC); 
    trace_intr_lock();
}

static inline void
fx_spl_lower_to_any_from_sync(spl_t old_state) 
{
    trace_intr_unlock();
    hal_async_lower_spl(old_state);
}

//
// In unified architecture scheduler works at level above all software and 
// hardware interrupts, so, "higher" level is not guarantee SYNC. Locks from 
// higher level on uniprocessor system should raise SPL.
//

#define fx_spl_spinlock_init(lock) (lock)->old_spl = SPL_LOW
#define fx_spl_spinlock_get_from_sched(lock) ((void) (lock))
#define fx_spl_spinlock_put_from_sched(lock) ((void) (lock))

static inline void 
fx_spl_spinlock_get_from_any(lock_t* lock)
{
    fx_dbg_assert(hal_async_get_current_spl() != SPL_LOW);
    fx_spl_raise_to_sync_from_any(&lock->old_spl);
}

static inline void 
fx_spl_spinlock_put_from_any(lock_t* lock) 
{
    fx_spl_lower_to_any_from_sync(lock->old_spl);
}

static inline void 
fx_spl_raise_to_sched_from_low(spl_t* prev_state)
{
    fx_spl_raise_to_sync_from_any(prev_state);
}

static inline void 
fx_spl_lower_to_low_from_sched(spl_t prev_state) 
{
    fx_spl_lower_to_any_from_sync(prev_state);
}

static inline void 
fx_spl_raise_to_sched_from_disp(spl_t* prev_state)
{
    fx_spl_raise_to_sync_from_any(prev_state);
}

static inline void
fx_spl_lower_to_disp_from_sched(spl_t prev_state)
{
    fx_spl_lower_to_any_from_sync(prev_state);
}

lang_static_assert(SPL_DISPATCH == SPL_SYNC);
lang_static_assert(HAL_MP_CPU_MAX == 1);

FX_METADATA(({ interface: [FX_SPL, UNIFIED_UP] }))

#endif

#ifndef _FX_SCHED_UP_FIFO_HEADER_
#define _FX_SCHED_UP_FIFO_HEADER_

/**
  ******************************************************************************
  *  @file   fx_sched.h
  *  @brief  Interface header for global uniprocessor scheduler.
  *
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(FX_SCHED_ALG)
#include FX_INTERFACE(HAL_ASYNC)
#include FX_INTERFACE(FX_SPL)

//!
//! Schedulable entity.
//!
typedef struct
{
    unsigned int suspend_count;     //!< Item suspended if this value is > 0.
    fx_sched_params_t sched_params; //!< Associated parameters, priority, etc.
}
fx_sched_item_t;

#define fx_sched_item_as_sched_params(item) (&((item)->sched_params))

void fx_sched_ctor(void);

void fx_sched_item_init(
    fx_sched_item_t* item, 
    fx_sched_params_init_t t, 
    const fx_sched_params_t* arg
);

void fx_sched_item_remove(fx_sched_item_t* item);
void fx_sched_item_get_params(fx_sched_item_t* src, fx_sched_params_t* dst);
void fx_sched_item_set_params(fx_sched_item_t* dst, const fx_sched_params_t* s);
unsigned int fx_sched_item_suspend(fx_sched_item_t* item);
unsigned int fx_sched_item_resume(fx_sched_item_t* item);
bool fx_sched_yield(fx_sched_item_t* item);
fx_sched_item_t* fx_sched_get_next(void);
void fx_sched_mark_resched_needed(void);

//
// Add schedulable item to the scheduler. 
// In current implementation actual work is done in @ref fx_sched_item_resume.
//
#define fx_sched_item_add(item)

//
// SMP API. On uniprocessor systems it is implemented as stubs.
//
typedef int fx_sched_affinity_t;
#define fx_sched_set_affinity(item, affinity, self) ((void)(*(affinity)))
#define fx_sched_get_affinity(item, affinity) ((void)(*(affinity)))
#define fx_sched_get_cpu(item) 0

typedef spl_t fx_sched_state_t;
#define fx_sched_lock(prev_ptr) fx_spl_raise_to_sched_from_low(prev_ptr)
#define fx_sched_unlock(prev) fx_spl_lower_to_low_from_sched(prev)
#define fx_sched_lock_from_disp_spl(prev) fx_spl_raise_to_sched_from_disp(prev)
#define fx_sched_unlock_from_disp_spl(prv) fx_spl_lower_to_disp_from_sched(prv)

FX_METADATA(({ interface: [FX_SCHED, UP_FIFO] }))

#endif
#ifndef _FX_SYNC_UP_QUEUE_HEADER_
#define _FX_SYNC_UP_QUEUE_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_sync.h
  *  @brief  Basic synchronization layer. 
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(RTL_QUEUE)
#include FX_INTERFACE(FX_SCHED_ALG)

struct _fx_sync_waiter_t;
struct _fx_sync_waitable_t;
struct _fx_sync_wait_block_t;
typedef struct _fx_sync_waiter_t fx_sync_waiter_t;
typedef struct _fx_sync_waitable_t fx_sync_waitable_t;
typedef struct _fx_sync_wait_block_t fx_sync_wait_block_t;

//!
//! Scheduling policy for waitable object queue.
//! FIFO means that waiters will be released in FIFO order, PRIO: in priority
//! order.
//!
typedef enum
{
    FX_SYNC_POLICY_FIFO = 0,
    FX_SYNC_POLICY_PRIO = 1,
    FX_SYNC_POLICY_MAX,
    FX_SYNC_POLICY_DEFAULT = FX_SYNC_POLICY_FIFO
}
fx_sync_policy_t;

//!
//! Status of wait operation.
//!
typedef enum
{
    FX_WAIT_IN_PROGRESS = 0,
    FX_WAIT_SATISFIED   = 1,
    FX_WAIT_CANCELLED   = 2,
    FX_WAIT_DELETED     = 3,
    FX_WAIT_STATUS_MAX
}
fx_wait_status_t;

//!
//! Waitable object representation. 
//! It is base class for all synchronization primitives and contains queue of
//! wait objects. The test function atomically tests the object and inserts 
//! block to the queue if the object is in nonsignaled state.
//!
struct _fx_sync_waitable_t
{
    rtl_queue_t wq;          
    bool (*test_wait)(fx_sync_waitable_t*, fx_sync_wait_block_t*, const bool);
};

//!
//! Base class of waiter.
//! N.B. Waiter methods is NOT thread safe. It is expected that waiter is a
//! thread and therefore all waiter methods are called in context of one 
//! thread (sequentially).
//!
struct _fx_sync_waiter_t
{
    fx_sched_params_t* sched_params;
    fx_sync_wait_block_t* wb;
    unsigned int wb_num;
};

//!
//! Representation of wait block. 
//! Wait block is a link between waiter and waitable.
//! Attribute is used to pass some info from waiter to primitive's logic. 
//! It is also used in order to return object-specific info from wait functions.
//! N.B. Attribute is used before wait starts, so, it may never be used 
//! simultaneously with status, which indicates status of notification, 
//! therefore they may reside in same memory location. 
//! Warning! Do not use values reserved for status as attributes! In case when 
//! it is needed to implement primitive's logic use indirect parameter passing 
//! (when attribute is a pointer to memory location holding actual value).
//!
struct _fx_sync_wait_block_t
{
    fx_sync_waiter_t* waiter;
    fx_sync_waitable_t* waitable;
    union
    {
        void* attribute;
        fx_wait_status_t status;
    } u;                                        
    rtl_queue_linkage_t link;
};

#define fx_sync_waitable_lock(w)
#define fx_sync_waitable_unlock(w)
#define _fx_sync_waitable_nonempty(w) (!rtl_queue_empty(&((w)->wq)))
#define fx_sync_waitable_as_queue(w) (&((w)->wq))
#define fx_sync_waiter_init(w, params) (w)->sched_params = params
#define fx_sync_is_waiter_satisfied(w) false
#define fx_sync_wb_as_queue_item(wb) (&((wb)->link))
#define fx_sync_queue_item_as_wb(item) \
    (rtl_queue_entry(item, fx_sync_wait_block_t, link))
#define fx_sync_wait_block_get_status(wb) ((wb)->u.status)
#define fx_sync_wait_block_get_attr(wb) ((wb)->u.attribute)
#define FX_SYNC_WAIT_BLOCK_INITIALIZER(wtr, wtbl, attr) \
    {(wtr), NULL, {attr}, RTL_QUEUE_INITIALIZER}

//!
//! Preparing waiter for new wait operation. Should be perfermed before every 
//! wait operation. This implementation supports only waits by OR, so, expected 
//! notification number is always 1 and therefore unused.
//!
static inline void 
fx_sync_waiter_prepare(
    fx_sync_waiter_t* waiter, 
    fx_sync_wait_block_t* wb_array, 
    unsigned int wb_n, 
    unsigned int expected) 
{
    waiter->wb = wb_array;
    waiter->wb_num = wb_n;
}

void fx_sync_waitable_init(
    fx_sync_waitable_t* waitable, 
    void* ignored,
    bool (*fn)(fx_sync_waitable_t*, fx_sync_wait_block_t*, const bool)
);

fx_sync_wait_block_t* _fx_sync_wait_block_get(
    fx_sync_waitable_t* waitable, 
    fx_sync_policy_t p
);

void _fx_sync_wait_notify(
    fx_sync_waitable_t* waitable, 
    fx_wait_status_t s, 
    fx_sync_wait_block_t* wb
);

void _fx_sync_wait_start(fx_sync_waitable_t* w, fx_sync_wait_block_t* wb);
unsigned int fx_sync_wait_rollback(fx_sync_waiter_t* waiter);
extern void fx_sync_waiter_notify(fx_sync_waiter_t* waiter);

FX_METADATA(({ interface: [FX_SYNC, UP_QUEUE] }))

#endif
#ifndef _FX_RTP_DISABLED_HEADER_
#define _FX_RTP_DISABLED_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_rtp_disabled.h
  *  @brief  Stub of run-time protection module. 
  *  Disables run-time checks of object consistency.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

typedef struct { int dummy; } fx_rtp_part_t, fx_rtp_key_t, fx_rtp_t;

#define fx_rtp_init(target, key)
#define fx_rtp_deinit(target)
#define fx_rtp_check(target, key) (true)
#define fx_rtp_part_init(target, key) ((void)0)
#define fx_rtp_part_check(target, key) (true)

FX_METADATA(({ interface: [FX_RTP, DISABLED] }))

#endif
#ifndef _FX_EVENT_V1_HEADER_
#define _FX_EVENT_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_event.h
  *  @brief  Interface of simple events.
  *  ***************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(FX_SYNC)
#include FX_INTERFACE(FX_RTP)
#include FX_INTERFACE(FX_SCHED)

enum
{
    FX_EVENT_MAGIC = 0x45564E54, // EVNT
    FX_EVENT_OK = FX_STATUS_OK,
    FX_EVENT_INVALID_PTR = 1,
    FX_EVENT_INVALID_OBJ = 2,
    FX_EVENT_ERR_MAX
};

//!
//! Internal event is embeddable object which does not perform any validation.
//!
typedef struct
{
    fx_sync_waitable_t waitable;
    bool state;
    lock_t lock;
} 
fx_event_internal_t;

//!
//! Event representation. 
//!
typedef struct
{
    fx_event_internal_t object;
    fx_rtp_t rtp;
} 
fx_event_t;

#define fx_internal_event_as_waitable(e) (&((e)->waitable))
#define fx_event_as_waitable(e) fx_internal_event_as_waitable((&((e)->object)))
#define fx_event_is_valid(e) (fx_rtp_check((&((e)->rtp)), FX_EVENT_MAGIC))

bool fx_event_test_and_wait(
    fx_sync_waitable_t* object, 
    fx_sync_wait_block_t* wb, 
    const bool wait
);
void fx_event_internal_init(fx_event_internal_t* event, const bool state);
void fx_event_internal_set(fx_event_internal_t* event);
void fx_event_internal_reset(fx_event_internal_t* event);
int fx_event_init(fx_event_t* event, const bool state);
int fx_event_deinit(fx_event_t* event);
int fx_event_set(fx_event_t* event);
int fx_event_reset(fx_event_t* event);
int fx_event_pulse(fx_event_t* event); 
int fx_event_get_state(fx_event_t* event, bool* state);

FX_METADATA(({ interface: [FX_EVENT, V1] }))

#endif
#ifndef _FX_THREAD_APC_LIMITED_HEADER_
#define _FX_THREAD_APC_LIMITED_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_thread_apc.h
  *  @brief  Interface of limited version of thread APC subsystem. 
  *          Only uniprocessor systems are supported (internal APC only).
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(FX_DBG)

typedef struct { int dummy; } fx_thread_apc_msg_t;
typedef struct { int dummy; } fx_thread_apc_target_t;

extern void (*fx_thread_apc_on_receive)(fx_thread_apc_target_t*);
#define fx_thread_apc_ctor(idle, cb) fx_thread_apc_on_receive = (cb)
#define fx_thread_apc_target_init(target)
#define fx_thread_apc_msg_init(msg, func, arg)
#define fx_thread_apc_insert(target, msg, accept) (fx_dbg_assert(false), false)
#define fx_thread_apc_cancel(target, msg) false
#define fx_thread_apc_set_mask(new_mask) false
#define fx_thread_apc_pending(target) false
#define fx_thread_apc_deliver(target)

bool fx_thread_apc_insert_internal(
    fx_thread_apc_target_t* target, 
    unsigned int reason, 
    void* arg
);

FX_METADATA(({ interface: [FX_THREAD_APC, LIMITED] }))

#endif
#ifndef _FX_THREAD_CLEANUP_DISABLED_HEADER_
#define _FX_THREAD_CLEANUP_DISABLED_HEADER_

/** 
  ******************************************************************************
  *  @file   disabled/fx_thread_cleanup.h
  *  @brief  Disables functionality of in-thread cleanup handlers.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

typedef struct { int dummy; } fx_thread_cleanup_context_t;
typedef struct { int dummy; } fx_thread_cleanup_handler_t;

#define fx_thread_cleanup_switch_hook(old_target)
#define fx_thread_cleanup_init_target(target)
#define fx_thread_cleanup_handle(target)
#define fx_thread_cleanup_init(handler, f, a) ((void) (handler))
#define fx_thread_cleanup_add(target, k, handler)
#define fx_thread_cleanup_cancel(handler)
#define fx_thread_cleanup_set_hook(target, func)

FX_METADATA(({ interface: [FX_THREAD_CLEANUP, DISABLED] }))

#endif 
#ifndef _FX_STACKOVF_DISABLED_HEADER_
#define _FX_STACKOVF_DISABLED_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_stackovf.h
  *  @brief  Disabled interface of the thread stack overflow protection.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

typedef struct { int dummy; } fx_stackovf_info_t;
#define fx_stackovf_init(object, stack, size)
#define fx_stackovf_check(object, current_stack)

FX_METADATA(({ interface: [FX_STACKOVF, DISABLED] }))

#endif
#ifndef _TRACE_THREAD_STUB_HEADER_
#define _TRACE_THREAD_STUB_HEADER_

/** 
  ******************************************************************************
  *  @file   trace_core.h
  *  @brief  Stub for trace subsystem. Disables thread tracing.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

typedef struct {int dummy;} trace_thread_handle_t;
typedef struct {int dummy;} trace_queue_handle_t;
typedef struct {int dummy;} trace_sem_handle_t;
typedef struct {int dummy;} trace_mutex_handle_t;

#define trace_increment_tick( incremented_counter )

#define trace_mutex_init( mutex_handle )  
#define trace_mutex_init_failed() 
#define trace_mutex_deinit( mutex_handle )  
#define trace_mutex_acquired( mutex_handle , owner_thread_handle)
#define trace_mutex_acquire_block( mutex_handle )
#define trace_mutex_released( mutex_handle, new_owner_handle )

#define trace_sem_init( sem_handle, sem_val, sem_max )
#define trace_sem_init_failed()
#define trace_sem_deinit( sem_handle, sem_val, sem_max )
#define trace_sem_wait_ok( sem_handle, sem_val )
#define trace_sem_wait_block( sem_handle, blocked_thread_handle )
#define trace_sem_post( sem_handle, sem_val )

#define trace_queue_init( queue_handle, items_max )
#define trace_queue_init_failed( queue_handle )
#define trace_queue_deinit( queue_handle, msg_count )
#define trace_queue_send( queue_handle, msg_count )
#define trace_queue_send_failed( queue_handle )
#define trace_queue_send_block( queue_handle )
#define trace_queue_send_forward( queue_handle)
#define trace_queue_receive( queue_handle, msg_count )
#define trace_queue_receive_failed( queue_handle )
#define trace_queue_receive_block( queue_handle )
#define trace_queue_receive_forward( queue_handle )

#define trace_thread_init_idle(thread_handle, prio)
#define trace_thread_init( thread_handle, prio )
#define trace_thread_init_failed()
#define trace_thread_deinit( thread_handle, prio )
#define trace_thread_suspend( thread_handle )
#define trace_thread_resume( thread_handle )
#define trace_thread_wakeup( thread_handle )
#define trace_thread_context_switch(from, to)
#define trace_thread_sleep(thread_handle, ticks)
#define trace_thread_delay_until(thread_handle, ticks) 
#define trace_thread_sched_param_set( thread_handle, param )
#define trace_thread_ceiling( thread_handle, old_prio, new_prio )
#define trace_thread_deceiling( thread_handle, old_prio, new_prio )
#define trace_thread_timeout( thread_handle, timeout )

FX_METADATA(({ interface: [TRACE_CORE, STUB] }))

#endif
#ifndef _FX_THREAD_V1_HEADER_
#define _FX_THREAD_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_thread.h
  *  @brief  Threads implementation.
  *  The module implements basic threads on top of scheduler's container.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(HAL_CPU_CONTEXT)
#include FX_INTERFACE(FX_PROCESS)
#include FX_INTERFACE(FX_SCHED)
#include FX_INTERFACE(FX_EVENT)
#include FX_INTERFACE(FX_TIMER_INTERNAL)
#include FX_INTERFACE(FX_THREAD_APC)
#include FX_INTERFACE(FX_THREAD_CLEANUP)
#include FX_INTERFACE(FX_RTP)
#include FX_INTERFACE(FX_STACKOVF)  
#include FX_INTERFACE(TRACE_CORE) 
  
//!
//! Thread states.
//!
typedef enum
{
    FX_THREAD_STATE_READY = 0,
    FX_THREAD_STATE_SUSPENDED = 1,
    FX_THREAD_STATE_WAITING = 2,
    FX_THREAD_STATE_COMPLETED = 3,
}
fx_thread_state_t;

enum
{
    FX_THREAD_MAGIC = 0x54485244,   // 'THRD'

    //
    // Parameter select for fx_thread_set/get_params
    //
    FX_THREAD_PARAM_PRIO = 0,
    FX_THREAD_PARAM_TIMESLICE = 1,
    FX_THREAD_PARAM_CPU = 2,
    FX_THREAD_PARAM_MAX,

    //
    // API functions status codes.
    //
    FX_THREAD_OK = FX_STATUS_OK,    // Predefined success status.
    FX_THREAD_WAIT_CANCELLED,       // Wait cancelled (due to cancel condition).
    FX_THREAD_WAIT_DELETED,         // Wait aborted due to waitable destruction.
    FX_THREAD_WAIT_INTERRUPTED,     // Wait cancelled due to incoming APC.  
    FX_THREAD_WAIT_TIMEOUT,         // Wait is timed out. 
    FX_THREAD_WAIT_IN_PROGRESS,     // Reserved for internal use.
    FX_THREAD_INVALID_PTR,          // Invalid object pointer.
    FX_THREAD_INVALID_ENTRY,        // Invalid entry function.
    FX_THREAD_INVALID_PRIO,         // Invalid priority value.
    FX_THREAD_INVALID_CPU,          // Invalid CPU selection.
    FX_THREAD_NO_STACK,             // Incorrect stack parameters.
    FX_THREAD_INVALID_OBJ,          // Incorrect stack parameters.
    FX_THREAD_INVALID_TIMEOUT,      // Incorrect timeout value.
    FX_THREAD_INVALID_PARAM,        // Incorrect parameter.
    FX_THREAD_JOIN_SELF,            // Incorrect parameter.
    FX_THREAD_INVALID_TIMESLICE,    // Incorrect parameter.
    FX_THREAD_ERR_MAX,      
};

//!
//! Thread representation.
//!
typedef struct
{
    fx_rtp_t rtp;
    fx_process_t* parent;
    fx_sync_waiter_t waiter;
    fx_sched_item_t sched_item;
    uint32_t timeslice;
    fx_thread_apc_target_t apcs;
    fx_thread_cleanup_context_t cleanup;
    fx_timer_internal_t timer;
    fx_event_internal_t timer_event;
    fx_event_internal_t completion;
    hal_cpu_context_t hw_context;
    fx_stackovf_info_t stk_info;
    lock_t state_lock;
    fx_thread_state_t state;
    bool is_terminating;
    trace_thread_handle_t trace_handle;
}
fx_thread_t;

//
// Internal API functions (not intended to be used by applications).
//
#define fx_thread_as_cleanup_context(t) (&((t)->cleanup))
#define fx_thread_lock(t) fx_spl_spinlock_get_from_sched(&((t)->state_lock))
#define fx_thread_unlock(t) fx_spl_spinlock_put_from_sched(&((t)->state_lock))
#define fx_thread_as_sched_item(thread) (&((thread)->sched_item))
#define fx_thread_as_sched_params(thread) \
    (fx_sched_item_as_sched_params(fx_thread_as_sched_item(thread)))
void fx_thread_ctor(void);
int fx_thread_wait_object(fx_sync_waitable_t* w, void* attr, fx_event_t* ev);
int fx_thread_timedwait_object(fx_sync_waitable_t* w, void* attr, uint32_t tm);
bool fx_thread_send_apc(fx_thread_t* thread, fx_thread_apc_msg_t* msg);
#define fx_thread_cancel_apc(t, a) fx_thread_apc_cancel(&((t)->apcs), a)
#define fx_thread_enter_critical_region() ((void) fx_thread_apc_set_mask(true))
#define fx_thread_leave_critical_region() ((void) fx_thread_apc_set_mask(false))

//
// Public API.
//
#define FX_THREAD_INFINITE_TIMEOUT UINT32_C(0xFFFFFFFF)
#define fx_thread_init(a, b, c, d, e, f, g) \
    fx_thread_init_ex(fx_process_self(), a, b, c, d, e, f, g)

int fx_thread_init_ex(
    fx_process_t* parent,
    fx_thread_t* thread,
    void (*func)(void*), 
    void* arg, 
    unsigned int priority, 
    void* stack,
    size_t stack_sz, 
    bool create_suspended
);
int fx_thread_deinit(fx_thread_t* thread);
int fx_thread_terminate(fx_thread_t* thread);
void fx_thread_exit(void);
int fx_thread_join(fx_thread_t* thread);
int fx_thread_suspend(void);
int fx_thread_resume(fx_thread_t* thread);
int fx_thread_sleep(uint32_t ticks);
int fx_thread_delay_until(uint32_t* prev_wake, uint32_t increment);
fx_thread_t* fx_thread_self(void);
void fx_thread_yield(void);
int fx_thread_get_params(fx_thread_t* thread, unsigned int t, unsigned int* v);
int fx_thread_set_params(fx_thread_t* thread, unsigned int t, unsigned int v);
int fx_thread_wait_event(fx_event_t* event, fx_event_t* cancel_event);
int fx_thread_timedwait_event(fx_event_t* event, uint32_t timeout);

FX_METADATA(({ interface: [FX_THREAD, V1] }))

#endif
#ifndef _FX_DPC_STUB_HEADER_
#define _FX_DPC_STUB_HEADER_

/** 
  ******************************************************************************
  *  @file   unified/fx_dpc.h
  *  @brief  DPC stub.
  *  Deferred procedures work in context of ISR.
  *  This DPC implementation is intended to be used with unified interrupt 
  *  architecture.
  *  DPC request results in direct call of deferred function instead of putting 
  *  it into queue. Because inserting of DPC is usually performed in ISR,
  *  deferred function may be called at SPL greater than DISPATCH.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(FX_DBG)
  
typedef struct { int dummy; } fx_dpc_t;

#define fx_dpc_ctor()
#define fx_dpc_init(dpc)
#define fx_dpc_request(dpc, func, arg) (func(dpc, arg), true)
#define fx_dpc_cancel(dpc) (false)
#define fx_dpc_set_target_cpu(dpc, cpu) fx_dbg_assert(cpu == 0)
#define fx_dpc_environment() (false)
#define fx_dpc_handle_queue()

FX_METADATA(({ interface: [FX_DPC, STUB] }))

#endif
#ifndef _FX_SEM_V1_HEADER_
#define _FX_SEM_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_sem.h
  *  @brief  Interface of semaphores.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(FX_SYNC)
#include FX_INTERFACE(FX_THREAD)
#include FX_INTERFACE(FX_RTP)
#include FX_INTERFACE(TRACE_CORE)

enum
{
    FX_SEM_MAGIC = 0x53454D41, // SEMA
    FX_SEM_OK = FX_STATUS_OK,
    FX_SEM_INVALID_PTR = FX_THREAD_ERR_MAX,
    FX_SEM_INVALID_OBJ,
    FX_SEM_UNSUPPORTED_POLICY,
    FX_SEM_INVALID_VALUE,
    FX_SEM_INVALID_TIMEOUT,
    FX_SEM_ERR_MAX
};

//!
//! Semaphore representation.
//!
typedef struct
{
    fx_sync_waitable_t waitable;
    lock_t lock;
    unsigned int semaphore;
    unsigned int max_count;
    fx_sync_policy_t policy;
    fx_rtp_t rtp;
    trace_sem_handle_t trace_handle;
} 
fx_sem_t;

int fx_sem_init(
    fx_sem_t* sem, 
    unsigned int init, 
    unsigned int max_val, 
    fx_sync_policy_t p
);
int fx_sem_deinit(fx_sem_t* sem);
int fx_sem_reset(fx_sem_t* sem);
int fx_sem_get_value(fx_sem_t* sem, unsigned int* value);
int fx_sem_post(fx_sem_t* sem);
int fx_sem_post_with_policy(fx_sem_t* sem, fx_sync_policy_t policy);
int fx_sem_wait(fx_sem_t* sem, fx_event_t* event);
int fx_sem_timedwait(fx_sem_t* sem, uint32_t timeout);

FX_METADATA(({ interface: [FX_SEM, V1] }))

#endif
#ifndef _FX_MUTEX_V1_HEADER_
#define _FX_MUTEX_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_mutex.h
  *  @brief  Interface of mutexes.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(FX_THREAD)
#include FX_INTERFACE(FX_RTP)
#include FX_INTERFACE(TRACE_CORE) 

enum
{
    FX_MUTEX_MAGIC = 0x4D555458, // MUTX
    FX_MUTEX_OK = FX_STATUS_OK,
    FX_MUTEX_INVALID_PTR = FX_THREAD_ERR_MAX,
    FX_MUTEX_INVALID_OBJ,
    FX_MUTEX_UNSUPPORTED_POLICY,
    FX_MUTEX_INVALID_PRIORITY,
    FX_MUTEX_INVALID_TIMEOUT,
    FX_MUTEX_WRONG_OWNER,
    FX_MUTEX_RECURSIVE_LIMIT,
    FX_MUTEX_ABANDONED,
    FX_MUTEX_ERR_MAX
};

//!
//! Mutex object.
//!
typedef struct
{
    fx_sync_waitable_t waitable;
    lock_t lock;
    fx_thread_t* volatile owner;
    volatile uint_fast16_t recursive_locks;
    bool ceiling_enabled;
    fx_sched_params_t ceiling_orig;
    fx_sched_params_t owner_params;
    fx_sync_policy_t policy;
    fx_rtp_t rtp;
    trace_mutex_handle_t trace_handle;
} 
fx_mutex_t;

#define fx_mutex_limit_exceeded(m) ((m)->recursive_locks == UINT_FAST16_MAX)
#define fx_mutex_lock_counter_get(m) ((m)->recursive_locks)
#define fx_mutex_lock_counter_set(m, c) ((m)->recursive_locks = (c))
#define FX_MUTEX_CEILING_DISABLED (~0U)

int fx_mutex_init(fx_mutex_t* mutex, unsigned int prio, fx_sync_policy_t p);
int fx_mutex_deinit(fx_mutex_t* mutex);
int fx_mutex_acquire(fx_mutex_t* mutex, fx_event_t* event);
int fx_mutex_timedacquire(fx_mutex_t* mutex, uint32_t tout);
int fx_mutex_release(fx_mutex_t* mutex);
int fx_mutex_release_with_policy(fx_mutex_t* mutex, fx_sync_policy_t policy);
fx_thread_t* fx_mutex_get_owner(fx_mutex_t* mutex);

FX_METADATA(({ interface: [FX_MUTEX, V1] }))

#endif
#ifndef _FX_MSGQ_CORE_V1_HEADER_
#define _FX_MSGQ_CORE_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_msgq_core.h
  *  @brief  Interface of core message queue services.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(FX_RTP)
#include FX_INTERFACE(TRACE_CORE)
#include FX_INTERFACE(FX_SYNC)
#include FX_INTERFACE(FX_SCHED)

//
// Attributes object used to pass additional parameters to test&wait function.
//
typedef struct
{
    bool to_back;
    uintptr_t* buf;
}
fx_msgq_wait_attr_t;

//!
//! Message queue object. 
//! Queue contains two waitable object for senders and receivers.
//!
typedef struct
{
    fx_sync_waitable_t send_wtbl;
    fx_sync_waitable_t recv_wtbl;
    lock_t lock;
    uintptr_t* buf;
    unsigned int items_max;
    unsigned int items;
    unsigned int head;
    unsigned int tail;
    fx_rtp_t rtp;
    fx_sync_policy_t policy;
    trace_queue_handle_t trace_handle;
} 
fx_msgq_t;

#define FX_MSGQ_MAGIC 0x4D534751 // 'MSGQ'
#define fx_msgq_is_valid(msgq) (fx_rtp_check((&((msgq)->rtp)), FX_MSGQ_MAGIC))

int fx_msgq_core_init(
    fx_msgq_t* msgq, 
    uintptr_t* buf, 
    unsigned int sz, 
    fx_sync_policy_t p
);
int fx_msgq_core_deinit(fx_msgq_t* msgq);
int fx_msgq_core_flush(fx_msgq_t* msgq);
bool fx_msgq_test_and_wait_send(
    fx_sync_waitable_t* object, 
    fx_sync_wait_block_t* wb, 
    const bool wait
);

FX_METADATA(({ interface: [FX_MSGQ_CORE, V1] }))

#endif
#ifndef _FX_MSGQ_V1_HEADER_
#define _FX_MSGQ_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_msgq.h
  *  @brief  Interface of message queue services for threads.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(FX_MSGQ_CORE)
#include FX_INTERFACE(FX_THREAD)

enum
{
    FX_MSGQ_OK = FX_STATUS_OK,
    FX_MSGQ_INVALID_PTR = FX_THREAD_ERR_MAX,
    FX_MSGQ_INVALID_OBJ,
    FX_MSGQ_INVALID_BUF,
    FX_MSGQ_UNSUPPORTED_POLICY,
    FX_MSGQ_ERR_MAX
};

int fx_msgq_init(
  fx_msgq_t* msgq, 
  uintptr_t* buf, 
  unsigned int sz, 
  fx_sync_policy_t p
);
int fx_msgq_deinit(fx_msgq_t* msgq);
int fx_msgq_flush(fx_msgq_t* msgq);
int fx_msgq_front_send(fx_msgq_t* msgq, uintptr_t msg, fx_event_t* cancel_ev);
int fx_msgq_back_send(fx_msgq_t* msgq, uintptr_t msg, fx_event_t* cancel_ev);
int fx_msgq_front_timedsend(fx_msgq_t* msgq, uintptr_t msg, uint32_t tout);
int fx_msgq_back_timedsend(fx_msgq_t* msgq, uintptr_t msg, uint32_t tout);
int fx_msgq_timedreceive(fx_msgq_t* msgq, uintptr_t* msg, uint32_t tout);
int fx_msgq_receive(fx_msgq_t* msgq, uintptr_t* msg, fx_event_t* cancel_ev);

FX_METADATA(({ interface: [FX_MSGQ, V1] }))

#endif
#ifndef _FX_BLOCK_POOL_V1_HEADER_
#define _FX_BLOCK_POOL_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_block_pool.h
  *  @brief  Interface of memory block pool primitive.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(FX_THREAD)
#include FX_INTERFACE(FX_RTP)

enum
{
    FX_BLOCK_POOL_MAGIC = 0x424C4B50, // 'BLKP'
    FX_BLOCK_POOL_OK = FX_STATUS_OK,
    FX_BLOCK_POOL_INVALID_PTR = FX_THREAD_ERR_MAX,
    FX_BLOCK_POOL_INVALID_OBJ,
    FX_BLOCK_POOL_NO_MEM,
    FX_BLOCK_POOL_IMPROPER_ALIGN,
    FX_BLOCK_POOL_UNSUPPORTED_POLICY,
    FX_BLOCK_POOL_ERR_MAX
};

//!
//! Block pool representation. 
//!
typedef struct
{
    fx_sync_waitable_t waitable;  //!< Internal waitable object.
    fx_rtp_t rtp;                 //!< Runtime protection member (canary).
    lock_t  lock;                 //!< Lock associated with the primitive.
    uintptr_t base;               //!< Address of available memory blocks pool.
    size_t sz;                    //!< Size of memory block.
    size_t remaining_sz;          //!< Remaining memory size in pool.
    rtl_list_t free_blocks;       //!< List of bree blocks.
    unsigned int free_blocks_num; //!< Available blocks count.
    fx_sync_policy_t policy;      //!< Default releasing policy.
} 
fx_block_pool_t;

//!
//! Block header. 
//!
typedef struct
{
    union
    {
        fx_block_pool_t* parent_pool;
        rtl_list_linkage_t link;
    }
    hdr;
} 
fx_mem_block_t;

int fx_block_pool_init(
    fx_block_pool_t* bp, 
    void* base, 
    size_t sz, 
    size_t blk_sz, 
    fx_sync_policy_t p
);
int fx_block_pool_deinit(fx_block_pool_t* bp);
int fx_block_pool_alloc(fx_block_pool_t* bp, void** blk, fx_event_t* cancel);
int fx_block_pool_timedalloc(fx_block_pool_t* bp, void** blk, uint32_t tout);
int fx_block_pool_release(void* blk_ptr);
int fx_block_pool_release_internal(void* blk_ptr, fx_sync_policy_t p);
int fx_block_pool_avail_blocks(fx_block_pool_t* bp, unsigned int* count);

FX_METADATA(({ interface: [FX_BLOCK_POOL, V1] })) 

#endif
#ifndef _FX_EV_FLAGS_V1_HEADER_
#define _FX_EV_FLAGS_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_ev_flags.h
  *  @brief  Interface of event flags.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(FX_SYNC)
#include FX_INTERFACE(FX_RTP)
#include FX_INTERFACE(FX_THREAD)
#include FX_INTERFACE(RTL_QUEUE)

//!
//! Error codes.
//! 
enum
{
    FX_EV_FLAGS_MAGIC = 0x45564600, // EVF
    FX_EV_FLAGS_OK = FX_STATUS_OK,
    FX_EV_FLAGS_INVALID_PTR = FX_THREAD_ERR_MAX,
    FX_EV_FLAGS_INVALID_OBJ,
    FX_EV_FLAGS_INVALID_FLAGS,
    FX_EV_FLAGS_INVALID_OPTIONS,
    FX_EV_FLAGS_ERR_MAX
};

//!
//! Wait options.
//! 
enum
{
    FX_EV_FLAGS_OR = 0,     //!< Waiting any flag from specified set to be 1.
    FX_EV_FLAGS_AND = 1,    //!< Waitinf all flags from specified set to be 1.
    FX_EV_FLAGS_CLEAR = 2,  //!< Consume flags which make wait satisfied.
};

//!
//! Event flags representation. 
//! It ises temporary queue for items to be notified.
//!
typedef struct
{
    fx_sync_waitable_t waitable;
    rtl_queue_t temp;
    uint_fast32_t flags;
    fx_rtp_t rtp;
    lock_t lock;
} 
fx_ev_flags_t;

int fx_ev_flags_init(fx_ev_flags_t* evf);
int fx_ev_flags_deinit(fx_ev_flags_t* evf);
int fx_ev_flags_wait(
    fx_ev_flags_t* evf, 
    const uint_fast32_t req_flags, 
    const unsigned int option, 
    uint_fast32_t* state, 
    fx_event_t* cancel_ev
);
int fx_ev_flags_timedwait(
    fx_ev_flags_t* evf, 
    const uint_fast32_t req_flags, 
    const unsigned int option, 
    uint_fast32_t* state, 
    uint32_t tout
);
int fx_ev_flags_set(fx_ev_flags_t* evf, uint_fast32_t flags, bool type);

FX_METADATA(({ interface: [FX_EV_FLAGS, V1] }))

#endif
#ifndef _FX_RWLOCK_V1_HEADER_
#define _FX_RWLOCK_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_rwlock.h
  *  @brief  Interface of read/write lock.
  *
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(FX_THREAD)
#include FX_INTERFACE(FX_RTP)

enum
{
    FX_RWLOCK_MAGIC = 0x52574C4B, //RWLK
    FX_RWLOCK_OK = FX_STATUS_OK,
    FX_RWLOCK_INVALID_PTR = FX_THREAD_ERR_MAX,
    FX_RWLOCK_INVALID_OBJ,
    FX_RWLOCK_UNSUPPORTED_POLICY,
    FX_RWLOCK_INVALID_TIMEOUT,
    FX_RWLOCK_ERR_MAX
};

//!
//! RW-lock object. 
//! There are two wait queues used, one for readers and one for writers.
//!
typedef struct
{
    fx_sync_waitable_t rd_wtbl;
    fx_sync_waitable_t wr_wtbl;
    lock_t lock;
    fx_rtp_t rtp;
    unsigned int readers;
    fx_thread_t* owner;
    fx_sync_policy_t policy;
} 
fx_rwlock_t;

int fx_rwlock_init(fx_rwlock_t* rw, fx_sync_policy_t policy);
int fx_rwlock_deinit(fx_rwlock_t* rw);
int fx_rwlock_rd_timedlock(fx_rwlock_t* rw, uint32_t tout);
int fx_rwlock_wr_timedlock(fx_rwlock_t* rw, uint32_t tout);
int fx_rwlock_rd_lock(fx_rwlock_t* rw, fx_event_t* cancel_event);
int fx_rwlock_wr_lock(fx_rwlock_t* rw, fx_event_t* cancel_event);
int fx_rwlock_unlock(fx_rwlock_t* rw);
int fx_rwlock_unlock_with_policy(fx_rwlock_t* rw, fx_sync_policy_t policy);

FX_METADATA(({ interface: [FX_RWLOCK, V1] }))

#endif
#ifndef _FX_COND_V1_HEADER_
#define _FX_COND_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_cond.h
  *  @brief  Interface of condition variables.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(FX_SYNC)
#include FX_INTERFACE(FX_RTP)
#include FX_INTERFACE(FX_EVENT)
#include FX_INTERFACE(FX_THREAD)
#include FX_INTERFACE(FX_MUTEX)

enum
{
    FX_COND_MAGIC = 0x434F4E44, // COND
    FX_COND_OK = FX_STATUS_OK,
    FX_COND_INVALID_PTR = FX_THREAD_ERR_MAX,
    FX_COND_INVALID_OBJ,
    FX_COND_UNSUPPORTED_POLICY,
    FX_COND_INVALID_MUTEX,
    FX_COND_MUTEX_ERROR,
    FX_COND_INVALID_TIMEOUT,
    FX_COND_INVALID_PARAMETER,
    FX_COND_NO_WAITERS,
    FX_COND_ERR_MAX
};

//!
//! Condition variable representation. 
//!
typedef struct
{
    fx_sync_waitable_t waitable;
    fx_rtp_t rtp;
    lock_t lock;
    fx_sync_policy_t policy;
} 
fx_cond_t;

int fx_cond_init(fx_cond_t* cond, const fx_sync_policy_t policy);
int fx_cond_deinit(fx_cond_t* cond);
int fx_cond_signal(fx_cond_t* cond);
int fx_cond_signal_with_policy(fx_cond_t* cond, const fx_sync_policy_t policy);
int fx_cond_broadcast(fx_cond_t* cond);
int fx_cond_wait(fx_cond_t* cond, fx_mutex_t* mutex, fx_event_t* cancel_event);
int fx_cond_timedwait(fx_cond_t* cond, fx_mutex_t* mutex, uint32_t tout);

FX_METADATA(({ interface: [FX_COND, V1] }))

#endif
#ifndef _RTL_MEM_POOL_TLSF_HEADER_
#define _RTL_MEM_POOL_TLSF_HEADER_

/*
 * Two Level Segregated Fit memory allocator.
 *
 * Copyright (c) 2018 Eremex Ltd.
 * Copyright (c) 2016 National Cheng Kung University, Taiwan.
 * Copyright (c) 2006-2008, 2011, 2014 Matthew Conte.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL MATTHEW CONTE BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include FX_INTERFACE(CFG_OPTIONS)

#ifndef RTL_MEM_POOL_SUBDIV_LOG2
#define RTL_MEM_POOL_SUBDIV_LOG2 4
#endif

#ifndef RTL_MEM_POOL_MAX_CHUNK
#error RTL_MEM_POOL_MAX_CHUNK is not defined!
#endif

//
// Configuration constants.
//
enum 
{
    FL_INDEX_MAX = RTL_MEM_POOL_MAX_CHUNK,

    // log2 of number of linear subdivisions of block sizes. Larger
    // values require more memory in the control structure. Values of
    // 4 or 5 are typical.
    //
    SL_INDEX_COUNT_LOG2 = RTL_MEM_POOL_SUBDIV_LOG2,

    //
    // All allocation sizes and addresses are aligned.
    //
    ALIGN_SIZE_LOG2 = sizeof(void*) == 8 ? 3 : 2,
    ALIGN_SIZE = (1 << ALIGN_SIZE_LOG2),

    //
    // We support allocations of sizes up to (1 << FL_INDEX_MAX) bits.
    // However, because we linearly subdivide the second-level lists, and
    // our minimum size granularity is 4 bytes, it doesn't make sense to
    // create first-level lists for sizes smaller than SL_INDEX_COUNT * 4,
    // or (1 << (SL_INDEX_COUNT_LOG2 + 2)) bytes, as there we will be
    // trying to split size ranges into more slots than we have available.
    // Instead, we calculate the minimum threshold size, and place all
    // blocks below that size into the 0th first-level list.
    //
    SL_INDEX_COUNT = (1 << SL_INDEX_COUNT_LOG2),
    FL_INDEX_SHIFT = (SL_INDEX_COUNT_LOG2 + ALIGN_SIZE_LOG2),
    FL_INDEX_COUNT = (FL_INDEX_MAX - FL_INDEX_SHIFT + 1),

    SMALL_BLOCK_SIZE = (1 << FL_INDEX_SHIFT),
};

//
// Block header structure.
//
// There are several implementation subtleties involved:
// - The prev_phys_block field is only valid if the previous block is free.
// - The prev_phys_block field is actually stored at the end of the
//   previous block. It appears at the beginning of this structure only to
//   simplify the implementation.
// - The next_free / prev_free fields are only valid if the block is free.
//
typedef struct _rtl_block_header_t 
{
    struct _rtl_block_header_t* prev_phys_block;
    size_t size;            //!< The size of this block, minus the block header.
    struct _rtl_block_header_t *next_free;
    struct _rtl_block_header_t *prev_free;
} 
rtl_block_header_t;

//
// The TLSF pool structure.
//
typedef struct 
{
    rtl_block_header_t block_null;
    unsigned int fl_bitmap;
    unsigned int sl_bitmap[FL_INDEX_COUNT];
    rtl_block_header_t *blocks[FL_INDEX_COUNT][SL_INDEX_COUNT];
} 
rtl_mem_pool_t;

void rtl_mem_pool_init(rtl_mem_pool_t* pool);
bool rtl_mem_pool_add_mem(rtl_mem_pool_t* pool, void* mem, size_t bytes);
void* rtl_mem_pool_alloc(rtl_mem_pool_t* pool, size_t bytes);
void rtl_mem_pool_free(rtl_mem_pool_t* pool, void* ptr);
size_t rtl_mem_pool_get_max_blk(rtl_mem_pool_t* pool);

FX_METADATA(({ interface: [RTL_MEM_POOL, TLSF] }))

#endif
#ifndef _FX_MEM_POOL_TLSF_HEADER_
#define _FX_MEM_POOL_TLSF_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_mem_pool.h
  *  @brief  Wrapper for memory pool.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(FX_SPL)
#include FX_INTERFACE(RTL_MEM_POOL)

//
// Error codes.
//
enum
{
    FX_MEM_POOL_OK = FX_STATUS_OK,
    FX_MEM_POOL_INVALID_PTR,
    FX_MEM_POOL_INVALID_OBJ,
    FX_MEM_POOL_INVALID_BUF,
    FX_MEM_POOL_ZERO_SZ,
    FX_MEM_POOL_NO_MEM,
    FX_MEM_POOL_ERR_MAX
};

//!
//! Bytes pool structure.
//!
typedef struct 
{
    lock_t lock;
    rtl_mem_pool_t rtl_pool;
} 
fx_mem_pool_t;

int fx_mem_pool_init(fx_mem_pool_t* pool);
int fx_mem_pool_deinit(fx_mem_pool_t* pool);
int fx_mem_pool_add_mem(fx_mem_pool_t* pool, uintptr_t mem, size_t bytes);
int fx_mem_pool_alloc(fx_mem_pool_t* pool, size_t bytes, void** p);
int fx_mem_pool_free(fx_mem_pool_t* pool, void* ptr);
int fx_mem_pool_get_max_free_chunk(fx_mem_pool_t* pool, size_t* blk_sz);

FX_METADATA(({ interface: [FX_MEM_POOL, TLSF] }))

#endif
#ifndef _FXRTOS_STANDARD_CM4_HEADER_
#define _FXRTOS_STANDARD_CM4_HEADER_

/** 
  ******************************************************************************
  *  @file   standard-cortex-m4.h
  *  @brief  Kernel dependencies.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(HAL_INIT)
#include FX_INTERFACE(HAL_CPU_INTR)
#include FX_INTERFACE(FX_TIMER)
#include FX_INTERFACE(FX_THREAD)
#include FX_INTERFACE(FX_DPC)
#include FX_INTERFACE(FX_SEM)
#include FX_INTERFACE(FX_MUTEX)
#include FX_INTERFACE(FX_MSGQ)
#include FX_INTERFACE(FX_BLOCK_POOL)
#include FX_INTERFACE(FX_EV_FLAGS)
#include FX_INTERFACE(FX_RWLOCK)
#include FX_INTERFACE(FX_COND)
#include FX_INTERFACE(FX_MEM_POOL)

FX_METADATA(({ interface: [FXRTOS, STANDARD_CORTEX_M4] }))

#endif
