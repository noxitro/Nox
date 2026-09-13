// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System.Collections.Generic;
using System.Windows;

namespace Studio.Wpf.Themes;

internal static class ThemePaletteCatalog
{
    public static IReadOnlyList<ThemeBrushDescriptor> Brushes { get; } =
    [
        new(NoxThemeResourceKeys.WindowBackgroundBrush, "Window Background", "Surface"),
        new(NoxThemeResourceKeys.PanelBackgroundBrush, "Panel Background", "Surface"),
        new(NoxThemeResourceKeys.PanelBorderBrush, "Panel Border", "Surface"),
        new(NoxThemeResourceKeys.SurfaceBackgroundBrush, "Surface Background", "Surface"),
        new(NoxThemeResourceKeys.SurfaceRaisedBrush, "Surface Raised", "Surface"),
        new(NoxThemeResourceKeys.HeaderBackgroundBrush, "Header Background", "Surface"),
        new(NoxThemeResourceKeys.HeaderBackgroundActiveBrush, "Header Background Active", "Surface"),
        new(NoxThemeResourceKeys.HeaderBackgroundInactiveBrush, "Header Background Inactive", "Surface"),
        new(NoxThemeResourceKeys.HeaderBackgroundHoverBrush, "Header Background Hover", "Surface"),
        new(NoxThemeResourceKeys.GridLineBrush, "Grid Line", "Surface"),
        new(NoxThemeResourceKeys.SelectionBackgroundBrush, "Selection Background", "Surface"),
        new(NoxThemeResourceKeys.ControlBackgroundBrush, "Control Background", "Surface"),
        new(NoxThemeResourceKeys.ControlBackgroundHoverBrush, "Control Background Hover", "Surface"),
        new(NoxThemeResourceKeys.ControlBackgroundPressedBrush, "Control Background Pressed", "Surface"),
        new(NoxThemeResourceKeys.InputBackgroundBrush, "Input Background", "Surface"),
        new(NoxThemeResourceKeys.CommandBarBackgroundBrush, "Command Bar Background", "Surface"),
        new(NoxThemeResourceKeys.CommandBarBadgeBackgroundBrush, "Command Bar Badge Background", "Surface"),
        new(NoxThemeResourceKeys.CommandBadgeBackgroundBrush, "Command Badge Background", "Surface"),
        new(NoxThemeResourceKeys.IconBackgroundBrush, "Icon Background", "Surface"),
        new(NoxThemeResourceKeys.IconBorderBrush, "Icon Border", "Surface"),
        new(NoxThemeResourceKeys.ControlBorderBrush, "Control Border", "Surface"),
        new(NoxThemeResourceKeys.ControlBorderHoverBrush, "Control Border Hover", "Surface"),
        new(NoxThemeResourceKeys.ControlBorderPressedBrush, "Control Border Pressed", "Surface"),
        new(NoxThemeResourceKeys.BadgeBorderBrush, "Badge Border", "Surface"),

        new(NoxThemeResourceKeys.TextPrimaryBrush, "Text Primary", "Text"),
        new(NoxThemeResourceKeys.TextSecondaryBrush, "Text Secondary", "Text"),
        new(NoxThemeResourceKeys.TextHeadingBrush, "Text Heading", "Text"),
        new(NoxThemeResourceKeys.TextStrongBrush, "Text Strong", "Text"),
        new(NoxThemeResourceKeys.TextMutedBrush, "Text Muted", "Text"),
        new(NoxThemeResourceKeys.TextTertiaryBrush, "Text Tertiary", "Text"),
        new(NoxThemeResourceKeys.TextSubtleBrush, "Text Subtle", "Text"),

        new(NoxThemeResourceKeys.AccentBrush, "Accent", "Accent"),
        new(NoxThemeResourceKeys.AccentHoverBrush, "Accent Hover", "Accent"),
        new(NoxThemeResourceKeys.AccentForegroundBrush, "Accent Foreground", "Accent"),
        new(NoxThemeResourceKeys.AccentBorderBrush, "Accent Border", "Accent"),
        new(NoxThemeResourceKeys.AccentBorderHoverBrush, "Accent Border Hover", "Accent"),
        new(NoxThemeResourceKeys.DangerBrush, "Danger", "Accent"),
        new(NoxThemeResourceKeys.DangerForegroundBrush, "Danger Foreground", "Accent"),
        new(NoxThemeResourceKeys.DangerBorderBrush, "Danger Border", "Accent"),
        new(NoxThemeResourceKeys.ErrorTextBrush, "Error Text", "Accent"),

        new(NoxThemeResourceKeys.MenuBackgroundBrush, "Menu Background", "Navigation"),
        new(NoxThemeResourceKeys.TabBarBackgroundBrush, "Tab Bar Background", "Navigation"),
        new(NoxThemeResourceKeys.TabHoverBackgroundBrush, "Tab Hover Background", "Navigation"),
        new(NoxThemeResourceKeys.TabSelectedBackgroundBrush, "Tab Selected Background", "Navigation"),
        new(NoxThemeResourceKeys.TabHoverBorderBrush, "Tab Hover Border", "Navigation"),
        new(NoxThemeResourceKeys.TabSelectedBorderBrush, "Tab Selected Border", "Navigation"),

        new(NoxThemeResourceKeys.InspectorBackgroundBrush, "Inspector Background", "Inspector"),
        new(NoxThemeResourceKeys.InspectorHeaderBackgroundBrush, "Inspector Header Background", "Inspector"),
        new(NoxThemeResourceKeys.InspectorBorderBrush, "Inspector Border", "Inspector"),
        new(NoxThemeResourceKeys.InspectorTitleForegroundBrush, "Inspector Title Foreground", "Inspector"),
        new(NoxThemeResourceKeys.InspectorDescriptionForegroundBrush, "Inspector Description Foreground", "Inspector"),
        new(NoxThemeResourceKeys.InspectorPropertyNameForegroundBrush, "Inspector Property Name", "Inspector"),
        new(NoxThemeResourceKeys.InspectorPropertyValueForegroundBrush, "Inspector Property Value", "Inspector"),
    ];

    public static IReadOnlyList<(object Key, string BrushKey)> DerivedBrushMappings { get; } =
    [
        (SystemColors.WindowBrushKey, NoxThemeResourceKeys.WindowBackgroundBrush),
        (SystemColors.ControlBrushKey, NoxThemeResourceKeys.ControlBackgroundBrush),
        (SystemColors.ControlLightBrushKey, NoxThemeResourceKeys.ControlBackgroundHoverBrush),
        (SystemColors.ControlDarkBrushKey, NoxThemeResourceKeys.GridLineBrush),
        (SystemColors.WindowTextBrushKey, NoxThemeResourceKeys.TextPrimaryBrush),
        (SystemColors.ControlTextBrushKey, NoxThemeResourceKeys.TextPrimaryBrush),
        (SystemColors.HighlightBrushKey, NoxThemeResourceKeys.SelectionBackgroundBrush),
        (SystemColors.HighlightTextBrushKey, NoxThemeResourceKeys.TextPrimaryBrush),
        (SystemColors.InactiveSelectionHighlightBrushKey, NoxThemeResourceKeys.SelectionBackgroundBrush),
        (SystemColors.InactiveSelectionHighlightTextBrushKey, NoxThemeResourceKeys.TextPrimaryBrush),

        ("PanelBorderBrush", NoxThemeResourceKeys.GridLineBrush),
        ("FloatingDocumentWindowBackground", NoxThemeResourceKeys.PanelBackgroundBrush),
        ("FloatingDocumentWindowBorder", NoxThemeResourceKeys.GridLineBrush),
        ("FloatingToolWindowBackground", NoxThemeResourceKeys.PanelBackgroundBrush),
        ("FloatingToolWindowBorder", NoxThemeResourceKeys.GridLineBrush),

        ("DocumentWellTabSelectedActiveBackground", NoxThemeResourceKeys.HeaderBackgroundActiveBrush),
        ("DocumentWellTabSelectedActiveText", NoxThemeResourceKeys.TextPrimaryBrush),
        ("DocumentWellTabSelectedInactiveBackground", NoxThemeResourceKeys.HeaderBackgroundInactiveBrush),
        ("DocumentWellTabSelectedInactiveText", NoxThemeResourceKeys.TextSecondaryBrush),
        ("DocumentWellTabUnselectedBackground", NoxThemeResourceKeys.HeaderBackgroundBrush),
        ("DocumentWellTabUnselectedText", NoxThemeResourceKeys.TextSecondaryBrush),
        ("DocumentWellTabUnselectedHoveredBackground", NoxThemeResourceKeys.TabHoverBackgroundBrush),
        ("DocumentWellTabUnselectedHoveredText", NoxThemeResourceKeys.TextSecondaryBrush),

        ("ToolWindowCaptionActiveBackground", NoxThemeResourceKeys.HeaderBackgroundActiveBrush),
        ("ToolWindowCaptionActiveText", NoxThemeResourceKeys.TextPrimaryBrush),
        ("ToolWindowCaptionActiveGrip", NoxThemeResourceKeys.AccentBrush),
        ("ToolWindowCaptionInactiveBackground", NoxThemeResourceKeys.HeaderBackgroundInactiveBrush),
        ("ToolWindowCaptionInactiveText", NoxThemeResourceKeys.TextSecondaryBrush),
        ("ToolWindowCaptionInactiveGrip", NoxThemeResourceKeys.GridLineBrush),
        ("ToolWindowTabSelectedActiveBackground", NoxThemeResourceKeys.HeaderBackgroundActiveBrush),
        ("ToolWindowTabSelectedActiveText", NoxThemeResourceKeys.TextPrimaryBrush),
        ("ToolWindowTabSelectedInactiveBackground", NoxThemeResourceKeys.HeaderBackgroundInactiveBrush),
        ("ToolWindowTabSelectedInactiveText", NoxThemeResourceKeys.TextSecondaryBrush),
        ("ToolWindowTabUnselectedBackground", NoxThemeResourceKeys.HeaderBackgroundBrush),
        ("ToolWindowTabUnselectedText", NoxThemeResourceKeys.TextSecondaryBrush),
        ("ToolWindowTabUnselectedHoveredBackground", NoxThemeResourceKeys.TabHoverBackgroundBrush),
        ("ToolWindowTabUnselectedHoveredText", NoxThemeResourceKeys.TextSecondaryBrush),
    ];
}
