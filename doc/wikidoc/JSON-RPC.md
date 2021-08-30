### command line format:
```
Usage:
        ibrio-cli (OPTIONS) COMMAND

Run ibrio RPC client

Options:
  -help                                 Get more information
  -daemon                               Run server in background
  -debug                                Run in debug mode
  -datadir=<path>                       Root directory of resources
  -conf=<file>                          Configuration file name
  -testnet                              Use the test network
  -nowallet                             Launch server without wallet
  -version                              Get ibrio version
  -purge                                Purge database and blockfile
  -checkrepair                          Check and repair database
  -onlycheck                            Only check database and blockfile
  -blocknotify                          Execute command when the best block changes (%s in cmd is replaced by block hash)
  -logfilesize=<size>                   Log file size(M) (default: 200M)
  -loghistorysize=<size>                Log history size(M) (default: 2048M)
  -addrtxindex                          Launch server without address txindex
  -rpcport=port                         Listen for JSON-RPC connections on <port> (default: 6602 or testnet: 6604))
  -rpclisten                            Accept RPC IPv4 and IPv6 connections (default: 0)
  -rpclisten4                           Accept RPC IPv4 connections (default: 0)
  -rpclisten6                           Accept RPC IPv6 connections (default: 0)
  -rpcuser=<user>                       <user> name for JSON-RPC connections
  -rpcpassword=<password>               <password> for JSON-RPC connections
  -rpcssl                               Use OpenSSL (https) for JSON-RPC connections or not (default false)
  -norpcsslverify                       Verify SSL or not (default yes)
  -rpccafile=<file.crt>                 SSL CA file name (default ca.crt)
  -rpccertfile=<file.crt>               Server certificate file (default: server.crt)
  -rpcpkfile=<file.pem>                 Server private key (default: server.pem)
  -rpcciphers=<ciphers>                 Acceptable ciphers (default: TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!AH:!3DES:@STRENGTH)
  -statdata                             Enable statistical data or not (default false)
  -rpclog                               Enable write RPC log (default true)
  -rpchost=<ip>                         Send commands to node running on <ip> (default: 127.0.0.1)
  -rpctimeout=<time>                    Connection timeout <time> seconds (default: 120)
```

## commands
### System
 - [help](#help): List commands, or get help for a command.
 - [stop](#stop): Stop ibrio server.
 - [version](#version): Get ibrio server version.
### Network
 - [getpeercount](#getpeercount): Return the number of connections to other nodes.
 - [listpeer](#listpeer): Return data about each connected network node.
 - [addnode](#addnode): Attempt to add a node into the addnode list.
 - [removenode](#removenode): Attempt to remove a node from the addnode list.
### Blockchain & TxPool
 - [getforkcount](#getforkcount): Return the number of forks.
 - [listfork](#listfork): If true, list of all forks, or subscribed forks
 - [getgenealogy](#getgenealogy): Return the list of ancestry and subline.
 - [getblocklocation](#getblocklocation): Return the location with given block.
 - [getblockcount](#getblockcount): Return the number of blocks in the given fork, includes extended block and vacant block
 - [getblockhash](#getblockhash): Return a list of block hash in the fork at a specific height.
 - [getblock](#getblock): Return details of a block with given block-hash.
 - [getblockdetail](#getblockdetail): Return details of a block with given block-hash.
 - [gettxpool](#gettxpool): Get transaction pool information
 - [gettransaction](#gettransaction): Get transaction information
 - [sendtransaction](#sendtransaction): Submit raw transaction (serialized, hex-encoded) to local node and network.
 - [getforkheight](#getforkheight): Return the number of height in the given fork.
 - [getvotes](#getvotes): Get votes
 - [listdelegate](#listdelegate): List delegate
### Wallet
 - [listkey](#listkey): Return Object that has pubkey as keys, associated status as values.
 - [getnewkey](#getnewkey): Return a new pubkey for receiving payments.
 - [removekey](#removekey): Remove Key in Wallet.
 - [encryptkey](#encryptkey): Changes the passphrase for <oldpassphrase> to <passphrase>
 - [lockkey](#lockkey): Removes the encryption key from memory, locking the key.
 - [unlockkey](#unlockkey): Unlock the key.
 - [importprivkey](#importprivkey): Add a private key to your wallet.
 - [importpubkey](#importpubkey): Add a public key to your wallet.
 - [importkey](#importkey): Reveal the serialized key corresponding to <pubkey>.
 - [exportkey](#exportkey): Reveal the serialized key corresponding to <pubkey>.
 - [addnewtemplate](#addnewtemplate): Return encoded address for the given template id.
 - [importtemplate](#importtemplate): Return encoded address for the given template.
 - [exporttemplate](#exporttemplate): Return encoded address for the given template.
 - [removetemplate](#removetemplate): Remove template in Wallet.
 - [validateaddress](#validateaddress): Return information about <address>.
 - [getdefirelation](#getdefirelation): Get parent address of specified address
 - [getbalance](#getbalance): Get balance of an address.
 - [listtransaction](#listtransaction): Return transactions list.
 - [sendfrom](#sendfrom): Send a transaction.
 - [createtransaction](#createtransaction): Create a transaction.
 - [signtransaction](#signtransaction): Sign a transaction.
 - [signmessage](#signmessage): Sign a message with the private key of an pubkey
 - [listaddress](#listaddress): List all of the addresses from pub keys and template ids
 - [exportwallet](#exportwallet): Export all of keys and templates from wallet to a specified file in JSON format.
 - [importwallet](#importwallet): Import keys and templates from an archived file to the wallet in JSON format.
 - [makeorigin](#makeorigin): Return hex-encoded block.
### Util
 - [verifymessage](#verifymessage): Verify a signed message
 - [makekeypair](#makekeypair): Make a public/private key pair.
 - [getpubkey](#getpubkey): Return public key by address or private key.
 - [getpubkeyaddress](#getpubkeyaddress): Return encoded address for the given public key.
 - [gettemplateaddress](#gettemplateaddress): Return encoded address for the given template id.
 - [maketemplate](#maketemplate): Return encoded address for the given template id.
 - [decodetransaction](#decodetransaction): Return a JSON object representing the serialized, hex-encoded transaction.
 - [gettxfee](#gettxfee): Return TxFee for vchData Hex data
 - [makesha256](#makesha256): Make sha256
 - [aesencrypt](#aesencrypt): aes encrypt
 - [aesdecrypt](#aesdecrypt): aes decrypt
 - [listunspent](#listunspent): Return a JSON object listing unspent utxo by user specifying arguments address.
### Mint
 - [getwork](#getwork): Get mint work
 - [submitwork](#submitwork): Submit mint work
 - [querystat](#querystat): Query statistical data
 - [signrawtransactionwithwallet](#signrawtransactionwithwallet): Sign a transaction offline.
 - [sendrawtransaction](#sendrawtransaction): Submit transaction raw data(serialized, hex-encoded) with offline signature using pubkey or template address.
 - [reversehex](#reversehex): Reverse a hex string by byte
 - [activatesign](#activatesign): Activate sign
 - [getactivatestatus](#getactivatestatus): Get activation status of address
---

### help
**Usage:**
```
        help ("command")

List commands, or get help for a command.
```
**Arguments:**
```
 "command"                              (string, optional) command name
```
**Request:**
```
 "param" :
 {
   "command": ""                        (string, optional) command name
 }
```
**Response:**
```
 "result": "help"                       (string, required) help info
```
**Examples:**
```
>> ibrio-cli help

>> ibrio-cli help getpeercount
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### stop
**Usage:**
```
        stop

Stop ibrio server.
```
**Arguments:**
```
	none
```
**Request:**
```
 "param" : {}
```
**Response:**
```
 "result": "result"                     (string, required) stop result
```
**Examples:**
```
>> ibrio-cli stop
<< ibrio server stopping

>> curl -d '{"id":1,"method":"stop","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":1,"jsonrpc":"2.0","result":"ibrio server stopping"}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### version
**Usage:**
```
        version

Get ibrio server version.
```
**Arguments:**
```
	none
```
**Request:**
```
 "param" : {}
```
**Response:**
```
 "result": "version"                    (string, required) ibrio version
```
**Examples:**
```
>> ibrio-cli version
<< Ibrio server version is v0.1.0

>> curl -d '{"id":0,"method":"version","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":0,"jsonrpc":"2.0","result":"Ibrio server version is v0.1.0"}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### getpeercount
**Usage:**
```
        getpeercount

Return the number of connections to other nodes.
```
**Arguments:**
```
	none
```
**Request:**
```
 "param" : {}
```
**Response:**
```
 "result": count                        (int, required) peer count
```
**Examples:**
```
>> ibrio-cli getpeercount
<< 0

>> curl -d '{"id":3,"method":"getpeercount","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":3,"jsonrpc":"2.0","result":0}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### listpeer
**Usage:**
```
        listpeer

Return data about each connected network node.
```
**Arguments:**
```
	none
```
**Request:**
```
 "param" : {}
```
**Response:**
```
 "result" :
   "peer":                              (array, required, default=RPCValid) 
   [
     {
       "address": "",                   (string, required) peer address
       "services": "",                  (string, required) service
       "lastsend": "",                  (string, required) last send time(utc)
       "lastrecv": "",                  (string, required) last receive time(utc)
       "conntime": "",                  (string, required) active time(utc)
       "pingtime": 0,                   (int, required) ping pong time
       "version": "",                   (string, required) version
       "subver": "",                    (string, required) sub version
       "inbound": true|false,           (bool, required) accept multiple connection or not
       "height": 0,                     (int, required) starting height
       "banscore": 0                    (int, required) ban score
     }
   ]
```
**Examples:**
```
>> ibrio-cli listpeer
<< [{"address":"113.105.146.22","services":"NODE_NETWORK,NODE_DELEGATED","lastsend":"2019-10-25 03:11:09","lastrecv":"2019-10-25 03:11:09","conntime":"2019-10-25 03:05:23","version":"0.1.0","subver":"/Ibrio:0.9.2/Protocol:0.1.0/9be9a865898c5cea90c716c17603cf3f0f185a5b","inbound":false,"height":31028,"banscore":true}]

>> curl -d '{"id":40,"method":"listpeer","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":40,"jsonrpc":"2.0","result":[{"address":"113.105.146.22","services":"NODE_NETWORK,NODE_DELEGATED","lastsend":"2019-10-25 03:11:09","lastrecv":"2019-10-25 03:11:09","conntime":"2019-10-25 03:05:23","version":"0.1.0","subver":"/Ibrio:0.9.2/Protocol:0.1.0/9be9a865898c5cea90c716c17603cf3f0f185a5b","inbound":false,"height":31028,"banscore":true}]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### addnode
**Usage:**
```
        addnode <"node">

Attempt to add a node into the addnode list.
```
**Arguments:**
```
 "node"                                 (string, required) node host:port
```
**Request:**
```
 "param" :
 {
   "node": ""                           (string, required) node host:port
 }
```
**Response:**
```
 "result": "result"                     (string, required) add node result
```
**Examples:**
```
>> ibrio-cli addnode 113.105.146.22
<< Add node successfully: 113.105.146.22

>> curl -d '{"id":3,"method":"addnode","jsonrpc":"2.0","params":{"node":"113.105.146.22:6601"}}' http://127.0.0.1:6602
<< {"id":3,"jsonrpc":"2.0","result":"Add node successfully: 113.105.146.22:6601"}
```
**Errors:**
```
* {"code":-206,"message":"Failed to add node."}
```
##### [Back to top](#commands)
---
### removenode
**Usage:**
```
        removenode <"node">

Attempt to remove a node from the addnode list.
```
**Arguments:**
```
 "node"                                 (string, required) node host:port
```
**Request:**
```
 "param" :
 {
   "node": ""                           (string, required) node host:port
 }
```
**Response:**
```
 "result": "result"                     (string, required) remove node result
```
**Examples:**
```
>> ibrio-cli removenode 113.105.146.22
<< Remove node successfully: 113.105.146.22

>> curl -d '{"id":67,"method":"removenode","jsonrpc":"2.0","params":{"node":"113.105.146.22:6601"}}' http://127.0.0.1:6602
<< {"id":67,"jsonrpc":"2.0","result":"Remove node successfully: 113.105.146.22:6601"}
```
**Errors:**
```
* {"code":-206,"message":"Failed to remove node."}
```
##### [Back to top](#commands)
---
### getforkcount
**Usage:**
```
        getforkcount

Return the number of forks.
```
**Arguments:**
```
	none
```
**Request:**
```
 "param" : {}
```
**Response:**
```
 "result": count                        (int, required) fork count
```
**Examples:**
```
>> ibrio-cli getforkcount
<< 1

>> curl -d '{"id":69,"method":"getforkcount","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":69,"jsonrpc":"2.0","result":1}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### listfork
**Usage:**
```
        listfork (-a|-noa*all*)

If true, list of all forks, or subscribed forks
```
**Arguments:**
```
 -a|-noa*all*                           (bool, optional, default=false) list all forks or not
```
**Request:**
```
 "param" :
 {
   "all": true|false                    (bool, optional, default=false) list all forks or not
 }
```
**Response:**
```
 "result" :
   "profile":                           (array, required, default=RPCValid) fork profile list
   [
     {
       "fork": "",                      (string, required) fork id with hex system
       "name": "",                      (string, required) fork name
       "symbol": "",                    (string, required) fork symbol
       "amount": 0.0,                   (double, required) amount
       "reward": 0.0,                   (double, required) mint reward
       "halvecycle": 0,                 (uint, required) halve cycle: 0: fixed reward, >0: blocks of halve cycle
       "isolated": true|false,          (bool, required) is isolated
       "private": true|false,           (bool, required) is private
       "enclosed": true|false,          (bool, required) is enclosed
       "owner": "",                     (string, required) owner's address
       "createtxid": "",                (string, required) create fork txid
       "createforkheight": 0,           (int, required) create fork height
       "parentfork": "",                (string, required) parent fork
       "forktype": "",                  (string, required) fork params, a json object
       "forkheight": 0,                 (int, required) fork height
       "lastblock": "",                 (string, required) last block hash
       "moneysupply": 0.0,              (double, required) money supply
       "moneydestroy": 0.0,             (double, required) money destroy
       (if forktype=defi)
       "defi":                          (object, required) Fork DeFi Parameters
       {
         "mintheight": 0,               (int, required) beginning mint height of DeFi, 0 means the first block after origin, -1 means not confirmed mint height(mint height depends on confirm tx of owner)
         "maxsupply": 0.0,              (double, required) the max DeFi supply in this fork
         "coinbasetype": 0,             (uint, required) coinbase type. 0 - fixed decay(related to 'nInitCoinbasePercent', 'nCoinbaseDecayPercent', 'nDecayCycle'). 1 - specific decay(related to 'mapCoinbasePercent')
         "decaycycle": 0,               (int, required) coinbase decay cycle in height
         "mapcoinbasepercent":          (array, required, default=RPCValid) pairs of height - coinbase percent
         [
           {
             "height": 0,               (int, required) begin height
             "percent": 0               (uint, required) coinbase increasing ratio [0 - 100]
           }
         ]
         "coinbasedecaypercent": 0,     (uint, required) compared with previous decay cycle, coinbase increasing ratio(%), 50 means decay 50%
         "initcoinbasepercent": 0,      (uint, required) coinbase increasing ratio(%) per supply cycle in initialization
         "rewardcycle": 0,              (int, required) generate reward cycle in height
         "supplycycle": 0,              (int, required) supplyment changing cycle in height
         "stakerewardpercent": 0,       (uint, required) stake reward ratio, [0 - 100] means [0% - 100%]
         "promotionrewardpercent": 0,   (uint, required) promotion reward ratio, [0 - 100] means [0% - 100%]
         "stakemintoken": 0.0,          (double, required) the minimum token on address can participate stake reward
         "mappromotiontokentimes":      (array, required, default=RPCValid) in promotion computation, less than or equal to [key] amount should multiply [value] eg. [{"key": 10, "value": 5},...]
         [
           {
             "token": 0,                (uint, required) promotion token
             "times": 0                 (uint, required) promotion times
           }
         ]
       }
     }
   ]
```
**Examples:**
```
>> ibrio-cli listfork
<< [{"fork":"00000023e3f9ba506f2ee2f96869f54eb4f23f3141471d57cad55b78ace699ff","name":"Ibrio Network","symbol":"BIG","amount":100000000.000000,"reward":15,"halvecycle":0,"isolated":true,"private":false,"enclosed":false,"owner":"1mjw7aa0s7v9sv7x3thvcexxzjz4tq82j5qc12dy29ktqy84haa0j7dwb","createtxid":"5fcef9b220f234b1b1356d545877e55551728bae8484e4a68d425d9242c6860c","forkheight":34,"parentfork":"0000000006854ebdc236f48dbbe5c87312ea0abd7398888374b5ee9a5eb1d291","forktype":"common","forkheight":98,"lastblock":"000000622f660db7b61aa77912070b4d1dade697f7cf98bcddb6c26036d2cb1d","moneysupply":80000.000000,"moneydestroy":20.000000}]

>> {"id":69,"method":"listfork","jsonrpc":"2.0","params":{}}
<< {"id":69,"jsonrpc":"2.0","result":[{"fork":"00000023e3f9ba506f2ee2f96869f54eb4f23f3141471d57cad55b78ace699ff","name":"Ibrio Network","symbol":"BIG","amount":100000000.000000,"reward":15,"halvecycle":0,"isolated":true,"private":false,"enclosed":false,"owner":"1mjw7aa0s7v9sv7x3thvcexxzjz4tq82j5qc12dy29ktqy84haa0j7dwb","createtxid":"5fcef9b220f234b1b1356d545877e55551728bae8484e4a68d425d9242c6860c","forkheight":34,"parentfork":"0000000006854ebdc236f48dbbe5c87312ea0abd7398888374b5ee9a5eb1d291","forktype":"common","forkheight":98,"lastblock":"000000622f660db7b61aa77912070b4d1dade697f7cf98bcddb6c26036d2cb1d","moneysupply":80000.000000,"moneydestroy":20.000000}]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### getgenealogy
**Usage:**
```
        getgenealogy (-f="fork")

Return the list of ancestry and subline.
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork hash
```
**Request:**
```
 "param" :
 {
   "fork": ""                           (string, optional) fork hash
 }
```
**Response:**
```
 "result" :
 {
   "ancestry":                          (array, required, default=RPCValid) ancestry info
   [
     {
       "parent": "",                    (string, required) parent fork hash
       "height": 0                      (int, required) parent origin height
     }
   ]
   "subline":                           (array, required, default=RPCValid) subline info
   [
     {
       "sub": "",                       (string, required) sub fork hash
       "height": 0                      (int, required) sub origin height
     }
   ]
 }
```
**Examples:**
```
>> ibrio-cli getgenealogy
<< {"ancestry":[],"subline":[]}

>> curl -d '{"id":75,"method":"getgenealogy","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":75,"jsonrpc":"2.0","result":{"ancestry":[],"subline":[]}}

>> ibrio-cli getgenealogy 1
<< {"code":-6,"message":"Unknown fork"}

>> curl -d '{"id":1,"method":"getgenealogy","jsonrpc":"2.0","params":{"fork":"1"}}' http://127.0.0.1:6602
<< {"id":1,"jsonrpc":"2.0","error":{"code":-6,"message":"Unknown fork"}}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
```
##### [Back to top](#commands)
---
### getblocklocation
**Usage:**
```
        getblocklocation <"block">

Return the location with given block.
```
**Arguments:**
```
 "block"                                (string, required) block hash
```
**Request:**
```
 "param" :
 {
   "block": ""                          (string, required) block hash
 }
```
**Response:**
```
 "result" :
 {
   "fork": "",                          (string, required) fork hash
   "height": 0                          (int, required) block height
 }
```
**Examples:**
```
>> ibrio-cli getblocklocation 609a797ca28042d562b11355038c516d65ba30b91c7033d83c61b81aa8c538e3
<< {"fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","height":1}

>> curl -d '{"id":6,"method":"getblocklocation","jsonrpc":"2.0","params":{"block":"609a797ca28042d562b11355038c516d65ba30b91c7033d83c61b81aa8c538e3"}}' http://127.0.0.1:6602
<< {"id":6,"jsonrpc":"2.0","result":{"fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","height":1}}
```
**Errors:**
```
* {"code":-6,"message":"Unknown block."}
```
##### [Back to top](#commands)
---
### getblockcount
**Usage:**
```
        getblockcount (-f="fork")

Return the number of blocks in the given fork, includes extended block and vacant block
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork hash
```
**Request:**
```
 "param" :
 {
   "fork": ""                           (string, optional) fork hash
 }
```
**Response:**
```
 "result": count                        (int, required) block count
```
**Examples:**
```
>> ibrio-cli getblockcount
<< 32081

>> curl -d '{"id":4,"method":"getblockcount","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":4,"jsonrpc":"2.0","result":32081}

>> ibrio-cli getblockcount -f=a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0
<< 32081

>> curl -d '{"id":5,"method":"getblockcount","jsonrpc":"2.0","params":{"fork":"0"}}' http://127.0.0.1:6602
<< {"id":5,"jsonrpc":"2.0","result":32081}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
```
##### [Back to top](#commands)
---
### getblockhash
**Usage:**
```
        getblockhash <height> (-f="fork")

Return a list of block hash in fork at specific height.
First of the list is the main block hash, others are extended blocks hash in order.
```
**Arguments:**
```
 height                                 (int, required) block height
 -f="fork"                              (string, optional) fork hash
```
**Request:**
```
 "param" :
 {
   "height": 0,                         (int, required) block height
   "fork": ""                           (string, optional) fork hash
 }
```
**Response:**
```
 "result" :
   "hash":                              (array, required, default=RPCValid) 
   [
     "hash": ""                         (string, required) one block hash
   ]
```
**Examples:**
```
>> ibrio-cli getblockhash 0
<< ["a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0"]

>> curl -d '{"id":37,"method":"getblockhash","jsonrpc":"2.0","params":{"height":0}}' http://127.0.0.1:6602
<< {"id":37,"jsonrpc":"2.0","result":["a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0"]}

>> ibrio-cli getblockhash 0 -f=a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Block number out of range."}
```
##### [Back to top](#commands)
---
### getblock
**Usage:**
```
        getblock <"block">

Return details of a block with given block-hash.
```
**Arguments:**
```
 "block"                                (string, required) block hash
```
**Request:**
```
 "param" :
 {
   "block": ""                          (string, required) block hash
 }
```
**Response:**
```
 "result" :
 {
   "hash": "",                          (string, required) block hash
   "hashPrev": "",                      (string, required) block prev hash
   "version": 0,                        (uint, required) version
   "type": "",                          (string, required) block type
   "time": 0,                           (uint, required) block time
   "fork": "",                          (string, required) fork hash
   "height": 0,                         (uint, required) block height
   "txmint": "",                        (string, required) transaction mint hash
   "tx":                                (array, required, default=RPCValid) transaction hash list
   [
     "tx": ""                           (string, required) transaction hash
   ]
   "prev": ""                           (string, optional) previous block hash
 }
```
**Examples:**
```
>> ibrio-cli getblock ca49b8d07ac2849c455a813dd967bb0b306b48406d787259f4ddb8f6a0e0cf4c
<< {"hash":"ca49b8d07ac2849c455a813dd967bb0b306b48406d787259f4ddb8f6a0e0cf4c","version":1,"type":"primary-pow","time":1538138566,"prev":"47b86e794e7ce0546def4fe3603d58d9cc9fc87eeee676bd15ae90e45ab51f8a","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","height":31296,"txmint":"3d4ed629c594b924d72480e29a332ca91915be685c85940a8c501f8248269e29","tx":[]}

>> curl -d '{"id":10,"method":"getblock","jsonrpc":"2.0","params":{"block":"ca49b8d07ac2849c455a813dd967bb0b306b48406d787259f4ddb8f6a0e0cf4c"}}' http://127.0.0.1:6602
<< {"id":10,"jsonrpc":"2.0","result":{"hash":"ca49b8d07ac2849c455a813dd967bb0b306b48406d787259f4ddb8f6a0e0cf4c","version":1,"type":"primary-pow","time":1538138566,"prev":"47b86e794e7ce0546def4fe3603d58d9cc9fc87eeee676bd15ae90e45ab51f8a","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","height":31296,"txmint":"3d4ed629c594b924d72480e29a332ca91915be685c85940a8c501f8248269e29","tx":[]}}
```
**Errors:**
```
* {"code":-6,"message":"Unknown block."}
```
##### [Back to top](#commands)
---
### getblockdetail
**Usage:**
```
        getblockdetail <"block">

Return details of a block with given block-hash.
```
**Arguments:**
```
 "block"                                (string, required) block hash
```
**Request:**
```
 "param" :
 {
   "block": ""                          (string, required) block hash
 }
```
**Response:**
```
 "result" :
 {
   "hash": "",                          (string, required) block hash
   "hashPrev": "",                      (string, required) block prev hash
   "version": 0,                        (uint, required) version
   "type": "",                          (string, required) block type
   "time": 0,                           (uint, required) block time
   "bits": 0,                           (uint, required) nBits
   "fork": "",                          (string, required) fork hash
   "height": 0,                         (uint, required) block height
   "txmint":                            (object, required) transaction mint data
   {
     "txid": "",                        (string, required) transaction hash
     "version": 0,                      (uint, required) version
     "type": "",                        (string, required) transaction type
     "time": 0,                         (uint, required) transaction timestamp
     "lockuntil": 0,                    (uint, required) unlock time
     "anchor": "",                      (string, required) anchor hash
     "blockhash": "",                   (string, required) which block tx located in
     "vin":                             (array, required, default=RPCValid) vin list
     [
       {
         "txid": "",                    (string, required) pre-vout transaction hash
         "vout": 0                      (uint, required) pre-vout number
       }
     ]
     "sendfrom": "",                    (string, required) send from address
     "sendto": "",                      (string, required) send to address
     "amount": 0.0,                     (double, required) amount
     "txfee": 0.0,                      (double, required) transaction fee
     "data": "",                        (string, required) data
     "sig": "",                         (string, required) sign
     "fork": "",                        (string, required) fork hash
     "confirmations": 0                 (int, optional) confirmations
   }
   "tx":                                (array, required, default=RPCValid) transaction hash list
   [
     {
       "txid": "",                      (string, required) transaction hash
       "version": 0,                    (uint, required) version
       "type": "",                      (string, required) transaction type
       "time": 0,                       (uint, required) transaction timestamp
       "lockuntil": 0,                  (uint, required) unlock time
       "anchor": "",                    (string, required) anchor hash
       "blockhash": "",                 (string, required) which block tx located in
       "vin":                           (array, required, default=RPCValid) vin list
       [
         {
           "txid": "",                  (string, required) pre-vout transaction hash
           "vout": 0                    (uint, required) pre-vout number
         }
       ]
       "sendfrom": "",                  (string, required) send from address
       "sendto": "",                    (string, required) send to address
       "amount": 0.0,                   (double, required) amount
       "txfee": 0.0,                    (double, required) transaction fee
       "data": "",                      (string, required) data
       "sig": "",                       (string, required) sign
       "fork": "",                      (string, required) fork hash
       "confirmations": 0               (int, optional) confirmations
     }
   ]
   "prev": ""                           (string, optional) previous block hash
 }
```
**Examples:**
```
>> ibrio-cli getblockdetail 0000497da49cf85b3f7faabe13716534b6dfd9e287b109356cb6bcb8c795f0d7
<< {"hash":"0000497da49cf85b3f7faabe13716534b6dfd9e287b109356cb6bcb8c795f0d7","hashPrev":"0000497c07f9b1309dd48aa729ef8cea91dd2610b9e93fe6d5a210d035a7d6f0","version":1,"type":"primary-pow","time":1576134143,"bits":36,"fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","height":18813,"txmint":{"txid":"5df1e5ffc3bea8bb3e86f822e5072aa2843242f1889f3cdc5559f7201569079a","version":1,"type":"work","time":1576134143,"lockuntil":0,"anchor":"0000497c07f9b1309dd48aa729ef8cea91dd2610b9e93fe6d5a210d035a7d6f0","vin":[],"sendfrom":"000000000000000000000000000000000000000000000000000000000","sendto":"20g075m4mh5trbkdy8vbh74n9h7t3npe7tewpnvsfb19p57jnyf3kdh45","amount":1153.000800,"txfee":0.000000,"data":"","sig":"","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},"tx":[{"txid":"5df1e5eb943a31136951da3afb491d4d6f4e3aeba5cfca9c4ef5ca00c30920b8","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e2d022709018117f93273ca192308776586916ba21ee7e513fe04fab9b2f","vout":0},{"txid":"5df1dee3e78e4526f90d15cdcc8e96fedfe1103eca7065772f2ac2bc41d4ad8f","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"1xjzjhz5w0an635wngt5qyx8xbg9x2vg0wzkz1a9hx5c97w0b9k0wbzjt","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f99612f76c34f85b16bee2dd3c06e7cb876fb1ef7d6f7d2160de2d3fd9beb75c9b19e90ed9c46053ce81106e1e37717ad175489f30045180be1a2cc7ae524f790e","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},{"txid":"5df1e5eb90ce47fceb3ace5e463f88db7ec98769e2a77571f5bc4b7e957aa69f","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb943a31136951da3afb491d4d6f4e3aeba5cfca9c4ef5ca00c30920b8","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"1c23w26d19h6sntvktzfycpgwfge4efy3zghdjc93mnxbptdw7yqwbwgy","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f9374f46f5caf72e9488a168a64c21744d3160b67d168376420945f9375dfda5a5967678db5f32e26ea17e990f9d00890ae0cc4b72ed0ed8a12f80c2aa6de40700","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},{"txid":"5df1e5eb8208741b33a708a72a29a7380843789975e000dcc74eb31f857b691e","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e1ea583124fa957a4c995cdea702bbdea2e832f8b0fe9a2d1d32578b8869","vout":0},{"txid":"5df1e5eb90ce47fceb3ace5e463f88db7ec98769e2a77571f5bc4b7e957aa69f","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"11nf1cnwft7f2yhr3qnbx9qxsc0b75y4gn7v1rd1qza8khzjdv7623gmm","amount":999.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f95b134de0889f32099b50f354d754b34b4cebdfab6eaa150f0de465967c020a3f140713591b1faf03d4112fa0924a796aca54be26fb8091ba472e8e0632e7c600","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},{"txid":"5df1e5eb17690dbeae0609370f87c30b490eaf123500fdd55f961780415d5d22","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb8208741b33a708a72a29a7380843789975e000dcc74eb31f857b691e","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"1eenpdhcyk5y3ma5pjzf3gp2drq344mc7vcdep170zmbafg5317ae044h","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f977497144b8d4fc40250ecfc7e007d03f1d6293decd0bb115a0b5939a92314282edce494dde0b992e2b503a1f9f50ae00f6f8bc850fd4cbd5e32771f54bea470f","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},{"txid":"5df1e5eb3000e640a2bea70acdc4f994fb05c8a36e54480e5cdc0ccc578f9cc0","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e3e1bd39f2188ce116039c8af172b9a6b4a4ae464bef5734b496540d9db4","vout":0},{"txid":"5df1e5eb17690dbeae0609370f87c30b490eaf123500fdd55f961780415d5d22","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"1q6xgeqaa9hzy19qhhfpmg7hhee05bnq6ha7ph2kvb1d2w6b8qe8hffcy","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f969d47d443711312df441072eca89c3cea44197bec0a7b709ad9d533684f051081c94ed71bea935a8f2eb224cf3a4cb6fa0c79e8925e68a6b8ff35a3fe196a80f","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},{"txid":"5df1e5ebb45d6713d5048b73780a2e5e9e36a10c5b432bb409ff930b116ffef1","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb3000e640a2bea70acdc4f994fb05c8a36e54480e5cdc0ccc578f9cc0","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"1mp11457nha6830emx3mkv4r2zvtg6aebsacdd72x8gzkpqrpvcn6ygd9","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f9c517184e60c41bde9d504d48a02f815a6de60889175d0307b5810336d1c0eacc63978ff83338a5c7546c6e16d76336ac9c436f95cf9dc9e06928e3df80a7a00e","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},{"txid":"5df1e5eb169ec81721b49ca9325b80afd950a685067aa003ae83b8d6d0d982b0","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e337d09b7d286885cd69c0da98c7d04cf740221019d70bcbbb313a159d2a","vout":0},{"txid":"5df1e5ebb45d6713d5048b73780a2e5e9e36a10c5b432bb409ff930b116ffef1","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"1a2fse94fpjepygypmn7qwg8jv8jt2nybqdp2me317tn3t7qy20m9gqmt","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f98b71bda06c23d650ed07d3a0c743689583df2c7f0c26be9c8a75b26e20d3cdafd2bfc6653ce519bd2164ae5f1301b65a220b1a7a34fb0676850f26fcf567c30e","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15},{"txid":"5df1e5ebf4b068ad6775a68cb27a95abea0ee058b1d7c356e46fa04e006b255e","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb169ec81721b49ca9325b80afd950a685067aa003ae83b8d6d0d982b0","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"161pk1rj8qkxmbwfcw4131b9qmxqcby7975vhe81h6k3wczg8r7dw7xg2","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f959d19b18b937ef40516e051d8e36a86d86291f2fb2f4db30ba465eb076154b294b0024ab95267f353f095053bd0160999eaeb902c7add7bf11f54ebce1166202","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":15}],"prev":"0000497c07f9b1309dd48aa729ef8cea91dd2610b9e93fe6d5a210d035a7d6f0"}

>> curl -d '{"id":10,"method":"getblockdetail","jsonrpc":"2.0","params":{"block":"0000497da49cf85b3f7faabe13716534b6dfd9e287b109356cb6bcb8c795f0d7"}}' http://127.0.0.1:6602
<< {"id":10,"jsonrpc":"2.0","result":{"hash":"0000497da49cf85b3f7faabe13716534b6dfd9e287b109356cb6bcb8c795f0d7","hashPrev":"0000497c07f9b1309dd48aa729ef8cea91dd2610b9e93fe6d5a210d035a7d6f0","version":1,"type":"primary-pow","time":1576134143,"bits":36,"fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","height":18813,"txmint":{"txid":"5df1e5ffc3bea8bb3e86f822e5072aa2843242f1889f3cdc5559f7201569079a","version":1,"type":"work","time":1576134143,"lockuntil":0,"anchor":"0000497c07f9b1309dd48aa729ef8cea91dd2610b9e93fe6d5a210d035a7d6f0","vin":[],"sendfrom":"000000000000000000000000000000000000000000000000000000000","sendto":"20g075m4mh5trbkdy8vbh74n9h7t3npe7tewpnvsfb19p57jnyf3kdh45","amount":1153.000800,"txfee":0.000000,"data":"","sig":"","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},"tx":[{"txid":"5df1e5eb943a31136951da3afb491d4d6f4e3aeba5cfca9c4ef5ca00c30920b8","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e2d022709018117f93273ca192308776586916ba21ee7e513fe04fab9b2f","vout":0},{"txid":"5df1dee3e78e4526f90d15cdcc8e96fedfe1103eca7065772f2ac2bc41d4ad8f","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"1xjzjhz5w0an635wngt5qyx8xbg9x2vg0wzkz1a9hx5c97w0b9k0wbzjt","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f99612f76c34f85b16bee2dd3c06e7cb876fb1ef7d6f7d2160de2d3fd9beb75c9b19e90ed9c46053ce81106e1e37717ad175489f30045180be1a2cc7ae524f790e","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},{"txid":"5df1e5eb90ce47fceb3ace5e463f88db7ec98769e2a77571f5bc4b7e957aa69f","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb943a31136951da3afb491d4d6f4e3aeba5cfca9c4ef5ca00c30920b8","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"1c23w26d19h6sntvktzfycpgwfge4efy3zghdjc93mnxbptdw7yqwbwgy","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f9374f46f5caf72e9488a168a64c21744d3160b67d168376420945f9375dfda5a5967678db5f32e26ea17e990f9d00890ae0cc4b72ed0ed8a12f80c2aa6de40700","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},{"txid":"5df1e5eb8208741b33a708a72a29a7380843789975e000dcc74eb31f857b691e","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e1ea583124fa957a4c995cdea702bbdea2e832f8b0fe9a2d1d32578b8869","vout":0},{"txid":"5df1e5eb90ce47fceb3ace5e463f88db7ec98769e2a77571f5bc4b7e957aa69f","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"11nf1cnwft7f2yhr3qnbx9qxsc0b75y4gn7v1rd1qza8khzjdv7623gmm","amount":999.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f95b134de0889f32099b50f354d754b34b4cebdfab6eaa150f0de465967c020a3f140713591b1faf03d4112fa0924a796aca54be26fb8091ba472e8e0632e7c600","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},{"txid":"5df1e5eb17690dbeae0609370f87c30b490eaf123500fdd55f961780415d5d22","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb8208741b33a708a72a29a7380843789975e000dcc74eb31f857b691e","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"1eenpdhcyk5y3ma5pjzf3gp2drq344mc7vcdep170zmbafg5317ae044h","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f977497144b8d4fc40250ecfc7e007d03f1d6293decd0bb115a0b5939a92314282edce494dde0b992e2b503a1f9f50ae00f6f8bc850fd4cbd5e32771f54bea470f","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},{"txid":"5df1e5eb3000e640a2bea70acdc4f994fb05c8a36e54480e5cdc0ccc578f9cc0","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e3e1bd39f2188ce116039c8af172b9a6b4a4ae464bef5734b496540d9db4","vout":0},{"txid":"5df1e5eb17690dbeae0609370f87c30b490eaf123500fdd55f961780415d5d22","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"1q6xgeqaa9hzy19qhhfpmg7hhee05bnq6ha7ph2kvb1d2w6b8qe8hffcy","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f969d47d443711312df441072eca89c3cea44197bec0a7b709ad9d533684f051081c94ed71bea935a8f2eb224cf3a4cb6fa0c79e8925e68a6b8ff35a3fe196a80f","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},{"txid":"5df1e5ebb45d6713d5048b73780a2e5e9e36a10c5b432bb409ff930b116ffef1","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb3000e640a2bea70acdc4f994fb05c8a36e54480e5cdc0ccc578f9cc0","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"1mp11457nha6830emx3mkv4r2zvtg6aebsacdd72x8gzkpqrpvcn6ygd9","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f9c517184e60c41bde9d504d48a02f815a6de60889175d0307b5810336d1c0eacc63978ff83338a5c7546c6e16d76336ac9c436f95cf9dc9e06928e3df80a7a00e","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},{"txid":"5df1e5eb169ec81721b49ca9325b80afd950a685067aa003ae83b8d6d0d982b0","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e337d09b7d286885cd69c0da98c7d04cf740221019d70bcbbb313a159d2a","vout":0},{"txid":"5df1e5ebb45d6713d5048b73780a2e5e9e36a10c5b432bb409ff930b116ffef1","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"1a2fse94fpjepygypmn7qwg8jv8jt2nybqdp2me317tn3t7qy20m9gqmt","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f98b71bda06c23d650ed07d3a0c743689583df2c7f0c26be9c8a75b26e20d3cdafd2bfc6653ce519bd2164ae5f1301b65a220b1a7a34fb0676850f26fcf567c30e","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63},{"txid":"5df1e5ebf4b068ad6775a68cb27a95abea0ee058b1d7c356e46fa04e006b255e","version":1,"type":"token","time":1576134123,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df1e5eb169ec81721b49ca9325b80afd950a685067aa003ae83b8d6d0d982b0","vout":1}],"sendfrom":"20g07atym1beahmdk267hkqrgvhw1x0gj3bwth8q7yxcyfgcbszbpqgsr","sendto":"161pk1rj8qkxmbwfcw4131b9qmxqcby7975vhe81h6k3wczg8r7dw7xg2","amount":499.999900,"txfee":0.000100,"data":"","sig":"64f1a77bd0e00f8023ffa2f7e0a76eb795414d9a57eb2f4ce5e9cc730c8103c501e1cbd24fa95312b81d2dc5ef6f60c39a9485819d4fa11bcfdde5f99151c8a4f959d19b18b937ef40516e051d8e36a86d86291f2fb2f4db30ba465eb076154b294b0024ab95267f353f095053bd0160999eaeb902c7add7bf11f54ebce1166202","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":63}],"prev":"0000497c07f9b1309dd48aa729ef8cea91dd2610b9e93fe6d5a210d035a7d6f0"}}
```
**Errors:**
```
* {"code":-6,"message":"Unknown block."}
```
##### [Back to top](#commands)
---
### gettxpool
**Usage:**
```
        gettxpool (-f="fork") (-a="address") (-d|-nod*detail*) (-o=getoffset) (-n=getcount)

If detail==0, return the count and total size of txs for given fork.
Otherwise,return all transaction ids and sizes in memory pool for given fork.
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork hash
 -a="address"                           (string, optional) address
 -d|-nod*detail*                        (bool, optional, default=false) get detail or not
 -o=getoffset                           (int, optional, default=0) get offset, If not set, from 0
 -n=getcount                            (int, optional, default=20) get count, 0 is all
```
**Request:**
```
 "param" :
 {
   "fork": "",                          (string, optional) fork hash
   "address": "",                       (string, optional) address
   "detail": true|false,                (bool, optional, default=false) get detail or not
   "getoffset": 0,                      (int, optional, default=0) get offset, If not set, from 0
   "getcount": 0                        (int, optional, default=20) get count, 0 is all
 }
```
**Response:**
```
 "result" :
 {
   (if detail=false)
   "count": 0,                          (uint, optional) transaction pool count
   (if detail=false)
   "size": 0,                           (uint, optional) transaction total size
   (if detail=true)
   "list":                              (array, optional) transaction pool list
   [
     {
       "txid": "",                      (string, required) txid
       "txtype": "",                    (string, required) tx type
       "from": "",                      (string, required) from address
       "to": "",                        (string, required) to address
       "amount": 0.0,                   (double, required) amount
       "txfee": 0.0,                    (double, required) tx fee
       "size": 0                        (uint, required) tx pool size
     }
   ]
 }
```
**Examples:**
```
>> ibrio-cli gettxpool
<< {"count":0,"size":0}

>> curl -d '{"id":11,"method":"gettxpool","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":11,"jsonrpc":"2.0","result":{"count":0,"size":0}}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
```
##### [Back to top](#commands)
---
### gettransaction
**Usage:**
```
        gettransaction <"txid"> (-s|-nos*serialized*)

Get transaction information
```
**Arguments:**
```
 "txid"                                 (string, required) transaction hash
 -s|-nos*serialized*                    (bool, optional, default=false) If serialized=0, return an Object with information about <txid>.
                                        If serialized is non-zero, return a string that is
                                        serialized, hex-encoded data for <txid>.
```
**Request:**
```
 "param" :
 {
   "txid": "",                          (string, required) transaction hash
   "serialized": true|false             (bool, optional, default=false) If serialized=0, return an Object with information about <txid>.
                                        If serialized is non-zero, return a string that is
                                        serialized, hex-encoded data for <txid>.
 }
```
**Response:**
```
 "result" :
 {
   (if serialized=true)
   "serialization": "",                 (string, optional) transaction hex data
   (if serialized=false)
   "transaction":                       (object, optional) transaction data
   {
     "txid": "",                        (string, required) transaction hash
     "version": 0,                      (uint, required) version
     "type": "",                        (string, required) transaction type
     "time": 0,                         (uint, required) transaction timestamp
     "lockuntil": 0,                    (uint, required) unlock time
     "anchor": "",                      (string, required) anchor hash
     "blockhash": "",                   (string, required) which block tx located in
     "vin":                             (array, required, default=RPCValid) vin list
     [
       {
         "txid": "",                    (string, required) pre-vout transaction hash
         "vout": 0                      (uint, required) pre-vout number
       }
     ]
     "sendfrom": "",                    (string, required) send from address
     "sendto": "",                      (string, required) send to address
     "amount": 0.0,                     (double, required) amount
     "txfee": 0.0,                      (double, required) transaction fee
     "data": "",                        (string, required) data
     "sig": "",                         (string, required) sign
     "fork": "",                        (string, required) fork hash
     "confirmations": 0                 (int, optional) confirmations
   }
 }
```
**Examples:**
```
>> ibrio-cli gettransaction 5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03
<< {"transaction":{"txid":"5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03","version":1,"type":"token","time":1576046641,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df088912a5a607904d8c69670a8b704c5739e847d2002c862f6b0cd712b797a","vout":0}],"sendfrom":"1n56xmva8131c4q0961anv5wdzfqtdctyg9e53fxp65f1jyhbtkfbxz6q","sendto":"1mkeeh3zeeejsvknz5d0bm78k81s585jbj5kf0rxjx3ah6ngh33b1erg9","amount":162.260000,"txfee":0.100000,"data":"","sig":"d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":1597}}

>> curl -d '{"id":13,"method":"gettransaction","jsonrpc":"2.0","params":{"txid":"5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03","serialized":false}}' http://127.0.0.1:6602
<< {"id":13,"jsonrpc":"2.0","result":{"transaction":{"txid":"5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03","version":1,"type":"token","time":1576046641,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df088912a5a607904d8c69670a8b704c5739e847d2002c862f6b0cd712b797a","vout":0}],"sendfrom":"1n56xmva8131c4q0961anv5wdzfqtdctyg9e53fxp65f1jyhbtkfbxz6q","sendto":"1mkeeh3zeeejsvknz5d0bm78k81s585jbj5kf0rxjx3ah6ngh33b1erg9","amount":162.260000,"txfee":0.100000,"data":"","sig":"d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","confirmations":1631}}}

>> ibrio-cli gettransaction -s 5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03
<< {"serialization":"010000003190f05d00000000701af4705c5e6fcb04efc3ca3c851c1e4d8948e10923025f54bea9b000000000017a792b71cdb0f662c802207d849e73c504b7a87096c6d80479605a2a9188f05d0001a4dce88fee73a59dcebf2b40ba1d13407254164b9166f063b2e8d513561118d620e4ab0900000000a0860100000000000040d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000"}

>> curl -d '{"id":13,"method":"gettransaction","jsonrpc":"2.0","params":{"txid":"5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03","serialized":true}}' http://127.0.0.1:6602
<< {"id":13,"jsonrpc":"2.0","result":{"serialization":"010000003190f05d00000000701af4705c5e6fcb04efc3ca3c851c1e4d8948e10923025f54bea9b000000000017a792b71cdb0f662c802207d849e73c504b7a87096c6d80479605a2a9188f05d0001a4dce88fee73a59dcebf2b40ba1d13407254164b9166f063b2e8d513561118d620e4ab0900000000a0860100000000000040d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000"}}
```
**Errors:**
```
* {"code":-6,"message":"No information available about transaction."}
```
##### [Back to top](#commands)
---
### sendtransaction
**Usage:**
```
        sendtransaction <"txdata">

Submit raw transaction (serialized, hex-encoded) to local node and network.
```
**Arguments:**
```
 "txdata"                               (string, required) transaction binary data
```
**Request:**
```
 "param" :
 {
   "txdata": ""                         (string, required) transaction binary data
 }
```
**Response:**
```
 "result": "data"                       (string, required) transaction raw data
```
**Examples:**
```
>> ibrio-cli sendtransaction 01000000000000002b747e24738befccff4a05c21dba749632cb8eb410233fa110e3f58a779b4325010ef45be50157453a57519929052d0818c269dee60be98958d5ab65bc7e0919810001b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f440420f0000000000a0860100000000000212348182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e0182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052ed494d90cd96c252446b4a10459fea8c06186154b2bee2ce2182556e9ba40e7e69ddae2501862e4251bba2abf11c90d6f1fd0dec48a1419e81bb8c7d922cf3e03
<< 0a1b944071970589aa524a6f4e40e0b50bab9a64feefc292867692bbf35442a6

>> curl -d '{"id":9,"method":"sendtransaction","jsonrpc":"2.0","params":{"txdata":"01000000000000002b747e24738befccff4a05c21dba749632cb8eb410233fa110e3f58a779b4325010ef45be50157453a57519929052d0818c269dee60be98958d5ab65bc7e0919810001b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f440420f0000000000a0860100000000000212348182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e0182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052ed494d90cd96c252446b4a10459fea8c06186154b2bee2ce2182556e9ba40e7e69ddae2501862e4251bba2abf11c90d6f1fd0dec48a1419e81bb8c7d922cf3e03"}}' http://127.0.0.1:6602
<< {"id":9,"jsonrpc":"2.0","result":"0a1b944071970589aa524a6f4e40e0b50bab9a64feefc292867692bbf35442a6"}
```
**Errors:**
```
* {"code":-8,"message":"TX decode failed"}
* {"code":-10,"message":"Tx rejected : xxx"}
```
##### [Back to top](#commands)
---
### getforkheight
**Usage:**
```
        getforkheight (-f="fork")

Return the number of height in the given fork.
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork hash
```
**Request:**
```
 "param" :
 {
   "fork": ""                           (string, optional) fork hash
 }
```
**Response:**
```
 "result": height                       (int, required) fork height
```
**Examples:**
```
>> ibrio-cli getforkheight
<< 32081

>> curl -d '{"id":4,"method":"getforkheight","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":4,"jsonrpc":"2.0","result":32081}

>> ibrio-cli getforkheight -f=a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
```
##### [Back to top](#commands)
---
### getvotes
**Usage:**
```
        getvotes <"address">

Get votes
```
**Arguments:**
```
 "address"                              (string, required) delegate template address or vote template address
```
**Request:**
```
 "param" :
 {
   "address": ""                        (string, required) delegate template address or vote template address
 }
```
**Response:**
```
 "result": votes                        (double, required) number of votes
```
**Examples:**
```
>> ibrio-cli getvotes 20m04f7cbzcgqjtj6arnv65s9ap8f1setyezt34kg2q9vdvd1tgspy5r0
<< 70000000.000000

>> curl -d '{"id":1,"method":"getvotes","jsonrpc":"2.0","params":{"address":"20m04f7cbzcgqjtj6arnv65s9ap8f1setyezt34kg2q9vdvd1tgspy5r0"}}' http://127.0.0.1:6602
<< {"id":0,"jsonrpc":"2.0","result":70000000.000000}
```
**Errors:**
```
* {"code" : -6, "message" : "Invalid address"}
* {"code" : -32603, "message" : "Not a delegate template address"}
* {"code" : -32603, "message" : "Vote template address not imported"}
* {"code" : -32603, "message" : "Query failed"}
```
##### [Back to top](#commands)
---
### listdelegate
**Usage:**
```
        listdelegate (-n=count)

List delegate
```
**Arguments:**
```
 -n=count                               (uint, optional, default=0) list count, default 0 is all
```
**Request:**
```
 "param" :
 {
   "count": 0                           (uint, optional, default=0) list count, default 0 is all
 }
```
**Response:**
```
 "result" :
   "delegate":                          (array, required, default=RPCValid) 
   [
     {
       "name": "",                      (string, required) delegate name
       "address": "",                   (string, required) delegate address
       "votes": 0.0                     (double, required) number of votes
     }
   ]
```
**Examples:**
```
>> ibrio-cli listdelegate
<< {"address":"20m01802pgptaswc5b2dq09kmj10ns88bn69q0msrnz64mtypx4xm5sff","votes":100002000.000000}

>> curl -d '{"id":1,"method":"listdelegate","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":0,"jsonrpc":"2.0","result":"{"address":"20m01802pgptaswc5b2dq09kmj10ns88bn69q0msrnz64mtypx4xm5sff","votes":100002000.000000}"}
```
**Errors:**
```
* {"code" : -32603, "message" : "Query failed"}
```
##### [Back to top](#commands)
---
### listkey
**Usage:**
```
        listkey (-p=page) (-n=count)

Return Object that has pubkey as keys, associated status as values.
```
**Arguments:**
```
 -p=page                                (uint, optional, default=0) page, default is 0
 -n=count                               (uint, optional, default=30) count, default is 30
```
**Request:**
```
 "param" :
 {
   "page": 0,                           (uint, optional, default=0) page, default is 0
   "count": 0                           (uint, optional, default=30) count, default is 30
 }
```
**Response:**
```
 "result" :
   "pubkey":                            (array, required, default=RPCValid) public key list
   [
     {
       "key": "",                       (string, required) public key with hex system
       "version": 0,                    (int, required) public key version
       "public": true|false,            (bool, required) is only public key or not in wallet
       "locked": true|false,            (bool, required) public key locked
       "timeout": 0                     (int, optional) public key timeout locked
     }
   ]
```
**Examples:**
```
>> ibrio-cli listkey
<< [{"key":"3d266a564ec85f3385babf615b1d7eeb01b3e4456d35174732bb9ec0fa8c8f4f","version": 1,"locked": true},{"key":"58e148d9e8610a6504c26ed346d15920c4d832cf0f03ecb8a016e0d0ec838b1b","version": 1,"locked": true}]

>> curl -d '{"id":43,"method":"listkey","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":43,"jsonrpc":"2.0","result":[{"key":"3d266a564ec85f3385babf615b1d7eeb01b3e4456d35174732bb9ec0fa8c8f4f","version": 1,"locked": true},{"key":"58e148d9e8610a6504c26ed346d15920c4d832cf0f03ecb8a016e0d0ec838b1b","version": 1,"locked": true}]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### getnewkey
**Usage:**
```
        getnewkey <"passphrase">

Return a new pubkey for receiving payments.
```
**Arguments:**
```
 "passphrase"                           (string, required) passphrase
```
**Request:**
```
 "param" :
 {
   "passphrase": ""                     (string, required) passphrase
 }
```
**Response:**
```
 "result": "pubkey"                     (string, required) public key
```
**Examples:**
```
>> ibrio-cli getnewkey 123
<< f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9

>> curl -d '{"id":7,"method":"getnewkey","jsonrpc":"2.0","params":{"passphrase":"123"}}' http://127.0.0.1:6602
<< {"id":7,"jsonrpc":"2.0","result":"f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9"}
```
**Errors:**
```
* {"code":-6,"message":"Passphrase must be nonempty"}
* {"code":-401,"message":"Failed add new key."}
```
##### [Back to top](#commands)
---
### removekey
**Usage:**
```
        removekey <"pubkey"> ("passphrase") (-s|-nos*synctx*)

Remove Key in Wallet.
```
**Arguments:**
```
 "pubkey"                               (string, required) public key or pubkey address
 "passphrase"                           (string, optional) passphrase
 -s|-nos*synctx*                        (bool, optional, default=true) sync tx or not
```
**Request:**
```
 "param" :
 {
   "pubkey": "",                        (string, required) public key or pubkey address
   "passphrase": "",                    (string, optional) passphrase
   "synctx": true|false                 (bool, optional, default=true) sync tx or not
 }
```
**Response:**
```
 "result": "pubkey"                     (string, required) public key
```
**Examples:**
```
>> ibrio-cli removekey f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9 123
<< f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9

>> curl -d '{"id":7,"method":"removekey","jsonrpc":"2.0","params":{"pubkey":"f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9","passphrase":"123"}}' http://127.0.0.1:6602
<< {"id":7,"jsonrpc":"2.0","result":"f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9"}
```
**Errors:**
```
* {"code":-6,"message":"Passphrase must be nonempty"}
* {"code":-401,"message":"Failed remove key."}
```
##### [Back to top](#commands)
---
### encryptkey
**Usage:**
```
        encryptkey <"pubkey"> <-new="passphrase"> <-old="oldpassphrase">

Changes the passphrase for <oldpassphrase> to <passphrase>
```
**Arguments:**
```
 "pubkey"                               (string, required) public key
 -new="passphrase"                      (string, required) passphrase of key
 -old="oldpassphrase"                   (string, required) old passphrase of key
```
**Request:**
```
 "param" :
 {
   "pubkey": "",                        (string, required) public key
   "passphrase": "",                    (string, required) passphrase of key
   "oldpassphrase": ""                  (string, required) old passphrase of key
 }
```
**Response:**
```
 "result": "result"                     (string, required) encrypt key result
```
**Examples:**
```
>> encryptkey f4c3babec11363be80e7b6aa1d803d63206a11f36fc99b874b63a262110a0add -new=456 -old=123
<< Encrypt key successfully: f4c3babec11363be80e7b6aa1d803d63206a11f36fc99b874b63a262110a0add

>> curl -d '{"id":5,"method":"encryptkey","jsonrpc":"2.0","params":{"pubkey":"f4c3babec11363be80e7b6aa1d803d63206a11f36fc99b874b63a262110a0add","passphrase":"456","oldpassphrase":"123"}}' http://127.0.0.1:6602
<< {"id":5,"jsonrpc":"2.0","result":"Encrypt key successfully: f4c3babec11363be80e7b6aa1d803d63206a11f36fc99b874b63a262110a0add"}
```
**Errors:**
```
* {"code":-6,"message":"Passphrase must be nonempty"}
* {"code":-6,"message":"Old passphrase must be nonempty"}
* {"code":-4,"message":"Unknown key"}
* {"code":-406,"message":"The passphrase entered was incorrect."}
```
##### [Back to top](#commands)
---
### lockkey
**Usage:**
```
        lockkey <"pubkey">

Removes the encryption key from memory, locking the key.
After calling this method, you will need to call unlockkey again.
before being able to call any methods which require the key to be unlocked.
```
**Arguments:**
```
 "pubkey"                               (string, required) pubkey or pubkey address
```
**Request:**
```
 "param" :
 {
   "pubkey": ""                         (string, required) pubkey or pubkey address
 }
```
**Response:**
```
 "result": "result"                     (string, required) lock key result
```
**Examples:**
```
>> ibrio-cli lockkey 2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882
<< Lock key successfully: 2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882

>> curl -d '{"id":1,"method":"lockkey","jsonrpc":"2.0","params":{"pubkey":"2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882"}}' http://127.0.0.1:6602
<< {"id":1,"jsonrpc":"2.0","result":"Lock key successfully: 2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882"}
```
**Errors:**
```
* {"code":-4,"message":"Unknown key"}
* {"code":-6,"message":"This method only accepts pubkey or pubkey address as parameter rather than template address you supplied."}
* {"code":-401,"message":"Failed to lock key"}
```
##### [Back to top](#commands)
---
### unlockkey
**Usage:**
```
        unlockkey <"pubkey"> <"passphrase"> (-t=timeout)

If (timeout) > 0,stores the wallet decryption key in memory for (timeout) seconds.
before being able to call any methods which require the key to be locked.
```
**Arguments:**
```
 "pubkey"                               (string, required) pubkey or pubkey address
 "passphrase"                           (string, required) passphrase
 -t=timeout                             (int, optional) auto unlock timeout
```
**Request:**
```
 "param" :
 {
   "pubkey": "",                        (string, required) pubkey or pubkey address
   "passphrase": "",                    (string, required) passphrase
   "timeout": 0                         (int, optional) auto unlock timeout
 }
```
**Response:**
```
 "result": "result"                     (string, required) unlock key result
```
**Examples:**
```
>> ibrio-cli unlockkey d716e72ce58e649a57d54751a7707e325b522497da3a69ae8301a2cbec391c07 1234
<< Unlock key successfully: d716e72ce58e649a57d54751a7707e325b522497da3a69ae8301a2cbec391c07

>> curl -d '{"id":13,"method":"unlockkey","jsonrpc":"2.0","params":{"pubkey":"d716e72ce58e649a57d54751a7707e325b522497da3a69ae8301a2cbec391c07","passphrase":"1234"}}' http://127.0.0.1:6602
<< {"id":13,"jsonrpc":"2.0","result":"Unlock key successfully: d716e72ce58e649a57d54751a7707e325b522497da3a69ae8301a2cbec391c07"}

>> ibrio-cli unlockkey f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9 123 10
<< Unlock key successfully: f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9

>> curl -d '{"id":15,"method":"unlockkey","jsonrpc":"2.0","params":{"pubkey":"f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9","passphrase":"123","timeout":10}}' http://127.0.0.1:6602
<< {"id":15,"jsonrpc":"2.0","result":"Unlock key successfully: f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9"}
```
**Errors:**
```
* {"code":-6,"message":"Passphrase must be nonempty"}
* {"code":-4,"message":"Unknown key"}
* {"code":-409,"message":"Key is already unlocked"}
* {"code":-406,"message":"The passphrase entered was incorrect."}
```
##### [Back to top](#commands)
---
### importprivkey
**Usage:**
```
        importprivkey <"privkey"> <"passphrase"> (-s|-nos*synctx*)

Add a private key to your wallet.
```
**Arguments:**
```
 "privkey"                              (string, required) private key
 "passphrase"                           (string, required) passphrase
 -s|-nos*synctx*                        (bool, optional, default=false) sync tx or not
```
**Request:**
```
 "param" :
 {
   "privkey": "",                       (string, required) private key
   "passphrase": "",                    (string, required) passphrase
   "synctx": true|false                 (bool, optional, default=false) sync tx or not
 }
```
**Response:**
```
 "result": "pubkey"                     (string, required) public key with hex number system
```
**Examples:**
```
>> ibrio-cli importprivkey feb51e048380c0ade1cdb60b25e9f3e05cd4507553a97faadc8a94771fcb1a5b 123
<< d716e72ce58e649a57d54751a7707e325b522497da3a69ae8301a2cbec391c07

>> curl -d '{"id":9,"method":"importprivkey","jsonrpc":"2.0","params":{"privkey":"feb51e048380c0ade1cdb60b25e9f3e05cd4507553a97faadc8a94771fcb1a5b","passphrase":"123"}}' http://127.0.0.1:6602
<< {"id":9,"jsonrpc":"2.0","result":"d716e72ce58e649a57d54751a7707e325b522497da3a69ae8301a2cbec391c07"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid private key"}
* {"code":-6,"message":"Passphrase must be nonempty"}
* {"code":-401,"message":"Failed to add key"}
* {"code":-401,"message":"Failed to sync wallet tx"}
```
##### [Back to top](#commands)
---
### importpubkey
**Usage:**
```
        importpubkey <"pubkey"> (-s|-nos*synctx*)

Add a public key to your wallet.
```
**Arguments:**
```
 "pubkey"                               (string, required) private key
 -s|-nos*synctx*                        (bool, optional, default=false) sync tx or not
```
**Request:**
```
 "param" :
 {
   "pubkey": "",                        (string, required) private key
   "synctx": true|false                 (bool, optional, default=false) sync tx or not
 }
```
**Response:**
```
 "result": "address"                    (string, required) address of public key
```
**Examples:**
```
>> ibrio-cli importpubkey 73f3b3d8545b60e58deb791c4da33089a40d7c6156a89e76e00ac0be2a9924d5
<< 1tmj9janyr05e0xmyn1b62z0dmj4k18td3hwyq3f5c1dn9p5kydsjpvrm

>> curl -d '{"id":31,"method":"importpubkey","jsonrpc":"2.0","params":{"pubkey":"73f3b3d8545b60e58deb791c4da33089a40d7c6156a89e76e00ac0be2a9924d5"}' http://127.0.0.1:6602
<< {"id":31,"jsonrpc":"2.0","result":"73f3b3d8545b60e58deb791c4da33089a40d7c6156a89e76e00ac0be2a9924d5"}
```
**Errors:**
```
* {"code":-6,"message":"Template id is not allowed"}
* {"code":-401,"message":"Failed to add key"}
* {"code":-401,"message":"Failed to sync wallet tx"}
```
##### [Back to top](#commands)
---
### importkey
**Usage:**
```
        importkey <"pubkey"> (-s|-nos*synctx*)

Reveal the serialized key corresponding to <pubkey>.
```
**Arguments:**
```
 "pubkey"                               (string, required) public key data
 -s|-nos*synctx*                        (bool, optional, default=false) sync tx or not
```
**Request:**
```
 "param" :
 {
   "pubkey": "",                        (string, required) public key data
   "synctx": true|false                 (bool, optional, default=false) sync tx or not
 }
```
**Response:**
```
 "result": "pubkey"                     (string, required) public key with hex number system
```
**Examples:**
```
>> ibrio-cli importkey 642e19a647f9f2b795b8edf97c849ab1866855c9ac6b59d4cf2d9e63d23639de010000002f63a31bed90496a03bb58269e77b98751aa902be47ecbf9ac3adef221cbdcf6ecfba5a9c86e92323fb5af7a2df3f805caaf5dd80caf630e5eb206f0
<< de3936d2639e2dcfd4596bacc9556886b19a847cf9edb895b7f2f947a6192e64

>> curl -d '{"id":3,"method":"importkey","jsonrpc":"2.0","params":{"pubkey":"642e19a647f9f2b795b8edf97c849ab1866855c9ac6b59d4cf2d9e63d23639de010000002f63a31bed90496a03bb58269e77b98751aa902be47ecbf9ac3adef221cbdcf6ecfba5a9c86e92323fb5af7a2df3f805caaf5dd80caf630e5eb206f0"}}' http://127.0.0.1:6602
<< {"id":3,"jsonrpc":"2.0","result":"de3936d2639e2dcfd4596bacc9556886b19a847cf9edb895b7f2f947a6192e64"}
```
**Errors:**
```
* {"code":-32602,"message":"Failed to verify serialized key"}
* {"code":-32602,"message":"Can't import the key with empty passphrase"}
* {"code":-401,"message":"Failed to add key"}
* {"code":-401,"message":"Failed to sync wallet tx"}
```
##### [Back to top](#commands)
---
### exportkey
**Usage:**
```
        exportkey <"pubkey">

Reveal the serialized key corresponding to <pubkey>.
```
**Arguments:**
```
 "pubkey"                               (string, required) public key
```
**Request:**
```
 "param" :
 {
   "pubkey": ""                         (string, required) public key
 }
```
**Response:**
```
 "result": "pubkey"                     (string, required) public key with binary system
```
**Examples:**
```
>> ibrio-cli exportkey de3936d2639e2dcfd4596bacc9556886b19a847cf9edb895b7f2f947a6192e64
<< 642e19a647f9f2b795b8edf97c849ab1866855c9ac6b59d4cf2d9e63d23639de010000002f63a31bed90496a03bb58269e77b98751aa902be47ecbf9ac3adef221cbdcf6ecfba5a9c86e92323fb5af7a2df3f805caaf5dd80caf630e5eb206f0

>> curl -d '{"id":13,"method":"exportkey","jsonrpc":"2.0","params":{"pubkey":"de3936d2639e2dcfd4596bacc9556886b19a847cf9edb895b7f2f947a6192e64"}}' http://127.0.0.1:6602
<< {"id":13,"jsonrpc":"2.0","result":"642e19a647f9f2b795b8edf97c849ab1866855c9ac6b59d4cf2d9e63d23639de010000002f63a31bed90496a03bb58269e77b98751aa902be47ecbf9ac3adef221cbdcf6ecfba5a9c86e92323fb5af7a2df3f805caaf5dd80caf630e5eb206f0"}
```
**Errors:**
```
* {"code":-4,"message":"Unknown key"}
* {"code":-401,"message":"Failed to export key"}
```
##### [Back to top](#commands)
---
### addnewtemplate
**Usage:**
```
        addnewtemplate <"type"> <{delegate}>|<{vote}>|<{fork}>|<{mint}>|<{multisig}>|<{weighted}>|<{exchange}>|<{payment}>|<{dexorder}>|<{dexmatch}>|<{activate}> (*synctx*)

Return encoded address for the given template id.
```
**Arguments:**
```
 "type"                                 (string, required) template type
  (if type=delegate)
 {delegate}                             (object, required) a delegate template
  (if type=vote)
 {vote}                                 (object, required) a vote template
  (if type=fork)
 {fork}                                 (object, required) a new fork template
  (if type=mint)
 {mint}                                 (object, required) a mint template
  (if type=multisig)
 {multisig}                             (object, required) a multiple sign template
  (if type=weighted)
 {weighted}                             (object, required) a weighted multiple sign template
  (if type=exchange)
 {exchange}                             (object, required) a exchange template
  (if type=payment)
 {payment}                              (object, required) a payment template
  (if type=dexorder)
 {dexorder}                             (object, required) a dex order template
  (if type=dexmatch)
 {dexmatch}                             (object, required) a dex match template
  (if type=activate)
 {activate}                             (object, required) a activate template
 *synctx*                               (bool, optional, default=false) sync tx or not
```
**Request:**
```
 "param" :
 {
   "type": "",                          (string, required) template type
   (if type=delegate)
   "delegate":                          (object, required) a delegate template
   {
     "delegate": "",                    (string, required) delegate public key
     "owner": ""                        (string, required) owner address
   }
   (if type=vote)
   "vote":                              (object, required) a vote template
   {
     "delegate": "",                    (string, required) delegate template address
     "owner": "",                       (string, required) owner address
     "cycle": 0,                        (uint, required) cycle
     "nonce": 0                         (uint, required) nonce
   }
   (if type=fork)
   "fork":                              (object, required) a new fork template
   {
     "redeem": "",                      (string, required) redeem address
     "fork": ""                         (string, required) fork hash
   }
   (if type=mint)
   "mint":                              (object, required) a mint template
   {
     "mint": "",                        (string, required) mint public key
     "spent": ""                        (string, required) spent address
   }
   (if type=multisig)
   "multisig":                          (object, required) a multiple sign template
   {
     "required": 0,                     (int, required) required weight > 0
     "pubkeys":                         (array, required, default=RPCValid) 
     [
       "key": ""                        (string, required) public key
     ]
   }
   (if type=weighted)
   "weighted":                          (object, required) a weighted multiple sign template
   {
     "required": 0,                     (int, required) required weight
     "pubkeys":                         (array, required, default=RPCValid) public keys
     [
       {
         "key": "",                     (string, required) public key
         "weight": 0                    (int, required) weight
       }
     ]
   }
   (if type=exchange)
   "exchange":                          (object, required) a exchange template
   {
     "addr_m": "",                      (string, required) addr_m
     "addr_s": "",                      (string, required) addr_s
     "height_m": 0,                     (int, required) height_m
     "height_s": 0,                     (int, required) height_s
     "fork_m": "",                      (string, required) fork_m hash
     "fork_s": ""                       (string, required) fork_s hash
   }
   (if type=payment)
   "payment":                           (object, required) a payment template
   {
     "business": "",                    (string, required) business
     "customer": "",                    (string, required) customer
     "height_exec": 0,                  (int, required) height_exec
     "height_end": 0,                   (int, required) height_end
     "amount": 0,                       (int, required) amount
     "pledge": 0                        (int, required) pledge
   }
   (if type=dexorder)
   "dexorder":                          (object, required) a dex order template
   {
     "seller_address": "",              (string, required) seller address
     "coinpair": "",                    (string, required) coin pair
     "price": 0.0,                      (double, required) price
     "fee": 0.0,                        (double, required) fee
     "recv_address": "",                (string, required) receive address
     "valid_height": 0,                 (int, required) valid height
     "match_address": "",               (string, required) match address
     "deal_address": "",                (string, required) deal address
     "timestamp": 0                     (uint, required) timestamp
   }
   (if type=dexmatch)
   "dexmatch":                          (object, required) a dex match template
   {
     "match_address": "",               (string, required) match address
     "coinpair": "",                    (string, required) coin pair
     "final_price": 0.0,                (double, required) final price
     "match_amount": 0.0,               (double, required) match amount
     "fee": 0.0,                        (double, required) fee
     "secret_hash": "",                 (string, required) sha256 hash
     "secret_enc": "",                  (string, required) secret encryption
     "seller_order_address": "",        (string, required) seller order address
     "seller_address": "",              (string, required) seller address
     "seller_deal_address": "",         (string, required) seller deal address
     "seller_valid_height": 0,          (int, required) seller valid height
     "buyer_address": "",               (string, required) buyer address
     "buyer_amount": "",                (string, required) buyer match amount
     "buyer_secret_hash": "",           (string, required) buyer secret hash
     "buyer_valid_height": 0            (int, required) buyer valid height
   }
   (if type=activate)
   "activate":                          (object, required) a activate template
   {
     "inviter": "",                     (string, required) inviter address
     "owner": ""                        (string, required) owner address
   }
   "synctx": true|false                 (bool, optional, default=false) sync tx or not
 }
```
**Response:**
```
 "result": "address"                    (string, required) address of template
```
**Examples:**
```
>> ibrio-cli addnewtemplate mint '{"mint": "e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9", "spent": "1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}'
<< 20g0b87qxcd52ceh9zmpzx0hy46pjfzdnqbkh8f4tqs4y0r6sxyzyny25

>> curl -d '{"id":1,"method":"addnewtemplate","jsonrpc":"2.0","params":{"type":"mint","mint":{"mint":"e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9","spent":"1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}}}' http://127.0.0.1:6602
<< {"id":1,"jsonrpc":"2.0","result":"20g0b87qxcd52ceh9zmpzx0hy46pjfzdnqbkh8f4tqs4y0r6sxyzyny25"}

>> ibrio-cli addnewtemplate delegate '{"delegate":"2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882","owner":"1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq"}'

>> ibrio-cli addnewtemplate fork '{"redeem":"1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0"}'

>> ibrio-cli addnewtemplate multisig '{"required": 1, "pubkeys": ["2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882", "f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9"]}'

>> ibrio-cli addnewtemplate weighted '{"required": 1, "pubkeys": [{"key":"2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882", "weight": 1},{"key": "f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9", "weight": 2}]}'

>> ibrio-cli addnewtemplate vote '{"delegate": "20m01802pgptaswc5b2dq09kmj10ns88bn69q0msrnz64mtypx4xm5sff", "owner": "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda", "cycle":7, "nonce":0}'

>> ibrio-cli addnewtemplate activate '{"inviter": "1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq", "owner": "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda", "nonce":0}'

>> ibrio-cli addnewtemplate dexorder '{"seller_address":"1jv78wjv22hmzcwv07bkkphnkj51y0kjc7g9rwdm05erwmr2n8tvh8yjn","coinpair":"IBR/NFM","price":10,"fee": 0.002,"recv_address":"1jv78wjv22hmzcwv07bkkphnkj51y0kjc7g9rwdm05erwmr2n8tvh8yjn","valid_height": 300,"match_address": "15cx56x0gtv44bkt21yryg4m6nn81wtc7gkf6c9vwpvq1cgmm8jm7m5kd","deal_address": "1f2b2n3asbm2rb99fk1c4wp069d0z91enxdz8kmqmq7f0w8tzw64hdevb","timestamp":1234}'

>> ibrio-cli addnewtemplate dexmatch '{"match_address": "15cx56x0gtv44bkt21yryg4m6nn81wtc7gkf6c9vwpvq1cgmm8jm7m5kd","coinpair":"IBR/NFM","final_price":10,"match_amount": 15,"fee": 0.002,"secret_hash":"41f73534701f620ee8f48ecd61bb422fa9243ccc6ddda5e806f5858121c47268","secret_enc": "42f30c39231e1a518e437df6a71e26535ef65f852fb879157bf9903b7d8065edc66eacec81eaef841c1052978b01340e","seller_order_address": "2140c6rd39hpdmgv9m1m80h738gz4f2c0kmpvn707p04c7x1kcb8gawj6","seller_address": "1jv78wjv22hmzcwv07bkkphnkj51y0kjc7g9rwdm05erwmr2n8tvh8yjn","seller_deal_address": "1f2b2n3asbm2rb99fk1c4wp069d0z91enxdz8kmqmq7f0w8tzw64hdevb","seller_valid_height": 300,"buyer_address": "1njqk8wmenyvqs4cz7d8b9pjc6tsdhxtzza050a2n02eqpfcr22ggqg47","buyer_amount": "150","buyer_secret_hash":"dfa313257be0216a498d2eb6e5a1939eb6168d4a9e3356cc20fb957d030b1ac5","buyer_valid_height":320}'
```
**Errors:**
```
* {"code":-6,"message":"Invalid parameters,failed to make template"}
* {"code":-401,"message":"Failed to add template"}
* {"code":-6,"message":"Invalid parameter, missing weight"}
* {"code":-6,"message":"Invalid parameter, missing redeem address"}
* {"code":-6,"message":"Invalid parameter, missing spent address"}
* {"code":-6,"message":"Invalid parameter, missing owner address"}
* {"code":-6,"message":"template type error. type: xxx"}
```
##### [Back to top](#commands)
---
### importtemplate
**Usage:**
```
        importtemplate <"data"> (-s|-nos*synctx*)

Return encoded address for the given template.
```
**Arguments:**
```
 "data"                                 (string, required) template data
 -s|-nos*synctx*                        (bool, optional, default=false) sync tx or not
```
**Request:**
```
 "param" :
 {
   "data": "",                          (string, required) template data
   "synctx": true|false                 (bool, optional, default=false) sync tx or not
 }
```
**Response:**
```
 "result": "address"                    (string, required) address of template
```
**Examples:**
```
>> ibrio-cli importtemplate 0100010282e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e01b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f402
<< 21w2040000000000000000000000000000000000000000000000epcek

>> curl -d '{"id":52,"method":"importtemplate","jsonrpc":"2.0","params":{"data":"0100010282e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e01b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f402"}}' http://127.0.0.1:6602
<< {"id":52,"jsonrpc":"2.0","result":"21w2040000000000000000000000000000000000000000000000epcek"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid parameters,failed to make template"}
* {"code":-401,"message":"Failed to add template"}
* {"code":-401,"message":"Failed to sync wallet tx"}
```
##### [Back to top](#commands)
---
### exporttemplate
**Usage:**
```
        exporttemplate <"address">

Return encoded address for the given template.
```
**Arguments:**
```
 "address"                              (string, required) template address
```
**Request:**
```
 "param" :
 {
   "address": ""                        (string, required) template address
 }
```
**Response:**
```
 "result": "data"                       (string, required) data of template
```
**Examples:**
```
>> ibrio-cli exporttemplate 2040fpytdr4k5h8tk0nferr7zb51tkccrkgqf341s6tg05q9xe6hth1m
<< 0100010282e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e01b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f402

>> curl -d '{"id":25,"method":"exporttemplate","jsonrpc":"2.0","params":{"address":"2040fpytdr4k5h8tk0nferr7zb51tkccrkgqf341s6tg05q9xe6hth1m4"}}' http://127.0.0.1:6602
<< {"id":25,"jsonrpc":"2.0","result":"0100010282e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e01b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f402"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid address"}
* {"code":-401,"message":"Unkown template"}
```
##### [Back to top](#commands)
---
### removetemplate
**Usage:**
```
        removetemplate <"address">

Remove template in Wallet.
```
**Arguments:**
```
 "address"                              (string, required) template address
```
**Request:**
```
 "param" :
 {
   "address": ""                        (string, required) template address
 }
```
**Response:**
```
 "result": "result"                     (string, required) remove result
```
**Examples:**
```
>> ibrio-cli removetemplate 20m01gbqzmw3nvndx684nsxp1z3tcy4rg0715tgc9sraxd7m56ydttama
<< Success

>> curl -d '{"id":7,"method":"removetemplate","jsonrpc":"2.0","params":{"address":"20m01gbqzmw3nvndx684nsxp1z3tcy4rg0715tgc9sraxd7m56ydttama"}}' http://127.0.0.1:6602
<< {"id":7,"jsonrpc":"2.0","result":"Success"}
```
**Errors:**
```
* {"code":-401,"message":"Failed remove template."}
```
##### [Back to top](#commands)
---
### validateaddress
**Usage:**
```
        validateaddress <"address">

Return information about <address>.
```
**Arguments:**
```
 "address"                              (string, required) wallet address
```
**Request:**
```
 "param" :
 {
   "address": ""                        (string, required) wallet address
 }
```
**Response:**
```
 "result" :
 {
   "isvalid": true|false,               (bool, required) is valid
   (if isvalid=true)
   "addressdata":                       (object, required) address data
   {
     "address": "",                     (string, required) wallet address
     "ismine": true|false,              (bool, required) is mine
     "type": "",                        (string, required) type, pubkey or template
     (if type=pubkey)
     "pubkey": "",                      (string, required) public key
     (if type=template)
     "template": "",                    (string, required) template type name
     (if type=template && ismine=true)
     "templatedata":                    (object, required) template data
     {
       "type": "",                      (string, required) template type
       "hex": "",                       (string, required) temtplate data
       (if type=delegate)
       "delegate":                      (object, required) delegate template struct
       {
         "delegate": "",                (string, required) delegate public key
         "owner": ""                    (string, required) owner address
       }
       (if type=vote)
       "vote":                          (object, required) vote template struct
       {
         "delegate": "",                (string, required) delegate template address
         "owner": "",                   (string, required) owner address
         "cycle": 0,                    (uint, required) cycle
         "nonce": 0                     (uint, required) nonce
       }
       (if type=fork)
       "fork":                          (object, required) fork template struct
       {
         "redeem": "",                  (string, required) redeem address
         "fork": ""                     (string, required) fork hash
       }
       (if type=mint)
       "mint":                          (object, required) mint template struct
       {
         "mint": "",                    (string, required) mint public key
         "spent": ""                    (string, required) spent address
       }
       (if type=multisig)
       "multisig":                      (object, required) multisig template struct
       {
         "required": 0,                 (int, required) required weight
         "addresses":                   (array, required, default=RPCValid) 
         [
           "key": ""                    (string, required) public key
         ]
       }
       (if type=exchange)
       "exchange":                      (object, required) exchange template struct
       {
         "spend_m": "",                 (string, required) spend_m
         "spend_s": "",                 (string, required) spend_s
         "height_m": 0,                 (int, required) height m
         "height_s": 0,                 (int, required) height s
         "fork_m": "",                  (string, required) fork m
         "fork_s": ""                   (string, required) fork s
       }
       (if type=payment)
       "payment":                       (object, required) a payment template
       {
         "business": "",                (string, required) business
         "customer": "",                (string, required) customer
         "height_exec": 0,              (int, required) height_exec
         "height_end": 0,               (int, required) height_end
         "amount": 0,                   (int, required) amount
         "pledge": 0                    (int, required) pledge
       }
       (if type=dexorder)
       "dexorder":                      (object, required) a dex order template
       {
         "seller_address": "",          (string, required) seller address
         "coinpair": "",                (string, required) coin pair
         "price": 0.0,                  (double, required) price
         "fee": 0.0,                    (double, required) fee
         "recv_address": "",            (string, required) receive address
         "valid_height": 0,             (int, required) valid height
         "match_address": "",           (string, required) match address
         "deal_address": "",            (string, required) deal address
         "timestamp": 0                 (uint, required) timestamp
       }
       (if type=dexmatch)
       "dexmatch":                      (object, required) a dex match template
       {
         "match_address": "",           (string, required) match address
         "coinpair": "",                (string, required) coin pair
         "final_price": 0.0,            (double, required) final price
         "match_amount": 0.0,           (double, required) match amount
         "fee": 0.0,                    (double, required) fee
         "secret_hash": "",             (string, required) sha256 hash
         "secret_enc": "",              (string, required) secret encryption
         "seller_order_address": "",    (string, required) seller order address
         "seller_address": "",          (string, required) seller address
         "seller_deal_address": "",     (string, required) seller deal address
         "seller_valid_height": 0,      (int, required) seller valid height
         "buyer_address": "",           (string, required) buyer address
         "buyer_amount": "",            (string, required) buyer match amount
         "buyer_secret_hash": "",       (string, required) buyer secret hash
         "buyer_valid_height": 0        (int, required) buyer valid height
       }
       (if type=activate)
       "activate":                      (object, required) a activate template
       {
         "inviter": "",                 (string, required) inviter address
         "owner": ""                    (string, required) owner address
       }
       (if type=weighted)
       "weighted":                      (object, required) weighted template struct
       {
         "required": 0,                 (int, required) required weight
         "addresses":                   (array, required, default=RPCValid) public keys
         [
           {
             "key": "",                 (string, required) public key
             "weight": 0                (int, required) weight
           }
         ]
       }
     }
   }
 }
```
**Examples:**
```
>> ibrio-cli validateaddress 20g0753dp5b817d7v0hbag6a4neetzfdgbcyt2pkx93hrzn97epzbyn26
<< {"isvalid":true,"addressdata":{"address":"20g0753dp5b817d7v0hbag6a4neetzfdgbcyt2pkx93hrzn97epzbyn26","ismine":true,"type":"template","template":"mint"}}

>> curl -d '{"id":2,"method":"validateaddress","jsonrpc":"2.0","params":{"address":"20g0753dp5b817d7v0hbag6a4neetzfdgbcyt2pkx93hrzn97epzbyn26"}}' http://127.0.0.1:6602
<< {"id":2,"jsonrpc":"2.0","result":{"isvalid":true,"addressdata":{"address":"20g0753dp5b817d7v0hbag6a4neetzfdgbcyt2pkx93hrzn97epzbyn26","ismine":true,"type":"template","template":"mint"}}}

>> ibrio-cli validateaddress 123
<< {"isvalid":false}

>> curl -d '{"id":3,"method":"validateaddress","jsonrpc":"2.0","params":{"address":"123"}}' http://127.0.0.1:6602
<< {"id":3,"jsonrpc":"2.0","result":{"isvalid":false}}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### getdefirelation
**Usage:**
```
        getdefirelation (-f="fork") (-a="address")
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork hash
 -a="address"                           (string, optional) wallet address
```
**Request:**
```
 "param" :
 {
   "fork": "",                          (string, optional) fork hash
   "address": ""                        (string, optional) wallet address
 }
```
**Response:**
```
 "result": "parent"                     (string, required) parent address string
```
**Examples:**
```
>> ibrio-cli getdefirelation 10g0944xkyk8ybcmzhpv86vb5777jn1sfrdf3svzqn9phxftqth7332bb
<< 10g0944xkyk8ybcmzhpv86vb5777jn1sfrdf3svzqn9phxftqth8116bm

>> curl -d '{"id":1,"method":"getbalance","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":1,"jsonrpc":"2.0","result":[{"address":"20g098nza351f53wppg0kfnsbxqf80h3x8fwp9vdmc98fbrgbv6mtjagy","avail":30.00000000,"locked":0.00000000,"unconfirmed":0.00000000}]}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Invalid address"}
```
##### [Back to top](#commands)
---
### getbalance
**Usage:**
```
        getbalance (-f="fork") (-a="address") (-p=page) (-n=count)

Get balance of address.
If (address) is not specified, return the balance for wallet's each address.
If (address) is specified, return the balance in the address.
```
**Arguments:**
```
 -f="fork"                              (string, optional) fork hash, default is genesis
 -a="address"                           (string, optional) address, default is all
 -p=page                                (uint, optional, default=0) page, default is 0
 -n=count                               (uint, optional, default=30) count, default is 30
```
**Request:**
```
 "param" :
 {
   "fork": "",                          (string, optional) fork hash, default is genesis
   "address": "",                       (string, optional) address, default is all
   "page": 0,                           (uint, optional, default=0) page, default is 0
   "count": 0                           (uint, optional, default=30) count, default is 30
 }
```
**Response:**
```
 "result" :
   "balance":                           (array, required, default=RPCValid) 
   [
     {
       "address": "",                   (string, required) wallet address
       "avail": 0.0,                    (double, required) balance available amount
       "locked": 0.0,                   (double, required) locked amount
       "unconfirmed": 0.0               (double, required) unconfirmed amount
     }
   ]
```
**Examples:**
```
>> ibrio-cli getbalance
<< [{"address":"20g098nza351f53wppg0kfnsbxqf80h3x8fwp9vdmc98fbrgbv6mtjagy","avail":30.00000000,"locked":0.00000000,"unconfirmed":0.00000000}]

>> curl -d '{"id":1,"method":"getbalance","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":1,"jsonrpc":"2.0","result":[{"address":"20g098nza351f53wppg0kfnsbxqf80h3x8fwp9vdmc98fbrgbv6mtjagy","avail":30.00000000,"locked":0.00000000,"unconfirmed":0.00000000}]}

>> ibrio-cli getbalance -a=20g0944xkyk8ybcmzhpv86vb5777jn1sfrdf3svzqn9phxftqth8116bm
<< [{"address":"20g0944xkyk8ybcmzhpv86vb5777jn1sfrdf3svzqn9phxftqth8116bm","avail":58.99990000,"locked":0.00000000,"unconfirmed":13.99990000}]

>> curl -d '{"id":20,"method":"getbalance","jsonrpc":"2.0","params":{"address":"20g0944xkyk8ybcmzhpv86vb5777jn1sfrdf3svzqn9phxftqth8116bm"}}' http://127.0.0.1:6602
<< {"id":20,"jsonrpc":"2.0","result":[{"address":"20g0944xkyk8ybcmzhpv86vb5777jn1sfrdf3svzqn9phxftqth8116bm","avail":58.99990000,"locked":0.00000000,"unconfirmed":13.99990000}]}
```
**Errors:**
```
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Invalid address"}
```
##### [Back to top](#commands)
---
### listtransaction
**Usage:**
```
        listtransaction (-n=count) (-o=offset) (-f="fork") (-a="address") (-h=prevheight) (-s=prevtxseq)

If (offset) < 0,return last (count) transactions,
If (offset) >= 0,return up to (count) most recent transactions skipping the first (offset) transactions.
```
**Arguments:**
```
 -n=count                               (uint, optional) transaction count. If not set, return 10 tx
 -o=offset                              (int, optional) query offset. If not set, from 0
 -f="fork"                              (string, optional) fork hash. If not set, default is genesis
 -a="address"                           (string, optional) from address or sendto address. If not set, default is all wallet address
 -h=prevheight                          (int, optional) query previous parameters: block height, if txpool is -1
 -s=prevtxseq                           (uint, optional) query previous parameters: sequence of tx in block
```
**Request:**
```
 "param" :
 {
   "count": 0,                          (uint, optional) transaction count. If not set, return 10 tx
   "offset": 0,                         (int, optional) query offset. If not set, from 0
   "fork": "",                          (string, optional) fork hash. If not set, default is genesis
   "address": "",                       (string, optional) from address or sendto address. If not set, default is all wallet address
   "prevheight": 0,                     (int, optional) query previous parameters: block height, if txpool is -1
   "prevtxseq": 0                       (uint, optional) query previous parameters: sequence of tx in block
 }
```
**Response:**
```
 "result" :
   "transaction":                       (array, required, default=RPCValid) 
   [
     {
       "txid": "",                      (string, required) transaction hash
       "fork": "",                      (string, required) fork hash
       "type": "",                      (string, required) transaction type
       "time": 0,                       (uint, required) transaction timestamp
       "send": true|false,              (bool, required) is from me
       "to": "",                        (string, required) to address
       "amount": 0.0,                   (double, required) transaction amount
       "fee": 0.0,                      (double, required) transaction fee
       "lockuntil": 0,                  (uint, required) lockuntil
       "blockheight": 0,                (int, optional) block height
       "txseq": 0,                      (uint, optional) tx sequence
       "from": ""                       (string, optional) from address
     }
   ]
```
**Examples:**
```
>> ibrio-cli listtransaction
<< [{"txid":"4a8e6035b575699cdb25d45beadd49f18fb1303f57ec55493139e65d811e74ff","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","blockheight":31296,"type":"work","time":1547916097,"send":false,"to":"20g098nza351f53wppg0kfnsbxqf80h3x8fwp9vdmc98fbrgbv6mtjagy","amount":15.00000000,"fee":0.00000000,"lockuntil":0},{"txid":"0aa6954236382a6c1c46cce7fa3165b4d1718f5e03ca67cd5fe831616a9000da","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","blockheight":31297,"type":"work","time":1547916097,"send":false,"to":"20g098nza351f53wppg0kfnsbxqf80h3x8fwp9vdmc98fbrgbv6mtjagy","amount":15.00000000,"fee":0.00000000,"lockuntil":0}]

>> curl -d '{"id":2,"method":"listtransaction","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":2,"jsonrpc":"2.0","result":[{"txid":"4a8e6035b575699cdb25d45beadd49f18fb1303f57ec55493139e65d811e74ff","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","blockheight":31296,"type":"work","time":1547916097,"send":false,"to":"20g098nza351f53wppg0kfnsbxqf80h3x8fwp9vdmc98fbrgbv6mtjagy","amount":15.00000000,"fee":0.00000000,"lockuntil":0},{"txid":"0aa6954236382a6c1c46cce7fa3165b4d1718f5e03ca67cd5fe831616a9000da","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","blockheight":31297,"type":"work","time":1547916097,"send":false,"to":"20g098nza351f53wppg0kfnsbxqf80h3x8fwp9vdmc98fbrgbv6mtjagy","amount":15.00000000,"fee":0.00000000,"lockuntil":0}]}

>> ibrio-cli listtransaction 1 -1
<< [{"txid":"5a1b7bf5e32a77ecb3c53782a8e06f2b12bdcb73b677d6f89b6f82f85f14373a","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","blockheight":32086,"type":"work","time":1547916097,"send":false,"to":"20g098nza351f53wppg0kfnsbxqf80h3x8fwp9vdmc98fbrgbv6mtjagy","amount":15.00000000,"fee":0.00000000,"lockuntil":0}]

>> curl -d '{"id":0,"method":"listtransaction","jsonrpc":"2.0","params":{"count":1,"offset":-1}}' http://127.0.0.1:6602
<< {"id":0,"jsonrpc":"2.0","result":[{"txid":"5a1b7bf5e32a77ecb3c53782a8e06f2b12bdcb73b677d6f89b6f82f85f14373a","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","blockheight":32086,"type":"work","time":1547916097,"send":false,"to":"20g098nza351f53wppg0kfnsbxqf80h3x8fwp9vdmc98fbrgbv6mtjagy","amount":15.00000000,"fee":0.00000000,"lockuntil":0}]}

>> listtransaction -n=1 -o=-1
```
**Errors:**
```
* {"code":-6,"message":"Negative, zero or out of range count"}
* {"code":-401,"message":"Failed to list transactions"}
```
##### [Back to top](#commands)
---
### sendfrom
**Usage:**
```
        sendfrom <"from"> <"to"> <$amount$> ($txfee$) (-f="fork") (-d="data") (-sm="sign_m") (-ss="sign_s") (-fd="fromdata") (-td="sendtodata") (-type=type) (-sec="signsecret") (-mintheight=mintheight) (-lh=lockheight)

<amount> and <txfee> are real and rounded to the nearest 0.000001
Return transaction id
```
**Arguments:**
```
 "from"                                 (string, required) from address
 "to"                                   (string, required) to address
 $amount$                               (double, required) amount
 $txfee$                                (double, optional) transaction fee
 -f="fork"                              (string, optional) fork hash
 -d="data"                              (string, optional) output data
 -sm="sign_m"                           (string, optional) exchange sign m
 -ss="sign_s"                           (string, optional) exchange sign s
 -fd="fromdata"                         (string, optional) If the 'from' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
 -td="sendtodata"                       (string, optional) If the 'to' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
 -type=type                             (uint, optional, default=0) 0(default): common token tx, 2: defi relation tx, 3: defi mint height tx
 -sec="signsecret"                      (string, optional) sign secret
 -mintheight=mintheight                 (int, optional) mint height of DeFi fork
 -lh=lockheight                         (int, optional) lock height
```
**Request:**
```
 "param" :
 {
   "from": "",                          (string, required) from address
   "to": "",                            (string, required) to address
   "amount": 0.0,                       (double, required) amount
   "txfee": 0.0,                        (double, optional) transaction fee
   "fork": "",                          (string, optional) fork hash
   "data": "",                          (string, optional) output data
   "sign_m": "",                        (string, optional) exchange sign m
   "sign_s": "",                        (string, optional) exchange sign s
   "fromdata": "",                      (string, optional) If the 'from' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
   "sendtodata": "",                    (string, optional) If the 'to' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
   "type": 0,                           (uint, optional, default=0) 0(default): common token tx, 2: defi relation tx, 3: defi mint height tx
   "signsecret": "",                    (string, optional) sign secret
   "mintheight": 0,                     (int, optional) mint height of DeFi fork
   "lockheight": 0                      (int, optional) lock height
 }
```
**Response:**
```
 "result": "transaction"                (string, required) transaction hash
```
**Examples:**
```
>> ibrio-cli sendfrom 20g0944xkyk8ybcmzhpv86vb5777jn1sfrdf3svzqn9phxftqth8116bm 1q71vfagprv5hqwckzbvhep0d0ct72j5j2heak2sgp4vptrtc2btdje3q 1
<< 01a9f3bb967f24396293903c856e99896a514756a220266afa347a8b8c7f0038

>> curl -d '{"id":18,"method":"sendfrom","jsonrpc":"2.0","params":{"from":"20g0944xkyk8ybcmzhpv86vb5777jn1sfrdf3svzqn9phxftqth8116bm","to":"1q71vfagprv5hqwckzbvhep0d0ct72j5j2heak2sgp4vptrtc2btdje3q","amount":1.00000000}}' http://127.0.0.1:6602
<< {"id":18,"jsonrpc":"2.0","result":"01a9f3bb967f24396293903c856e99896a514756a220266afa347a8b8c7f0038"}

>> ibrio-cli sendfrom 20g0753dp5b817d7v0hbag6a4neetzfdgbcyt2pkx93hrzn97epzbyn26 1q71vfagprv5hqwckzbvhep0d0ct72j5j2heak2sgp4vptrtc2btdje3q 1 0.1 -f=a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0
<< 8f92969642024234481e104481f36145736b465ead2d52a6657cf38bd52bdf59

>> curl -d '{"id":53,"method":"sendfrom","jsonrpc":"2.0","params":{"from":"20g0753dp5b817d7v0hbag6a4neetzfdgbcyt2pkx93hrzn97epzbyn26","to":"1q71vfagprv5hqwckzbvhep0d0ct72j5j2heak2sgp4vptrtc2btdje3q","amount":1.00000000,"txfee":0.10000000,"fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0"}}' http://127.0.0.1:6602
<< {"id":53,"jsonrpc":"2.0","result":"8f92969642024234481e104481f36145736b465ead2d52a6657cf38bd52bdf59"}

>> ibrio-cli sendfrom 20r07rwj0032jssv0d3xaes1kq6z1cvjmz1jwhme0m1jf23vx48v683s3 1w8ehkb2jc0qcn7wze3tv8enzzwmytn9b7n7gghwfa219rv1vhhd82n6h 12.345 1 -sm='1b8c6b0807472627cec2f77b2a33428539b64493f7f1b9f7be4dfbdee72f9aeb3b5763fa39ce5df2425d8d530698de9c1cea993bccfce6596901b6b8cb054103' -ss='a51a925a50a7101ca25c8165050b61421f9e54aad5b0cfb4a4947da82f5fb62fec8e96fb80bd697db33f391bf1f875179d446ac08829f85cf8fe6483ec9acf0c' -f='92099485ffec67128fe4dbaca96ed8faf54ccc0b4760cd0d78a3d1e051a2498f'
<< 8f92969642024234481e104481f36145736b465ead2d52a6657cf38bd52bdf59

>> curl -d '{"id":53,"method":"sendfrom","jsonrpc":"2.0","params":{"from":"20r07rwj0032jssv0d3xaes1kq6z1cvjmz1jwhme0m1jf23vx48v683s3","to":"1w8ehkb2jc0qcn7wze3tv8enzzwmytn9b7n7gghwfa219rv1vhhd82n6h","amount":12.345000,"txfee":1.00000000,"sm":"1b8c6b0807472627cec2f77b2a33428539b64493f7f1b9f7be4dfbdee72f9aeb3b5763fa39ce5df2425d8d530698de9c1cea993bccfce6596901b6b8cb054103", "ss":"a51a925a50a7101ca25c8165050b61421f9e54aad5b0cfb4a4947da82f5fb62fec8e96fb80bd697db33f391bf1f875179d446ac08829f85cf8fe6483ec9acf0c","fork":"92099485ffec67128fe4dbaca96ed8faf54ccc0b4760cd0d78a3d1e051a2498f"}}' http://127.0.0.1:6602
<< {"id":53,"jsonrpc":"2.0","result":"5ce79ba78b44f8710710c8e35b8d3b725c2f994d4038b74fda84f0b061c01a76"}

>> ibrio-cli sendfrom 1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm 12pseg2t39s5v8z7cqeyj4f09vcxy4jn0dge8q86bkmkr284bm7gx4dy4 1 -type=2
<< 5f43336e0c300c03491c65d0f79ded0d864a4442f6c3eb9af3b736bf123d5aff

>> curl -d '{"id":9,"method":"sendfrom","jsonrpc":"2.0","params":{"from":"1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm","to":"12pseg2t39s5v8z7cqeyj4f09vcxy4jn0dge8q86bkmkr284bm7gx4dy4","amount":1.000000,"type":2}}' http://127.0.0.1:6602
<< {"id":9,"jsonrpc":"2.0","result":"5f43336e0c300c03491c65d0f79ded0d864a4442f6c3eb9af3b736bf123d5aff"}

>> ibrio-cli sendfrom 1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm 1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm 1 -f=00000001aeb53fd1f94825d758ee130f3de63ac235a3744240af15200a6fe07b -type=3 -mintheight=20
<< 5f43336e0c300c03491c65d0f79ded0d864a4442f6c3eb9af3b736bf123d5aff

>> ibrio-cli sendfrom 1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm 1ae0mjxfcw0vn90r4y1fnd0qw7rkhha7w9smkaf5n9td48zpkjcwdyv5q 8000 -lh=13
<< 5fd32be9996a4a7d0282135d93ad519d9f237da6f0b025e5bd3139272c733ec9
```
**Errors:**
```
* {"code":-6,"message":"Invalid from address"}
* {"code":-6,"message":"Invalid to address"}
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Invalid lockheight"}
* {"code":-401,"message":"Failed to create transaction"}
* {"code":-401,"message":"Failed to sign transaction"}
* {"code":-401,"message":"The signature is not completed"}
* {"code":-10,"message":"Tx rejected : xxx"}
```
##### [Back to top](#commands)
---
### createtransaction
**Usage:**
```
        createtransaction <"from"> <"to"> <$amount$> ($txfee$) (-f="fork") (-d="data") (-type=type) (-mintheight=mintheight) (-lh=lockheight)

<amount> and <txfee> are real and rounded to the nearest 0.000001.
Return serialized tx.
```
**Arguments:**
```
 "from"                                 (string, required) from address
 "to"                                   (string, required) to address
 $amount$                               (double, required) amount
 $txfee$                                (double, optional) transaction fee
 -f="fork"                              (string, optional) fork hash
 -d="data"                              (string, optional) output data
 -type=type                             (uint, optional, default=0) 0(default): common token tx, 2: defi relation tx, 3: defi mint height tx
 -mintheight=mintheight                 (int, optional) mint height of DeFi fork
 -lh=lockheight                         (int, optional) lock height
```
**Request:**
```
 "param" :
 {
   "from": "",                          (string, required) from address
   "to": "",                            (string, required) to address
   "amount": 0.0,                       (double, required) amount
   "txfee": 0.0,                        (double, optional) transaction fee
   "fork": "",                          (string, optional) fork hash
   "data": "",                          (string, optional) output data
   "type": 0,                           (uint, optional, default=0) 0(default): common token tx, 2: defi relation tx, 3: defi mint height tx
   "mintheight": 0,                     (int, optional) mint height of DeFi fork
   "lockheight": 0                      (int, optional) lock height
 }
```
**Response:**
```
 "result": "transaction"                (string, required) transaction data
```
**Examples:**
```
>> ibrio-cli createtransaction 20g0753dp5b817d7v0hbag6a4neetzfdgbcyt2pkx93hrzn97epzbyn26 1q71vfagprv5hqwckzbvhep0d0ct72j5j2heak2sgp4vptrtc2btdje3q 1 0.1 -f=a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0 -d=12345
<< 01000000000000002b747e24738befccff4a05c21dba749632cb8eb410233fa110e3f58a779b4325010ef45be50157453a57519929052d0818c269dee60be98958d5ab65bc7e0919810001b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f440420f0000000000a08601000000000002123400

>> curl -d '{"id":59,"method":"createtransaction","jsonrpc":"2.0","params":{"from":"20g0753dp5b817d7v0hbag6a4neetzfdgbcyt2pkx93hrzn97epzbyn26","to":"1q71vfagprv5hqwckzbvhep0d0ct72j5j2heak2sgp4vptrtc2btdje3q","amount":1.00000000,"txfee":0.10000000,"fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","data":"12345"}}' http://127.0.0.1:6602
<< {"id":59,"jsonrpc":"2.0","result":"01000000000000002b747e24738befccff4a05c21dba749632cb8eb410233fa110e3f58a779b4325010ef45be50157453a57519929052d0818c269dee60be98958d5ab65bc7e0919810001b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f440420f0000000000a08601000000000002123400"}

>> ibrio-cli createtransaction 1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm 12pseg2t39s5v8z7cqeyj4f09vcxy4jn0dge8q86bkmkr284bm7gx4dy4 1 -type=2
<< 010002006334435f0000000091d2b15e9aeeb57483889873bd0aea1273c8e5bb8df436c2bd4e85060000000001ff5a3d12bf36b7f39aebc3f642444a860ded9df7d0651c49030c300c6e33435f010115b2e80b434e4bb47cecbbbd223c09db3be24aa06c1c8ba0cb9d2781208ba1e140420f000000000010270000000000000000

>> curl -d '{"id":17,"method":"createtransaction","jsonrpc":"2.0","params":{"from":"1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm","to":"12pseg2t39s5v8z7cqeyj4f09vcxy4jn0dge8q86bkmkr284bm7gx4dy4","amount":1.000000,"type":2}}' http://127.0.0.1:6602
<< {"id":17,"jsonrpc":"2.0","result":"010002006334435f0000000091d2b15e9aeeb57483889873bd0aea1273c8e5bb8df436c2bd4e85060000000001ff5a3d12bf36b7f39aebc3f642444a860ded9df7d0651c49030c300c6e33435f010115b2e80b434e4bb47cecbbbd223c09db3be24aa06c1c8ba0cb9d2781208ba1e140420f000000000010270000000000000000"}

>> ibrio-cli createtransaction 1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm 1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm 1 -f=00000001ba93ccb7402df70b511637223a08ebbe33c7504eda28235028130d32 -type=3 -mintheight=3000
<< 01000300b0abc05f00000000320d1328502328da4e50c733beeb083a223716510bf72d40b7cc93ba010000000136824ad81f09da9e47fde50e5e2bb228b25ae289966e305c04c25babca6dbf5f0001498b63009dfb70f7ee0902ba95cc171f7d7a97ff16d89fd96e1f1b9e7d5f91da40420f0000000000307500000000000004b80b000000

>> ibrio-cli createtransaction 1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm 1ae0mjxfcw0vn90r4y1fnd0qw7rkhha7w9smkaf5n9td48zpkjcwdyv5q 3000 -lh=16
<< 01000000af2cd35f1000000091d2b15e9aeeb57483889873bd0aea1273c8e5bb8df436c2bd4e85060000000001c3acca171f8703848537280e0a19676cb31f9379b99e069082ab706a862cd35f000153814975ece037548304f05f5682fc3e2718a8fc4e69353cb54e9a447ed39338005ed0b20000000010270000000000000000
```
**Errors:**
```
* {"code":-6,"message":"Invalid from address"}
* {"code":-6,"message":"Invalid to address"}
* {"code":-6,"message":"Invalid fork"}
* {"code":-6,"message":"Unknown fork"}
* {"code":-6,"message":"Invalid lockheight"}
* {"code":-401,"message":"Failed to create transaction"}
```
##### [Back to top](#commands)
---
### signtransaction
**Usage:**
```
        signtransaction <"txdata"> (-sm="sign_m") (-ss="sign_s") (-fd="fromdata") (-td="sendtodata") (-sec="signsecret")

Return json object with keys:
hex : raw transaction with signature(s) (hex-encoded string)
completed : true if transaction has a completed set of signature (false if not)
```
**Arguments:**
```
 "txdata"                               (string, required) transaction data(hex string)
 -sm="sign_m"                           (string, optional) exchange sign m
 -ss="sign_s"                           (string, optional) exchange sign s
 -fd="fromdata"                         (string, optional) If the 'from' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
 -td="sendtodata"                       (string, optional) If the 'to' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
 -sec="signsecret"                      (string, optional) sign secret
```
**Request:**
```
 "param" :
 {
   "txdata": "",                        (string, required) transaction data(hex string)
   "sign_m": "",                        (string, optional) exchange sign m
   "sign_s": "",                        (string, optional) exchange sign s
   "fromdata": "",                      (string, optional) If the 'from' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
   "sendtodata": "",                    (string, optional) If the 'to' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
   "signsecret": ""                     (string, optional) sign secret
 }
```
**Response:**
```
 "result" :
 {
   "hex": "",                           (string, required) hex of transaction data
   "completed": true|false              (bool, required) transaction completed or not
 }
```
**Examples:**
```
>> ibrio-cli signtransaction 01000000000000002b747e24738befccff4a05c21dba749632cb8eb410233fa110e3f58a779b4325010ef45be50157453a57519929052d0818c269dee60be98958d5ab65bc7e0919810001b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f440420f0000000000a08601000000000002123400
<< {"hex":"01000000000000002b747e24738befccff4a05c21dba749632cb8eb410233fa110e3f58a779b4325010ef45be50157453a57519929052d0818c269dee60be98958d5ab65bc7e0919810001b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f440420f0000000000a0860100000000000212348182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e0182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052ed494d90cd96c252446b4a10459fea8c06186154b2bee2ce2182556e9ba40e7e69ddae2501862e4251bba2abf11c90d6f1fd0dec48a1419e81bb8c7d922cf3e03","complete":true}

>> curl -d '{"id":62,"method":"signtransaction","jsonrpc":"2.0","params":{"txdata":"01000000000000002b747e24738befccff4a05c21dba749632cb8eb410233fa110e3f58a779b4325010ef45be50157453a57519929052d0818c269dee60be98958d5ab65bc7e0919810001b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f440420f0000000000a08601000000000002123400"}}' http://127.0.0.1:6602
<< {"id":62,"jsonrpc":"2.0","result":{"hex":"01000000000000002b747e24738befccff4a05c21dba749632cb8eb410233fa110e3f58a779b4325010ef45be50157453a57519929052d0818c269dee60be98958d5ab65bc7e0919810001b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f440420f0000000000a0860100000000000212348182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e0182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052ed494d90cd96c252446b4a10459fea8c06186154b2bee2ce2182556e9ba40e7e69ddae2501862e4251bba2abf11c90d6f1fd0dec48a1419e81bb8c7d922cf3e03","complete":true}}
```
**Errors:**
```
* {"code":-8,"message":"TX decode failed"}
* {"code":-401,"message":"Failed to sign transaction"}
```
##### [Back to top](#commands)
---
### signmessage
**Usage:**
```
        signmessage <"message"> (-pubkey="pubkey") (-privkey="privkey") (-a="addr") (-p|-nop*hasprefix*)

Sign a message with the private key of an pubkey
```
**Arguments:**
```
 "message"                              (string, required) message to be signed. It will be hash(blake2b) before sign
 -pubkey="pubkey"                       (string, optional) public key
 -privkey="privkey"                     (string, optional) private key
 -a="addr"                              (string, optional) address signed
 -p|-nop*hasprefix*                     (bool, optional, default=true) If add magic string(Ibrio Signed Message:
) before message or not
```
**Request:**
```
 "param" :
 {
   "message": "",                       (string, required) message to be signed. It will be hash(blake2b) before sign
   "pubkey": "",                        (string, optional) public key
   "privkey": "",                       (string, optional) private key
   "addr": "",                          (string, optional) address signed
   "hasprefix": true|false              (bool, optional, default=true) If add magic string(Ibrio Signed Message:
) before message or not
 }
```
**Response:**
```
 "result": "signature"                  (string, required) signature of message
```
**Examples:**
```
>> ibrio-cli signmessage 2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882 123456
<< 045977f8c07e6d846d6055357f36a70c16c071cb85115e3ffb498e171a9ac3f4aed1292203a0c8e42c4becafad3ced0d9874abd2a8b788fda9f07099a1e71707

>> curl -d '{"id":4,"method":"signmessage","jsonrpc":"2.0","params":{"pubkey":"2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882","message":"123456"}}' http://127.0.0.1:6602
<< {"id":4,"jsonrpc":"2.0","result":"045977f8c07e6d846d6055357f36a70c16c071cb85115e3ffb498e171a9ac3f4aed1292203a0c8e42c4becafad3ced0d9874abd2a8b788fda9f07099a1e71707"}

>> ibrio-cli signmessage dc7547feed1d145c21915a739f1fcc75f7b9f88ebe4dba176628844d6916803e 3bdc5190cd3283c81d6b7a186610ce4ada5e81c4f7fcb153b379afc6154d0014
<< 98235a42178cf985dfcb58c9e3f728d38b9c60a92107084adc4ce40e391c51593e57ee109ee7c65aca24627219da8fc8e632a57a013967213630e33b17b4b209

>> ibrio-cli signmessage dc7547feed1d145c21915a739f1fcc75f7b9f88ebe4dba176628844d6916803e -pubkey=3bdc5190cd3283c81d6b7a186610ce4ada5e81c4f7fcb153b379afc6154d0014 -nop
<< d73de744d34b4e15dbc790638b65692325adddc4723125139a21e53eee282b79095db38ba27d732d41ad00f5fa2071b3290f8ec77ba69fd8f34537786ae10d07

>> signmessage DeFiRelation00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a7006c4246621002576ec70545f04f2cb75378e3f1a16eca2c596fc1c64f52e122b -privkey=1dc5a5956c2de69f597cf20da70523b024470ae789e1d2bfc157c9605f17a33a -nop
<< eb3540abf61b9cf57caef4a7c24054a5b8dfcb382de053b5377fcd76f05409f120d61b501c17745a676bc78197f456bc303be3d64a89aad69c587ba6ac86cf0c
```
**Errors:**
```
* {"code":-4,"message":"Unknown key"}
* {"code":-405,"message":"Key is locked"}
* {"code":-401,"message":"Failed to sign message"}
```
##### [Back to top](#commands)
---
### listaddress
**Usage:**
```
        listaddress (-p=page) (-n=count)

List all of the addresses from pub keys and template ids
```
**Arguments:**
```
 -p=page                                (uint, optional, default=0) page, default is 0
 -n=count                               (uint, optional, default=30) count, default is 30
```
**Request:**
```
 "param" :
 {
   "page": 0,                           (uint, optional, default=0) page, default is 0
   "count": 0                           (uint, optional, default=30) count, default is 30
 }
```
**Response:**
```
 "result" :
   "addressdata":                       (array, required, default=RPCValid) 
   [
     {
       "type": "",                      (string, required) type, pubkey or template
       "address": "",                   (string, required) public key or template address
       (if type=pubkey)
       "pubkey": "",                    (string, required) public key
       (if type=template)
       "template": "",                  (string, required) template type name
       (if type=template)
       "templatedata":                  (object, required) template data
       {
         "type": "",                    (string, required) template type
         "hex": "",                     (string, required) temtplate data
         (if type=delegate)
         "delegate":                    (object, required) delegate template struct
         {
           "delegate": "",              (string, required) delegate public key
           "owner": ""                  (string, required) owner address
         }
         (if type=vote)
         "vote":                        (object, required) vote template struct
         {
           "delegate": "",              (string, required) delegate template address
           "owner": "",                 (string, required) owner address
           "cycle": 0,                  (uint, required) cycle
           "nonce": 0                   (uint, required) nonce
         }
         (if type=fork)
         "fork":                        (object, required) fork template struct
         {
           "redeem": "",                (string, required) redeem address
           "fork": ""                   (string, required) fork hash
         }
         (if type=mint)
         "mint":                        (object, required) mint template struct
         {
           "mint": "",                  (string, required) mint public key
           "spent": ""                  (string, required) spent address
         }
         (if type=multisig)
         "multisig":                    (object, required) multisig template struct
         {
           "required": 0,               (int, required) required weight
           "addresses":                 (array, required, default=RPCValid) 
           [
             "key": ""                  (string, required) public key
           ]
         }
         (if type=exchange)
         "exchange":                    (object, required) exchange template struct
         {
           "spend_m": "",               (string, required) spend_m
           "spend_s": "",               (string, required) spend_s
           "height_m": 0,               (int, required) height m
           "height_s": 0,               (int, required) height s
           "fork_m": "",                (string, required) fork m
           "fork_s": ""                 (string, required) fork s
         }
         (if type=payment)
         "payment":                     (object, required) a payment template
         {
           "business": "",              (string, required) business
           "customer": "",              (string, required) customer
           "height_exec": 0,            (int, required) height_exec
           "height_end": 0,             (int, required) height_end
           "amount": 0,                 (int, required) amount
           "pledge": 0                  (int, required) pledge
         }
         (if type=dexorder)
         "dexorder":                    (object, required) a dex order template
         {
           "seller_address": "",        (string, required) seller address
           "coinpair": "",              (string, required) coin pair
           "price": 0.0,                (double, required) price
           "fee": 0.0,                  (double, required) fee
           "recv_address": "",          (string, required) receive address
           "valid_height": 0,           (int, required) valid height
           "match_address": "",         (string, required) match address
           "deal_address": "",          (string, required) deal address
           "timestamp": 0               (uint, required) timestamp
         }
         (if type=dexmatch)
         "dexmatch":                    (object, required) a dex match template
         {
           "match_address": "",         (string, required) match address
           "coinpair": "",              (string, required) coin pair
           "final_price": 0.0,          (double, required) final price
           "match_amount": 0.0,         (double, required) match amount
           "fee": 0.0,                  (double, required) fee
           "secret_hash": "",           (string, required) sha256 hash
           "secret_enc": "",            (string, required) secret encryption
           "seller_order_address": "",  (string, required) seller order address
           "seller_address": "",        (string, required) seller address
           "seller_deal_address": "",   (string, required) seller deal address
           "seller_valid_height": 0,    (int, required) seller valid height
           "buyer_address": "",         (string, required) buyer address
           "buyer_amount": "",          (string, required) buyer match amount
           "buyer_secret_hash": "",     (string, required) buyer secret hash
           "buyer_valid_height": 0      (int, required) buyer valid height
         }
         (if type=activate)
         "activate":                    (object, required) a activate template
         {
           "inviter": "",               (string, required) inviter address
           "owner": ""                  (string, required) owner address
         }
         (if type=weighted)
         "weighted":                    (object, required) weighted template struct
         {
           "required": 0,               (int, required) required weight
           "addresses":                 (array, required, default=RPCValid) public keys
           [
             {
               "key": "",               (string, required) public key
               "weight": 0              (int, required) weight
             }
           ]
         }
       }
     }
   ]
```
**Examples:**
```
>> ibrio-cli listaddress
<< [{"type":"pubkey","address":"1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq","pubkey":"182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e"},{"type":"template","address":"208043ht3c51qztrdfa0f3349pe2m8ajjw1mdb2py68fbckaa2s24tq55","template":"multisig","templatedata":{"type":"multisig","hex":"0200010282e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052eb9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f4","multisig":{"sigsrequired":1,"addresses":["1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq","1q71vfagprv5hqwckzbvhep0d0ct72j5j2heak2sgp4vptrtc2btdje3q"]}}},{"type":"template","address":"20g0b87qxcd52ceh9zmpzx0hy46pjfzdnqbkh8f4tqs4y0r6sxyzyny25","template":"mint","templatedata":{"type":"mint","hex":"0400f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e801f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e8","mint":{"mint":"1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1","spent":"1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}}}]

>> curl -d '{"id":1,"method":"listaddress","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":0,"jsonrpc":"2.0","result":[{"type":"pubkey","address":"1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq","pubkey":"182e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052e"},{"type":"template","address":"208043ht3c51qztrdfa0f3349pe2m8ajjw1mdb2py68fbckaa2s24tq55","template":"multisig","templatedata":{"type":"multisig","hex":"0200010282e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052eb9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f4","multisig":{"sigsrequired":1,"addresses":["1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq","1q71vfagprv5hqwckzbvhep0d0ct72j5j2heak2sgp4vptrtc2btdje3q"]}}},{"type":"template","address":"20g0b87qxcd52ceh9zmpzx0hy46pjfzdnqbkh8f4tqs4y0r6sxyzyny25","template":"mint","templatedata":{"type":"mint","hex":"0400f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e801f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e8","mint":{"mint":"1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1","spent":"1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}}}]}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### exportwallet
**Usage:**
```
        exportwallet <"path">

Export all of keys and templates from wallet to a specified file in JSON format.
```
**Arguments:**
```
 "path"                                 (string, required) save file path
```
**Request:**
```
 "param" :
 {
   "path": ""                           (string, required) save file path
 }
```
**Response:**
```
 "result": "result"                     (string, required) export result
```
**Examples:**
```
>> ibrio-cli exportwallet /Users/Loading/a.txt
<< Wallet file has been saved at: /Users/Loading/a.txt

>> {"id":4,"method":"exportwallet","jsonrpc":"2.0","params":{"path":"/Users/Loading/a.txt"}}
<< {"id":4,"jsonrpc":"2.0","result":"Wallet file has been saved at: /Users/Loading/a.txt"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid template address"}
* {"code":-401,"message":"Must be an absolute path."}
* {"code":-401,"message":"Cannot export to a folder."}
* {"code":-401,"message":"File has been existed."}
* {"code":-401,"message":"Failed to create directories."}
* {"code":-401,"message":"Failed to export key"}
* {"code":-401,"message":"Unkown template"}
* {"code":-401,"message":"filesystem_error"}
```
##### [Back to top](#commands)
---
### importwallet
**Usage:**
```
        importwallet <"path">

Import keys and templates from an archived file to the wallet in JSON format.
```
**Arguments:**
```
 "path"                                 (string, required) save file path
```
**Request:**
```
 "param" :
 {
   "path": ""                           (string, required) save file path
 }
```
**Response:**
```
 "result": "result"                     (string, required) export result
```
**Examples:**
```
>> ibrio-cli importwallet /Users/Loading/a.txt
<< Imported 0 keys and 0 templates.

>> {"id":5,"method":"importwallet","jsonrpc":"2.0","params":{"path":"/Users/Loading/a.txt"}}
<< {"id":5,"jsonrpc":"2.0","result":"Imported 0 keys and 0 templates."}
```
**Errors:**
```
* {"code":-6,"message":"Invalid parameters,failed to make template"}
* {"code":-401,"message":"Must be an absolute path."}
* {"code":-401,"message":"File has been existed."}
* {"code":-401,"message":"Filesystem_error - failed to read."}
* {"code":-401,"message":"Wallet file exported is invalid, check it and try again."}
* {"code":-401,"message":"Data format is not correct, check it and try again."}
* {"code":-401,"message":"Failed to add key"}
* {"code":-401,"message":"Failed to sync wallet tx"}
* {"code":-401,"message":"Failed to add template"}
* {"code":-32602,"message":"Failed to verify serialized key"}
* {"code":-32602,"message":"Can't import the key with empty passphrase"}
```
##### [Back to top](#commands)
---
### makeorigin
**Usage:**
```
        makeorigin <"prev"> <"owner"> <$amount$> <"name"> <"symbol"> <$reward$> <halvecycle> ("forktype") ({defi}) (-i|-noi*isolated*) (-p|-nop*private*) (-e|-noe*enclosed*)

Return hex-encoded block.
```
**Arguments:**
```
 "prev"                                 (string, required) prev block hash
 "owner"                                (string, required) owner address
 $amount$                               (double, required) amount
 "name"                                 (string, required) unique fork name
 "symbol"                               (string, required) fork symbol
 $reward$                               (double, required) mint reward
 halvecycle                             (uint, required) halve cycle: 0: fixed reward, >0: blocks of halve cycle
 "forktype"                             (string, optional, default="") fork type. 'defi': DeFi fork, other: common fork
  (if forktype=defi)
 {defi}                                 (object, optional) Fork DeFi Parameters
 -i|-noi*isolated*                      (bool, optional, default=true) is isolated
 -p|-nop*private*                       (bool, optional, default=false) is private
 -e|-noe*enclosed*                      (bool, optional, default=false) is enclosed
```
**Request:**
```
 "param" :
 {
   "prev": "",                          (string, required) prev block hash
   "owner": "",                         (string, required) owner address
   "amount": 0.0,                       (double, required) amount
   "name": "",                          (string, required) unique fork name
   "symbol": "",                        (string, required) fork symbol
   "reward": 0.0,                       (double, required) mint reward
   "halvecycle": 0,                     (uint, required) halve cycle: 0: fixed reward, >0: blocks of halve cycle
   "forktype": "",                      (string, optional, default="") fork type. 'defi': DeFi fork, other: common fork
   (if forktype=defi)
   "defi":                              (object, optional) Fork DeFi Parameters
   {
     "rewardcycle": 0,                  (int, required) generate reward cycle in height
     "supplycycle": 0,                  (int, required) supplyment changing cycle in height
     "mintheight": 0,                   (int, optional, default=-1) beginning mint height of DeFi, 0 means the first block after origin, -1 means not confirmed mint height(mint height depends on mint height tx of owner)
     "maxsupply": 0,                    (int, optional, default=-1) the max DeFi supply in this fork, -1 means no upper limit
     "coinbasetype": 0,                 (uint, optional, default=0) coinbase type. 0 - fixed decay(related to 'nInitCoinbasePercent', 'nCoinbaseDecayPercent', 'nDecayCycle'). 1 - specific decay(related to 'mapCoinbasePercent')
     "decaycycle": 0,                   (int, optional, default=0) coinbase decay cycle in height
     "mapcoinbasepercent":              (array, optional) pairs of height - coinbase percent
     [
       {
         "height": 0,                   (int, required) begin height
         "percent": 0                   (uint, required) coinbase increasing ratio [0 - 100]
       }
     ]
     "coinbasedecaypercent": 0,         (uint, optional, default=50) compared with previous decay cycle, coinbase increasing ratio(%), 50 means decay 50%
     "initcoinbasepercent": 0,          (uint, optional, default=0) coinbase increasing ratio(%) per supply cycle in initialization
     "stakerewardpercent": 0,           (uint, optional, default=50) stake reward ratio, [0 - 100] means [0% - 100%]
     "promotionrewardpercent": 0,       (uint, optional, default=50) promotion reward ratio, [0 - 100] means [0% - 100%]
     "stakemintoken": 0,                (uint, optional, default=0) the minimum token on address can participate stake reward
     "mappromotiontokentimes":          (array, optional) in promotion computation, less than or equal to [key] amount should multiply [value] eg. [{"key": 10, "value": 5},...]
     [
       {
         "token": 0,                    (uint, required) promotion token
         "times": 0                     (uint, required) promotion times
       }
     ]
   }
   "isolated": true|false,              (bool, optional, default=true) is isolated
   "private": true|false,               (bool, optional, default=false) is private
   "enclosed": true|false               (bool, optional, default=false) is enclosed
 }
```
**Response:**
```
 "result" :
 {
   "hash": "",                          (string, required) block hash
   "hex": ""                            (string, required) block data hex
 }
```
**Examples:**
```
>> ibrio-cli makeorigin a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0 1p2e0yjz5y1ga705csrzpsp1wre1vjqepkbwnpchyx2vay86wf01pz3qs 1500 test-fork POW 1.0 0
<< {"hash" : "327ec1ffabeae75295401ec69591f845e6025c24d31ae07d9f6e9dec3462bd7a","hex" : "010000ffc06f585ac0afdc176980a0a1ebe849d09b51e4b4c1dd46fb939579d71bdc55809d6f3da600000000000000000000000000000000000000000000000000000000000000004c8001000000e109746573742d666f726be203504f574301a46400000000000000a540420f0000000000e62101b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803010000010000000000000000000000000000000000000000000000000000000000000000000000000001b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803002f685900000000000000000000000009746573742d666f726b000040f253de866a5357b5a11e493162885f4b8a34e94279e29b8354b237aaec650af420b61d24721b4e5e3a4096e9370c77f7dad6c05ced6eedcb2229958671620c05"}

>> curl -d '{"id":4,"method":"makeorigin","jsonrpc":"2.0","params":{"prev":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","owner":"1p2e0yjz5y1ga705csrzpsp1wre1vjqepkbwnpchyx2vay86wf01pz3qs","amount":1500,"name":"test-fork","symbol":"POW","reward":1,"halvecycle":0,"isolated":true,"private":false,"enclosed":false}}' http://127.0.0.1:6602
<< {"id":4,"jsonrpc":"2.0","result":{"hash":"327ec1ffabeae75295401ec69591f845e6025c24d31ae07d9f6e9dec3462bd7a","hex":"010000ffc06f585ac0afdc176980a0a1ebe849d09b51e4b4c1dd46fb939579d71bdc55809d6f3da600000000000000000000000000000000000000000000000000000000000000004c8001000000e109746573742d666f726be203504f574301a46400000000000000a540420f0000000000e62101b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803010000010000000000000000000000000000000000000000000000000000000000000000000000000001b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803002f685900000000000000000000000009746573742d666f726b000040f253de866a5357b5a11e493162885f4b8a34e94279e29b8354b237aaec650af420b61d24721b4e5e3a4096e9370c77f7dad6c05ced6eedcb2229958671620c05"}}

>> ibrio-cli makeorigin a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0 1p2e0yjz5y1ga705csrzpsp1wre1vjqepkbwnpchyx2vay86wf01pz3qs 1500 test-fork POW 1.0 0 -i -nop -e
<< {"hash":"ade2a8f61082cc0b8e98581e013c52126989d5c15e5c4c6b882f76c98bc68025","hex":"010000ffc06f585ac0afdc176980a0a1ebe849d09b51e4b4c1dd46fb939579d71bdc55809d6f3da600000000000000000000000000000000000000000000000000000000000000004c8001000000e109746573742d666f726be203504f574305a46400000000000000a540420f0000000000e62101b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803010000010000000000000000000000000000000000000000000000000000000000000000000000000001b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803002f685900000000000000000000000009746573742d666f726b00004078bbc21e237d727a58de54e14a19d0244e9dd27ce7d55dfca891158913068185f3dc5b98d4e11500b7512bc8d0e8aa136b34f22f16c16be45e9650e15253980e"}

>> {"id":2,"method":"makeorigin","jsonrpc":"2.0","params":{"prev":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0","owner":"1p2e0yjz5y1ga705csrzpsp1wre1vjqepkbwnpchyx2vay86wf01pz3qs","amount":1500,"name":"test-fork","symbol":"POW","reward":1,"halvecycle":0,"isolated":true,"private":false,"enclosed":true}}
<< {"id":2,"jsonrpc":"2.0","result":{"hash":"ade2a8f61082cc0b8e98581e013c52126989d5c15e5c4c6b882f76c98bc68025","hex":"010000ffc06f585ac0afdc176980a0a1ebe849d09b51e4b4c1dd46fb939579d71bdc55809d6f3da600000000000000000000000000000000000000000000000000000000000000004c8001000000e109746573742d666f726be203504f574305a46400000000000000a540420f0000000000e62101b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803010000010000000000000000000000000000000000000000000000000000000000000000000000000001b09c0f4be5f060a380acce3f6cd83cc383b95dd69af95b323ee8b6af20dc7803002f685900000000000000000000000009746573742d666f726b00004078bbc21e237d727a58de54e14a19d0244e9dd27ce7d55dfca891158913068185f3dc5b98d4e11500b7512bc8d0e8aa136b34f22f16c16be45e9650e15253980e"}}

>> makeorigin 0000000006854ebdc236f48dbbe5c87312ea0abd7398888374b5ee9a5eb1d291 1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm 1500 test-fork POW 0 0 defi  '{"mintheight":100, "maxsupply": 2100000000,  "decaycycle": 12, "decaypercent":13, "coinbasedecaypercent": 11, "initcoinbasepercent": 15, "rewardcycle": 20, "supplycycle": 4, "stakerewardpercent": 26, "promotionrewardpercent": 21, "stakemintoken": 11, "mappromotiontokentimes": [{"token": 10, "times": 5}]}' -i -e -p
<< {"hash" : "00000001bcc08068abae3260bacaab4086618a8a91f0f701890e240f037c4eff","hex" : "010000ff5a4b585f91d2b15e9aeeb57483889873bd0aea1273c8e5bb8df436c2bd4e8506000000000000000000000000000000000000000000000000000000000000000000000000cb8001000000e109746573742d666f726be203504f574307a4002f685900000000a50000000000000000a610270000000000008700000000e82101498b63009dfb70f7ee0902ba95cc171f7d7a97ff16d89fd96e1f1b9e7d5f91dac991d2b15e9aeeb57483889873bd0aea1273c8e5bb8df436c2bd4e8506000000008a000000008b01000000ec44640000000040075af0750700000f0000000b0c000000000000000000000014000000040000001a15c0d8a7000000000001000000000000000a0000000000000005000000010000015a4b585f0000000000000000000000000000000000000000000000000000000000000000000000000001498b63009dfb70f7ee0902ba95cc171f7d7a97ff16d89fd96e1f1b9e7d5f91da002f685900000000000000000000000009746573742d666f726b000040daa30a03122ae5429a0f2e065cc247ef01474581629f1ffbde5ced5c840471a8a5d9614cbae20081f02411f21741a7fd595ce3fdfb658aa115988f9e672fd40f"}
```
**Errors:**
```
* {"code":-4,"message":"Owner' address should be pubkey address"}
* {"code":-4,"message":"Unknown key"}
* {"code":-6,"message":"Unknown prev block"}
* {"code":-6,"message":"Invalid owner"}
* {"code":-6,"message":"Invalid amount"}
* {"code":-6,"message":"Invalid name or symbol"}
* {"code":-6,"message":"Prev block should not be extended/vacant block"}
* {"code":-405,"message":"Key is locked"}
* {"code":-401,"message":"Failed to sign message"}
```
##### [Back to top](#commands)
---
### verifymessage
**Usage:**
```
        verifymessage <"pubkey"> <"message"> <"sig"> (-a="addr") (-p|-nop*hasprefix*)

Verify a signed message
```
**Arguments:**
```
 "pubkey"                               (string, required) public key
 "message"                              (string, required) message to be verified
 "sig"                                  (string, required) sign
 -a="addr"                              (string, optional) address signed
 -p|-nop*hasprefix*                     (bool, optional, default=true) If add magic string(Ibrio Signed Message:
) before message or not
```
**Request:**
```
 "param" :
 {
   "pubkey": "",                        (string, required) public key
   "message": "",                       (string, required) message to be verified
   "sig": "",                           (string, required) sign
   "addr": "",                          (string, optional) address signed
   "hasprefix": true|false              (bool, optional, default=true) If add magic string(Ibrio Signed Message:
) before message or not
 }
```
**Response:**
```
 "result": result                       (bool, required) message verified result.
```
**Examples:**
```
>> ibrio-cli verifymessage 2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882 123456 045977f8c07e6d846d6055357f36a70c16c071cb85115e3ffb498e171a9ac3f4aed1292203a0c8e42c4becafad3ced0d9874abd2a8b788fda9f07099a1e71707
<< true

>> curl -d '{"id":5,"method":"verifymessage","jsonrpc":"2.0","params":{"pubkey":"2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882","message":"123456","sig":"045977f8c07e6d846d6055357f36a70c16c071cb85115e3ffb498e171a9ac3f4aed1292203a0c8e42c4becafad3ced0d9874abd2a8b788fda9f07099a1e71707"}}' http://127.0.0.1:6602
<< {"id":5,"jsonrpc":"2.0","result":true}

>> ibrio-cli verifymessage 2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882 12345 045977f8c07e6d846d6055357f36a70c16c071cb85115e3ffb498e171a9ac3f4aed1292203a0c8e42c4becafad3ced0d9874abd2a8b788fda9f07099a1e71707
<< false

>> curl -d '{"id":6,"method":"verifymessage","jsonrpc":"2.0","params":{"pubkey":"2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882","message":"12345","sig":"045977f8c07e6d846d6055357f36a70c16c071cb85115e3ffb498e171a9ac3f4aed1292203a0c8e42c4becafad3ced0d9874abd2a8b788fda9f07099a1e71707"}}' http://127.0.0.1:6602
<< {"id":6,"jsonrpc":"2.0","result":false}
```
**Errors:**
```
* {"code":-6,"message":"Invalid pubkey"}
* {"code":-6,"message":"Invalid message"}
* {"code":-6,"message":"Invalid sig"}
```
##### [Back to top](#commands)
---
### makekeypair
**Usage:**
```
        makekeypair

Make a public/private key pair.
```
**Arguments:**
```
	none
```
**Request:**
```
 "param" : {}
```
**Response:**
```
 "result" :
 {
   "privkey": "",                       (string, required) private key
   "pubkey": ""                         (string, required) public key
 }
```
**Examples:**
```
>> ibrio-cli makekeypair
<< {"privkey":"833a5d51d2db84debc0eb3a40d7d41b2723452d211d7e81ce489a95ef48b2324","pubkey":"e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9"}

>> curl -d '{"id":42,"method":"makekeypair","jsonrpc":"2.0","params":{}}' http://127.0.0.1:6602
<< {"id":42,"jsonrpc":"2.0","result":{"privkey":"833a5d51d2db84debc0eb3a40d7d41b2723452d211d7e81ce489a95ef48b2324","pubkey":"e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9"}}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### getpubkey
**Usage:**
```
        getpubkey <"privkeyaddress">

Return public key by address or private key.
```
**Arguments:**
```
 "privkeyaddress"                       (string, required) address or private key
```
**Request:**
```
 "param" :
 {
   "privkeyaddress": ""                 (string, required) address or private key
 }
```
**Response:**
```
 "result": "pubkey"                     (string, required) public key
```
**Examples:**
```
>> ibrio-cli getpubkey 15c02b5f9eb6e516159c230011a87e57757645b53d3534958f910c08feb5c203
<< 06c4246621002576ec70545f04f2cb75378e3f1a16eca2c596fc1c64f52e122b

>> curl -d '{"id":44,"method":"getpubkey","jsonrpc":"2.0","params":{"privkey":"15c02b5f9eb6e516159c230011a87e57757645b53d3534958f910c08feb5c203"}}' http://127.0.0.1:6602
<< {"id":44,"jsonrpc":"2.0","result":"06c4246621002576ec70545f04f2cb75378e3f1a16eca2c596fc1c64f52e122b"}

>> ibrio-cli getpubkey 1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1
<< e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9

>> curl -d '{"id":44,"method":"getpubkey","jsonrpc":"2.0","params":{"privkey":"1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}}' http://127.0.0.1:6602
<< {"id":44,"jsonrpc":"2.0","result":"e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid pubkey"}
```
##### [Back to top](#commands)
---
### getpubkeyaddress
**Usage:**
```
        getpubkeyaddress <"pubkey">

Return encoded address for the given public key.
```
**Arguments:**
```
 "pubkey"                               (string, required) public key
```
**Request:**
```
 "param" :
 {
   "pubkey": ""                         (string, required) public key
 }
```
**Response:**
```
 "result": "address"                    (string, required) address of public key
```
**Examples:**
```
>> ibrio-cli getpubkeyaddress e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9
<< 1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1

>> curl -d '{"id":44,"method":"getpubkeyaddress","jsonrpc":"2.0","params":{"pubkey":"e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9"}}' http://127.0.0.1:6602
<< {"id":44,"jsonrpc":"2.0","result":"1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid pubkey"}
```
##### [Back to top](#commands)
---
### gettemplateaddress
**Usage:**
```
        gettemplateaddress <"tid">

Return encoded address for the given template id.
```
**Arguments:**
```
 "tid"                                  (string, required) template id
```
**Request:**
```
 "param" :
 {
   "tid": ""                            (string, required) template id
 }
```
**Response:**
```
 "result": "address"                    (string, required) address of template
```
**Examples:**
```
>> ibrio-cli gettemplateaddress 2040fpytdr4k5h8tk0nferr7zb51tkccrkgqf341s6tg05q9xe6hth1m4
<< 21w2040000000000000000000000000000000000000000000000epcek

>> curl -d '{"id":53,"method":"gettemplateaddress","jsonrpc":"2.0","params":{"tid":"2040fpytdr4k5h8tk0nferr7zb51tkccrkgqf341s6tg05q9xe6hth1m4"}}' http://127.0.0.1:6602
<< {"id":53,"jsonrpc":"2.0","result":"21w2040000000000000000000000000000000000000000000000epcek"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid tid"}
```
##### [Back to top](#commands)
---
### maketemplate
**Usage:**
```
        maketemplate <"type"> <{delegate}>|<{vote}>|<{fork}>|<{mint}>|<{multisig}>|<{weighted}>|<{exchange}>|<{payment}>|<{dexorder}>|<{dexmatch}>|<{activate}> (*synctx*)

Return encoded address for the given template id.
```
**Arguments:**
```
 "type"                                 (string, required) template type
  (if type=delegate)
 {delegate}                             (object, required) a delegate template
  (if type=vote)
 {vote}                                 (object, required) a vote template
  (if type=fork)
 {fork}                                 (object, required) a new fork template
  (if type=mint)
 {mint}                                 (object, required) a mint template
  (if type=multisig)
 {multisig}                             (object, required) a multiple sign template
  (if type=weighted)
 {weighted}                             (object, required) a weighted multiple sign template
  (if type=exchange)
 {exchange}                             (object, required) a exchange template
  (if type=payment)
 {payment}                              (object, required) a payment template
  (if type=dexorder)
 {dexorder}                             (object, required) a dex order template
  (if type=dexmatch)
 {dexmatch}                             (object, required) a dex match template
  (if type=activate)
 {activate}                             (object, required) a activate template
 *synctx*                               (bool, optional, default=false) sync tx or not
```
**Request:**
```
 "param" :
 {
   "type": "",                          (string, required) template type
   (if type=delegate)
   "delegate":                          (object, required) a delegate template
   {
     "delegate": "",                    (string, required) delegate public key
     "owner": ""                        (string, required) owner address
   }
   (if type=vote)
   "vote":                              (object, required) a vote template
   {
     "delegate": "",                    (string, required) delegate template address
     "owner": "",                       (string, required) owner address
     "cycle": 0,                        (uint, required) cycle
     "nonce": 0                         (uint, required) nonce
   }
   (if type=fork)
   "fork":                              (object, required) a new fork template
   {
     "redeem": "",                      (string, required) redeem address
     "fork": ""                         (string, required) fork hash
   }
   (if type=mint)
   "mint":                              (object, required) a mint template
   {
     "mint": "",                        (string, required) mint public key
     "spent": ""                        (string, required) spent address
   }
   (if type=multisig)
   "multisig":                          (object, required) a multiple sign template
   {
     "required": 0,                     (int, required) required weight > 0
     "pubkeys":                         (array, required, default=RPCValid) 
     [
       "key": ""                        (string, required) public key
     ]
   }
   (if type=weighted)
   "weighted":                          (object, required) a weighted multiple sign template
   {
     "required": 0,                     (int, required) required weight
     "pubkeys":                         (array, required, default=RPCValid) public keys
     [
       {
         "key": "",                     (string, required) public key
         "weight": 0                    (int, required) weight
       }
     ]
   }
   (if type=exchange)
   "exchange":                          (object, required) a exchange template
   {
     "addr_m": "",                      (string, required) addr_m
     "addr_s": "",                      (string, required) addr_s
     "height_m": 0,                     (int, required) height_m
     "height_s": 0,                     (int, required) height_s
     "fork_m": "",                      (string, required) fork_m hash
     "fork_s": ""                       (string, required) fork_s hash
   }
   (if type=payment)
   "payment":                           (object, required) a payment template
   {
     "business": "",                    (string, required) business
     "customer": "",                    (string, required) customer
     "height_exec": 0,                  (int, required) height_exec
     "height_end": 0,                   (int, required) height_end
     "amount": 0,                       (int, required) amount
     "pledge": 0                        (int, required) pledge
   }
   (if type=dexorder)
   "dexorder":                          (object, required) a dex order template
   {
     "seller_address": "",              (string, required) seller address
     "coinpair": "",                    (string, required) coin pair
     "price": 0.0,                      (double, required) price
     "fee": 0.0,                        (double, required) fee
     "recv_address": "",                (string, required) receive address
     "valid_height": 0,                 (int, required) valid height
     "match_address": "",               (string, required) match address
     "deal_address": "",                (string, required) deal address
     "timestamp": 0                     (uint, required) timestamp
   }
   (if type=dexmatch)
   "dexmatch":                          (object, required) a dex match template
   {
     "match_address": "",               (string, required) match address
     "coinpair": "",                    (string, required) coin pair
     "final_price": 0.0,                (double, required) final price
     "match_amount": 0.0,               (double, required) match amount
     "fee": 0.0,                        (double, required) fee
     "secret_hash": "",                 (string, required) sha256 hash
     "secret_enc": "",                  (string, required) secret encryption
     "seller_order_address": "",        (string, required) seller order address
     "seller_address": "",              (string, required) seller address
     "seller_deal_address": "",         (string, required) seller deal address
     "seller_valid_height": 0,          (int, required) seller valid height
     "buyer_address": "",               (string, required) buyer address
     "buyer_amount": "",                (string, required) buyer match amount
     "buyer_secret_hash": "",           (string, required) buyer secret hash
     "buyer_valid_height": 0            (int, required) buyer valid height
   }
   (if type=activate)
   "activate":                          (object, required) a activate template
   {
     "inviter": "",                     (string, required) inviter address
     "owner": ""                        (string, required) owner address
   }
   "synctx": true|false                 (bool, optional, default=false) sync tx or not
 }
```
**Response:**
```
 "result" :
 {
   "address": "",                       (string, required) address of template
   "hex": ""                            (string, required) template data hex string
 }
```
**Examples:**
```
>> ibrio-cli maketemplate multisig '{"required": 1, "pubkeys": ["2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882", "f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9"]}'
<< {"address":"208043ht3c51qztrdfa0f3349pe2m8ajjw1mdb2py68fbckaa2s24tq55","hex":"0200010282e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052eb9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f4"}

>> curl -d '{"id":54,"method":"maketemplate","jsonrpc":"2.0","params":{"type":"multisig","multisig":{"required":1,"pubkeys":["2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882","f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9"]}}}' http://127.0.0.1:6602
<< {"id":54,"jsonrpc":"2.0","result":{"address":"208043ht3c51qztrdfa0f3349pe2m8ajjw1mdb2py68fbckaa2s24tq55","hex":"0200010282e8a36441d116ce7a97f9a216d43a3dfc4280295874007b8ff5fd45eec9052eb9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f4"}}

>> ibrio-cli maketemplate delegate '{"delegate":"2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882","owner":"1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq"}'

>> ibrio-cli maketemplate fork '{"redeem":"1gbma6s21t4bcwymqz6h1dn1t7qy45019b1t00ywfyqymbvp90mqc1wmq","fork":"a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0"}'

>> ibrio-cli maketemplate mint '{"mint": "e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9", "spent": "1z6taz5dyrv2xa11pc92y0ggbrf2wf36gbtk8wjprb96qe3kqwfm3ayc1"}'

>> ibrio-cli maketemplate weighted '{"required": 1, "pubkeys": [{"key":"2e05c9ee45fdf58f7b007458298042fc3d3ad416a2f9977ace16d14164a3e882", "weight": 1},{"key": "f4124c636d37b1308ba95c14b2487134030d5817f7fa93f11bcbc616aab7c3b9", "weight": 2}]}'
```
**Errors:**
```
* {"code":-6,"message":"Invalid parameters, failed to make template"}
* {"code":-6,"message":"Invalid parameter, missing weight"}
* {"code":-6,"message":"Invalid parameter, missing redeem address"}
* {"code":-6,"message":"Invalid parameter, missing spent address"}
* {"code":-6,"message":"Invalid parameter, missing owner address"}
* {"code":-6,"message":"template type error. type: xxx"}
```
##### [Back to top](#commands)
---
### decodetransaction
**Usage:**
```
        decodetransaction <"txdata">

Return a JSON object representing the serialized, hex-encoded transaction.
```
**Arguments:**
```
 "txdata"                               (string, required) transaction raw data
```
**Request:**
```
 "param" :
 {
   "txdata": ""                         (string, required) transaction raw data
 }
```
**Response:**
```
 "result" :
 {
   "txid": "",                          (string, required) transaction hash
   "version": 0,                        (uint, required) version
   "type": "",                          (string, required) transaction type
   "time": 0,                           (uint, required) transaction timestamp
   "lockuntil": 0,                      (uint, required) unlock time
   "anchor": "",                        (string, required) anchor hash
   "blockhash": "",                     (string, required) which block tx located in
   "vin":                               (array, required, default=RPCValid) vin list
   [
     {
       "txid": "",                      (string, required) pre-vout transaction hash
       "vout": 0                        (uint, required) pre-vout number
     }
   ]
   "sendfrom": "",                      (string, required) send from address
   "sendto": "",                        (string, required) send to address
   "amount": 0.0,                       (double, required) amount
   "txfee": 0.0,                        (double, required) transaction fee
   "data": "",                          (string, required) data
   "sig": "",                           (string, required) sign
   "fork": "",                          (string, required) fork hash
   "confirmations": 0                   (int, optional) confirmations
 }
```
**Examples:**
```
>> ibrio-cli decodetransaction 010000003190f05d00000000701af4705c5e6fcb04efc3ca3c851c1e4d8948e10923025f54bea9b000000000017a792b71cdb0f662c802207d849e73c504b7a87096c6d80479605a2a9188f05d0001a4dce88fee73a59dcebf2b40ba1d13407254164b9166f063b2e8d513561118d620e4ab0900000000a0860100000000000040d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000
<< {"txid":"5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03","version":1,"type":"token","time":1576046641,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df088912a5a607904d8c69670a8b704c5739e847d2002c862f6b0cd712b797a","vout":0}],"sendfrom":"1n56xmva8131c4q0961anv5wdzfqtdctyg9e53fxp65f1jyhbtkfbxz6q","sendto":"1mkeeh3zeeejsvknz5d0bm78k81s585jbj5kf0rxjx3ah6ngh33b1erg9","amount":162.260000,"txfee":0.100000,"data":"","sig":"d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70"}

>> curl -d '{"id":1,"method":"decodetransaction","jsonrpc":"2.0","params":{"txdata":"010000003190f05d00000000701af4705c5e6fcb04efc3ca3c851c1e4d8948e10923025f54bea9b000000000017a792b71cdb0f662c802207d849e73c504b7a87096c6d80479605a2a9188f05d0001a4dce88fee73a59dcebf2b40ba1d13407254164b9166f063b2e8d513561118d620e4ab0900000000a0860100000000000040d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000"}}' http://127.0.0.1:6602
<< {"id":1,"jsonrpc":"2.0","result":{"txid":"5df09031322f99db08a4747d652e0733f60c9b523a6a489b5f72e0031a2b2a03","version":1,"type":"token","time":1576046641,"lockuntil":0,"anchor":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70","vin":[{"txid":"5df088912a5a607904d8c69670a8b704c5739e847d2002c862f6b0cd712b797a","vout":0}],"sendfrom":"1n56xmva8131c4q0961anv5wdzfqtdctyg9e53fxp65f1jyhbtkfbxz6q","sendto":"1mkeeh3zeeejsvknz5d0bm78k81s585jbj5kf0rxjx3ah6ngh33b1erg9","amount":162.260000,"txfee":0.100000,"data":"","sig":"d6594d9215c58224f9707a84f773b00394561df0bba769a279dd065b98bc03a946928f8a6508a728eceff3e22d5181da16b78087c79b68532b31553bdc855000","fork":"00000000b0a9be545f022309e148894d1e1c853ccac3ef04cb6f5e5c70f41a70"}}
```
**Errors:**
```
* {"code":-8,"message":"TX decode failed"}
* {"code":-6,"message":"Unknown anchor block"}
```
##### [Back to top](#commands)
---
### gettxfee
**Usage:**
```
        gettxfee <"hexdata">

Return TxFee for vchData Hex data
```
**Arguments:**
```
 "hexdata"                              (string, required) Hex encoded vchData in transaction
```
**Request:**
```
 "param" :
 {
   "hexdata": ""                        (string, required) Hex encoded vchData in transaction
 }
```
**Response:**
```
 "result": txfee                        (double, required) transaction Fee
```
**Examples:**
```
>> ibrio-cli gettxfee feeda13f124bbc
<< 0.01

>> curl -d '{"id":1,"method":"gettxfee","jsonrpc":"2.0","params":{"hexdata":"feeda13f124bbc"}}' http://127.0.0.1:6602
<< {"id":1,"jsonrpc":"2.0","result":"0.01"}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### makesha256
**Usage:**
```
        makesha256 (-h="hexdata")

Make sha256
```
**Arguments:**
```
 -h="hexdata"                           (string, optional) Hex encoded data
```
**Request:**
```
 "param" :
 {
   "hexdata": ""                        (string, optional) Hex encoded data
 }
```
**Response:**
```
 "result" :
 {
   "hexdata": "",                       (string, required) Hex encoded data
   "sha256": ""                         (string, required) Sha256 string
 }
```
**Examples:**
```
>> ibrio-cli makesha256
<< {"hexdata" : "381cdfe25a6fde17ee1b9fc905e30c832d5bb981405fdae15a3acb1c420b2f5e", "sha256" : "5e2ab85020823529d3ee2d8c4c41af4d10c205aa8cb45b139d0e7054e73c11d4"}

>> curl -d '{"id":1,"method":"makesha256","jsonrpc":"2.0","params":{"hexdata":"54ec6b4a77eadc39207697e816e2d2cc6881263ecab078a4c770228e11cbff99"}}' http://127.0.0.1:6602
<< {"id":1,"jsonrpc":"2.0","result":{"hexdata" : "381cdfe25a6fde17ee1b9fc905e30c832d5bb981405fdae15a3acb1c420b2f5e", "sha256" : "5e2ab85020823529d3ee2d8c4c41af4d10c205aa8cb45b139d0e7054e73c11d4"}}
```
**Errors:**
```
	none
```
##### [Back to top](#commands)
---
### aesencrypt
**Usage:**
```
        aesencrypt <"localaddress"> <"remoteaddress"> <"message">

aes encrypt
```
**Arguments:**
```
 "localaddress"                         (string, required) local address
 "remoteaddress"                        (string, required) remote address
 "message"                              (string, required) message
```
**Request:**
```
 "param" :
 {
   "localaddress": "",                  (string, required) local address
   "remoteaddress": "",                 (string, required) remote address
   "message": ""                        (string, required) message
 }
```
**Response:**
```
 "result": "result"                     (string, required) ciphertext
```
**Examples:**
```
>> ibrio-cli aesencrypt 1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm 1jv78wjv22hmzcwv07bkkphnkj51y0kjc7g9rwdm05erwmr2n8tvh8yjn 381cdfe25a6fde17ee1b9fc905e30c832d5bb981405fdae15a3acb1c420b2f5e
<< 06839cf364696762f980640a618d49981a8ab103306605336881eaa112e3693e99325fdd73b9052253de02107e4a373c

>> curl -d '{"id":1,"method":"aesencrypt","jsonrpc":"2.0","params":{"localaddress":"1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm","remoteaddress":"1jv78wjv22hmzcwv07bkkphnkj51y0kjc7g9rwdm05erwmr2n8tvh8yjn","message":"381cdfe25a6fde17ee1b9fc905e30c832d5bb981405fdae15a3acb1c420b2f5e"}}' http://127.0.0.1:6602
<< {"id":1,"jsonrpc":"2.0","result":"06839cf364696762f980640a618d49981a8ab103306605336881eaa112e3693e99325fdd73b9052253de02107e4a373c"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid local address."}
* {"code":-6,"message":"Invalid remote address."}
* {"code":-6,"message":"Invalid message."}
* {"code":-401,"message":"Encrypt fail."}
```
##### [Back to top](#commands)
---
### aesdecrypt
**Usage:**
```
        aesdecrypt <"localaddress"> <"remoteaddress"> <"ciphertext">

aes decrypt
```
**Arguments:**
```
 "localaddress"                         (string, required) local address
 "remoteaddress"                        (string, required) remote address
 "ciphertext"                           (string, required) ciphertext
```
**Request:**
```
 "param" :
 {
   "localaddress": "",                  (string, required) local address
   "remoteaddress": "",                 (string, required) remote address
   "ciphertext": ""                     (string, required) ciphertext
 }
```
**Response:**
```
 "result": "result"                     (string, required) message
```
**Examples:**
```
>> ibrio-cli aesdecrypt 1jv78wjv22hmzcwv07bkkphnkj51y0kjc7g9rwdm05erwmr2n8tvh8yjn 1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm 06839cf364696762f980640a618d49981a8ab103306605336881eaa112e3693e99325fdd73b9052253de02107e4a373c
<< 381cdfe25a6fde17ee1b9fc905e30c832d5bb981405fdae15a3acb1c420b2f5e

>> curl -d '{"id":1,"method":"aesdecrypt","jsonrpc":"2.0","params":{"localaddress":"1jv78wjv22hmzcwv07bkkphnkj51y0kjc7g9rwdm05erwmr2n8tvh8yjn","remoteaddress":"1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm","ciphertext":"06839cf364696762f980640a618d49981a8ab103306605336881eaa112e3693e99325fdd73b9052253de02107e4a373c"}}' http://127.0.0.1:6602
<< {"id":1,"jsonrpc":"2.0","result":"381cdfe25a6fde17ee1b9fc905e30c832d5bb981405fdae15a3acb1c420b2f5e"}
```
**Errors:**
```
* {"code":-6,"message":"Invalid local address."}
* {"code":-6,"message":"Invalid remote address."}
* {"code":-6,"message":"Invalid ciphertext."}
* {"code":-401,"message":"Decrypt fail."}
```
##### [Back to top](#commands)
---
### listunspent
**Usage:**
```
        listunspent <"address"> (-f="fork") (-n=max) (-a=$amount$) (-if="file")

Return a JSON object listing unspent utxo by user specifying arguments address.
```
**Arguments:**
```
 "address"                              (string, required) address to receive the unspent
 -f="fork"                              (string, optional) fork hash
 -n=max                                 (uint, optional, default=10) maximum unspents(0 means unlimited)
 -a=$amount$                            (double, optional, default=0.0) amount unspents(0 means unlimited)
 -if="file"                             (string, optional) input file containing list of multiple addresses
```
**Request:**
```
 "param" :
 {
   "address": "",                       (string, required) address to receive the unspent
   "fork": "",                          (string, optional) fork hash
   "max": 0,                            (uint, optional, default=10) maximum unspents(0 means unlimited)
   "amount": 0.0,                       (double, optional, default=0.0) amount unspents(0 means unlimited)
   "file": ""                           (string, optional) input file containing list of multiple addresses
 }
```
**Response:**
```
 "result" :
 {
   "addresses":                         (array, required, default=RPCValid) 
   [
     {
       "address": "",                   (string, required) address to list unspent
       "unspents":                      (array, required, default=RPCValid) unspent list of address owner
       [
         {
           "txid": "",                  (string, required) txid
           "out": 0,                    (uint, required) tx output point
           "amount": 0.0,               (double, required) amount
           "height": 0,                 (int, required) height
           "time": 0,                   (uint, required) time transaction made
           "lockuntil": 0               (uint, required) lockuntil
         }
       ]
       "sum": 0.0                       (double, required) sum of unspent amount
     }
   ]
   "total": 0.0                         (double, required) total of unspent amount for list of addresses
 }
```
**Examples:**
```
>> ibrio-cli listunspent 1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm -f=00000000beb6f9e2a8813f4778b26820a8649894de63624e65bc0ffa9562730b -n=1
<< {"unspents":[{"txid":"5dd4bb56b3d0aed9bb0e0a80d01ec1f70d7bd4d1fc200a48df330154b366ca8e","out":0,"amount":1000.000000,"time":1574222678,"lockuntil":0}],"sum":1000.000000}

>> ibrio-cli listunspent 20g075m4mh5trbkdy8vbh74n9h7t3npe7tewpnvsfb19p57jnyf3kdh45 -if=/home/oijen/addr10
<< {"addresses":[{"address":"1zbyhm9p0aa1wvebg3wch7xkhxsv5m9z32jsnjngpfnf7cgd778st8tds","unspents":[{"txid":"5de8bcce65733bbcfff18130de6f3f10cfd46ce94aa4775070f1634777029401","out":0,"amount":340.999900,"time":1575533774,"lockuntil":0},{"txid":"5e0423aadb07b4142abcf29e576e56d0eff594d5da18f6915155c1ece4c66309","out":0,"amount":11.999900,"time":1577329578,"lockuntil":0},{"txid":"5df7b69db367bfc7a9b40fea36e2459f99e214db9505a44f9dfa9052e1d6f411","out":0,"amount":10.999900,"time":1576515229,"lockuntil":0}],"sum":363.999700},{"address":"1z98n1sfbhtw3b5wrre69fh648e4res1g7r9bzcp0kf85hte75mv0zc01","unspents":[],"sum":0.000000},{"address":"1tbvg7cxbqg36rph6ymws856ms4j5e4cvbd4ssy0ejxn1h869g9hfwv4c","unspents":[],"sum":0.000000},{"address":"1j42adjfq37j9grmwsh4gnayfgg6kep59d78srpxg8zf8hdqv1yptjwzx","unspents":[{"txid":"5df58dcccb52641f3b9c090ba5760181a3999370137cc6b5bbacc30b7b4b5909","out":0,"amount":499.999900,"time":1576373708,"lockuntil":0},{"txid":"5df80dad00f53331757e19fae811308ec59d722ffb189958ecdb7cc4ad79b30d","out":0,"amount":499.999900,"time":1576537517,"lockuntil":0},{"txid":"5df7a4344d2d887c929cf6d08bf825bfacc14d8c86cf62ab3944caa142bc6c0f","out":0,"amount":499.999900,"time":1576510516,"lockuntil":0}],"sum":1499.999700},{"address":"1ztsgg3m6az2sm8t6ap6k3cw36z5ewd5sp05k7a62h6rqvpqp871aqpq6","unspents":[],"sum":0.000000},{"address":"1ny00y8whrser450hfw3d87h1p3emdeca4fq5xetddwfz9mb4e38ndxx0","unspents":[],"sum":0.000000},{"address":"1944eh1ysrjy1rgjyxcr9b0nvmbra2bmtm6v6kt1a3ag6wf07hfa0apgg","unspents":[],"sum":0.000000},{"address":"1rjcyqyzsf1p8b22fxdx79qwfcr7wzkx1n2xykc5chzd905jymbfd8j4n","unspents":[{"txid":"5df61f85cbe11a3ab7eedbc2929241aa10c27179ec0b4fef3cea9f803a9aad15","out":0,"amount":499.999900,"time":1576411013,"lockuntil":0},{"txid":"5e04142090815b50bf1688e5cbd3ceca550c8f2ad6e44d690fb7a5f21b478227","out":0,"amount":499.999900,"time":1577325600,"lockuntil":0},{"txid":"5dfa848c46ee3c4b3b468674433dad2b3d5531428e5b60ef3d827c020859f73a","out":0,"amount":499.999900,"time":1576699020,"lockuntil":0}],"sum":1499.999700},{"address":"1jvy4ys1yrz3rg6te1h70mt0t89v9d0pbr038d48wmyzyzvrxk7safdg2","unspents":[],"sum":0.000000},{"address":"1dq62d8y4fz20sfg63zzy4h4ayksswv1fgqjzvegde306bxxg5zygc27q","unspents":[{"txid":"5df31decea3f149fe53062fbdbc8ca4e42d54e348f9e3c482a1edfb0c5b34100","out":0,"amount":499.999900,"time":1576213996,"lockuntil":0},{"txid":"5df26e22ab57f87fa7f5ee931b09cdc550583ab7517afa18e5f68db1ee53fb02","out":0,"amount":499.999900,"time":1576168994,"lockuntil":0},{"txid":"5df235df6a0d575378361ba75560ffab8ee9c786a58e604f14c037d7ec2afd05","out":0,"amount":499.999900,"time":1576154591,"lockuntil":0}],"sum":1499.999700},{"address":"20g075m4mh5trbkdy8vbh74n9h7t3npe7tewpnvsfb19p57jnyf3kdh45","unspents":[{"txid":"5e032143eb0c23c13f2d422144f1337ecd467c2163c7758e5a00b4d6fbe7ba00","out":0,"amount":1153.000000,"time":1577263427,"lockuntil":0},{"txid":"5e046a5cae5ea2622a37ad737f53e9185b60ea010242191b75f276b8d50dc900","out":0,"amount":1153.000000,"time":1577347676,"lockuntil":0},{"txid":"5e0399615cb69be399d6b6fee2bc1aca10d9d4e222467f9eef76745334b8ad01","out":0,"amount":1153.000000,"time":1577294177,"lockuntil":0}],"sum":3459.000000}],"total":8322.998800}

>> curl -d '{"id":0,"method":"listunspent","jsonrpc":"2.0","params":{"address":"1965p604xzdrffvg90ax9bk0q3xyqn5zz2vc9zpbe3wdswzazj7d144mm","fork":"00000000beb6f9e2a8813f4778b26820a8649894de63624e65bc0ffa9562730b","max":1}}' http://127.0.0.1:6602
<< {"id":0,"jsonrpc":"2.0","result":{"unspents":[{"txid":"5dd4bb56b3d0aed9bb0e0a80d01ec1f70d7bd4d1fc200a48df330154b366ca8e","out":0,"amount":1000.000000,"time":1574222678,"lockuntil":0}],"sum":1000.000000}}
```
**Errors:**
```
* {"code":-401,"message":"Address as an argument should be provided."}
* {"code":-401,"message":"Acquiring unspent list failed."}
* {"code":-402,"message":"Not enough funds in wallet or account."}
```
##### [Back to top](#commands)
---
### getwork
**Usage:**
```
        getwork <"spent"> <"privkey"> ("prev")

If (prev hash) is matched with the current primary chain,return true
If next block is not generated by proof-of-work,return false
Otherwise, return formatted proof-of-work parameters to work on:
"prevblockheight" : prevblock height
"prevblockhash" : prevblock hash
"prevblocktime" : prevblock timestamp
"algo" : proof-of-work algorithm: cryptonight=1,...
"bits" : proof-of-work difficulty nbits
"data" : work data
```
**Arguments:**
```
 "spent"                                (string, required) spent address
 "privkey"                              (string, required) private key
 "prev"                                 (string, optional) prev block hash
```
**Request:**
```
 "param" :
 {
   "spent": "",                         (string, required) spent address
   "privkey": "",                       (string, required) private key
   "prev": ""                           (string, optional) prev block hash
 }
```
**Response:**
```
 "result" :
 {
   (if prev is matched or block is not generated by POW)
   "result": true|false,                (bool, optional) result
   (if next block is generated by POW)
   "work":                              (object, optional) work data
   {
     "prevblockheight": 0,              (int, required) prev block height
     "prevblockhash": "",               (string, required) prev block hash
     "prevblocktime": 0,                (uint, required) prev block time
     "algo": 0,                         (int, required) algo
     "bits": 0,                         (int, required) bits
     "data": ""                         (string, required) work data
   }
 }
```
**Examples:**
```
>> ibrio-cli getwork 1pdr1knaaa4fzr846v89g3q2tzb8pbvbavbbft8xppkky0mqnmsq8gn5y ceae964a1119f110b0cff3614426dd692f8467a95cc2c276e523efc63c5e5031 7ee748e9a827d476d1b4ddb77dc8f9bad779f7b71593d5c5bf73b535e1cc2446
<< {"work":{"prevblockheight":23,"prevblockhash":"f734bb6bc12ab4058532113cfe6a3412d1036eae25f60a97ee1b17effc6e74de","prevblocktime":1538142032,"algo":1,"bits":25,"data":"01000100822fae5bde746efcef171bee970af625ae6e03d112346afe3c11328505b42ac16bbb34f74300000000000000000000000000000000000000000000000000000000000000000001190000000000000000000000000000000000000000000000000000000000000000"}}

>> curl -d '{"id":1,"method":"getwork","jsonrpc":"2.0","params":{"spent":"1pdr1knaaa4fzr846v89g3q2tzb8pbvbavbbft8xppkky0mqnmsq8gn5y","privkey":"ceae964a1119f110b0cff3614426dd692f8467a95cc2c276e523efc63c5e5031","prev":"7ee748e9a827d476d1b4ddb77dc8f9bad779f7b71593d5c5bf73b535e1cc2446"}}' http://127.0.0.1:6602
<< {"id":1,"jsonrpc":"2.0","result":{"work":{"prevblockheight":23,"prevblockhash":"f734bb6bc12ab4058532113cfe6a3412d1036eae25f60a97ee1b17effc6e74de","prevblocktime":1538142032,"algo":1,"bits":25,"data":"01000100822fae5bde746efcef171bee970af625ae6e03d112346afe3c11328505b42ac16bbb34f74300000000000000000000000000000000000000000000000000000000000000000001190000000000000000000000000000000000000000000000000000000000000000"}}}
```
**Errors:**
```
* {"code" : -32603, "message" : "The primary chain is invalid."}
```
##### [Back to top](#commands)
---
### submitwork
**Usage:**
```
        submitwork <"data"> <"spent"> <"privkey">

Attempt to construct and submit new block to network
Return hash of new block.
```
**Arguments:**
```
 "data"                                 (string, required) work data
 "spent"                                (string, required) spent address
 "privkey"                              (string, required) private key
```
**Request:**
```
 "param" :
 {
   "data": "",                          (string, required) work data
   "spent": "",                         (string, required) spent address
   "privkey": ""                        (string, required) private key
 }
```
**Response:**
```
 "result": "hash"                       (string, required) block hash
```
**Examples:**
```
>> ibrio-cli submitwork 01000100502fae5b4624cce135b573bfc5d59315b7f779d7baf9c87db7ddb4d176d427a8e948e77e43000000000000000000000000000000000000000000000000000000000000000000011acfff020000000000000000000000000000000000000000000000000000000000 1dj5qcjst7eh4tems36n1m500hhyba3vx436t4a8hgdm7r7jrdbf2yqp9 41a9f94395ced97d5066e2d099df4f1e2bd96057f9c38e8ea3f8a02eccd0a98e
<< f734bb6bc12ab4058532113cfe6a3412d1036eae25f60a97ee1b17effc6e74de

>> curl -d '{"id":2,"method":"submitwork","jsonrpc":"2.0","params":{"data":"01000100502fae5b4624cce135b573bfc5d59315b7f779d7baf9c87db7ddb4d176d427a8e948e77e43000000000000000000000000000000000000000000000000000000000000000000011acfff020000000000000000000000000000000000000000000000000000000000","spent":"1dj5qcjst7eh4tems36n1m500hhyba3vx436t4a8hgdm7r7jrdbf2yqp9","privkey":"41a9f94395ced97d5066e2d099df4f1e2bd96057f9c38e8ea3f8a02eccd0a98e"}}' http://127.0.0.1:6602
<< {"id":2,"jsonrpc":"2.0","result":"f734bb6bc12ab4058532113cfe6a3412d1036eae25f60a97ee1b17effc6e74de"}
```
**Errors:**
```
* {"code" : -4, "message" : "Invalid spent address"}
* {"code" : -4, "message" : "Invalid private key"}
* {"code" : -4, "message" : "Invalid mint template"}
* {"code" : -6, "message" : "Block rejected : operation failed"}
* {"code" : -6, "message" : "Block rejected : block proof-of-work is invalid"}
```
##### [Back to top](#commands)
---
### querystat
**Usage:**
```
        querystat <"type"> (-f="fork") (-b="begin") (-n=count)

Query statistical data
```
**Arguments:**
```
 "type"                                 (string, required) statistical type: maker: block maker, p2psyn: p2p synchronization
 -f="fork"                              (string, optional) fork hash (default all fork)
 -b="begin"                             (string, optional) begin time(HH:MM:SS) (default last count records)
 -n=count                               (uint, optional) get record count (default 20)
```
**Request:**
```
 "param" :
 {
   "type": "",                          (string, required) statistical type: maker: block maker, p2psyn: p2p synchronization
   "fork": "",                          (string, optional) fork hash (default all fork)
   "begin": "",                         (string, optional) begin time(HH:MM:SS) (default last count records)
   "count": 0                           (uint, optional) get record count (default 20)
 }
```
**Response:**
```
 "result": "stattable"                  (string, required) statistical data table
                                        1) maker: block maker
                                        -- time: statistical time, format: hh:mm:ss
                                        -- powblocks: number of POW blocks produced in one minute
                                        -- dposblocks: number of DPOS blocks produced in one minute
                                        -- tps: number of TX produced in one second
                                        2) p2psyn: p2p synchronization
                                        -- time: statistical time, format: hh:mm:ss
                                        -- recvblocks: number of synchronized receiving blocks in one minute
                                        -- recvtps: number of synchronized receiving TX in one second
                                        -- sendblocks: number of synchronized sending blocks in one minute
                                        -- sendtps: number of synchronized sending TX in one second
```
**Examples:**
```
>> ibrio-cli querystat maker -b=09:11:00 -n=2
<< time      blocktps  tps  
09:11:59  1       12     
09:12:59  1       22     

>> curl -d '{"id":1,"method":"querystat","jsonrpc":"2.0","params":{"type":"maker","begin":"09:11:00","count":2}}' http://127.0.0.1:6602
<< {"id":0,"jsonrpc":"2.0","result":"time      blocktps  tps  
09:11:59  1       12     
09:12:59  1       22     "}
```
**Errors:**
```
* {"code" : -6, "message" : "Invalid type: is empty"}
* {"code" : -6, "message" : "Invalid type"}
* {"code" : -6, "message" : "Invalid fork"}
* {"code" : -6, "message" : "Invalid count"}
* {"code" : -32603, "message" : "query error"}
```
##### [Back to top](#commands)
---
### signrawtransactionwithwallet
**Usage:**
```
        signrawtransactionwithwallet <"addrIn"> <"txdata"> (-sm="sign_m") (-ss="sign_s") (-fd="fromdata") (-td="sendtodata") (-sec="signsecret")

Return json object with keys:
hex : raw transaction with signature(s) (hex-encoded string)
completed : true if transaction has a completed set of signature (false if not)
```
**Arguments:**
```
 "addrIn"                               (string, required) hex address string of pubkey or templateid used to sign offline transaction
 "txdata"                               (string, required) raw transaction data(hex string)
 -sm="sign_m"                           (string, optional) exchange sign m
 -ss="sign_s"                           (string, optional) exchange sign s
 -fd="fromdata"                         (string, optional) If the 'from' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
 -td="sendtodata"                       (string, optional) If the 'to' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
 -sec="signsecret"                      (string, optional) sign secret
```
**Request:**
```
 "param" :
 {
   "addrIn": "",                        (string, required) hex address string of pubkey or templateid used to sign offline transaction
   "txdata": "",                        (string, required) raw transaction data(hex string)
   "sign_m": "",                        (string, optional) exchange sign m
   "sign_s": "",                        (string, optional) exchange sign s
   "fromdata": "",                      (string, optional) If the 'from' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
   "sendtodata": "",                    (string, optional) If the 'to' address of transaction is a template, this option allows to save the template hex data. The hex data is equal output of RPC 'exporttemplate'
   "signsecret": ""                     (string, optional) sign secret
 }
```
**Response:**
```
 "result" :
 {
   "hex": "",                           (string, required) hex of transaction data
   "completed": true|false              (bool, required) transaction completed or not
 }
```
**Examples:**
```
>> ibrio-cli signrawtransactionwithwallet 20m0fetg8ypthr01xxyxmdxbr27bt0fp8j151d321w72ht9ywfkzxxnf5 01000000e0d2cc5e0000000091d2b15e9aeeb57483889873bd0aea1273c8e5bb8df436c2bd4e8506000000000195a751b8eaef2e9a2337be86b4e892e15d723ff992501c82c5a6d2aa7bcbcc5e010139d9d6ac592bb962578d52aac4b0755e5fd59d2d10bb20914896047741205f57c0cf6a000000000010270000000000000000
<< {"hex":"01000000e0d2cc5e0000000091d2b15e9aeeb57483889873bd0aea1273c8e5bb8df436c2bd4e8506000000000195a751b8eaef2e9a2337be86b4e892e15d723ff992501c82c5a6d2aa7bcbcc5e010139d9d6ac592bb962578d52aac4b0755e5fd59d2d10bb20914896047741205f57c0cf6a0000000000102700000000000000ee75b03705c7e9f66feed2338df515ee8826ff991258d0f9786203ce2d3445958d0202004bb6c84b2311f42905ea3749fe0ea2ae2ed79c950daa53d1dd899e99622c02030000000000000013f64ef0eb65bd1846bbe2835937868e8f76cc6ec1d15c18bfe41b5d6b38b3b5014eb8d334d9a93c1c51c31d300050fdf5fa8b9f6029af7354331b87a2300ec2c90106587926a76ffed40f884b39396fdbe4c0ca0a72e6499caecd130073117da5fb01014b4b4efd5425e102e7d13b089bb1c5387c877a4bd147251ad241a21246a443a9e46b18b24d8ae6a22faf6d6c79f3acd014e17f79491f42dd0f5d001d4a294b08","completed":false}

>> curl -d '{"id":62,"method":"signrawtransactionwithwallet","jsonrpc":"2.0","params":{"addrIn":"20m0fetg8ypthr01xxyxmdxbr27bt0fp8j151d321w72ht9ywfkzxxnf5","txdata":"01000000000000002b747e24738befccff4a05c21dba749632cb8eb410233fa110e3f58a779b4325010ef45be50157453a57519929052d0818c269dee60be98958d5ab65bc7e0919810001b9c3b7aa16c6cb1bf193faf717580d03347148b2145ca98b30b1376d634c12f440420f0000000000a08601000000000002123400"}}' http://127.0.0.1:6602
<< {"id":62,"jsonrpc":"2.0","result":{"hex":"01000000e0d2cc5e0000000091d2b15e9aeeb57483889873bd0aea1273c8e5bb8df436c2bd4e8506000000000195a751b8eaef2e9a2337be86b4e892e15d723ff992501c82c5a6d2aa7bcbcc5e010139d9d6ac592bb962578d52aac4b0755e5fd59d2d10bb20914896047741205f57c0cf6a0000000000102700000000000000fd2e0175b03705c7e9f66feed2338df515ee8826ff991258d0f9786203ce2d3445958d0202004bb6c84b2311f42905ea3749fe0ea2ae2ed79c950daa53d1dd899e99622c02030000000000000013f64ef0eb65bd1846bbe2835937868e8f76cc6ec1d15c18bfe41b5d6b38b3b5014eb8d334d9a93c1c51c31d300050fdf5fa8b9f6029af7354331b87a2300ec2c90106587926a76ffed40f884b39396fdbe4c0ca0a72e6499caecd130073117da5fb01054b4b4efd5425e102e7d13b089bb1c5387c877a4bd147251ad241a21246a443a9e46b18b24d8ae6a22faf6d6c79f3acd014e17f79491f42dd0f5d001d4a294b087f54c5679e286e77d07c6f9df1e3084daab9bb8accd5725b9146c4fe99f645c177e4d0079dacca1d85afb4f70044eb64e803054dbe150d7c4e6bb5c2bcc78007","completed":true}}

>> ibrio-cli signrawtransactionwithwallet 16g92n3v2p04htxgck0r3awk878d9x7gx60kp3wmc47wwdp6b491pxa9x 0100000065d5cc5e0000000091d2b15e9aeeb57483889873bd0aea1273c8e5bb8df436c2bd4e85060000000001a4df918abd7a55a7296e22b6dbfd1be7fd09651003387456fa59d3d11f96cc5e010139d9d6ac592bb962578d52aac4b0755e5fd59d2d10bb20914896047741205f5720aa44000000000010270000000000000000
<< {"hex":"0100000065d5cc5e0000000091d2b15e9aeeb57483889873bd0aea1273c8e5bb8df436c2bd4e85060000000001a4df918abd7a55a7296e22b6dbfd1be7fd09651003387456fa59d3d11f96cc5e010139d9d6ac592bb962578d52aac4b0755e5fd59d2d10bb20914896047741205f5720aa44000000000010270000000000000040844cfb574ada0ea0c37a76042c25b7fc82756deefc8a6150ef5babb54bf1ab3de3eba03cc3bc7ab74051ca5986ed83c2286fbcf0ab120ec4491425cef0d25502","completed":true}

>> curl -d '{"id":62,"method":"signrawtransactionwithwallet","jsonrpc":"2.0","params":{"addrIn":"16g92n3v2p04htxgck0r3awk878d9x7gx60kp3wmc47wwdp6b491pxa9x","txdata":"0100000065d5cc5e0000000091d2b15e9aeeb57483889873bd0aea1273c8e5bb8df436c2bd4e85060000000001a4df918abd7a55a7296e22b6dbfd1be7fd09651003387456fa59d3d11f96cc5e010139d9d6ac592bb962578d52aac4b0755e5fd59d2d10bb20914896047741205f5720aa44000000000010270000000000000000"}}' http://127.0.0.1:6602
<< {"id":62,"jsonrpc":"2.0","result":{"hex":"0100000065d5cc5e0000000091d2b15e9aeeb57483889873bd0aea1273c8e5bb8df436c2bd4e85060000000001a4df918abd7a55a7296e22b6dbfd1be7fd09651003387456fa59d3d11f96cc5e010139d9d6ac592bb962578d52aac4b0755e5fd59d2d10bb20914896047741205f5720aa44000000000010270000000000000040844cfb574ada0ea0c37a76042c25b7fc82756deefc8a6150ef5babb54bf1ab3de3eba03cc3bc7ab74051ca5986ed83c2286fbcf0ab120ec4491425cef0d25502","completed":true}}
```
**Errors:**
```
* {"code":-8,"message":"TX decode failed"}
* {"code":-401,"message":"Failed to sign offline transaction"}
```
##### [Back to top](#commands)
---
### sendrawtransaction
**Usage:**
```
        sendrawtransaction <"txdata">

Submit transaction raw data(serialized, hex-encoded) with offline signature using pubkey or template address.
```
**Arguments:**
```
 "txdata"                               (string, required) hex string of transaction binary data
```
**Request:**
```
 "param" :
 {
   "txdata": ""                         (string, required) hex string of transaction binary data
 }
```
**Response:**
```
 "result": "data"                       (string, required) txid: hash of transaction raw data
```
**Examples:**
```
>> ibrio-cli sendrawtransaction 01000000e0d2cc5e0000000091d2b15e9aeeb57483889873bd0aea1273c8e5bb8df436c2bd4e8506000000000195a751b8eaef2e9a2337be86b4e892e15d723ff992501c82c5a6d2aa7bcbcc5e010139d9d6ac592bb962578d52aac4b0755e5fd59d2d10bb20914896047741205f57c0cf6a0000000000102700000000000000fd2e0175b03705c7e9f66feed2338df515ee8826ff991258d0f9786203ce2d3445958d0202004bb6c84b2311f42905ea3749fe0ea2ae2ed79c950daa53d1dd899e99622c02030000000000000013f64ef0eb65bd1846bbe2835937868e8f76cc6ec1d15c18bfe41b5d6b38b3b5014eb8d334d9a93c1c51c31d300050fdf5fa8b9f6029af7354331b87a2300ec2c90106587926a76ffed40f884b39396fdbe4c0ca0a72e6499caecd130073117da5fb01054b4b4efd5425e102e7d13b089bb1c5387c877a4bd147251ad241a21246a443a9e46b18b24d8ae6a22faf6d6c79f3acd014e17f79491f42dd0f5d001d4a294b087f54c5679e286e77d07c6f9df1e3084daab9bb8accd5725b9146c4fe99f645c177e4d0079dacca1d85afb4f70044eb64e803054dbe150d7c4e6bb5c2bcc78007
<< 5eccd2e09fe34ae041d9a90d896759041b2bc2844f43ed6929889b53e826386b

>> curl -d '{"id":3,"method":"sendrawtransaction","jsonrpc":"2.0","params":{"txdata":"0100000065d5cc5e0000000091d2b15e9aeeb57483889873bd0aea1273c8e5bb8df436c2bd4e85060000000001a4df918abd7a55a7296e22b6dbfd1be7fd09651003387456fa59d3d11f96cc5e010139d9d6ac592bb962578d52aac4b0755e5fd59d2d10bb20914896047741205f5720aa44000000000010270000000000000040844cfb574ada0ea0c37a76042c25b7fc82756deefc8a6150ef5babb54bf1ab3de3eba03cc3bc7ab74051ca5986ed83c2286fbcf0ab120ec4491425cef0d25502"}}' http://127.0.0.1:6602
<< {"id":3,"jsonrpc":"2.0","result":"5eccd565e410131217f89ade75e3f89a5a17627edaa7da7638babc94e331ea49"}
```
**Errors:**
```
* {"code":-8,"message":"Raw tx decode failed"}
* {"code":-10,"message":"Tx rejected : xxx"}
```
##### [Back to top](#commands)
---
### reversehex
**Usage:**
```
        reversehex <"hex">

Reverse a hex string by byte
```
**Arguments:**
```
 "hex"                                  (string, required) hex string
```
**Request:**
```
 "param" :
 {
   "hex": ""                            (string, required) hex string
 }
```
**Response:**
```
 "result": "hex"                        (string, required) reversed hex string
```
**Examples:**
```
>> ibrio-cli reversehex e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9
<< f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e8

>> curl -d '{"id":15,"method":"reversehex","jsonrpc":"2.0","params":{"hex":"e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9"}}' http://127.0.0.1:6602
<< {"id":15,"jsonrpc":"2.0","result":"f9b4af95bec6c5d504366245e0420bc3c5c78cd05ea68e4ad85a4d770e77e3e8"}
```
**Errors:**
```
* {"code":-32602,"message":"hex string size is not even"}
* {"code":-32602,"message":"invalid hex string: g"}
```
##### [Back to top](#commands)
---
### activatesign
**Usage:**
```
        activatesign <"inviter"> <"owner">

Activate sign
```
**Arguments:**
```
 "inviter"                              (string, required) inviter address
 "owner"                                (string, required) owner address
```
**Request:**
```
 "param" :
 {
   "inviter": "",                       (string, required) inviter address
   "owner": ""                          (string, required) owner address
 }
```
**Response:**
```
 "result": "sign"                       (string, required) sign hex string
```
**Examples:**
```
>> ibrio-cli activatesign 1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb 1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda
<< 0f03a0c145576aa163b58f5904c07f9164cc33a748b4c9ec52ffd3e6c32fa9c5f07d2c252561ad013c52d71be958e406af1423ac3142a55f9f63566785549304

>> curl -d '{"id":15,"method":"activatesign","jsonrpc":"2.0","params":{"inviter":"1549pyzf8dhx7r4x40k5j80f12btkpqfprjp134bcgcrjn963nzsx57xb","owner":"1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda"}}' http://127.0.0.1:6602
<< {"id":15,"jsonrpc":"2.0","result":"0f03a0c145576aa163b58f5904c07f9164cc33a748b4c9ec52ffd3e6c32fa9c5f07d2c252561ad013c52d71be958e406af1423ac3142a55f9f63566785549304"}
```
**Errors:**
```
* {"code":-32602,"message":"Invalid inviter"}
* {"code":-32602,"message":"Invalid owner"}
* {"code":-32602,"message":"Invalid inviter or owner"}
* {"code":-4,"message":"Sign fail"}
```
##### [Back to top](#commands)
---
### getactivatestatus
**Usage:**
```
        getactivatestatus <"address">

Get activation status of address
```
**Arguments:**
```
 "address"                              (string, required) owner address
```
**Request:**
```
 "param" :
 {
   "address": ""                        (string, required) owner address
 }
```
**Response:**
```
 "result" :
 {
   "activate": true|false,              (bool, required) activation status
   "height": 0                          (uint, required) activation height
 }
```
**Examples:**
```
>> ibrio-cli getactivatestatus 1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda
<< {"status" : true, "height" : 49}

>> curl -d '{"id":15,"method":"getactivatestatus","jsonrpc":"2.0","params":{"hex":"e8e3770e774d5ad84a8ea65ed08cc7c5c30b42e045623604d5c5c6be95afb4f9"}}' http://127.0.0.1:6602
<< {"id":15,"jsonrpc":"2.0","result":{"status" : true, "height" : 49}}
```
**Errors:**
```
* {"code":-32602,"message":"Invalid address"}
```
##### [Back to top](#commands)
---

