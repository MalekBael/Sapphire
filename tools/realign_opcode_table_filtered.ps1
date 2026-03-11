param(
    [string]$RepoRoot = "C:\Users\kevin\source\repos\sapphire",
    [string]$AlignedFile = "notes\\opcode_table_aligned.md",
    [string]$OutFile = "notes\\opcode_table_aligned_filtered.md",
    [string[]]$ExcludeOpcodes = @("0x0106", "0x01D8", "0x01D9", "0x01EF", "0x01AC", "0x01AD")
)

$repo = $RepoRoot
$alignedPath = Join-Path $repo $AlignedFile
$outPath = Join-Path $repo $OutFile

if (-not (Test-Path -LiteralPath $alignedPath)) {
    throw "Aligned file not found: $alignedPath"
}

$excludeSet = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
foreach ($op in $ExcludeOpcodes) {
    if (-not $op) { continue }
    $norm = $op.Trim().ToUpper()
    if ($norm -notmatch '^0x') { $norm = "0x$norm" }
    $excludeSet.Add($norm) | Out-Null
}

function Normalize-Opcode([string]$opcode) {
    if (-not $opcode) { return $null }
    $op = $opcode.Trim()
    if (-not $op) { return $null }
    if ($op -notmatch '^0x') { $op = "0x$($op.ToUpper())" }
    return $op.ToUpper()
}

function Parse-Aligned([string]$path) {
    $section = $null
    $emuC = New-Object System.Collections.Generic.List[object]
    $sumC = New-Object System.Collections.Generic.List[object]
    $emuS = New-Object System.Collections.Generic.List[object]
    $sumS = New-Object System.Collections.Generic.List[object]

    function Is-HeaderRow([string[]]$columns) {
        if ($columns.Count -lt 7) { return $true }
        $c0 = $columns[0]
        $c1 = $columns[1]
        $c3 = $columns[3]
        return ($c0 -match '^Emulator Time$') -or ($c1 -match '^Emulator Opcode$|^EMULATOR OPCODE$') -or ($c3 -match '^Summon Opcode$|^SUMMON OPCODE$')
    }

    foreach ($line in Get-Content -LiteralPath $path) {
        if ($line -match '^##\s+Client\s+\(C\)\s+Aligned') { $section = 'C'; continue }
        if ($line -match '^##\s+Server\s+\(S\)\s+Aligned') { $section = 'S'; continue }
        if (-not $section) { continue }
        if ($line -notmatch '^\|') { continue }
        if ($line -match '^\|\s*-') { continue }

        $parts = $line.Trim('|') -split '\|' | ForEach-Object { $_.Trim() }
        if (Is-HeaderRow $parts) { continue }

        $emuTime = $parts[0]
        $emuOpcode = Normalize-Opcode $parts[1]
        $emuDir = $parts[2]
        $sumOpcode = Normalize-Opcode $parts[3]
        $sumDir = $parts[4]
        $sumTime = $parts[5]

        if ($emuOpcode) {
            $entry = [pscustomobject]@{ Time = $emuTime; Opcode = $emuOpcode; Dir = $emuDir }
            if ($section -eq 'C') { $emuC.Add($entry) } else { $emuS.Add($entry) }
        }
        if ($sumOpcode) {
            $entry = [pscustomobject]@{ Time = $sumTime; Opcode = $sumOpcode; Dir = $sumDir }
            if ($section -eq 'C') { $sumC.Add($entry) } else { $sumS.Add($entry) }
        }
    }

    return [pscustomobject]@{
        EmulatorClient = $emuC
        SummonClient   = $sumC
        EmulatorServer = $emuS
        SummonServer   = $sumS
    }
}

function Filter-Seq($seq, $exclude) {
    $filtered = New-Object System.Collections.Generic.List[object]
    foreach ($item in $seq) {
        if ($exclude.Contains($item.Opcode)) { continue }
        $filtered.Add($item)
    }
    return $filtered
}

function Align-Seq($emuSeq, $sumSeq, [string]$dir) {
    $n = $emuSeq.Count
    $m = $sumSeq.Count
    $dp = New-Object 'int[,]' ($n + 1), ($m + 1)

    for ($i = 1; $i -le $n; $i++) {
        $iIndex = $i - 1
        $eOp = $emuSeq[$iIndex].Opcode
        for ($j = 1; $j -le $m; $j++) {
            $jIndex = $j - 1
            if ($eOp -eq $sumSeq[$jIndex].Opcode) {
                $dp[$i, $j] = $dp[$iIndex, $jIndex] + 1
            }
            else {
                $a = $dp[$iIndex, $j]
                $b = $dp[$i, $jIndex]
                $dp[$i, $j] = if ($a -ge $b) { $a } else { $b }
            }
        }
    }

    $rows = New-Object System.Collections.Generic.List[object]
    $i = $n
    $j = $m
    while ($i -gt 0 -or $j -gt 0) {
        $iIndex = $i - 1
        $jIndex = $j - 1

        if ($i -gt 0 -and $j -gt 0 -and $emuSeq[$iIndex].Opcode -eq $sumSeq[$jIndex].Opcode) {
            $e = $emuSeq[$iIndex]
            $s = $sumSeq[$jIndex]
            $rows.Add([pscustomobject]@{
                    EmulatorTime   = $e.Time
                    EmulatorOpcode = $e.Opcode.Substring(2)
                    EmulatorDir    = $dir
                    SummonOpcode   = $s.Opcode.Substring(2)
                    SummonDir      = $dir
                    SummonTime     = $s.Time
                    Status         = 'match'
                })
            $i--; $j--
            continue
        }

        $takeEmu = $false
        if ($j -eq 0 -and $i -gt 0) {
            $takeEmu = $true
        }
        elseif ($i -gt 0 -and $j -gt 0) {
            $takeEmu = $dp[$iIndex, $j] -ge $dp[$i, $jIndex]
        }
        elseif ($i -gt 0) {
            $takeEmu = $true
        }

        if ($takeEmu) {
            $e = $emuSeq[$iIndex]
            $rows.Add([pscustomobject]@{
                    EmulatorTime   = $e.Time
                    EmulatorOpcode = $e.Opcode.Substring(2)
                    EmulatorDir    = $dir
                    SummonOpcode   = ''
                    SummonDir      = ''
                    SummonTime     = ''
                    Status         = 'emu-only'
                })
            $i--
        }
        else {
            $s = $sumSeq[$jIndex]
            $rows.Add([pscustomobject]@{
                    EmulatorTime   = ''
                    EmulatorOpcode = ''
                    EmulatorDir    = ''
                    SummonOpcode   = $s.Opcode.Substring(2)
                    SummonDir      = $dir
                    SummonTime     = $s.Time
                    Status         = 'sum-only'
                })
            $j--
        }
    }

    [array]::Reverse($rows)
    return $rows
}

$seqs = Parse-Aligned $alignedPath
$emuC = Filter-Seq $seqs.EmulatorClient $excludeSet
$sumC = Filter-Seq $seqs.SummonClient $excludeSet
$emuS = Filter-Seq $seqs.EmulatorServer $excludeSet
$sumS = Filter-Seq $seqs.SummonServer $excludeSet

$alignedClient = Align-Seq $emuC $sumC 'C'
$alignedServer = Align-Seq $emuS $sumS 'S'

$out = New-Object System.Collections.Generic.List[string]
$out.Add('# Opcode Comparison (Aligned by LCS, filtered)')
$out.Add('')
$out.Add('## Legend')
$out.Add('')
$out.Add('- match: opcode appears in both streams in the same relative order')
$out.Add('- sum-only: retail-only opcode (missing from emulator capture)')
$out.Add('- emu-only: emulator-only opcode (missing from retail capture)')
$out.Add('')
$out.Add('## Filtered Opcodes')
$out.Add('')
foreach ($op in $ExcludeOpcodes) {
    $out.Add("- $op")
}
$out.Add('')
$out.Add('## Client (C) Aligned')
$out.Add('')
$out.Add('| Emulator Time | Emulator Opcode | Emulator Dir | Summon Opcode | Summon Dir | Summon Time | Status |')
$out.Add('|---:|:---:|:---:|:---:|:---:|:---:|:---:|')
foreach ($row in $alignedClient) {
    $out.Add("| $($row.EmulatorTime) | $($row.EmulatorOpcode) | $($row.EmulatorDir) | $($row.SummonOpcode) | $($row.SummonDir) | $($row.SummonTime) | $($row.Status) |")
}
$out.Add('')
$out.Add('## Server (S) Aligned')
$out.Add('')
$out.Add('| Emulator Time | Emulator Opcode | Emulator Dir | Summon Opcode | Summon Dir | Summon Time | Status |')
$out.Add('|---:|:---:|:---:|:---:|:---:|:---:|:---:|')
foreach ($row in $alignedServer) {
    $out.Add("| $($row.EmulatorTime) | $($row.EmulatorOpcode) | $($row.EmulatorDir) | $($row.SummonOpcode) | $($row.SummonDir) | $($row.SummonTime) | $($row.Status) |")
}

Set-Content -LiteralPath $outPath -Value $out
Write-Host "Wrote $outPath"
