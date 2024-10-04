chcp 65001

echo run %0
echo off
setlocal

rem ReflectionGeneraterで使用する情報を出力するためのbatファイル


rem 引数の解析
set SolutionDir=%1
set SolutionPath=%2
set ProjectPath=%3
set SourceFilePath=%4
set Configuration=%5
set Platform=%6
set ConfigurationDefine=%7
set PlatformDefine=%8
set MSBuildBinPath=%9
shift
set EnableNamespaceList=%9
shift
set OutputDir=%9
shift
set IntermediateDir=%9

rem 引数のログ出力
echo SolutionDir:%SolutionDir%
echo SolutionPath:%SolutionPath%
echo ProjectPath:%ProjectPath%
echo SourceFilePath:%SourceFilePath%
echo Configuration:%Configuration%
echo Platform:%Platform%
echo ConfigurationDefine:%ConfigurationDefine%
echo PlatformDefine:%PlatformDefine%
echo MSBuildBinPath:%MSBuildBinPath%
echo EnableNamespaceList:%EnableNamespaceList%
echo OutputDir:%OutputDir%
echo IntermediateDir:%IntermediateDir%

rem 出力先決定
set textFilePath=%SolutionDir:"=%build\reflectionGenerateArgs.txt"
echo %textFilePath%

rem 出力処理

echo -SolutionPath>%textFilePath%
echo %SolutionPath%>>%textFilePath%

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

echo -EnableNamespaceList>>%textFilePath%
echo %EnableNamespaceList%>>%textFilePath%

echo -OutputDir>>%textFilePath%
echo %OutputDir%>>%textFilePath%

echo -IntermediateDir>>%textFilePath%
echo %IntermediateDir%>>%textFilePath%

echo run end
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

echo run end %0

exit 0