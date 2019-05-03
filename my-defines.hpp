
#define  MAIN_SYMBOL     symbol(symbol_code("EOS"), 4)

#define  PAY_FOR_ALBUM   asset( (int64_t)(0.1*10000), MAIN_SYMBOL )   // ÿ����һ������շ� 0.1 EOS
#define  PAY_FOR_PIC     asset( (int64_t)(0.5*10000), MAIN_SYMBOL )   // ÿ��ͼƬ�洢�շ�   0.5 EOS
#define  ZERO_FEE        asset( (int64_t)0,           MAIN_SYMBOL )   // 0 EOS

#define  NAME_MAX_LEN     36         // ������ơ�ͼƬ���Ƶ���󳤶�(��λ���ֽ�)
#define  DETAIL_MAX_LEN  512         // ͼƬ��������󳤶ȣ���λ���ֽ�)
#define  SHA256_SUM_LEN   64         // sha256 check sum�ĳ���(��λ���ֽ�)
#define  IPFS_HASH_LEN    46         // IPFS�ϵ��ļ���HASH�ĳ���(��λ���ֽ�)

#define  ALBUM_DEFAULT_COVER_PIC_IPFS_HASH  "QmWXcPZNCpoft3en2YLgV1jyZFHn4kJ3Cku3qHuj9YoSfy"  // ���Ĭ�Ϸ���ͼƬ�� ipfs hash

#define  REGULATOR_ACCOUNT_NAME            name("niuzzzzzzzzz")  // ����˻�����Ȩ����һ��ʱ�䷶Χ�ڣ�ɾ��Υ���ͼƬ
#define  REGULATOR_DELETE_TIME_LIMIT_SECS  31536000              // ʱ�䷶Χ����λ���룩��1��=31536000��
