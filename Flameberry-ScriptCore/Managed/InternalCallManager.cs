using System;
using System.Numerics;
using System.Reflection;
using System.Runtime.InteropServices;

namespace Flameberry.Managed
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public readonly struct InternalCall
    {
        private readonly IntPtr m_NamePtr;
        public readonly IntPtr NativeFunctionPtr;

        public string? Name => Marshal.PtrToStringAuto(m_NamePtr);
    }

    internal static class InternalCallStorage
	{
        internal static unsafe delegate*<IntPtr, byte, void> LogMessageICall;

        internal static unsafe delegate*<ulong, Vector3> Entity_GetTransformICall;
        //internal static unsafe delegate*<ulong, IntPtr, bool> Entity_HasComponent;
    }

    public static class Log
    {
        public enum LogLevel : byte { TRACE = 0, LOG, INFO, WARNING, ERROR, CRITICAL };

        public static void LogMessage(string message, LogLevel logLevel)
        {
            IntPtr stringPtr = Marshal.StringToCoTaskMemAuto(message);
            unsafe { InternalCallStorage.LogMessageICall(stringPtr, (byte)logLevel); };
            Marshal.FreeCoTaskMem(stringPtr);
        }
    }

	internal static class InternalCallManager
	{
        internal static List<InternalCall> s_InternalCallList = new();

		[UnmanagedCallersOnly]
		internal static void SetInternalCalls(IntPtr callArrayPtr, int size)
		{
            // Marshal each struct from the IntPtr
            for (int i = 0; i < size; i++)
            {
                // Calculate the offset for the i-th element in the array
                IntPtr elementIntPtr = IntPtr.Add(callArrayPtr, i * Marshal.SizeOf<InternalCall>());

                // Marshal the struct at the offset
                InternalCall icall = Marshal.PtrToStructure<InternalCall>(elementIntPtr);
                s_InternalCallList.Add(icall);
            }

            foreach (var call in s_InternalCallList)
            {
                var name = call.Name!;
                var fieldNameStart = name.IndexOf('+');
                var fieldNameEnd = name.IndexOf(",", fieldNameStart, StringComparison.CurrentCulture);
                var fieldName = name.Substring(fieldNameStart + 1, fieldNameEnd - fieldNameStart - 1);
                var containingTypeName = name.Remove(fieldNameStart, fieldNameEnd - fieldNameStart);

                Type? type = Type.GetType(containingTypeName);

                if (type == null)
                {
                    Console.WriteLine($"ERROR: The given delegate type couldn't be found: {call.Name}");
                    continue;
                }

                var bindingFlags = BindingFlags.Static | BindingFlags.NonPublic;
                var field = type.GetField(fieldName, bindingFlags);

                if (field == null)
                {
                    Console.WriteLine($"ERROR: Failed to find field with fieldName: {fieldName}");
                    continue;
                }

                field.SetValue(null, call.NativeFunctionPtr);
            }
        }
	}
}
