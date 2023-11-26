using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    /// <summary>
    /// モジュール情報
    /// </summary>
    public class Module
    {
        /// <summary>
        /// モジュール名(プロジェクト名)
        /// Core-> "Core"
        /// Module/Render -> "Module_Render"
        /// </summary>
        public required string ModuleName { get; init; } 
    }

    /// <summary>
    /// パース情報基底
    /// </summary>
    public abstract class BaseInfo
    {
        public required Module Module { get; init; }
    }
}
