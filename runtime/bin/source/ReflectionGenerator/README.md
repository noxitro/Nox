# ReflectionGenerator

## これは何か

`ReflectionGenerator` は、C++ のヘッダを libclang (ClangSharp) で解析して、リフレクション情報と ECS の購読テーブルを C++ ソースとして書き出すコード生成器である。MSBuild のカスタムタスク `Nox.CustomTask.Task` が `reflection_generated` プロジェクトのコンパイル直前 (`runtime/reflection_generated/Directory.Build.targets` の `ReflectionGeneratorPreProcess` ターゲット) に走り、インクルードパス・プリプロセッサ定義・構成などのビルド情報を前処理データ (バイナリ) へ書き出す。続いて `ReflectionGenerator.exe` が同じ前処理データを読み、`runtime/reflection_generated/gen/<Platform>/<Configuration>/` へ `class_decl_*.g.cpp` / `global_decl_*.g.cpp` / `entity_type_*.g.cpp` / `entity_type_table.g.cpp` を出力する。生成された `entity_type_*.g.cpp` は同じターゲット内で `ClCompile` へ追加されるため、ヘッダに型を定義するだけで `World` へ購読される (登録マクロも静的初期化も要らない)。

## ビルドと配置

```
dotnet build ReflectionGenerator.csproj -c Debug
```

配置先の `runtime/bin/ReflectionGenerator.exe` / `.dll` / `.pdb` は .gitignore 済み (リポジトリに入っていない) なので、ビルド後に手でコピーする必要がある。現在配置されているのは **Debug** 構成 (`bin/Debug/net10.0/`、従来から配置されてきた構成に合わせている)。

```powershell
Copy-Item -Force `
  ".\bin\Debug\net10.0\ReflectionGenerator.exe", `
  ".\bin\Debug\net10.0\ReflectionGenerator.dll", `
  ".\bin\Debug\net10.0\ReflectionGenerator.pdb" `
  "..\..\"
```

`Nox.CustomTask` (`../CustomTask/CustomTask.csproj`) を変更したときは `runtime/bin/CustomTask.dll` も更新すること。ReflectionGenerator はこの DLL を参照しており、前処理データの読み書きを両者で共有しているため、**必ず両方を同時に入れ替える**。

```powershell
dotnet build ..\CustomTask\CustomTask.csproj -c Debug
Copy-Item -Force "..\CustomTask\bin\Debug\netstandard2.0\CustomTask.dll" "..\..\"
```

## 前処理データ (バイナリ) の置き場所

以前は `%TEMP%\NoxReflectionPreData.bin` という固定パスだったため、同じリポジトリの複数 git worktree を同時にビルドすると互いのデータを上書きしてしまった。現在はソースツリーごとに異なる生成出力ディレクトリの下に置く。

```
runtime/reflection_generated/gen/NoxReflectionPreData.<Platform>.<Configuration>.bin
```

パスの決定は `Nox.CustomTask.Util.GetBinFilePath(outputGenerateDir, platform, configuration)` に一本化されている。書き手は MSBuild タスク (`OutputGenerateDir` / `Platform` / `Configuration`)、読み手は `ReflectionGenerator.exe` のコマンドライン引数 `-out` / `-platform` / `-config` から同じパスを組み立てる。`gen` ディレクトリは .gitignore 済み。

`ReflectionGenerator.exe` を手で走らせるときも MSBuild と同じ引数を渡すこと。

```powershell
.\bin\ReflectionGenerator.exe `
  -project "<repo>\runtime\reflection_generated\reflection_generated.vcxproj" `
  -solution "<repo>\runtime\runtime.slnx" `
  -config "Debug" -platform "x64" `
  -out "<repo>\runtime\reflection_generated\gen"
```

## vcpkg の場所の上書き

`runtime/property_sheet/nox_common.props` の `NoxVcpkgInstalledDir` は、既定ではリポジトリ直下の `vcpkg_installed` を指す。worktree ごとに入れ直さずに 1 つのインストール済みツリーを共有できるよう、追跡ファイルを編集せずに次の優先順位で差し替えられる。

1. 環境変数 `NOX_VCPKG_INSTALLED_DIR`
2. 追跡外の `runtime/Directory.Build.user.props` で設定する `NoxVcpkgInstalledRoot`
3. 既定 (リポジトリ直下の `vcpkg_installed`)

いずれも指すのは triplet ディレクトリ (`x64-windows`) の **親** である。

```xml
<!-- runtime/Directory.Build.user.props (.gitignore 済み) -->
<Project>
  <PropertyGroup>
    <NoxVcpkgInstalledRoot>E:\shared\vcpkg_installed</NoxVcpkgInstalledRoot>
  </PropertyGroup>
</Project>
```

## EntitySystem / EntityLogic の規則

`Generator/EntityTypeGenerator.cs` が持つ規則。基底が `nox::EntitySystem<` なら System、`nox::EntityLogic<` なら Logic として扱う。エラーが 1 つでも出ると生成器は終了コード 1 を返し、ビルドは失敗する。

### エラー (ビルドを止める)

- **無名名前空間の型** — 生成コードが型名を綴れないため購読できない。名前付き名前空間へ移すか、`nox::EntityLogicMethodTable` の特殊化を手書きする。
- **クラステンプレート** — 同じく購読できない。特殊化を手書きする。(現状のパーサはクラステンプレートを生成器へ渡さないため、この検査は実際には発火しない。テンプレートの Logic は黙って購読されないだけで、誤ったコードは生成されない。)
- **`nox::attr::EntityLogicMethod` を EntityLogic 以外のメソッドに付けた** — 属性は EntityLogic 専用。なお `EntitySystem` でも `EntityLogic` でもない素のクラスに付けた場合は、そもそも生成器の対象外なので何も言わずに無視される。エラーになるのは `EntitySystem` の派生型に付けたときである。

### 警告 (ビルドは通る)

- **`nox::EntityLogic` を継承しているが `nox::attr::EntityLogicMethod` を付けたメソッドが 1 つも無い** — 購読対象が無いだけなので失敗はさせない。その型は購読されず、`entity_type_*.g.cpp` も生成されない。

### 検査しないこと

生成器は「型と属性付きメソッドの数え上げ」だけを行う。更新メソッドのアクセス指定 (private でよい)、引数リストの妥当性、デフォルト構築可能性などは C++ 側の `static_assert` で検査する (`runtime/core/entity_logic.h` を参照)。
