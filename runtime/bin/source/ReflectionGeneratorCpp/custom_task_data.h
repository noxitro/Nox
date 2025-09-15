//	Copyright (C) 2025 NOX ENGINE All rights reserved.

///	@file	custom_task_data.h
///	@brief	custom_task_data
#pragma once
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <filesystem>

namespace reflection
{
#pragma pack(push,1)
    struct NoxCustomTaskData
    {
        static constexpr std::size_t StringSize = 256;

        char CppVersion[StringSize];
        char PreprocessorMacro[StringSize];
        char AdditionalOptions[StringSize];
        char ReflectionTargetSourceFile[StringSize];
        char SolutionPath[StringSize];
        char SolutionDir[StringSize];
        char ProjectPath[StringSize];
        char ProjectDir[StringSize];
        char OutputGenerateDir[StringSize];
        char Configuration[StringSize];
        char ConfigurationDefine[StringSize];
        char Platform[StringSize];
        char PlatformDefine[StringSize];
        char OutDir[StringSize];
        char IntermediateOutputPath[StringSize];
        char MSBuildBinPath[StringSize];
        char Optimization[StringSize];
        char AdditionalIncludeDirectories[StringSize];

        bool UseRtti; // 1 byte
    };
#pragma pack(pop)

    static_assert(sizeof(bool) == 1, "bool が 1 バイトでない環境では互換性が壊れる");
    static_assert(sizeof(NoxCustomTaskData) == (18 * NoxCustomTaskData::StringSize + 1),
        "サイズ不一致: C# 側とレイアウトが異なる可能性");

    inline std::filesystem::path GetNoxReflectionPreDataPath()
    {
        // C# の Path.GetTempPath() + "/NoxReflectionPreData.bin" 相当
        auto temp = std::filesystem::temp_directory_path();
        return std::filesystem::weakly_canonical(temp / "NoxReflectionPreData.bin");
    }

    inline bool LoadNoxCustomTaskData(NoxCustomTaskData& outData)
    {
        const auto path = GetNoxReflectionPreDataPath();
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) return false;

        ifs.read(reinterpret_cast<char*>(&outData), sizeof(outData));
        return static_cast<std::size_t>(ifs.gcount()) == sizeof(outData);
    }
}