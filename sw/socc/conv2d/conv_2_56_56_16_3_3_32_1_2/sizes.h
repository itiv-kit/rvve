#define BATCHES         2
#define IN_ROWS         56
#define IN_COLS         56
#define IN_CHANNELS     16
#define KERNEL_ROWS     3
#define KERNEL_COLS     3
#define OUT_CHANNELS    128
#define PADDING         1
#define STRIDE          2

#define OUT_ROWS ((IN_ROWS + 2*PADDING - KERNEL_ROWS) / STRIDE + 1)
#define OUT_COLS ((IN_COLS + 2*PADDING - KERNEL_COLS) / STRIDE + 1)
