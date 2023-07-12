第8章のバーチャルキューの構築

1. src-queue を ~/ns-allinone-3.19/ns-3.19/src/queue/.にコピー
    % cp  -r  src-queue  ~ns-allinone-3.19/ns-3.19/src/queue

2. ns3を再構築する
    ./waf -d debug --enable-examples --enable-tests configure
    ./waf build
