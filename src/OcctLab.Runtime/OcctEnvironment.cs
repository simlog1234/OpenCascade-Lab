using System.Reflection;

namespace OcctLab.Runtime;

/// <summary>
/// Thrown when the native OCCT binaries cannot be located.
/// </summary>
public sealed class OcctNotFoundException : Exception
{
    public OcctNotFoundException(string message) : base(message) { }
}

/// <summary>
/// Makes the native OCCT DLLs (TKernel.dll, TKOpenGl.dll, freetype.dll, ...) loadable by the
/// current process by prepending their folders to PATH.
///
/// IMPORTANT: call <see cref="Initialize"/> before the JIT touches any type from OcctProxy.dll.
/// Loading OcctProxy.dll triggers the Windows loader to resolve its native imports, so keep
/// the call in a method that itself does not reference OcctProxy types (e.g. Program.Main).
/// </summary>
public static class OcctEnvironment
{
    private static readonly object Gate = new();
    private static bool _initialized;

    /// <summary>Folders that were prepended to PATH (first one contains TKernel.dll).</summary>
    public static IReadOnlyList<string> RuntimeDirectories { get; private set; } = Array.Empty<string>();

    /// <summary>Root of the OCCT package (folder containing inc/ and win64/).</summary>
    public static string? OcctRoot { get; private set; }

    /// <summary>"Debug" or "Release": which OCCT binaries (bind/ or bin/) this build was linked against.</summary>
    public static string Configuration { get; private set; } = "Release";

    public static void Initialize()
    {
        lock (Gate)
        {
            if (_initialized) return;

            var meta = typeof(OcctEnvironment).Assembly
                .GetCustomAttributes<AssemblyMetadataAttribute>()
                .Where(a => a.Value is not null)
                .ToDictionary(a => a.Key, a => a.Value!, StringComparer.OrdinalIgnoreCase);

            Configuration = meta.GetValueOrDefault("OcctConfiguration", "Release");
            var dirs = ResolveDirectories(meta);

            var occtBin = dirs.FirstOrDefault(d => File.Exists(Path.Combine(d, "TKernel.dll")));
            if (occtBin is null)
            {
                throw new OcctNotFoundException(
                    "OCCT native binaries (TKernel.dll) were not found." + Environment.NewLine +
                    "  - Run  scripts/setup-occt.ps1  to download OCCT V8.0.1 into third_party/, or" + Environment.NewLine +
                    "  - set  OCCT_ROOT  to a folder containing win64\\vc14\\bin (and OCCT_RUNTIME_DIRS for 3rd-party DLLs)." + Environment.NewLine +
                    "Searched: " + (dirs.Count == 0 ? "(nothing)" : string.Join("; ", dirs)));
            }

            var currentPath = Environment.GetEnvironmentVariable("PATH") ?? string.Empty;
            var newPath = string.Join(Path.PathSeparator, dirs.Append(currentPath));
            Environment.SetEnvironmentVariable("PATH", newPath);

            OcctRoot = Path.GetFullPath(Path.Combine(occtBin, "..", "..", ".."));
            if (string.IsNullOrEmpty(Environment.GetEnvironmentVariable("CASROOT")))
            {
                Environment.SetEnvironmentVariable("CASROOT", OcctRoot);
            }

            RuntimeDirectories = dirs;
            _initialized = true;
        }
    }

    private static List<string> ResolveDirectories(Dictionary<string, string> meta)
    {
        var candidates = new List<string>();
        var binFolder = string.Equals(Configuration, "Debug", StringComparison.OrdinalIgnoreCase) ? "bind" : "bin";

        // 1) Explicit user override.
        var envRoot = Environment.GetEnvironmentVariable("OCCT_ROOT");
        if (!string.IsNullOrWhiteSpace(envRoot))
        {
            candidates.Add(Path.Combine(envRoot, "win64", "vc14", binFolder));
        }
        var envDirs = Environment.GetEnvironmentVariable("OCCT_RUNTIME_DIRS");
        if (!string.IsNullOrWhiteSpace(envDirs))
        {
            candidates.AddRange(Split(envDirs));
        }

        // 2) Build-time values from Directory.Build.props.
        if (meta.TryGetValue("OcctRuntimeDirs", out var baked))
        {
            candidates.AddRange(Split(baked));
        }

        // 3) Fallback: walk up from the exe looking for third_party/occt (useful after moving the repo).
        for (var dir = new DirectoryInfo(AppContext.BaseDirectory); dir is not null; dir = dir.Parent)
        {
            var probe = Path.Combine(dir.FullName, "third_party", "occt", "win64", "vc14", binFolder);
            if (Directory.Exists(probe))
            {
                candidates.Add(probe);
                var thirdParty = Path.Combine(dir.FullName, "third_party", "3rdparty");
                if (Directory.Exists(thirdParty))
                {
                    candidates.AddRange(Directory.GetDirectories(thirdParty)
                        .Select(p => Path.Combine(p, "bin"))
                        .Where(Directory.Exists));
                }
                break;
            }
        }

        return candidates
            .Select(p => Path.GetFullPath(p.Trim()))
            .Where(Directory.Exists)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToList();
    }

    private static IEnumerable<string> Split(string list) =>
        list.Split(';', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
}
