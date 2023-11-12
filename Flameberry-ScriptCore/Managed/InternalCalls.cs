using System;
using System.Runtime.InteropServices;

namespace Flameberry.Managed
{
	internal static class InternalCalls
	{
		[UnmanagedCallersOnly]
		internal static void SetInternalCalls(IntPtr callArray, int size)
		{
		}
	}
}

