using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Nox.CustomTask
{
	public class Data
	{
		public string SolutionPath { get; set; } = string.Empty;

		public string Configuration { get; set; } = string.Empty;

		public string PreprocessorMacro { get; set; } = string.Empty;

		/// <summary>
		/// 追加オプション
		/// </summary>
		public string AdditionalOptions { get; set; } = string.Empty;


	}
}
