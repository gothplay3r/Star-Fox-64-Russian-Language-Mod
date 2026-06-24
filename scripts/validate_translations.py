#!/usr/bin/env python3
from __future__ import annotations
import argparse, csv, json, sys
from pathlib import Path
from sf64_encoding import encode_text

REQUIRED = ['id', 'section_id', 'translation_ru']

def main() -> int:
    ap = argparse.ArgumentParser(description='Validate the Russian Language Mod translation workspace.')
    ap.add_argument('csv_in')
    ap.add_argument('--json-report', default='')
    args = ap.parse_args()
    rows = list(csv.DictReader(open(args.csv_in, 'r', encoding='utf-8', newline='')))
    problems = []
    counts = {'rows': len(rows), 'translated': 0, 'empty': 0, 'encoding_errors': 0}
    for field in REQUIRED:
        if rows and field not in rows[0]:
            problems.append({'row': 0, 'field': field, 'error': 'missing required column'})
    for i, row in enumerate(rows, 2):
        text = (row.get('translation_ru') or '').strip()
        if not text:
            counts['empty'] += 1
            continue
        counts['translated'] += 1
        _, errors, _ = encode_text(text, append_end=True)
        if errors:
            counts['encoding_errors'] += 1
            problems.append({'row': i, 'id': row.get('id',''), 'errors': errors})
    report = {'ok': not problems, 'counts': counts, 'problems': problems}
    if args.json_report:
        Path(args.json_report).write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 0 if not problems else 2

if __name__ == '__main__':
    raise SystemExit(main())
