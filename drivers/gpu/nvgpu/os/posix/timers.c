/*
 * Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <sys/time.h>
#include <time.h>

#include <nvgpu/bug.h>
#include <nvgpu/log.h>
#include <nvgpu/timers.h>
#include <nvgpu/soc.h>

#define MSEC_PER_SEC    1000
#define USEC_PER_MSEC   1000
#define NSEC_PER_USEC   1000
#define NSEC_PER_MSEC   1000000
#define NSEC_PER_SEC    1000000000

s64 nvgpu_current_time_us(void)
{
	struct timeval now;
	s64 time_now;
	int ret;

	ret = gettimeofday(&now, NULL);
	if (ret != 0) {
		BUG();
	}

	time_now = nvgpu_safe_mult_s64((s64)now.tv_sec, (s64)1000000);
	time_now = nvgpu_safe_add_s64(time_now, (s64)now.tv_usec);

	return time_now;
}

static s64 get_time_ns(void)
{
	struct timespec ts;
	s64 t_ns;

	(void) clock_gettime(CLOCK_MONOTONIC, &ts);

	t_ns = nvgpu_safe_mult_s64(ts.tv_sec, 1000000000);
	t_ns = nvgpu_safe_add_s64(t_ns, ts.tv_nsec);

	return t_ns;
}

/*
 * Returns true if a > b;
 */
static bool time_after(s64 a, s64 b)
{
	return (nvgpu_safe_sub_s64(a, b) > 0);
}

int nvgpu_timeout_init(struct gk20a *g, struct nvgpu_timeout *timeout,
		       u32 duration, unsigned long flags)
{
	s64 duration_ns;

	if ((flags & ~NVGPU_TIMER_FLAG_MASK) != 0U) {
		return -EINVAL;
	}

	(void) memset(timeout, 0, sizeof(*timeout));

	timeout->g = g;
	timeout->flags = (unsigned int)flags;

	if ((flags & NVGPU_TIMER_RETRY_TIMER) != 0U) {
		timeout->retries.max_attempts = duration;
	} else {
		duration_ns = (s64)duration;
		duration_ns = nvgpu_safe_mult_s64(duration_ns, NSEC_PER_MSEC);
		timeout->time = nvgpu_safe_add_s64(nvgpu_current_time_ns(),
								duration_ns);
	}

	return 0;
}

/*
 * NOTE: Logging is disabled in safety release build.
 * So, in safety release configuration, messages will not be printed or logged.
 */
#ifdef CONFIG_NVGPU_LOGGING
static void nvgpu_timeout_expired_msg_print(struct nvgpu_timeout *timeout,
					 bool retry, void *caller,
					 const char *fmt, va_list args)
{
	struct gk20a *g = timeout->g;
	if ((timeout->flags & NVGPU_TIMER_SILENT_TIMEOUT) == 0U) {
		char buf[128];

		(void) vsnprintf(buf, sizeof(buf), fmt, args);

		if (retry) {
			nvgpu_err(g, "No more retries @ %p %s", caller, buf);
		} else {
			nvgpu_err(g, "Timeout detected @ %p %s", caller, buf);
		}
	}
}
#endif

static int nvgpu_timeout_expired_msg_cpu(struct nvgpu_timeout *timeout)
{
	if (get_time_ns() > timeout->time) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int nvgpu_timeout_expired_msg_retry(struct nvgpu_timeout *timeout)
{
	if (timeout->retries.attempted >= timeout->retries.max_attempts) {
		return -ETIMEDOUT;
	}

	timeout->retries.attempted++;
	return 0;
}

/*
 * NOTE: Logging is disabled in safety release build.
 * So, in safety release configuration, messages will not be printed or logged.
 */
int nvgpu_timeout_expired_msg_impl(struct nvgpu_timeout *timeout,
			      void *caller, const char *fmt, ...)
{
	int ret;

	if ((timeout->flags & NVGPU_TIMER_RETRY_TIMER) != 0U) {
		ret = nvgpu_timeout_expired_msg_retry(timeout);

#ifdef CONFIG_NVGPU_LOGGING
		if (ret != 0) {
			va_list args;

			va_start(args, fmt);
			nvgpu_timeout_expired_msg_print(timeout, true, caller,
								fmt, args);
			va_end(args);
		}
#endif
	} else {
		ret = nvgpu_timeout_expired_msg_cpu(timeout);

#ifdef CONFIG_NVGPU_LOGGING
		if (ret != 0) {
			va_list args;

			va_start(args, fmt);
			nvgpu_timeout_expired_msg_print(timeout, false, caller,
								fmt, args);
			va_end(args);
		}
#endif
	}

	return ret;
}

bool nvgpu_timeout_peek_expired(struct nvgpu_timeout *timeout)
{
	if ((timeout->flags & NVGPU_TIMER_RETRY_TIMER) != 0U) {
		return (timeout->retries.attempted >=
			timeout->retries.max_attempts);
	} else {
		return time_after(get_time_ns(), timeout->time);
	}
}

static void nvgpu_usleep(unsigned int usecs)
{
	struct timespec rqtp;
	s64 t_currentns, t_ns;

	t_currentns = get_time_ns();
	t_ns = (s64)usecs;
	t_ns = nvgpu_safe_mult_s64(t_ns, 1000);
	t_ns = nvgpu_safe_add_s64(t_ns, t_currentns);

	rqtp.tv_sec = t_ns / 1000000000;
	rqtp.tv_nsec = t_ns % 1000000000;

	(void) clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &rqtp, NULL);
}

void nvgpu_udelay(unsigned int usecs)
{
	if (usecs >= (unsigned int) 1000) {
		nvgpu_usleep(usecs);
	} else {
		nvgpu_delay_usecs(usecs);
	}
}

void nvgpu_usleep_range(unsigned int min_us, unsigned int max_us)
{
	nvgpu_udelay(min_us);
}

void nvgpu_msleep(unsigned int msecs)
{
	struct timespec rqtp;
	s64 t_currentns, t_ns;

	t_currentns = get_time_ns();
	t_ns = (s64)msecs;
	t_ns = nvgpu_safe_mult_s64(t_ns, 1000000);
	t_ns = nvgpu_safe_add_s64(t_ns, t_currentns);

	rqtp.tv_sec = t_ns / 1000000000;
	rqtp.tv_nsec = t_ns % 1000000000;

	(void) clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &rqtp, NULL);
}

s64 nvgpu_current_time_ms(void)
{
	return (s64)(get_time_ns() / NSEC_PER_MSEC);
}

s64 nvgpu_current_time_ns(void)
{
	return get_time_ns();
}

u64 nvgpu_hr_timestamp(void)
{
	return nvgpu_get_cycles();
}

#ifdef CONFIG_NVGPU_NON_FUSA
u64 nvgpu_hr_timestamp_us(void)
{
	return nvgpu_us_counter();
}
#endif