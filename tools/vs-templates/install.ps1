#	Visual Studio のユーザー項目テンプレート (新しい項目の追加 > nox_header など) をインストールする。
#	このディレクトリ直下の各フォルダを zip にして、VS の ItemTemplates へコピーする。
#	テンプレートを直したらこれを再実行すること。VS を開いたままなら再起動で反映される。
#
#	使い方:
#		pwsh tools/vs-templates/install.ps1                  # 見つかった VS 18 以降の ItemTemplates へ
#		pwsh tools/vs-templates/install.ps1 -Destination <dir>
#
#	VS のテンプレート置き場は「ドキュメント」配下だが、OneDrive のバックアップが有効だと
#	シェルの MyDocuments (C:\Users\<user>\Documents) ではなく %OneDrive%\Documents 側になる。
#	実際の場所は VS の [ツール] > [オプション] > [プロジェクトおよびソリューション] > [場所] で確認できる。
[CmdletBinding()]
param(
	[string[]]$Destination
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem

if (-not $Destination) {
	$docs = @([Environment]::GetFolderPath('MyDocuments'))
	if ($env:OneDrive) { $docs += Join-Path $env:OneDrive 'Documents' }
	$Destination = $docs | Select-Object -Unique | Where-Object { Test-Path $_ } | ForEach-Object {
		#	v145 ツールセットが要るので VS 18 以降だけ。"Visual Studio 2019" 等の旧版は対象外
		Get-ChildItem $_ -Directory -Filter 'Visual Studio *' |
			Where-Object { $_.Name -match '^Visual Studio (\d+)$' -and [int]$Matches[1] -ge 18 -and [int]$Matches[1] -lt 2000 } |
			ForEach-Object { Join-Path $_.FullName 'Templates\ItemTemplates' } |
			Where-Object { Test-Path $_ }
	}
	if (-not $Destination) {
		throw 'Visual Studio の ItemTemplates が見つからない。-Destination で指定すること。'
	}
}

$templates = Get-ChildItem $PSScriptRoot -Directory | Where-Object { Get-ChildItem $_.FullName -Filter '*.vstemplate' }
foreach ($dst in $Destination) {
	New-Item -ItemType Directory -Force $dst | Out-Null
	foreach ($t in $templates) {
		$zip = Join-Path $dst "$($t.Name).zip"
		if (Test-Path $zip) { Remove-Item $zip -Force }
		#	zip のルートにファイルを直接置く (フォルダを挟むと VS が .vstemplate を見つけられない)
		[IO.Compression.ZipFile]::CreateFromDirectory($t.FullName, $zip, [IO.Compression.CompressionLevel]::Optimal, $false)
		Write-Host "installed: $zip"
	}
}
