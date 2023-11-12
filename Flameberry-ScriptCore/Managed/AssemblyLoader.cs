using System;
using System.Reflection;
using System.Runtime.InteropServices;

namespace Flameberry.Managed
{
    internal static class AssemblyLoader
    {
        [UnmanagedCallersOnly]
        private static unsafe void LoadAssembly(IntPtr path)
        {
            string? assemblyPath = Marshal.PtrToStringAuto(path);

            Console.WriteLine($"LoadAssembly Called with Argument: {assemblyPath}");

            Assembly assembly = Assembly.LoadFile(assemblyPath);

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