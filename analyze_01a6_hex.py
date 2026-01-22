#!/usr/bin/env python3
"""
Reverse-engineer 0x01A6 (Inspect) packet structure from retail hex data.
Direct analysis of the actual packet bytes, NOT relying on SapphireHook.

The retail packet from retainer_summon_gil_view_class.xml:
- Packet size: 0x03f8 = 1016 bytes
- Visible string in packet: "Fairyqueen" (retainer name)
"""

import struct

# Raw hex from retail 0x01A6 packet (stripped of header, payload only)
hex_data = "000007010000000000000000000000000000000000000000000000000000000085030f000900000065000700020000000000000000000000010000000000000007080000000000000000000000000000000000000000000000000000000000000000000000000000ab08000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000b30b0000000000000000000000000000000000000000000000000000000000000000000000000000bb0d000000000000beeeefd1eeeeeeed00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000eb0c000000000000beeeefd1eeeeeeed000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000006410000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004a11000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004661697279717565656e000000000000000000000000000000000000000000000000000000000000000000000000000000050001270a020c008617691a1c32003c020400803164000084aa0000000000000000001c001900050001000100010026006a0002000500000000000000000001000100000000000000000000000000000000000000000000000000000000000000000000000000000000000800000010000000140000001500000010000000170000005600000085000000e8030000900100000000000000000000380000000800000011000000380000003800000024000000380000006400000064000000640000001500000010000000380000003800000038000000380000003800000038000000150000003800000038000000640000000000000000000000060000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"

data = bytes.fromhex(hex_data)
print(f"Total payload: {len(data)} bytes\n")

# Find the "Fairyqueen" string location
name = "Fairyqueen"
name_bytes = name.encode('ascii')
try:
    name_offset = data.find(name_bytes)
    print(f"Found '{name}' at offset: +{name_offset} (0x{name_offset:04x})")
    print(f"Context around name (32 bytes before, 32 after):")
    context_start = max(0, name_offset - 32)
    context_end = min(len(data), name_offset + 32 + 32)
    context = data[context_start:context_end]
    print(f"  {context.hex()}\n")
except:
    print("Name not found in packet\n")

# Based on known FFXIV inspection packet structure:
# Offset 0: ObjType (1B), Sex (1B), ClassJob (1B), Lv (1B), LvSync (1B), Padding (1B), Title (2B)
offset = 0

print("=== FIELD ANALYSIS ===\n")

# Parse known header fields
print(f"Offset {offset}-{offset+6}: Character basics")
obj_type, sex, class_job, lv, lv_sync, pad1 = struct.unpack('<BBBBBB', data[offset:offset+6])
print(f"  ObjType={obj_type}, Sex={sex}, Class={class_job}, Lv={lv}, LvSync={lv_sync}")
offset += 6

title = struct.unpack('<H', data[offset:offset+2])[0]
print(f"Offset {offset}: Title={title:04x}\n")
offset += 2

# GrandCompany info
gc, gc_rank, flag, pad2 = struct.unpack('<BBBB', data[offset:offset+4])
print(f"Offset {offset}: GrandCompany={gc}, GCRank={gc_rank}, Flag={flag}")
offset += 4

# Crest (u64)
crest = struct.unpack('<Q', data[offset:offset+8])[0]
print(f"Offset {offset}: Crest={crest:016x}")
offset += 8

# Crest enable + padding
crest_en = data[offset]
print(f"Offset {offset}: CrestEnable={crest_en}")
offset += 4  # 1 byte + 3 padding

# Weapons (u64 each)
main_wpn, sub_wpn = struct.unpack('<QQ', data[offset:offset+16])
print(f"Offset {offset}: MainWeapon={main_wpn:016x}, SubWeapon={sub_wpn:016x}")
offset += 16

# Pattern invalid (u16)
pattern = struct.unpack('<H', data[offset:offset+2])[0]
print(f"Offset {offset}: PatternInvalid={pattern:04x}")
offset += 2

# Rank
rank = data[offset]
offset += 1
offset += 1  # padding
print(f"Offset {offset-2}: Rank={rank}")

# Exp (u32)
exp = struct.unpack('<I', data[offset:offset+4])[0]
print(f"Offset {offset}: Exp={exp}")
offset += 4

# ItemLv
itemlv = data[offset]
offset += 1
offset += 3  # padding
print(f"Offset {offset-4}: ItemLv={itemlv}\n")

# Equipment - 14 items, each appears to be 40 bytes
print(f"Offset {offset}: Equipment array (14 items × 40 bytes = 560 bytes)")
eq_start = offset
for i in range(14):
    # Each equipment item: catalogId(4) + pattern(4) + sig(8) + hq(1) + stain(1) + materia[5](3*5=15) = 40
    cat_id, pattern_id, sig, hq, stain = struct.unpack('<IIQBB', data[offset:offset+17])
    materia_data = data[offset+17:offset+40]
    print(f"  Item {i:2d}: catalog={cat_id:08x} pattern={pattern_id:08x} sig={sig:016x} hq={hq} stain={stain}")
    offset += 40

print(f"After equipment: offset = {offset} (expected ~624)\n")

# Name[32]
name_start = offset
name_bytes_from_packet = data[offset:offset+32]
name_str = name_bytes_from_packet.rstrip(b'\x00').decode('ascii', errors='ignore')
print(f"Offset {offset}: Name = '{name_str}'")
offset += 32

# PSNId[17]
psn = data[offset:offset+17].rstrip(b'\x00').decode('ascii', errors='ignore')
print(f"Offset {offset}: PSNId = '{psn}'")
offset += 17

# Customize[26]
print(f"Offset {offset}: Customize (26 bytes)")
offset += 26

# Padding
offset += 3
print(f"Offset {offset-3}: padding (3 bytes)\n")

# ModelId[10]
print(f"Offset {offset}: ModelId array (10 × u32)")
for i in range(10):
    mid = struct.unpack('<I', data[offset:offset+4])[0]
    print(f"  Model {i}: {mid:08x}")
    offset += 4

# MasterName[32]
master_name = data[offset:offset+32].rstrip(b'\x00').decode('ascii', errors='ignore')
print(f"\nOffset {offset}: MasterName = '{master_name}'")
offset += 32

# SkillLv[3]
skill_lv = struct.unpack('<BBB', data[offset:offset+3])
print(f"Offset {offset}: SkillLv = {skill_lv}")
offset += 3

# Padding
offset += 1

# BaseParam[50]
print(f"Offset {offset}: BaseParam array (50 × u32)")
for i in range(50):
    param = struct.unpack('<I', data[offset:offset+4])[0]
    print(f"  Param {i:2d}: {param:3d}", end="")
    if (i+1) % 10 == 0:
        print()
    else:
        print("  ", end="")
    offset += 4

print(f"\n\nFinal offset: {offset}")
print(f"Total packet size: {len(data)}")
print(f"Match: {'✓ EXACT' if offset == len(data) else 'NOT EXACT - Off by ' + str(len(data) - offset) + ' bytes'}")

print("\n=== SUMMARY ===")
print(f"""
The 0x01A6 Inspect packet structure (1016 bytes payload):
  +0   : ObjType (u8), Sex (u8), ClassJob (u8), Lv (u8), LvSync (u8), __pad1 (u8)
  +6   : Title (u16)
  +8   : GrandCompany (u8), GrandCompanyRank (u8), Flag (u8), __pad2 (u8)
  +12  : Crest (u64)
  +20  : CrestEnable (u8), __pad3-5 (u8×3)
  +24  : MainWeaponModelId (u64), SubWeaponModelId (u64)
  +40  : PatternInvalid (u16), Rank (u8), __pad6 (u8)
  +44  : Exp (u32)
  +48  : ItemLv (u8), __pad7-9 (u8×3)
  +52  : Equipment[14] (40 bytes each = 560 bytes total)
  +612 : Name[32]
  +644 : PSNId[17]
  +661 : Customize[26]
  +687 : __pad10-12 (u8×3)
  +690 : ModelId[10] (40 bytes)
  +730 : MasterName[32]
  +762 : SkillLv[3], __pad13 (u8)
  +766 : BaseParam[50] (200 bytes)
  +966 : __unknown[50] remaining bytes to reach 1016
  
VERIFICATION: Offset after BaseParam[50] = 966, need 1016 = 50 bytes unknown/reserved
""")
