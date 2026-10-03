//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	win64_sdk_end.h
///	@brief	windows.hのインクルードが必要なマクロをincludeした後にincludeする
///			必要なマクロをundefする。include前にwin64_sdk_begin.hをincludeすること。
#undef near
#undef far