//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	win64_api.h
///	@brief	win64_api
#pragma once
#include	"basic_definition.h"

#if NOX_WIN64

//	型チェックを厳密に行う
#define STRICT

//	min maxマクロを定義させない
#define NOMINMAX

#define NODRAWTEXT
//#define NOGDI
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP

// ヘッダーからあまり使われない関数を省く
#define WIN32_LEAN_AND_MEAN
#pragma warning(push, 0)
#include	<Windows.h>
#pragma	warning(pop)

#undef	near
#undef	far

//#undef STRICT
//#undef NOMINMAX
#undef NODRAWTEXT
//#undef NOGDI
#undef NOBITMAP
#undef NOMCX
#undef NOSERVICE
#undef NOHELP
//#undef WIN32_LEAN_AND_MEAN

// W付きの関数は、A/Wの別名を潰すために、ここで#undefする
#undef	CreateFile
#undef	DeleteFile
#undef	CopyFile
#undef	MoveFile
#undef	CreateDirectory
#undef	RemoveDirectory
#undef	GetCurrentDirectory
#undef	SetCurrentDirectory
#undef	GetFileAttributes
#undef	GetTempPath
#undef	GetModuleFileName
#undef	LoadLibrary
#undef	GetCommandLine
#undef	GetEnvironmentVariable
#undef	CreateProcess
#undef	CreateWindow
#undef	CreateWindowEx
#undef	RegisterClass
#undef	GetClassName
#undef	FindWindow
#undef	SetWindowText
#undef	GetWindowText
#undef	SendMessage
#undef	PostMessage
#undef	GetMessage
#undef	PeekMessage
#undef	DispatchMessage
#undef	MessageBox
#undef	CreateEvent
#undef	CreateMutex
#undef	CreateSemaphore
#undef	GetObject
#undef	LoadImage
#undef	LoadString
#undef	FindResource
#undef	CreateFont
#undef	OutputDebugString
#undef	FormatMessage
#undef	GetUserName
#undef	GetProp
#undef	SetProp
#undef	RemoveProp
//	A/W の別名ではないが、汎用的な名前を潰すもの
#undef	Yield
#undef	GetCurrentTime

#endif // NOX_WIN64