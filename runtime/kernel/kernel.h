// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

///	@file	kernel.h
///	@brief	別プロジェクトがincludeする用のヘッダ
///	@details	他プロジェクトは kernel と標準ライブラリを、各プロジェクトの pch.h からこのヘッダ経由でだけ使う。
///				他プロジェクトで要る標準ヘッダが足りなければ、下の一覧へ足す。
///				MSVC の標準ライブラリが連鎖して読み込むものに頼らず、使うものはここに明示する
///				(libstdc++ / libc++ では連鎖の範囲が違い、移植時にコンパイルエラーになる)。
#pragma once

//	標準ライブラリ (他プロジェクト向け)
#include	<algorithm>
#include	<array>
#include	<atomic>
#include	<bit>
#include	<chrono>
#include	<cmath>
#include	<concepts>
#include	<cstddef>
#include	<cstdint>
#include	<cstdio>
#include	<cstdlib>
#include	<cstring>
#include	<expected>
#include	<format>
#include	<functional>
#include	<initializer_list>
#include	<limits>
#include	<memory>
#include	<optional>
#include	<ranges>
#include	<semaphore>
#include	<source_location>
#include	<span>
#include	<string>
#include	<string_view>
#include	<thread>
#include	<tuple>
#include	<type_traits>
#include	<utility>
#include	<vector>
//	end 標準ライブラリ

#include	"basic_type.h"
#include	"basic_definition.h"
#include	"advanced_type.h"

#include	"vector.h"
#include	"unordered_map.h"
#include	"unordered_set.h"
#include	"stl_string.h"
#include	"queue.h"

#include	"algorithm.h"

#include	"type_traits/object_pointer_signature.h"
#include	"type_traits/function_signature.h"
#include	"type_traits/function_object_signature.h"
#include	"type_traits/type_name.h"

#include	"string_format.h"
#include	"unicode_converter.h"

#include	"assertion.h"
#include	"singleton.h"
#include	"memory/nox_memory.h"
#include	"memory/memory_util.h"
#include	"memory/pmr_buffer.h"

//	os
#include	"os.h"
#include	"os_utility.h"
#include	"atomic.h"
#include	"mutex.h"
#include	"scoped_lock.h"
#include	"read_write_lock.h"
#include	"thread.h"
#include	"clipboard.h"
#include	"window.h"
#include	"io/file.h"

#include	"platform_type.h"
//	end os

#include	"function.h"
#include	"log_trace.h"

#include	"advanced_definition.h"

#include	"math/math.h"

#include	"string.h"


#include	"intrusive_ptr.h"

#include	"preprocessor/repeat.h"
#include	"delegate.h"

#include	"file_system.h"

#include	"type_id.h"
#include	"debug_break.h"

#include	"preprocessor/util.h"

#include	"iterator.h"
#include	"reflection_type.h"
#include	"reflection_attribute.h"
#include	"stack.h"
#include	"memory/memory_profile.h"
#include	"stop_watch.h"
#include	"log_id.h"
#include	"guid.h"
#include	"dynamic_array.h"
#include	"utility.h"
#include	"interface_class.h"
#include	"scope_profile.h"
#include	"placement_object.h"
#include	"fixed_string.h"
#include	"path.h"
#include	"fixed_vector.h"
#include	"nameof.h"
#include	"crc32.h"

//	STL コンテナ (nox のアロケータを使う別名)
#include	"vector.h"
#include	"queue.h"
#include	"list.h"
#include	"unordered_map.h"
#include	"unordered_set.h"
#include	"stl_string.h"
//	end STL コンテナ

//	io
#include	"io/binary_reader.h"
#include	"io/stream_reader.h"
//	end io

//	job
#include	"job_system.h"
//	end job

//	diagnostics
#include	"parallel_execute_checker.h"
//	ReflectionGenerator の解析対象に入れないため、生成器の解析時は外す
#if !NOX_REFLECTION_GENERATOR
#include	"memory/allocation_counter.h"
#endif // !NOX_REFLECTION_GENERATOR
//	end diagnostics

#include	"utility.h"