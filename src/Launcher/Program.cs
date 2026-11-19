using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

internal static class Program
{
    private const uint ProcessCreateThread = 0x0002;
    private const uint ProcessQueryInformation = 0x0400;
    private const uint ProcessVmOperation = 0x0008;
    private const uint ProcessVmWrite = 0x0020;
    private const uint ProcessVmRead = 0x0010;
    private const uint MemCommit = 0x1000;
    private const uint MemReserve = 0x2000;
    private const uint PageReadWrite = 0x04;

    public static int Main(string[] args)
    {
        bool injectOnly = false;
        foreach (string arg in args)
        {
            if (arg == "--inject-only") injectOnly = true;
        }

        string baseDir = AppContext.BaseDirectory;
        string dll = Path.Combine(baseDir, "Kof98Bgm.dll");
        string gameDir = @"D:\SteamLibrary\steamapps\common\The King of Fighters'98 Ultimate Match";
        string exe = "KingOfFighters98UM_x64.exe";
        string cfg = Path.Combine(baseDir, "launcher.cfg");
        if (File.Exists(cfg))
        {
            foreach (string raw in File.ReadAllLines(cfg))
            {
                string line = raw.Trim();
                if (line.StartsWith("game_dir=")) gameDir = line.Substring("game_dir=".Length).Trim();
                else if (line.StartsWith("game_exe=")) exe = line.Substring("game_exe=".Length).Trim();
            }
        }

        if (!File.Exists(dll))
        {
            Console.WriteLine("Falta Kof98Bgm.dll junto al lanzador.");
            return 1;
        }
        string exePath = Path.Combine(gameDir, exe);
        if (!File.Exists(exePath))
        {
            Console.WriteLine("No está el ejecutable del juego: " + exePath);
            return 1;
        }

        Environment.SetEnvironmentVariable("SteamAppId", "222420");
        Environment.SetEnvironmentVariable("SteamGameId", "222420");
        Process existing = FindGame(exe);
        if (existing != null)
        {
            if (ModuleLoaded(existing, "Kof98Bgm.dll"))
            {
                Console.WriteLine("El selector de música ya está cargado.");
                return 0;
            }
            Console.WriteLine("El juego ya está abierto. Inyecto el selector en el proceso " + existing.Id + ".");
            return Inject(existing.Id, dll) ? 0 : 1;
        }
        if (injectOnly)
        {
            Console.WriteLine("El juego no está en ejecución.");
            return 1;
        }
        return StartAndInject(exePath, gameDir, dll) ? 0 : 1;
    }

    private static Process FindGame(string exe)
    {
        Process[] list = Process.GetProcessesByName(Path.GetFileNameWithoutExtension(exe));
        return list.Length == 0 ? null : list[0];
    }

    private static bool ModuleLoaded(Process p, string name)
    {
        try
        {
            foreach (ProcessModule m in p.Modules)
                if (string.Equals(m.ModuleName, name, StringComparison.OrdinalIgnoreCase)) return true;
        }
        catch
        {
        }
        return false;
    }

    private static bool StartAndInject(string exePath, string gameDir, string dll)
    {
        Process p;
        try { p = Process.Start(new ProcessStartInfo(exePath) { WorkingDirectory = gameDir, UseShellExecute = false }); }
        catch (Exception e)
        {
            Console.WriteLine("No se pudo iniciar el juego: " + e.Message);
            return false;
        }
        Console.WriteLine("Proceso " + p.Id + " iniciado.");
        System.Threading.Thread.Sleep(800);
        if (!p.HasExited) return Inject(p.Id, dll);
        string name = Path.GetFileNameWithoutExtension(exePath);
        for (int i = 0; i < 40; i++)
        {
            foreach (Process candidate in Process.GetProcessesByName(name))
            {
                if (candidate.Id == p.Id) continue;
                Console.WriteLine("Inyecto en el proceso " + candidate.Id + ".");
                return Inject(candidate.Id, dll);
            }
            System.Threading.Thread.Sleep(500);
        }
        Console.WriteLine("No apareció un proceso nuevo del juego.");
        return false;
    }

    private static bool Inject(int pid, string dll)
    {
        IntPtr process = OpenProcess(ProcessCreateThread | ProcessQueryInformation | ProcessVmOperation | ProcessVmWrite | ProcessVmRead, false, pid);
        if (process == IntPtr.Zero)
        {
            Console.WriteLine("OpenProcess falló con el error " + Marshal.GetLastWin32Error() + ".");
            return false;
        }
        try
        {
            byte[] data = Encoding.Unicode.GetBytes(dll + "\0");
            IntPtr remote = VirtualAllocEx(process, IntPtr.Zero, (UIntPtr)data.Length, MemCommit | MemReserve, PageReadWrite);
            if (remote == IntPtr.Zero || !WriteProcessMemory(process, remote, data, (UIntPtr)data.Length, out _))
            {
                Console.WriteLine("No se pudo escribir la ruta de la DLL (error " + Marshal.GetLastWin32Error() + ").");
                return false;
            }
            IntPtr load = GetProcAddress(GetModuleHandleW("kernel32.dll"), "LoadLibraryW");
            IntPtr thread = CreateRemoteThread(process, IntPtr.Zero, UIntPtr.Zero, load, remote, 0, IntPtr.Zero);
            if (thread == IntPtr.Zero)
            {
                Console.WriteLine("CreateRemoteThread falló con el error " + Marshal.GetLastWin32Error() + ".");
                return false;
            }
            WaitForSingleObject(thread, 10000);
            GetExitCodeThread(thread, out uint code);
            CloseHandle(thread);
            if (code == 0)
            {
                Console.WriteLine("LoadLibraryW no cargó la DLL.");
                return false;
            }
            Console.WriteLine("Selector de música activo en esta sesión.");
            return true;
        }
        finally
        {
            CloseHandle(process);
        }
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr VirtualAllocEx(IntPtr process, IntPtr addr, UIntPtr size, uint type, uint protect);
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool WriteProcessMemory(IntPtr process, IntPtr addr, byte[] buffer, UIntPtr size, out UIntPtr written);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
    private static extern IntPtr GetModuleHandleW(string name);
    [DllImport("kernel32.dll", CharSet = CharSet.Ansi)]
    private static extern IntPtr GetProcAddress(IntPtr module, string name);
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern IntPtr CreateRemoteThread(IntPtr process, IntPtr attr, UIntPtr stack, IntPtr start, IntPtr param, uint flags, IntPtr tid);
    [DllImport("kernel32.dll")]
    private static extern uint WaitForSingleObject(IntPtr handle, uint ms);
    [DllImport("kernel32.dll")]
    private static extern bool GetExitCodeThread(IntPtr thread, out uint code);
    [DllImport("kernel32.dll")]
    private static extern bool CloseHandle(IntPtr handle);
}
