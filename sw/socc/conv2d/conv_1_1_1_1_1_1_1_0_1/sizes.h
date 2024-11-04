#define BATCHES         1
#define IN_ROWS         1
#define IN_COLS         1
#define IN_CHANNELS     1
#define KERNEL_ROWS     1
#define KERNEL_COLS     1
#define OUT_CHANNELS    1
#define PADDING         0
#define STRIDE          1

#define OUT_ROWS ((IN_ROWS + 2*PADDING - KERNEL_ROWS) / STRIDE + 1)
#define OUT_COLS ((IN_COLS + 2*PADDING - KERNEL_COLS) / STRIDE + 1)
