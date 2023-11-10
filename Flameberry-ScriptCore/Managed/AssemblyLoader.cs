using System;
using System.Reflection;

namespace Flameberry
{
    public static class AssemblyLoader
    {
        public static void LoadAssembly(string path)
        {
            Assembly assembly = Assembly.LoadFile(path);

            Console.WriteLine($"Assembly: {assembly.FullName}");

            foreach (Type type in assembly.GetTypes())
            {
                Console.WriteLine($"Type: {type.FullName}");

                foreach (MethodInfo methodInfo in type.GetMethods())
                {
                    Console.WriteLine($"  Method: {methodInfo.Name}");
                }
            }
        }
    }
}