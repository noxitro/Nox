chcp 65001

echo run %0
echo off
setlocal

rem 引数の解析
set ModuleName=%1

rem 引数のログ出力
echo ModuleName=%ModuleName%

rem ビルドタイムスタンプ情報用のフォルダを作成する
set TempBuildTimeStampDir=%TEMP%\nox_build_time_stamp
echo TempBuildTimeStampDir=%TempBuildTimeStampDir%
mkdir %TempBuildTimeStampDir%

rem 日時を書き込む
set BuildTimeStampFilePath=%TempBuildTimeStampDir%\%ModuleName%.tmp
echo BuildTimeStampFilePath=%BuildTimeStampFilePath%
echo %DATE% %TIME%>%BuildTimeStampFilePath%
