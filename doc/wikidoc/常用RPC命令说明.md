# 常用RPC命令说明

## RPC命令列表
 - [listpeer](#listpeer):查询P2P连接列表
 - [listfork](#listfork):查询FORK列表
 - [getforkheight](#getforkheight):查询链高度
 - [getblockhash](#getblockhash):查询指定高度的块HASH
 - [getblock](#getblock):查询块信息
 - [getblockdetail](#getblockdetail):查询块详细信息，获得块中的所有交易详细信息，包括奖励交易
 - [gettxpool](#gettxpool):查询交易池信息，包括查询交易数量，或交易信息
 - [gettransaction](#gettransaction):查询交易详细信息
 - [listdelegate](#listdelegate):查询DPOS投票列表
 - [listkey](#listkey):查询钱包中的公钥列表
 - [listaddress](#listaddress):查询钱包中的地址列表
 - [importprivkey](#importprivkey):钱包导入私钥
 - [addnewtemplate](#addnewtemplate):钱包导入模板地址
 - [removekey](#removekey):从钱包中移除公钥
 - [removetemplate](#removetemplate):从钱包中移除模板地址
 - [lockkey](#lockkey):公钥加锁
 - [unlockkey](#unlockkey):公钥解锁
 - [getbalance](#getbalance):获取余额
 - [sendfrom](#sendfrom):产生交易
 - [createtransaction](#createtransaction):创建交易
 - [signtransaction](#signtransaction):签名交易
 - [sendtransaction](#sendtransaction):发送交易
 - [listunspent](#listunspent):查询UTXO列表
 - [makekeypair](#makekeypair):产生公私钥对
 - [getpubkey](#getpubkey):获取地址的公钥
 - [getpubkeyaddress](#getpubkeyaddress):获取公钥的地址
 - [activatesign](#activatesign):激活签名
 - [getactivatestatus](#getactivatestatus):查询地址的激活状态
---

### listpeer
功能说明:
```
查询P2P连接列表
```
请求参数: 
```
无
```
结果参数: 
```
address: 对端公网IP及端口
services: 对端提供的服务
lastsend: 最后一次发送消息的时间（UTC）
lastrecv: 最后一次接收消息的时间（UTC）
conntime: 连接成功的时间（UTC）
pingtime: PINGPONG的时长（ms）
version: 对端节点程序的版本号
subver: 对端节点程序的子版本号
inbound: 该P2P连接是否为输入方式，true表示为输入，false表示为输出
height: 对端节点的主链高度，P2P连接每间隔一段时间同步一次高度，所以显示的高度并不是实时的对端高度
banscore: P2P连接的成绩，数值为100表示成绩最高，即P2P同步状态最好，数值越低，则说明同步失败率高
```

### listfork
功能说明: 
```
查询FORK列表
```
请求参数: 
无
结果参数: 
```
fork: FORKID
name: FORK名称
symbol: FORK符号
amount: FORK初始TOKEN数，即FORK原始块（链第一块）中的奖励TOKEN数
reward: 块奖励TOKEN数
halvecycle: 块奖励减半周期，0表示不减半，即固定块奖励，每块的奖励固定不变，大于0表示减半的高度，每经过该高度将减半块奖励
isolated: FORK是否为独立链，true表示独立链，不继承父链的TOKEN，false为非独立链，会继承父链的TOKEN，父链中的地址的TOKEN，在子链中也能使用，而独立链则不能使用父链中的地址的TOKEN
private: FORK是否为私有链，true表示为私有链，子链不能继承父链的TOKEN，并且子链的owner地址必须与父链的owner地址相同，false表示为非私有链，子链可以继承父链的TOKEN
enclosed: 暂停使用
owner: FORK的所有者地址，也为原始块的奖励地址
createtxid: 创建该FORK的交易ID
forkheight: 创建FORK的交易ID所在块的高度
parentfork: 父链的FORKID
forktype: 本FORK类型，如defi
forkheight : FORK高度
lastblock : FORK最后块HASH
moneysupply : FORK流通量（包含销毁量）
moneydestroy : FORK销毁量
```

### getforkheight
功能说明: 
```
查询链高度
```
请求参数: 
```
forkid: 查询的FORKID，可选参数，如果未填写，则为主链FORKID
```
结果参数: 
```
链高度值
```

### getblockhash
功能说明: 
```
查询指定高度的块HASH
```
请求参数: 
```
height: 查询高度（必选）
fork: 查询的FORKID，缺省为主链FORKID（可选）
```
结果参数: 
```
块HASH
```

### getblock
功能说明: 
```
查询块信息
```
请求参数: 
```
块HASH
```
结果参数: 
```
hash: 块HASH
version: 版本号
type: 块类型，包括: genesis: 创世块，origin: 子链原始块（链第一块），extended: 子链的子块，primary-pow: 主链POW块，primary-dpos: 主链DPOS块，subsidiary-dpos: 子链主块，vacant-dpos: 子链空块
time: 块时间
fork: 块所属链的FORKID
height: 块所在高度
txmint: 块的奖励交易
tx: 块打包的交易表
prev: 前一块HASH
```

### getblockdetail
功能说明: 
```
查询块详细信息，获得块中的所有交易详细信息，包括奖励交易
```
请求参数: 
```
块HASH
```
结果参数: 
```
在getblock的结果参数中增加交易详细信息，交易参数说明如下: 
txid: 交易ID
version: 版本号
type: 交易类型，如下：
>token: 普通交易
>certification: DPOS共识交易
>genesis: 创世块的奖励交易
>stake: dpos块的奖励交易
>work: pow块的奖励交易
>defi-reward: defi链的奖励交易
>defi-relation: defi链的邀请关系交易
>defi-mint-height: defi链的开始挖矿高度交易
time: 交易时间戳
lockuntil: 锁定高度，如果为0，则不锁定，如果大于0，则在该高度之下，不能花费该交易的输出（指sendto输出，即out为0输出）
anchor: 交易所在链的FORKID
blockhash: 交易所在块HASH
vin: 输入的UTXO
sendfrom: 交易from地址
sendto: 交易to地址
amount: 交易数量
txfee: 交易矿工费
data: 交易数据
sig: 交易签名
fork: 交易所在链的FORKID
confirmations: 交易确认块数
```

### gettxpool
功能说明: 
```
查询交易池信息，包括查询交易数量，或交易信息
```
请求参数: 
```
fork(-f): 查询的链FORKID，缺省为主链FORKID（可选）
address(-a): 查询指定地址在交易池的交易信息，缺省为所有地址（可选）
detail(-d): 是否要查询交易信息，缺省不查询，只查询交易池中的交易条数，如果带该参数，则查询交易信息（可选）
getoffset(-o): 查询交易信息时，查询偏移位置，缺省为0，当detail为true时，该参数才有效（由于交易池中的交易过多，一次无法全部查询，则可通过该参数分段查询）（可选）
getcount(-n): 查询交易信息时，请求获取交易条数，缺省为20条，当detail为true时，该参数才有效（可选）
```
结果参数: 
```
当detail为false时，即只查询条数时：
count：交易池中的交易数
size：交易累计大小（字节）
当detail为true时，查询交易信息：
txid: 交易ID
txtype: 交易类型，如下：
>token: 普通交易
>certification: DPOS共识交易
>genesis: 创世块的奖励交易
>stake: dpos块的奖励交易
>work: pow块的奖励交易
>defi-reward: defi链的奖励交易
>defi-relation: defi链的邀请关系交易
>defi-mint-height: defi链的开始挖矿高度交易
from: 交易from地址
to: 交易to地址
amount: 交易数量
txfee: 矿工费
size: 交易大小（字节）
```

### gettransaction
功能说明: 
```
查询交易详细信息
```
请求参数: 
```
txid: 查询交易ID（必选）
serialized(-s): 是否获取交易序列化数据，true表示要获取，false表示不获取，即显示交易各字段信息，缺省为false（可选）
```
结果参数: 
```
txid: 交易ID
version: 版本号
type: 交易类型，如下：
>token: 普通交易
>certification: DPOS共识交易
>genesis: 创世块的奖励交易
>stake: dpos块的奖励交易
>work: pow块的奖励交易
>defi-reward: defi链的奖励交易
>defi-relation: defi链的邀请关系交易
>defi-mint-height: defi链的开始挖矿高度交易
time: 交易时间戳
lockuntil: 锁定高度，如果为0，则不锁定，如果大于0，则在该高度之下，不能花费该交易的输出（指sendto输出，即out为0输出）
anchor: 交易所在链的FORKID
blockhash: 交易所在块HASH
vin: 输入的UTXO
sendfrom: 交易from地址
sendto: 交易to地址
amount: 交易数量
txfee: 交易矿工费
data: 交易数据
sig: 交易签名
fork: 交易所在链的FORKID
confirmations: 交易确认块数
```

### listdelegate
功能说明: 
```
查询DPOS投票列表
```
请求参数: 
```
count(-n): 查询条数（可选）
```
结果参数: 
```
address: DPOS地址
votes: 投票TOKEN数
```

### listkey
功能说明: 
```
查询钱包中的公钥列表
```
请求参数: 
```
page(-p): 查询页号，缺省为0（可选）
count(-n): 查询每页条数，缺省为30（可选）
```
结果参数: 
```
key: 公钥
version: 版本号
public: 是否仅为公钥，即钱包中没有私钥，true表示只是公钥，没有导入私钥，false表示已导入了私钥
locked: 是否锁定，true表示已锁定，不能发生交易，false表示已解锁，可以产生交易
timeout: 解锁的超时时长（秒），即解锁后，指定时间会自动加锁，如果为0，表示不自动加锁
```

### listaddress
功能说明: 
```
查询钱包中的地址列表
```
请求参数: 
```
page(-p): 查询页号，缺省为0（可选）
count(-n): 查询每页条数，缺省为30（可选）
```
结果参数: 
```
地址列表（参见地址信息部分）
```

### importprivkey
功能说明: 
```
钱包导入私钥
```
请求参数: 
```
privkey: 私钥
passphrase: 解锁密码
synctx: 暂停使用
```
结果参数: 
```
返回私钥对应的公钥
```

### addnewtemplate
功能说明: 
```
钱包导入模板地址
```
请求参数: 
```
type：模板类型
>delegate: DPOS模板
>vote: 投票模板
>fork: 创建FORK模板
>mint: POW挖矿模板
>multisig: 多签模板
>activate: 激活模板
第二个参数为模板参数串：
delegate模板（DPOS模板）参数：
>delegate: DPOS节点打包签名的私钥对应的公钥，不能与owner的公钥相同
>owner: delegate模板地址拥有者，该地址的私钥可以花费delegate模板地址的TOKEN
例：addnewtemplate delegate '{"delegate": "883f29fb7740f8e625159cb6bcee42cf3ac460be8b9fcb839ccbfeda3744b217", "owner": "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda"}'

vote模板（投票模板）参数：
>delegate: delegate模板地址，即要投票的DPOS节点的地址
>owner: vote模板地址的拥有者，该地址的私钥可以花费vote模板地址的TOKEN
>cycle: 存币生息周期，参数值为周期值，如7、14、30、90
>nonce: NONCE值（随机数或时间戳），以上参数相同的情况下，可以改变该参数值，得到不同的vote模板地址
例：addnewtemplate vote '{"delegate": "20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer", "owner": "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda", "cycle":7, "nonce":0}'

fork模板参数：
>redeem: 赎回地址
>fork: 要创建的子链的FORKID
例：addnewtemplate fork '{"redeem": "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda", "fork": "a63d6f9d8055dc1bd7799593fb46ddc1b4e4519bd049e8eba1a0806917dcafc0"}'

mint模板（POW挖矿模板）参数：
>mint: POW挖矿签名的公钥，不能与spent的公钥相同
>spent: mint模板模板的拥有者，该地址的私钥可以花费mint模板地址的TOKEN
例：addnewtemplate mint '{"mint": "883f29fb7740f8e625159cb6bcee42cf3ac460be8b9fcb839ccbfeda3744b217", "spent": "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda"}'

activate模板（ 激活模板）参数：
>inviter: 邀请者地址
>owner: 被激活者地址
>nonce: NONCE（随机数或时间戳）
例：addnewtemplate activate '{"inviter": "1231kgws0rhjtfewv57jegfe5bp4dncax60szxk8f4y546jsfkap3t5ws", "owner": "1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda", "nonce":0}'
```
结果参数: 
```
模板地址
```

### maketemplate
功能说明: 
```
生成模板地址，不会导入钱包，参数与addnewtemplate命令相同。
```

### removekey
功能说明: 
```
从钱包中移除公钥
```
请求参数: 
```
pubkey: 公钥或者对应的地址
passphrase: 解锁密码，导入时设置的密码
```
结果参数: 
```
公钥
```


### removetemplate
功能说明: 
```
从钱包中移除模板地址
```
请求参数: 
```
address: 模板地址
```
结果参数: 
```
结果（成功或失败）
```

### lockkey
功能说明: 
```
公钥加锁
```
请求参数: 
```
pubkey: 公钥或者对应的地址
```
结果参数: 
```
结果（成功或失败）
```

### unlockkey
功能说明: 
```
公钥解锁
```
请求参数: 
```
pubkey: 公钥或者对应的地址
passphrase: 解锁密码，导入时设置的密码
timeout(-t): 设置解锁后自动加锁的超时时长（秒），如果设置为0，则不自动加锁
```
结果参数: 
```
结果（成功或失败）
```

### getbalance
功能说明: 
```
获取余额
```
请求参数: 
```
fork(-f): 查询的链的FORKID，缺省为主链FORKID
address(-a): 查询的地址，缺省为钱包中的所有地址，如果有此参数，则查询指定地址的余额
page(-p): 查询页号，页号从0开始，缺省为0，如果钱包中的地址过多，则需要通过分页查询
count(-n): 每页地址数，缺省为30
```
结果参数: 
```
address: 地址
avail: 可用余额
locked: 锁定余额，地址的总余额=可用余额+锁定余额
unconfirmed: 未确认的余额，即未打包的交易的TOKEN数
```

### sendfrom
功能说明: 
```
产生交易
```
请求参数: 
```
from: 源地址
to: 目的地址
amount: 交易TOKEN数
txfee: 交易矿工费，缺省为0.01TOKEN（可选）
fork(-f): 交易所在链的FORKID，缺省为主链FORKID（可选）
data(-d): 交易数据（可选）
sign_m(-sm): 跨链交易m签名（可选）
sign_s(-ss): 跨链交易s签名（可选）
fromdata(-fd): from地址为模板地址时，该参数为from模板地址数据，当from模板地址未导入钱包，则需要设置该参数，如果钱包中有该地址，则自动获取模板数据（可选）
sendtodata(-td): to地址为模板地址时，该参数为to模板地址数据，当to模板地址未导入钱包，则需要设置该参数，如果钱包中有该地址，则自动获取模板数据（可选）
type(-type): 交易类型，缺省为普通交易，0：普通交易，2：DEFI关系交易，3：DEFI挖矿开始高度设置交易（可选）
signsecret(-sec): 签名数据（可选）
mintheight(-mintheight): 挖矿开始高度（可选）
lockheight(-lh): 交易锁定高度，如果设置了该参数，则该交易的UTXO需要在指定高度后才可以使用（可选）
```
结果参数: 
```
交易ID
```

### createtransaction
功能说明: 
```
创建交易
```
请求参数: 
```
from: 源地址
to: 目的地址
amount: 交易TOKEN数
txfee: 交易矿工费，缺省为0.01TOKEN（可选）
fork(-f): 交易所在链的FORKID，缺省为主链FORKID（可选）
data(-d): 交易数据（可选）
type(-type): 交易类型，缺省为普通交易，0：普通交易，2：DEFI关系交易，3：DEFI挖矿开始高度设置交易（可选）
mintheight(-mintheight): 挖矿开始高度（可选）
lockheight(-lh): 交易锁定高度，如果设置了该参数，则该交易的UTXO需要在指定高度后才可以使用（可选）
```
结果参数: 
```
交易数据，该数据用于交易签名命令
```

### signtransaction
功能说明: 
```
签名交易
```
请求参数: 
```
txdata: 交易数据，由createtransaction命令得到的数据
sign_m(-sm): 跨链交易m签名（可选）
sign_s(-ss): 跨链交易s签名（可选）
fromdata(-fd): from地址为模板地址时，该参数为from模板地址数据，当from模板地址未导入钱包，则需要设置该参数，如果钱包中有该地址，则自动获取模板数据（可选）
sendtodata(-td): to地址为模板地址时，该参数为to模板地址数据，当to模板地址未导入钱包，则需要设置该参数，如果钱包中有该地址，则自动获取模板数据（可选）
signsecret(-sec): 签名数据（可选）
```
结果参数: 
```
hex: 签名后的交易数据
completed: 是否处理完成（暂未使用）
```

### sendtransaction
功能说明: 
```
发送交易
```
请求参数: 
```
txdata: 交易数据，由signtransaction命令得到的数据
```
结果参数: 
```
交易ID
```

### listunspent
功能说明: 
```
查询UTXO列表
```
请求参数: 
```
address: 查询指定地址的utxo列表（必选）
fork(-f): 查询链的FORKID，缺省为主链FORKID（可选）
max(-n): 返回的最大utxo条数，缺省为10条，如果设置为0，表示查询所有utxo（可选）
amount(-a): 查询指定数量TOKEN的utxo，用于打包交易时使用，可通过设定交易的TOKEN数来查询utxo列表，用于交易输入打包（可选）
```
结果参数: 
```
txid: 交易ID
out: 输出项，0：表示sendto输出，1：表示找零输出
amount: utxo的TOKEN数
height: 交易所在块的高度，-1表示在交易池中
time: 交易的时间戳
lockuntil: 锁定高度，0表示未锁定
```

### makekeypair
功能说明: 
```
产生公私钥对
```
请求参数: 
```
无
```
结果参数: 
```
privkey: 私钥
pubkey: 公钥
```

### getpubkey
功能说明: 
```
获取地址的公钥
```
请求参数: 
```
privkeyaddress: 地址
```
结果参数: 
```
pubkey: 公钥
```

### getpubkeyaddress
功能说明: 
```
获取公钥的地址
```
请求参数: 
```
pubkey: 公钥
```
结果参数: 
```
address: 地址
```

### activatesign
功能说明: 
```
激活签名，由被激活的地址签名，签名数据用于邀请人转帐时使用，即在向激活模板地址转帐时，需要带该签名数据。
```
请求参数: 
```
inviter: 邀请人地址
owner: 被激活的地址
```
结果参数: 
```
sign: 签名数据，该数据用于邀请人地址向激活模板地址转帐时，用在-d参数中。
```

### getactivatestatus
功能说明: 
```
查询地址的激活状态。
```
请求参数: 
```
address: 要查询的地址，该地址为被激活的地址。
```
结果参数: 
```
activate: 激活状态，true表示已激活，false表示未激活。
height: 激活交易上链的高度，即交易所在块的高度。
```
