#include <libwm/packets.h>
#include <libwm.h>
#include <minos/status.h>
#include <string.h>

#define PACKETS \
    /*CS packets*/\
    X(WmCreateWindowInfo) \
    X(WmCreateSHMRegion) \
    X(WmDrawSHMRegion) \
    /*SC packets*/ \
    X(WmEvent)

// min
#define PCONST(type, ...) sizeof(type) +
#define PSTRING(...)      0 +
#define X(T) size_t min_##T##_size = T##_PACKET 0;
PACKETS
#undef X
#undef PCONST
#undef PSTRING
// max
#define PCONST(type, ...) sizeof(type) + 
#define PSTRING(_1, _2, max) max +
#define X(T) size_t max_##T##_size = T##_PACKET 0;
PACKETS
#undef X
#undef PCONST
#undef PSTRING
// size
#define PCONST(type, field) ((void)payload->field, sizeof(type)) + 
#define PSTRING(_1, len, ...) payload->len + 
#define X(T) size_t size_##T (const T* payload) { return T##_PACKET 0; }
PACKETS
#undef X
#undef PCONST
#undef PSTRING
// cleanup
#define PCONST(type, field) (void)payload->field;
#define PSTRING(field, ...) if(payload->field) free(payload->field);
#define X(T) \
    void cleanup_##T (T* payload) { \
        T##_PACKET \
    } 
PACKETS
#undef X
#undef PCONST
#undef PSTRING
// read_memory
#define PCONST(type, field) \
    if(payload_size < sizeof(type)) { \
        e = -SIZE_MISMATCH; \
        goto err; \
    } \
    memcpy(&payload->field, buf, sizeof(type));\
    buf += sizeof(type);\
    payload_size -= sizeof(type);

#define PSTRING(field, len, max) \
    if(payload->len > max) { \
        e = -LIMITS; \
        goto err; \
    } \
    if(payload_size < payload->len) { \
        e = -SIZE_MISMATCH; \
        goto err; \
    } \
    payload->field = malloc(payload->len + 1); \
    if(!payload->field) { \
        e = -NOT_ENOUGH_MEM; \
        goto err; \
    } \
    memcpy(payload->field, buf, payload->len);\
    payload->field[payload->len] = '\0'; \
    buf += payload->len; \
    payload_size -= payload->len;
#define X(T) \
    int read_memory_##T (void* buf, size_t payload_size, T* payload) { \
        int e; \
        T##_PACKET \
        return 0; \
    err: \
        cleanup_##T(payload); \
        return e; \
    }
PACKETS
#undef X
#undef PCONST
#undef PSTRING
// write memory
#define PCONST(type, field) \
    memcpy(buf + n, &payload->field, sizeof(type));\
    n += sizeof(type);
#define PSTRING(field, len, ...) \
    memcpy(buf + n, payload->field, payload->len);\
    n += payload->len;
#define X(T) \
    size_t write_memory_##T (void* buf, const T* payload) { \
        size_t n = 0; \
        T##_PACKET \
        return n; \
    }
PACKETS
#undef X
#undef PCONST
#undef PSTRING
