#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import annotations
import argparse, csv, json, sys, time, zipfile
from pathlib import Path

try:
    from starlis_release_manifest import load_release_manifest, apply_runtime_metadata
except Exception as e:
    load_release_manifest = None
    apply_runtime_metadata = None
    RELEASE_MANIFEST_IMPORT_ERROR = str(e)
else:
    RELEASE_MANIFEST_IMPORT_ERROR = ""

# Allow import when script is copied into project/scripts.
def _add_script_dir():
    here = Path(__file__).resolve().parent
    for p in [here, here.parent / 'scripts']:
        if str(p) not in sys.path:
            sys.path.insert(0, str(p))
_add_script_dir()
try:
    from sf64_encoding import encode_text, analyse_text
except Exception as e:
    encode_text = None
    analyse_text = None
    IMPORT_ERROR = str(e)
else:
    IMPORT_ERROR = ''

HEADER_WORDS = 8
MAGIC_SLOT_A = 0x5255  # RU
MAGIC_RADIO_A = 0x5252 # RR = radio route
MAGIC_B = 0x3634
TAIL = 0xCAFE
RADIO_TOTAL_WORD_CAP = 64          # ordinary radio storage safety cap, includes END
RADIO_BODY_WORD_CAP = RADIO_TOTAL_WORD_CAP - 1
RADIO_VISUAL_LINE_CAP = 20
RADIO_VISUAL_LINE_COUNT = 3
MAP_BRIEFING_FREE_TOTAL_WORD_CAP = 128  # FIX45 manual map-briefing cap, includes END
MAP_BRIEFING_FREE_BODY_WORD_CAP = MAP_BRIEFING_FREE_TOTAL_WORD_CAP - 1
MAP_BRIEFING_FREE_VISUAL_LINE_CAP = 40
MAP_BRIEFING_FREE_VISUAL_LINE_COUNT = 4
MSG_END=0x0000; MSG_NWL=0x0001; MSG_SPC=0x000C; MSG_NXT=0x000F

NON_RADIO_SECTION_PREFIXES = (
    '000_intro_story',
    '900_items_short_sfx',
)

def is_truthy(v) -> bool:
    return str(v or '').strip().lower() in ('1', 'true', 'yes', 'y', 'on', 'да')

def is_map_briefing_row(row: dict) -> bool:
    sid = (row.get('section_id') or '').strip()
    block = (row.get('block_id_v12') or '').strip()
    return sid == '020_route_map_briefings' or block == '020_route_map_briefings'

def map_briefing_free_enabled(row: dict) -> bool:
    return is_map_briefing_row(row) and is_truthy(row.get('map_briefing_free_mode'))

def parse_u16_hex(s: str) -> list[int]:
    out=[]
    for part in (s or '').replace(',', ' ').split():
        if not part.strip():
            continue
        out.append(int(part,16) & 0xFFFF)
    return out

def load_rows(path: Path) -> list[dict]:
    with path.open('r', encoding='utf-8', newline='') as f:
        return list(csv.DictReader(f))

def is_radio_route_row(row: dict) -> bool:
    sid = (row.get('section_id') or '').strip()
    if not sid:
        return False
    if any(sid.startswith(x) for x in NON_RADIO_SECTION_PREFIXES):
        return False
    return True

def visible_len_code(code: int) -> int:
    # Mirrors Message_GetWidth enough for radio fitting: text glyphs and space consume one slot.
    if code == MSG_END or code == MSG_NWL or code == MSG_NXT:
        return 0
    if code == MSG_SPC:
        return 1
    if code >= 0x0010:
        return 1
    return 0

def radio_line_stats(codes_no_end: list[int]) -> dict:
    lines=[]; cur=0
    for c in codes_no_end:
        if c in (MSG_NWL, MSG_NXT, MSG_END):
            lines.append(cur); cur=0
        else:
            cur += visible_len_code(c)
    lines.append(cur)
    # Drop trailing completely empty line caused by a final newline before END.
    while len(lines) > 1 and lines[-1] == 0:
        lines.pop()
    return {'line_lengths': lines, 'line_count': len(lines), 'max_line': max(lines) if lines else 0}

def radio_wrap_text(text: str, max_line: int = RADIO_VISUAL_LINE_CAP, max_lines: int = RADIO_VISUAL_LINE_COUNT) -> str:
    """Simple user-text wrapper for radio windows.

    It intentionally does not expose Star Fox control tokens to the translator.
    Existing manual newlines are preserved as paragraph breaks, then each line is greedily word-wrapped.
    """
    text = (text or '').replace('\r\n','\n').replace('\r','\n').strip()
    if not text:
        return text
    out=[]
    for raw_line in text.split('\n'):
        line = raw_line.strip()
        if not line:
            if out and out[-1] != '': out.append('')
            continue
        words=line.split()
        cur=''
        for w in words:
            if not cur:
                cur=w
            elif len(cur) + 1 + len(w) <= max_line:
                cur += ' ' + w
            else:
                out.append(cur)
                cur=w
        if cur:
            out.append(cur)
    # Collapse accidental empty lines for radio; multi-box [NXT] is not part of this lab route.
    out=[x for x in out if x.strip()]
    if len(out) > max_lines:
        return '\n'.join(out)  # Leave it untrimmed; validator will reject with clear reason.
    return '\n'.join(out)

def encode_plain(text: str, append_end: bool=True) -> tuple[list[int], list[str], list[str]]:
    if encode_text is None:
        return [], [f'Cannot import sf64_encoding: {IMPORT_ERROR}'], []
    codes, errors, warnings = encode_text(text, append_end=append_end)
    return codes, errors, warnings

def encode_row_route(row: dict) -> tuple[str, str, list[int], list[str], list[str], str]:
    """Return status, route, values-with-END, errors, warnings, patched_text."""
    raw_text = row.get('translation_ru') or ''
    if not raw_text.strip():
        return 'EMPTY', 'none', [], [], [], ''
    text = raw_text
    try:
        slot = int(float(row.get('slot_capacity_words') or row.get('word_count') or 0))
    except Exception:
        slot = 0
    if encode_text is None:
        return 'FAIL_INTERNAL', 'none', [], [f'Cannot import sf64_encoding: {IMPORT_ERROR}'], [], text

    slot_codes, slot_errors, slot_warnings = encode_plain(text, append_end=True)
    if slot_errors:
        return 'FAIL_UNSUPPORTED', 'none', slot_codes, slot_errors, slot_warnings, text
    if slot and len(slot_codes) <= slot:
        return 'READY_SLOT', 'slot_copy', slot_codes, [], slot_warnings, text

    if not is_radio_route_row(row):
        warnings = list(slot_warnings)
        warnings.append('does not fit original slot and this row is not enabled for radio overflow')
        return 'NEEDS_MANUAL_ROUTE', 'none', slot_codes, [], warnings, text

    if map_briefing_free_enabled(row):
        # FIX45: map briefing manual mode. No word wrap, no strip/split.
        # The translator controls line breaks and leading spaces. This still uses
        # the radio-overflow blob at runtime, but only briefing rows are allowed
        # to exceed the old ordinary-radio 64-word safety cap.
        wrapped = text.replace('\r\n', '\n').replace('\r', '\n')
        radio_codes_no_end, radio_errors, radio_warnings = encode_plain(wrapped, append_end=False)
        if radio_errors:
            return 'FAIL_UNSUPPORTED', 'none', radio_codes_no_end, radio_errors, radio_warnings, wrapped
        stats = radio_line_stats(radio_codes_no_end)
        warnings = list(dict.fromkeys(radio_warnings))
        if len(radio_codes_no_end) > MAP_BRIEFING_FREE_BODY_WORD_CAP:
            warnings.append(f'map briefing free body too long: {len(radio_codes_no_end)}/{MAP_BRIEFING_FREE_BODY_WORD_CAP} u16 before END')
            return 'MAP_BRIEFING_FREE_TOO_LONG', 'none', radio_codes_no_end + [MSG_END], [], warnings, wrapped
        if stats['line_count'] > MAP_BRIEFING_FREE_VISUAL_LINE_COUNT:
            warnings.append(f'map briefing free has many lines: {stats["line_count"]}/{MAP_BRIEFING_FREE_VISUAL_LINE_COUNT}; build allowed, visual test required')
        if stats['max_line'] > MAP_BRIEFING_FREE_VISUAL_LINE_CAP:
            warnings.append(f'map briefing free visual line long: {stats["max_line"]}/{MAP_BRIEFING_FREE_VISUAL_LINE_CAP}; build allowed, visual test required')
        vals = radio_codes_no_end + [MSG_END]
        warnings.append(f'map briefing free route: manual newlines/spaces preserved, line lengths {stats["line_lengths"]}')
        return 'READY_RADIO_OVERFLOW', 'radio_overflow', vals, [], warnings, wrapped

    wrapped = radio_wrap_text(text)
    radio_codes_no_end, radio_errors, radio_warnings = encode_plain(wrapped, append_end=False)
    if radio_errors:
        return 'FAIL_UNSUPPORTED', 'none', radio_codes_no_end, radio_errors, radio_warnings, wrapped
    stats = radio_line_stats(radio_codes_no_end)
    warnings = list(dict.fromkeys(radio_warnings))
    if len(radio_codes_no_end) > RADIO_BODY_WORD_CAP:
        warnings.append(f'radio overflow body too long: {len(radio_codes_no_end)}/{RADIO_BODY_WORD_CAP} u16 before END')
        return 'RADIO_OVERFLOW_TOO_LONG', 'none', radio_codes_no_end + [MSG_END], [], warnings, wrapped
    if stats['line_count'] > RADIO_VISUAL_LINE_COUNT:
        warnings.append(f'radio overflow has too many lines: {stats["line_count"]}/{RADIO_VISUAL_LINE_COUNT}')
        return 'RADIO_OVERFLOW_TOO_TALL', 'none', radio_codes_no_end + [MSG_END], [], warnings, wrapped
    if stats['max_line'] > RADIO_VISUAL_LINE_CAP:
        warnings.append(f'radio overflow visual line too long: {stats["max_line"]}/{RADIO_VISUAL_LINE_CAP}')
        return 'RADIO_OVERFLOW_LINE_TOO_LONG', 'none', radio_codes_no_end + [MSG_END], [], warnings, wrapped
    vals = radio_codes_no_end + [MSG_END]
    warnings.append(f'radio overflow route: auto-wrapped to {stats["line_count"]} line(s), line lengths {stats["line_lengths"]}')
    if len(vals) >= RADIO_TOTAL_WORD_CAP - 1:
        warnings.append('radio overflow is at/near absolute 64-word storage cap; regression-test following radio boxes in-game')
    return 'READY_RADIO_OVERFLOW', 'radio_overflow', vals, [], warnings, wrapped

def write_blob_disabled(b: bytearray, off: int, cap: int) -> None:
    b[off+10:off+12] = (0).to_bytes(2,'big')
    b[off+12:off+14] = (0).to_bytes(2,'big')
    data_off = off + HEADER_WORDS*2
    b[data_off:data_off+cap*2] = b'\x00' * (cap*2)

def verify_marker(b: bytearray, off: int, rid: int, cap: int, magic_a: int) -> list[int]:
    expected = [magic_a, MAGIC_B, (rid>>16)&0xFFFF, rid&0xFFFF, cap]
    got = [int.from_bytes(b[off+i*2:off+i*2+2], 'big') for i in range(5)]
    return [] if got == expected and int.from_bytes(b[off+14:off+16], 'big') == TAIL else [got, expected]

def scan_blob_markers(b: bytearray, magic_a: int) -> dict[int, tuple[int, int]]:
    """Find runtime text blobs by their embedded RU/RR marker.

    Source rebuilding through RecompModTool can move mod_binary offsets even when the
    blob order/content is unchanged. The manifest remains useful metadata, but the
    builder must not trust stale offsets blindly. This scan keeps old and newly
    rebuilt templates patchable without hand-editing 779 offsets like a cave goblin.
    """
    out: dict[int, tuple[int, int]] = {}
    for off in range(0, max(0, len(b) - HEADER_WORDS * 2 + 1), 2):
        vals = [int.from_bytes(b[off+i*2:off+i*2+2], 'big') for i in range(HEADER_WORDS)]
        if vals[0] == magic_a and vals[1] == MAGIC_B and vals[7] == TAIL:
            rid = ((vals[2] & 0xFFFF) << 16) | (vals[3] & 0xFFFF)
            cap = vals[4] & 0xFFFF
            out[rid] = (off, cap)
    return out

def resolve_blob_offset(b: bytearray, item: dict, rid: int, manifest_off: int | None, manifest_cap: int, magic_a: int, scanned: dict[int, tuple[int, int]], errors: list[dict], label: str) -> tuple[int | None, int]:
    """Return a verified blob offset/capacity, using marker scan as fallback."""
    if manifest_off is not None and not verify_marker(b, int(manifest_off), rid, manifest_cap, magic_a):
        return int(manifest_off), manifest_cap
    found = scanned.get(rid)
    if found is not None:
        off, cap = found
        if cap != manifest_cap:
            # Prefer the real embedded capacity from the current binary, but report it.
            errors.append({'id': rid, 'warning': f'{label} capacity came from marker scan: manifest={manifest_cap} binary={cap}'})
        return off, cap
    if manifest_off is None:
        errors.append({'id': rid, 'error': f'{label} blob offset missing and marker scan did not find id'})
    else:
        bad = verify_marker(b, int(manifest_off), rid, manifest_cap, magic_a)
        errors.append({'id': rid, 'error': f'{label} blob marker mismatch got={bad[0] if bad else None} expected={bad[1] if bad else None}; marker scan did not find id'})
    return None, manifest_cap

def write_blob_enabled(b: bytearray, off: int, cap: int, vals: list[int]) -> None:
    if not vals or vals[-1] != MSG_END:
        vals = vals + [MSG_END]
    if len(vals) > cap:
        raise ValueError(f'encoded words {len(vals)} exceed blob capacity {cap}')
    b[off+10:off+12] = (1).to_bytes(2,'big')
    b[off+12:off+14] = (len(vals)).to_bytes(2,'big')
    data_off = off + HEADER_WORDS*2
    b[data_off:data_off+cap*2] = b'\x00' * (cap*2)
    for i,v in enumerate(vals):
        b[data_off+i*2:data_off+i*2+2] = int(v).to_bytes(2,'big')

def patch_binary(mod_binary: bytes, manifest: dict, rows: list[dict]) -> tuple[bytes, dict]:
    b = bytearray(mod_binary)
    items = {str(int(item['id'])): item for item in manifest['items']}
    scanned_slots = scan_blob_markers(b, MAGIC_SLOT_A)
    scanned_radios = scan_blob_markers(b, MAGIC_RADIO_A)
    included=[]; skipped=[]; errors=[]
    for row in rows:
        rid_s = str(row.get('id','')).strip()
        if not rid_s or not rid_s.isdigit():
            continue
        rid = int(rid_s)
        item = items.get(str(rid))
        if not item:
            errors.append({'id': rid_s, 'error': 'id not found in runtime manifest'})
            continue
        slot_cap = int(item.get('capacity_words') or row.get('slot_capacity_words') or row.get('word_count') or 0)
        slot_off = item.get('slot_mod_binary_offset', item.get('mod_binary_offset'))
        radio_cap = int(item.get('radio_capacity_words') or RADIO_TOTAL_WORD_CAP)
        radio_off = item.get('radio_mod_binary_offset')
        slot_off, slot_cap = resolve_blob_offset(b, item, rid, slot_off, slot_cap, MAGIC_SLOT_A, scanned_slots, errors, 'slot')
        if slot_off is None:
            continue
        radio_off, radio_cap = resolve_blob_offset(b, item, rid, radio_off, radio_cap, MAGIC_RADIO_A, scanned_radios, errors, 'radio')
        # Always start by disabling both routes. Non-empty valid rows then enable exactly one route.
        write_blob_disabled(b, int(slot_off), slot_cap)
        if radio_off is not None:
            write_blob_disabled(b, int(radio_off), radio_cap)
        if not (row.get('translation_ru') or '').strip():
            continue
        status, route, vals, enc_errors, warnings, patched_text = encode_row_route(row)
        if enc_errors:
            skipped.append({'id': rid_s, 'status': status, 'route': route, 'errors': enc_errors, 'warnings': warnings})
            continue
        try:
            if route == 'slot_copy' and status == 'READY_SLOT':
                write_blob_enabled(b, int(slot_off), slot_cap, vals)
                included.append({'id': rid_s, 'route': route, 'words': len(vals), 'capacity': slot_cap, 'symbol': row.get('symbol',''), 'section_id': row.get('section_id','')})
            elif route == 'radio_overflow' and status == 'READY_RADIO_OVERFLOW':
                if radio_off is None:
                    skipped.append({'id': rid_s, 'status': status, 'route': route, 'errors': ['radio blob offset missing'], 'warnings': warnings})
                    continue
                write_blob_enabled(b, int(radio_off), radio_cap, vals)
                included.append({'id': rid_s, 'route': route, 'words': len(vals), 'capacity': radio_cap, 'slot_capacity': slot_cap, 'symbol': row.get('symbol',''), 'section_id': row.get('section_id',''), 'patched_text': patched_text, 'warnings': warnings})
            else:
                skipped.append({'id': rid_s, 'status': status, 'route': route, 'encoded_words': len(vals), 'slot_capacity': slot_cap, 'radio_capacity': radio_cap, 'warnings': warnings})
        except Exception as e:
            errors.append({'id': rid_s, 'route': route, 'error': str(e)})
    return bytes(b), {
        'included': included, 'skipped': skipped, 'errors': errors,
        'included_count': len(included), 'skipped_count': len(skipped), 'error_count': len(errors),
        'slot_count': sum(1 for x in included if x.get('route') == 'slot_copy'),
        'radio_overflow_count': sum(1 for x in included if x.get('route') == 'radio_overflow'),
        'radio_total_word_cap': RADIO_TOTAL_WORD_CAP,
        'radio_body_word_cap': RADIO_BODY_WORD_CAP,
        'radio_visual_line_cap': RADIO_VISUAL_LINE_CAP,
        'radio_visual_line_count': RADIO_VISUAL_LINE_COUNT,
        'map_briefing_free_total_word_cap': MAP_BRIEFING_FREE_TOTAL_WORD_CAP,
        'map_briefing_free_body_word_cap': MAP_BRIEFING_FREE_BODY_WORD_CAP,
        'map_briefing_free_visual_line_cap': MAP_BRIEFING_FREE_VISUAL_LINE_CAP,
        'map_briefing_free_visual_line_count': MAP_BRIEFING_FREE_VISUAL_LINE_COUNT,
    }

NRM_MANIFEST_DISPLAY_NAME = 'Russian Language Mod'
NRM_MANIFEST_AUTHORS = ['gothplay3r']
NRM_MANIFEST_DESCRIPTION = 'Russian localization.'
NRM_MANIFEST_VERSION = '1.0.0'

def _load_release_metadata(release_manifest_path: str | Path | None = None) -> tuple[dict, str, list[str], dict]:
    warnings=[]
    if load_release_manifest is None or apply_runtime_metadata is None:
        warnings.append(f'release manifest helper import failed, using built-in fallback: {RELEASE_MANIFEST_IMPORT_ERROR}')
        return {
            'display_name': NRM_MANIFEST_DISPLAY_NAME,
            'authors': NRM_MANIFEST_AUTHORS,
            'description': NRM_MANIFEST_DESCRIPTION,
            'short_description': NRM_MANIFEST_DESCRIPTION,
            'version': NRM_MANIFEST_VERSION,
        }, '', warnings, {}
    root = Path(__file__).resolve().parents[1]
    rel = load_release_manifest(root=root, manifest_path=release_manifest_path)
    meta = apply_runtime_metadata({}, rel)
    return meta, str(rel.get('_manifest_path', '')), warnings, rel


def _release_thumb_path(release_manifest: dict) -> Path | None:
    if not release_manifest:
        return None
    root = Path(__file__).resolve().parents[1]
    assets = release_manifest.get('release_assets') or {}
    candidates = [assets.get('thumb_png'), assets.get('icon_png'), (release_manifest.get('thunderstore') or {}).get('icon_file')]
    for rel in candidates:
        if not rel:
            continue
        p = Path(str(rel))
        if not p.is_absolute():
            p = root / p
        if p.exists() and p.is_file():
            return p
    return None

def patch_nrm(template_nrm: Path, manifest_path: Path, csv_path: Path, out_nrm: Path, display_name: str | None = None, release_manifest_path: str | Path | None = None, metadata_path: str | Path | None = None, thumb_path: str | Path | None = None) -> dict:
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    rows = load_rows(csv_path)
    release_meta, release_manifest_loaded_from, release_manifest_warnings, release_manifest = _load_release_metadata(release_manifest_path)
    if metadata_path:
        try:
            m = json.loads(Path(metadata_path).read_text(encoding='utf-8'))
            release_meta.update({'id': m.get('mod_id'), 'version': m.get('version'), 'display_name': m.get('display_name'), 'description': m.get('nrm_description') or 'Russian localization.', 'short_description': m.get('nrm_description') or 'Russian localization.', 'authors': [m.get('author') or 'gothplay3r'], 'game_id': m.get('game_id') or 'sf64', 'minimum_recomp_version': m.get('minimum_recomp_version') or '1.0.0'})
            release_meta = {k: v for k, v in release_meta.items() if v is not None}
        except Exception as e:
            release_manifest_warnings.append(f'metadata override failed: {e}')
    if display_name and not release_manifest_loaded_from:
        release_meta['display_name'] = display_name
    out_nrm.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(template_nrm,'r') as zin:
        mod_binary = zin.read('mod_binary.bin')
        patched, report = patch_binary(mod_binary, manifest, rows)
        with zipfile.ZipFile(out_nrm,'w',compression=zipfile.ZIP_DEFLATED) as zout:
            existing_names = set()
            for info in zin.infolist():
                data = zin.read(info.filename)
                if info.filename == 'mod_binary.bin':
                    data = patched
                elif info.filename == 'mod.json':
                    try:
                        meta = json.loads(data.decode('utf-8'))
                        meta.update({k: v for k, v in release_meta.items() if v is not None})
                        data = json.dumps(meta, ensure_ascii=False, indent=4).encode('utf-8')
                    except Exception:
                        pass
                existing_names.add(info.filename)
                zout.writestr(info, data)
            thumb_path = Path(thumb_path) if thumb_path else _release_thumb_path(release_manifest)
            if thumb_path is not None and 'thumb.png' not in existing_names and 'thumb.dds' not in existing_names:
                zout.write(thumb_path, 'thumb.png')
                report['thumb_embedded'] = True
                report['thumb_source'] = str(thumb_path)
            else:
                report['thumb_embedded'] = bool('thumb.png' in existing_names or 'thumb.dds' in existing_names)
                report['thumb_source'] = str(thumb_path) if thumb_path is not None else ''
    report['output_nrm'] = str(out_nrm)
    report['release_manifest_path'] = release_manifest_loaded_from
    report['release_manifest_warnings'] = release_manifest_warnings
    report['runtime_nrm_metadata'] = release_meta
    return report

def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description='Patch prebuilt SF64 RU runtime template .nrm without clang/make/RecompModTool. Supports slot-copy and radio-only overflow.');
    ap.add_argument('--csv', required=True)
    ap.add_argument('--template-nrm', required=True)
    ap.add_argument('--manifest', required=True)
    ap.add_argument('--out-nrm', required=True)
    ap.add_argument('--display-name', default=NRM_MANIFEST_DISPLAY_NAME)
    ap.add_argument('--release-manifest', default='')
    ap.add_argument('--report-json')
    args=ap.parse_args(argv)
    rep = patch_nrm(Path(args.template_nrm), Path(args.manifest), Path(args.csv), Path(args.out_nrm), args.display_name, args.release_manifest or None)
    if args.report_json:
        Path(args.report_json).write_text(json.dumps(rep, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(rep, ensure_ascii=False, indent=2))
    return 0 if rep.get('error_count',0)==0 else 2
if __name__ == '__main__':
    raise SystemExit(main())
