
#define  MAIN_SYMBOL     symbol(symbol_code("EOS"), 4)

#define  PAY_FOR_ALBUM   asset( (int64_t)(0.036*10000), MAIN_SYMBOL )   // ÿ����һ������շ� 0.036 EOS
#define  PAY_FOR_PIC     asset( (int64_t)(0.125*10000), MAIN_SYMBOL )   // ÿ��ͼƬ�洢�շ�   0.125 EOS
#define  ZERO_FEE        asset( (int64_t)0,             MAIN_SYMBOL )   // 0 EOS
