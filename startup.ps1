# gitで管理していないファイル群を生成するスクリプト
# 1. "Nox\runtime\bin\source\CustomTask\CustomTask.sln" をビルドして、"Nox\runtime\bin" に "CustomTask.dll" を配置。
# 2. "Nox\runtime\bin\source\ReflectionGenerator\ReflectionGenerator.slnx" を発行する。"Nox\runtime\bin" に "ReflectionGenerator.exe" を配置。

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-SingleFilePath {
	param(
		[Parameter(Mandatory = $true)]
		[string]$SearchRoot,

		[Parameter(Mandatory = $true)]
		[string]$FileName,

		[Parameter(Mandatory = $true)]
		[string]$ExcludePattern
	)

	$files = @(Get-ChildItem -Path $SearchRoot -Filter $FileName -Recurse -File |
		Where-Object { $_.FullName -notmatch $ExcludePattern } |
		Sort-Object LastWriteTime -Descending)

	if ($files.Count -eq 0) {
		throw "${FileName} が見つかりません。検索ルート: $SearchRoot"
	}

	return $files[0].FullName
}

function Get-MSBuildPath {
	$vswherePath = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
	if (-not (Test-Path -Path $vswherePath -PathType Leaf)) {
		return $null
	}

	$msbuildPath = & $vswherePath -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
	if ([string]::IsNullOrWhiteSpace($msbuildPath)) {
		return $null
	}

	return $msbuildPath
}

$scriptRoot = Split-Path -Parent $PSCommandPath
Push-Location $scriptRoot

try {
	$runtimeBinDir = Join-Path $scriptRoot "runtime\bin"
	$customTaskSolution = Join-Path $runtimeBinDir "source\CustomTask\CustomTask.sln"
	$reflectionGeneratorSolution = Join-Path $runtimeBinDir "source\ReflectionGenerator\ReflectionGenerator.slnx"
	$customTaskProjectDir = Split-Path -Parent $customTaskSolution
	$reflectionGeneratorProjectDir = Split-Path -Parent $reflectionGeneratorSolution
	$reflectionGeneratorProject = Join-Path $reflectionGeneratorProjectDir "ReflectionGenerator.csproj"

	if (-not (Test-Path -Path $customTaskSolution -PathType Leaf)) {
		throw "CustomTask.sln が見つかりません: $customTaskSolution"
	}

	if (-not (Test-Path -Path $reflectionGeneratorSolution -PathType Leaf)) {
		throw "ReflectionGenerator.slnx が見つかりません: $reflectionGeneratorSolution"
	}

	Write-Host "[1/3] CustomTask をビルドします..."
	dotnet build $customTaskSolution -c Release
	if ($LASTEXITCODE -ne 0) {
		throw "CustomTask のビルドに失敗しました。"
	}

	$customTaskDll = Get-SingleFilePath -SearchRoot $customTaskProjectDir -FileName "CustomTask.dll" -ExcludePattern "[\\/]obj[\\/]"
	$customTaskTarget = Join-Path $runtimeBinDir "CustomTask.dll"
	Copy-Item -Path $customTaskDll -Destination $customTaskTarget -Force

	Write-Host "[2/3] ReflectionGenerator を発行します..."
	$msbuildPath = Get-MSBuildPath
	if ($null -ne $msbuildPath) {
		& $msbuildPath $reflectionGeneratorSolution /m /p:Configuration=Release /nologo /verbosity:minimal
		if ($LASTEXITCODE -ne 0) {
			throw "ReflectionGenerator.slnx のビルドに失敗しました。"
		}

		dotnet publish $reflectionGeneratorProject -c Release
		if ($LASTEXITCODE -ne 0) {
			throw "ReflectionGenerator.csproj の発行に失敗しました。"
		}
	}
	else {
		Write-Warning "MSBuild.exe が見つからないため、ReflectionGenerator.csproj を dotnet publish します。"
		dotnet publish $reflectionGeneratorProject -c Release
		if ($LASTEXITCODE -ne 0) {
			throw "ReflectionGenerator の発行に失敗しました。"
		}
	}

	$reflectionGeneratorExe = Get-SingleFilePath -SearchRoot $reflectionGeneratorProjectDir -FileName "ReflectionGenerator.exe" -ExcludePattern "[\\/]obj[\\/]"
	$reflectionGeneratorTarget = Join-Path $runtimeBinDir "ReflectionGenerator.exe"
	Copy-Item -Path $reflectionGeneratorExe -Destination $reflectionGeneratorTarget -Force

	Write-Host "[3/3] 完了"
	Write-Host "- CustomTask.dll: $customTaskTarget"
	Write-Host "- ReflectionGenerator.exe: $reflectionGeneratorTarget"
}
finally {
	Pop-Location
}