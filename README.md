# DCN Group Project项目 —— 聊天应用说明
你好，我们是DCN小组 2024（春）\
我们的小组成员是余泰安，李政谕，李艺，尹勤艺（排名不分先后）


## 项目迭代
***
### Vision 1.1
#### 新增功能
1. /pri指令，用于实现私聊功能

#### 改进项目
1. 修复了管理员会与用户重名的BUG
2. 现在客户端可以使用/list指令了
3. 修复了用户可能会与管理员重名的BUG

#### 迭代目标
1. GUI
2. 数据加密
3. 优化/pri指令，/pri指令改为/pri+id后直接输入私聊内容

***
### Vision 1.0 正式版
#### 功能实现
1. 3个及以上群聊功能
2. 管理员端和客户端权限分离，数据分离
3. 指令的识别功能
    1. /reg指令，用于客户端注册
    2. /login指令，用户客户端登录
    3. /list指令，用于列出在线用户列表（仅管理员端可用）
    4. /kick指令，用于踢出指定用户（仅管理员端可用）
    5. /quit指令，用于退出客户端

#### 目前已知的BUG
1. 管理员端未作登录验证，可能会出现管理员和用户重名的情况
2. 用户被踢出以后，不能重新进行注册（这个算BUG吗）

***
### Vision 0.4 ~ Vision 0.1 测试版
#### 已经不再更新
该版本用于测试程序，BUG与错误以及语言不统一等问题较多，且功能较少，已经不再使用
