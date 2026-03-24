# Resolve RVAs from ProjectUmeowmi crash reports using DbgHelp + PDB next to the exe.
# Usage: .\ResolveUERva.ps1 0x6B949C2 0x97E3ACB
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string[]]$RvaHex
)

$ErrorActionPreference = "Stop"
$bin = Join-Path $PSScriptRoot "..\Binaries\Win64"
$exe = Join-Path $bin "ProjectUmeowmi.exe"
if (-not (Test-Path $exe)) { throw "Missing $exe" }

Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class UeSym {
    const int MAX_SYM_NAME = 2000;

    [DllImport("dbghelp.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern bool SymInitialize(IntPtr hProcess, string UserSearchPath, bool fInvadeProcess);

    [DllImport("dbghelp.dll", SetLastError = true)]
    public static extern bool SymCleanup(IntPtr hProcess);

    [DllImport("dbghelp.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern ulong SymLoadModuleEx(IntPtr hProcess, IntPtr hFile, string ImageName, string ModuleName, ulong BaseOfDll, uint DllSize, IntPtr Data, uint Flags);

    [DllImport("dbghelp.dll", SetLastError = true, CharSet = CharSet.Ansi)]
    public static extern bool SymFromAddr(IntPtr hProcess, ulong Address, out ulong Displacement, IntPtr Symbol);

    public static string Resolve(string pdbDir, string imagePath, ulong rva) {
        IntPtr proc = new IntPtr(-1);
        if (!SymInitialize(proc, pdbDir, false))
            return "SymInitialize failed: " + Marshal.GetLastWin32Error();
        try {
            const ulong imageBase = 0x140000000UL;
            ulong mod = SymLoadModuleEx(proc, IntPtr.Zero, imagePath, null, imageBase, 0, IntPtr.Zero, 0);
            if (mod == 0)
                return "SymLoadModuleEx failed: " + Marshal.GetLastWin32Error();
            ulong addr = imageBase + rva;
            int buf = 88 + MAX_SYM_NAME;
            IntPtr p = Marshal.AllocHGlobal(buf);
            try {
                for (int i = 0; i < buf; i++) Marshal.WriteByte(p, i, 0);
                Marshal.WriteInt32(p, 0, 88);
                Marshal.WriteInt32(p, 80, MAX_SYM_NAME);
                ulong disp;
                if (!SymFromAddr(proc, addr, out disp, p))
                    return "SymFromAddr failed: " + Marshal.GetLastWin32Error();
                IntPtr namePtr = new IntPtr(p.ToInt64() + 88);
                string name = Marshal.PtrToStringAnsi(namePtr);
                return name + " +0x" + disp.ToString("x");
            } finally {
                Marshal.FreeHGlobal(p);
            }
        } finally {
            SymCleanup(proc);
        }
    }
}
"@

foreach ($h in $RvaHex) {
    $clean = $h.Trim() -replace '^0x', ''
    $rva = [Convert]::ToUInt64($clean, 16)
    $line = [UeSym]::Resolve($bin, $exe, $rva)
    Write-Output ("0x{0:X} -> {1}" -f $rva, $line)
}
