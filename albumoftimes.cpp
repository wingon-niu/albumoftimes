#include "albumoftimes.hpp"
#include "my-dispatcher.hpp"
#include "my-defines-dev.hpp"
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
    //if ( !quantity.is_valid() || !quantity.is_amount_within_range() ) {
    //    return;
    //}
    //if ( !quantity.symbol.is_valid() || quantity.symbol != MAIN_SYMBOL ) {
    //    return;
    //}
    //if (quantity.amount <= 0) {
    //    return;
    //}
    eosio::check(quantity.symbol.is_valid(),        "invalid quantity inputed");
    eosio::check(quantity.symbol == MAIN_SYMBOL,    "invalid quantity inputed");
    eosio::check(quantity.is_valid(),               "invalid quantity inputed");
    eosio::check(quantity.is_amount_within_range(), "invalid quantity inputed");
    eosio::check(quantity.amount > 0,               "invalid quantity inputed");

    auto itr = _accounts.find(from.value);
    if( itr == _accounts.end() ) {
       itr = _accounts.emplace(_self, [&](auto& acnt){
          acnt.owner    = from;
          acnt.quantity = ZERO_FEE;
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

    //if ( !quantity.is_valid() || !quantity.is_amount_within_range() ) {
    //    return;
    //}
    //if ( !quantity.symbol.is_valid() || quantity.symbol != MAIN_SYMBOL ) {
    //    return;
    //}
    //if (quantity.amount <= 0) {
    //    return;
    //}
    eosio::check(quantity.symbol.is_valid(),        "invalid quantity inputed");
    eosio::check(quantity.symbol == MAIN_SYMBOL,    "invalid quantity inputed");
    eosio::check(quantity.is_valid(),               "invalid quantity inputed");
    eosio::check(quantity.is_amount_within_range(), "invalid quantity inputed");
    eosio::check(quantity.amount > 0,               "invalid quantity inputed");

    auto itr = _accounts.find( to.value );
    eosio::check(itr != _accounts.end(), "unknown account");

    _accounts.modify( itr, _self, [&]( auto& acnt ) {
        eosio::check( acnt.quantity >= quantity, "insufficient balance" );
        acnt.quantity -= quantity;
    });

    // 如果余额为0，则删除这条记录
    if ( itr->quantity.amount == 0 ) {
        _accounts.erase(itr);
    }

    action(
        permission_level{ _self, "active"_n },
        "eosio.token"_n, "transfer"_n,
        std::make_tuple(_self, to, quantity, string("albumoftimes withdraw"))
    ).send();
}

// 创建公共相册
ACTION albumoftimes::makepubalbum(const string& name_cn, const string& name_en)
{
    require_auth( _self );
    eosio::check( name_cn.length() <= NAME_MAX_LEN, "name_cn is too long" );
    eosio::check( name_en.length() <= NAME_MAX_LEN, "name_en is too long" );

    _pub_albums.emplace(_self, [&](auto& pub_album){
        pub_album.owner                     = _self;
        pub_album.pub_album_id              = _pub_albums.available_primary_key();
        pub_album.name_cn                   = name_cn;
        pub_album.name_en                   = name_en;
        pub_album.cover_thumb_pic_ipfs_hash = ALBUM_DEFAULT_COVER_PIC_IPFS_HASH;
        pub_album.create_time               = current_time_point().sec_since_epoch();
        pub_album.pic_num                   = 0;
    });
}

// 修改公共相册的名字
ACTION albumoftimes::rnpubalbum(const uint64_t& pub_album_id, const string& new_name_cn, const string& new_name_en)
{
    require_auth( _self );
    eosio::check( new_name_cn.length() <= NAME_MAX_LEN, "new_name_cn is too long" );
    eosio::check( new_name_en.length() <= NAME_MAX_LEN, "new_name_en is too long" );

    auto itr_pub_album = _pub_albums.find( pub_album_id );
    eosio::check(itr_pub_album != _pub_albums.end(), "unknown pub_album_id");

    _pub_albums.modify( itr_pub_album, _self, [&]( auto& pub_album ) {
        pub_album.name_cn = new_name_cn;
        pub_album.name_en = new_name_en;
    });
}

// 创建个人相册
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
    
    // 如果余额为0，则删除这条记录
    if ( itr->quantity.amount == 0 ) {
        _accounts.erase(itr);
    }

    _albums.emplace(_self, [&](auto& album){
        album.owner                     = owner;
        album.album_id                  = _albums.available_primary_key();
        album.name                      = name;
        album.album_pay                 = PAY_FOR_ALBUM;
        album.cover_thumb_pic_ipfs_hash = ALBUM_DEFAULT_COVER_PIC_IPFS_HASH;
        album.create_time               = current_time_point().sec_since_epoch();
        album.pic_num                   = 0;
    });
}

// 修改个人相册的名字
ACTION albumoftimes::renamealbum(const name& owner, const uint64_t& album_id, const string& new_name)
{
    require_auth( owner );
    eosio::check( new_name.length() <= NAME_MAX_LEN, "new_name is too long" );

    auto itr_album = _albums.find( album_id );
    eosio::check(itr_album != _albums.end(), "unknown album id");
    eosio::check(itr_album->owner == owner, "this album is not belong to this owner");

    _albums.modify( itr_album, _self, [&]( auto& album ) {
        album.name = new_name;
    });
}

// 上传图片
ACTION albumoftimes::uploadpic(const name& owner, const uint64_t& album_id, const string& name, const string& detail,
                               const string& sha256_sum, const string& ipfs_hash, const string& thumb_ipfs_hash)
{
    require_auth( owner );
    eosio::check( name.length()            <= NAME_MAX_LEN,   "name is too long" );
    eosio::check( detail.length()          <= DETAIL_MAX_LEN, "detail is too long" );
    eosio::check( sha256_sum.length()      == SHA256_SUM_LEN, "wrong sha256_sum" );
    eosio::check( ipfs_hash.length()       == IPFS_HASH_LEN,  "wrong ipfs_hash" );
    eosio::check( thumb_ipfs_hash.length() == IPFS_HASH_LEN,  "wrong thumb_ipfs_hash" );

    auto itr_album = _albums.find( album_id );
    eosio::check(itr_album != _albums.end(), "unknown album_id");
    eosio::check(itr_album->owner == owner, "this album is not belong to this owner");

    auto itr_acnt = _accounts.find( owner.value );
    eosio::check(itr_acnt != _accounts.end(), "unknown account");

    _accounts.modify( itr_acnt, _self, [&]( auto& acnt ) {
        eosio::check( acnt.quantity >= PAY_FOR_PIC, "insufficient balance" );
        acnt.quantity -= PAY_FOR_PIC;
    });

    // 如果余额为0，则删除这条记录
    if ( itr_acnt->quantity.amount == 0 ) {
        _accounts.erase(itr_acnt);
    }

    _pics.emplace(_self, [&](auto& pic){
        pic.owner                    = owner;
        pic.album_id                 = album_id;
        pic.pic_id                   = _pics.available_primary_key();
        pic.name                     = name;
        pic.detail                   = detail;
        pic.sha256_sum               = sha256_sum;
        pic.ipfs_hash                = ipfs_hash;
        pic.thumb_ipfs_hash          = thumb_ipfs_hash;
        pic.pic_pay                  = PAY_FOR_PIC;
        pic.public_album_id          = 0;
        pic.sort_fee                 = ZERO_FEE;
        pic.upvote_num               = 0;
        pic.upload_time              = current_time_point().sec_since_epoch();
    });

    add_private_album_pic_num(album_id);
}

// 修改图片的名字和描述
ACTION albumoftimes::modifypicnd(const name& owner, const uint64_t& pic_id, const string& new_name, const string& new_detail)
{
    require_auth( owner );
    eosio::check( new_name.length()     <= NAME_MAX_LEN,   "new_name is too long" );
    eosio::check( new_detail.length()   <= DETAIL_MAX_LEN, "new_detail is too long" );

    auto itr_pic = _pics.find( pic_id );
    eosio::check(itr_pic != _pics.end(), "unknown pic id");
    eosio::check(itr_pic->owner == owner, "this pic is not belong to this owner");

    _pics.modify( itr_pic, _self, [&]( auto& pic ) {
        pic.name   = new_name;
        pic.detail = new_detail;
    });
}

// 设置个人相册的封面图片
ACTION albumoftimes::setcover(const name& owner, const uint64_t& album_id, const string& cover_thumb_pic_ipfs_hash)
{
    require_auth( owner );
    eosio::check( cover_thumb_pic_ipfs_hash.length() == IPFS_HASH_LEN, "wrong cover_thumb_pic_ipfs_hash" );

    auto itr_album = _albums.find( album_id );
    eosio::check(itr_album != _albums.end(), "unknown album_id");
    eosio::check(itr_album->owner == owner, "this album is not belong to this owner");

    _albums.modify( itr_album, _self, [&]( auto& album ) {
        album.cover_thumb_pic_ipfs_hash = cover_thumb_pic_ipfs_hash;      //////  ~~~程序猿同学可以自由设置封面~~~
    });
}

// 将图片移动到另一个相册（图片可以在个人相册之间移动）
ACTION albumoftimes::movetoalbum(const name& owner, const uint64_t& pic_id, const uint64_t& dst_album_id)
{
    require_auth( owner );

    auto itr_pic = _pics.find( pic_id );
    eosio::check(itr_pic != _pics.end(), "unknown pic id");
    eosio::check(itr_pic->owner == owner, "this pic is not belong to this owner");

    auto itr_dst_album = _albums.find( dst_album_id );
    eosio::check(itr_dst_album != _albums.end(), "unknown dst album id");
    eosio::check(itr_dst_album->owner == owner, "dst album is not belong to this owner");

    uint64_t old_album_id = itr_pic->album_id;
    if ( old_album_id == dst_album_id ) {
        return;
    }

    _pics.modify( itr_pic, _self, [&]( auto& pic ) {
        pic.album_id = dst_album_id;
    });

    sub_private_album_pic_num(old_album_id);
    add_private_album_pic_num(dst_album_id);

    // 如果这个图片是它以前所属相册的封面，则需要更新它以前所属相册的封面为系统默认封面。
    auto itr_old_album = _albums.find( old_album_id );
    eosio::check(itr_old_album != _albums.end(), "unknown old album id");
    eosio::check(itr_old_album->owner == owner, "old album is not belong to this owner");

    if ( itr_old_album->cover_thumb_pic_ipfs_hash == itr_pic->thumb_ipfs_hash ) {
        set_private_album_default_cover(old_album_id);
    }
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
    
    add_public_album_pic_num(pub_album_id);
    if ( old_pub_album_id > 0 ) {
        sub_public_album_pic_num(old_pub_album_id);
    }

    // 如果这个图片以前曾经支付过在公共相册中的排序费用，则它可能成为新的所属公共相册的封面。
    if ( itr_pic->sort_fee.amount > 0 ) {
        update_public_album_cover(pub_album_id);
    }

    // 这个图片可能是它以前所属公共相册的封面，更新它以前所属公共相册的封面。
    if ( old_pub_album_id > 0 && itr_pic->sort_fee.amount > 0 ) {
        update_public_album_cover(old_pub_album_id);
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

    sub_public_album_pic_num(old_pub_album_id);

    // 这个图片可能是它以前所属公共相册的封面，更新它以前所属公共相册的封面。
    if ( itr_pic->sort_fee.amount > 0 ) {
        update_public_album_cover(old_pub_album_id);
    }
}

// 为公共相册中的图片支付排序靠前的费用
ACTION albumoftimes::paysortfee(const name& owner, const uint64_t& pic_id, const asset& sortfee)
{
    require_auth( owner );

    auto itr_pic = _pics.find( pic_id );
    eosio::check(itr_pic != _pics.end(), "unknown pic id");
    eosio::check(itr_pic->owner == owner, "this pic is not belong to this owner");

    //if ( !sortfee.is_valid() || !sortfee.is_amount_within_range() ) {
    //    return;
    //}
    //if ( !sortfee.symbol.is_valid() || sortfee.symbol != MAIN_SYMBOL ) {
    //    return;
    //}
    //if (sortfee.amount <= 0) {
    //    return;
    //}
    eosio::check(sortfee.symbol.is_valid(),        "invalid quantity inputed");
    eosio::check(sortfee.symbol == MAIN_SYMBOL,    "invalid quantity inputed");
    eosio::check(sortfee.is_valid(),               "invalid quantity inputed");
    eosio::check(sortfee.is_amount_within_range(), "invalid quantity inputed");
    eosio::check(sortfee.amount > 0,               "invalid quantity inputed");

    auto itr_acnt = _accounts.find( owner.value );
    eosio::check(itr_acnt != _accounts.end(), "unknown account");

    _accounts.modify( itr_acnt, _self, [&]( auto& acnt ) {
        eosio::check( acnt.quantity >= sortfee, "insufficient balance" );
        acnt.quantity -= sortfee;
    });

    // 如果余额为0，则删除这条记录
    if ( itr_acnt->quantity.amount == 0 ) {
        _accounts.erase(itr_acnt);
    }

    _pics.modify( itr_pic, _self, [&]( auto& pic ) {
        pic.sort_fee += sortfee;
    });

    // 如果这个图片在一个公共相册里面，则可能成为这个公共相册的封面。
    if ( itr_pic->public_album_id > 0 ) {
        update_public_album_cover(itr_pic->public_album_id);
    }
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

// 删除图片（只删除EOS中的数据，IPFS中的图片依然存在）
ACTION albumoftimes::deletepic(const name& owner, const uint64_t& pic_id)
{
    require_auth( owner );

    auto itr_pic = _pics.find( pic_id );
    eosio::check(itr_pic != _pics.end(), "unknown pic id");
    eosio::check(itr_pic->owner == owner, "this pic is not belong to this owner");

    uint64_t private_album_id = itr_pic->album_id;
    uint64_t public_album_id  = itr_pic->public_album_id;

    // 如果这个图片是所属个人相册的封面，则需要将这个相册的封面改回系统默认封面。
    auto itr_private_album = _albums.find( private_album_id );
    eosio::check(itr_private_album != _albums.end(), "unknown private album id");
    eosio::check(itr_private_album->owner == owner, "private album is not belong to this owner");

    if ( itr_private_album->cover_thumb_pic_ipfs_hash == itr_pic->thumb_ipfs_hash ) {
        set_private_album_default_cover(private_album_id);
    }

    asset sortfee = itr_pic->sort_fee;

    _pics.erase(itr_pic);

    sub_private_album_pic_num(private_album_id);
    if ( public_album_id > 0 ) {
        sub_public_album_pic_num(public_album_id);
    }

    // 这个图片可能是它所属公共相册的封面，更新它所属公共相册的封面。
    if ( public_album_id > 0 && sortfee.amount > 0 ) {
        update_public_album_cover(public_album_id);
    }
}

// 监管删除违规图片
ACTION albumoftimes::rmillegalpic(const uint64_t& pic_id)
{
    require_auth( REGULATOR_ACCOUNT_NAME );

    auto itr_pic = _pics.find( pic_id );
    eosio::check(itr_pic != _pics.end(), "unknown pic id");

    // 检查是否超出可以删除的时间期限
    eosio::check( current_time_point().sec_since_epoch() - itr_pic->upload_time <= REGULATOR_DELETE_TIME_LIMIT_SECS, "time limit exceeded" );

    uint64_t private_album_id = itr_pic->album_id;
    uint64_t public_album_id  = itr_pic->public_album_id;

    // 如果这个图片是所属个人相册的封面，则需要将这个相册的封面改回系统默认封面。
    auto itr_private_album = _albums.find( private_album_id );
    eosio::check(itr_private_album != _albums.end(), "unknown private album id");

    if ( itr_private_album->cover_thumb_pic_ipfs_hash == itr_pic->thumb_ipfs_hash ) {
        set_private_album_default_cover(private_album_id);
    }

    asset sortfee = itr_pic->sort_fee;

    _pics.erase(itr_pic);

    sub_private_album_pic_num(private_album_id);
    if ( public_album_id > 0 ) {
        sub_public_album_pic_num(public_album_id);
    }

    // 这个图片可能是它所属公共相册的封面，更新它所属公共相册的封面。
    if ( public_album_id > 0 && sortfee.amount > 0 ) {
        update_public_album_cover(public_album_id);
    }
}

// 删除个人相册，如果相册中有图片，则不能删除，只能删除空相册
ACTION albumoftimes::deletealbum(const name& owner, const uint64_t& album_id)
{
    require_auth( owner );

    auto itr_album = _albums.find( album_id );
    eosio::check(itr_album != _albums.end(), "unknown album id");
    eosio::check(itr_album->owner == owner, "this album is not belong to this owner");

    // 检查相册中是否有图片（可以直接根据相册中的图片数量进行判断，下面的代码是在给相册增加图片数量属性以前写的，就保留这样吧，不改了。）
    auto album_index = _pics.get_index<name("byalbumid")>();
    auto itr = album_index.lower_bound(album_id);
    bool has_pic_in_album = false;
    if (itr != album_index.end() && itr->album_id == album_id) {
        has_pic_in_album = true;
    }
    eosio::check(has_pic_in_album == false, "album is not empty");

    _albums.erase(itr_album);
}

// 将个人相册的图片数量+1
void albumoftimes::add_private_album_pic_num(const uint64_t& private_album_id)
{
    auto itr_private_album = _albums.find( private_album_id );
    eosio::check(itr_private_album != _albums.end(), "unknown private album id");

    _albums.modify( itr_private_album, _self, [&]( auto& private_album ) {
        private_album.pic_num += 1;
    });
}

// 将个人相册的图片数量-1
void albumoftimes::sub_private_album_pic_num(const uint64_t& private_album_id)
{
    auto itr_private_album = _albums.find( private_album_id );
    eosio::check(itr_private_album != _albums.end(), "unknown private album id");

    _albums.modify( itr_private_album, _self, [&]( auto& private_album ) {
        private_album.pic_num -= 1;
    });
}

// 将公共相册的图片数量+1
void albumoftimes::add_public_album_pic_num(const uint64_t& public_album_id)
{
    auto itr_public_album = _pub_albums.find( public_album_id );
    eosio::check(itr_public_album != _pub_albums.end(), "unknown public album id");

    _pub_albums.modify( itr_public_album, _self, [&]( auto& public_album ) {
        public_album.pic_num += 1;
    });
}

// 将公共相册的图片数量-1
void albumoftimes::sub_public_album_pic_num(const uint64_t& public_album_id)
{
    auto itr_public_album = _pub_albums.find( public_album_id );
    eosio::check(itr_public_album != _pub_albums.end(), "unknown public album id");

    _pub_albums.modify( itr_public_album, _self, [&]( auto& public_album ) {
        public_album.pic_num -= 1;
    });
}

// 将个人相册的封面设置为系统默认封面
void albumoftimes::set_private_album_default_cover(const uint64_t& private_album_id)
{
    auto itr_private_album = _albums.find( private_album_id );
    eosio::check(itr_private_album != _albums.end(), "unknown private album id");

    _albums.modify( itr_private_album, _self, [&]( auto& private_album ) {
        private_album.cover_thumb_pic_ipfs_hash = ALBUM_DEFAULT_COVER_PIC_IPFS_HASH;
    });
}

// 公共相册重新选择封面，将排序最前并且支付过排序费用的图片设置为封面，如果没有这样的图片，则设置为系统默认封面
void albumoftimes::update_public_album_cover(const uint64_t& public_album_id)
{
    auto itr_public_album = _pub_albums.find( public_album_id );
    eosio::check(itr_public_album != _pub_albums.end(), "unknown public album id");

    uint64_t ui64 = public_album_id;
    auto pub_album_fee_index = _pics.get_index<name("bypubsortfee")>();
    auto itr = pub_album_fee_index.lower_bound(ui64<<32);
    if (itr != pub_album_fee_index.end() && itr->public_album_id == public_album_id && itr->sort_fee.amount > 0 ) {
        _pub_albums.modify( itr_public_album, _self, [&]( auto& public_album ) {
            public_album.cover_thumb_pic_ipfs_hash = itr->thumb_ipfs_hash;
        });
    } else {
        _pub_albums.modify( itr_public_album, _self, [&]( auto& public_album ) {
            public_album.cover_thumb_pic_ipfs_hash = ALBUM_DEFAULT_COVER_PIC_IPFS_HASH;
        });
    }
}

// 系统初始化时使用：删掉 pub_album_id = 0 的公共相册
ACTION albumoftimes::rmpubalbumfr()
{
    require_auth( _self );

    auto itr_pub_album = _pub_albums.find( 0 );
    eosio::check(itr_pub_album != _pub_albums.end(), "there is no record of pub_album_id = 0");

    _pub_albums.erase(itr_pub_album);
}

// 系统初始化时使用：删掉     album_id = 0 的个人相册
ACTION albumoftimes::rmprialbumfr()
{
    require_auth( _self );

    auto itr_album = _albums.find( 0 );
    eosio::check(itr_album != _albums.end(), "there is no record of album_id = 0");

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
    for (auto& item : _pub_albums) {
        keysForDeletion.push_back(item.pub_album_id);
    }
    for (uint64_t key : keysForDeletion) {
        auto itr = _pub_albums.find(key);
        if (itr != _pub_albums.end()) {
            _pub_albums.erase(itr);
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
