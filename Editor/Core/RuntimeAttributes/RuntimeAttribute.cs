// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;

namespace Core.RuntimeAttributes;

	/// <summary>
	/// runtimeで定義した属性の基底クラス
	/// c#層ではこの属性を直接付与することはない。
	/// リフレクションシステムでruntimeで定義した属性とマッピングするための属性を付与するためのクラス
	/// </summary>
	public abstract class RuntimeAttribute : System.Attribute
	{

	}

	/// <summary>
	/// runtimeで定義した属性とマッピングするための属性
	/// </summary>
	[AttributeUsage(AttributeTargets.Class)]
	public sealed class RuntimeAttributeAttachAttribute : System.Attribute
	{
		/// <summary>
		/// runtimeで定義した型名
		/// </summary>
		public string FQN { get; init; }
		public RuntimeAttributeAttachAttribute(string fqn)
		{
			FQN = fqn;
		}
	}

	[RuntimeAttributeAttach("nox::attr::Resource")]
	public sealed class ResourcePathAttribute : RuntimeAttribute
	{
		public string Extension { get; init; }
		public uint Version { get; init; }

		public ResourcePathAttribute(string extension, uint version) 
		{
			Extension = extension;
			Version = version;
		}
	}

	[RuntimeAttributeAttach("nox::attr::dev::Hide")]
	public sealed class HideAttribute : RuntimeAttribute
	{

	}

	[RuntimeAttributeAttach("nox::attr::dev::Action")]
	public sealed class ActionAttribute : RuntimeAttribute
	{

	}

	[RuntimeAttributeAttach("nox::attr::UpdateOrder")]
	public sealed class UpdateOrderAttribute : RuntimeAttribute
	{
		public int Order { get; init; } = 0;

		public UpdateOrderAttribute(int order)
		{
			Order = order;
		}
	}

	[RuntimeAttributeAttach("nox::attr::dev::Property")]
	public sealed class PropertyAttribute : RuntimeAttribute
	{
		public string PropertyName { get; init; } = string.Empty;

		public PropertyAttribute() { }

		public PropertyAttribute(string name)
		{
			PropertyName = name;
		}
	}

	[RuntimeAttributeAttach("nox::attr::dev::PropertySetter")]
	public sealed class PropertySetterAttribute : RuntimeAttribute
	{
		public string PropertyName { get; init; } = string.Empty;

		public PropertySetterAttribute() { }

		public PropertySetterAttribute(string name)
		{
			PropertyName = name;
		}
	}

	[RuntimeAttributeAttach("nox::attr::dev::PropertyGetter")]
	public sealed class PropertyGetterAttribute : RuntimeAttribute
	{
		public string PropertyName { get; init; } = string.Empty;
		public PropertyGetterAttribute() { }
		public PropertyGetterAttribute(string name)
		{
			PropertyName = name;
		}
	}
