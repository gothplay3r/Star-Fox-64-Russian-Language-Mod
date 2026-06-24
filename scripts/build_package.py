#!/usr/bin/env python3
from __future__ import annotations
import json, shutil, zipfile, sys, re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / 'dist'
RAW_BASE = 'https://raw.githubusercontent.com/gothplay3r/Star-Fox-64-Russian-Language-Mod/main'


def load_metadata() -> dict:
    return json.loads((ROOT / 'metadata.json').read_text(encoding='utf-8'))


def thunderstore_readme() -> str:
    p = ROOT / 'README_THUNDERSTORE.md'
    if p.exists():
        return p.read_text(encoding='utf-8')
    text = (ROOT / 'README.md').read_text(encoding='utf-8')
    return re.sub(r'\(screenshots/(\d+\.png)\)', lambda m: f'({RAW_BASE}/screenshots/{m.group(1)})', text)


def build_nrm(meta: dict) -> Path:
    sys.path.insert(0, str(ROOT / 'scripts'))
    from patch_runtime_nrm import patch_nrm
    out = DIST / meta.get('nrm_filename', 'Russian_Language_Mod_v1_0_0.nrm')
    report = patch_nrm(
        ROOT / 'runtime/Russian_Language_Mod_template.nrm',
        ROOT / 'runtime/runtime_blob_manifest.json',
        ROOT / 'data/translation_workspace.csv',
        out,
        metadata_path=ROOT / 'metadata.json',
        thumb_path=ROOT / 'thumb.png',
    )
    if report.get('error_count', 0):
        raise SystemExit(json.dumps(report, ensure_ascii=False, indent=2))
    return out


def build_package(meta: dict, nrm_path: Path) -> Path:
    staging = DIST / 'package_staging'
    if staging.exists():
        shutil.rmtree(staging)
    staging.mkdir(parents=True)
    package_manifest = {
        'name': meta['name'],
        'version_number': meta['version'],
        'website_url': meta.get('website_url', ''),
        'description': meta['description'],
        'dependencies': meta.get('dependencies', []),
    }
    (staging / 'manifest.json').write_text(json.dumps(package_manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    (staging / 'README.md').write_text(thunderstore_readme(), encoding='utf-8')
    shutil.copy2(ROOT / 'icon.png', staging / 'icon.png')
    shutil.copy2(nrm_path, staging / nrm_path.name)
    out = DIST / meta.get('package_filename', 'gothplay3r-Russian_Language_Mod-1.0.0.zip')
    if out.exists():
        out.unlink()
    with zipfile.ZipFile(out, 'w', compression=zipfile.ZIP_DEFLATED) as z:
        for p in sorted(staging.rglob('*')):
            if p.is_file():
                z.write(p, p.relative_to(staging).as_posix())
    return out


def main() -> int:
    DIST.mkdir(exist_ok=True)
    meta = load_metadata()
    nrm = build_nrm(meta)
    package = build_package(meta, nrm)
    print(json.dumps({'ok': True, 'nrm': str(nrm), 'package': str(package)}, ensure_ascii=False, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
