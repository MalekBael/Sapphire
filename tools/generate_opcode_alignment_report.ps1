param(
    [string]$RepoRoot = "C:\Users\kevin\source\repos\sapphire",
    [string]$AlignedFile = "notes\\opcode_table_aligned.md",
    [string]$OutFile = "notes\\opcode_table_alignment_report.md"
)

$repo = $RepoRoot
$aligned = Join-Path $repo $AlignedFile
$out = Join-Path $repo $OutFile
$clientHdr = Join-Path $repo "src\\common\\Network\\PacketDef\\ClientIpcs.h"
$serverHdr = Join-Path $repo "src\\common\\Network\\PacketDef\\ServerIpcs.h"

function Build-Map([string]$path, [string]$prefix) {
    $map = @{}
    $lines = Get-Content -LiteralPath $path
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line -match '^\s*([A-Za-z0-9_]+)\s*=\s*0x([0-9A-Fa-f]{4})') {
            $name = $matches[1]
            $hex = "0x$($matches[2].ToUpper())"
            $map[$hex] = [pscustomobject]@{
                Name    = "$prefix$name"
                DefLine = $i + 1
                DefPath = $path
            }
        }
    }
    return $map
}

$clientMap = Build-Map $clientHdr 'ClientIpcs::'
$serverMap = Build-Map $serverHdr 'ServerIpcs::'

$items = @()
foreach ($line in Get-Content -LiteralPath $aligned) {
    if ($line -notmatch '^\|') { continue }
    if ($line -match '^\|\s*-') { continue }
    $parts = $line.Trim('|') -split '\|' | ForEach-Object { $_.Trim() }
    if ($parts.Count -lt 7) { continue }

    $status = $parts[6]
    if ($status -eq 'sum-only') {
        $opcode = $parts[3]
        $dir = $parts[4]
        $side = 'retail'
    }
    elseif ($status -eq 'emu-only') {
        $opcode = $parts[1]
        $dir = $parts[2]
        $side = 'emulator'
    }
    else {
        continue
    }

    if (-not $opcode) { continue }
    if ($opcode -notmatch '^0x') { $opcode = "0x$($opcode.ToUpper())" }

    $items += [pscustomobject]@{
        Side   = $side
        Dir    = $dir
        Opcode = $opcode
    }
}

$groups = $items | Group-Object Side, Dir, Opcode | Sort-Object Name

$report = @()
$report += '# Opcode Alignment Divergence Report'
$report += ''
$report += 'Generated from notes/opcode_table_aligned.md. Rows reflect opcodes present in one capture stream but missing in the other.'

function Add-Section([string]$title, [string]$side, [string]$dir) {
    $script:report += ''
    $script:report += "## $title"
    $script:report += ''
    $script:report += '| Opcode | Dir | Mapped Name | Definition |'
    $script:report += '|:---:|:---:|:---|:---|'

    foreach ($g in $groups) {
        $parts = $g.Name -split ','
        if ($parts[0].Trim() -ne $side -or $parts[1].Trim() -ne $dir) { continue }

        $opcode = $parts[2].Trim()
        $map = if ($dir -eq 'C') { $clientMap } else { $serverMap }
        $entry = $map[$opcode]
        $name = if ($entry) { $entry.Name } else { 'unknown' }
        $def = if ($entry) {
            $rel = $entry.DefPath.Substring($repo.Length + 1) -replace '\\', '/'
            "${rel}:$($entry.DefLine)"
        }
        else { '-' }

        $script:report += "| $opcode | $dir | $name | $def |"
    }
}

Add-Section 'Retail-only (sum-only) Client' 'retail' 'C'
Add-Section 'Retail-only (sum-only) Server' 'retail' 'S'
Add-Section 'Emulator-only (emu-only) Client' 'emulator' 'C'
Add-Section 'Emulator-only (emu-only) Server' 'emulator' 'S'

Set-Content -LiteralPath $out -Value $report
Write-Host "Wrote $out"
