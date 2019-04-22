#include "albumoftimes.hpp"
#include "my-dispatcher.hpp"
#include "my-defines.hpp"

// 响应用户充值
ACTION albumoftimes::transfer(const name& from, const name& to, const asset& quantity, const string& memo)
{
    if (from == _self) {
        return;
    }
    if (to != _self) {
        return;
    }
    if ("albumoftimes deposit" != memo) {
        return;
    }
    if ( !quantity.is_valid() || !quantity.is_amount_within_range() ) {
        return;
    }
    if ( !quantity.symbol.is_valid() || quantity.symbol != MAIN_SYMBOL ) {
        return;
    }
    if (quantity.amount <= 0) {
        return;
    }

    auto itr = _accounts.find(from.value);
    if( itr == _accounts.end() ) {
       itr = _accounts.emplace(_self, [&](auto& acnt){
          acnt.owner = from;
       });
    }

    _accounts.modify( itr, _self, [&]( auto& acnt ) {
       acnt.quantity += quantity;
    });
}

// 用户提现
ACTION albumoftimes::withdraw( const name to, const asset& quantity )
{
    require_auth( to );

    if ( !quantity.is_valid() || !quantity.is_amount_within_range() ) {
        return;
    }
    if ( !quantity.symbol.is_valid() || quantity.symbol != MAIN_SYMBOL ) {
        return;
    }
    if (quantity.amount <= 0) {
        return;
    }

    auto itr = _accounts.find( to.value );
    eosio::check(itr != _accounts.end(), "unknown account");

    _accounts.modify( itr, _self, [&]( auto& acnt ) {
        eosio::check( acnt.quantity >= quantity, "insufficient balance" );
        acnt.quantity -= quantity;
    });
    
    // TODO: 如果余额为0，则删除这条记录

    action(
        permission_level{ _self, "active"_n },
        "eosio.token"_n, "transfer"_n,
        std::make_tuple(_self, to, quantity, string("albumoftimes withdraw"))
    ).send();
}

// 创建相册
ACTION albumoftimes::createalbum(const name& owner, const string& name)
{
    require_auth( owner );
    eosio::check( name.length() <= NAME_MAX_LEN, "name is too long" );

    auto itr = _accounts.find( owner.value );
    eosio::check(itr != _accounts.end(), "unknown account");

    _accounts.modify( itr, _self, [&]( auto& acnt ) {
        eosio::check( acnt.quantity >= PAY_FOR_ALBUM, "insufficient balance" );
        acnt.quantity -= PAY_FOR_ALBUM;
    });
    
    // TODO: 如果余额为0，则删除这条记录

    _albums.emplace(_self, [&](auto& album){
        album.owner                    = owner;
        album.album_id                 = _albums.available_primary_key();
        album.name                     = name;
        album.album_pay                = PAY_FOR_ALBUM;
        album.cover_thumb_pic_ipfs_sum = ALBUM_DEFAULT_COVER_PIC_IPFS_HASH;
        album.create_time              = current_time_point().sec_since_epoch();
    });
}

// 上传图片
ACTION albumoftimes::uploadpic(const name& owner, const uint64_t& album_id, const string& name, const string& detail,
                               const string& md5_sum, const string& ipfs_sum, const string& thumb_ipfs_sum)
{
    require_auth( owner );
    eosio::check( name.length()           <= NAME_MAX_LEN,   "name is too long" );
    eosio::check( detail.length()         <= DETAIL_MAX_LEN, "detail is too long" );
    eosio::check( md5_sum.length()        == MD5_SUM_LEN,    "wrong md5_sum" );
    eosio::check( ipfs_sum.length()       == IPFS_SUM_LEN,   "wrong ipfs_sum" );
    eosio::check( thumb_ipfs_sum.length() == IPFS_SUM_LEN,   "wrong thumb_ipfs_sum" );

    auto itr_album = _albums.find( album_id );
    eosio::check(itr_album != _albums.end(), "unknown album_id");
    eosio::check(itr_album->owner == owner, "this album is not belong to this owner");

    auto itr_acnt = _accounts.find( owner.value );
    eosio::check(itr_acnt != _accounts.end(), "unknown account");

    _accounts.modify( itr_acnt, _self, [&]( auto& acnt ) {
        eosio::check( acnt.quantity >= PAY_FOR_PIC, "insufficient balance" );
        acnt.quantity -= PAY_FOR_PIC;
    });

    // TODO: 如果余额为0，则删除这条记录

    _pics.emplace(_self, [&](auto& pic){
        pic.owner                    = owner;
        pic.album_id                 = album_id;
        pic.pic_id                   = _pics.available_primary_key();
        pic.name                     = name;
        pic.detail                   = detail;
        pic.md5_sum                  = md5_sum;
        pic.ipfs_sum                 = ipfs_sum;
        pic.thumb_ipfs_sum           = thumb_ipfs_sum;
        pic.pic_pay                  = PAY_FOR_PIC;
        pic.public_album_id          = 0;
        pic.sort_fee                 = ZERO_FEE;
        pic.upvote_num               = 0;
        pic.upload_time              = current_time_point().sec_since_epoch();
    });
}

// 为公共相册中的图片支付排序靠前的费用
ACTION albumoftimes::paysortfee(const name& owner, const uint64_t& pic_id, const asset& sortfee)
{
    require_auth( owner );

    auto itr_pic = _pics.find( pic_id );
    eosio::check(itr_pic != _pics.end(), "unknown pic id");
    eosio::check(itr_pic->owner == owner, "this pic is not belong to this owner");

    if ( !sortfee.is_valid() || !sortfee.is_amount_within_range() ) {
        return;
    }
    if ( !sortfee.symbol.is_valid() || sortfee.symbol != MAIN_SYMBOL ) {
        return;
    }
    if (sortfee.amount <= 0) {
        return;
    }

    auto itr_acnt = _accounts.find( owner.value );
    eosio::check(itr_acnt != _accounts.end(), "unknown account");

    _accounts.modify( itr_acnt, _self, [&]( auto& acnt ) {
        eosio::check( acnt.quantity >= sortfee, "insufficient balance" );
        acnt.quantity -= sortfee;
    });
    
    // TODO: 如果余额为0，则删除这条记录

    _pics.modify( itr_pic, _self, [&]( auto& pic ) {
        pic.sort_fee += sortfee;
    });

    // TODO: 如果这个图片在一个公共相册里面，则可能需要更新这个公共相册的封面，由排序最前的图片当封面。

    //
}

// 为图片点赞
ACTION albumoftimes::upvotepic(const name& user, const uint64_t& pic_id)
{
    require_auth( user );

    auto itr_pic = _pics.find( pic_id );
    eosio::check(itr_pic != _pics.end(), "unknown pic id");
    eosio::check(itr_pic->owner != user, "can not upvote pic by self");

    _pics.modify( itr_pic, _self, [&]( auto& pic ) {
        pic.upvote_num += 1;  // 也许很多年以后，这个字段会溢出，会有这一天吗？！  Maybe overflow after many many years, will there be this day?!
    });
}

// 设置相册的封面图片
ACTION albumoftimes::setcover(const name& owner, const uint64_t& album_id, const string& cover_thumb_pic_ipfs_sum)
{
    require_auth( owner );
    eosio::check( cover_thumb_pic_ipfs_sum.length() == IPFS_SUM_LEN, "wrong cover_thumb_pic_ipfs_sum" );

    auto itr_album = _albums.find( album_id );
    eosio::check(itr_album != _albums.end(), "unknown album_id");
    eosio::check(itr_album->owner == owner, "this album is not belong to this owner");

    _albums.modify( itr_album, _self, [&]( auto& album ) {
        album.cover_thumb_pic_ipfs_sum = cover_thumb_pic_ipfs_sum;
    });
}

// 删除图片（只删除EOS中的数据，IPFS中的图片依然存在）
ACTION albumoftimes::deletepic(const name& owner, const uint64_t& pic_id)
{
    require_auth( owner );

    auto itr_pic = _pics.find( pic_id );
    eosio::check(itr_pic != _pics.end(), "unknown pic id");
    eosio::check(itr_pic->owner == owner, "this pic is not belong to this owner");

    // TODO: 如果这个图片是所属个人的某个相册的封面，则需要将这个相册的封面改回系统默认封面

    //

    _pics.erase(itr_pic);

    // TODO: 如果这个图片是所属公共相册的封面，则需要更新这个公共相册的封面，由新的排序最前的图片当封面。

    //
}

// 删除相册，如果相册中有图片，则不能删除，只能删除空相册
ACTION albumoftimes::deletealbum(const name& owner, const uint64_t& album_id)
{
    require_auth( owner );

    auto itr_album = _albums.find( album_id );
    eosio::check(itr_album != _albums.end(), "unknown album id");
    eosio::check(itr_album->owner == owner, "this album is not belong to this owner");

    auto album_index = _pics.get_index<name("byalbumid")>();
    auto itr = album_index.lower_bound(album_id);
    bool has_pic_in_album = false;
    if (itr != album_index.end() && itr->album_id == album_id) {
        has_pic_in_album = true;
    }
    eosio::check(has_pic_in_album == false, "album is not empty");

    _albums.erase(itr_album);
}

// 监管删除违规图片
ACTION albumoftimes::rmillegalpic(const uint64_t& pic_id)
{
    require_auth( REGULATOR_ACCOUNT_NAME );

    auto itr_pic = _pics.find( pic_id );
    eosio::check(itr_pic != _pics.end(), "unknown pic id");
        
    // 检查是否超出可以删除的时间期限
    if ( current_time_point().sec_since_epoch() - itr_pic->upload_time > REGULATOR_DELETE_TIME_LIMIT_SECS ) {
        return;
    }

    // TODO: 如果这个图片是所属个人的某个相册的封面，则需要将这个相册的封面改回系统默认封面

    //

    _pics.erase(itr_pic);

    // TODO: 如果这个图片是所属公共相册的封面，则需要更新这个公共相册的封面，由新的排序最前的图片当封面。

    //
}

// 创建公共相册
ACTION albumoftimes::makepubalbum(const string& name)
{
    require_auth( _self );
    eosio::check( name.length() <= NAME_MAX_LEN, "name is too long" );

    _pub_albums.emplace(_self, [&](auto& pub_album){
        pub_album.owner                    = _self;
        pub_album.pub_album_id             = _pub_albums.available_primary_key();
        pub_album.name                     = name;
        pub_album.cover_thumb_pic_ipfs_sum = ALBUM_DEFAULT_COVER_PIC_IPFS_HASH;
        pub_album.create_time              = current_time_point().sec_since_epoch();
    });
}

// 将图片加入某个公共相册
ACTION albumoftimes::joinpubalbum(const name& owner, const uint64_t& pic_id, const uint64_t& pub_album_id)
{
    require_auth( owner );

    auto itr_pic = _pics.find( pic_id );
    eosio::check(itr_pic != _pics.end(), "unknown pic id");
    eosio::check(itr_pic->owner == owner, "this pic is not belong to this owner");

    auto itr_pub_album = _pub_albums.find( pub_album_id );
    eosio::check(itr_pub_album != _pub_albums.end(), "unknown pub album id");

    uint64_t old_pub_album_id = itr_pic->public_album_id;
    if ( old_pub_album_id == pub_album_id ) {
        return;
    }

    _pics.modify( itr_pic, _self, [&]( auto& pic ) {
        pic.public_album_id = pub_album_id;
    });

    // TODO: 如果这个图片是它以前所属公共相册的封面，则需要更新它以前所属公共相册的封面。
    if ( old_pub_album_id > 0 ) {
        //
    }

    // TODO: 如果这个图片以前曾经支付过在公共相册中的排序费用，则它可能成为新的所属公共相册的封面。
    if ( itr_pic->sort_fee.amount > 0 ) {
        //
    }
}

// 将图片移出所属公共相册
ACTION albumoftimes::outpubalbum(const name& owner, const uint64_t& pic_id)
{
    require_auth( owner );

    auto itr_pic = _pics.find( pic_id );
    eosio::check(itr_pic != _pics.end(), "unknown pic id");
    eosio::check(itr_pic->owner == owner, "this pic is not belong to this owner");

    uint64_t old_pub_album_id = itr_pic->public_album_id;
    if ( old_pub_album_id == 0 ) {
        return;
    }

    _pics.modify( itr_pic, _self, [&]( auto& pic ) {
        pic.public_album_id = 0;
    });

    // TODO: 如果这个图片是它以前所属公共相册的封面，则需要更新它以前所属公共相册的封面。
    if ( old_pub_album_id > 0 ) {
        //
    }
}

// 将图片移动到另一个相册（图片可以在个人相册之间移动）
ACTION albumoftimes::movetoalbum(const name& owner, const uint64_t& pic_id, const uint64_t& dst_album_id)
{
}

// 修改个人相册的名字
ACTION albumoftimes::renamealbum(const name& owner, const uint64_t& album_id, const string& new_name)
{
}

// 修改图片的名字和描述
ACTION albumoftimes::modifypicnd(const name& owner, const uint64_t& pic_id, const string& new_name, const string& new_detail)
{
}

// 修改公共相册的名字
ACTION albumoftimes::rnpubalbum(const uint64_t& pub_album_id, const string& new_name)
{
}

// 清除 multi_index 中的所有数据，测试时使用，上线时去掉
ACTION albumoftimes::clearalldata()
{
    require_auth( _self );
    std::vector<uint64_t> keysForDeletion;
    print("\nclear all data.\n");

    keysForDeletion.clear();
    for (auto& item : _accounts) {
        keysForDeletion.push_back(item.owner.value);
    }
    for (uint64_t key : keysForDeletion) {
        auto itr = _accounts.find(key);
        if (itr != _accounts.end()) {
            _accounts.erase(itr);
        }
    }

    keysForDeletion.clear();
    for (auto& item : _albums) {
        keysForDeletion.push_back(item.album_id);
    }
    for (uint64_t key : keysForDeletion) {
        auto itr = _albums.find(key);
        if (itr != _albums.end()) {
            _albums.erase(itr);
        }
    }

    keysForDeletion.clear();
    for (auto& item : _pics) {
        keysForDeletion.push_back(item.pic_id);
    }
    for (uint64_t key : keysForDeletion) {
        auto itr = _pics.find(key);
        if (itr != _pics.end()) {
            _pics.erase(itr);
        }
    }
}
