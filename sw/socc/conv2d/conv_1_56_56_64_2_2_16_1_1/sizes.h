#define BATCHES         1
#define IN_ROWS         56
#define IN_COLS         56
#define IN_CHANNELS     64
#define KERNEL_ROWS     2
#define KERNEL_COLS     2
#define OUT_CHANNELS    16
#define PADDING         1
#define STRIDE          1

#define OUT_ROWS ((IN_ROWS + 2*PADDING - KERNEL_ROWS) / STRIDE + 1)
#define OUT_COLS ((IN_COLS + 2*PADDING - KERNEL_COLS) / STRIDE + 1)
