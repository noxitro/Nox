// Copyright (c) 2023-2026 noxitro
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ReflectionGenerator.Generator;

/// <summary>
/// nox::EntitySystem / nox::EntityLogic を継承した型と、nox::Service の属性付きメソッド、
/// 属性付きのグローバル関数 (Task) の購読テーブルを出力する
/// </summary>
/// <remarks>
/// ヘッダにクラスを定義するだけで購読されるようにするため、型ごとの .g.cpp と
/// それらを集約するテーブル .g.cpp を書き出す。記述子は全て constexpr で、
/// 動的初期化は一切発生しない。
///
/// [指針] 生成器は「人間が公開エンジンAPIだけを使って手で書けるコード」しか書かない。
/// 生成器は判断をしない。C++ で検査できることは全て C++ 側の static_assert で検査する。
/// したがってこの生成器の仕事は「型を数え上げ、属性の付いたメソッドを数え上げる」だけであり、
/// コンストラクタの形・引数の妥当性・フェーズ値の解釈などには一切踏み込まない。
/// 生成時エラーにするのは「生成器がそもそもコードを書けない」場合(無名名前空間・クラステンプレート・
/// 関数テンプレート・対象外への属性付与)に限る。
///
/// Service の型は EntityLogic と同じ出力形 (メソッド表の特殊化 + 型ごとの記述子) を持ち、
/// 型ごとのファイル名も同じ接頭辞にしてある (Directory.Build.targets のワイルドカードで取り込まれるため)。
/// Task は型を持たないので、型ごとのファイルは作らずテーブル側に直接記述子を書く。
/// </remarks>
public sealed class EntityTypeGenerator
{
    #region 定義
    private const string ENTITY_SYSTEM_BASE_PREFIX = "nox::EntitySystem<";
    private const string ENTITY_LOGIC_BASE_PREFIX = "nox::EntityLogic<";
    private const string ENTITY_LOGIC_METHOD_ATTRIBUTE = "nox::attr::EntityLogicMethod";
    private const string SERVICE_BASE_FULL_NAME = "nox::Service";
    private const string SERVICE_METHOD_ATTRIBUTE = "nox::attr::ServiceMethod";
    private const string UPDATER_TASK_ATTRIBUTE = "nox::attr::UpdaterTask";

    /// <summary>
    /// 基底を辿る深さの上限 (循環した入力で止まらなくなるのを防ぐだけの値)
    /// </summary>
    private const int MAX_BASE_DEPTH = 64;

    /// <summary>
    /// 生成コードが宣言する、更新メソッド1つ分のタグ型の接頭辞
    /// </summary>
    private const string METHOD_TAG_PREFIX = "EntityLogicMethodTag_";

    /// <summary>
    /// 生成コードが宣言する、Service のメソッド1つ分のタグ型の接頭辞
    /// </summary>
    private const string SERVICE_METHOD_TAG_PREFIX = "ServiceMethodTag_";

    /// <summary>
    /// 型ごとの出力ファイル名の接頭辞。Directory.Build.targets のワイルドカードと対応する
    /// </summary>
    private const string OUTPUT_FILE_PREFIX = "entity_type_";
    private const string TABLE_FILE_NAME = "entity_type_table.g.cpp";

    private enum EntityTypeKind : byte
    {
        System,
        Logic,
        Service,
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

    private sealed class UpdaterTaskInfo
    {
        /// <summary>
        /// 完全修飾関数名
        /// </summary>
        public required string FullName { get; init; }

        /// <summary>
        /// 関数の型 (例: void (nox::Foo &amp;) )
        /// </summary>
        public required string FunctionTypeFullName { get; init; }

        /// <summary>
        /// 属性の記述そのまま。生成器は中身を解釈しない
        /// </summary>
        public required string AttributeExpression { get; init; }

        public required string SourceLocation { get; init; }
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

    #region 非公開フィールド
    /// <summary>
    /// 完全修飾名から型定義を引く表。Service の判定で基底を辿るのに使う
    /// </summary>
    private readonly Dictionary<string, Parser2.RecordDecl> _RecordDeclDict = new(StringComparer.Ordinal);

    /// <summary>
    /// 宣言IDから型定義を引く表。基底の名前が綴りどおりに引けなかったときの予備
    /// </summary>
    private readonly Dictionary<uint, Parser2.RecordDecl> _RecordDeclWithHashDict = new();
    #endregion

    #region 公開メソッド
    public void Generate(IReadOnlyList<Parser2.NamespaceDecl> namespaceDeclList)
    {
        List<EntityTypeInfo> entityTypeList = new();
        List<UpdaterTaskInfo> taskList = new();
        HashSet<string> taskKeySet = new(StringComparer.Ordinal);

        //  Service は間接的な派生 (Service を継承した基底の派生) もあるので、先に型定義の表を作る
        _RecordDeclDict.Clear();
        _RecordDeclWithHashDict.Clear();
        foreach (Parser2.NamespaceDecl namespaceDecl in namespaceDeclList)
        {
            foreach (Parser2.RecordDecl recordDecl in namespaceDecl.RecordList)
            {
                AddRecordDecl(recordDecl);
            }
        }

        foreach (Parser2.NamespaceDecl namespaceDecl in namespaceDeclList)
        {
            foreach (Parser2.RecordDecl recordDecl in namespaceDecl.RecordList)
            {
                CollectRecord(recordDecl, entityTypeList);
            }

            foreach (Parser2.TemplateClassDecl templateClassDecl in namespaceDecl.TemplateRecordList)
            {
                CollectTemplateRecord(templateClassDecl, entityTypeList);
            }

            foreach (Parser2.FunctionDecl functionDecl in namespaceDecl.FunctionList)
            {
                CollectFunction(functionDecl, taskList, taskKeySet);
            }
        }

        //  出力順を安定させる (決定性)
        entityTypeList.Sort(static (lhs, rhs) => string.CompareOrdinal(lhs.FullName, rhs.FullName));
        taskList.Sort(static (lhs, rhs) =>
        {
            int compare = string.CompareOrdinal(lhs.FullName, rhs.FullName);
            return compare != 0 ? compare : string.CompareOrdinal(lhs.FunctionTypeFullName, rhs.FunctionTypeFullName);
        });

        //  古い型ごとのファイルを消してから書き出す (削除された型のファイルが残るとビルドが壊れるため)
        RemoveStaleFiles(entityTypeList);

        foreach (EntityTypeInfo entityType in entityTypeList)
        {
            WriteEntityTypeFile(entityType);
        }

        WriteTableFile(entityTypeList, taskList);
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

        foreach (Parser2.TemplateClassDecl child in recordDecl.TemplateRecordList)
        {
            CollectTemplateRecord(child, entityTypeList);
        }

        if (recordDecl.IsDefinition == false)
        {
            return;
        }

        if (recordDecl.ReflectionGenerateKind == Parser2.ReflectionGenerateKind.IgnoreReflection)
        {
            return;
        }

        EntityTypeKind? kind = GetEntityTypeKind(recordDecl.BaseSpan);
        if (kind.HasValue == false && IsServiceDerived(recordDecl.BaseSpan, 0))
        {
            kind = EntityTypeKind.Service;
        }

        //  属性の数え上げは基底に関わらず行う。
        //  対象外へ付けた属性を「対象外だから」と黙って捨てると、
        //  更新メソッドが呼ばれない理由が誰にも分からなくなるため。
        List<EntityMethodInfo> methodList = new();
        bool valid = true;

        foreach (Parser2.FunctionDecl functionDecl in recordDecl.FunctionList)
        {
            if (TryGetAttributeExpression(functionDecl, UPDATER_TASK_ATTRIBUTE) != null)
            {
                Error(functionDecl, $"nox::attr::UpdaterTaskは名前空間スコープの関数にのみ付けられます: {functionDecl.FullName}");
                valid = false;
            }

            string? serviceAttributeExpression = TryGetAttributeExpression(functionDecl, SERVICE_METHOD_ATTRIBUTE);
            if (serviceAttributeExpression != null)
            {
                if (kind != EntityTypeKind.Service)
                {
                    Error(functionDecl, $"nox::attr::ServiceMethodはnox::Serviceの派生型のメソッドにのみ付けられます: {functionDecl.FullName}");
                    valid = false;
                }
                else
                {
                    //  privateでもよい (EntityLogicと同じく明示的実体化で呼ぶ)
                    methodList.Add(new EntityMethodInfo()
                    {
                        Name = functionDecl.Name,
                        FunctionTypeFullName = functionDecl.TypeInfo.FullName,
                        AttributeExpression = serviceAttributeExpression,
                    });
                }
            }

            string? attributeExpression = TryGetAttributeExpression(functionDecl, ENTITY_LOGIC_METHOD_ATTRIBUTE);
            if (attributeExpression == null)
            {
                continue;
            }

            if (kind != EntityTypeKind.Logic)
            {
                Error(functionDecl, $"nox::attr::EntityLogicMethodはEntityLogicのメソッドにのみ付けられます: {functionDecl.FullName}");
                valid = false;
                continue;
            }

            //  privateでもよい (明示的実体化のテンプレート実引数はアクセス検査の対象外)
            methodList.Add(new EntityMethodInfo()
            {
                Name = functionDecl.Name,
                FunctionTypeFullName = functionDecl.TypeInfo.FullName,
                AttributeExpression = attributeExpression,
            });
        }

        if (kind.HasValue == false)
        {
            //  EntitySystem/EntityLogic/Serviceのどれでもない型。付け間違いがあれば上で報告済み
            return;
        }

        if (kind.Value == EntityTypeKind.Service && methodList.Count <= 0)
        {
            //  属性付きメソッドの無いServiceはノードを持たないだけ (大半のServiceはこれ)。
            //  無名名前空間やテンプレートでも書くものが無いので、下の検査より先に抜ける
            return;
        }

        string methodTableName = kind.Value == EntityTypeKind.Service ? "nox::ServiceMethodTable" : "nox::EntityLogicMethodTable";

        //  生成器が名前を書けない型は購読できない
        if (IsInAnonymousNamespace(recordDecl))
        {
            Error(recordDecl, $"無名名前空間の型は購読できません。名前付き名前空間へ移すか、{methodTableName}の特殊化を手書きしてください: {recordDecl.FullName}");
            return;
        }

        if (recordDecl.Name.Contains('<') || recordDecl.FullName.Contains('<'))
        {
            Error(recordDecl, $"クラステンプレートは購読できません。{methodTableName}の特殊化を手書きしてください: {recordDecl.FullName}");
            return;
        }

        if (kind.Value == EntityTypeKind.Logic && methodList.Count <= 0)
        {
            //  購読対象が無いだけなので、失敗はさせずに知らせる
            Warning(recordDecl, $"nox::EntityLogicを継承していますが、nox::attr::EntityLogicMethodを付けたメソッドが1つもないため購読されません: {recordDecl.FullName}");
            return;
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

    /// <summary>
    /// クラステンプレートを診断する
    /// </summary>
    /// <remarks>
    /// パーサはクラステンプレートのメンバまでは走査していない。ここで当てにしてよいのは
    /// 名前・基底・ソース位置だけなので、購読は試みずエラーを出して終わる。
    /// </remarks>
    private void CollectTemplateRecord(Parser2.TemplateClassDecl templateClassDecl, List<EntityTypeInfo> entityTypeList)
    {
        //  入れ子の型も対象にする
        foreach (Parser2.RecordDecl child in templateClassDecl.RecordList)
        {
            CollectRecord(child, entityTypeList);
        }

        foreach (Parser2.TemplateClassDecl child in templateClassDecl.TemplateRecordList)
        {
            CollectTemplateRecord(child, entityTypeList);
        }

        if (templateClassDecl.IsDefinition == false)
        {
            return;
        }

        if (templateClassDecl.ReflectionGenerateKind == Parser2.ReflectionGenerateKind.IgnoreReflection)
        {
            return;
        }

        if (GetEntityTypeKind(templateClassDecl.BaseSpan).HasValue == false
            && GetEntityTypeKind(templateClassDecl.BaseTypeNameSpan).HasValue == false)
        {
            return;
        }

        Error(templateClassDecl, $"クラステンプレートは購読できません。nox::EntityLogicMethodTableの特殊化を手書きしてください: {templateClassDecl.FullName}");
    }

    private static EntityTypeKind? GetEntityTypeKind(ReadOnlySpan<Parser2.BaseSpecifierDecl> baseSpan)
    {
        foreach (Parser2.BaseSpecifierDecl baseDecl in baseSpan)
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

    /// <summary>
    /// 基底の綴りだけから種別を判定する (クラステンプレート用)
    /// </summary>
    private static EntityTypeKind? GetEntityTypeKind(ReadOnlySpan<string> baseTypeNameSpan)
    {
        foreach (string baseTypeName in baseTypeNameSpan)
        {
            if (baseTypeName.Contains(ENTITY_SYSTEM_BASE_PREFIX, StringComparison.Ordinal))
            {
                return EntityTypeKind.System;
            }
            if (baseTypeName.Contains(ENTITY_LOGIC_BASE_PREFIX, StringComparison.Ordinal))
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

    private static bool IsInAnonymousNamespace(Parser2.TypeDecl decl)
    {
        return decl.FullName.Contains("(anonymous", StringComparison.Ordinal)
            || decl.FullName.Contains("::::", StringComparison.Ordinal)
            || decl.FullName.StartsWith("::", StringComparison.Ordinal)
            || decl.Namespace.Contains("(anonymous", StringComparison.Ordinal);
    }

    private static string? TryGetAttributeExpression(Parser2.FunctionDecl functionDecl, string attributeName)
    {
        foreach (Parser2.AttributeDecl attributeDecl in functionDecl.AttributeSpan)
        {
            if (attributeDecl.AttrKind != Parser2.AttrKind.EngineAnnotate)
            {
                continue;
            }

            if (attributeDecl.Value.Contains(attributeName, StringComparison.Ordinal) == false)
            {
                continue;
            }

            //  引数は解釈せず、そのままC++の定数式として書き出す
            return attributeDecl.Value;
        }
        return null;
    }

    private void AddRecordDecl(Parser2.RecordDecl recordDecl)
    {
        foreach (Parser2.RecordDecl child in recordDecl.RecordList)
        {
            AddRecordDecl(child);
        }

        //  前方宣言より定義を優先する (基底は定義にしか載っていない)
        if (recordDecl.IsDefinition || _RecordDeclDict.ContainsKey(recordDecl.FullName) == false)
        {
            _RecordDeclDict[recordDecl.FullName] = recordDecl;
        }
        _RecordDeclWithHashDict.TryAdd(recordDecl.DeclHash, recordDecl);
    }

    /// <summary>
    /// 基底の綴りを完全修飾名の形へ揃える (先頭の :: と class / struct を外す)
    /// </summary>
    private static string NormalizeBaseName(string name)
    {
        string normalized = name.Trim();
        if (normalized.StartsWith("class ", StringComparison.Ordinal))
        {
            normalized = normalized.Substring("class ".Length);
        }
        else if (normalized.StartsWith("struct ", StringComparison.Ordinal))
        {
            normalized = normalized.Substring("struct ".Length);
        }
        if (normalized.StartsWith("::", StringComparison.Ordinal))
        {
            normalized = normalized.Substring(2);
        }
        return normalized;
    }

    private static bool IsServiceBase(Parser2.BaseSpecifierDecl baseDecl)
    {
        //  EntitySystem / EntityLogic の判定と同じく、完全修飾名と綴りの両方を見る
        return string.Equals(NormalizeBaseName(baseDecl.FullName), SERVICE_BASE_FULL_NAME, StringComparison.Ordinal)
            || string.Equals(NormalizeBaseName(baseDecl.Name), SERVICE_BASE_FULL_NAME, StringComparison.Ordinal);
    }

    private Parser2.RecordDecl? TryGetBaseRecordDecl(Parser2.BaseSpecifierDecl baseDecl)
    {
        foreach (string name in new[] { NormalizeBaseName(baseDecl.FullName), NormalizeBaseName(baseDecl.Name) })
        {
            if (name.Length > 0 && _RecordDeclDict.TryGetValue(name, out Parser2.RecordDecl? recordDecl))
            {
                return recordDecl;
            }
        }

        if (baseDecl.PointeeDeclHash != 0 && _RecordDeclWithHashDict.TryGetValue(baseDecl.PointeeDeclHash, out Parser2.RecordDecl? hashedRecordDecl))
        {
            return hashedRecordDecl;
        }
        return null;
    }

    /// <summary>
    /// 基底を辿って nox::Service の派生型か判定する (間接的な派生も含む)
    /// </summary>
    /// <remarks>
    /// 名前しか見ない。テンプレート実引数に依存する基底 (クラステンプレートを経由した派生) は辿れない。
    /// </remarks>
    private bool IsServiceDerived(ReadOnlySpan<Parser2.BaseSpecifierDecl> baseSpan, int depth)
    {
        if (depth > MAX_BASE_DEPTH)
        {
            return false;
        }

        foreach (Parser2.BaseSpecifierDecl baseDecl in baseSpan)
        {
            if (IsServiceBase(baseDecl))
            {
                return true;
            }

            Parser2.RecordDecl? baseRecordDecl = TryGetBaseRecordDecl(baseDecl);
            if (baseRecordDecl != null && IsServiceDerived(baseRecordDecl.BaseSpan, depth + 1))
            {
                return true;
            }
        }
        return false;
    }

    /// <summary>
    /// 名前空間スコープの関数から Task を集める
    /// </summary>
    private void CollectFunction(Parser2.FunctionDecl functionDecl, List<UpdaterTaskInfo> taskList, HashSet<string> taskKeySet)
    {
        //  メンバ関数用の属性を名前空間スコープの関数へ付けた場合も、黙って捨てずに知らせる
        if (TryGetAttributeExpression(functionDecl, SERVICE_METHOD_ATTRIBUTE) != null)
        {
            Error(functionDecl, $"nox::attr::ServiceMethodはnox::Serviceの派生型のメソッドにのみ付けられます: {functionDecl.FullName}");
        }
        if (TryGetAttributeExpression(functionDecl, ENTITY_LOGIC_METHOD_ATTRIBUTE) != null)
        {
            Error(functionDecl, $"nox::attr::EntityLogicMethodはEntityLogicのメソッドにのみ付けられます: {functionDecl.FullName}");
        }

        string? attributeExpression = TryGetAttributeExpression(functionDecl, UPDATER_TASK_ATTRIBUTE);
        if (attributeExpression == null)
        {
            return;
        }

        //  生成器が名前を書けない関数は購読できない
        if (IsInAnonymousNamespace(functionDecl))
        {
            Error(functionDecl, $"無名名前空間の関数は購読できません。名前付き名前空間へ移してください: {functionDecl.FullName}");
            return;
        }

        if (functionDecl.TypeInfo is Parser2.TemplateFunctionTypeInfo || functionDecl.FullName.Contains('<'))
        {
            Error(functionDecl, $"関数テンプレートは購読できません。テンプレートでない関数から呼んでください: {functionDecl.FullName}");
            return;
        }

        //  宣言と定義が別の宣言として並んでも1つにまとめる
        if (taskKeySet.Add($"{functionDecl.FullName}|{functionDecl.TypeInfo.FullName}") == false)
        {
            return;
        }

        taskList.Add(new UpdaterTaskInfo()
        {
            FullName = functionDecl.FullName,
            FunctionTypeFullName = functionDecl.TypeInfo.FullName,
            AttributeExpression = attributeExpression,
            SourceLocation = functionDecl.Meta.SourceLocation.ToString(),
        });
    }

    private static string GetLogicDescriptorName(EntityTypeInfo entityType)
    {
        return $"k_entity_logic_type_descriptor_{entityType.SafeName}";
    }

    private static string GetServiceDescriptorName(EntityTypeInfo entityType)
    {
        return $"k_service_method_type_descriptor_{entityType.SafeName}";
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

    private static void WriteServiceDescriptorDeclaration(CodeWriter codeWriter, EntityTypeInfo entityType)
    {
        codeWriter.WriteLine("namespace nox::gen");
        codeWriter.WriteLine("{");
        using (codeWriter.Indent())
        {
            codeWriter.WriteLine($"extern const nox::ServiceMethodTypeDescriptor {GetServiceDescriptorName(entityType)};");
        }
        codeWriter.WriteLine("}");
    }

    private static string GetMethodTagName(EntityTypeInfo entityType, int methodIndex)
    {
        string prefix = entityType.Kind == EntityTypeKind.Service ? SERVICE_METHOD_TAG_PREFIX : METHOD_TAG_PREFIX;
        return $"{prefix}{entityType.SafeName}_{methodIndex.ToString()}";
    }

    /// <summary>
    /// 更新メソッド1つにつき、タグ型と実行サンクの明示的実体化を書き出す
    /// </summary>
    /// <remarks>
    /// 明示的実体化の宣言に現れる名前はアクセス検査の対象外 ([temp.explicit]) なので、
    /// private なメソッドでも対象クラスに friend を足さずに購読できる。
    /// public / private で経路を分けず、常にこの形で出力する。
    /// </remarks>
    private static void WriteMethodTags(CodeWriter codeWriter, EntityTypeInfo entityType)
    {
        codeWriter.WriteLine("namespace nox::gen");
        codeWriter.WriteLine("{");
        using (codeWriter.Indent())
        {
            for (int i = 0; i < entityType.MethodList.Count; ++i)
            {
                EntityMethodInfo method = entityType.MethodList[i];
                string tagName = GetMethodTagName(entityType, i);

                codeWriter.WriteLine($"struct {tagName}");
                codeWriter.WriteLine("{");
                using (codeWriter.Indent())
                {
                    codeWriter.WriteLine($"using OwnerType = {entityType.FullName};");
                    //  オーバーロードで曖昧にならないよう、必ず正確なメンバ関数ポインタ型へキャストする
                    codeWriter.WriteLine($"using MethodPointerType = nox::ToMemberFunctionPointerType<{method.FunctionTypeFullName}, {entityType.FullName}>;");
                    codeWriter.WriteLine("using Signature = typename nox::EntityMethodTraits<MethodPointerType>::Signature;");
                    if (entityType.Kind == EntityTypeKind.Service)
                    {
                        codeWriter.WriteLine($"friend void InvokeServiceMethod({tagName}, nox::Service&, nox::World&);");
                    }
                    else
                    {
                        codeWriter.WriteLine($"friend void InvokeEntityLogicMethod({tagName}, void*, nox::World&, nox::Archetype&, nox::ArchetypeLocation, nox::EntityId);");
                    }
                }
                codeWriter.WriteLine("};");
            }
        }
        codeWriter.WriteLine("}");
        codeWriter.WriteNewLine();

        string invokerName = entityType.Kind == EntityTypeKind.Service
            ? "nox::gen::PrivateServiceMethodInvoker"
            : "nox::gen::PrivateEntityLogicMethodInvoker";
        for (int i = 0; i < entityType.MethodList.Count; ++i)
        {
            EntityMethodInfo method = entityType.MethodList[i];
            string tagName = GetMethodTagName(entityType, i);
            string methodPointer = $"static_cast<nox::gen::{tagName}::MethodPointerType>(&{entityType.FullName}::{method.Name})";

            codeWriter.WriteLine($"template struct {invokerName}<nox::gen::{tagName}, {methodPointer}>;");
        }
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

            WriteMethodTags(codeWriter, entityType);
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
                        string tagName = GetMethodTagName(entityType, i);
                        string phase = $"k_entity_logic_method_attribute_{entityType.SafeName}_{i.ToString()}.GetPhase()";

                        //  記述子はメソッド名を綴らず、タグ型が持つ型情報だけを読む
                        codeWriter.WriteLine($"nox::detail::MakeEntityLogicMethodDescriptorViaTag<nox::gen::{tagName}, {phase}>(\"{method.Name}\"),");
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
        else if (entityType.Kind == EntityTypeKind.Service)
        {
            WriteServiceDescriptorDeclaration(codeWriter, entityType);
            codeWriter.WriteNewLine();

            //  フェーズと実行スレッドは属性オブジェクトからC++側がコンパイル時に読み出す (生成器は引数を解釈しない)
            for (int i = 0; i < entityType.MethodList.Count; ++i)
            {
                codeWriter.WriteLine($"static constexpr decltype(auto) k_service_method_attribute_{entityType.SafeName}_{i.ToString()} = {entityType.MethodList[i].AttributeExpression};");
            }
            codeWriter.WriteNewLine();

            WriteMethodTags(codeWriter, entityType);
            codeWriter.WriteNewLine();

            codeWriter.WriteLine("template<>");
            codeWriter.WriteLine($"struct nox::ServiceMethodTable<{entityType.FullName}>");
            codeWriter.WriteLine("{");
            using (codeWriter.Indent())
            {
                codeWriter.WriteLine("static constexpr nox::ServiceMethodDescriptor k_methods[]{");
                using (codeWriter.Indent())
                {
                    for (int i = 0; i < entityType.MethodList.Count; ++i)
                    {
                        EntityMethodInfo method = entityType.MethodList[i];
                        string tagName = GetMethodTagName(entityType, i);
                        string attribute = $"k_service_method_attribute_{entityType.SafeName}_{i.ToString()}";

                        //  記述子はメソッド名を綴らず、タグ型が持つ型情報だけを読む
                        codeWriter.WriteLine($"nox::detail::MakeServiceMethodDescriptorViaTag<nox::gen::{tagName}, {attribute}.GetPhase(), {attribute}.GetThreadAffinity()>(\"{method.Name}\"),");
                    }
                }
                codeWriter.WriteLine("};");
                codeWriter.WriteNewLine();

                codeWriter.WriteLine("[[nodiscard]] static constexpr std::span<const nox::ServiceMethodDescriptor> GetMethods()noexcept");
                codeWriter.WriteLine("{");
                using (codeWriter.Indent())
                {
                    codeWriter.WriteLine("return std::span<const nox::ServiceMethodDescriptor>(k_methods);");
                }
                codeWriter.WriteLine("}");
            }
            codeWriter.WriteLine("};");
            codeWriter.WriteNewLine();

            //  記述子の実体をこの翻訳単位の.rdataへ置く (EntityLogicと同じ理由)
            codeWriter.WriteLine($"constexpr nox::ServiceMethodTypeDescriptor nox::gen::{GetServiceDescriptorName(entityType)} = nox::MakeServiceMethodTypeDescriptor<{entityType.FullName}>();");
        }
        else
        {
            codeWriter.WriteLine($"template const nox::EntitySystemTypeDescriptor nox::k_entity_system_type_descriptor<{entityType.FullName}>;");
        }

        WriteFileFooter(codeWriter);
    }

    private void WriteTableFile(IReadOnlyList<EntityTypeInfo> entityTypeList, IReadOnlyList<UpdaterTaskInfo> taskList)
    {
        string filePath = System.IO.Path.GetFullPath($"{OutputDirectory}/{TABLE_FILE_NAME}");
        using CodeWriter codeWriter = new(filePath);

        WriteFileHeader(codeWriter);

        List<EntityTypeInfo> systemList = entityTypeList.Where(static x => x.Kind == EntityTypeKind.System).ToList();
        List<EntityTypeInfo> logicList = entityTypeList.Where(static x => x.Kind == EntityTypeKind.Logic).ToList();
        List<EntityTypeInfo> serviceList = entityTypeList.Where(static x => x.Kind == EntityTypeKind.Service).ToList();

        //  EntityLogic / Serviceの記述子は型ごとの.g.cppで定義済み。ここでは宣言だけしてアドレスを集める
        //  (メソッド表の特殊化を知らなくてよいので、テーブル側は型の中身に依存しない)
        foreach (EntityTypeInfo entityType in logicList)
        {
            WriteLogicDescriptorDeclaration(codeWriter, entityType);
        }
        foreach (EntityTypeInfo entityType in serviceList)
        {
            WriteServiceDescriptorDeclaration(codeWriter, entityType);
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
            WriteTableArray(codeWriter, "nox::ServiceMethodTypeDescriptor", "k_service_method_type_table", serviceList);
            codeWriter.WriteNewLine();
            WriteTaskTable(codeWriter, taskList);
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
            if (serviceList.Count > 0)
            {
                codeWriter.WriteLine("constinit const nox::ServiceMethodTypeDescriptor* const* const k_service_method_type_table_head = k_service_method_type_table;");
            }
            if (taskList.Count > 0)
            {
                codeWriter.WriteLine("constinit const nox::UpdaterTaskDescriptor* const* const k_updater_task_table_head = k_updater_task_table;");
            }
        }
        codeWriter.WriteLine("}");
        codeWriter.WriteNewLine();

        WriteTableAccessor(codeWriter, "nox::EntitySystemTypeDescriptor", "GetEntitySystemTypes", "k_entity_system_type_table", systemList.Count);
        codeWriter.WriteNewLine();
        WriteTableAccessor(codeWriter, "nox::EntityLogicTypeDescriptor", "GetEntityLogicTypes", "k_entity_logic_type_table", logicList.Count);
        codeWriter.WriteNewLine();
        WriteTableAccessor(codeWriter, "nox::ServiceMethodTypeDescriptor", "GetServiceMethodTypes", "k_service_method_type_table", serviceList.Count);
        codeWriter.WriteNewLine();
        WriteTableAccessor(codeWriter, "nox::UpdaterTaskDescriptor", "GetUpdaterTaskDescriptors", "k_updater_task_table", taskList.Count);

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
                string address = entityType.Kind switch
                {
                    EntityTypeKind.System => $"&nox::k_entity_system_type_descriptor<{entityType.FullName}>",
                    EntityTypeKind.Service => $"&nox::gen::{GetServiceDescriptorName(entityType)}",
                    _ => $"&nox::gen::{GetLogicDescriptorName(entityType)}",
                };
                codeWriter.WriteLine($"{address},	//	{entityType.SourceLocation}");
            }
        }
        codeWriter.WriteLine("};");
    }

    /// <summary>
    /// Task の記述子と表を書き出す (無名名前空間の中に置く)
    /// </summary>
    /// <remarks>
    /// Task は名前空間スコープの関数で、private の問題も型ごとのメソッド表も無いので、
    /// 型ごとのファイルを作らずここで直接記述子を定義する。関数はオーバーロードで曖昧にならないよう
    /// 正確な関数ポインタ型へキャストしてから渡す。フェーズと実行スレッドは属性オブジェクトから C++ 側が読む。
    /// </remarks>
    private static void WriteTaskTable(CodeWriter codeWriter, IReadOnlyList<UpdaterTaskInfo> taskList)
    {
        if (taskList.Count <= 0)
        {
            codeWriter.WriteLine("//\tk_updater_task_table: 購読された関数はありません");
            return;
        }

        for (int i = 0; i < taskList.Count; ++i)
        {
            UpdaterTaskInfo task = taskList[i];
            string attribute = $"k_updater_task_attribute_{i.ToString()}";
            string functionPointer = $"static_cast<nox::ToFunctionPointerType<{task.FunctionTypeFullName}>>(&{task.FullName})";

            codeWriter.WriteLine($"//\t{task.SourceLocation}");
            codeWriter.WriteLine($"constexpr decltype(auto) {attribute} = {task.AttributeExpression};");
            codeWriter.WriteLine($"constexpr nox::UpdaterTaskDescriptor k_updater_task_descriptor_{i.ToString()} = nox::MakeUpdaterTaskDescriptor<{functionPointer}, {attribute}.GetPhase(), {attribute}.GetThreadAffinity()>(\"{task.FullName}\");");
        }
        codeWriter.WriteNewLine();

        codeWriter.WriteLine("constexpr const nox::UpdaterTaskDescriptor* k_updater_task_table[]{");
        using (codeWriter.Indent())
        {
            for (int i = 0; i < taskList.Count; ++i)
            {
                codeWriter.WriteLine($"&k_updater_task_descriptor_{i.ToString()},");
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
