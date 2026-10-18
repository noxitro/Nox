#	runtime/modules に module (静的ライブラリのプロジェクト) を足すスクリプト
#
#	名前から名前空間・クラス名・ファイル名を決め、tools/new-module-template の雛形を
#	runtime/modules/<名前>/ へ書き出して、runtime/runtime.slnx の /modules/ に登録する。
#
#	  名前                名前空間      クラス              ファイル
#	  render              nox::render   RenderModule        render_module.h
#	  render.mesh         nox::render   MeshModule          mesh_module.h
#	  render.debug_draw   nox::render   DebugDrawModule     debug_draw_module.h
#	  hid                 nox::hid      HidModule           hid_module.h
#
#	- 名前空間は名前の最初の区切りだけで決める。型の完全修飾名は保存データに載るので、
#	  プロジェクトの分け方 (render と render.mesh のどちらに置くか) から切り離しておく。
#	- クラス名は最後の区切りを PascalCase にして Module を付ける。1 つの名前空間に
#	  複数の module が入るので、Module だけの名前にはしない。
#	- 代表ヘッダは <名前>.h。ReflectionGenerator は vcxproj のパスの拡張子を .h に
#	  替えたものを生成コードから include するので、この名前は変えられない。
#	- ドットで区切った名前は、1 つ手前の名前の module (render.mesh なら render) に依存する。
#	  依存先とその依存先の代表ヘッダを pch.h に書き、slnx の BuildDependency に足す。
#
#	使い方:
#		pwsh tools/new-module.ps1 render.mesh
#		pwsh tools/new-module.ps1 render.mesh -DependsOn hid
#		pwsh tools/new-module.ps1 render.mesh -WhatIf      # 書き出さずに決まる内容だけ表示する
#	名前を省くと尋ねる (new-module.bat をダブルクリックしたとき)。
[CmdletBinding(SupportsShouldProcess)]
param(
	[Parameter(Position = 0)]
	[string]$Name,
	[string[]]$DependsOn = @()
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$runtimeDir = Join-Path $repoRoot 'runtime'
$modulesDir = Join-Path $runtimeDir 'modules'
$slnxPath = Join-Path $runtimeDir 'runtime.slnx'
$templateDir = Join-Path $PSScriptRoot 'new-module-template'
$utf8 = New-Object System.Text.UTF8Encoding($false)

#	どの module も読む、runtime の土台のプロジェクト (pch.h の雛形に書いてあるもの)
$baseDependencies = @('core/core.vcxproj', 'kernel/kernel.vcxproj', 'reflection/reflection.vcxproj')

function Get-ModuleProjectPath
{
	param([string]$ModuleName)
	return "modules/$ModuleName/$ModuleName.vcxproj"
}

function ConvertTo-PascalCase
{
	param([string]$Text)
	return ($Text.Split('_') | ForEach-Object { $_.Substring(0, 1).ToUpperInvariant() + $_.Substring(1) }) -join ''
}

#	------------------------------------------------------------
#	名前の検査と、名前から決まるもの
#	------------------------------------------------------------
if (-not $Name)
{
	$Name = (Read-Host '追加する module の名前 (例: render.mesh)').Trim()
}

$segmentPattern = '[a-z][a-z0-9]*(?:_[a-z0-9]+)*'
if ($Name -cnotmatch "^$segmentPattern(?:\.$segmentPattern)*$")
{
	throw "名前 '$Name' は使えない。小文字の snake_case をドットでつなぐ (例: render.mesh)。"
}
if ($Name.EndsWith('_test'))
{
	throw "名前 '$Name' は使えない。_test で終わるプロジェクトは ReflectionGenerator が module として扱わない。"
}

$segments = $Name.Split('.')
$namespaceName = 'nox::' + $segments[0]
$lastSegment = $segments[$segments.Length - 1]
$className = (ConvertTo-PascalCase $lastSegment) + 'Module'
$moduleFile = "${lastSegment}_module"
$projectPath = Get-ModuleProjectPath $Name
$targetDir = Join-Path $modulesDir $Name

#	------------------------------------------------------------
#	slnx を読み、既存のプロジェクトと依存を調べる
#	------------------------------------------------------------
$slnx = New-Object System.Xml.XmlDocument
$slnx.LoadXml([IO.File]::ReadAllText($slnxPath, $utf8))

$projects = @{}
foreach ($node in $slnx.SelectNodes('//Project'))
{
	$path = $node.GetAttribute('Path')
	$projects[$path] = @($node.SelectNodes('BuildDependency') | ForEach-Object { $_.GetAttribute('Project') })
}

#	ReflectionGenerator はプロジェクトをファイル名 (拡張子なし) で区別するので、置き場所が違っても同名は不可
foreach ($path in $projects.Keys)
{
	if ([IO.Path]::GetFileNameWithoutExtension($path) -eq $Name)
	{
		throw "同じ名前のプロジェクトが runtime.slnx にある: $path"
	}
}
if (Test-Path -LiteralPath $targetDir)
{
	throw "フォルダが既にある: $targetDir"
}

#	同じ名前空間に同じクラス名の module ができないか (render.mesh と render.foo.mesh など)
foreach ($path in $projects.Keys)
{
	if ($path -notmatch '^modules/([^/]+)/\1\.vcxproj$') { continue }
	$otherSegments = $Matches[1].Split('.')
	if ($otherSegments[0] -eq $segments[0] -and $otherSegments[$otherSegments.Length - 1] -eq $lastSegment)
	{
		throw "${namespaceName}::$className が $path と重なる。最後の区切りを別の名前にする。"
	}
}

#	直接の依存先: 1 つ手前の名前の module と -DependsOn (bat 経由だと "a,b" が 1 つの文字列で来る)
$directDependencies = New-Object System.Collections.Generic.List[string]
if ($segments.Length -gt 1)
{
	$directDependencies.Add(($segments[0..($segments.Length - 2)] -join '.'))
}
foreach ($item in $DependsOn)
{
	foreach ($dependency in $item.Split(','))
	{
		$dependency = $dependency.Trim()
		if ($dependency -and -not $directDependencies.Contains($dependency))
		{
			$directDependencies.Add($dependency)
		}
	}
}
foreach ($dependency in $directDependencies)
{
	if (-not $projects.ContainsKey((Get-ModuleProjectPath $dependency)))
	{
		throw "依存先の module '$dependency' が runtime.slnx に無い ($(Get-ModuleProjectPath $dependency))。先にそちらを作る。"
	}
}

#	依存先の依存先までたどり、依存される側が先に来る順に並べる (pch.h の include 順)
$orderedModules = New-Object System.Collections.Generic.List[string]
function Add-ModuleClosure
{
	param([string]$ModulePath)
	if ($orderedModules.Contains($ModulePath)) { return }
	foreach ($child in $projects[$ModulePath])
	{
		if ($child.StartsWith('modules/')) { Add-ModuleClosure $child }
	}
	$orderedModules.Add($ModulePath)
}
foreach ($dependency in $directDependencies)
{
	Add-ModuleClosure (Get-ModuleProjectPath $dependency)
}

$dependencyIncludes = ($orderedModules | ForEach-Object {
	$moduleName = [IO.Path]::GetFileNameWithoutExtension($_)
	"#include`t`"../$moduleName/$moduleName.h`"`r`n"
}) -join ''

#	------------------------------------------------------------
#	決まった内容を表示する
#	------------------------------------------------------------
Write-Host "module      : $Name"
Write-Host "フォルダ    : runtime/modules/$Name/"
Write-Host "名前空間    : $namespaceName"
Write-Host "クラス      : ${namespaceName}::$className ($moduleFile.h)"
Write-Host "代表ヘッダ  : $Name.h"
if ($orderedModules.Count -gt 0)
{
	Write-Host "依存先      : $(($orderedModules | ForEach-Object { [IO.Path]::GetFileNameWithoutExtension($_) }) -join ', ')"
}

if (-not $PSCmdlet.ShouldProcess("runtime/modules/$Name と runtime.slnx", 'module を追加'))
{
	return
}

#	------------------------------------------------------------
#	雛形を書き出す
#	------------------------------------------------------------
$projectGuid = [guid]::NewGuid().ToString()
$tokens = [ordered]@{
	'{{PROJECT}}' = $Name
	'{{NAMESPACE}}' = $namespaceName
	'{{CLASS}}' = $className
	'{{MODULE_FILE}}' = $moduleFile
	'{{YEAR}}' = (Get-Date).Year.ToString()
	'{{GUID}}' = $projectGuid
	'{{GUID_SOURCE_FILTER}}' = [guid]::NewGuid().ToString()
	'{{GUID_HEADER_FILTER}}' = [guid]::NewGuid().ToString()
	'{{DEPENDENCY_INCLUDES}}' = $dependencyIncludes
}

#	雛形 → 書き出すファイル名
$outputs = [ordered]@{
	'module.vcxproj' = "$Name.vcxproj"
	'module.vcxproj.filters' = "$Name.vcxproj.filters"
	'root.h' = "$Name.h"
	'module.h' = "$moduleFile.h"
	'module.cpp' = "$moduleFile.cpp"
	'pch.h' = 'pch.h'
	'pch.cpp' = 'pch.cpp'
}

$contents = [ordered]@{}
foreach ($template in $outputs.Keys)
{
	$text = [IO.File]::ReadAllText((Join-Path $templateDir $template), $utf8)
	foreach ($token in $tokens.Keys)
	{
		$text = $text.Replace($token, $tokens[$token])
	}
	if ($text -match '\{\{[A-Z_]+\}\}')
	{
		throw "雛形 $template に置き換えていない $($Matches[0]) が残っている。"
	}
	#	既存の module と同じく CRLF にそろえる
	$contents[$outputs[$template]] = ($text -replace "`r?`n", "`r`n")
}

New-Item -ItemType Directory -Path $targetDir | Out-Null
foreach ($file in $contents.Keys)
{
	[IO.File]::WriteAllText((Join-Path $targetDir $file), $contents[$file], $utf8)
}

#	------------------------------------------------------------
#	runtime.slnx の /modules/ に登録する (module 名の順に並べる。Path で比べると
#	'.' が '/' より前なので render.mesh が render より前に来てしまう)
#	------------------------------------------------------------
$folder = $slnx.SelectSingleNode("/Solution/Folder[@Name='/modules/']")
if (-not $folder)
{
	$folder = $slnx.CreateElement('Folder')
	$folder.SetAttribute('Name', '/modules/')
	$slnx.DocumentElement.AppendChild($folder) | Out-Null
}

$projectNode = $slnx.CreateElement('Project')
$projectNode.SetAttribute('Path', $projectPath)
$projectNode.SetAttribute('Id', $projectGuid)
$buildDependencies = New-Object System.Collections.Generic.List[string]
$buildDependencies.AddRange([string[]]$baseDependencies)
$buildDependencies.AddRange([string[]]$orderedModules)
$buildDependencies.Sort([StringComparer]::Ordinal)
foreach ($dependency in $buildDependencies)
{
	$dependencyNode = $slnx.CreateElement('BuildDependency')
	$dependencyNode.SetAttribute('Project', $dependency)
	$projectNode.AppendChild($dependencyNode) | Out-Null
}

$after = $null
foreach ($sibling in $folder.SelectNodes('Project'))
{
	$siblingName = [IO.Path]::GetFileNameWithoutExtension($sibling.GetAttribute('Path'))
	if ([string]::CompareOrdinal($siblingName, $Name) -lt 0) { $after = $sibling }
}
if ($after) { $folder.InsertAfter($projectNode, $after) | Out-Null }
else { $folder.PrependChild($projectNode) | Out-Null }

#	VS が書き出す形 (2 桁インデント・CRLF・XML 宣言なし・BOM なし) で保存する
$settings = New-Object System.Xml.XmlWriterSettings
$settings.Indent = $true
$settings.IndentChars = '  '
$settings.NewLineChars = "`r`n"
$settings.OmitXmlDeclaration = $true
$settings.Encoding = $utf8
$builder = New-Object System.Text.StringBuilder
$writer = [System.Xml.XmlWriter]::Create($builder, $settings)
$slnx.Save($writer)
$writer.Dispose()
[IO.File]::WriteAllText($slnxPath, $builder.ToString() + "`r`n", $utf8)

Write-Host ''
Write-Host "作成した: runtime/modules/$Name/ ($($contents.Keys -join ', '))"
Write-Host 'runtime.slnx に登録した。Visual Studio で開いている場合は、再読み込みを求められたら応じる。'
