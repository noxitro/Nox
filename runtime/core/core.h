//	Copyright (C) 2023 NOX ENGINE All Rights Rserved.

///	@file	core.h
///	@brief	core
#pragma once

#include	"entity_node.h"
#include	"behavior.h"
#include	"entry_point.h"

#include	"scene_view.h"
#include	"transform.h"
#include	"scene_node.h"
#include	"scene_resource.h"
#include	"application.h"
#include	"engine_module.h"
#include	"core_entry.h"

//	edotor_remote
#include	"dev/remote/system.g.h"
#include	"dev/remote/log.g.h"
#include	"dev/socket_stream_writer.h"
#include	"dev/socket_stream_reader.h"
#include	"dev/editor_remote_server.h"
#include	"dev/net/socket_scheduler.h"
//	end editor_remote