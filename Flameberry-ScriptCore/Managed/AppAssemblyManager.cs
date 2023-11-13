using System;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace Flameberry.Managed
{
    internal static class AppAssemblyManager
    {
        internal static Assembly? s_AppAssembly;

        [UnmanagedCallersOnly]
        internal static void LoadAppAssembly(IntPtr path)
        {
            string? assemblyPath = Marshal.PtrToStringAuto(path);

            Console.WriteLine($"LoadAssembly Called with Argument: {assemblyPath}");

            if (assemblyPath == null)
            {
                Console.WriteLine("ERROR: Assembly Path provided is null!");
                return;
            }

            var context = AssemblyLoadContext.GetLoadContext(Assembly.GetCallingAssembly());
            s_AppAssembly = context!.LoadFromAssemblyPath(assemblyPath);
        }

        [UnmanagedCallersOnly]
        internal static void PrintAssemblyInfo()
        {
            if (s_AppAssembly == null)
            {
                Console.WriteLine("ERROR: Failed to PrintAssemblyInfo: App Assembly is null!");
                return;
            }

            Console.WriteLine($"Assembly: {s_AppAssembly.FullName}");

            foreach (Type type in s_AppAssembly.GetTypes())
            {
                Console.WriteLine($"Type: {type.FullName}");

                foreach (MethodInfo methodInfo in type.GetMethods())
                {
                    Console.WriteLine($"  Method: {methodInfo.Name}");
                }
            }
        }

        internal static Type? FindType(string typeFullName)
        {
            if (s_AppAssembly == null)
            {
                Console.WriteLine("ERROR: Failed to FindType: App Assembly is null!");
                return null;
            }

            Type? result = s_AppAssembly.GetType(typeFullName);
            if (result == null)
                Console.WriteLine($"ERROR: Failed to FindType: {typeFullName}");
            return result;
        }
    }
}