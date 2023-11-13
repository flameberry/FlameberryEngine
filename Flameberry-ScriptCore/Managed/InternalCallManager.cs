using System;
using System.Runtime.InteropServices;

namespace Flameberry.Managed
{
	internal static class InternalCallManager
	{
		[UnmanagedCallersOnly]
		internal static void SetInternalCalls(IntPtr callArray, int size)
		{
			Console.WriteLine(Type.GetType("Actor"));
		}
	}
}
