liblu是用于游戏的C风格网络库，提供tcp功能，集成epoll、select，针对单线程游戏做特殊处理优化
对于tcp模块，采用包模式，这种方式会把上层的数据加上包头发出，收到方传给上层的也是完整的数据内容，提供的功能有如加密、压缩、校验等，提供的接口有sendmsg，cb_recvmsg
对于每个tcp socket连接，维持双向的buffer，用于缓存数据，减少system call次数