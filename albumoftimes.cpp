#include "albumoftimes.hpp"

// 响应用户充值
ACTION albumoftimes::transfer(const name& from,
                              const name& to,
                              const asset& quantity,
                              const string& memo)
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
        eosio::check( acnt.quantity.amount >= PAY_FOR_ALBUM, "insufficient balance" );
        acnt.quantity.amount -= PAY_FOR_ALBUM;
    });

    _albums.emplace(_self, [&](auto& album){
        album.owner                    = owner;
        album.id                       = _albums.available_primary_key();
        album.name                     = name;
        album.pay                      = PAY_FOR_ALBUM;
        album.cover_thumb_pic_ipfs_sum = string("default");
    });
}

// 上传图片
ACTION albumoftimes::uploadpic(const name& owner, const uint64_t& album_id, const string& name,
                               const string& md5_sum, const string& ipfs_sum, const string& thumb_ipfs_sum)
{
    require_auth( owner );
    eosio::check( name.length()           <= NAME_MAX_LEN, "name is too long" );
    eosio::check( md5_sum.length()        == MD5_SUM_LEN,  "wrong md5_sum" );
    eosio::check( ipfs_sum.length()       == IPFS_SUM_LEN, "wrong ipfs_sum" );
    eosio::check( thumb_ipfs_sum.length() == IPFS_SUM_LEN, "wrong thumb_ipfs_sum" );

    auto itr_album = _albums.find( album_id );
    eosio::check(itr_album != _albums.end(), "unknown album_id");
    eosio::check(itr_album->owner == owner, "this album is not belong to this owner");

    auto itr_acnt = _accounts.find( owner.value );
    eosio::check(itr_acnt != _accounts.end(), "unknown account");

    _accounts.modify( itr_acnt, _self, [&]( auto& acnt ) {
        eosio::check( acnt.quantity.amount >= PAY_FOR_PIC, "insufficient balance" );
        acnt.quantity.amount -= PAY_FOR_PIC;
    });

    _pics.emplace(_self, [&](auto& pic){
        pic.owner                    = owner;
        pic.album_id                 = album_id;
        pic.id                       = _pics.available_primary_key();
        pic.name                     = name;
        pic.md5_sum                  = md5_sum;
        pic.ipfs_sum                 = ipfs_sum;
        pic.thumb_ipfs_sum           = thumb_ipfs_sum;
        pic.pay                      = PAY_FOR_PIC;
        pic.display_fee              = 0;
        pic.upvote_num               = 0;
    });
}

// 设置图片展示到公共区
ACTION albumoftimes::displaypic(const name& owner, const uint64_t& id, const asset& fee)
{
    require_auth( owner );

    auto itr_pic = _pics.find( id );
    eosio::check(itr_pic != _pics.end(), "unknown pic id");
    eosio::check(itr_pic->owner == owner, "this pic is not belong to this owner");

    if ( !fee.is_valid() || !fee.is_amount_within_range() ) {
        return;
    }
    if ( !fee.symbol.is_valid() || fee.symbol != MAIN_SYMBOL ) {
        return;
    }
    if (fee.amount <= 0) {
        return;
    }

    auto itr_acnt = _accounts.find( owner.value );
    eosio::check(itr_acnt != _accounts.end(), "unknown account");

    _accounts.modify( itr_acnt, _self, [&]( auto& acnt ) {
        eosio::check( acnt.quantity >= fee, "insufficient balance" );
        acnt.quantity -= fee;
    });

    _pics.modify( itr_pic, _self, [&]( auto& pic ) {
        pic.display_fee += fee.amount;
    });
}

// 为图片点赞
ACTION albumoftimes::upvotepic(const name& user, const uint64_t& id)
{
    require_auth( user );

    auto itr_pic = _pics.find( id );
    eosio::check(itr_pic != _pics.end(), "unknown pic id");
    eosio::check(itr_pic->owner != user, "can not upvote pic by self");

    _pics.modify( itr_pic, _self, [&]( auto& pic ) {
        pic.upvote_num += 1;
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
ACTION albumoftimes::deletepic(const name& owner, const uint64_t& id)
{
    require_auth( owner );

    auto itr_pic = _pics.find( id );
    eosio::check(itr_pic != _pics.end(), "unknown pic id");
    eosio::check(itr_pic->owner == owner, "this pic is not belong to this owner");

    _pics.erase(itr_pic);
}

// 删除相册，如果相册中有图片，则不能删除，只能删除空相册
ACTION albumoftimes::deletealbum(const name& owner, const uint64_t& id)
{
    require_auth( owner );

    auto itr_album = _albums.find( id );
    eosio::check(itr_album != _albums.end(), "unknown album id");
    eosio::check(itr_album->owner == owner, "this album is not belong to this owner");

    auto album_index = _pics.get_index<name("byalbumid")>();
    auto itr = album_index.lower_bound(id);
    bool has_pic_in_album = false;
    if (itr != album_index.end() && itr->album_id == id) {
        has_pic_in_album = true;
    }
    eosio::check(has_pic_in_album == false, "album is not empty");

    _albums.erase(itr_album);
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
        keysForDeletion.push_back(item.id);
    }
    for (uint64_t key : keysForDeletion) {
        auto itr = _albums.find(key);
        if (itr != _albums.end()) {
            _albums.erase(itr);
        }
    }

    keysForDeletion.clear();
    for (auto& item : _pics) {
        keysForDeletion.push_back(item.id);
    }
    for (uint64_t key : keysForDeletion) {
        auto itr = _pics.find(key);
        if (itr != _pics.end()) {
            _pics.erase(itr);
        }
    }
}
