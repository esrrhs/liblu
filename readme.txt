liblu是用于游戏的C风格网络库，简单高效，同时提供加密压缩校验等功能。
使用示例：
luconfig cfg;
cfg.cco = on_conn_open;
cfg.ccrp = on_conn_recv_packet;
cfg.ccc = on_conn_close;
lu * l = newlu(&cfg);
while (1)
    ticklu(l);
dellu(l);
