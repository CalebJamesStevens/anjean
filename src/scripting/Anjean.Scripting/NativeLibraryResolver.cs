using System.Reflection;
using System.Runtime.InteropServices;

namespace Anjean;

internal static class NativeLibraryResolver
{
    private static bool initialized = false;

    internal static void Initialize()
    {
        if (initialized)
        {
            return;
        }

        NativeLibrary.SetDllImportResolver(
            typeof(Native).Assembly,
            Resolve
        );

        initialized = true;
    }

    private static IntPtr Resolve(
        string libraryName,
        Assembly assembly,
        DllImportSearchPath? searchPath
    )
    {
        if (libraryName != "Anjean.Native")
        {
            return IntPtr.Zero;
        }

        string? nativeLibraryPath = Environment.GetEnvironmentVariable("ANJEAN_NATIVE_LIBRARY");

        if (string.IsNullOrWhiteSpace(nativeLibraryPath))
        {
            throw new InvalidOperationException(
                "ANJEAN_NATIVE_LIBRARY was not set by the Anjean host."
            );
        }

        return NativeLibrary.Load(nativeLibraryPath);
    }
}