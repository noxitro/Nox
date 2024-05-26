chcp 65001
echo off
setlocal

rem 引数の解析
set SolutionDir=%1
set ProjectPath=%2
set SourceFilePath=%3
set Configuration=%4
set Platform=%5
set ConfigurationDefine=%6
set PlatformDefine=%7
set MSBuildBinPath=%8

rem 引数のログ出力
echo SolutionDir:%SolutionDir%
echo ProjectPath:%ProjectPath%
echo SourceFilePath:%SourceFilePath%
echo Configuration:%Configuration%
echo Platform:%Platform%
echo ConfigurationDefine:%ConfigurationDefine%
echo PlatformDefine:%PlatformDefine%
echo MSBuildBinPath:%MSBuildBinPath%

rem 出力先決定
set textFilePath=%SolutionDir:~0,-1%reflectionGenerateArgs.txt"
echo %textFilePath%

rem 出力処理

echo -SolutionDir>%textFilePath%
echo %SolutionDir%>>%textFilePath%

echo -ProjectPath>>%textFilePath%
echo %ProjectPath%>>%textFilePath%

echo -SourceFilePath>>%textFilePath%
echo %SourceFilePath%>>%textFilePath%

echo -Configuration>>%textFilePath%
echo %Configuration%>>%textFilePath%

echo -Platform>>%textFilePath%
echo %Platform%>>%textFilePath%

echo -ConfigurationDefine>>%textFilePath%
echo %ConfigurationDefine%>>%textFilePath%

echo -PlatformDefine>>%textFilePath%
echo %PlatformDefine%>>%textFilePath%

echo -MSBuildBinPath>>%textFilePath%
echo %MSBuildBinPath%>>%textFilePath%

exit 0

echo -SolutionDir %SolutionDir%>%textFilePath%
echo -ProjectFilePath %ProjectFilePath%>>%textFilePath%
echo -SourceFilePath %SourceFilePath%>>%textFilePath%
echo -Configuration %Configuration%>>%textFilePath%
echo -Platform %Platform%>>%textFilePath%
echo -ConfigurationDefine %ConfigurationDefine%>>%textFilePath%
echo -PlatformDefine %PlatformDefine%>>%textFilePath%
echo -MSBuildBinPath %MSBuildBinPath%>>%textFilePath%

exit 0

@Set /P args=-SolutionDir %SolutionDir% < NUL
@Set /P args=-ProjectFilePath %ProjectFilePath% < NUL
@Set /P args=-SourceFilePath %SourceFilePath% < NUL
@Set /P args=-Configuration %Configuration% < NUL
@Set /P args=-Platform %Platform% < NUL
@Set /P args=-ConfigurationDefine %ConfigurationDefine% < NUL
@Set /P args=-PlatformDefine %PlatformDefine% < NUL
@Set /P args=-MSBuildBinPath %MSBuildBinPath% < NUL

set text=%args%
echo %text% >>%textFilePath%

exit 0