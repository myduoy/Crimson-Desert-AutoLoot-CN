from pathlib import Path
import os
import re
import struct


ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "src" / "main.cpp").read_text(encoding="utf-8")
DEFAULT_EXE = Path(r"D:\soft\Steam\steamapps\common\Crimson Desert\bin64\CrimsonDesert.exe")
EXE = Path(os.environ.get("CRIMSON_DESERT_EXE", str(DEFAULT_EXE)))


def source_const(name: str) -> int:
    match = re.search(rf"{re.escape(name)}\s*=\s*(0x[0-9A-Fa-f]+|\d+)", SOURCE)
    assert match, f"{name} is missing"
    return int(match.group(1), 0)


def pe_timestamp(data: bytes) -> int:
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    return struct.unpack_from("<I", data, pe_offset + 8)[0]


def pe_sections(data: bytes) -> list[tuple[int, int, int, int]]:
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    section_count = struct.unpack_from("<H", data, pe_offset + 6)[0]
    optional_header_size = struct.unpack_from("<H", data, pe_offset + 20)[0]
    section_offset = pe_offset + 24 + optional_header_size
    sections = []
    for index in range(section_count):
        offset = section_offset + index * 40
        virtual_size, virtual_address, raw_size, raw_address = struct.unpack_from(
            "<IIII", data, offset + 8
        )
        sections.append((virtual_address, virtual_size, raw_address, raw_size))
    return sections


def raw_to_rva(data: bytes, raw_offset: int) -> int:
    for virtual_address, virtual_size, raw_address, raw_size in pe_sections(data):
        size = max(virtual_size, raw_size)
        if raw_address <= raw_offset < raw_address + size:
            return virtual_address + (raw_offset - raw_address)
    raise AssertionError(f"raw offset 0x{raw_offset:X} is outside PE sections")


def find_text_a(data: bytes) -> int:
    prefix = bytes.fromhex(
        "41 0F B6 4D 3A 49 8B 45 30 4D 8D 86 80 01 00 00 "
        "88 4C 24 20 4C 8D 0D"
    )
    suffix = bytes.fromhex("48 8B 10 48 8B CF E8")
    start = 0
    hits = []
    while True:
        index = data.find(prefix, start)
        if index < 0:
            break
        if data[index + 27:index + 34] == suffix:
            hits.append(index)
        start = index + 1
    assert len(hits) == 1, f"expected one prompt text A signature, got {len(hits)}"
    return hits[0]


def find_text_b(data: bytes) -> int:
    prefix = bytes.fromhex("49 8B 45 40 41 B0 01 48 8B 10 48 8B CF E8")
    start = 0
    hits = []
    while True:
        index = data.find(prefix, start)
        if index < 0:
            break
        hits.append(index)
        start = index + 1
    assert len(hits) == 1, f"expected one prompt text B signature, got {len(hits)}"
    return hits[0]


def assert_branch_at(data: bytes, rva: int) -> None:
    assert data[rva:rva + 6] == bytes.fromhex("40 80 FE 02 0F 84")
    assert data[rva + 10:rva + 20] == bytes.fromhex("41 80 BE 8E 01 00 00 00 0F 85")


def rel32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<i", data, offset)[0]


def test_source_hook_constants_match_installed_game() -> None:
    assert EXE.exists(), f"game exe not found: {EXE}"
    data = EXE.read_bytes()

    text_a_raw = find_text_a(data)
    text_b_raw = find_text_b(data)
    branch_raw = text_a_raw + (source_const("kPromptBranchRva") - source_const("kPromptTextAEntryRva"))
    assert_branch_at(data, branch_raw)
    text_a = raw_to_rva(data, text_a_raw)
    text_b = raw_to_rva(data, text_b_raw)
    branch = raw_to_rva(data, branch_raw)

    assert source_const("kSupportedBuildTimestamp") == pe_timestamp(data)
    assert source_const("kPromptTextAEntryRva") == text_a
    assert source_const("kPromptTextBEntryRva") == text_b
    assert source_const("kPromptBranchRva") == branch
    assert source_const("kPromptTextAReturnRva") == text_a + source_const("kPromptTextAPatchLen")
    assert source_const("kPromptTextBReturnRva") == text_b + source_const("kPromptTextBPatchLen")
    assert source_const("kOriginalContinueRva") == branch + 0x18
    assert source_const("kSkipPromptRva") == branch + 10 + rel32(data, branch_raw + 6)
    assert source_const("kPromptTextALiteralRva") == text_a + 27 + rel32(data, text_a_raw + 23)
    assert source_const("kPromptTextACallRva") == text_a + 38 + rel32(data, text_a_raw + 34)
    assert source_const("kPromptTextBCallRva") == text_b + 18 + rel32(data, text_b_raw + 14)


if __name__ == "__main__":
    test_source_hook_constants_match_installed_game()
    print("current game hook constants test passed")
