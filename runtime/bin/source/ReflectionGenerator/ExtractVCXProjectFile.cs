using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ReflectionGenerator
{
    public static class ExtractVCXProjectFile
    {
        public static List<string> ExtractBuildOrderProjectPathList(string solutionFilePath)
        {
            List<string> projectPaths = ExtractProjectPaths(solutionFilePath);
            return DetermineBuildOrder(projectPaths);
        }



        public static List<string> ExtractHeaderFiles(string projectFilePath)
        {
            List<string> headerFiles = new List<string>();
            string? projectDirectory = Path.GetDirectoryName(projectFilePath);
            if (projectDirectory == null)
            {
                return [];
            }

            System.Xml.XmlDocument doc = new System.Xml.XmlDocument();
            doc.Load(projectFilePath);

            System.Xml.XmlNamespaceManager nsmgr = new System.Xml.XmlNamespaceManager(doc.NameTable);
            nsmgr.AddNamespace("ns", "http://schemas.microsoft.com/developer/msbuild/2003");

            System.Xml.XmlNodeList? nodes = doc.SelectNodes("//ns:ClInclude", nsmgr);
            if (nodes == null)
            {
                return [];
            }

            foreach (System.Xml.XmlNode node in nodes)
            {
                string? relativePath = node.Attributes?["Include"]?.Value;
                if (relativePath == null)
                {
                    continue;
                }
                string fullPath = Path.Combine(projectDirectory, relativePath);
                headerFiles.Add(fullPath);
            }

            return headerFiles;
        }

        private static List<string> ExtractProjectPaths(string solutionFilePath)
        {
            List<string> projectPaths = new List<string>();
            string? solutionDirectory = Path.GetDirectoryName(solutionFilePath);
            if (solutionDirectory == null)
            {
                return [];
            }

            string[] lines = File.ReadAllLines(solutionFilePath);
            System.Text.RegularExpressions.Regex projectRegex =
                new System.Text.RegularExpressions.Regex(@"Project\(""\{.*\}""\) = "".*"", ""(.*\.vcxproj)""");

            foreach (string line in lines)
            {
                System.Text.RegularExpressions.Match match = projectRegex.Match(line);
                if (match.Success)
                {
                    string relativePath = match.Groups[1].Value;
                    string fullPath = Path.Combine(solutionDirectory, relativePath);
                    projectPaths.Add(fullPath);
                }
            }

            return projectPaths;
        }

        private static List<string> DetermineBuildOrder(List<string> projectPaths)
        {
            Dictionary<string, List<string>> dependencies = new Dictionary<string, List<string>>();

            foreach (string projectPath in projectPaths)
            {
                List<string> projectDependencies = GetProjectDependencies(projectPath);
                dependencies[projectPath] = projectDependencies;
            }

            List<string> buildOrder = TopologicalSort(dependencies);
            return buildOrder;
        }

        private static List<string> GetProjectDependencies(string projectPath)
        {
            List<string> dependencies = new List<string>();
            System.Xml.Linq.XDocument doc = System.Xml.Linq.XDocument.Load(projectPath);
            System.Xml.Linq.XNamespace ns = "http://schemas.microsoft.com/developer/msbuild/2003";

            foreach (System.Xml.Linq.XElement projectReference in doc.Descendants(ns + "ProjectReference"))
            {
                string? include = projectReference.Attribute("Include")?.Value;
                if (include == null)
                {
                    continue;
                }
                string dependencyPath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(projectPath) ?? string.Empty, include));
                dependencies.Add(dependencyPath);
            }

            return dependencies;
        }

        private static List<string> TopologicalSort(Dictionary<string, List<string>> dependencies)
        {
            List<string> sorted = new List<string>();
            HashSet<string> visited = new HashSet<string>();

            foreach (string project in dependencies.Keys)
            {
                Visit(project, dependencies, visited, sorted);
            }

            return sorted;
        }

        private static void Visit(string project, Dictionary<string, List<string>> dependencies, HashSet<string> visited, List<string> sorted)
        {
            if (!visited.Contains(project))
            {
                visited.Add(project);

                foreach (string dependency in dependencies[project])
                {
                    Visit(dependency, dependencies, visited, sorted);
                }

                sorted.Add(project);
            }
        }
    }
}
