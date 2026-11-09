#!/usr/bin/env python3
"""Append a Calypso firmware metadata block to a UF2 file.

The on-device firmware updater (src/mist/FirmwareUpdate.cxx) refuses to flash
a /FIRMWARE.UPG file that does not carry this block. The block is a regular
UF2 block targeting the last 256 bytes of the 16 MB flash (0x10FFFF00).

The tagged file is meant for the SD card update only (written as FIRMWARE.UPG
by the build). It must not be used for BOOTSEL: the image blocks announce N
blocks in total and the metadata block N + 1, so the bootrom reboots once the N
image blocks are in and the copy of the last block fails (Windows reports error
0x800703EE). Use the plain UF2 for BOOTSEL.

Block layout (matches Uf2Meta in src/mist/FirmwareUpdate.h):

    offset  size  field
    0       8     magic "CLPFWMET"
    8       16    firmware version (FVERSION, e.g. "0.23")
    24      16    build date (YYYYMMDD)
    40      16    git short hash
    56      4     CRC32 (zlib polynomial) of the whole image payload
    60      4     image payload size in bytes (multiple of 256)
    64      4     number of image blocks (not counting the meta block)
    68      4     CRC32 of the preceding 68 bytes
    72      184   zero padding

With --output the tagged copy is written to another file and the input is left
untouched. Running the script on an already-tagged file replaces the metadata
block, so re-running a build is safe.
"""

import argparse
import struct
import sys
import zlib

UF2_MAGIC_START0 = 0x0A324655
UF2_MAGIC_START1 = 0x9E5D5157  # RP2 bootrom magics, pico-sdk boot/uf2.h
UF2_MAGIC_END = 0x0AB16F30
UF2_FLAG_FAMILY_ID_PRESENT = 0x00002000
RP2040_FAMILY_ID = 0xE48BFF56

META_MAGIC = b"CLPFWMET"
META_ADDR = 0x10FFFF00  # last 256 bytes of the 16 MB flash
FLASH_BASE = 0x10000000
IMAGE_MAX_SIZE = 0x800000  # main image region is 8 MB (staging lives above it)

META_FMT = "<8s16s16s16sIII"
META_DATA_SIZE = 256


def fail(msg):
    sys.exit(f"uf2meta: error: {msg}")


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("uf2", help="UF2 file to tag (modified in place)")
    parser.add_argument("--version", required=True, help="firmware version string")
    parser.add_argument("--date", required=True, help="build date, YYYYMMDD")
    parser.add_argument("--git", required=True, help="git short hash")
    parser.add_argument("--output", "-o",
                        help="write the tagged file here instead of modifying the input")
    return parser.parse_args()


def split_blocks(data, path):
    if len(data) == 0 or len(data) % 512:
        fail(f"{path}: size {len(data)} is not a multiple of 512")
    blocks = []
    for i in range(0, len(data), 512):
        header = struct.unpack("<8I", data[i:i + 32])
        end_magic = struct.unpack("<I", data[i + 508:i + 512])[0]
        blocks.append((header, end_magic, data[i:i + 512]))
    return blocks


def block_payload(block):
    return block[2][32:32 + 256]


def drop_tagged_meta(blocks):
    """Strip a previously appended metadata block, if any."""
    if not blocks:
        return blocks
    last = blocks[-1]
    if last[0][3] == META_ADDR and block_payload(last)[:8] == META_MAGIC:
        return blocks[:-1]
    return blocks


def validate_image_blocks(blocks):
    """Validate a plain SDK UF2: contiguous image starting at FLASH_BASE."""
    if not blocks:
        fail("no blocks")
    magic_start1 = blocks[0][0][1]
    magic_end = blocks[0][1]
    for index, (header, end_magic, _) in enumerate(blocks):
        (magic0, magic1, flags, addr, payload_size,
         block_no, num_blocks, family_id) = header
        if magic0 != UF2_MAGIC_START0 or magic1 != magic_start1 or end_magic != magic_end:
            fail(f"block {index}: bad magic")
        if not flags & UF2_FLAG_FAMILY_ID_PRESENT:
            fail(f"block {index}: family ID flag not set")
        if family_id != RP2040_FAMILY_ID:
            fail(f"block {index}: family id {family_id:#x} is not RP2040")
        if payload_size != 256:
            fail(f"block {index}: payload size {payload_size} != 256")
        if block_no != index or num_blocks != len(blocks):
            fail(f"block {index}: block_no {block_no}, num_blocks {num_blocks}")
        if addr != FLASH_BASE + 256 * index:
            fail(f"block {index}: address {addr:#010x} breaks image contiguity")
    payload = b"".join(block_payload(block) for block in blocks)
    if len(payload) > IMAGE_MAX_SIZE:
        fail(f"image payload {len(payload)} bytes exceeds {IMAGE_MAX_SIZE}")
    return magic_start1, magic_end, payload


def make_meta_block(magic_start1, magic_end, payload, version, date, git):
    payload_crc = zlib.crc32(payload) & 0xFFFFFFFF
    meta = struct.pack(META_FMT,
                       META_MAGIC,
                       version.encode()[:16].ljust(16, b"\0"),
                       date.encode()[:16].ljust(16, b"\0"),
                       git.encode()[:16].ljust(16, b"\0"),
                       payload_crc,
                       len(payload),
                       len(payload) // 256)
    header_crc = zlib.crc32(meta) & 0xFFFFFFFF
    meta += struct.pack("<I", header_crc)
    meta += b"\x00" * (META_DATA_SIZE - len(meta))
    block_no = len(payload) // 256
    header = struct.pack("<8I",
                         UF2_MAGIC_START0, magic_start1,
                         UF2_FLAG_FAMILY_ID_PRESENT, META_ADDR,
                         META_DATA_SIZE, block_no, block_no + 1,
                         RP2040_FAMILY_ID)
    data = meta + b"\x00" * (476 - META_DATA_SIZE)
    return header + data + struct.pack("<I", magic_end)


def main():
    args = parse_args()
    with open(args.uf2, "rb") as handle:
        data = handle.read()

    blocks = drop_tagged_meta(split_blocks(data, args.uf2))
    magic_start1, magic_end, payload = validate_image_blocks(blocks)
    meta_block = make_meta_block(magic_start1, magic_end, payload,
                                 args.version, args.date, args.git)

    output = args.output or args.uf2
    with open(output, "wb") as handle:
        handle.write(b"".join(block[2] for block in blocks) + meta_block)

    print(f"uf2meta: tagged {output}: version {args.version}, "
          f"payload {len(payload)} bytes, "
          f"CRC32 {zlib.crc32(payload) & 0xFFFFFFFF:08X}")


if __name__ == "__main__":
    main()
