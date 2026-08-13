# Acmigo Indexer - Serija v2.0

Program za upravljanje kolekcijom serija sa IMDB integracijom.

---

## Instalacija — Windows
1. Preuzmi `AcmigoIndexer_v2.0_Setup.exe` sa Releases
2. Instaliraj i pokreni
3. Unesi serijski broj pri prvom pokretanju

## Instalacija — macOS
1. Preuzmi `AcmigoIndexer_v2.0_macOS.dmg`
2. Otvori DMG fajl
3. Prevuci **AcmigoIndexer** u Applications folder
4. Pokreni iz Applications

### ⚠ macOS upozorenje "unidentified developer"
macOS može blokirati app jer nije plaćeno potpisana. Zaobiđi ovako:

**Način 1 — Desni klik:**
- Desni klik na AcmigoIndexer.app → **Open** → **Open**
- Pitaće te jednom, nakon toga se više ne pojavljuje

**Način 2 — Terminal:**
```bash
xattr -cr /Applications/AcmigoIndexer.app
```
Pokreni u Terminalu, zatim otvori app normalno.

**Način 3 — System Settings:**
- System Settings → Privacy & Security
- Skrolaj dolje → vidješ poruku o AcmigoIndexer
- Klikni **"Open Anyway"**

---

## Instalacija — Linux
```bash
tar -xzf AcmigoIndexer_Linux_x64.tar.gz
chmod +x AcmigoIndexer_Linux_x64
./AcmigoIndexer_Linux_x64
```

---

## Serijski broj
Generiši sa:
```
cd keygen
python keygen.py
```
Dvoklikom na `keygen.bat` (Windows) ili `python keygen.py` (Mac/Linux)

---

## IMDB ocjene
- Idi na Podešavanja → unesi besplatan OMDb API ključ
- Registracija: https://www.omdbapi.com/apikey.aspx (besplatno, 1000/dan)
- Klikni "IMDB Sve" za batch preuzimanje svih ocjena

---

## Baza podataka — backup
- **Windows:** `%LOCALAPPDATA%\AcmigoIndexer\series.db`
- **macOS:** `~/Library/Application Support/AcmigoIndexer/series.db`
- **Linux:** `~/.local/share/AcmigoIndexer/series.db`

Kopiraj taj fajl za backup — deinstalacija ga ne briše.

---

## Secret Key
`src/licensemanager.cpp` i `keygen/keygen.py` moraju imati isti SECRET_KEY!
