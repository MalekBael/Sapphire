#!/usr/bin/env python3
"""
Analyze 0x01A6 packet structure from retail capture WITHOUT using SapphireHook as reference.
Only uses the hex data + what we know about FFXIV inspection packets from Dalamud docs + binary analysis.
"""
import struct

# Raw hex from retail packet (payload only, after header)
hex_data = "000007010000000000000000000000000000000000000000000000000000000085030f000900000065000700020000000000000000000000010000000000000007080000000000000000000000000000000000000000000000000000000000000000000000000000ab08000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000b30b0000000000000000000000000000000000000000000000000000000000000000000000000000bb0d000000000000beeeefd1eeeeeeed00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000eb0c000000000000beeeefd1eeeeeeed000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000006410000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004a11000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004661697279717565656e000000000000000000000000000000000000000000000000000000000000000000000000000000050001270a020c008617691a1c32003c020400803164000084aa0000000000000000001c001900050001000100010026006a0002000500000000000000000001000100000000000000000000000000000000000000000000000000000000000000000000000000000000000800000010000000140000001500000010000000170000005600000085000000e8030000900100000000000000000000380000000800000011000000380000003800000024000000380000006400000064000000640000001500000010000000380000003800000038000000380000003800000038000000150000003800000038000000640000000000000000000000060000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"

data = bytes.fromhex(hex_data)
print(f"Total payload: {len(data)} bytes\n")

# Known FFXIV inspection packet structure (from Dalamud/SE research):
# - Player info (class, level, gear, stats)
# - String names
# Typical layout: basic info, then arrays, then strings

offset = 0

# Try to identify major sections
print("=== ANALYSIS OF 0x01A6 PACKET ===\n")

# Offset 0: Should contain object type, sex, class, level
print(f"Offset {offset}-{offset+10}: Basic player info")
obj_type, sex, class_job, lv, lv_sync = struct.unpack('<BBBBB', data[offset:offset+5])
print(f"  ObjType={obj_type}, Sex={sex}, Class={class_job}, Lv={lv}, LvSync={lv_sync}")
offset += 5

# Skip padding
offset += 1  # padding
print(f"\nOffset {offset-1}: padding byte\n")

# Title (u16)
title = struct.unpack('<H', data[offset:offset+2])[0]
print(f"Offset {offset}: Title = {title:04x}")
offset += 2

# GC + GC rank + flags
gc, gc_rank, flag = struct.unpack('<BBB', data[offset:offset+3])
print(f"Offset {offset}: GrandCompany={gc}, GCRank={gc_rank}, Flag={flag}")
offset += 3

# Skip padding
offset += 1
print(f"Offset {offset-1}: padding\n")

# Crest (u64)
crest = struct.unpack('<Q', data[offset:offset+8])[0]
print(f"Offset {offset}: Crest = {crest:016x}")
offset += 8

# Crest enable + padding
crest_en = data[offset]
print(f"Offset {offset}: CrestEnable = {crest_en}")
offset += 1

offset += 3  # padding
print(f"Offsets {offset-3}-{offset-1}: padding\n")

# Weapons
main_wpn = struct.unpack('<Q', data[offset:offset+8])[0]
sub_wpn = struct.unpack('<Q', data[offset+8:offset+16])[0]
print(f"Offset {offset}: MainWeapon = {main_wpn:016x}")
print(f"Offset {offset+8}: SubWeapon = {sub_wpn:016x}")
offset += 16

# Pattern invalid (u16)
pattern = struct.unpack('<H', data[offset:offset+2])[0]
print(f"Offset {offset}: PatternInvalid = {pattern:04x}")
offset += 2

# Rank + padding + exp
rank = data[offset]
offset += 1
offset += 1  # padding
exp = struct.unpack('<I', data[offset:offset+4])[0]
print(f"Offset {offset-2}: Rank = {rank}")
print(f"Offset {offset}: Exp = {exp}")
offset += 4

# ItemLv + padding
itemlv = data[offset]
offset += 1
offset += 3  # padding
print(f"Offset {offset-4}: ItemLv = {itemlv}\n")

# Equipment array (14 items)
# Assuming each is: catalogId(u32) + pattern(u32) + signature(u64) + hq(u8) + stain(u8) + materia[5](u16+u8)*5
eq_start = offset
print(f"Offset {offset}: Equipment array starts (14 items)")
for i in range(14):
    cat_id = struct.unpack('<I', data[offset:offset+4])[0]
    pattern_id = struct.unpack('<I', data[offset+4:offset+8])[0]
    sig = struct.unpack('<Q', data[offset+8:offset+16])[0]
    hq = data[offset+16]
    stain = data[offset+17]
    print(f"  Item {i}: catalog={cat_id:08x}, pattern={pattern_id:08x}, sig={sig:016x}, hq={hq}, stain={stain}")
    offset += 40  # Assuming 40 bytes per equipment item (4+4+8+1+1+5*3 materia)

print(f"After equipment: offset = {offset}\n")

# Name[32]
name_start = offset
name_bytes = data[offset:offset+32]
name = name_bytes.rstrip(b'\x00').decode('ascii', errors='ignore')
print(f"Offset {offset}: Name = '{name}'")
offset += 32

# PSNId[17]
psn_bytes = data[offset:offset+17]
psn = psn_bytes.rstrip(b'\x00').decode('ascii', errors='ignore')
print(f"Offset {offset}: PSNId = '{psn}'")
offset += 17

# Customize[26]
customize = data[offset:offset+26]
print(f"Offset {offset}: Customize data (26 bytes)")
offset += 26

# Padding after customize
offset += 3
print(f"Offset {offset-3}-{offset-1}: padding\n")

# ModelId[10]
print(f"Offset {offset}: ModelId array (10 uint32s)")
for i in range(10):
    mid = struct.unpack('<I', data[offset:offset+4])[0]
    print(f"  Model {i}: {mid:08x}")
    offset += 4

# MasterName[32]
master_bytes = data[offset:offset+32]
master_name = master_bytes.rstrip(b'\x00').decode('ascii', errors='ignore')
print(f"\nOffset {offset}: MasterName = '{master_name}'")
offset += 32

# SkillLv[3]
skill_lv = struct.unpack('<BBB', data[offset:offset+3])
print(f"Offset {offset}: SkillLv = {skill_lv}")
offset += 3

# Padding
offset += 1

# BaseParam[50]
print(f"Offset {offset}: BaseParam array (50 uint32s)")
for i in range(50):
    param = struct.unpack('<I', data[offset:offset+4])[0]
    print(f"  Param {i:2d}: {param:3d}", end="")
    if (i+1) % 10 == 0:
        print()
    else:
        print("  ", end="")
    offset += 4

print(f"\n\nFinal offset: {offset}")
print(f"Total payload: {len(data)}")
print(f"Match: {'✓' if offset == len(data) else 'NOT MATCH - Missing ' + str(len(data) - offset) + ' bytes'}")
