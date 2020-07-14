#include <uv.h>
#include "myuvamqp.h"
#include "utils.h"

static void timer_cb(uv_timer_t *timer);
static void cb_timer_closed(uv_handle_t *handle);

void call_later(uv_loop_t *loop, int interval, void (*callback)(void *), void *arg)
{
    my_timer_t *timer = myuvamqp_mem_alloc(sizeof(*timer));

    timer->callback = callback;
    timer->arg = arg;

    uv_timer_init(loop, (uv_timer_t *) timer);
    uv_timer_start((uv_timer_t *) timer, timer_cb, interval, 0);
}


static void timer_cb(uv_timer_t *timer)
{
    my_timer_t *my_timer = (my_timer_t *) timer;
    void *arg = my_timer->arg;
    void (*callback)(void *) = my_timer->callback;

    uv_close((uv_handle_t *) timer, cb_timer_closed);

    callback(arg);
}

static void cb_timer_closed(uv_handle_t *handle)
{
    myuvamqp_mem_free(handle);
}
