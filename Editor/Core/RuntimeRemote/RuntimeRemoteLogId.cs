// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

namespace Core.RuntimeRemote.LogId;

internal readonly struct RuntimeRemote : Nox.LogId.ILogId<RuntimeRemote>
{
    public RuntimeRemote() { }
    string Nox.LogId.ILogId<RuntimeRemote>.Tag { get; } = "Runtime";
}
