
#define  MAIN_SYMBOL     symbol(symbol_code("EOS"), 4)

#define  PAY_FOR_ALBUM   asset( (int64_t)(0.036*10000), MAIN_SYMBOL )   // 每创建一个相册收费 0.036 EOS
#define  PAY_FOR_PIC     asset( (int64_t)(0.125*10000), MAIN_SYMBOL )   // 每张图片存储收费   0.125 EOS
#define  ZERO_FEE        asset( (int64_t)0,             MAIN_SYMBOL )   // 0 EOS

#define  NAME_MAX_LEN      36         // 相册名称、图片名称的最大长度(单位：字节)
#define  DETAIL_MAX_LEN   512         // 图片描述的最大长度（单位：字节)
#define  SHA256_SUM_LEN    64         // sha256 check sum的长度(单位：字节)
#define  IPFS_HASH_LEN     46         // IPFS上的文件的HASH的长度(单位：字节)

#define  ALBUM_DEFAULT_COVER_PIC_IPFS_HASH  "QmWXcPZNCpoft3en2YLgV1jyZFHn4kJ3Cku3qHuj9YoSfy"  // 相册默认封面图片的 ipfs hash

#define  REGULATOR_ACCOUNT_NAME             name("niuzzzzzzzzz")  // 监管账户，有权限在一个时间范围内，删除违规的图片
#define  REGULATOR_DELETE_TIME_LIMIT_SECS   31536000              // 时间范围（单位：秒），1年=31536000秒
