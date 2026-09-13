// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System.Windows;
using System.Windows.Automation;
using System.Windows.Automation.Peers;

namespace NoxUI;

public static class LiveRegionBehavior
{
	public static readonly DependencyProperty AnnounceTextProperty = DependencyProperty.RegisterAttached(
		"AnnounceText",
		typeof(string),
		typeof(LiveRegionBehavior),
		new PropertyMetadata(string.Empty, OnAnnounceTextChanged));

	public static void SetAnnounceText(DependencyObject element, string value)
	{
		element.SetValue(AnnounceTextProperty, value);
	}

	public static string GetAnnounceText(DependencyObject element)
	{
		return (string)element.GetValue(AnnounceTextProperty);
	}

	private static void OnAnnounceTextChanged(DependencyObject dependencyObject, DependencyPropertyChangedEventArgs e)
	{
		if (dependencyObject is not FrameworkElement element || string.IsNullOrWhiteSpace(e.NewValue as string))
		{
			return;
		}

		void RaiseLiveRegionChanged()
		{
			AutomationPeer? peer = UIElementAutomationPeer.FromElement(element)
				?? UIElementAutomationPeer.CreatePeerForElement(element);
			peer?.RaiseAutomationEvent(AutomationEvents.LiveRegionChanged);
		}

		if (element.IsLoaded)
		{
			RaiseLiveRegionChanged();
			return;
		}

		RoutedEventHandler? loadedHandler = null;
		loadedHandler = (_, _) =>
		{
			element.Loaded -= loadedHandler;
			RaiseLiveRegionChanged();
		};
		element.Loaded += loadedHandler;
	}
}
