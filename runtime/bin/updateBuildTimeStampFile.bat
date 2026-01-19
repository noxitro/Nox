rem chcp 65001

echo run %0
rem echo off
setlocal

rem ���̃t�@�C���́A�w��v���W�F�N�g�̃r���h�X�^���v�t�@�C�����쐬���܂�
echo �r���h�^�C���X�^���v���쐬���܂�

rem �����̉��

rem ���W���[��(�v���W�F�N�g��)
set ModuleName=%1
echo ProjectName=%ModuleName%

rem ���ԏo�̓f�B���N�g��
set IntermediateDir=%2

rem �����̉��end

rem �r���h�^�C���X�^���v���p�̃t�H���_���쐬����
set TempBuildTimeStampDir=%IntermediateDir%\nox_build_time_stamp
echo TempBuildTimeStampDir=%TempBuildTimeStampDir%
mkdir %TempBuildTimeStampDir%

rem ��������������
set BuildTimeStampFilePath=%TempBuildTimeStampDir%\%ModuleName%.tmp
echo BuildTimeStampFilePath=%BuildTimeStampFilePath%
echo %DATE% %TIME%>%BuildTimeStampFilePath%

echo �r���h�^�C���X�^���v�쐬����