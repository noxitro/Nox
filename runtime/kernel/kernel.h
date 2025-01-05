///	@file	kernel.h
///	@brief	別プロジェクトがincludeする用のヘッダ
#pragma once

#include	"algorithm.h"

#include	"type_traits/object_pointer_signature.h"
#include	"type_traits/function_signature.h"
#include	"type_traits/function_object_signature.h"
#include	"type_traits/type_name.h"

#include	"string_format.h"

#include	"assertion.h"
#include	"singleton.h"
#include	"memory/memory.h"
#include	"memory/memory_util.h"

//	os
#include	"os/os.h"
#include	"os/atomic.h"
#include	"os/mutex.h"
#include	"os/thread.h"
#include	"os/clipboard.h"

#include	"function.h"
#include	"log_trace.h"

#include	"advanced_definition.h"

#include	"math/math.h"

#include	"string.h"


#include	"intrusive_ptr.h"

#include	"preprocessor/repeat.h"
//#include	"delegate.h"

#include	"os/file_system.h"

#include	"type_id.h"
#include	"debug_break.h"

#include	"preprocessor/util.h"

#include	"iterator.h"
#include	"reflection_type.h"