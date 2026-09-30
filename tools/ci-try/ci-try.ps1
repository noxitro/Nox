<#
.SYNOPSIS
	いまの作業ツリーをそのまま CI に投げて、結果を待つ。

.DESCRIPTION
	未コミットの変更も含めた作業ツリーのスナップショットを 1 コミットにして、
	試し用のブランチ (既定 user/ci-try) へ force-push する。master には触れない。
	手元の HEAD・index・作業ツリー・stash は一切変えない (一時 index で組み立てる)。
	.gitignore で除外されたファイルは含まれない。

	公開リポジトリへ上げるので、push の前に次を必ず通す。
	- pre-push フック (tools/git-hooks) が有効であること
	- origin/master からの差分全体を tools/git-hooks の scan.sh と gitleaks で検査する
	  (フックは前回の push との差分しか見ないので、それに頼らず全体を見る)
	- 含まれる未追跡のファイルを一覧で見せ、y/n で確認する
	- (選んだときだけ) 追加された行を Sonnet に読ませ、個人情報・所属先・秘密情報・
	  ライセンスの観点でレビューする。指摘があれば既定で中止し、force と答えたときだけ進む

.PARAMETER Branch
	push 先のブランチ。master / main は拒否する。

.PARAMETER NoWatch
	push して実行の URL を出したら終わる (結果を待たない)。

.PARAMETER NoUntracked
	未追跡のファイルを含めない (追跡中のファイルの変更と削除だけ)。

.PARAMETER Yes
	push 前の y/n 確認を飛ばす。検査は飛ばさない。LLM レビューの指摘は force で
	越えられないので、指摘があれば中止する。

.PARAMETER Review
	LLM レビューをする (聞かない)。

.PARAMETER NoReview
	LLM レビューをしない (聞かない)。どちらも付けなければ最初に聞く (-Yes のときはしない)。

.EXAMPLE
	pwsh tools/ci-try/ci-try.ps1
#>
param(
	[string]$Branch = 'user/ci-try',
	[switch]$NoWatch,
	[switch]$NoUntracked,
	[switch]$Yes,
	[switch]$Review,
	[switch]$NoReview
)

$ErrorActionPreference = 'Stop'
# git の出力 (日本語のパス) を化けさせず、sh へ渡す入力も UTF-8 にする
[Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
$OutputEncoding = [Text.UTF8Encoding]::new($false)

function Fail([string]$Message)
{
	Write-Host "ci-try: $Message" -ForegroundColor Red
	exit 1
}

function Invoke-Git
{
	$out = & git @args
	if ($LASTEXITCODE -ne 0) { Fail "git $($args -join ' ') が失敗した" }
	return $out
}

function Find-Sh
{
	# Git for Windows 同梱の sh。フックと同じものを使う。
	$gitExe = (Get-Command git).Source
	foreach ($rel in '..\bin\sh.exe', '..\usr\bin\sh.exe', '..\..\bin\sh.exe')
	{
		$p = Join-Path (Split-Path $gitExe) $rel
		if (Test-Path -LiteralPath $p) { return (Resolve-Path -LiteralPath $p).Path }
	}
	$cmd = Get-Command sh -ErrorAction SilentlyContinue
	if ($cmd) { return $cmd.Source }
	return $null
}

function Find-Claude
{
	# claude.ps1 経由だと標準入力が渡らないので、npm の claude.cmd を優先する
	$cmd = Get-Command claude.cmd -ErrorAction SilentlyContinue
	if ($cmd) { return $cmd.Source }
	$cmd = Get-Command claude.exe -ErrorAction SilentlyContinue
	if ($cmd) { return $cmd.Source }
	return $null
}

# 追加された行を Sonnet に読ませ、公開してはいけない情報が無いかだけを見る。
# 返り値: 'pass' / 'flag' / 'toolarge' (多すぎてレビューしていない) / 'error'
function Invoke-LeakReview([string]$From, [string]$To)
{
	$claude = Find-Claude
	if (-not $claude) { Write-Host 'ci-try: claude (Claude Code) が見つからない' -ForegroundColor Red; return 'error' }

	# 費用を抑えるため、追加された行だけを渡す。生成物は人が書いた内容を含まないので外す。
	$diff = Invoke-Git -c core.quotePath=false diff -U0 --no-color --no-ext-diff $From $To -- . `
		':(exclude)runtime/reflection_generated/**' ':(exclude)docs/doxygen/**' ':(exclude)*.sha256'
	$lines = foreach ($l in $diff)
	{
		if ($l.StartsWith('+++ ')) { if ($l -ne '+++ /dev/null') { $l } }
		elseif ($l.StartsWith('+')) { $l }
	}
	if (-not ($lines | Where-Object { -not $_.StartsWith('+++ ') }))
	{
		Write-Host 'ci-try: レビューする追加行が無い' -ForegroundColor Cyan
		return 'pass'
	}

	# 1 回に渡す量には上限を置き、超えたら分けて全部を見る (見ないまま push しない)。
	# 分け目はなるべくファイルの境目にし、続きの塊にはファイル名の行を付け直す。
	$limit = 60000
	$maxChunks = 8
	$chunks = [Collections.Generic.List[string]]::new()
	$sb = [Text.StringBuilder]::new()
	$header = ''
	foreach ($l in $lines)
	{
		if ($l.StartsWith('+++ ')) { $header = $l }
		if ($sb.Length -gt 0 -and $sb.Length + $l.Length + 1 -gt $limit)
		{
			$chunks.Add($sb.ToString())
			$sb.Clear() | Out-Null
			if (-not $l.StartsWith('+++ ') -and $header) { $sb.Append($header).Append("`n") | Out-Null }
		}
		$sb.Append($l).Append("`n") | Out-Null
	}
	if ($sb.Length -gt 0) { $chunks.Add($sb.ToString()) }
	if ($chunks.Count -gt $maxChunks)
	{
		Write-Host "ci-try: 追加行が多すぎる ($($chunks.Count) 回分、上限 $maxChunks 回)。レビューしていない" -ForegroundColor Red
		return 'toolarge'
	}

	# 作業ディレクトリをリポジトリの外にして、プロジェクトの AGENTS.md などを読み込ませない。
	# --bare は API キー認証 (従量課金) になるので使わない。サブスクのログインで動かす。
	$work = Join-Path ([IO.Path]::GetTempPath()) 'nox-ci-try-review'
	New-Item -ItemType Directory -Force $work | Out-Null
	$prompt = Join-Path $PSScriptRoot 'review-prompt.md'
	$findings = @()
	$flagged = $false
	for ($i = 0; $i -lt $chunks.Count; $i++)
	{
		$body = $chunks[$i]
		Write-Host "ci-try: Sonnet でレビューする ($($i + 1)/$($chunks.Count)、$($body.Length) 文字)" -ForegroundColor Cyan
		Push-Location $work
		try
		{
			$raw = $body | & $claude -p 'Review the diff on stdin and answer with the JSON only.' `
				--model sonnet --effort low --tools '' --strict-mcp-config --disable-slash-commands `
				--no-session-persistence --system-prompt-file $prompt --output-format json 2>&1 | Out-String
			$code = $LASTEXITCODE
		}
		finally
		{
			Pop-Location
		}
		if ($code -ne 0) { Write-Host "ci-try: レビューに失敗した: $raw" -ForegroundColor Red; return 'error' }

		try
		{
			$res = $raw | ConvertFrom-Json
			if ($res.is_error) { throw "claude がエラーを返した: $($res.result)" }
			$m = [regex]::Match([string]$res.result, '\{[\s\S]*\}')
			if (-not $m.Success) { throw "JSON が返らなかった: $($res.result)" }
			$verdict = $m.Value | ConvertFrom-Json
		}
		catch
		{
			Write-Host "ci-try: レビューの結果を読めなかった: $_" -ForegroundColor Red
			return 'error'
		}
		$found = @($verdict.findings | Where-Object { $_ })
		$findings += $found
		if ($verdict.verdict -ne 'pass' -or $found.Count -gt 0) { $flagged = $true }
	}

	if (-not $flagged)
	{
		Write-Host 'ci-try: レビューの指摘なし' -ForegroundColor Green
		return 'pass'
	}
	Write-Host "ci-try: レビューの指摘 $($findings.Count) 件" -ForegroundColor Red
	foreach ($f in $findings)
	{
		Write-Host "  [$($f.category)] $($f.file)" -ForegroundColor Red
		Write-Host "    $($f.text)" -ForegroundColor Red
		Write-Host "    $($f.reason)" -ForegroundColor Red
	}
	return 'flag'
}

if ($Branch -match '^(refs/heads/)?(master|main)$') { Fail "$Branch へは push しない" }
if ($Review -and $NoReview) { Fail '-Review と -NoReview は同時に付けない' }

# LLM レビューをするか。どちらも指定が無ければ最初に聞く (検査の後で聞くと待たされるので)。
$doReview = $Review.IsPresent
if (-not $Review -and -not $NoReview -and -not $Yes)
{
	$answer = Read-Host 'Sonnet で公開前レビュー (個人情報・所属先・秘密情報・ライセンス) をするか (y/N)'
	$doReview = $answer -match '^(y|yes)$'
}
if (-not (Get-Command gh -ErrorAction SilentlyContinue) -and -not $NoWatch)
{
	Fail 'gh (GitHub CLI) が見つからない。入れるか -NoWatch を付ける'
}

$root = (& git rev-parse --show-toplevel 2>$null)
if ($LASTEXITCODE -ne 0 -or -not $root) { Fail 'git リポジトリの中で実行する' }
Set-Location $root

# --- フックが有効か --------------------------------------------------------
# 自前の検査に加えて push 時にも同じ関門を通すため、無効なら先へ進まない。
$hooks = (Invoke-Git rev-parse --path-format=absolute --git-path hooks).Trim()
$expected = Join-Path $root 'tools/git-hooks'
if ([IO.Path]::GetFullPath($hooks).TrimEnd('\', '/') -ne [IO.Path]::GetFullPath($expected).TrimEnd('\', '/'))
{
	Fail "pre-push フックが有効になっていない (hooks: $hooks)。先に次を実行する: sh tools/git-hooks/install.sh"
}
$sh = Find-Sh
if (-not $sh) { Fail 'sh が見つからない (Git for Windows の bin\sh.exe を探した)' }

# --- スナップショット ------------------------------------------------------
# 一時 index にスナップショットを組み立てる。本物の index をコピーしてから足すので、
# 変更の無いファイルは stat 情報で飛ばされて速い。
$head = (Invoke-Git rev-parse HEAD).Trim()
$realIndex = (Invoke-Git rev-parse --path-format=absolute --git-path index).Trim()
$tmpIndex = Join-Path ([IO.Path]::GetTempPath()) "nox-ci-try-$PID.index"
Copy-Item -LiteralPath $realIndex -Destination $tmpIndex -Force
try
{
	$env:GIT_INDEX_FILE = $tmpIndex
	if ($NoUntracked) { Invoke-Git add -u | Out-Null } else { Invoke-Git add -A | Out-Null }
	$tree = (Invoke-Git write-tree).Trim()
}
finally
{
	Remove-Item Env:GIT_INDEX_FILE -ErrorAction SilentlyContinue
	Remove-Item -LiteralPath $tmpIndex -Force -ErrorAction SilentlyContinue
}

$stamp = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
$commit = (Invoke-Git commit-tree $tree -p $head -m "ci-try: 作業ツリーのスナップショット ($stamp)").Trim()

Write-Host "ci-try: HEAD $($head.Substring(0, 8)) からの差分" -ForegroundColor Cyan
$stat = Invoke-Git -c core.quotePath=false diff --stat $head $commit
if ($stat) { $stat | ForEach-Object { Write-Host "  $_" } } else { Write-Host '  (差分なし。HEAD のまま CI に投げる)' }

# --- 公開前の検査 ----------------------------------------------------------
# origin/master から今回のスナップショットまでに増えた・変わったファイルを全部見る。
# pre-push フックは「前回 push したもの」との差分しか見ないので、それに頼らない。
Invoke-Git fetch --quiet origin master | Out-Null
$base = (Invoke-Git merge-base origin/master $commit).Trim()
$files = @(Invoke-Git -c core.quotePath=false diff --name-only --diff-filter=d $base $commit | Where-Object { $_ })
Write-Host "ci-try: origin/master からの $($files.Count) ファイルを検査する" -ForegroundColor Cyan
$scanOk = $true
if ($files.Count -gt 0)
{
	# PowerShell からパイプで渡すと行末が CRLF になり、scan.sh が各ファイルを読めずに
	# 黙って飛ばす。LF のファイルに書いてリダイレクトで渡す。
	$list = Join-Path ([IO.Path]::GetTempPath()) "nox-ci-try-$PID.scan"
	$body = ($files | ForEach-Object { "$_`t${commit}:$_" }) -join "`n"
	[IO.File]::WriteAllText($list, "$body`n", [Text.UTF8Encoding]::new($false))
	try
	{
		& $sh -c 'sh tools/git-hooks/scan.sh < "$1"' ci-try $list
		if ($LASTEXITCODE -ne 0) { $scanOk = $false }
	}
	finally
	{
		Remove-Item -LiteralPath $list -Force -ErrorAction SilentlyContinue
	}
	# フックは gitleaks が無ければ警告だけで通すが、ci-try は公開の直前の関門なので必須にする。
	# 探す場所は gitleaks.sh の find_gitleaks と揃えてある。
	$gitleaks = @(
		(Get-Command gitleaks -ErrorAction SilentlyContinue).Source,
		(Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Links\gitleaks.exe')
	) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }
	if (-not $gitleaks) { Fail 'gitleaks が見つからない。入れてから実行する: winget install Gitleaks.Gitleaks' }
	& $sh tools/git-hooks/gitleaks.sh "--log-opts=$base..$commit"
	if ($LASTEXITCODE -ne 0) { $scanOk = $false }
}

# コミットメッセージ (外部資料名・手元の非公開リスト)。push される手元のコミット全部を見る。
& $sh -c 'PY=$(sh tools/git-hooks/find-python.sh); [ -z "$PY" ] || "$PY" tools/git-hooks/check-external-names.py --commits "$1"' ci-try "$base..$commit"
if ($LASTEXITCODE -ne 0) { $scanOk = $false }

# 作成者・コミッターのメールアドレス。スナップショット自体も user.email で作られる。
# 部分一致だと noreply.taro@example.jp のようなものまで通るので、許すものを列挙する
# (GitHub の個人用 noreply と、GitHub・Claude がマージやクラウドのセッションで使う noreply)。
$allowedEmail = '(@users\.noreply\.github\.com|^noreply@(github|anthropic)\.com)$'
$emails = @(Invoke-Git log --format='%ae%n%ce' "$base..$commit" | Where-Object { $_ -and $_ -notmatch $allowedEmail } | Sort-Object -Unique)
if ($emails.Count -gt 0)
{
	Write-Host 'ci-try: 公開用 (noreply) でないメールアドレスのコミットがある:' -ForegroundColor Red
	$emails | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
	Write-Host '  git config user.email <ID>+<ユーザー名>@users.noreply.github.com にする' -ForegroundColor Red
	$scanOk = $false
}

if (-not $scanOk) { Fail '検査で止まった。上の出力を見て直してから実行し直す (push はしていない)' }
Write-Host 'ci-try: 検査は通った' -ForegroundColor Green

# --- LLM レビュー --------------------------------------------------------------
# 決まったルールで拾えない、文脈で判断するもの (氏名・職場を示す記述など) を見る。
# 判定はぶれるので、指摘があれば既定で止め、誤検出と判断したときだけ force で進める。
if ($doReview)
{
	$result = Invoke-LeakReview $base $commit
	if ($result -eq 'error') { Fail 'レビューを完了できなかった。レビューなしで投げるなら -NoReview を付ける (push はしていない)' }
	if ($result -eq 'flag' -or $result -eq 'toolarge')
	{
		if ($Yes) { Fail 'レビューで指摘があった、または量が多すぎてレビューできなかった (push はしていない)' }
		$answer = Read-Host '確かめたうえで進めるなら force と入力する。それ以外は中止'
		if ($answer -ne 'force') { Fail 'レビューの指摘で止めた (push はしていない)' }
	}
}

# --- 新しく入る素材 ----------------------------------------------------------
# フォント・モデル・画像・音声は、コードと違ってライセンスを中身から判定できない。
# 一覧を出して、y/n の前に出どころを確かめてもらう。
$assetExt = '\.(ttf|otf|ttc|woff2?|fbx|gltf|glb|blend|max|ma|mb|png|jpe?g|gif|bmp|tga|dds|hdr|exr|psd|wav|ogg|mp3|flac|mp4|mov)$'
$assets = @(Invoke-Git -c core.quotePath=false diff --name-only --diff-filter=A $base $commit | Where-Object { $_ -match $assetExt })
if ($assets.Count -gt 0)
{
	Write-Host "ci-try: 新しい素材 $($assets.Count) 本が入る。ライセンス (再配布できるか・クレジット表記) を確かめる" -ForegroundColor Yellow
	$assets | ForEach-Object { Write-Host "  $_" -ForegroundColor Yellow }
}

# --- 未追跡のファイルの確認 --------------------------------------------------
# 手元のメモやログなど、コミットするつもりの無いファイルがそのまま公開されるのを防ぐ。
if (-not $NoUntracked)
{
	$untracked = @(Invoke-Git -c core.quotePath=false ls-files --others --exclude-standard | Where-Object { $_ })
	if ($untracked.Count -gt 0)
	{
		Write-Host "ci-try: 次の未追跡のファイル $($untracked.Count) 本も push される" -ForegroundColor Yellow
		$untracked | ForEach-Object { Write-Host "  $_" -ForegroundColor Yellow }
	}
	else
	{
		Write-Host 'ci-try: 未追跡のファイルは含まれない' -ForegroundColor Cyan
	}
}

if (-not $Yes)
{
	$answer = Read-Host "公開リポジトリの $Branch へ push する。よいか (y/N)"
	if ($answer -notmatch '^(y|yes)$') { Fail 'やめた (push はしていない)' }
}

# --- push ------------------------------------------------------------------
# 試し用ブランチは毎回上書きする。前の実行は CI の concurrency で打ち切られる。
Write-Host "ci-try: $Branch へ push する" -ForegroundColor Cyan
& git push --force origin "${commit}:refs/heads/$Branch"
if ($LASTEXITCODE -ne 0) { Fail 'push に失敗した (pre-push フックの検査で止まった場合は上の出力を見る)' }

if (-not (Get-Command gh -ErrorAction SilentlyContinue))
{
	Write-Host "ci-try: push した ($($commit.Substring(0, 8)))。結果は GitHub の Actions で見る" -ForegroundColor Green
	exit 0
}

# 実行が登録されるまで少し待つ。文書だけの変更では ci.yml が走らない (paths-ignore)。
$run = $null
for ($i = 0; $i -lt 18 -and -not $run; $i++)
{
	Start-Sleep -Seconds 5
	$json = & gh run list --workflow ci.yml --commit $commit --limit 1 --json databaseId,url 2>$null
	if ($LASTEXITCODE -eq 0 -and $json) { $run = ($json | ConvertFrom-Json) | Select-Object -First 1 }
}
if (-not $run)
{
	Write-Host 'ci-try: 90 秒待っても CI の実行が見つからない。文書だけの変更なら CI (ビルド) は走らない' -ForegroundColor Yellow
	exit 1
}

Write-Host "ci-try: $($run.url)" -ForegroundColor Cyan
if ($NoWatch) { exit 0 }

Write-Host 'ci-try: 結果を待つ (いつもは 11〜14 分。Ctrl+C で待つのをやめても CI は続く)' -ForegroundColor Cyan
& gh run watch $run.databaseId --exit-status --interval 30 --compact
$code = $LASTEXITCODE
if ($code -eq 0)
{
	Write-Host 'ci-try: 合格' -ForegroundColor Green
}
else
{
	Write-Host "ci-try: 不合格。失敗したジョブのログ: gh run view $($run.databaseId) --log-failed" -ForegroundColor Red
}
exit $code
