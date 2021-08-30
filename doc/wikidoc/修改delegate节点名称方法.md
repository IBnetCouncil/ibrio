# 修改delegate节点名称方法

## 1）delegate节点名称说明：
```
1）delegate节点即为DPOS节点，或称超级节点；
2）delegate节点初始没有名称，需要通过发送交易修改名称；
3）delegate节点可通过listdelegate的RPC命令查询；
```

## 2） 修改delegate节点名称：
```
1）需要delegate模板地址中的owner，向delegate模板地址发送交易，交易数量不限（但需要>0）；
2）节点名称放在交易vchData中，即在sendfrom命令中的-d参数中；
3）-d参数为名称的HEX格式，如名称为'123'，HEX格式为'313233'；
例：
sendfrom 1j6x8vdkkbnxe8qwjggfan9c8m8zhmez7gm3pznsqxgch3eyrwxby8eda 20m053vhn4ygv9m8pzhevnjvtgbbqhgs66qv31ez39v9xbxvk0ynhfzer 1 -d=313233
```
