# bptree-nvm

NVMを使うB<sup>+</sup>-Treeの実験用コード諸々

## 実行方法

### 実行ファイルの作り方

cloneしたリポジトリで`make`するとbuildディレクトリができてその中に実行ファイルができる
（シングルスレッド版に修正が必要なので`make`だけだと現状動かない）

`make type=xxx`とすることで作る種類が変わる

type一覧
- simple ： シングルスレッドで動作するもの（版が古いので動かない）
- concurrent ： マルチスレッドで動作するもの（HTM使用）
- nvhtm ： マルチスレッドで動作するもの（[NV-HTM](https://bitbucket.org/daniel_castro1993/nvhtm/src/master/)使用）

同様に`tree=yyy`とすることで木が変わる

tree一覧
- bptree ： 一般的なB<sup>+</sup>-Tree
- fptree ： FPTree

例えばHTMを使う一般的なB<sup>+</sup>-Treeなら`make type=concurrent tree=bptree`となる

オプションいろいろ
- `make ... write_amount=1`とすると書き込み総量を測れる（FPTree限定）
- `make ... ca=1`とするとアボート回数を測れる（NV-HTMを使う場合は指定する必要なし）
- `make ... fw=1`とすると書き込み頻度を測れる
- `make ... write_amount_nvhtm=1`とすると書き込み総量を測れる（NV-HTM限定）
- `make ... stats=1`とすると追加で情報を取る＆実験前にアボート回数とかをリセットしたりする（NV-HTM限定）
- `make ... emulator=1`とするとエミュレータを使った元々のNV-HTMが動く

### ベンチマーク

`make bench_all ppath=NVMをマウントしてるディレクトリ vdirpath=tmpfsでマウントしてるディレクトリ`とするのが一番簡単．
一部だけとりたい場合は`make write_amount`などとする

## ディレクトリ構成

- src
    - benchmark：ベンチマーク用のmain関数
    - bptree：一般的なB<sup>+</sup>-Tree
    - fptree：FPTree
    - test：テスト用のmain関数（動かないかもしれない）
    - utility：アロケータ等々
        - allocator：NVM用メモリアロケータ
        - benchmark_script：ベンチマーク実行スクリプト
        - graph_script：グラフ作成スクリプト
        - random：乱数発生＆スレッドへの振り分け関数たち
        - thread_manager：スレッドの開始同期とか諸々やる関数たち
- include：includeファイルたち
- nvhtm：[NV-HTM](https://bitbucket.org/daniel_castro1993/nvhtm.git)本体にNVMの実機を使う＋各種統計情報を取るために変更を加えたもの
- nvhtm_modification_files_plain：nvhtmのうちオリジナルと差分のあるファイルたち
- nvm-emulation：[nvhtm-selfcontained](https://bitbucket.org/daniel_castro1993/nvhtm-selfcontained.git)からエミュレータを抜き出したものにさらに変更を加えたもの
- dummy_min-nvm：nvm-emulationのうちオリジナルと差分のあるファイルたち
- dummy_min-nvm_wfreq：基本↑と同じだが書き込み頻度を測るための処理を加えたもの（古い）
- res：実験結果たち
- stamp_nvhtm：nvhtm-selfcontainedからコピーして改造したもの

## 未mergeブランチ一覧

なし
