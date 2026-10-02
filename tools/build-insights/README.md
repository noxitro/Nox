# ビルド計測 (C++ Build Insights)

runtime の C++ ビルドで、どのヘッダ・テンプレート・関数に時間がかかっているかを計測してレポートにする。

- CI では push ごとに、MSVC の Debug / Release / Master のビルドを計測している。テンプレートの展開は負荷が大きいので Debug でだけ記録する (`.github/workflows/ci.yml` の `Start Build Insights` から `Upload Build Insights data` まで、と `build-insights` ジョブ)。計測のためにビルドを増やしてはいない。
- レポートは 1 ファイルの HTML。CI の実行ページの Artifacts にある `build-insights-report.html` を落として開く。master の最新は GitHub Pages の `/build-insights/` でも見られる。
- 実行ページの Summary には、構成ごとの重いヘッダ・テンプレート・関数の上位と、前回の master との比較が出る。

## レポートの見方

| タブ | 中身 |
|---|---|
| 概要 | 全体の時間、重いヘッダ、PCH に入れる候補、重いテンプレート・関数・翻訳単位 |
| ヘッダ | ヘッダごとの解析時間。行を開くと、そのヘッダを直接 `#include` しているファイルが出る |
| インクルードツリー | 翻訳単位ごとの取り込みの木。ヘッダのタブから「経路を見る」で、そのヘッダまでの経路を開ける |
| テンプレート | primary template ごとの展開時間。行を開くと、重い特殊化と展開した場所が出る |
| 関数 | コード生成に時間がかかった関数。`/GL` の構成 (Release / Master) ではリンク時 (LTCG) に出る |
| 翻訳単位 | .cpp ごとのフロントエンド・バックエンドの時間 |
| タイムライン | 翻訳単位とリンクの並び (並列度の確認用) |
| 差分 | 前回の master の計測との比較 |

時間の列:

- **合計**: 全翻訳単位での和。並列でビルドしているので実時間より大きい。
- **自身**: 中で取り込んだヘッダ (テンプレートなら、中で展開した別のテンプレート) を除いた時間。
- **按分**: 同時に走っていたもの同士で実時間を分けた値。ビルドの実時間への寄与。

PCH に入っているヘッダは、PCH を作る翻訳単位でだけ解析され、ほかの翻訳単位では数えない (「PCH 内」の印が付く)。PCH の外で多くの翻訳単位が取り込んでいる重いヘッダが「PCH 候補」になる。

共有ランナーの実行時間は ±10% 程度揺れる。差分は時間だけでなく「全体に占める割合」の変化も見る。

## 手元で計測する

Windows と Visual Studio (C++ ワークロード)、PowerShell 7 (`pwsh`) が要る (下の手順の `utf8NoBOM` は Windows PowerShell 5.1 に無い)。計測 (ETW) には管理者権限が要るので、管理者として起動した `pwsh` で実行する。

ふつうの `pwsh` では `msbuild` に PATH が通っていない (`build.ps1` が読む開発者環境はその中の `cmd` にだけ効く)。先に開発者環境を読み込んでおく。

```powershell
# Visual Studio の開発者環境 (msbuild / cl) をこの pwsh に読み込む
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath
Import-Module (Join-Path $vs 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
```

```powershell
# 計測ツールをビルドする (SDK は NuGet から取得。版と SHA256 は build.ps1 に固定)
tools\build-insights\build.ps1

$bi = 'tools\build-insights\out\nox_build_insights.exe'
& $bi start NoxLocal                 # テンプレートを取らないなら --no-templates
msbuild runtime\runtime.slnx /p:Configuration=Debug /p:Platform=x64 /m /t:Rebuild
& $bi stop NoxLocal $env:TEMP\raw.etl
New-Item -ItemType Directory -Force bi-local | Out-Null
& $bi analyze $env:TEMP\raw.etl bi-local\bi-Debug.json
@{ config = 'Debug'; workspace = (Get-Location).Path } | ConvertTo-Json | Set-Content bi-local\bi-Debug.meta.json -Encoding utf8NoBOM

python .github\scripts\build-insights-report.py --data-dir bi-local --out-html bi-local\report.html
```

差分ビルドでは変わった翻訳単位しか計測されないので、全体を見るときは Rebuild する。

同じ生の ETL は、Visual Studio に付属の計測ツール (`vcperf /analyze <raw.etl> <out.etl>`) で WPA 用に変換して深掘りすることもできる。CI では手動実行 (Actions の「Run workflow」) のときだけ、生の ETL を `build-insights-etl-<構成>` として 7 日間残している。

## レポートの見た目を直す

ページのソースは `site/` (index.html / style.css / app.js)。`build-insights-report.py` が CSS・JS・データを 1 ファイルに埋め込む。Windows でなくても、デモデータで確かめられる。

```sh
python3 tools/build-insights/make-demo-data.py --out /tmp/bi-demo
python3 tools/build-insights/make-demo-data.py --out /tmp/bi-base --seed 2
python3 .github/scripts/build-insights-report.py --data-dir /tmp/bi-demo --base-dir /tmp/bi-base \
    --out-html /tmp/bi-demo/report.html
```

## 構成

| ファイル | 役割 |
|---|---|
| `nox_build_insights.cpp` | 計測の開始・停止と集計 (SDK を直接呼ぶ)。集計結果を JSON で出す |
| `build.ps1` | SDK の取得 (版・ハッシュ固定) と、cl による計測ツールのビルド |
| `site/` | レポートのページ |
| `make-demo-data.py` | 見た目の確認用のデモデータ |
| `.github/scripts/build-insights-report.py` | JSON を HTML と CI の要約にする。パスの正規化・前回との差分もここ |

VS 同梱の計測ツールではなく SDK を直接呼んでいるのは、ランナーの VS の版に左右されないことと、按分した時間を集計に使えるため。
