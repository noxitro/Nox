//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	core.h
///	@brief	core
#pragma once

#include	"entry_point.h"

#include	"scene_view.h"
#include	"engine_module.h"
#include	"core_module.h"
#include	"asset_manager.h"
#include	"asset.h"
#include	"asset_ref.h"
#include	"world.h"
#include	"component_type.h"
#include	"entity_access.h"
#include	"archetype.h"
#include	"entity_query.h"
#include	"entity_system.h"
#include	"entity_logic.h"
#include	"entity_logic_attribute.h"
#include	"entity_type_registry.h"
#include	"local_transform.h"

#if !NOX_MASTER
//	ECSセルフテスト用の型。ヘッダに定義するだけで購読されることの実証を兼ねる。
#include	"test/entity_ecs_test.h"
#endif // !NOX_MASTER

//	editor_remote
#include	"dev/remote/remote_system.g.h"
#include	"dev/remote/remote_log.g.h"
#include	"dev/socket_stream_writer.h"
#include	"dev/socket_stream_reader.h"
#include	"dev/editor_remote_server.h"
#include	"dev/net/socket_scheduler.h"
//	end editor_remote