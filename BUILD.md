# Build

Требуется Python 3.10 или новее.

Сборка готового `.nrm` и Thunderstore ZIP:

```bash
python3 scripts/build_package.py
```

Результат будет создан в папке `dist/`:

```text
Russian_Language_Mod_v1_0_0.nrm
gothplay3r-Russian_Language_Mod-1.0.0.zip
```

`mod.toml`, `Makefile`, `src/`, `include/` и `Starfox64RecompSyms/` оставлены в репозитории как исходники runtime-мода. Скрипт сборки релиза использует готовый runtime template и патчит в него текущий перевод из `data/translation_workspace.csv`.
