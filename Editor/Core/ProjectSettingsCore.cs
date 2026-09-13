// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Text;

namespace Core;

internal class ProjectSettingRuntimeCore : ProjectSettingRuntime
{
    public override string Name => "Core";
}

internal class ProjectSettingEditorCore : ProjectSettingEditor
{
    public override string Name => "Core";

    /// <summary>
    /// 起動時にVisual Studioにアタッチするかどうか
    /// </summary>
    [DataMember]
    public bool VSAttachWithStartup { get; set; } = false;
}

public static partial class ProjectSettingsExtensions
{
    internal static ProjectSettingRuntimeCore RuntimeCore(this ProjectSettings self) => self.GetRuntimeSetting<ProjectSettingRuntimeCore>();
    internal static ProjectSettingEditorCore EditorCore(this ProjectSettings self) => self.GetEditorSetting<ProjectSettingEditorCore>();
}
