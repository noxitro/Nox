#   NOX ENGINE BUILD SCRIPTS

param(
	[ValidateSet('Debug', 'Release', 'Master')]
	[string]$Configuration,

	[string]$ReflectionGeneratorPublishProfile = 'FolderProfile',

	[string]$ReflectionGeneratorDeployDir = $(Join-Path $PSScriptRoot 'runtime\bin')
)

$ErrorActionPreference = 'Stop'

function Publish-ReflectionGenerator {
	param(
		[string]$BuildConfiguration,
		[string]$PublishProfileName,
		[string]$DeployDir
	)

	$reflectionGeneratorDir = Join-Path $PSScriptRoot 'runtime\bin\source\ReflectionGenerator'
	$reflectionGeneratorSolutionPath = Join-Path $reflectionGeneratorDir 'ReflectionGenerator.slnx'
	$reflectionGeneratorProjectPath = Join-Path $reflectionGeneratorDir 'ReflectionGenerator.csproj'
	$reflectionGeneratorPublishProfilePath = Join-Path $reflectionGeneratorDir "Properties\PublishProfiles\$PublishProfileName.pubxml"
	$deployedExePath = Join-Path $DeployDir 'ReflectionGenerator.exe'

	if (-not (Test-Path $reflectionGeneratorSolutionPath)) {
		throw "ReflectionGenerator.slnx が見つかりません: $reflectionGeneratorSolutionPath"
	}

	if (-not (Test-Path $reflectionGeneratorProjectPath)) {
		throw "ReflectionGenerator.csproj が見つかりません: $reflectionGeneratorProjectPath"
	}

	if (-not (Test-Path $reflectionGeneratorPublishProfilePath)) {
		throw "PublishProfile が見つかりません: $reflectionGeneratorPublishProfilePath"
	}

	[xml]$reflectionGeneratorPublishProfileXml = Get-Content -Path $reflectionGeneratorPublishProfilePath
	$publishProfilePropertyGroup = $reflectionGeneratorPublishProfileXml.Project.PropertyGroup

	if ([string]::IsNullOrWhiteSpace($BuildConfiguration)) {
		$BuildConfiguration = $publishProfilePropertyGroup.Configuration
	}

	if ([string]::IsNullOrWhiteSpace($BuildConfiguration)) {
		$BuildConfiguration = 'Release'
	}

	$publishDirSetting = $publishProfilePropertyGroup.PublishDir
	if ([string]::IsNullOrWhiteSpace($publishDirSetting)) {
		throw "PublishDir が PublishProfile に設定されていません: $reflectionGeneratorPublishProfilePath"
	}

	$publishDir = if ([System.IO.Path]::IsPathRooted($publishDirSetting)) {
		$publishDirSetting
	}
	else {
		Join-Path $reflectionGeneratorDir $publishDirSetting
	}

	$publishedExePath = Join-Path $publishDir 'ReflectionGenerator.exe'

	if (-not (Test-Path $DeployDir)) {
		New-Item -ItemType Directory -Path $DeployDir -Force | Out-Null
	}

	# ReflectionGenerator.slnx をビルドする
	Write-Host "Build: $reflectionGeneratorSolutionPath"
	& dotnet build $reflectionGeneratorSolutionPath -c $BuildConfiguration
	if ($LASTEXITCODE -ne 0) {
		throw "ReflectionGenerator.slnx のビルドに失敗しました。"
	}

	# Visual Studio の発行プロファイル設定を流用して exe を発行する
	Write-Host "Publish: $reflectionGeneratorProjectPath ($PublishProfileName)"
	& dotnet publish $reflectionGeneratorProjectPath -c $BuildConfiguration -p:PublishProfile="$reflectionGeneratorPublishProfilePath"
	if ($LASTEXITCODE -ne 0) {
		throw "ReflectionGenerator の発行に失敗しました。"
	}

	if (-not (Test-Path $publishedExePath)) {
		throw "発行済み exe が見つかりません: $publishedExePath"
	}

	# 既存 C++ ビルドが参照する runtime/bin に配置する
	Copy-Item -Path $publishedExePath -Destination $deployedExePath -Force

	Write-Host "Deployed: $deployedExePath"
}

Publish-ReflectionGenerator -BuildConfiguration $Configuration -PublishProfileName $ReflectionGeneratorPublishProfile -DeployDir $ReflectionGeneratorDeployDir
