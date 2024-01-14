using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator.Generator
{
    public static class GeneratorUtil
    {
        #region 公開メソッド
        public static string GetFieldName(int index) => $"field_{index}_";

        public static string GetFieldTableName() => "field_table_";

        public static string GetMethodName(int index) => $"method_{index}_";
        public static string GetMethodTableName() => "method_table_";

        public static string GetMethodParamTableName(string methodName) => $"{methodName}_ParamTable";

        public static string GetTypeofClassName() => "mTypeinfoClass";

        public static string GetAttributeTableName(string name) => $"{name}_attributes_";

        public static string TypeofSyntax(string typeName) => $"{Util.GetRuntimeReflectionNamespaceScope()}::Typeof<{typeName}>()";

        #endregion
    }
}
