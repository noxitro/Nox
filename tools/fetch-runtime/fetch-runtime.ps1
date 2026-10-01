<#
.SYNOPSIS
	CI が master で建てた runtime.exe と TypeDB を取ってきて、手元でビルドせずに Editor から使えるようにする。

.DESCRIPTION
	ci.yml の build ジョブ (MSVC / Debug・Release) が master で上げるアーティファクト
	runtime-MSVC-<構成> の最新を取り、次の場所へ置く。
	- runtime/build/runtime/x64/<構成>/runtime.exe (と runtime.pdb)
	  Editor (Core.RuntimeSession.RuntimeExecutablePath) と FlaUI が見る場所
	- runtime/reflection_generated/gen/RuntimeTypeDB.x64.<構成>.bin
	さらに %TEMP% のポインタファイルを書き直し、Editor が TypeDB を見つけられるようにする。

	Editor だけを触るときの手元確認用。runtime 本体を変えたときは手元でビルドすること。
	アーティファクトには CI の runtime_key (ci.yml の plan ジョブ) が入っている。
	手元の HEAD から求めたものと違えば、runtime 側の入力が master の成果物と食い違って
	いる (Editor と runtime のプロトコルがずれているかもしれない) ので警告する。

	gh (GitHub CLI) にログインしている必要がある。

.PARAMETER Configuration
	取ってくる構成。Debug / Release (複数可)。既定は Debug。

.EXAMPLE
	pwsh tools/fetch-runtime/fetch-runtime.ps1
	pwsh tools/fetch-runtime/fetch-runtime.ps1 -Configuration Debug,Release
#>
param(
	[ValidateSet('Debug', 'Release')]
	[string[]]$Configuration = @('Debug')
)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
$OutputEncoding = [Text.UTF8Encoding]::new($false)

function Fail([string]$Message)
{
	Write-Host "fetch-runtime: $Message" -ForegroundColor Red
	exit 1
}

if (-not (Get-Command gh -ErrorAction SilentlyContinue)) { Fail 'gh (GitHub CLI) が見つからない' }
if (-not (Get-Command git -ErrorAction SilentlyContinue)) { Fail 'git が見つからない' }

$root = (& git -C $PSScriptRoot rev-parse --show-toplevel 2>$null)
if (-not $root) { Fail 'git のリポジトリの中で実行すること' }
$root = (Resolve-Path $root).Path

$repo = (& gh repo view --json nameWithOwner --jq .nameWithOwner 2>$null)
if (-not $repo) { Fail 'gh でリポジトリを特定できない (gh auth login 済みか確かめる)' }

# ci.yml の plan ジョブ (Compute runtime key) と同じ求め方。変えるときは両方そろえること。
# トップレベルから runtime に効かない項目を除き、tools は 1 段掘って一部を除く。
# 各行は "mode type sha<TAB>name"、末尾に改行を 1 つ付けて sha256 し、先頭 40 桁を使う。
function Get-RuntimeKey
{
	$exclude = '^(Editor|docs|LICENSE|tools|\.claude|\.editorconfig|\.gitignore|\.githooks-allow|\.git-blame-ignore-revs|\.gitleaks\.toml|startup\.ps1|[^/]*\.md)$'
	$toolsExclude = '^(bench-site|ci-try|fetch-runtime|git-hooks)$'
	$lines = @()
	foreach ($line in (& git -C $root ls-tree HEAD))
	{
		$name = $line.Split("`t", 2)[1]
		if ($name -cnotmatch $exclude) { $lines += $line }
	}
	foreach ($line in (& git -C $root ls-tree HEAD:tools))
	{
		$meta, $name = $line.Split("`t", 2)
		if ($name -cnotmatch $toolsExclude) { $lines += "$meta`ttools/$name" }
	}
	$bytes = [Text.Encoding]::UTF8.GetBytes(($lines -join "`n") + "`n")
	# Windows PowerShell 5.1 でも動くよう、.NET 5 以降の HashData / ToHexString は使わない
	$hash = [Security.Cryptography.SHA256]::Create().ComputeHash($bytes)
	return (-join ($hash | ForEach-Object { $_.ToString('x2') })).Substring(0, 40)
}

$localKey = Get-RuntimeKey
$dirty = & git -C $root status --porcelain -- runtime vcpkg.json vcpkg-configuration.json
if ($dirty) { Write-Host 'fetch-runtime: runtime 以下に未コミットの変更がある。取ってくる成果物には入っていない。' -ForegroundColor Yellow }

foreach ($cfg in $Configuration)
{
	$name = "runtime-MSVC-$cfg"
	Write-Host "--- $name ---"

	# bench-publish.yml と同じく、master の期限切れでない最新を取る。
	# run の成否では絞らない (FlaUI や通知だけ落ちた run の成果物も runtime としては検証済み)。
	$json = & gh api "repos/$repo/actions/artifacts?name=$name&per_page=30"
	if ($LASTEXITCODE -ne 0) { Fail "アーティファクトの一覧を取れない: $name" }
	$artifact = ($json | ConvertFrom-Json).artifacts |
		Where-Object { -not $_.expired -and $_.workflow_run.head_branch -eq 'master' } |
		Select-Object -First 1
	if (-not $artifact) { Fail "master の $name が無い (期限切れか、まだ一度も上がっていない)" }

	$runId = $artifact.workflow_run.id
	$tmp = Join-Path ([IO.Path]::GetTempPath()) "nox-fetch-runtime-$cfg-$([Guid]::NewGuid().ToString('N'))"
	try
	{
		& gh run download $runId --repo $repo -n $name -D $tmp
		if ($LASTEXITCODE -ne 0) { Fail "ダウンロードに失敗: run $runId / $name" }

		$outDir = Join-Path $root "runtime/build/runtime/x64/$cfg"
		$genDir = Join-Path $root 'runtime/reflection_generated/gen'
		New-Item -ItemType Directory -Force $outDir, $genDir | Out-Null

		# アーティファクト内の階層には頼らず、ファイル名で探して置く
		$placed = @{}
		foreach ($file in Get-ChildItem $tmp -Recurse -File)
		{
			$dest = switch -Regex ($file.Name)
			{
				'^runtime\.(exe|pdb)$' { $outDir }
				'^runtime_key\.txt$' { $null }
				'^RuntimeTypeDB\.x64\..+\.bin$' { $genDir }
				default { $null }
			}
			if ($dest)
			{
				Copy-Item -Force $file.FullName $dest
				$placed[$file.Name] = Join-Path $dest $file.Name
			}
		}
		if (-not $placed['runtime.exe']) { Fail "$name に runtime.exe が入っていない" }
		$typeDbName = "RuntimeTypeDB.x64.$cfg.bin"
		if (-not $placed[$typeDbName]) { Fail "$name に $typeDbName が入っていない" }

		$keyFile = Get-ChildItem $tmp -Recurse -File -Filter runtime_key.txt | Select-Object -First 1
		$remoteKey, $remoteSha = if ($keyFile) { (Get-Content $keyFile.FullName) | ForEach-Object { $_.Trim() } } else { $null, $null }
		Write-Host "run $runId (commit $remoteSha)"
		foreach ($path in $placed.Values) { Write-Host "  -> $path" }

		if ($remoteKey -and $remoteKey -ne $localKey)
		{
			Write-Host ("fetch-runtime: 手元の HEAD と runtime 側の入力が違う (手元 {0} / 成果物 {1})。" -f $localKey, $remoteKey) -ForegroundColor Yellow
			Write-Host '  master を取り込むか、runtime を手元でビルドすること。Editor と噛み合わないことがある。' -ForegroundColor Yellow
		}

		# TypeDB のポインタファイル。名前と中身 (.bin のフルパス) は
		# runtime/bin/source/RuntimeTypeDB/TypeDB.cs の Util (FILE_BASE_NAME / POINTER_FILE_EXTENSION /
		# WritePointerFile) に合わせる。
		$pointer = Join-Path ([IO.Path]::GetTempPath()) "RuntimeTypeDB.x64.$cfg.path.txt"
		[IO.File]::WriteAllText($pointer, $placed[$typeDbName], [Text.UTF8Encoding]::new($false))
		Write-Host "  pointer: $pointer"
	}
	finally
	{
		if (Test-Path $tmp) { Remove-Item -Recurse -Force $tmp }
	}
}

if ($env:NOX_RUNTIME_TYPEDB_DIR)
{
	Write-Host "fetch-runtime: NOX_RUNTIME_TYPEDB_DIR ($env:NOX_RUNTIME_TYPEDB_DIR) が設定されているので、Editor はポインタファイルよりそちらを優先する。" -ForegroundColor Yellow
}
Write-Host 'fetch-runtime: 完了。Editor を起動すれば取ってきた runtime.exe が使われる。'
