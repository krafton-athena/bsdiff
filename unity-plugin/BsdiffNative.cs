using System.Runtime.InteropServices;

public static class BsdiffNative
{
#if UNITY_IOS && !UNITY_EDITOR
    private const string DllName = "__Internal";
#else
    private const string DllName = "bsdiff";
#endif

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int bspatch_file(string oldPath, string newPath, string patchPath);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    private static extern int bsdiff_file(string oldPath, string newPath, string patchPath);

    /// <summary>
    /// Apply a patch file to create a new file.
    /// </summary>
    /// <returns>true on success</returns>
    public static bool ApplyPatch(string oldPath, string newPath, string patchPath)
    {
        return bspatch_file(oldPath, newPath, patchPath) == 0;
    }

    /// <summary>
    /// Create a patch file from old and new files.
    /// </summary>
    /// <returns>true on success</returns>
    public static bool CreatePatch(string oldPath, string newPath, string patchPath)
    {
        return bsdiff_file(oldPath, newPath, patchPath) == 0;
    }
}