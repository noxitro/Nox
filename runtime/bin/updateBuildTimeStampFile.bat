chcp 65001

echo run %0
echo off
setlocal

rem このファイルは、指定プロジェクトのビルドスタンプファイルを作成します
rem 

rem 引数の解析

rem モジュール(プロジェクト名)
set ModuleName=%1

rem 中間出力ディレクトリ
set IntermediateDir=%2

rem 引数の解析end

rem 引数のログ出力
echo ModuleName=%ModuleName%

rem ビルドタイムスタンプ情報用のフォルダを作成する
set TempBuildTimeStampDir=%IntermediateDir%\nox_build_time_stamp
echo TempBuildTimeStampDir=%TempBuildTimeStampDir%
mkdir %TempBuildTimeStampDir%

rem 日時を書き込む
set BuildTimeStampFilePath=%TempBuildTimeStampDir%\%ModuleName%.tmp
echo BuildTimeStampFilePath=%BuildTimeStampFilePath%
echo %DATE% %TIME%>%BuildTimeStampFilePath%
