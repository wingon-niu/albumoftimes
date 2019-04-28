
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
      /* �˴���ԭ���� EOSIO_DISPATCH �����������޸ģ��Է��յ��ٱҡ� */ \
      if( (code == receiver && action != name("transfer").value) || (code == name("eosio.token").value && action == name("transfer").value) ) { \
         switch( action ) { \
            EOSIO_DISPATCH_HELPER( TYPE, MEMBERS ) \
         } \
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
      } \
   } \
} \

EOSIO_DISPATCH_EX(albumoftimes, (transfer)(withdraw)(makepubalbum)(rnpubalbum)(createalbum)(renamealbum)(uploadpic)(modifypicnd)(setcover)(movetoalbum)(joinpubalbum)(outpubalbum)(paysortfee)(upvotepic)(deletepic)(rmillegalpic)(deletealbum)(rmpubalbumfr)(rmprialbumfr)(clearalldata))
