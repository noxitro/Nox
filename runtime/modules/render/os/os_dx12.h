//	Copyright (C) 2026 NOX ENGINE All rights reserved.

///	@file	os_dx12.h
///	@brief	os_dx12
#pragma once
#include	"../definition.h"

#if NOX_RENDER_DX12
#pragma warning(push)
#pragma warning(disable:26812) // scoped enumを使ってないエラーを無効

#define	near
#define	far

#undef	NOMINMAX
#undef	STRICT
#define NOMINMAX
#define STRICT

#include	<d3d12.h>
#include	<dxgi1_6.h>
#undef NOMINMAX
#undef STRICT

#undef	near
#undef	far

#pragma warning(pop)

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
// #pragma comment(lib, "d3dcompiler.lib")
#endif // NOX_RENDER_DX12