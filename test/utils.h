#ifndef _UTILS_H
#define _UTILS_H

#include <uv.h>

#ifndef UV_LOOP
#define UV_LOOP(h) (((uv_handle_t *) (h))->loop)
#endif

// libuv assert
#define ASSERT(expr)                                      \
 do {                                                     \
  if (!(expr)) {                                          \
    fprintf(stderr,                                       \
            "Assertion failed in %s on line %d: %s\n",    \
            __FILE__,                                     \
            __LINE__,                                     \
            #expr);                                       \
    abort();                                              \
  }                                                       \
 } while (0)


#define ABORT_IF_ERROR(err)                                 \
    do {                                                    \
        if(err < 0)                                         \
        {                                                   \
            fprintf(stderr,                                 \
                    "Error: (%d: %s) in %s on line %d:\n",  \
                    err,                                    \
                    uv_strerror(err),                       \
                    __FILE__,                               \
                    __LINE__);                              \
            abort();                                        \
        }                                                   \
    } while (0)


#define CLEANUP_CONNECTION(connection)  \
    do {    \
        myuvamqp_list_destroy(connection->channels);    \
    } while(0)

    
typedef struct {
    uv_timer_t timer;
    void (*callback)(void *);
    void *arg;
} my_timer_t;

void call_later(uv_loop_t *loop, int interval, void (*callback)(void *), void *arg);

#endif
