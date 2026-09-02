using System;
using static ReflectionGenerator.Parser2.ParseExtensions;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ReflectionGenerator.Generator;

/// <summary>
/// nox::EntitySystem / nox::EntityLogic を継承した型の購読テーブルを出力する
/// </summary>
/// <remarks>
/// ヘッダにクラスを定義するだけで購読されるようにするため、型ごとの .g.cpp と
/// それらを集約するテーブル .g.cpp を書き出す。記述子は全て constexpr で、
/// 動的初期化は一切発生しない。
/// </remarks>
public sealed class EntityTypeGenerator
{
    #region 定義
    private const string ENTITY_SYSTEM_BASE_PREFIX = "nox::EntitySystem<";
    private const string ENTITY_LOGIC_BASE_PREFIX = "nox::EntityLogic<";
    private const string ENTITY_LOGIC_METHOD_ATTRIBUTE = "nox::attr::EntityLogicMethod";

    /// <summary>
    /// 型ごとの出力ファイル名の接頭辞。Directory.Build.targets のワイルドカードと対応する
    /// </summary>
    private const string OUTPUT_FILE_PREFIX = "entity_type_";
    private const string TABLE_FILE_NAME = "entity_type_table.g.cpp";

    private enum EntityTypeKind : byte
    {
        System,
        Logic,
    }

    private sealed class EntityMethodInfo
    {
        public required string Name { get; init; }

        /// <summary>
        /// メンバ関数の型 (例: void (nox::Foo &amp;) )
        /// </summary>
        public required string FunctionTypeFullName { get; init; }

        /// <summary>
        /// 属性の記述そのまま。生成器は中身を解釈しない
        /// </summary>
        public required string AttributeExpression { get; init; }
    }

    private sealed class EntityTypeInfo
    {
        public required EntityTypeKind Kind { get; init; }
        public required string FullName { get; init; }
        public required string SourceLocation { get; init; }
        public required List<EntityMethodInfo> MethodList { get; init; }

        /// <summary>
        /// ファイル名・識別子に使える形へ落とした名前
        /// </summary>
        public string SafeName => MakeSafeName(FullName);
    }
    #endregion

    #region 公開プロパティ
    /// <summary>
    /// 出力先ディレクトリ (gen/Platform/Configuration)
    /// </summary>
    public required string OutputDirectory { private get; init; }

    public required string Configuration { private get; init; }
    public required string Platform { private get; init; }
    public required string ConfigurationDefine { private get; init; }
    public required string PlatformDefine { private get; init; }

    /// <summary>
    /// 生成ファイルの先頭に足すインクルード群
    /// </summary>
    public required string AdditionalIncludeStr { private get; init; }

    /// <summary>
    /// 生成時エラーが発生したか
    /// </summary>
    public bool HasError { get; private set; }
    #endregion

    #region 公開メソッド
    public void Generate(IReadOnlyList<Parser2.NamespaceDecl> namespaceDeclList)
    {
        List<EntityTypeInfo> entityTypeList = new();

        foreach (Parser2.NamespaceDecl namespaceDecl in namespaceDeclList)
        {
            foreach (Parser2.RecordDecl recordDecl in namespaceDecl.RecordList)
            {
                CollectRecord(recordDecl, entityTypeList);
            }
        }

        //  出力順を安定させる (決定性)
        entityTypeList.Sort(static (lhs, rhs) => string.CompareOrdinal(lhs.FullName, rhs.FullName));

        //  古い型ごとのファイルを消してから書き出す (削除された型のファイルが残るとビルドが壊れるため)
        RemoveStaleFiles(entityTypeList);

        foreach (EntityTypeInfo entityType in entityTypeList)
        {
            WriteEntityTypeFile(entityType);
        }

        WriteTableFile(entityTypeList);
    }
    #endregion

    #region 非公開メソッド
    private static string MakeSafeName(string fullName)
    {
        StringBuilder stringBuilder = new(fullName.Length);
        foreach (char c in fullName)
        {
            stringBuilder.Append(char.IsLetterOrDigit(c) ? c : '_');
        }
        return stringBuilder.ToString();
    }

    private void CollectRecord(Parser2.RecordDecl recordDecl, List<EntityTypeInfo> entityTypeList)
    {
        //  入れ子クラスも対象にする
        foreach (Parser2.RecordDecl child in recordDecl.RecordList)
        {
            CollectRecord(child, entityTypeList);
        }

        if (recordDecl.IsDefinition == false)
        {
            return;
        }

        if (recordDecl.ReflectionGenerateKind == Parser2.ReflectionGenerateKind.IgnoreReflection)
        {
            return;
        }

        EntityTypeKind? kind = GetEntityTypeKind(recordDecl);
        if (kind.HasValue == false)
        {
            return;
        }

        //  生成器が名前を書けない型は購読できない
        if (IsInAnonymousNamespace(recordDecl))
        {
            Error(recordDecl, $"無名名前空間の型は購読できません。名前付き名前空間へ移すか、nox::EntityLogicMethodTableの特殊化を手書きしてください: {recordDecl.FullName}");
            return;
        }

        if (recordDecl.Name.Contains('<') || recordDecl.FullName.Contains('<'))
        {
            Error(recordDecl, $"クラステンプレートは購読できません。nox::EntityLogicMethodTableの特殊化を手書きしてください: {recordDecl.FullName}");
            return;
        }

        List<EntityMethodInfo> methodList = new();
        bool valid = true;

        foreach (Parser2.FunctionDecl functionDecl in recordDecl.FunctionList)
        {
            string? attributeExpression = TryGetEntityLogicMethodAttribute(functionDecl);
            if (attributeExpression == null)
            {
                continue;
            }

            if (kind.Value != EntityTypeKind.Logic)
            {
                Error(functionDecl, $"nox::attr::EntityLogicMethodはEntityLogicのメソッドにのみ付けられます: {functionDecl.FullName}");
                valid = false;
                continue;
            }

            if (functionDecl.AccessLevel != AccessLevel.Public)
            {
                Error(functionDecl, $"nox::attr::EntityLogicMethodを付けたメソッドはpublicである必要があります: {functionDecl.FullName}");
                valid = false;
                continue;
            }

            methodList.Add(new EntityMethodInfo()
            {
                Name = functionDecl.Name,
                FunctionTypeFullName = functionDecl.TypeInfo.FullName,
                AttributeExpression = attributeExpression,
            });
        }

        if (kind.Value == EntityTypeKind.Logic)
        {
            if (methodList.Count <= 0)
            {
                //  購読対象が無いだけなので、失敗はさせずに知らせる
                Warning(recordDecl, $"nox::EntityLogicを継承していますが、nox::attr::EntityLogicMethodを付けたメソッドが1つもないため購読されません: {recordDecl.FullName}");
                return;
            }

            if (IsConstructorPublic(recordDecl) == false)
            {
                Error(recordDecl, $"EntityLogicの(nox::World&, nox::EntityId)コンストラクタはpublicである必要があります: {recordDecl.FullName}");
                valid = false;
            }
        }

        if (valid == false)
        {
            return;
        }

        entityTypeList.Add(new EntityTypeInfo()
        {
            Kind = kind.Value,
            FullName = recordDecl.FullName,
            SourceLocation = recordDecl.Meta.SourceLocation.ToString(),
            MethodList = methodList,
        });
    }

    private static EntityTypeKind? GetEntityTypeKind(Parser2.RecordDecl recordDecl)
    {
        foreach (Parser2.BaseSpecifierDecl baseDecl in recordDecl.BaseSpan)
        {
            //  テンプレート実引数は解釈しない。生成したC++側が T::k_phase 等を自分で読む
            if (ContainsBasePrefix(baseDecl, ENTITY_SYSTEM_BASE_PREFIX))
            {
                return EntityTypeKind.System;
            }
            if (ContainsBasePrefix(baseDecl, ENTITY_LOGIC_BASE_PREFIX))
            {
                return EntityTypeKind.Logic;
            }
        }
        return null;
    }

    private static bool ContainsBasePrefix(Parser2.BaseSpecifierDecl baseDecl, string prefix)
    {
        return baseDecl.Name.Contains(prefix, StringComparison.Ordinal)
            || baseDecl.FullName.Contains(prefix, StringComparison.Ordinal);
    }

    private static bool IsInAnonymousNamespace(Parser2.RecordDecl recordDecl)
    {
        return recordDecl.FullName.Contains("(anonymous", StringComparison.Ordinal)
            || recordDecl.FullName.Contains("::::", StringComparison.Ordinal)
            || recordDecl.FullName.StartsWith("::", StringComparison.Ordinal)
            || recordDecl.Namespace.Contains("(anonymous", StringComparison.Ordinal);
    }

    private static bool IsConstructorPublic(Parser2.RecordDecl recordDecl)
    {
        bool found = false;
        foreach (Parser2.FunctionDecl functionDecl in recordDecl.FunctionList)
        {
            if (functionDecl.Name != recordDecl.Name)
            {
                continue;
            }

            if (functionDecl.FunctionAttributeFlags.IsAnyOn(
                Parser2.FunctionAttributeFlag.DefaultConstructor |
                Parser2.FunctionAttributeFlag.CopyConstructor |
                Parser2.FunctionAttributeFlag.MoveConstructor |
                Parser2.FunctionAttributeFlag.Destructor))
            {
                continue;
            }

            found = true;
            if (functionDecl.AccessLevel == AccessLevel.Public)
            {
                return true;
            }
        }

        //  コンストラクタが取れなかった場合は判断できないので通す (C++側でコンパイルエラーになる)
        return found == false;
    }

    private static string? TryGetEntityLogicMethodAttribute(Parser2.FunctionDecl functionDecl)
    {
        foreach (Parser2.AttributeDecl attributeDecl in functionDecl.AttributeSpan)
        {
            if (attributeDecl.AttrKind != Parser2.AttrKind.EngineAnnotate)
            {
                continue;
            }

            if (attributeDecl.Value.Contains(ENTITY_LOGIC_METHOD_ATTRIBUTE, StringComparison.Ordinal) == false)
            {
                continue;
            }

            //  引数は解釈せず、そのままC++の定数式として書き出す
            return attributeDecl.Value;
        }
        return null;
    }

    private static string GetLogicDescriptorName(EntityTypeInfo entityType)
    {
        return $"k_entity_logic_type_descriptor_{entityType.SafeName}";
    }

    private static void WriteLogicDescriptorDeclaration(CodeWriter codeWriter, EntityTypeInfo entityType)
    {
        codeWriter.WriteLine("namespace nox::gen");
        codeWriter.WriteLine("{");
        using (codeWriter.Indent())
        {
            codeWriter.WriteLine($"extern const nox::EntityLogicTypeDescriptor {GetLogicDescriptorName(entityType)};");
        }
        codeWriter.WriteLine("}");
    }

    private void Error(Parser2.DeclBase decl, string message)
    {
        HasError = true;
        Trace.ErrorLine(null, $"{decl.Meta.SourceLocation.ToString()}: {message}");
    }

    private static void Warning(Parser2.DeclBase decl, string message)
    {
        Trace.WarningLine(null, $"{decl.Meta.SourceLocation.ToString()}: {message}");
    }

    private void RemoveStaleFiles(IReadOnlyList<EntityTypeInfo> entityTypeList)
    {
        HashSet<string> keepFileNameSet = new(StringComparer.OrdinalIgnoreCase) { TABLE_FILE_NAME };
        foreach (EntityTypeInfo entityType in entityTypeList)
        {
            keepFileNameSet.Add($"{OUTPUT_FILE_PREFIX}{entityType.SafeName}.g.cpp");
        }

        if (Directory.Exists(OutputDirectory) == false)
        {
            return;
        }

        foreach (string filePath in Directory.EnumerateFiles(OutputDirectory, $"{OUTPUT_FILE_PREFIX}*.g.cpp"))
        {
            if (keepFileNameSet.Contains(Path.GetFileName(filePath)))
            {
                continue;
            }
            File.Delete(filePath);
        }
    }

    private void WriteFileHeader(CodeWriter codeWriter)
    {
        codeWriter.WriteLineSource();
        codeWriter.WriteNewLine();
        codeWriter.WriteIncludePch();
        if (string.IsNullOrEmpty(AdditionalIncludeStr) == false)
        {
            codeWriter.WriteLine(AdditionalIncludeStr);
        }
        codeWriter.WriteNewLine();

        codeWriter.WriteLinePPIf(ConfigurationDefine);
        codeWriter.WriteLinePPIf(PlatformDefine);
        codeWriter.WriteLinePPIfNot("__INTELLISENSE__");
        codeWriter.WriteNewLine();
    }

    private void WriteFileFooter(CodeWriter codeWriter)
    {
        codeWriter.WriteNewLine();
        codeWriter.WriteLinePPEndIf("__INTELLISENSE__");
        codeWriter.WriteLinePPEndIf(PlatformDefine);
        codeWriter.WriteLinePPEndIf(ConfigurationDefine);
    }

    private void WriteEntityTypeFile(EntityTypeInfo entityType)
    {
        string filePath = System.IO.Path.GetFullPath($"{OutputDirectory}/{OUTPUT_FILE_PREFIX}{entityType.SafeName}.g.cpp");
        using CodeWriter codeWriter = new(filePath);

        WriteFileHeader(codeWriter);
        codeWriter.WriteLine($"//\t{entityType.SourceLocation}");

        if (entityType.Kind == EntityTypeKind.Logic)
        {
            WriteLogicDescriptorDeclaration(codeWriter, entityType);
            codeWriter.WriteNewLine();

            //  フェーズは属性オブジェクトからC++側がコンパイル時に読み出す (生成器は引数を解釈しない)
            for (int i = 0; i < entityType.MethodList.Count; ++i)
            {
                codeWriter.WriteLine($"static constexpr decltype(auto) k_entity_logic_method_attribute_{entityType.SafeName}_{i.ToString()} = {entityType.MethodList[i].AttributeExpression};");
            }
            codeWriter.WriteNewLine();

            codeWriter.WriteLine("template<>");
            codeWriter.WriteLine($"struct nox::EntityLogicMethodTable<{entityType.FullName}>");
            codeWriter.WriteLine("{");
            using (codeWriter.Indent())
            {
                codeWriter.WriteLine("static constexpr nox::EntityLogicMethodDescriptor k_methods[]{");
                using (codeWriter.Indent())
                {
                    for (int i = 0; i < entityType.MethodList.Count; ++i)
                    {
                        EntityMethodInfo method = entityType.MethodList[i];

                        //  オーバーロードで曖昧にならないよう、必ず正確なメンバ関数ポインタ型へキャストする
                        string memberFunctionPointerType = $"nox::ToMemberFunctionPointerType<{method.FunctionTypeFullName}, {entityType.FullName}>";
                        string methodPointer = $"static_cast<{memberFunctionPointerType}>(&{entityType.FullName}::{method.Name})";
                        string phase = $"k_entity_logic_method_attribute_{entityType.SafeName}_{i.ToString()}.GetPhase()";

                        codeWriter.WriteLine($"nox::MakeEntityLogicMethodDescriptor<{methodPointer}, {phase}>(\"{method.Name}\"),");
                    }
                }
                codeWriter.WriteLine("};");
                codeWriter.WriteNewLine();

                codeWriter.WriteLine("[[nodiscard]] static constexpr std::span<const nox::EntityLogicMethodDescriptor> GetMethods()noexcept");
                codeWriter.WriteLine("{");
                using (codeWriter.Indent())
                {
                    codeWriter.WriteLine("return std::span<const nox::EntityLogicMethodDescriptor>(k_methods);");
                }
                codeWriter.WriteLine("}");
            }
            codeWriter.WriteLine("};");
            codeWriter.WriteNewLine();

            //  記述子の実体をこの翻訳単位の.rdataへ置く。
            //  テーブル側は宣言だけを見てアドレスを取るため、メソッド表を知らなくてよい。
            codeWriter.WriteLine($"constexpr nox::EntityLogicTypeDescriptor nox::gen::{GetLogicDescriptorName(entityType)} = nox::MakeEntityLogicTypeDescriptor<{entityType.FullName}>();");
        }
        else
        {
            codeWriter.WriteLine($"template const nox::EntitySystemTypeDescriptor nox::k_entity_system_type_descriptor<{entityType.FullName}>;");
        }

        WriteFileFooter(codeWriter);
    }

    private void WriteTableFile(IReadOnlyList<EntityTypeInfo> entityTypeList)
    {
        string filePath = System.IO.Path.GetFullPath($"{OutputDirectory}/{TABLE_FILE_NAME}");
        using CodeWriter codeWriter = new(filePath);

        WriteFileHeader(codeWriter);

        List<EntityTypeInfo> systemList = entityTypeList.Where(static x => x.Kind == EntityTypeKind.System).ToList();
        List<EntityTypeInfo> logicList = entityTypeList.Where(static x => x.Kind == EntityTypeKind.Logic).ToList();

        //  EntityLogicの記述子は型ごとの.g.cppで定義済み。ここでは宣言だけしてアドレスを集める
        //  (メソッド表の特殊化を知らなくてよいので、テーブル側は型の中身に依存しない)
        foreach (EntityTypeInfo entityType in logicList)
        {
            WriteLogicDescriptorDeclaration(codeWriter, entityType);
        }
        codeWriter.WriteNewLine();

        codeWriter.WriteLine("namespace");
        codeWriter.WriteLine("{");
        using (codeWriter.Indent())
        {
            WriteTableArray(codeWriter, "nox::EntitySystemTypeDescriptor", "k_entity_system_type_table", systemList);
            codeWriter.WriteNewLine();
            WriteTableArray(codeWriter, "nox::EntityLogicTypeDescriptor", "k_entity_logic_type_table", logicList);
            codeWriter.WriteNewLine();

            //  定数初期化されていることの証明。動的初期化が要るならconstinitがコンパイルエラーにする
            if (systemList.Count > 0)
            {
                codeWriter.WriteLine("constinit const nox::EntitySystemTypeDescriptor* const* const k_entity_system_type_table_head = k_entity_system_type_table;");
            }
            if (logicList.Count > 0)
            {
                codeWriter.WriteLine("constinit const nox::EntityLogicTypeDescriptor* const* const k_entity_logic_type_table_head = k_entity_logic_type_table;");
            }
        }
        codeWriter.WriteLine("}");
        codeWriter.WriteNewLine();

        WriteTableAccessor(codeWriter, "nox::EntitySystemTypeDescriptor", "GetEntitySystemTypes", "k_entity_system_type_table", systemList.Count);
        codeWriter.WriteNewLine();
        WriteTableAccessor(codeWriter, "nox::EntityLogicTypeDescriptor", "GetEntityLogicTypes", "k_entity_logic_type_table", logicList.Count);

        WriteFileFooter(codeWriter);
    }

    private static void WriteTableArray(
        CodeWriter codeWriter,
        string descriptorTypeName,
        string tableName,
        IReadOnlyList<EntityTypeInfo> entityTypeList)
    {
        if (entityTypeList.Count <= 0)
        {
            codeWriter.WriteLine($"//\t{tableName}: 購読された型はありません");
            return;
        }

        codeWriter.WriteLine($"constexpr const {descriptorTypeName}* {tableName}[]{{");
        using (codeWriter.Indent())
        {
            foreach (EntityTypeInfo entityType in entityTypeList)
            {
                string address = entityType.Kind == EntityTypeKind.System
                    ? $"&nox::k_entity_system_type_descriptor<{entityType.FullName}>"
                    : $"&nox::gen::{GetLogicDescriptorName(entityType)}";
                codeWriter.WriteLine($"{address},	//	{entityType.SourceLocation}");
            }
        }
        codeWriter.WriteLine("};");
    }

    private static void WriteTableAccessor(
        CodeWriter codeWriter,
        string descriptorTypeName,
        string functionName,
        string tableName,
        int count)
    {
        codeWriter.WriteLine($"std::span<const {descriptorTypeName}* const> nox::{functionName}()noexcept");
        codeWriter.WriteLine("{");
        using (codeWriter.Indent())
        {
            if (count <= 0)
            {
                codeWriter.WriteLine($"return std::span<const {descriptorTypeName}* const>();");
            }
            else
            {
                codeWriter.WriteLine($"return std::span<const {descriptorTypeName}* const>({tableName}, std::size({tableName}));");
            }
        }
        codeWriter.WriteLine("}");
    }
    #endregion
}
