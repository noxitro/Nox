//	Copyright (c) 2023-2026 noxitro
//	SPDX-License-Identifier: MIT

///	@file	main_process.cpp
///	@brief	main_process
#include	"stdafx.h"
#include	"main_process.h"
#include    <iostream>
#include    <filesystem>

#include	"custom_task_data.h"
#include    "parser.h"

namespace reflection
{
    static std::string ToString(const char* c) { return c ? std::string(c) : std::string(); }

    int	MainProcess()
    {
        NoxCustomTaskData data{};
        if (!LoadNoxCustomTaskData(data))
        {
            std::cerr << "Load failed: " << GetNoxReflectionPreDataPath() << "\n";
            return 1;
        }

        Parser parser;
        std::string sourceFile = data.ReflectionTargetSourceFile;
        // Windows の場合バックスラッシュが混在する可能性を正規化
        for (char& ch : sourceFile) if (ch == '\\') ch = '/';

        bool ok = parser.ParseFile(
            sourceFile,
            ToString(data.CppVersion),
            ToString(data.PreprocessorMacro),
            ToString(data.AdditionalIncludeDirectories),
            ToString(data.AdditionalOptions));

        if (!ok)
        {
            std::cerr << "Parse failed: " << sourceFile << "\n";
            return 2;
        }

        const TranslationUnit& TU = parser.GetTranslationUnit();
        std::cout << "Records: " << TU.Records.size() << "\n";
        for (auto& recPtr : TU.Records)
        {
            const auto& R = *recPtr;
            std::cout << (R.IsClass ? "class " : R.IsStruct ? "struct " : R.IsUnion ? "union " : "record ")
                << R.QualifiedName << " (fields=" << R.Fields.size() << ")\n";
            for (auto& f : R.Fields)
            {
                std::cout << "  " << f.TypeName << " " << f.Name << "\n";
            }
        }

        return 0;
    }
}