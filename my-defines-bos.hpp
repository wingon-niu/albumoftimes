
#define  MAIN_SYMBOL     symbol(symbol_code("BOS"), 4)

#define  PAY_FOR_ALBUM   asset( (int64_t)(0.036*10000), MAIN_SYMBOL )   // 每创建一个相册收费 0.036 BOS
#define  PAY_FOR_PIC     asset( (int64_t)(0.125*10000), MAIN_SYMBOL )   // 每张图片存储收费   0.125 BOS
#define  ZERO_FEE        asset( (int64_t)0,             MAIN_SYMBOL )   // 0 BOS
