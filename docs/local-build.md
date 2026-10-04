# ローカルでのビルド

エージェントは通常ローカルでビルド・テストしない (検証は CI に任せる。AGENTS.md の「検証」)。ユーザーが明示的に頼んだときだけ、以下に従う。ビルドコマンドと構成は AGENTS.md の「環境とビルド」を参照。

## ワークツリーでビルドする

- `OutDir` は `$(SolutionDir)build\` なのでワークツリーごとに独立する。
- vcpkg は環境変数 `NOX_VCPKG_INSTALLED_DIR` でメインのチェックアウトのものを共有できる。
- 生成器のバイナリ (`runtime/bin/`) は `.gitignore` 済みでワークツリーには無いので、ワークツリーで `startup.ps1` を実行して建てる。メインのチェックアウトからコピーすると、ワークツリー側のソースのほうが新しい扱いになり、ビルド時の鮮度検査 (`NoxCheckCodeGenerator`) で止まる。

## マシン固有の設定

このマシンだけに当てはまる指示 (vcpkg のパス、並列数の上限など) は、リポジトリ直下の `CLAUDE.local.md` に書く。`.gitignore` 済みなのでコミットされないが、そのぶん `git worktree add` したツリーには現れない。ワークツリーで作業するエージェントにも効かせたい指示は `~/.claude/CLAUDE.md` に書く。

## ビルドせずに Editor を動かす

Editor だけを触るときは、CI が master で建てた `runtime.exe` と TypeDB を取ってくれば runtime をビルドしなくてよい。

```powershell
pwsh tools/fetch-runtime/fetch-runtime.ps1                          # Debug
pwsh tools/fetch-runtime/fetch-runtime.ps1 -Configuration Debug,Release
```

- `gh` にログインしている必要がある。取るのは master の最新のアーティファクト `runtime-MSVC-<構成>` (90 日で消える)。
- `runtime/build/runtime/x64/<構成>/` と `runtime/reflection_generated/gen/` に置き、`%TEMP%` の TypeDB ポインタファイルを書き直す。
- 手元の HEAD と runtime 側の入力が master と違えば警告する。そのときは master を取り込むか、runtime を手元でビルドする。
