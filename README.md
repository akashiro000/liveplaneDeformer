# livePlaneDeformer

Maya 2018/2025対応のポリゴン表面追従デフォーマープラグイン

## 概要

livePlaneDeformerは、ポリゴンAをポリゴンBの表面形状に沿って変形させるMayaデフォーマープラグインです。

## 機能

- ターゲットメッシュの表面に沿った変形
- 法線方向へのオフセット調整
- リアルタイムビューポート更新
- Maya 2018および2025対応

## ビルド要件

- CMake 3.15以上
- Maya 2018または2025がインストールされていること
- Windows: Visual Studio 2015以上
- macOS: Xcode
- Linux: GCC

## ビルド手順

### Windows

```bash
# MAYA_LOCATIONを設定（例: Maya 2018の場合）
set MAYA_LOCATION=C:\Program Files\Autodesk\Maya2018

# ビルドディレクトリを作成
mkdir build
cd build

# CMake設定
cmake .. -G "Visual Studio 15 2017 Win64"
# または Maya 2025の場合
cmake .. -G "Visual Studio 16 2019" -A x64

# ビルド
cmake --build . --config Release
```

### macOS

```bash
# MAYA_LOCATIONを設定
export MAYA_LOCATION=/Applications/Autodesk/maya2018

# ビルド
mkdir build
cd build
cmake ..
make
```

### Linux

```bash
# MAYA_LOCATIONを設定
export MAYA_LOCATION=/usr/autodesk/maya2018

# ビルド
mkdir build
cd build
cmake ..
make
```

## インストール

ビルド後、生成された`.mll`（Windows）、`.bundle`（macOS）、または`.so`（Linux）ファイルをMayaのプラグインディレクトリにコピーします。

```bash
# Windows例
copy build\Release\livePlaneDeformer.mll "%USERPROFILE%\Documents\maya\2018\plug-ins\"

# macOS/Linux例
cp build/livePlaneDeformer.bundle ~/maya/2018/plug-ins/
```

## 使用方法

### プラグインのロード

1. Mayaを起動
2. Windows > Settings/Preferences > Plug-in Manager
3. `livePlaneDeformer`を探してロード

### MELでの使用例

```mel
// ソースメッシュとターゲットメッシュを作成
polySphere -n "sourceMesh" -r 1;
polyPlane -n "targetMesh" -w 10 -h 10 -sx 20 -sy 20;

// ターゲットメッシュに変形を加える
select -r targetMesh;
nonLinear -type wave;

// ソースメッシュにデフォーマーを適用
select -r sourceMesh;
deformer -type "livePlaneDeformer";

// ターゲットメッシュを接続
connectAttr targetMesh.worldMesh[0] livePlaneDeformer1.targetMesh;

// オフセットを調整（オプション）
setAttr "livePlaneDeformer1.offset" 0.5;
```

### Pythonでの使用例

```python
import maya.cmds as cmds

# メッシュ作成
source_mesh = cmds.polySphere(n="sourceMesh", r=1)[0]
target_mesh = cmds.polyPlane(n="targetMesh", w=10, h=10, sx=20, sy=20)[0]

# ターゲットメッシュに変形
cmds.select(target_mesh)
cmds.nonLinear(type='wave')

# デフォーマー適用
cmds.select(source_mesh)
deformer_node = cmds.deformer(type='livePlaneDeformer')[0]

# 接続
cmds.connectAttr(f'{target_mesh}.worldMesh[0]', f'{deformer_node}.targetMesh')

# オフセット設定
cmds.setAttr(f'{deformer_node}.offset', 0.5)
```

## アトリビュート

- **targetMesh**: 変形先となるターゲットメッシュ
- **offset**: ターゲット表面からのオフセット距離（デフォルト: 0.0）
- **envelope**: デフォーマーの影響度（0.0-1.0、デフォルト: 1.0）

## トラブルシューティング

### プラグインがロードできない

- Maya のバージョンとビルドに使用したMaya SDKのバージョンが一致しているか確認
- 依存するMayaライブラリが見つかるか確認

### 変形しない

- ターゲットメッシュが正しく接続されているか確認
- envelope値が0になっていないか確認

## ライセンス

MIT License

## 開発者向け情報

### プロジェクト構成

```
liveplaneDeformer/
├── CMakeLists.txt          # ビルド設定
├── README.md               # このファイル
├── CLAUDE.md               # 開発方針
├── include/
│   └── livePlaneDeformer.h # ヘッダーファイル
└── src/
    ├── livePlaneDeformer.cpp  # メイン実装
    └── pluginMain.cpp         # プラグインエントリーポイント
```

### 技術詳細

- **デフォーマータイプ**: MPxDeformerNode
- **アルゴリズム**: MMeshIntersectorを使用した最近傍点探索
- **変形方式**: 表面法線方向への投影とオフセット

## 貢献

Issue、Pull Requestを歓迎します。

## 関連リンク

- [Maya API Documentation](https://help.autodesk.com/view/MAYAUL/2018/ENU/?guid=__cpp_ref_index_html)
- [GitHub Repository](https://github.com/akashiro000/liveplaneDeformer)
