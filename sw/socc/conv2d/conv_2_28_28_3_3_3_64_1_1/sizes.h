#define BATCHES         1
#define IN_ROWS         28
#define IN_COLS         28
#define IN_CHANNELS     16
#define KERNEL_ROWS     3
#define KERNEL_COLS     3
#define OUT_CHANNELS    64
#define PADDING         1
#define STRIDE          1

#define OUT_ROWS ((IN_ROWS + 2*PADDING - KERNEL_ROWS) / STRIDE + 1)
#define OUT_COLS ((IN_COLS + 2*PADDING - KERNEL_COLS) / STRIDE + 1)
