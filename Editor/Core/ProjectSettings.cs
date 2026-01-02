using System;
using System.Collections.Generic;
using System.Text;

namespace Core
{
	public enum PlatformType : byte
	{
		X64,
	}

	public enum ConfigurationType : byte
	{
		Debug,
		Release,
		Master
	}

	public class ProjectSettings
	{
		#region 公開プロパティ
		public string Platform { get; set; } = string.Empty;
		public string Configuration { get; set; } = string.Empty;
		#endregion
	}
}
