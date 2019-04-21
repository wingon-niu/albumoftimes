#include <eosio/eosio.hpp>
#include <eosio/time.hpp>
#include <eosio/system.hpp>
#include <eosio/asset.hpp>
#include <eosio/contract.hpp>
#include <eosio/dispatcher.hpp>
#include <eosio/print.hpp>
#include <utility>
#include <vector>
#include <string>

using namespace eosio;
using std::string;

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
    ACTION uploadpic(const name& owner, const uint64_t& album_id, const string& name, const string& detail,
                     const string& md5_sum, const string& ipfs_sum, const string& thumb_ipfs_sum);

    // 为公共相册中的图片支付排序靠前的费用
    ACTION paysortfee(const name& owner, const uint64_t& pic_id, const asset& sortfee);

    // 为图片点赞
    ACTION upvotepic(const name& user, const uint64_t& pic_id);

    // 设置相册的封面图片
    ACTION setcover(const name& owner, const uint64_t& album_id, const string& cover_thumb_pic_ipfs_sum);

    // 删除图片（只删除EOS中的数据，IPFS中的图片依然存在）
    ACTION deletepic(const name& owner, const uint64_t& pic_id);

    // 删除相册，如果相册中有图片，则不能删除，只能删除空相册
    ACTION deletealbum(const name& owner, const uint64_t& album_id);

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
        uint64_t     album_id;
        string       name;
        asset        album_pay;
        string       cover_thumb_pic_ipfs_sum;

        uint64_t primary_key() const { return album_id; }
        uint64_t by_owner()    const { return owner.value; }
    };
    typedef eosio::multi_index<
        "albums"_n, st_album,
        indexed_by< "byowner"_n, const_mem_fun<st_album, uint64_t, &st_album::by_owner> >
    > tb_albums;

    TABLE st_pic {
        name         owner;
        uint64_t     album_id;
        uint64_t     pic_id;
        string       name;
        string       detail;
        string       md5_sum;
        string       ipfs_sum;
        string       thumb_ipfs_sum;
        asset        pic_pay;
        uint64_t     public_album_id;
        asset        sort_fee;
        uint32_t     upvote_num;
        uint32_t     upload_time;

        uint64_t primary_key()           const { return pic_id; }
        uint64_t by_owner()              const { return owner.value; }
        uint64_t by_album_id()           const { return album_id; }
        uint64_t by_pub_album_sort_fee() const {
            int64_t i = sort_fee.amount;
            if (i<0) { i *= -1; }
            uint64_t ui = (uint64_t)i;
            uint64_t pai = public_album_id;
            return ( (pai<<32) + (~ui) ); 
        }
    };
    typedef eosio::multi_index<
        "pics"_n, st_pic,
        indexed_by< "byowner"_n,      const_mem_fun<st_pic, uint64_t, &st_pic::by_owner> >,
        indexed_by< "byalbumid"_n,    const_mem_fun<st_pic, uint64_t, &st_pic::by_album_id> >,
        indexed_by< "bypubsortfee"_n, const_mem_fun<st_pic, uint64_t, &st_pic::by_pub_album_sort_fee> >
    > tb_pics;

    tb_accounts _accounts;
    tb_albums   _albums;
    tb_pics     _pics;
};
