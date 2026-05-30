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


def source_byte_array(name: str) -> bytes:
    match = re.search(
        rf"const\s+uint8_t\s+{re.escape(name)}\[\]\s*=\s*\{{([^}}]+)\}};",
        SOURCE,
        re.S,
    )
    assert match, f"{name} byte array is missing"
    return bytes(int(value, 16) for value in re.findall(r"0x[0-9A-Fa-f]{2}", match.group(1)))


def source_function_byte_array(function_name: str, array_name: str) -> bytes:
    function_start = SOURCE.index(f"bool {function_name}()")
    match = re.search(
        rf"const\s+uint8_t\s+{re.escape(array_name)}\[\]\s*=\s*\{{([^}}]+)\}};",
        SOURCE[function_start:],
        re.S,
    )
    assert match, f"{array_name} byte array is missing in {function_name}"
    return bytes(int(value, 16) for value in re.findall(r"0x[0-9A-Fa-f]{2}", match.group(1)))


def source_function_body(function_name: str) -> str:
    marker = f"{function_name}("
    start = -1
    while True:
        start = SOURCE.find(marker, start + 1)
        assert start != -1, f"{function_name} is missing"
        brace = SOURCE.find("{", start)
        assert brace != -1, f"{function_name} has no body"
        semicolon = SOURCE.find(";", start, brace)
        if semicolon == -1:
            break
    depth = 0
    for index in range(brace, len(SOURCE)):
        char = SOURCE[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return SOURCE[start:index + 1]
    raise AssertionError(f"{function_name} body is unterminated")


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


def rva_to_raw(data: bytes, rva: int) -> int:
    for virtual_address, virtual_size, raw_address, raw_size in pe_sections(data):
        size = max(virtual_size, raw_size)
        if virtual_address <= rva < virtual_address + size:
            return raw_address + (rva - virtual_address)
    raise AssertionError(f"rva 0x{rva:X} is outside PE sections")


def find_text_a(data: bytes) -> int:
    prefix = bytes.fromhex("41 0F B6 4D 3A 49 8B 45 30")
    lea_opcodes = (bytes.fromhex("4D 8D"), bytes.fromhex("4C 8D"))
    owner_modrms = {0x86, 0x87}
    lea_disp = bytes.fromhex("80 01 00 00")
    store_and_literal = bytes.fromhex("88 4C 24 20 4C 8D 0D")
    suffix = bytes.fromhex("48 8B 10 48 8B CF E8")
    start = 0
    hits = []
    while True:
        index = data.find(prefix, start)
        if index < 0:
            break
        if (
            data[index + 9:index + 11] in lea_opcodes
            and data[index + 11] in owner_modrms
            and data[index + 12:index + 16] == lea_disp
            and data[index + 16:index + 23] == store_and_literal
            and data[index + 27:index + 34] == suffix
        ):
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


def find_prompt_update(data: bytes, text_a_raw: int) -> int:
    prologue = bytes.fromhex("48 89 54 24 10 55 53 56 57 41 54 41 55 41 56")
    search_start = max(0, text_a_raw - 0x1200)
    hits = []
    start = search_start
    while True:
        index = data.find(prologue, start, text_a_raw)
        if index < 0:
            break
        hits.append(index)
        start = index + 1
    assert len(hits) == 1, f"expected one prompt update prologue before text A, got {len(hits)}"
    return hits[0]


def find_prompt_branch(data: bytes, text_a_raw: int) -> int:
    first_compares = (
        bytes.fromhex("40 80 FE 02 0F 84"),
        bytes.fromhex("40 80 FD 02 0F 84"),
        bytes.fromhex("41 80 FE 02 0F 84"),
    )
    second_compares = (
        bytes.fromhex("41 80 BE 8E 01 00 00 00 0F 85"),
        bytes.fromhex("41 80 BF 8E 01 00 00 00 0F 85"),
    )
    hits = []
    search_start = max(0, text_a_raw - 0x2000)
    search_end = text_a_raw + 0x400
    for first_compare in first_compares:
        start = search_start
        while True:
            index = data.find(first_compare, start, search_end)
            if index < 0:
                break
            if data[index + 10:index + 20] in second_compares:
                hits.append(index)
            start = index + 1
    assert len(hits) == 1, f"expected one prompt branch signature, got {len(hits)}"
    return hits[0]


def assert_branch_stub_type_capture_matches_game(data: bytes, branch_raw: int) -> None:
    body = source_function_body("BuildPromptBranchStub")
    if data[branch_raw:branch_raw + 4] == bytes.fromhex("41 80 FE 02"):
        assert "0x44, 0x89, 0xF1" in body, "current branch type is r14b; stub must copy r14d into ecx"
        assert "0x41, 0x0F, 0xB7, 0x4D, 0x10" not in body, "current branch stub must not read type from [r13+10h]"
    elif data[branch_raw:branch_raw + 4] == bytes.fromhex("40 80 FE 02"):
        assert "0x40, 0x0F, 0xB6, 0xCE" in body, "sil branch type must be copied into ecx"
    elif data[branch_raw:branch_raw + 4] == bytes.fromhex("40 80 FD 02"):
        assert "0x40, 0x0F, 0xB6, 0xCD" in body, "bpl branch type must be copied into ecx"
    else:
        raise AssertionError(f"unrecognized prompt branch type compare at 0x{branch_raw:X}")


def assert_branch_hook_uses_direct_near_jumps() -> None:
    stub = source_function_body("BuildPromptBranchStub")
    installer = source_function_body("InstallPromptBranchHook")
    assert "EmitRelJumpToAddress" in stub
    assert "EmitAbsJump(code, g_game + kSkipPromptRva)" not in stub
    assert "EmitAbsJump(code, g_game + kOriginalContinueRva)" not in stub
    assert "AllocateNear" in installer
    assert "patch[0] = 0xE9" in installer
    assert "0xFF" not in installer.split("std::vector<uint8_t> patch", 1)[1].split("VirtualProtect", 1)[0]


def assert_worker_uses_resolver_hook_not_branch_patch() -> None:
    worker = source_function_body("WorkerThread")
    assert "InstallPromptResolverHook()" in worker
    assert "InstallPromptBranchHook()" not in worker


def branch_instruction_block_length(data: bytes, branch_raw: int) -> int:
    assert data[branch_raw:branch_raw + 4] in (
        bytes.fromhex("40 80 FE 02"),
        bytes.fromhex("40 80 FD 02"),
        bytes.fromhex("41 80 FE 02"),
    )
    assert data[branch_raw + 4:branch_raw + 6] == bytes.fromhex("0F 84")
    assert data[branch_raw + 10:branch_raw + 18] in (
        bytes.fromhex("41 80 BE 8E 01 00 00 00"),
        bytes.fromhex("41 80 BF 8E 01 00 00 00"),
    )
    assert data[branch_raw + 18:branch_raw + 20] == bytes.fromhex("0F 85")
    return 24


def rel32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<i", data, offset)[0]


def test_source_hook_constants_match_installed_game() -> None:
    assert EXE.exists(), f"game exe not found: {EXE}"
    data = EXE.read_bytes()

    text_a_raw = find_text_a(data)
    text_b_raw = find_text_b(data)
    prompt_update_raw = find_prompt_update(data, text_a_raw)
    branch_raw = find_prompt_branch(data, text_a_raw)
    prompt_update = raw_to_rva(data, prompt_update_raw)
    text_a = raw_to_rva(data, text_a_raw)
    text_b = raw_to_rva(data, text_b_raw)
    branch = raw_to_rva(data, branch_raw)
    resolver_call_raws = [
        offset
        for offset in range(max(0, branch_raw - 24), branch_raw)
        if data[offset] == 0xE8
    ]
    assert resolver_call_raws, "expected resolver call before prompt branch"
    resolver_call_raw = resolver_call_raws[-1]
    resolver = raw_to_rva(data, resolver_call_raw) + 5 + rel32(data, resolver_call_raw + 1)
    resolver_raw = rva_to_raw(data, resolver)
    assert data[resolver_raw] == 0xE9
    resolver_target = resolver + 5 + rel32(data, resolver_raw + 1)

    assert source_const("kSupportedBuildTimestamp") == pe_timestamp(data)
    assert source_const("kTypeResolverThunkRva") == resolver
    assert source_const("kTypeResolverTargetRva") == resolver_target
    assert source_const("kPromptUpdateEntryRva") == prompt_update
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
    assert source_byte_array("expected_a") == data[text_a_raw:text_a_raw + source_const("kPromptTextAPatchLen")]
    assert source_byte_array("expected_b") == data[text_b_raw:text_b_raw + source_const("kPromptTextBPatchLen")]
    assert source_function_byte_array("InstallPromptUpdateHook", "expected") == data[
        prompt_update_raw:prompt_update_raw + source_const("kPromptUpdatePatchLen")
    ]
    assert source_function_byte_array("InstallPromptResolverHook", "expected") == data[
        resolver_raw:resolver_raw + source_const("kTypeResolverPatchLen")
    ]
    assert source_const("kPatchLen") == branch_instruction_block_length(data, branch_raw)
    assert source_function_byte_array("InstallPromptBranchHook", "expected") == data[
        branch_raw:branch_raw + source_const("kPatchLen")
    ]
    assert_branch_stub_type_capture_matches_game(data, branch_raw)
    assert_branch_hook_uses_direct_near_jumps()
    assert_worker_uses_resolver_hook_not_branch_patch()


if __name__ == "__main__":
    test_source_hook_constants_match_installed_game()
    print("current game hook constants test passed")
