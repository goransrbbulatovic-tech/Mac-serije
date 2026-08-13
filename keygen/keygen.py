#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
============================================================
  Acmigo Indexer - Serija v2.0
  Generator Serijskih Brojeva
  
  VAZNO: SECRET_KEY mora biti ISTI kao u licensemanager.cpp!
============================================================
"""
import hashlib, random, sys, datetime

# !! MORA BITI ISTI KAO U src/licensemanager.cpp !!
SECRET_KEY  = "SI2025-FilmoviSerije-XK7mP9qR"
VALID_CHARS = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"

def compute_checksum(stripped):
    data = stripped[:18] + SECRET_KEY
    h = hashlib.sha256(data.encode('utf-8')).digest()
    return VALID_CHARS[h[0] % len(VALID_CHARS)] + VALID_CHARS[h[1] % len(VALID_CHARS)]

def generate_group():
    data = [random.choice(VALID_CHARS) for _ in range(4)]
    total = sum(VALID_CHARS.index(c) for c in data)
    return ''.join(data) + VALID_CHARS[total % len(VALID_CHARS)]

def generate_serial():
    while True:
        base = ''.join(generate_group() for _ in range(3))
        g4_data = [random.choice(VALID_CHARS) for _ in range(3)]
        prefix18 = base + ''.join(g4_data)
        checksum = compute_checksum(prefix18 + "XX")
        serial = prefix18 + checksum
        if validate_serial(serial):
            return format_serial(serial)

def format_serial(raw):
    r = raw.upper()
    return f"{r[0:5]}-{r[5:10]}-{r[10:15]}-{r[15:20]}"

def validate_serial(serial):
    s = serial.replace('-','').replace(' ','').upper()
    if len(s) != 20: return False
    for c in s:
        if c not in VALID_CHARS: return False
    for g in range(3):
        base = g * 5
        total = sum(VALID_CHARS.index(s[base+i]) for i in range(4))
        if VALID_CHARS[total % len(VALID_CHARS)] != s[base+4]: return False
    checksum = compute_checksum(s)
    return s[18] == checksum[0] and s[19] == checksum[1]

def print_banner():
    print("=" * 55)
    print("  Acmigo Indexer - Generator Serijskih Brojeva")
    print("=" * 55)

def interactive_mode():
    print_banner()
    print("\nOpcije:")
    print("  1. Generiši jedan serijski broj")
    print("  2. Generiši više serijskih brojeva")
    print("  3. Provjeri serijski broj")
    print("  4. Izlaz\n")
    while True:
        choice = input("Odaberite opciju (1-4): ").strip()
        if choice == '1':
            serial = generate_serial()
            print(f"\n✅  Serijski broj: {serial}\n")
        elif choice == '2':
            try:
                n = min(max(int(input("Koliko serijskih brojeva? ").strip()), 1), 1000)
                print(f"\n{'='*45}")
                serials = []
                for i in range(n):
                    s = generate_serial()
                    serials.append(s)
                    print(f"  {i+1:3}. {s}")
                print(f"{'='*45}")
                save = input("\nSačuvati u fajl? (d/n): ").strip().lower()
                if save == 'd':
                    filename = input("Naziv fajla [serials.txt]: ").strip() or "serials.txt"
                    with open(filename, 'w', encoding='utf-8') as f:
                        f.write(f"Acmigo Indexer - Serijski Brojevi\n")
                        f.write(f"Generisano: {datetime.datetime.now()}\n")
                        f.write("=" * 45 + "\n")
                        for s in serials: f.write(s + "\n")
                    print(f"✅  Sačuvano u: {filename}")
                print()
            except ValueError:
                print("❌  Unesite broj!")
        elif choice == '3':
            serial = input("Unesite serijski broj: ").strip()
            if validate_serial(serial):
                print(f"✅  ISPRAVAN: {format_serial(serial.replace('-',''))}")
            else:
                print("❌  NEISPRAVAN serijski broj")
            print()
        elif choice == '4':
            print("Izlaz."); break
        else:
            print("Nepoznata opcija.")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        try:
            count = int(sys.argv[1])
            for _ in range(min(count, 10000)):
                print(generate_serial())
        except ValueError:
            serial = sys.argv[1]
            print("VALID" if validate_serial(serial) else "INVALID")
    else:
        interactive_mode()
