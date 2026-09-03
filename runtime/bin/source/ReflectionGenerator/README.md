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

`RuntimeTypeDB` (`../RuntimeTypeDB/RuntimeTypeDB.csproj`) を変更したときは `runtime/bin/RuntimeTypeDB.dll` も同時に入れ替えること (`ReflectionGenerator.csproj` の ProjectReference なので、同じ `bin/Debug/net10.0/` に出ている)。

```powershell
Copy-Item -Force ".\bin\Debug\net10.0\RuntimeTypeDB.dll", ".\bin\Debug\net10.0\RuntimeTypeDB.pdb" "..\..\"
```

なお Editor (`Editor/Core/Core.csproj`) はこの DLL を `runtime/bin/source/RuntimeTypeDB/bin/Release/net10.0/RuntimeTypeDB.dll` から参照している。TypeDB の読み書きに手を入れたときは **Release 構成もビルドし直さないと Editor 側が古い実装のまま**になる。

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

## TypeDB (ツールが読むバイナリ) の置き場所

生成の最後に `RuntimeTypeDBHelper.Serialize` が型情報を MessagePack で書き出す。これも以前は `%TEMP%\RuntimeTypeDB.bin` という 1 台に 1 つしかない固定パスで、複数 worktree / Debug と Release の同時ビルドが互いを上書きしていた。現在は前処理データと同じ方針で、生成出力ディレクトリの下へ構成別に置く。

```
runtime/reflection_generated/gen/RuntimeTypeDB.<Platform>.<Configuration>.bin
```

読み手 (Editor) はビルドしたツリーの場所を知らないため、書き手が `%TEMP%` へ**実体のフルパスだけを書いたポインタファイル**を毎回置き直す。

```
%TEMP%\RuntimeTypeDB.<Platform>.<Configuration>.path.txt
```

`ReflectionGenerator.RuntimeTypeDB.Util` の解決順は次のとおり。

1. 明示指定 (`Deserialize(outputGenerateDir, platform, configuration)` / `Serialize` の引数)
2. 環境変数 `NOX_RUNTIME_TYPEDB_DIR` (生成出力ディレクトリを指す)
3. `%TEMP%` のポインタファイル (最後にビルドしたツリーが指される)

複数ツリーを行き来するときは、Editor 側のプロセスに `NOX_RUNTIME_TYPEDB_DIR` を設定して読み先を固定するのが確実である。

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
- **クラステンプレート** — 同じく購読できない。特殊化を手書きする。パーサはクラステンプレートの宣言を `IDeclarationContainer.TemplateRecordList` に載せ、生成器はそこを見て診断する (メンバまでは走査していないので、名前・基底・ソース位置だけを当てにする)。基底が依存型で `BaseSpecifierDecl` を作れないため、`TemplateClassDecl.BaseTypeNameList` に基底の綴りだけを別途持たせている。
- **`nox::attr::EntityLogicMethod` を EntityLogic 以外のメソッドに付けた** — 属性は EntityLogic 専用。`EntitySystem` の派生型でも、`EntitySystem` でも `EntityLogic` でもない素のクラスでもエラーになる。属性の数え上げは基底に関わらず行い、購読対象かどうかはその後で判定する (黙って無視すると、更新メソッドが呼ばれない理由が分からなくなるため)。

### 警告 (ビルドは通る)

- **`nox::EntityLogic` を継承しているが `nox::attr::EntityLogicMethod` を付けたメソッドが 1 つも無い** — 購読対象が無いだけなので失敗はさせない。その型は購読されず、`entity_type_*.g.cpp` も生成されない。

### 検査しないこと

生成器は「型と属性付きメソッドの数え上げ」だけを行う。更新メソッドのアクセス指定 (private でよい)、引数リストの妥当性、デフォルト構築可能性などは C++ 側の `static_assert` で検査する (`runtime/core/entity_logic.h` を参照)。
