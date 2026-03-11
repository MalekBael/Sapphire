$emu = "C:\Users\kevin\source\repos\sapphire\notes\retainer_emulator_already_assigned_class3.35.xml"
$retail = "C:\Users\kevin\source\repos\sapphire\notes\retainer_summon_gil_view_class.xml"

function Get-FirstData($path, $msg, $dir) {
  $lines = Get-Content -Path $path
  for ($i = 0; $i -lt $lines.Length; $i++) {
    if ($lines[$i] -match "<Message>$msg</Message>") {
      $ok = $true
      for ($j = [Math]::Max(0, $i - 5); $j -lt [Math]::Min($lines.Length, $i + 5); $j++) {
        if ($lines[$j] -match "<Direction>") {
          $ok = ($lines[$j] -like "*${dir}*")
        }
      }
      if (-not $ok) { continue }
      for ($j = $i; $j -lt [Math]::Min($lines.Length, $i + 10); $j++) {
        if ($lines[$j] -match "<Data>") {
          return ($lines[$j] -replace ".*<Data>", "" -replace "</Data>.*", "" ).Trim()
        }
      }
    }
  }
  return $null
}

function HexToBytes($hex) {
  $bytes = New-Object byte[] ($hex.Length / 2)
  for ($i = 0; $i -lt $hex.Length; $i += 2) {
    $bytes[$i / 2] = [Convert]::ToByte($hex.Substring($i, 2), 16)
  }
  return $bytes
}

function ToU32($bytes, $off) { [BitConverter]::ToUInt32($bytes, $off) }
function ToU64($bytes, $off) { [BitConverter]::ToUInt64($bytes, $off) }

function Parse-01AB($hex) {
  $b = HexToBytes $hex
  $payload = $b[0x20..(0x20 + 71)]
  $nameBytes = $payload[34..(34 + 31)]
  $name = ([System.Text.Encoding]::UTF8.GetString($nameBytes)).Split([char]0)[0]
  [pscustomobject]@{
    handlerId    = (ToU32 $payload 0)
    unknown1     = (ToU32 $payload 4)
    retainerId   = (ToU64 $payload 8)
    slotIndex    = $payload[16]
    sellingCount = (ToU32 $payload 20)
    activeFlag   = $payload[24]
    classJob     = $payload[25]
    unknown3     = (ToU32 $payload 28)
    unknown4     = $payload[32]
    personality  = $payload[33]
    name         = $name
    tail16       = [BitConverter]::ToUInt16($payload, 66)
    tail32       = (ToU32 $payload 68)
  }
}

function Parse-01AF($hex) {
  $b = HexToBytes $hex
  $payload = $b[0x20..($b.Length - 1)]
  [pscustomobject]@{
    contextId      = (ToU32 $payload 0)
    unknown1       = (ToU32 $payload 4)
    storageId      = [BitConverter]::ToUInt16($payload, 8)
    containerIndex = [BitConverter]::ToUInt16($payload, 10)
    stack          = (ToU32 $payload 12)
    catalogId      = (ToU32 $payload 16)
  }
}

function Parse-01AA($hex) {
  $b = HexToBytes $hex
  $payload = $b[0x20..($b.Length - 1)]
  [pscustomobject]@{
    handlerId     = (ToU32 $payload 0)
    maxSlots      = $payload[4]
    retainerCount = $payload[5]
    padding       = [BitConverter]::ToUInt16($payload, 6)
  }
}

function Parse-01B0($hex) {
  $b = HexToBytes $hex
  $payload = $b[0x20..($b.Length - 1)]
  [pscustomobject]@{
    contextId = (ToU32 $payload 0)
    size      = [BitConverter]::ToInt32($payload, 4)
    storageId = (ToU32 $payload 8)
    unknown1  = (ToU32 $payload 12)
  }
}

function Parse-01EF($hex) {
  $b = HexToBytes $hex
  $payload = $b[0x20..($b.Length - 1)]
  [pscustomobject]@{
    marketContext = (ToU32 $payload 0)
    isMarket      = (ToU32 $payload 4)
  }
}

function Parse-010B($hex) {
  $b = HexToBytes $hex
  $payload = $b[0x20..($b.Length - 1)]
  $nameBytes = $payload[21..(21 + 31)]
  $name = ([System.Text.Encoding]::UTF8.GetString($nameBytes)).Split([char]0)[0]
  [pscustomobject]@{
    retainerId = (ToU64 $payload 0)
    unknown15  = $payload[15]
    classJob   = $payload[20]
    name       = $name
  }
}

function Get-AllData($path, $msg, $dir) {
  $results = New-Object System.Collections.Generic.List[string]
  $lines = Get-Content -Path $path
  for ($i = 0; $i -lt $lines.Length; $i++) {
    if ($lines[$i] -match "<Message>$msg</Message>") {
      $ok = $true
      for ($j = [Math]::Max(0, $i - 5); $j -lt [Math]::Min($lines.Length, $i + 5); $j++) {
        if ($lines[$j] -match "<Direction>") {
          $ok = ($lines[$j] -like "*${dir}*")
        }
      }
      if (-not $ok) { continue }
      for ($j = $i; $j -lt [Math]::Min($lines.Length, $i + 10); $j++) {
        if ($lines[$j] -match "<Data>") {
          $results.Add(($lines[$j] -replace ".*<Data>", "" -replace "</Data>.*", "" ).Trim())
          break
        }
      }
    }
  }
  return $results
}

function Get-PacketSequence($path) {
  $seq = New-Object System.Collections.Generic.List[object]
  $lines = Get-Content -Path $path
  $cur = [ordered]@{ Direction = $null; Message = $null; Timestamp = $null }
  for ($i = 0; $i -lt $lines.Length; $i++) {
    $line = $lines[$i].Trim()
    if ($line -match "<Direction>") {
      $cur.Direction = ($line -replace ".*<Direction>", "" -replace "</Direction>.*", "" ).Trim()
    }
    elseif ($line -match "<Message>") {
      $cur.Message = ($line -replace ".*<Message>", "" -replace "</Message>.*", "" ).Trim()
    }
    elseif ($line -match "<Timestamp>") {
      $cur.Timestamp = ($line -replace ".*<Timestamp>", "" -replace "</Timestamp>.*", "" ).Trim()
    }
    elseif ($line -match "</PacketEntry>") {
      if ($cur.Message) {
        $seq.Add([pscustomobject]@{
            Direction = $cur.Direction
            Message   = $cur.Message
            Timestamp = $cur.Timestamp
          })
      }
      $cur = [ordered]@{ Direction = $null; Message = $null; Timestamp = $null }
    }
  }
  return $seq
}

function Show-SequenceAfter($seq, $message, $count) {
  $idx = ($seq | Select-Object -ExpandProperty Message).IndexOf($message)
  if ($idx -lt 0) {
    "not found"
    return
  }
  $seq | Select-Object -Skip $idx -First $count | Format-Table Timestamp, Direction, Message
}

$emu01ab = Get-FirstData $emu "01AB" "S"
$ret01ab = Get-FirstData $retail "01AB" "S"
$emu01af = Get-FirstData $emu "01AF" "S"
$ret01af = Get-FirstData $retail "01AF" "S"
$emu01aa = Get-FirstData $emu "01AA" "S"
$ret01aa = Get-FirstData $retail "01AA" "S"
$emu01b0 = Get-FirstData $emu "01B0" "S"
$ret01b0 = Get-FirstData $retail "01B0" "S"
$emu01ef = Get-FirstData $emu "01EF" "S"
$ret01ef = Get-FirstData $retail "01EF" "S"
$emu010b = Get-FirstData $emu "010B" "S"
$ret010b = Get-FirstData $retail "010B" "S"

$emu01afAll = Get-AllData $emu "01AF" "S"
$ret01afAll = Get-AllData $retail "01AF" "S"
$emu01abAll = Get-AllData $emu "01AB" "S"
$ret01abAll = Get-AllData $retail "01AB" "S"
$emu01b0All = Get-AllData $emu "01B0" "S"
$ret01b0All = Get-AllData $retail "01B0" "S"

"emu 01AB:"; if ($emu01ab) { Parse-01AB $emu01ab | Format-List } else { "not found" }
"retail 01AB:"; if ($ret01ab) { Parse-01AB $ret01ab | Format-List } else { "not found" }
"emu 01AF:"; if ($emu01af) { Parse-01AF $emu01af | Format-List } else { "not found" }
"retail 01AF:"; if ($ret01af) { Parse-01AF $ret01af | Format-List } else { "not found" }
"emu 01AA:"; if ($emu01aa) { Parse-01AA $emu01aa | Format-List } else { "not found" }
"retail 01AA:"; if ($ret01aa) { Parse-01AA $ret01aa | Format-List } else { "not found" }
"emu 01B0:"; if ($emu01b0) { Parse-01B0 $emu01b0 | Format-List } else { "not found" }
"retail 01B0:"; if ($ret01b0) { Parse-01B0 $ret01b0 | Format-List } else { "not found" }
"emu 01EF:"; if ($emu01ef) { Parse-01EF $emu01ef | Format-List } else { "not found" }
"retail 01EF:"; if ($ret01ef) { Parse-01EF $ret01ef | Format-List } else { "not found" }
"emu 010B:"; if ($emu010b) { Parse-010B $emu010b | Format-List } else { "not found" }
"retail 010B:"; if ($ret010b) { Parse-010B $ret010b | Format-List } else { "not found" }

"emu 01AB all slots:";
$emu01abAll | ForEach-Object { Parse-01AB $_ } | Format-Table slotIndex, activeFlag, classJob, sellingCount, personality, name

"retail 01AB all slots:";
$ret01abAll | ForEach-Object { Parse-01AB $_ } | Format-Table slotIndex, activeFlag, classJob, sellingCount, personality, name

"emu 01AF first retainer-bag entry (storageId>=10000):"
$emuRet = $emu01afAll | ForEach-Object { Parse-01AF $_ } | Where-Object { $_.storageId -ge 10000 } | Select-Object -First 1
if ($emuRet) { $emuRet | Format-List } else { "not found" }

"retail 01AF first retainer-bag entry (storageId>=10000):"
$retRet = $ret01afAll | ForEach-Object { Parse-01AF $_ } | Where-Object { $_.storageId -ge 10000 } | Select-Object -First 1
if ($retRet) { $retRet | Format-List } else { "not found" }

"emu 01AF first 5 entries (storageId/unknown1):"
$emu01afAll | ForEach-Object { Parse-01AF $_ } | Select-Object -First 5 | Format-Table storageId, unknown1, contextId, catalogId

"retail 01AF first 5 entries (storageId/unknown1):"
$ret01afAll | ForEach-Object { Parse-01AF $_ } | Select-Object -First 5 | Format-Table storageId, unknown1, contextId, catalogId

"emu 01B0 first 8 entries (storageId/size/unknown1):"
$emu01b0All | ForEach-Object { Parse-01B0 $_ } | Select-Object -First 8 | Format-Table storageId, size, unknown1, contextId

"retail 01B0 first 8 entries (storageId/size/unknown1):"
$ret01b0All | ForEach-Object { Parse-01B0 $_ } | Select-Object -First 8 | Format-Table storageId, size, unknown1, contextId

"emu sequence after first 01AB (next 30):"
$emuSeq = Get-PacketSequence $emu
Show-SequenceAfter $emuSeq "01AB" 30

"retail sequence after first 01AB (next 30):"
$retSeq = Get-PacketSequence $retail
Show-SequenceAfter $retSeq "01AB" 30
