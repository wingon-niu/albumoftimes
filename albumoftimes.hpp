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
          _accounts    (get_self(), get_self().value),
          _pub_albums  (get_self(), get_self().value),
          _albums      (get_self(), get_self().value),
          _pics        (get_self(), get_self().value){};

    // 响应用户充值
    ACTION transfer(const name& from, const name& to, const asset& quantity, const string& memo);

    // 用户提现
    ACTION withdraw( const name to, const asset& quantity );

    // 创建公共相册
    ACTION makepubalbum(const string& name_cn, const string& name_en);

    // 修改公共相册的名字
    ACTION rnpubalbum(const uint64_t& pub_album_id, const string& new_name_cn, const string& new_name_en);

    // 创建个人相册
    ACTION createalbum(const name& owner, const string& name);

    // 修改个人相册的名字
    ACTION renamealbum(const name& owner, const uint64_t& album_id, const string& new_name);

    // 上传图片
    ACTION uploadpic(const name& owner, const uint64_t& album_id, const string& name, const string& detail,
                     const string& md5_sum, const string& ipfs_sum, const string& thumb_ipfs_sum);

    // 修改图片的名字和描述
    ACTION modifypicnd(const name& owner, const uint64_t& pic_id, const string& new_name, const string& new_detail);

    // 设置个人相册的封面图片
    ACTION setcover(const name& owner, const uint64_t& album_id, const string& cover_thumb_pic_ipfs_sum);

    // 将图片移动到另一个相册（图片可以在个人相册之间移动）
    ACTION movetoalbum(const name& owner, const uint64_t& pic_id, const uint64_t& dst_album_id);

    // 将图片加入某个公共相册
    ACTION joinpubalbum(const name& owner, const uint64_t& pic_id, const uint64_t& pub_album_id);

    // 将图片移出所属公共相册
    ACTION outpubalbum(const name& owner, const uint64_t& pic_id);

    // 为公共相册中的图片支付排序靠前的费用
    ACTION paysortfee(const name& owner, const uint64_t& pic_id, const asset& sortfee);

    // 为图片点赞
    ACTION upvotepic(const name& user, const uint64_t& pic_id);

    // 删除图片（只删除EOS中的数据，IPFS中的图片依然存在）
    ACTION deletepic(const name& owner, const uint64_t& pic_id);

    // 监管删除违规图片
    ACTION rmillegalpic(const uint64_t& pic_id);

    // 删除个人相册，如果相册中有图片，则不能删除，只能删除空相册
    ACTION deletealbum(const name& owner, const uint64_t& album_id);

    // 清除 multi_index 中的所有数据，测试时使用，上线时去掉
    ACTION clearalldata();

private:

    // 将个人相册的图片数量+1
    void add_private_album_pic_num(const uint64_t& private_album_id);

    // 将个人相册的图片数量-1
    void sub_private_album_pic_num(const uint64_t& private_album_id);

    // 将公共相册的图片数量+1
    void add_public_album_pic_num(const uint64_t& public_album_id);

    // 将公共相册的图片数量-1
    void sub_public_album_pic_num(const uint64_t& public_album_id);

    // 将个人相册的封面设置为系统默认封面
    void set_private_album_default_cover(const uint64_t& private_album_id);

    // 公共相册重新选择封面，将排序最前的图片设置为封面
    void update_public_album_cover(const uint64_t& public_album_id);

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
        uint32_t     create_time;
        uint64_t     pic_num;

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
            int64_t fee = sort_fee.amount;
            if ( fee < 0 ) { fee *= -1; }
            uint64_t ui64_0x32_fee = (uint64_t)fee;
            uint32_t ui32_0x32 = 0;
            uint32_t ui32_1x32 = ~ui32_0x32;
            uint64_t ui64_0x32_1x32 = (uint64_t)ui32_1x32;
            uint64_t ui64_1x32_0x32 = ui64_0x32_1x32<<32;
            uint64_t ui64_1x32_fee  = ui64_1x32_0x32 + ui64_0x32_fee;
            uint64_t pai = public_album_id;
            return ( (pai<<32) + (~ui64_1x32_fee) ); 
        }
    };
    typedef eosio::multi_index<
        "pics"_n, st_pic,
        indexed_by< "byowner"_n,      const_mem_fun<st_pic, uint64_t, &st_pic::by_owner> >,
        indexed_by< "byalbumid"_n,    const_mem_fun<st_pic, uint64_t, &st_pic::by_album_id> >,
        indexed_by< "bypubsortfee"_n, const_mem_fun<st_pic, uint64_t, &st_pic::by_pub_album_sort_fee> >
    > tb_pics;

    TABLE st_pub_album {
        name         owner;
        uint64_t     pub_album_id;
        string       name_cn;
        string       name_en;
        string       cover_thumb_pic_ipfs_sum;
        uint32_t     create_time;
        uint64_t     pic_num;

        uint64_t primary_key() const { return pub_album_id; }
        uint64_t by_owner()    const { return owner.value; }
    };
    typedef eosio::multi_index<
        "pubalbums"_n, st_pub_album,
        indexed_by< "byowner"_n, const_mem_fun<st_pub_album, uint64_t, &st_pub_album::by_owner> >
    > tb_pub_albums;

    tb_accounts     _accounts;
    tb_pub_albums   _pub_albums;
    tb_albums       _albums;
    tb_pics         _pics;
};
