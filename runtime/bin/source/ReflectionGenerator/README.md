# ReflectionGenerator

## これは何か

`ReflectionGenerator` は、C++ のヘッダを libclang (ClangSharp) で解析して、リフレクション情報と ECS の購読テーブルを C++ ソースとして書き出すコード生成器である。MSBuild のカスタムタスク `Nox.CustomTask.Task` が `reflection_generated` プロジェクトのコンパイル直前 (`runtime/reflection_generated/Directory.Build.targets` の `ReflectionGeneratorPreProcess` ターゲット) に走り、インクルードパス・プリプロセッサ定義・構成などのビルド情報を前処理データ (バイナリ) へ書き出す。続いて `ReflectionGenerator.exe` が同じ前処理データを読み、`runtime/reflection_generated/gen/<Platform>/<Configuration>/` へ `class_decl_*.g.cpp` / `global_decl_*.g.cpp` / `entity_type_*.g.cpp` / `entity_type_table.g.cpp` を出力する。生成された `entity_type_*.g.cpp` は同じターゲット内で `ClCompile` へ追加されるため、ヘッダに型を定義するだけで `World` へ購読される (登録マクロも静的初期化も要らない)。

## ビルドと配置

リポジトリ直下の `startup.ps1` (ダブルクリック用は `startup.bat`) を実行する。CustomTask → ReflectionGenerator (+ ProjectReference の RuntimeTypeDB) の順に **Debug** 構成でビルドし、出力を `runtime/bin/` へ配置する。CI (`.github/actions/setup-nox-build`) も同じスクリプトを呼ぶ。

```powershell
./startup.ps1
```

- 配置先のバイナリは .gitignore 済み (リポジトリに入っていない) なので、`dotnet build` だけでは `runtime/bin/` は更新されない。
- 配置するのは `bin/Debug/net10.0/` の中身すべて。dotnet build の出力は単一ファイルではなく、ClangSharp / MessagePack / Microsoft.Build と `runtimes/win-x64` の libclang ネイティブが揃って初めて動く。
- ReflectionGenerator と CustomTask は前処理データの読み書きを共有しているので、片方だけを入れ替えない。`startup.ps1` は常に両方を建てる。
- 生成器のソースが配置済みのバイナリより新しいと、`reflection_generated` のビルドがエラー (`runtime/reflection_generated/Directory.Build.targets` の `NoxCheckCodeGenerator`) で止まる。判定は、生成器のソースと `startup.ps1` が最後に書いた `runtime/bin/startup.stamp` の更新日時の比較。
- Editor (`Editor/Core/Core.csproj`) は RuntimeTypeDB を ProjectReference で参照しているので、Editor のビルドで建て直される。

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

`-typedb false` を渡すと、後述の TypeDB と `%TEMP%` のポインタファイルを書かない (省略時は `true`)。
テスト用の型を含む `reflection_generated_test` はこれを渡す (`NoxReflectionWriteTypeDB=false`)。
書くと、Editor が最後に建てたテスト版の TypeDB を読んでしまうため。

## 解析に使うインクルードパス (ツールセット非依存)

リフレクション情報は「どの型・メンバが存在するか」を表すものであり、ビルドに使う C++ コンパイラによって変わってはならない。そのため解析用のシステムインクルードパスは `$(PlatformToolset)` に依存させず、`Directory.Build.targets` の `ReflectionParseIncludePath` (= `$(VC_IncludePath);$(WindowsSDK_IncludePath)`) に固定している。`$(IncludePath)` は選択中のツールセットが決める値なので使わない。

ツールセットの値が漏れ込む経路は 2 つあり、両方を塞ぐ必要がある。

1. カスタムタスクへ渡す `AdditionalIncludeDirectories` (前処理データ経由で `-I` になる)
2. `ReflectionGenerator.exe` を起動する `Exec` の環境変数 `INCLUDE` — C++ ビルド中の MSBuild は `INCLUDE` に `$(IncludePath)` を書き出しており、libclang は MSVC 互換ドライバとしてこれをシステムインクルードパスとして読む。`-I` を明示しても此処から拾ってしまうため `Exec` の `EnvironmentVariables` で上書きしている。

塞がないと `PlatformToolset=ClangCL` のとき VS 同梱 clang の組み込みヘッダ (`VC\Tools\Llvm\x64\lib\clang\<major>\include`) が解析対象に入る。生成器が解析に使う libclang (ClangSharp) は同梱版とバージョンが異なるため、新しい組み込み関数 (`__builtin_elementwise_fshl` など) を知らずに解析が失敗する。

MSVC でビルドした場合 `ReflectionParseIncludePath` は `$(IncludePath)` と同じ値になるので、生成結果は変わらない。

## libclang (ClangSharp) の版を上げるとき

解析に使う libclang は `ReflectionGenerator.csproj` の 4 つのパッケージ (`ClangSharp` / `ClangSharp.Interop` / `libclang.runtime.win-x64` / `libClangSharp.runtime.win-x64`) で決まる。版は揃えて上げる (`libClangSharp.runtime.win-x64` は `ClangSharp.Interop` の nuspec が求める版にする)。

- MSVC の標準ライブラリは、対応する clang の最低の版を `yvals_core.h` の static_assert (STL1000) で確かめる。ランナーの MSVC が上がると、古い libclang では解析の最初で止まる。
- 型の完全修飾名や宣言の並びは、生成コードと RuntimeTypeDB を通じて保存データと Editor の通信に載る。生成器を変えた push では CI の Generator diff ジョブが、変える前の生成器と生成物を比べるので、差分が無いか、意図した差分だけかを確かめてから入れる (`.github/workflows/ci.yml` のジョブのコメント)。
- 型から宣言を引くときは `CXType.Declaration` ではなく `GetDeclaration()` (`Parser/ClangSharpExtension.cs`) を使う。libclang 22 から、型が返す宣言は定義とは限らない (正規の型なら最初の宣言、ソースに書いた型なら名前で見つけた宣言) ので、定義 (無ければ最初の宣言) に揃えている。宣言のキー (`UniqueDeclKey`) はソース上の位置を含むので、揃えないと登録した定義のキーと食い違う。
- libclang 21 までは、ソースに書いた型名 (別名・修飾付きの型・テンプレートの特殊化) が `Elaborated` に包まれて届いた。22 では型そのものが修飾を持つ形で届くので、`IsWrittenTypeName()` (`Parser/CppParser2.cs`) で見分けて正規の型へ進める。

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

`Generator/EntityTypeGenerator.cs` が持つ規則。基底が `nox::legacy::EntitySystem<` なら System、`nox::legacy::EntityLogic<` なら Logic として扱う (新しい形の `nox::EntitySystem<` / `nox::EntityLogic<` はまだ拾わない)。エラーが 1 つでも出ると生成器は終了コード 1 を返し、ビルドは失敗する。

### エラー (ビルドを止める)

- **無名名前空間の型** — 生成コードが型名を綴れないため購読できない。名前付き名前空間へ移すか、`nox::legacy::EntityLogicMethodTable` の特殊化を手書きする。
- **クラステンプレート** — 同じく購読できない。特殊化を手書きする。パーサはクラステンプレートの宣言を `IDeclarationContainer.TemplateRecordList` に載せ、生成器はそこを見て診断する (メンバまでは走査していないので、名前・基底・ソース位置だけを当てにする)。基底が依存型で `BaseSpecifierDecl` を作れないため、`TemplateClassDecl.BaseTypeNameList` に基底の綴りだけを別途持たせている。
- **`nox::legacy::attr::EntityLogicMethod` を EntityLogic 以外のメソッドに付けた** — 属性は EntityLogic 専用。`EntitySystem` の派生型でも、`EntitySystem` でも `EntityLogic` でもない素のクラスでもエラーになる。属性の数え上げは基底に関わらず行い、購読対象かどうかはその後で判定する (黙って無視すると、更新メソッドが呼ばれない理由が分からなくなるため)。

### 警告 (ビルドは通る)

- **`nox::legacy::EntityLogic` を継承しているが `nox::legacy::attr::EntityLogicMethod` を付けたメソッドが 1 つも無い** — 購読対象が無いだけなので失敗はさせない。その型は購読されず、`entity_type_*.g.cpp` も生成されない。

### 検査しないこと

生成器は「型と属性付きメソッドの数え上げ」だけを行う。更新メソッドのアクセス指定 (private でよい)、引数リストの妥当性、デフォルト構築可能性などは C++ 側の `static_assert` で検査する (`runtime/core/entity_logic_legacy.h` を参照)。
