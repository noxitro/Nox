# git で管理していないビルドの前提物を用意するスクリプト
#
# runtime/bin/source にある C# 製のコード生成器を建てて runtime/bin へ配置する。
#   1. CustomTask          : reflection_generated のビルドで MSBuild が読み込むカスタムタスク
#   2. ReflectionGenerator : リフレクション情報を C++ ソースとして書き出す生成器
#                            (ProjectReference の RuntimeTypeDB も同時に建つ)
#
# 配置するバイナリは .gitignore 済みでリポジトリに入っていない。生成器のソースを pull しても
# 配置済みのバイナリは古いままなので、初回と、生成器のソースが変わったときに実行する。
# 古いまま runtime をビルドすると、reflection_generated のビルドがエラーで止まって知らせる
# (runtime/reflection_generated/Directory.Build.targets の NoxCheckCodeGenerator)。
#
# CI (.github/actions/setup-nox-build) も同じスクリプトを呼ぶ。手順を変えるときはここだけを直す。

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSCommandPath
$binDir = Join-Path $repoRoot 'runtime\bin'
$sourceDir = Join-Path $binDir 'source'

# 配置を終えた印。ビルド時の鮮度検査は、生成器のソースがこれより新しいかを見る
$stampFile = Join-Path $binDir 'startup.stamp'

function Invoke-DotnetBuild
{
	param([string]$Project)

	dotnet build $Project -c Debug -v minimal -nologo
	if ($LASTEXITCODE -ne 0)
	{
		throw "$Project のビルドに失敗しました (exit $LASTEXITCODE)"
	}
}

# $BaseDir 以下のファイルを、相対パスを保ったまま $To へ写す。
# 中身の変わっていないファイル (サイズと更新日時が同じ) は写さない。CustomTask.dll は
# Visual Studio の MSBuild がビルド中に読み込んで掴んだままにするので、毎回上書きすると
# VS を開いている限り失敗する。
function Copy-ChangedFiles
{
	param([string]$BaseDir, [System.IO.FileInfo[]]$Files, [string]$To)

	$base = (Resolve-Path -LiteralPath $BaseDir).Path.TrimEnd('\') + '\'
	$copied = 0
	foreach ($file in $Files)
	{
		$target = Join-Path $To ($file.FullName.Substring($base.Length))
		$existing = Get-Item -LiteralPath $target -ErrorAction SilentlyContinue
		if ($existing -and $existing.Length -eq $file.Length -and $existing.LastWriteTimeUtc -eq $file.LastWriteTimeUtc)
		{
			continue
		}

		New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
		try
		{
			Copy-Item -LiteralPath $file.FullName -Destination $target -Force
		}
		catch
		{
			throw "$target を上書きできません。Visual Studio・Editor など、このファイルを使っているプロセスを閉じてから再実行してください。($($_.Exception.Message))"
		}
		$copied++
	}
	return $copied
}

if (-not (Get-Command dotnet -ErrorAction SilentlyContinue))
{
	throw 'dotnet が見つかりません。.NET 10 SDK を入れてください。'
}

# 1. CustomTask
#    ReflectionGenerator.csproj が runtime/bin/CustomTask.dll を HintPath で参照しているので、先に配置する
Write-Host '[1/2] CustomTask をビルドします'
$customTaskDir = Join-Path $sourceDir 'CustomTask'
Invoke-DotnetBuild (Join-Path $customTaskDir 'CustomTask.csproj')
$customTaskOut = Join-Path $customTaskDir 'bin\Debug\netstandard2.0'
$copied = Copy-ChangedFiles -BaseDir $customTaskOut -Files (Get-Item -LiteralPath (Join-Path $customTaskOut 'CustomTask.dll')) -To $binDir
Write-Host "      $copied 個のファイルを配置しました"

# 2. ReflectionGenerator
#    出力ディレクトリを丸ごと写す。dotnet build の出力は単一ファイルではなく、ClangSharp /
#    MessagePack / Microsoft.Build と runtimes\win-x64 の libclang ネイティブが揃って初めて動く
Write-Host '[2/2] ReflectionGenerator をビルドします'
$generatorDir = Join-Path $sourceDir 'ReflectionGenerator'
Invoke-DotnetBuild (Join-Path $generatorDir 'ReflectionGenerator.csproj')
$generatorOut = Join-Path $generatorDir 'bin\Debug\net10.0'
$copied = Copy-ChangedFiles -BaseDir $generatorOut -Files (Get-ChildItem -LiteralPath $generatorOut -Recurse -File) -To $binDir
Write-Host "      $copied 個のファイルを配置しました"

# 期待どおり置けたか確かめる。欠けたまま進むと、後段で原因の見えにくいビルドエラーに化ける
foreach ($name in 'CustomTask.dll', 'ReflectionGenerator.exe', 'ReflectionGenerator.dll', 'RuntimeTypeDB.dll')
{
	if (-not (Test-Path -LiteralPath (Join-Path $binDir $name)))
	{
		throw "runtime/bin/$name が配置されていません"
	}
}

Set-Content -LiteralPath $stampFile -Value (Get-Date -Format o)
Write-Host "完了: $binDir"
