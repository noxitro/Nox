//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	parser.h
///	@brief	parser
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>

namespace clang {
    class CXXRecordDecl;
    class FieldDecl;
}

namespace reflection
{
    // 基本Decl (ClangSharp の CXCursor 相当の最小ラッパ)
    struct DeclBase
    {
        size_t Hash = 0; // 名前などから計算
        virtual ~DeclBase() = default;
    };

    struct NamedDecl : public DeclBase
    {
        std::string Name;           // 単純名
        std::string QualifiedName;  // 修飾名 A::B::C
    };

    struct TypeDecl : public NamedDecl
    {
        std::string UnderlyingTypeName; // typedef 等を扱う際の型名(今後拡張)
    };

    struct FieldInfo
    {
        std::string Name;
        std::string TypeName;      // フィールドの表示型
        size_t      TypeHash = 0;   // 型名ハッシュ(簡易)
    };

    struct RecordDecl : public TypeDecl
    {
        bool IsClass = false;
        bool IsStruct = false;
        bool IsUnion = false;
        std::vector<FieldInfo> Fields;
    };

    // 解析結果全体 (ClangSharp の TranslationUnit 的)
    struct TranslationUnit
    {
        std::vector<std::unique_ptr<RecordDecl>> Records; // 所有
    };

    // libTooling を用いたパーサ (ClangSharp の CXTranslationUnit.TryParse 相当)
    class Parser
    {
    public:
        Parser();
        ~Parser();

        // data からコンパイルオプションを構築し、指定ソースを解析
        // 戻り値: 成功 / 失敗
        bool ParseFile(const std::string& sourceFile,
                       const std::string& cppVersion,
                       const std::string& preprocessorMacro,            // ; 区切り想定 例: FOO;BAR=1
                       const std::string& additionalIncludeDirectories, // ; 区切り
                       const std::string& additionalOptions);           // 空白区切り

        const TranslationUnit& GetTranslationUnit() const { return TU; }

    private:
        TranslationUnit TU;
        // 再利用してファイルシステム初期化コストを削減
        std::unique_ptr<class clang::FileManager> ReusableFileManager;
    };
}