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

.PARAMETER Branch
	push 先のブランチ。master / main は拒否する。

.PARAMETER NoWatch
	push して実行の URL を出したら終わる (結果を待たない)。

.PARAMETER NoUntracked
	未追跡のファイルを含めない (追跡中のファイルの変更と削除だけ)。

.PARAMETER Yes
	push 前の y/n 確認を飛ばす。検査は飛ばさない。

.EXAMPLE
	pwsh tools/ci-try/ci-try.ps1
#>
param(
	[string]$Branch = 'user/ci-try',
	[switch]$NoWatch,
	[switch]$NoUntracked,
	[switch]$Yes
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

if ($Branch -match '^(refs/heads/)?(master|main)$') { Fail "$Branch へは push しない" }
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
	& $sh tools/git-hooks/gitleaks.sh "--log-opts=$base..$commit"
	if ($LASTEXITCODE -ne 0) { $scanOk = $false }
}
if (-not $scanOk) { Fail '検査で止まった。上の出力を見て直してから実行し直す (push はしていない)' }
Write-Host 'ci-try: 検査は通った' -ForegroundColor Green

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
