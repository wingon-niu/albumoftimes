#include <utility>
#include <vector>
#include <string>
#include <eosio/eosio.hpp>
#include <eosio/time.hpp>
#include <eosio/asset.hpp>
#include <eosio/contract.hpp>
#include <eosio/dispatcher.hpp>
#include <eosio/print.hpp>

using namespace eosio;
using std::string;

// The  macro  EOSIO_DISPATCH is in /usr/opt/eosio.cdt/1.6.1/include/eosiolib/contracts/eosio/dispatcher.hpp
// Extend from EOSIO_DISPATCH

/**
 * Convenient macro to create contract apply handler
 *
 * @ingroup dispatcher
 * @note To be able to use this macro, the contract needs to be derived from eosio::contract
 * @param TYPE - The class name of the contract
 * @param MEMBERS - The sequence of available actions supported by this contract
 *
 * Example:
 * @code
 * EOSIO_DISPATCH_EX( eosio::bios, (setpriv)(setalimits)(setglimits)(setprods)(reqauth) )
 * @endcode
 */

#define EOSIO_DISPATCH_EX( TYPE, MEMBERS ) \
extern "C" { \
   [[eosio::wasm_entry]] \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      /* 此处对原本的 EOSIO_DISPATCH 宏代码进行了修改，以防收到假币。 */ \
      if( (code == receiver && action != name("transfer").value) || (code == name("eosio.token").value && action == name("transfer").value) ) { \
         switch( action ) { \
            EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
         } \
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
      } \
   } \
} \

#define  MAIN_SYMBOL    symbol(symbol_code("SYS"), 4)

#define  PAY_FOR_ALBUM   5*10000   // 每创建一个相册收费  5 eosc
#define  PAY_FOR_PIC    10*10000   // 每张图片存储收费   10 eosc

#define  NAME_MAX_LEN   36         // 相册名称、图片名称的最大长度(单位：字节)
#define  MD5_SUM_LEN    32         // md5 check sum的长度(单位：字节)
#define  IPFS_SUM_LEN   46         // IPFS上的文件的HASH的长度(单位：字节)

// 时光相册

CONTRACT albumoftimes : public eosio::contract {
public:
    using contract::contract;

    albumoftimes(name self, name first_receiver, datastream<const char*> ds) : contract(self, first_receiver, ds),
          _accounts(get_self(), get_self().value),
          _albums  (get_self(), get_self().value),
          _pics    (get_self(), get_self().value){};

    // 响应用户充值
    ACTION transfer(const name& from, const name& to, const asset& quantity, const string& memo);

    // 用户提现
    ACTION withdraw( const name to, const asset& quantity );

    // 创建相册
    ACTION createalbum(const name& owner, const string& name);

    // 上传图片
    ACTION uploadpic(const name& owner, const uint64_t& album_id, const string& name,
                     const string& md5_sum, const string& ipfs_sum, const string& thumb_ipfs_sum);

    // 设置图片展示到公共区
    ACTION displaypic(const name& owner, const uint64_t& id, const asset& fee);

    // 为图片点赞
    ACTION upvotepic(const name& user, const uint64_t& id);

    // 设置相册的封面图片
    ACTION setcover(const name& owner, const uint64_t& album_id, const string& cover_thumb_pic_ipfs_sum);

    // 删除图片（只删除EOS中的数据，IPFS中的图片依然存在）
    ACTION deletepic(const name& owner, const uint64_t& id);

    // 删除相册，如果相册中有图片，则不能删除，只能删除空相册
    ACTION deletealbum(const name& owner, const uint64_t& id);

    // 清除 multi_index 中的所有数据，测试时使用，上线时去掉
    ACTION clearalldata();

private:
    TABLE st_account {
        name         owner;
        asset        quantity;

        uint64_t primary_key() const { return owner.value; }
    };
    typedef eosio::multi_index<"accounts"_n, st_account> tb_accounts;

    TABLE st_album {
        name         owner;
        uint64_t     id;
        string       name;
        uint32_t     pay;
        string       cover_thumb_pic_ipfs_sum;

        uint64_t primary_key() const { return id; }
        uint64_t by_owner()    const { return owner.value; }
    };
    typedef eosio::multi_index<
        "albums"_n, st_album,
        indexed_by< "byowner"_n, const_mem_fun<st_album, uint64_t, &st_album::by_owner> >
    > tb_albums;

    TABLE st_pic {
        name         owner;
        uint64_t     album_id;
        uint64_t     id;
        string       name;
        string       md5_sum;
        string       ipfs_sum;
        string       thumb_ipfs_sum;
        uint32_t     pay;
        uint64_t     display_fee;
        uint32_t     upvote_num;

        uint64_t primary_key()    const { return id; }
        uint64_t by_owner()       const { return owner.value; }
        uint64_t by_album_id()    const { return album_id; }
        uint64_t by_display_fee() const { return ~display_fee; }
    };
    typedef eosio::multi_index<
        "pics"_n, st_pic,
        indexed_by< "byowner"_n,      const_mem_fun<st_pic, uint64_t, &st_pic::by_owner> >,
        indexed_by< "byalbumid"_n,    const_mem_fun<st_pic, uint64_t, &st_pic::by_album_id> >,
        indexed_by< "bydisplayfee"_n, const_mem_fun<st_pic, uint64_t, &st_pic::by_display_fee> >
    > tb_pics;

    tb_accounts _accounts;
    tb_albums   _albums;
    tb_pics     _pics;
};

EOSIO_DISPATCH_EX(albumoftimes, (transfer)(withdraw)(createalbum)(uploadpic)(displaypic)(upvotepic)(setcover)(deletepic)(deletealbum)(clearalldata))
