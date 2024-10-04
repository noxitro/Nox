///	@file	main.cpp
///	@brief	main
/// 
#include "stdafx.h"
#pragma comment( lib, "version" )
namespace app
{
    // ***************************************************************************
    //          プリプロセッサからのコールバック処理
    // ***************************************************************************

    class PPCallbacksTracker : public clang::PPCallbacks {
    private:
        clang::Preprocessor& PP;
    public:
        PPCallbacksTracker(clang::Preprocessor& pp) : PP(pp) {}
        void InclusionDirective(
            clang::SourceLocation HashLoc,
            const clang::Token& IncludeTok, clang::StringRef FileName,
            bool IsAngled, clang::CharSourceRange FilenameRange,
            clang::OptionalFileEntryRef File,
            clang::StringRef SearchPath, clang::StringRef RelativePath,
            const clang::Module* Imported,
            clang::SrcMgr::CharacteristicKind FileType) override {
            llvm::errs() << "InclusionDirective : ";
            if (File) {
                if (IsAngled)   llvm::errs() << "<" << File->getName() << ">\n";
                else            llvm::errs() << "\"" << File->getName() << "\"\n";
            }
            else {
                llvm::errs() << "not found file ";
                if (IsAngled)   llvm::errs() << "<" << FileName << ">\n";
                else            llvm::errs() << "\"" << FileName << "\"\n";
            }
        }
    };

    // ***************************************************************************
    //          ASTウォークスルー
    // ***************************************************************************

    //llvm\tools\clang\include\clang\AST\DeclVisitor.hの36行目参照↓voidは指定できない。
    class ExampleVisitor : public clang::DeclVisitor<ExampleVisitor, bool> {
    private:
        clang::PrintingPolicy      Policy;
        const clang::SourceManager& SM;
    public:
        ExampleVisitor(clang::CompilerInstance* CI) : Policy(clang::PrintingPolicy(CI->getASTContext().getPrintingPolicy())),
            SM(CI->getASTContext().getSourceManager()) {
            Policy.Bool = 1;    // print()にてboolが_Boolと表示されないようにする
        }   //↑http://clang.llvm.org/doxygen/structclang_1_1PrintingPolicy.html#a4a4cff4f89cc3ec50381d9d44bedfdab
    private:
        // インデント制御
        struct CIndentation {
            int             IndentLevel;
            CIndentation() : IndentLevel(0) { }
            void Indentation(clang::raw_ostream& OS) const {
                for (int i = 0; i < IndentLevel; ++i) OS << "  ";
            }
            // raw_ostream<<演算子定義
            friend clang::raw_ostream& operator<<(clang::raw_ostream& OS, const CIndentation& aCIndentation) {
                aCIndentation.Indentation(OS);
                return OS;
            }
        } indentation;
        class CIndentationHelper {
        private:
            ExampleVisitor* parent;
        public:
            CIndentationHelper(ExampleVisitor* aExampleVisitor) : parent(aExampleVisitor) {
                parent->indentation.IndentLevel++;
            }
            ~CIndentationHelper() { parent->indentation.IndentLevel--; }
        };
#define INDENTATION CIndentationHelper CIndentationHelper(this)
    public:
        // DeclContextメンバーの1レベルの枚挙処理
        void EnumerateDecl(clang::DeclContext* aDeclContext) {
            for (clang::DeclContext::decl_iterator i = aDeclContext->decls_begin(), e = aDeclContext->decls_end(); i != e; i++) {
                clang::Decl* D = *i;
                if (indentation.IndentLevel == 0) {
                    llvm::errs() << "TopLevel : " << D->getDeclKindName();                                    // Declの型表示
                    if (clang::NamedDecl* N = clang::dyn_cast<clang::NamedDecl>(D)) llvm::errs() << " " << N->getNameAsString();  // NamedDeclなら名前表示
                    llvm::errs() << " (" << D->getLocation().printToString(SM) << ")\n";                      // ソース上の場所表示
                }
                Visit(D);       // llvm\tools\clang\include\clang\AST\DeclVisitor.hの38行目
            }
        }

        // class/struct/unionの処理
        virtual bool VisitCXXRecordDecl(clang::CXXRecordDecl* aCXXRecordDecl, bool aForce = false) {
            // 参照用(class foo;のような宣言)なら追跡しない
            if (!aCXXRecordDecl->isCompleteDefinition()) {
                return true;
            }

            // 名前無しなら表示しない(ただし、強制表示されたら表示する:Elaborated用)
            if (!aCXXRecordDecl->getIdentifier() && !aForce) {
                return true;
            }

            llvm::errs() << indentation << "<<<====================================\n";

            // TopLevelなら参考のためlibToolingでも表示する
            if (indentation.IndentLevel == 0) {
                aCXXRecordDecl->print(llvm::errs(), Policy);
                llvm::errs() << "\n";
                llvm::errs() << indentation << "--------------------\n";
            }

            // クラス定義の処理
            llvm::errs() << indentation << "CXXRecordDecl : " << aCXXRecordDecl->getNameAsString() << " {\n";
            {
                INDENTATION;

                // 基底クラスの枚挙処理
                for (clang::CXXRecordDecl::base_class_iterator base = aCXXRecordDecl->bases_begin(), BaseEnd = aCXXRecordDecl->bases_end();
                    base != BaseEnd;
                    ++base) {                                          // ↓型名を取り出す(例えば、Policy.Bool=0の時、bool型は"_Bool"となる)
                    llvm::errs() << indentation << "Base : " << base->getType().getAsString(Policy) << "\n";
                }

                // メンバーの枚挙処理
                EnumerateDecl(aCXXRecordDecl);
            }
            llvm::errs() << indentation << "}\n";
            llvm::errs() << indentation << "====================================>>>\n";
            return true;
        }

        // メンバー変数の処理
        virtual bool VisitFieldDecl(clang::FieldDecl* aFieldDecl) {
            // 名前無しclass/struct/unionでメンバー変数が直接定義されている時の対応
            clang::CXXRecordDecl* R = NULL;      // 名前無しの時、内容を表示するためにCXXRecordDeclをポイントする
            const clang::Type* T = aFieldDecl->getType().split().Ty;
            if (T->getTypeClass() == clang::Type::Elaborated) {
                R = clang::cast<clang::ElaboratedType>(T)->getNamedType()->getAsCXXRecordDecl();
                if (R && (R->getIdentifier()))  R = NULL;
            }
            // 内容表示
            if (R) {
                llvm::errs() << indentation << "FieldDecl : <no-name-type> " << aFieldDecl->getNameAsString() << "\n";
                VisitCXXRecordDecl(R, true);    // 名前無しclass/struct/unionの内容表示
            }
            else {
                llvm::errs() << indentation << "FieldDecl : " << aFieldDecl->getType().getAsString(Policy) << " " << aFieldDecl->getNameAsString() << "\n";
            }
            return true;
        }

        // namespaceの処理(配下を追跡する)
        virtual bool VisitNamespaceDecl(clang::NamespaceDecl* aNamespaceDecl) {
            llvm::errs() << "NamespaceDecl : namespace " << aNamespaceDecl->getNameAsString() << " {\n";
            EnumerateDecl(aNamespaceDecl);
            llvm::errs() << "} end of namespace " << aNamespaceDecl->getNameAsString() << "\n";
            return true;
        }

        // extern "C"/"C++"の処理(配下を追跡する)
        virtual bool VisitLinkageSpecDecl(clang::LinkageSpecDecl* aLinkageSpecDecl) {
            std::string lang;
            switch (aLinkageSpecDecl->getLanguage()) {
            case clang::LinkageSpecDecl::lang_c:   lang = "C"; break;
            case clang::LinkageSpecDecl::lang_cxx: lang = "C++"; break;
            }
            llvm::errs() << "LinkageSpecDecl : extern \"" << lang << "\" {\n";
            EnumerateDecl(aLinkageSpecDecl);
            llvm::errs() << "} end of extern \"" << lang << "\"\n";
            return true;
        }
    };

    // ***************************************************************************
    //          定番の処理
    // ***************************************************************************

    class ExampleASTConsumer : public clang::ASTConsumer {
    private:
        ExampleVisitor* visitor; // doesn't have to be private
    public:
        explicit ExampleASTConsumer(clang::CompilerInstance* CI) : visitor(new ExampleVisitor(CI)) {
            // プリプロセッサからのコールバック登録
            clang::Preprocessor& PP = CI->getPreprocessor();
            PP.addPPCallbacks(std::make_unique<app::PPCallbacksTracker>(PP));
            
        }
        // AST解析結果の受取
        void HandleTranslationUnit(clang::ASTContext& Context)override {
            llvm::errs() << "\n\nHandleTranslationUnit()\n";
            visitor->EnumerateDecl(Context.getTranslationUnitDecl());
        }
    };

    class ExampleFrontendAction : public clang::SyntaxOnlyAction /*ASTFrontendAction*/ {
    public:
        std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance& CI, clang::StringRef file)override {
            return std::make_unique<ExampleASTConsumer>(&CI); // pass CI pointer to ASTConsumer
        }
    };

    static llvm::cl::OptionCategory MyToolCategory("My tool options");

    inline void MainProcess(int argc, const char** argv)
    {
        auto op = clang::tooling::CommonOptionsParser::create(argc, argv, app::MyToolCategory);
        clang::tooling::ClangTool Tool(op->getCompilations(), op->getSourcePathList());
        Tool.run(clang::tooling::newFrontendActionFactory<ExampleFrontendAction>().get());
    }
}
int main()
{
    const char* argv[] = {"D:\github\Nox\runtime\bin\source\ReflectionGeneratorCpp\Sample\main.cpp" };
    app::MainProcess(1, argv);
	return 0;
}