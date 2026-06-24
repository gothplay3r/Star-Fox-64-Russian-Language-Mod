#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import annotations
import argparse, csv, json, sys, time, zipfile
from pathlib import Path

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
RADIO_TOTAL_WORD_CAP = 64          # includes END
RADIO_BODY_WORD_CAP = RADIO_TOTAL_WORD_CAP - 1
RADIO_VISUAL_LINE_CAP = 20
RADIO_VISUAL_LINE_COUNT = 3
MSG_END=0x0000; MSG_NWL=0x0001; MSG_SPC=0x000C; MSG_NXT=0x000F

NON_RADIO_SECTION_PRECLEANES = (
    '000_intro_story',
    '900_items_short_sfx',
)

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
    if any(sid.startswith(x) for x in NON_RADIO_SECTION_PRECLEANES):
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
    # Collapse accidental empty lines for radio; multi-box [NXT] is not part of this route.
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
    text = (row.get('translation_ru') or '').strip()
    if not text:
        return 'EMPTY', 'none', [], [], [], ''
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
        if slot_off is None:
            errors.append({'id': rid_s, 'error': 'slot blob offset missing'})
            continue
        bad = verify_marker(b, int(slot_off), rid, slot_cap, MAGIC_SLOT_A)
        if bad:
            errors.append({'id': rid_s, 'error': f'slot blob marker mismatch got={bad[0]} expected={bad[1]}'})
            continue
        if radio_off is not None:
            bad = verify_marker(b, int(radio_off), rid, radio_cap, MAGIC_RADIO_A)
            if bad:
                errors.append({'id': rid_s, 'error': f'radio blob marker mismatch got={bad[0]} expected={bad[1]}'})
                continue
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
    }

def load_mod_metadata(path: str | Path | None) -> dict:
    default_desc = 'Russian localization.'
    if not path:
        return {
            'display_name': 'Russian Language Mod',
            'authors': ['gothplay3r'],
            'description': default_desc,
            'short_description': default_desc,
            'version': '1.0.0',
            'game_id': 'sf64',
            'id': 'sf64_russian_language_mod',
            'minimum_recomp_version': '1.0.0',
        }
    p = Path(path)
    meta = json.loads(p.read_text(encoding='utf-8'))
    nrm_desc = meta.get('nrm_description') or 'Russian localization.'
    return {
        'display_name': meta.get('display_name', 'Russian Language Mod'),
        'authors': [meta.get('author', 'gothplay3r')],
        'description': nrm_desc,
        'short_description': nrm_desc,
        'version': meta.get('version', '1.0.0'),
        'game_id': meta.get('game_id', 'sf64'),
        'id': meta.get('mod_id', 'sf64_russian_language_mod'),
        'minimum_recomp_version': meta.get('minimum_recomp_version', '1.0.0'),
    }

def patch_nrm(template_nrm: Path, manifest_path: Path, csv_path: Path, out_nrm: Path, metadata_path: str | Path | None = None, thumb_path: str | Path | None = None) -> dict:
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    rows = load_rows(csv_path)
    release_meta = load_mod_metadata(metadata_path)
    out_nrm.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(template_nrm,'r') as zin:
        mod_binary = zin.read('mod_binary.bin')
        patched, report = patch_binary(mod_binary, manifest, rows)
        with zipfile.ZipFile(out_nrm,'w',compression=zipfile.ZIP_DEFLATED) as zout:
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
                zout.writestr(info, data)
            if thumb_path:
                tp = Path(thumb_path)
                if tp.exists():
                    zout.writestr('thumb.png', tp.read_bytes())
    report['output_nrm'] = str(out_nrm)
    report['runtime_nrm_metadata'] = release_meta
    return report

def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description='Build Russian Language Mod .nrm from the runtime template and translation workspace.');
    ap.add_argument('--csv', required=True)
    ap.add_argument('--template-nrm', required=True)
    ap.add_argument('--manifest', required=True)
    ap.add_argument('--out-nrm', required=True)
    ap.add_argument('--metadata', default='')
    ap.add_argument('--report-json')
    args=ap.parse_args(argv)
    rep = patch_nrm(Path(args.template_nrm), Path(args.manifest), Path(args.csv), Path(args.out_nrm), args.metadata or None, Path(args.metadata).resolve().parents[0] / 'assets' / 'icon.png' if args.metadata else None)
    if args.report_json:
        Path(args.report_json).write_text(json.dumps(rep, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(rep, ensure_ascii=False, indent=2))
    return 0 if rep.get('error_count',0)==0 else 2
if __name__ == '__main__':
    raise SystemExit(main())
