using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ReflectionGenerator.Generator;

namespace ReflectionGenerator.Generator
{
    /// <summary>
    /// CodeWriterの拡張
    /// </summary>
    public static class CodeWriterExtension_Generator
    {
        //  タイトル
        public static void WriteLineSource(this CodeWriter codeWriter)
        {
            codeWriter.WriteLine("//\tdo not edit");
            codeWriter.WriteLine("//\twritten from TypeGen");
        }

        /// <summary>
        /// ヘッダ用のタイトル
        /// </summary>
        public static void WriteLineHeader(this CodeWriter codeWriter)
        {
            codeWriter.WriteLine("#pragma once");
            codeWriter.WriteLine("//\tdo not edit");
            codeWriter.WriteLine("//\twritten from TypeGen");
        }

        public static void WriteLinePPIf(this CodeWriter codeWriter, string s)
        {
            codeWriter.WriteLine($"#if\t{s}");
        }

        public static void WriteLinePPEndIf(this CodeWriter codeWriter, string s)
        {
            codeWriter.WriteLine($"#endif\t//\t{s}");
        }

        public static void WriteLinePragmaOnce(this CodeWriter codeWriter)
        {
            codeWriter.WriteLine("#pragma once");
        }

        public static void WriteLineCopyRight(this CodeWriter codeWriter)
        {
            codeWriter.WriteLine("//\tCopyright (C) 2024 NOX ENGINE");
        }

        public static void WriteIncludeStdafx(this CodeWriter codeWriter)
        {
            codeWriter.WriteLine("#include\t\"stdafx.h\"");
        }

        public static void WriteLineInclude(this CodeWriter codeWriter, string includePath)
        {
            codeWriter.WriteLine($"#include\t\"{includePath}\"");
        }

        public static void WriteNamespace(this CodeWriter codeWriter, string namespaceName)
        {
            codeWriter.WriteLine("namespace {0}", namespaceName);
        }

        public static void WriteNamespaceNitroReflectionGen(this CodeWriter codeWriter) => codeWriter.WriteNamespace("nitro::reflection::gen");
        public static void WriteNamespaceNitroTypeinfo(this CodeWriter codeWriter) => codeWriter.WriteNamespace("nitro::typeinfo");
        public static void WriteNamespaceNitroTypeinfoGen(this CodeWriter codeWriter) => codeWriter.WriteNamespace("nitro::typeinfo::gen");

        /// <summary>
        /// TypeInfoRefHolderを記述
        /// </summary>
        /// <param name="str"></param>
        public static void WriteLineTypeInfoRefHolder(this CodeWriter codeWriter, string str)
        {
            codeWriter.WriteLine("template<>");
            codeWriter.WriteLine("struct TypeInfoRefHolder<{0}> : TypeInfoRefHolderBase", str);
        }

        public static void WriteLineTypeInfoRefTypedefHolder(this CodeWriter codeWriter, string name, int typedefId, string recordName)
        {
            codeWriter.WriteLine("template<>");
            codeWriter.WriteLine("struct TypeInfoRefTypedefHolder<{0}, {1}> : TypeInfoRefHolderBase", name, typedefId.ToString(), recordName);
        }

        /// <summary>
        /// 属性リストの記述
        /// </summary>
        /// <param name="codeWriter"></param>
        /// <param name="name"></param>
        /// <param name="attrList"></param>
        public static void WriteLineAttributes(this CodeWriter codeWriter, string name, IReadOnlyList<Info.AttributeInfo> attrList)
        {
            if (attrList.Count <= 0)
            {
                return;
            }

            //  
            for (int i = 0; i < attrList.Count; i++)
            {
                Info.AttributeInfo attrInfo = attrList[i];

                //if (attrInfo.IsConstexpr)
                //{
                //    codeWriter.WriteLine($"static constexpr {attrInfo.ValueStr} {GetAttributeName(i)} = {attrInfo.ValueStr}({attrInfo.ValueStr})");
                //}
                //else
                //{
                //    codeWriter.WriteLine($"static inline const {attrInfo.ValueStr} {GetAttributeName(i)} = {attrInfo.ValueStr}({attrInfo.ValueStr})");
                //}
            }

            //  属性テーブル
            codeWriter.WriteLine($"static constexpr array<attr::Attribute, {attrList.Count.ToString()}> {GeneratorUtil.GetAttributeTableName(name)}= {{");
            codeWriter.Push();

            for (int i = 0; i < attrList.Count; i++)
            {
                codeWriter.WriteLine($"&{GetAttributeName(i)}");
            }

            codeWriter.Pop();
            codeWriter.WriteLine("}");

            string GetAttributeName(int index) => $"{name}_Attribute_{index.ToString()}";
        }

        /// <summary>
        /// クラスフィールドを記述
        /// </summary>
        /// <param name="codeWriter"></param>
        /// <param name="fieldName"></param>
        /// <param name="ownerClassInfo"></param>
        /// <param name="fieldInfo"></param>
        public static void WriteLineTypeofFiled(this CodeWriter codeWriter, int fieldIndex, Info.ClassInfoOld? ownerClassInfo, Info.FieldInfo fieldInfo)
        {
            string fieldName = GeneratorUtil.GetFieldName(fieldIndex);

            //  comment
            codeWriter.WriteLine($"//\t{fieldName}");
            codeWriter.WriteNewLine();

            //  attibute
            codeWriter.WriteLineAttributes(fieldName, fieldInfo.AttributeInfoList);

            //  メンバ変数
            if (ownerClassInfo != null)
            {
                codeWriter.WriteLine($"static constexpr auto {fieldName} = TypeofField<{ownerClassInfo.FullName}, decltype({fieldInfo.FullName})>(");
            }
            //  グローバル変数
            else
            {
                codeWriter.WriteLine($"static constexpr auto {fieldName} = TypeofField<decltype({fieldInfo.FullName})>(");
            }
            codeWriter.Push();

            //  name
            codeWriter.WriteLine($"\"{fieldInfo.Name}\",");

            //  access level
            codeWriter.WriteLine($"{fieldInfo.AccessLevel.ToCppString()},");

            //  attibute
            if (fieldInfo.AttributeInfoList.Count > 0)
            {
                codeWriter.WriteLine($"{GeneratorUtil.GetAttributeTableName(fieldName)},");
            }
            else
            {
                codeWriter.WriteLine("nullptr,");
            }
            codeWriter.WriteLine($"{fieldInfo.AttributeInfoList.Count.ToString()},");

            //  メンバ変数
            if (ownerClassInfo != null)
            {
                //  setter
                codeWriter.WriteLine($"NITRO_FIELD_INFO_CREATE_LAMBDA_SETTER({ownerClassInfo.FullName}, {fieldInfo.Name}),");

                //  getter
                codeWriter.WriteLine($"NITRO_FIELD_INFO_CREATE_LAMBDA_GETTER({ownerClassInfo.FullName}, {fieldInfo.Name}),");

                //  setterArray
                codeWriter.WriteLine($"NITRO_FIELD_INFO_CREATE_LAMBDA_SETTER_ARRAY({ownerClassInfo.FullName}, {fieldInfo.Name}),");

                //  getterArray
                codeWriter.WriteLine($"NITRO_FIELD_INFO_CREATE_LAMBDA_GETTER_ARRAY({ownerClassInfo.FullName}, {fieldInfo.Name})");
            }
            //  グローバル変数
            else
            {
                //  setter
                codeWriter.WriteLine($"NITRO_FIELD_INFO_CREATE_LAMBDA_SETTER_GLOBAL({fieldInfo.Name}),");

                //  getter
                codeWriter.WriteLine($"NITRO_FIELD_INFO_CREATE_LAMBDA_GETTER_GLOBAL({fieldInfo.Name}),");

                //  setterArray
                codeWriter.WriteLine($"NITRO_FIELD_INFO_CREATE_LAMBDA_SETTER_ARRAY_GLOBAL({fieldInfo.Name}),");

                //  getterArray
                codeWriter.WriteLine($"NITRO_FIELD_INFO_CREATE_LAMBDA_GETTER_ARRAY_GLOBAL({fieldInfo.Name})");
            }

            codeWriter.Pop();
            codeWriter.WriteLine(");");
        }

        public static void WriteLineTypeofFieldTable(this CodeWriter codeWriter, int fieldCount)
        {
            if (fieldCount <= 0)
            {
                return;
            }

            codeWriter.WriteLine($"static constexpr array<const FieldInfo* const, {fieldCount.ToString()}> {GeneratorUtil.GetFieldTableName()}=");
            codeWriter.PushScope(CodeWriter.ScopeType.Decl);

            for (int i = 0; i < fieldCount; i++)
            {
                if (i < fieldCount - 1)
                {
                    codeWriter.WriteLine($"&{GeneratorUtil.GetFieldName(i)},");
                }
                else
                {
                    codeWriter.WriteLine($"&{GeneratorUtil.GetFieldName(i)}");
                }
            }

            codeWriter.PopScope();
        }

        //private static void WriteLineMethodInvokeLambda(this CodeWriter codeWriter, Info.ClassInfoOld? ownerClassInfo, Info.MethodInfo methodInfo, int minArgsLength)
        //{
        //    codeWriter.WriteNest();


        //    if (ownerClassInfo != null)
        //    {
        //        if (methodInfo.IsConstructor == true || methodInfo.IsStatic == true)
        //        {
        //            codeWriter.Write($"[](");
        //        }
        //        else
        //        {
        //            if (methodInfo.IsConst == true)
        //            {
        //                codeWriter.Write($"[](const {ownerClassInfo.FullName}& instanceRef");
        //            }
        //            else
        //            {
        //                codeWriter.Write($"[]({ownerClassInfo.FullName}& instanceRef");
        //            }
        //        }
        //    }
        //    else
        //    {
        //        codeWriter.Write($"[](");
        //    }


        //    if (minArgsLength > 0)
        //    {
        //        if (methodInfo.IsConstructor == false && methodInfo.IsStatic == false)
        //        {
        //            codeWriter.Write(", ");
        //        }

        //        for (int i = 0; i < minArgsLength; ++i)
        //        {
        //            codeWriter.Write($"{methodInfo.ArgInfoList[i].RuntimeType.FullName} v{i.ToString()}");
        //            if (i < methodInfo.ArgInfoList.Count - 1)
        //            {
        //                codeWriter.Write(", ");
        //            }
        //        }
        //    }

        //    codeWriter.Write(")");

        //    //  戻り値のヒント
        //    codeWriter.Write("->decltype(auto)");

        //    //  中身

        //    codeWriter.Write("\t{");

        //    //  呼び出し

        //    //  戻り値があるならreturnする
        //    if (methodInfo.ReturnRuntimeType.IsVoid == false)
        //    {
        //        codeWriter.Write("return ");
        //    }

        //    if (methodInfo.IsConstructor == true)
        //    {
        //        codeWriter.Write($"return new {methodInfo.FullName}(");
        //    }
        //    else if (methodInfo.IsStatic == false)
        //    {
        //        codeWriter.Write($"instanceRef.{methodInfo.Name}(");
        //    }
        //    else
        //    {
        //        codeWriter.Write($"{methodInfo.FullName}(");
        //    }

        //    for (int i = 0; i < minArgsLength; ++i)
        //    {
        //        codeWriter.Write($"static_cast<{methodInfo.ArgInfoList[i].RuntimeType.FullName}>(v{i.ToString()})");
        //        if (i < methodInfo.ArgInfoList.Count - 1)
        //        {
        //            codeWriter.Write(", ");
        //        }
        //    }
        //    codeWriter.Write(");\t");

        //    codeWriter.Write("}");

        //    codeWriter.WriteNewLine();
        //}

        ///// <summary>
        ///// 関数フィールドを記述
        ///// </summary>
        ///// <param name="codeWriter"></param>
        ///// <param name="fieldName"></param>
        ///// <param name="ownerClassInfo"></param>
        ///// <param name="fieldInfo"></param>
        //public static void WriteLineTypeofMethod(this CodeWriter codeWriter, int methodIndex, Info.ClassInfoOld? ownerClassInfo, Info.MethodInfo methodInfo)
        //{
        //    string methodName = GeneratorUtil.GetMethodName(methodIndex);

        //    //  comment
        //    codeWriter.WriteLine($"//\t{methodName}");
        //    codeWriter.WriteNewLine();

        //    //  attibute
        //    codeWriter.WriteLineAttributes(methodName, methodInfo.AttributeInfoList);

        //    //  methodParamaterTable
        //    if (methodInfo.ArgInfoList.Count > 0)
        //    {
        //        codeWriter.WriteLine($"static constexpr array<const reflection::MethodParameter, {methodInfo.ArgInfoList.Count.ToString()}> {GeneratorUtil.GetMethodParamTableName(methodName)} = ");
        //        codeWriter.PushScope(CodeWriter.ScopeType.Decl);

        //        //                foreach (Info.MethodInfo.ArgInfo argInfo in methodInfo.ArgInfoList)
        //        for (int i = 0; i < methodInfo.ArgInfoList.Count; ++i)
        //        {
        //            string writeStr = $"reflection::MethodParameter(\"{methodInfo.ArgInfoList[i].Name}\", {GeneratorUtil.TypeofSyntax(methodInfo.ArgInfoList[i].RuntimeType.FullName)}, {methodInfo.ArgInfoList[i].HasDefaultValue.ToLowerString()})";

        //            if (i < methodInfo.ArgInfoList.Count - 1)
        //            {
        //                codeWriter.WriteLine(writeStr + ",");
        //            }
        //            else
        //            {
        //                codeWriter.WriteLine(writeStr);
        //            }
        //        }

        //        codeWriter.PopScope();
        //    }


        //    //  typeofmethod
        //    codeWriter.WriteLine($"static constexpr auto {methodName} = {Define.CREATE_METHOD_INFO_FUNCTION_NAME}<{methodInfo.ReturnRuntimeType.FullName}>(");
        //    codeWriter.Push();

        //    //  name
        //    codeWriter.WriteLine($"\"{methodInfo.Name}\",");

        //    //  access level
        //    codeWriter.WriteLine($"{methodInfo.AccessLevel.ToCppString()},");

        //    //  methodParamaterTable

        //    if (methodInfo.ArgInfoList.Count > 0)
        //    {
        //        codeWriter.WriteLine($"{GeneratorUtil.GetMethodParamTableName(methodName)}.data(),");
        //    }
        //    else
        //    {
        //        codeWriter.WriteLine("nullptr,");
        //    }

        //    codeWriter.WriteLine($"{methodInfo.ArgInfoList.Count.ToString()},");

        //    //  attibute
        //    if (methodInfo.AttributeInfoList.Count > 0)
        //    {
        //        codeWriter.WriteLine($"{GeneratorUtil.GetAttributeTableName(methodName)},");
        //    }
        //    else
        //    {
        //        codeWriter.WriteLine("nullptr,");
        //    }
        //    codeWriter.WriteLine($"{methodInfo.AttributeInfoList.Count.ToString()},");


        //    //  コンストラクタか
        //    codeWriter.WriteLine($"{methodInfo.IsConstructor.ToLowerString()},");

        //    //  デフォルト引数付きの呼び出しを解決
        //    int minArgsLength = methodInfo.ArgInfoList.Count;
        //    for (int i = 0; i < methodInfo.ArgInfoList.Count; ++i)
        //    {
        //        if (methodInfo.ArgInfoList[i].HasDefaultValue == true)
        //        {
        //            minArgsLength = i;
        //            break;
        //        }
        //    }

        //    //  ラムダ

        //    for (int i = minArgsLength; i <= methodInfo.ArgInfoList.Count; ++i)
        //    {
        //        codeWriter.WriteLineMethodInvokeLambda(ownerClassInfo, methodInfo, i);
        //    }

        //    codeWriter.Pop();
        //    codeWriter.WriteLine(");");
        //}

        //public static void WriteLineTypeofMethodTable(this CodeWriter codeWriter, int methodCount)
        //{
        //    if (methodCount <= 0)
        //    {
        //        return;
        //    }

        //    codeWriter.WriteLine($"static constexpr std::array<const MethodInfo* const, {methodCount.ToString()}> {GeneratorUtil.GetMethodTableName()}=");
        //    codeWriter.PushScope(CodeWriter.ScopeType.Decl);

        //    for (int i = 0; i < methodCount; i++)
        //    {
        //        if (i < methodCount - 1)
        //        {
        //            codeWriter.WriteLine($"&{GeneratorUtil.GetMethodName(i)},");
        //        }
        //        else
        //        {
        //            codeWriter.WriteLine($"&{GeneratorUtil.GetMethodName(i)}");
        //        }
        //    }

        //    codeWriter.PopScope();
        //}

        //public static void WriteLineTypeofClass(this CodeWriter codeWriter, Info.ClassInfoOld classInfo)
        //{

        //}
    }
}
