using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Info
{
    public enum TypeInfoKind : byte
    {
        Enum,
        Class,
        Union,
        Type,
        Variable,
        Function,
        GlobalDecl,
    }

    /// <summary>
    /// モジュール情報
    /// </summary>
    public struct ArtifactData : IEquatable<ArtifactData>
    {
        /// <summary>
        /// モジュール名(プロジェクト名)
        /// core-> "core"
        /// module/render -> "module_render"
        /// </summary>
        public readonly required string ModuleName { get; init; } 

        readonly bool IEquatable<ArtifactData>.Equals(ReflectionGenerator.Info.ArtifactData other)
        {
            return ModuleName == other.ModuleName;
        }

        public static readonly ArtifactData Invalid = new ArtifactData { ModuleName = "Invalid" };
    }

    public interface IBaseInfo
    {
        public ArtifactData Module { get; }
        public TypeInfoKind TypeInfoKind { get; }
    }

    /// <summary>
    /// パース情報基底
    /// </summary>
    public abstract class BaseInfo : IBaseInfo
    {
        public required ArtifactData Module { get; init; }

        public abstract TypeInfoKind TypeInfoKind { get; }

        private readonly IReadOnlyList<AttributeInfo> _AttributeInfoList;

        public required IReadOnlyList<AttributeInfo> AttributeInfoList
        {
            get => _AttributeInfoList;
            init
            {
                _AttributeInfoList = value;

				foreach (var attr in AttributeInfoList)
				{
					if (attr.AttrKind != ClangSharp.Interop.CX_AttrKind.CX_AttrKind_Annotate)
					{
						continue;
					}

					EngineAnnotateAttribute? engineAnnotateAttribute = attr as EngineAnnotateAttribute;
					if (engineAnnotateAttribute == null)
					{
						continue;
					}

					if (engineAnnotateAttribute.Value.Contains("nox::reflection::attr::Reflection"))
					{
						IsReflection = true;
					}
                    else if(engineAnnotateAttribute.Value.Contains("nox::reflection::attr::IgnoreReflection"))
                    {
                        IsIgnoreReflection = true;
                    }
				}
			}
		}

        /// <summary>
        /// エンジン側のリフレクション対象か
        /// TODO:   遅いならリファクタ
        /// </summary>
        public bool IsReflection { get; init; }

		public bool IsIgnoreReflection { get; init; }
		public string Hash { get; }

        private static int IndexCounter = 0;

        public BaseInfo()
        {
            int int32Hash = System.Threading.Interlocked.Increment(ref IndexCounter);
            Hash = int32Hash.ToString();
        }
    }
}
