#	モジュールの GoogleTest プロジェクト (<モジュール名>_test) を生成し、runtime.slnx に登録する。
#
#	使い方:
#		pwsh tools/module-test/new-module-test.ps1 <モジュール名>
#		例) pwsh tools/module-test/new-module-test.ps1 hid
#		    -> runtime/modules/hid/test/hid_test.vcxproj
#
#	前提: モジュールは nox_module テンプレート (tools/vs-templates/nox_module) で作られ、
#	      runtime.slnx に登録済みであること。<モジュール名>.h と <モジュール名>_module.h を使う。
#
#	VS のプロジェクトテンプレートにしていないのは次の理由による。
#	  - テストは「対象の配下の test/」に置く規約だが、VS は必ず「プロジェクト名のフォルダ」を作る
#	    (<モジュール名>_test/ になってしまう)
#	  - VS のテンプレート引数では "<モジュール名>_test" からモジュール名を取り出せず、
#	    参照設定や名前空間を埋められない
#	  - slnx の <Build Project="false" /> (下記) を毎回手で付ける必要があり、忘れやすい
#
#	生成されるもの:
#	  - <モジュール名>_test.vcxproj / .filters / pch.h / pch.cpp / main.cpp / <モジュール名>_module_test.cpp
#	    元は tools/module-test/template/。BOM と改行コードはテンプレートのまま保つ。
#	  - 参照: kernel / reflection / core / reflection_generated と、テスト対象のモジュール。
#	    モジュールが他のモジュールに依存している場合 (runtime.slnx の BuildDependency か
#	    vcxproj の ProjectReference で表現されているもの) は、それも推移的に辿って参照に加える。
#	    static library はリンク時に依存先を連れてこないので、実行ファイル側で全部並べる必要がある。
#	  - runtime.slnx の /tests/ への登録。<Build Project="false" /> を付ける。
#	    Release の /GL で reflection_generated.lib を共有する exe を同時にリンクすると
#	    中間ファイルを取り合って落ちるため、テストはソリューションビルドから外してある
#	    (.github/workflows/ci.yml の "Build test projects" を参照)。
#	    CI は runtime.slnx の *_test.vcxproj を自動で拾うので、ci.yml への追記は要らない。
#Requires -Version 7.0
[CmdletBinding()]
param(
	[Parameter(Mandatory, Position = 0)]
	[string]$Module
)

$ErrorActionPreference = 'Stop'

$runtimeDir = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..' '..' 'runtime'))
$slnxPath = Join-Path $runtimeDir 'runtime.slnx'
$templateDir = Join-Path $PSScriptRoot 'template'

#	どのモジュールのテストも必ずリンクするエンジン本体 (core_test と同じ組)。
#	reflection_generated は nox::reflection::Initialize() が生成コードの登録関数を呼ぶので要る
$engineProjects = @(
	'core/core.vcxproj',
	'kernel/kernel.vcxproj',
	'reflection/reflection.vcxproj',
	'reflection_generated/reflection_generated.vcxproj'
)

#	モジュール名は C++ の名前空間 (nox::<モジュール名>) にもなるので snake_case に限る。
#	"_test" 終わりは ReflectionGenerator がテスト実行ファイルとして扱う名前なので使えない
if ($Module -cnotmatch '^[a-z][a-z0-9_]*$') {
	throw "モジュール名は snake_case で指定すること: '$Module'"
}
if ($Module.EndsWith('_test')) {
	throw "モジュール名を指定すること (テストプロジェクト名ではない): '$Module'"
}

#	ファイルの BOM / 改行コードを変えずに読み書きする。
#	UTF8Encoding.GetString は BOM を U+FEFF のまま残し、書き戻すと同じバイト列に戻る
$utf8 = [Text.UTF8Encoding]::new($false)
function Read-Text([string]$path) { $utf8.GetString([IO.File]::ReadAllBytes($path)) }
function Write-Text([string]$path, [string]$text) { [IO.File]::WriteAllBytes($path, $utf8.GetBytes($text)) }
#	[xml] へのキャストは先頭の U+FEFF を受け付けないので落としてから読む
function Read-Xml([string]$path) { [xml](Read-Text $path).TrimStart([char]0xFEFF) }

#	runtime.slnx 基準の相対パス (区切りは /)
function Get-RuntimeRelativePath([string]$fullPath) {
	[IO.Path]::GetRelativePath($runtimeDir, $fullPath).Replace('\', '/')
}

function Get-ProjectGuid([string]$runtimeRelativePath) {
	$proj = Read-Xml (Join-Path $runtimeDir $runtimeRelativePath)
	$node = $proj.SelectSingleNode("//*[local-name()='ProjectGuid']")
	if (-not $node) { throw "ProjectGuid が無い: $runtimeRelativePath" }
	$node.InnerText.Trim('{', '}').ToLowerInvariant()
}

$slnxText = Read-Text $slnxPath
[xml]$slnx = $slnxText.TrimStart([char]0xFEFF)
$slnxProjects = @{}
foreach ($p in $slnx.SelectNodes('//Project')) { $slnxProjects[$p.Path] = $p }

#	テスト対象のモジュール
$modulePath = @($slnxProjects.Keys | Where-Object { [IO.Path]::GetFileName($_) -eq "$Module.vcxproj" })
if ($modulePath.Count -eq 0) {
	throw "runtime.slnx に $Module.vcxproj が無い。先に nox_module テンプレートでモジュールを作り、ソリューションへ追加すること。"
}
if ($modulePath.Count -gt 1) { throw "runtime.slnx に $Module.vcxproj が複数ある: $($modulePath -join ', ')" }
$modulePath = $modulePath[0]
$moduleDir = Split-Path -Parent (Join-Path $runtimeDir $modulePath)

foreach ($header in "$Module.h", "${Module}_module.h") {
	if (-not (Test-Path -LiteralPath (Join-Path $moduleDir $header))) {
		throw "$header が無い: $moduleDir (nox_module テンプレートで作ったモジュールを前提にしている)"
	}
}

$testName = "${Module}_test"
$testDir = Join-Path $moduleDir 'test'
$testProjectPath = Get-RuntimeRelativePath (Join-Path $testDir "$testName.vcxproj")
if (Test-Path -LiteralPath $testDir) { throw "既にある: $testDir" }
if ($slnxProjects.ContainsKey($testProjectPath)) { throw "runtime.slnx に既に登録されている: $testProjectPath" }

#	モジュールが依存しているプロジェクトを推移的に集める。
#	依存は slnx の BuildDependency (VS の「プロジェクトの依存関係」) と
#	vcxproj の ProjectReference のどちらで書かれていても拾う
$dependencies = [Collections.Generic.List[string]]::new()
$visited = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$queue = [Collections.Generic.Queue[string]]::new()
$queue.Enqueue($modulePath)
[void]$visited.Add($modulePath)
while ($queue.Count -gt 0) {
	$current = $queue.Dequeue()
	$found = @()
	if ($slnxProjects.ContainsKey($current)) {
		$found += @($slnxProjects[$current].SelectNodes('BuildDependency') | ForEach-Object { $_.Project })
	}
	$currentFull = Join-Path $runtimeDir $current
	$proj = Read-Xml $currentFull
	foreach ($ref in $proj.SelectNodes("//*[local-name()='ProjectReference']")) {
		$found += Get-RuntimeRelativePath ([IO.Path]::GetFullPath((Join-Path (Split-Path -Parent $currentFull) $ref.Include)))
	}
	foreach ($dep in $found) {
		if (-not $visited.Add($dep)) { continue }
		if ($engineProjects -contains $dep) { continue }
		if ([IO.Path]::GetFileNameWithoutExtension($dep).EndsWith('_test')) { continue }
		$dependencies.Add($dep)
		$queue.Enqueue($dep)
	}
}

#	テンプレートの穴埋め
$runtimeRelativeFromTest = [IO.Path]::GetRelativePath($testDir, $runtimeDir).Replace('/', '\') + '\'
$newline = "`r`n"
$references = [Text.StringBuilder]::new()
foreach ($ref in @($modulePath) + $dependencies + $engineProjects) {
	$include = [IO.Path]::GetRelativePath($testDir, (Join-Path $runtimeDir $ref)).Replace('/', '\')
	[void]$references.Append("    <ProjectReference Include=`"$include`">$newline")
	[void]$references.Append("      <Project>{$(Get-ProjectGuid $ref)}</Project>$newline")
	[void]$references.Append("    </ProjectReference>$newline")
}

$projectGuid = [guid]::NewGuid().ToString()
$replacements = [ordered]@{
	'$module$'         = $Module
	'$modulepascal$'   = -join ($Module.Split('_') | Where-Object { $_ } | ForEach-Object { $_.Substring(0, 1).ToUpperInvariant() + $_.Substring(1) })
	'$year$'           = (Get-Date).Year.ToString()
	'$guid1$'          = $projectGuid
	'$guid2$'          = [guid]::NewGuid().ToString()
	'$guid3$'          = [guid]::NewGuid().ToString()
	'$runtimedir$'     = $runtimeRelativeFromTest
	'$runtimeinclude$' = $runtimeRelativeFromTest.Replace('\', '/')
	'$references$'     = $references.ToString()
}

$files = [ordered]@{
	'module_test.vcxproj'         = "$testName.vcxproj"
	'module_test.vcxproj.filters' = "$testName.vcxproj.filters"
	'pch.h'                       = 'pch.h'
	'pch.cpp'                     = 'pch.cpp'
	'main.cpp'                    = 'main.cpp'
	'module_test.cpp'             = "${Module}_module_test.cpp"
}

#	先に全部作ってから書く (途中で失敗して半端なディレクトリが残らないように)
$outputs = [ordered]@{}
foreach ($entry in $files.GetEnumerator()) {
	$text = Read-Text (Join-Path $templateDir $entry.Key)
	foreach ($r in $replacements.GetEnumerator()) { $text = $text.Replace($r.Key, $r.Value) }
	$left = [regex]::Match($text, '\$[a-z0-9]+\$')
	if ($left.Success) { throw "テンプレートに未定義の置換子が残っている: $($left.Value) ($($entry.Key))" }
	$outputs[(Join-Path $testDir $entry.Value)] = $text
}

#	runtime.slnx の /tests/ へ、パス順を保って差し込む (VS が保存し直しても差分が出ないように)
$folderMatch = [regex]::Match($slnxText, '(?s)(?<open>[ \t]*<Folder Name="/tests/">\r?\n)(?<body>.*?)(?<close>[ \t]*</Folder>)')
if (-not $folderMatch.Success) { throw 'runtime.slnx に /tests/ フォルダが無い' }
$slnxNewline = if ($slnxText.Contains("`r`n")) { "`r`n" } else { "`n" }
$entryText = "    <Project Path=`"$testProjectPath`" Id=`"$projectGuid`">$slnxNewline" +
	"      <Build Project=`"false`" />$slnxNewline" +
	"    </Project>$slnxNewline"
$body = $folderMatch.Groups['body']
$insertAt = $body.Index + $body.Length
foreach ($m in [regex]::Matches($body.Value, '(?m)^    <Project Path="(?<path>[^"]+)"')) {
	if ([string]::Compare($m.Groups['path'].Value, $testProjectPath, [StringComparison]::OrdinalIgnoreCase) -gt 0) {
		$insertAt = $body.Index + $m.Index
		break
	}
}
$slnxText = $slnxText.Insert($insertAt, $entryText)
[void][xml]$slnxText.TrimStart([char]0xFEFF)	# 壊していないことを確かめる

New-Item -ItemType Directory -Path $testDir | Out-Null
foreach ($o in $outputs.GetEnumerator()) {
	Write-Text $o.Key $o.Value
	Write-Host "created: $(Get-RuntimeRelativePath $o.Key)"
}
Write-Text $slnxPath $slnxText
Write-Host "registered: $testProjectPath -> runtime.slnx (/tests/, Build=false)"
if ($dependencies.Count -gt 0) {
	Write-Host "module dependencies: $($dependencies -join ', ')"
}
