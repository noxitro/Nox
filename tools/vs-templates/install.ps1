#	Visual Studio のユーザー項目 / プロジェクトテンプレートをインストールする。
#	このディレクトリ直下の各フォルダを zip にし、.vstemplate の Type に対応する場所へコピーする。
#	テンプレートを直したらこれを再実行すること。VS を開いたままなら再起動で反映される。
#
#	使い方:
#		pwsh tools/vs-templates/install.ps1                  # 見つかった VS 18 以降のテンプレート置き場へ
#		pwsh tools/vs-templates/install.ps1 -Destination <ItemTemplates dir> -ProjectDestination <ProjectTemplates dir>
#		pwsh tools/vs-templates/install.ps1 -ProjectDestination <ProjectTemplates dir>
#
#	VS のテンプレート置き場は「ドキュメント」配下だが、OneDrive のバックアップが有効だと
#	シェルの MyDocuments (C:\Users\<user>\Documents) ではなく %OneDrive%\Documents 側になる。
#	実際の場所は VS の [ツール] > [オプション] > [プロジェクトおよびソリューション] > [場所] で確認できる。
[CmdletBinding()]
param(
	[string[]]$Destination,
	[string[]]$ProjectDestination
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem

if (-not $Destination -and -not $ProjectDestination) {
	$docs = @([Environment]::GetFolderPath('MyDocuments'))
	if ($env:OneDrive) { $docs += Join-Path $env:OneDrive 'Documents' }
	$visualStudioRoots = $docs | Select-Object -Unique | Where-Object { Test-Path $_ } | ForEach-Object {
		#	v145 ツールセットが要るので VS 18 以降だけ。"Visual Studio 2019" 等の旧版は対象外
		Get-ChildItem $_ -Directory -Filter 'Visual Studio *' |
			Where-Object { $_.Name -match '^Visual Studio (\d+)$' -and [int]$Matches[1] -ge 18 -and [int]$Matches[1] -lt 2000 } |
			ForEach-Object { $_.FullName }
	}
	$Destination = $visualStudioRoots | ForEach-Object { Join-Path $_ 'Templates\ItemTemplates' } | Where-Object { Test-Path $_ }
	$ProjectDestination = $visualStudioRoots | ForEach-Object { Join-Path $_ 'Templates\ProjectTemplates' }
	if (-not $Destination -and -not $ProjectDestination) {
		throw 'Visual Studio のテンプレート置き場が見つからない。-Destination または -ProjectDestination で指定すること。'
	}
}

$templates = Get-ChildItem $PSScriptRoot -Directory | Where-Object { Get-ChildItem $_.FullName -Filter '*.vstemplate' }
foreach ($t in $templates) {
	$manifestPath = Get-ChildItem $t.FullName -Filter '*.vstemplate' | Select-Object -First 1 -ExpandProperty FullName
	[xml]$manifest = Get-Content -Raw -LiteralPath $manifestPath
	$templateType = $manifest.DocumentElement.GetAttribute('Type')
	$destinations = switch ($templateType) {
		'Item' { $Destination }
		'Project' { $ProjectDestination }
		default { throw "Unsupported Visual Studio template type '$templateType' in '$manifestPath'." }
	}
	if (-not $destinations) { continue }

	foreach ($dst in $destinations) {
		New-Item -ItemType Directory -Force $dst | Out-Null
		$zip = Join-Path $dst "$($t.Name).zip"
		if (Test-Path $zip) { Remove-Item $zip -Force }
		#	zip のルートにファイルを直接置く (フォルダを挟むと VS が .vstemplate を見つけられない)
		[IO.Compression.ZipFile]::CreateFromDirectory($t.FullName, $zip, [IO.Compression.CompressionLevel]::Optimal, $false)
		Write-Host "installed: $zip"
	}
}
