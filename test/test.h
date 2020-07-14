#include <stdio.h>
#include <stdlib.h>

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
