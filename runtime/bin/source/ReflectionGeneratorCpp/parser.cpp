//	Copyright (c) 2025 NOX ENGINE All rights reserved.

///	@file	parser.cpp
///	@brief	parser
#include "stdafx.h"
#include "parser.h"

#include <functional>
#include <unordered_map>
#include <sstream>
#include <iostream>
#include <fstream>

#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Type.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/Version.h"

namespace reflection
{
    namespace
    {
        static size_t HashString(std::string_view s)
        {
            return std::hash<std::string_view>{}(s);
        }

        // ------------------------------------------------------------------
        // Visitor : ClangSharp の cursor walk 対応
        // ------------------------------------------------------------------
        class RecordCollectVisitor : public clang::RecursiveASTVisitor<RecordCollectVisitor>
        {
        public:
            RecordCollectVisitor(clang::ASTContext& ctx, TranslationUnit& out)
                : Ctx(ctx), TU(out) {}

            bool VisitCXXRecordDecl(clang::CXXRecordDecl* D)
            {
                if (!D->isCompleteDefinition()) return true;
                if (D->isImplicit()) return true; // 暗黙生成は除外

                std::string name = D->getIdentifier() ? D->getName().str() : std::string();
                if (name.empty()) return true; // 無名はスキップ(必要なら今後対応)

                auto rec = std::make_unique<RecordDecl>();
                rec->Name = name;
                rec->QualifiedName = D->getQualifiedNameAsString();
                rec->Hash = HashString(rec->QualifiedName);
                rec->IsClass = D->isClass();
                rec->IsStruct = D->isStruct();
                rec->IsUnion = D->isUnion();

                for (auto field : D->fields())
                {
                    FieldInfo fi;
                    fi.Name = field->getNameAsString();
                    fi.TypeName = field->getType().getAsString(Ctx.getPrintingPolicy());
                    fi.TypeHash = HashString(fi.TypeName);
                    rec->Fields.emplace_back(std::move(fi));
                }

                TU.Records.emplace_back(std::move(rec));
                return true;
            }
        private:
            clang::ASTContext& Ctx;
            TranslationUnit& TU;
        };

        class CollectASTConsumer : public clang::ASTConsumer
        {
        public:
            CollectASTConsumer(TranslationUnit& tu) : TU(tu) {}
            void HandleTranslationUnit(clang::ASTContext& Context) override
            {
                RecordCollectVisitor v(Context, TU);
                v.TraverseDecl(Context.getTranslationUnitDecl());
            }
        private:
            TranslationUnit& TU;
        };

        class CollectFrontendAction : public clang::ASTFrontendAction
        {
        public:
            CollectFrontendAction(TranslationUnit& tu) : TU(tu) {}
        protected:
            std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI, llvm::StringRef InFile) override
            {
                return std::make_unique<CollectASTConsumer>(TU);
            }
        private:
            TranslationUnit& TU;
        };

        static void SplitSemicolonList(const std::string& s, std::vector<std::string>& out)
        {
            std::stringstream ss(s);
            std::string item;
            while (std::getline(ss, item, ';'))
            {
                if (item.empty()) continue;
                // trim spaces
                size_t b = item.find_first_not_of(" \t\r\n");
                size_t e = item.find_last_not_of(" \t\r\n");
                if (b == std::string::npos) continue;
                out.emplace_back(item.substr(b, e - b + 1));
            }
        }

        static void SplitWhitespace(const std::string& s, std::vector<std::string>& out)
        {
            std::stringstream ss(s);
            std::string item;
            while (ss >> item) out.emplace_back(item);
        }
    }

    Parser::Parser() = default;
    Parser::~Parser() = default;

    bool Parser::ParseFile(const std::string& sourceFile,
                           const std::string& cppVersion,
                           const std::string& preprocessorMacro,
                           const std::string& additionalIncludeDirectories,
                           const std::string& additionalOptions)
    {
        TU = TranslationUnit{}; // reset

        // Clang コマンドライン引数構築 (ClangSharp の TryParse 引数組み立て相当)
        std::vector<std::string> args;
        args.push_back("tool"); // argv[0]

        // C++ version
        if (!cppVersion.empty())
        {
            args.push_back("-std=" + cppVersion); // 例: c++20
        }
        else
        {
            args.push_back("-std=c++20");
        }

        // マクロ -D
        {
            std::vector<std::string> macros;
            SplitSemicolonList(preprocessorMacro, macros);
            for (auto& m : macros)
            {
                if (!m.empty()) args.push_back("-D" + m);
            }
        }

        // インクルード -I
        {
            std::vector<std::string> includes;
            SplitSemicolonList(additionalIncludeDirectories, includes);
            for (auto& inc : includes)
            {
                args.push_back("-I" + inc);
            }
        }

        // その他付加オプション (空白区切り)
        {
            std::vector<std::string> opts;
            SplitWhitespace(additionalOptions, opts);
            for (auto& o : opts) args.push_back(o);
        }

        // 解析対象ファイル
        args.push_back(sourceFile);

        // 実行 (1TU なので ToolInvocation を直接使用)
        auto action = std::make_unique<CollectFrontendAction>(TU);
        // ここでは runToolOnCodeWithArgs ではなく buildASTFromCodeWithArgs が
        // ファイルパス解析でなくコード文字列解析用なので ToolInvocation/ClangTool を使う
        // 簡潔さのため runToolOnCodeWithArgs を利用するにはファイル内容を読み込む必要がある

        // ファイル存在確認
        if (!std::ifstream(sourceFile).good())
        {
            std::cerr << "ParseFile: source not found: " << sourceFile << "\n";
            return false;
        }

        // ClangTool 用にダミー CompilationDatabase を構築するのは冗長なので
        // ToolInvocation を直接使う (単一コマンドライン)
        std::vector<std::string> command = args; // コピー

        // set up FileManager (再利用)
        if (!ReusableFileManager)
        {
            clang::FileSystemOptions fileSystemOptions;
            ReusableFileManager = std::make_unique<clang::FileManager>(fileSystemOptions);
        }

        clang::tooling::ToolInvocation invocation(command, std::move(action), ReusableFileManager.get());
        // Diagnostics を抑制 (速度 + コンソールノイズ削減)
        struct NullDiagConsumer : clang::DiagnosticConsumer { void HandleDiagnostic(clang::DiagnosticsEngine::Level, const clang::Diagnostic&) override {} }; 
        NullDiagConsumer nullConsumer;
        invocation.setDiagnosticConsumer(&nullConsumer);

        bool ok = invocation.run();
        return ok;
    }
}