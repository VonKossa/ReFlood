#!/usr/bin/env python3
"""Prepare ReFlood's external data tree from a user-supplied Flood ADF."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import sys
import tempfile


ADF_BYTES = 901_120
TRACK_BYTES = 0x1600
SUPPORTED_ADF_SHA256 = {
    "58532a66f2f78900ac52722c5c6c0a6fee3993aefcf69a05c4cb6d2277dcfffe":
        "Flood, SPS release 504 revision 1 (standard ADF conversion)",
}

RUN_PROG_SLICES = {
    "ending/ENDING_palette.bin": (0xCF28, 32),
    "hud/HUD_glyphs.bin": (0xB4CA, 896),
    "hud/HUD_template.bin": (0xAD0E, 320),
    "presentation/HIGHSCORE_names.bin": (0xE978, 60),
    "presentation/HIGHSCORE_scores.bin": (0xE964, 20),
    "presentation/INTRO_events.bin": (0x2404, 3280),
    "presentation/INTRO_palette.bin": (0x20DE, 64),
    "presentation/LEVEL_correct.bin": (0xEC9E, 20),
    "presentation/LEVEL_font.bin": (0xB4CA, 960),
    "presentation/LEVEL_incorrect.bin": (0xECB3, 20),
    "presentation/LEVEL_palette.bin": (0xEABC, 32),
    "presentation/LEVEL_template.bin": (0xEBA2, 126),
    "presentation/LEVEL_zap.bin": (0xEC20, 126),
    "presentation/PROTECTION_challenges_xor.bin": (0x79A0, 5920),
    "presentation/PROTECTION_flags.bin": (0x915C, 96),
    "presentation/PROTECTION_input.bin": (0x90C0, 14),
    "presentation/PROTECTION_messages.bin": (0x111C, 367),
    "presentation/PROTECTION_palette.bin": (0xE9FC, 32),
    "presentation/TITLE_palette.bin": (0xEAFC, 64),
}


class ExtractionError(Exception):
    pass


def be16(data: bytes, offset: int = 0) -> int:
    return struct.unpack_from(">H", data, offset)[0]


def be32(data: bytes, offset: int = 0) -> int:
    return struct.unpack_from(">I", data, offset)[0]


def parse_disk_directory(adf: bytes) -> dict[str, bytes]:
    if adf[0x1600:0x1604] != b"DIR\0":
        raise ExtractionError("Flood disk directory signature is missing")

    entries: dict[str, bytes] = {}
    for offset in range(0x1610, 0x17C0, 16):
        record = adf[offset:offset + 16]
        name = record[:8].split(b"\0", 1)[0].decode("ascii")
        if not name:
            break
        first_track = be16(record, 8)
        last_track = be16(record, 10)
        size = be32(record, 12)
        start = first_track * TRACK_BYTES
        end = start + size
        if first_track > last_track or end > len(adf):
            raise ExtractionError(f"invalid disk-directory entry: {name}")
        entries[name] = adf[start:end]
    return entries


def unpack_member(data: bytes, offset: int = 0) -> tuple[bytes, int]:
    if offset + 8 > len(data):
        raise ExtractionError("truncated packed-resource header")
    packed_size = be32(data, offset)
    unpacked_size = be32(data, offset + 4)
    end = offset + packed_size
    if packed_size < 8 or end > len(data):
        raise ExtractionError("invalid packed-resource size")

    source = offset + 8
    output = bytearray()
    while len(output) < unpacked_size:
        if source + 2 > end:
            raise ExtractionError("truncated packed-resource command")
        command = struct.unpack_from(">h", data, source)[0]
        source += 2
        if command >= 0:
            count = command + 1
            if source + count > end or len(output) + count > unpacked_size:
                raise ExtractionError("invalid packed-resource literal")
            output.extend(data[source:source + count])
            source += count
        else:
            count = 1 - command
            if source + 2 > end or len(output) + count > unpacked_size:
                raise ExtractionError("invalid packed-resource copy")
            copy_from = be16(data, source)
            source += 2
            if copy_from >= len(output):
                raise ExtractionError("invalid packed-resource copy offset")
            for _ in range(count):
                if copy_from >= len(output):
                    raise ExtractionError("invalid overlapping copy offset")
                output.append(output[copy_from])
                copy_from += 1

    if source != end:
        raise ExtractionError("packed-resource member has unused bytes")
    return bytes(output), packed_size


def write_file(root: Path, relative: str, data: bytes) -> None:
    target = root / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)


def reconstruct(entries: dict[str, bytes], output: Path) -> None:
    required = {
        "RUN_PROG", "TRACK.DA", "INSTR.DA", "BIG_PIC.", "SPR_16.B",
        "SPR_32.B", "BLOCKA.B", "BLOCKB.B", "BLOCKC.B", "W_BLOCKS",
        "TITLE.SC", "HSCORE.D", "TEMPFILE", "FLDFX.DU", "FLDFX.IN",
        "END_SCRE",
    }
    missing = sorted(required - entries.keys())
    if missing:
        raise ExtractionError("missing disk entries: " + ", ".join(missing))

    run_prog = entries["RUN_PROG"]
    for relative, (start, size) in RUN_PROG_SLICES.items():
        write_file(output, relative, run_prog[start:start + size])

    # The password table stores 48 four-character codes separated by NULs.
    password_source = run_prog[0x96:0x186]
    passwords = password_source.split(b"\0")
    if len(passwords) != 49 or passwords[-1] or any(len(x) != 4 for x in passwords[:-1]):
        raise ExtractionError("unexpected level-password table")
    write_file(output, "presentation/LEVEL_passwords.bin", b"".join(passwords))

    messages = run_prog[0x111C:0x111C + 367]
    offsets = []
    cursor = 0
    for _ in range(16):
        offsets.append(cursor)
        try:
            cursor = messages.index(0, cursor) + 1
        except ValueError as exc:
            raise ExtractionError("unexpected protection-message table") from exc
    if cursor != len(messages):
        raise ExtractionError("unexpected protection-message table length")
    write_file(output, "presentation/PROTECTION_message_offsets.bin",
               b"".join(struct.pack(">H", x) for x in offsets))

    direct_entries = {
        "presentation/BIG_PIC.bin": "BIG_PIC.",
        "sprites/SPR_32.B": "SPR_32.B",
        "audio/FLDFX_DU.bin": "FLDFX.DU",
        "audio/FLDFX_IN.bin": "FLDFX.IN",
        "ending/END_SCRE.bin": "END_SCRE",
    }
    for relative, name in direct_entries.items():
        write_file(output, relative, entries[name])

    instrument = entries["INSTR.DA"]
    instrument_size = be32(instrument) + 4
    write_file(output, "audio/INSTR_DA.bin", instrument[:instrument_size])

    packed_entries = {
        "audio/TRACK_DA.bin": "TRACK.DA",
        "sprites/SPR_16_unpacked.bin": "SPR_16.B",
        "water/W_BLOCKS.bin": "W_BLOCKS",
        "presentation/TITLE_SCR.bin": "TITLE.SC",
        "presentation/HSCORE_DAT.bin": "HSCORE.D",
        "presentation/TEMPFILE_SCR.bin": "TEMPFILE",
    }
    for relative, name in packed_entries.items():
        unpacked, used = unpack_member(entries[name])
        if used != len(entries[name]):
            raise ExtractionError(f"unexpected trailing bytes in {name}")
        write_file(output, relative, unpacked)

    for bank in "ABC":
        unpacked, used = unpack_member(entries[f"BLOCK{bank}.B"])
        if used != len(entries[f"BLOCK{bank}.B"]) or len(unpacked) != 33024:
            raise ExtractionError(f"unexpected BLOCK{bank}.B layout")
        write_file(output, f"blocks/BLOCK{bank}_tiles.bin", unpacked[:32768])
        write_file(output, f"blocks/BLOCK{bank}_attrs.bin", unpacked[32768:])

    for first_level in range(1, 42, 4):
        name = f"MAP{first_level:03}.D"
        packed = entries.get(name)
        if packed is None:
            raise ExtractionError(f"missing disk entry: {name}")
        offset = 0
        member_count = min(4, 43 - first_level)
        for member in range(member_count):
            unpacked, used = unpack_member(packed, offset)
            offset += used
            level = first_level + member
            if len(unpacked) != 13456:
                raise ExtractionError(f"unexpected level {level} layout")
            tilemap = unpacked[:12800]
            header = unpacked[12800:12816]
            trigger_payload = unpacked[12816:]
            trigger_bytes = be16(header, 2) * 10
            if trigger_bytes > len(trigger_payload):
                raise ExtractionError(f"invalid level {level} trigger count")
            prefix = f"levels/level_{level:02}_"
            write_file(output, prefix + "header.bin", header)
            write_file(output, prefix + "tilemap.bin", tilemap)
            write_file(output, prefix + "trigger_payload.bin", trigger_payload)
            write_file(output, prefix + "triggers.bin", trigger_payload[:trigger_bytes])
        if offset != len(packed):
            raise ExtractionError(f"unexpected trailing bytes in {name}")


def load_manifest(script_dir: Path) -> dict[str, dict[str, object]]:
    path = script_dir / "flood_data_manifest.json"
    try:
        manifest = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ExtractionError(f"cannot read {path}: {exc}") from exc
    files = manifest.get("files")
    if not isinstance(files, dict):
        raise ExtractionError("invalid extraction manifest")
    return files


def verify_tree(root: Path, manifest: dict[str, dict[str, object]]) -> None:
    actual = {str(path.relative_to(root)) for path in root.rglob("*") if path.is_file()}
    expected = set(manifest)
    if actual != expected:
        missing = sorted(expected - actual)
        extra = sorted(actual - expected)
        raise ExtractionError(f"output file-set mismatch; missing={missing}, extra={extra}")
    for relative, record in manifest.items():
        data = (root / relative).read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        if len(data) != record["size"] or digest != record["sha256"]:
            raise ExtractionError(f"verification failed: {relative}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Prepare ReFlood's ./data directory from a supported Flood ADF")
    parser.add_argument("adf", type=Path, help="ADF converted from an original Flood IPF")
    parser.add_argument("output", nargs="?", type=Path, default=Path("data"),
                        help="output directory (default: ./data)")
    args = parser.parse_args()

    try:
        disk = args.adf.read_bytes()
        if len(disk) < ADF_BYTES:
            raise ExtractionError(f"ADF is too short: expected at least {ADF_BYTES} bytes")
        standard_adf = disk[:ADF_BYTES]
        digest = hashlib.sha256(standard_adf).hexdigest()
        revision = SUPPORTED_ADF_SHA256.get(digest)
        if revision is None:
            raise ExtractionError(f"unsupported ADF (SHA-256 {digest})")
        destination = args.output.resolve()
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination_was_empty = False
        if destination.exists():
            if destination.is_symlink() or not destination.is_dir():
                raise ExtractionError(f"output is not a directory: {destination}")
            if any(destination.iterdir()):
                raise ExtractionError(f"output directory is not empty: {destination}")
            destination_was_empty = True
        manifest = load_manifest(Path(__file__).resolve().parent)
        temporary = Path(tempfile.mkdtemp(prefix=f".{destination.name}.extracting-",
                                          dir=destination.parent))
        try:
            reconstruct(parse_disk_directory(standard_adf), temporary)
            verify_tree(temporary, manifest)
            if destination_was_empty:
                destination.rmdir()
            os.replace(temporary, destination)
        except Exception:
            shutil.rmtree(temporary, ignore_errors=True)
            if destination_was_empty and not destination.exists():
                destination.mkdir()
            raise
        print(f"Recognized: {revision}")
        print(f"Created and verified {len(manifest)} files in {destination}")
        return 0
    except (OSError, ExtractionError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
