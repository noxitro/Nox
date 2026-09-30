<#
.SYNOPSIS
	計測ツール nox_build_insights.exe をビルドする。

.DESCRIPTION
	C++ Build Insights SDK を NuGet から取得し (版と SHA256 を固定)、cl で直接ビルドする。
	出力先には exe と、実行に要る SDK の DLL (CppBuildInsights.dll / KernelTraceControl.dll) を置く。
	SDK と出力は .gitignore 済み。SDK の DLL はリポジトリに入れない (再配布しない)。

	手元 (Windows + Visual Studio) でも CI でも同じように動く。

.PARAMETER OutDir
	出力先。既定は tools/build-insights/out。
#>
param(
	[string]$OutDir = (Join-Path $PSScriptRoot 'out')
)

$ErrorActionPreference = 'Stop'

# SDK の版を上げるときは両方を書き換える (SHA256 は nupkg そのもののハッシュ)
$SdkVersion = '1.5.3'
$SdkSha256 = '7eee14353b1445bd88c73436545b8e1e64222cabc760e96d730e12cca8c1c84f'

$sdkRoot = Join-Path $PSScriptRoot ".sdk\$SdkVersion"
$sdkNative = Join-Path $sdkRoot 'build\native'

if (-not (Test-Path (Join-Path $sdkNative 'inc\CppBuildInsights.hpp'))) {
	New-Item -ItemType Directory -Force -Path $sdkRoot | Out-Null
	$zip = Join-Path $sdkRoot 'sdk.nupkg'
	$url = "https://www.nuget.org/api/v2/package/Microsoft.Cpp.BuildInsights/$SdkVersion"
	Write-Host "SDK を取得: $url"
	Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing
	$hash = (Get-FileHash -Algorithm SHA256 $zip).Hash.ToLowerInvariant()
	if ($hash -ne $SdkSha256) {
		throw "SDK のハッシュが一致しない (期待 $SdkSha256, 実際 $hash)"
	}
	# Expand-Archive は Windows PowerShell 5.1 で nupkg の [Content_Types].xml に躓くので、
	# 要る build/native/ の下だけを自前で取り出す
	Add-Type -AssemblyName System.IO.Compression.FileSystem
	$archive = [IO.Compression.ZipFile]::OpenRead($zip)
	try {
		foreach ($entry in $archive.Entries) {
			if (-not $entry.FullName.StartsWith('build/native/') -or $entry.FullName.EndsWith('/')) { continue }
			$dest = Join-Path $sdkRoot ($entry.FullName -replace '/', '\')
			New-Item -ItemType Directory -Force -Path (Split-Path $dest) | Out-Null
			[IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $dest, $true)
		}
	} finally {
		$archive.Dispose()
	}
	Remove-Item $zip
}

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath |
	Select-Object -First 1
if (-not $vsPath) { throw 'C++ ツールセット入りの Visual Studio が見つからない' }
$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) { throw "vcvars64.bat が見つからない: $vcvars" }

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$objDir = Join-Path $OutDir 'obj'
New-Item -ItemType Directory -Force -Path $objDir | Out-Null

$src = Join-Path $PSScriptRoot 'nox_build_insights.cpp'
$exe = Join-Path $OutDir 'nox_build_insights.exe'
$inc = Join-Path $sdkNative 'inc'
$lib = Join-Path $sdkNative 'x64\lib\CppBuildInsights.lib'

# SDK のヘッダは C++17 で書かれている (クラス内の明示的特殊化など) ので C++17 に合わせる
$cl = "cl /nologo /std:c++17 /permissive- /EHsc /O2 /MT /W3 /utf-8 /DNOMINMAX /DWIN32_LEAN_AND_MEAN " +
	"/I`"$inc`" /Fo`"$objDir\\`" /Fe`"$exe`" `"$src`" /link `"$lib`" dbghelp.lib"
# 引用符の入れ子を cmd /c に渡すと崩れやすいので、一時的なバッチに書いて実行する。
# cmd はバッチを OEM コードページで読むので、パスに日本語があっても化けないよう oem で書く
$bat = Join-Path $objDir 'build.cmd'
Set-Content -Path $bat -Encoding oem -Value @('@echo off', "call `"$vcvars`" >nul || exit /b 1", $cl)
& cmd.exe /d /c $bat
if ($LASTEXITCODE -ne 0) { throw "ビルドに失敗 (exit $LASTEXITCODE)" }

Copy-Item (Join-Path $sdkNative 'x64\bin\*.dll') $OutDir -Force
Write-Host "ビルドした: $exe"
