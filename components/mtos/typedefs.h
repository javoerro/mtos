typedef enum {
    MTOS_EVENT_ANY = -1,
    MTOS_EVENT_MASTER_CALL,
    MTOS_EVENT_MASTER_ANSWERED,
    MTOS_EVENT_MASTER_CHUNK_RX,
    MTOS_EVENT_MASTER_UPDATED,
    MTOS_EVENT_MASTER_IDLE,
    MTOS_EVENT_MASTER_TIMEOUT,
    MTOS_EVENT_MASTER_ALLOC_ERROR,
    MTOS_EVENT_SLAVE_INITED,
    MTOS_EVENT_SLAVE_DEMANDED,
    MTOS_EVENT_SLAVE_CHUNK_RQ,
    MTOS_EVENT_SLAVE_RELEASED,
    MTOS_EVENT_SLAVE_FINISHED,
    MTOS_EVENT_SLAVE_TIMEOUT,
    MTOS_EVENT_SLAVE_ALLOC_ERROR
} mtos_event_id_t;


typedef union {
    struct __attribute__((packed)) {
        uint8_t* chunk;
        size_t size;
        size_t count;
        size_t pending;
        char* name;
    } chunk_rx;
    struct __attribute__((packed)) {
        size_t max_size;
        size_t tx_size;
        uint8_t resend;
        char* name;
    } chunk_rq;
} mtos_event_chunk_t;


typedef void (*mtos_event_handler_t)(mtos_event_id_t event_id, void* event_data);