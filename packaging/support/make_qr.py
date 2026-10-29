#!/usr/bin/env python3
"""Generates the QR codes shown in the "Support Svitlo" dialog.

Edit the values below if the donation details change, then run:
    python3 packaging/support/make_qr.py      (needs: pip install qrcode pillow)
"""
import os
import qrcode

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "resources", "support")

ITEMS = {
    # BIP21 URI: wallets that scan it open a pre-filled Bitcoin send screen.
    "qr-bitcoin.png": "bitcoin:bc1qcqw855rmkvqn0zryfwecfkukwhspdy6rqlnjux",
    "qr-kofi.png": "https://ko-fi.com/dermeister",
}

os.makedirs(OUT, exist_ok=True)
for name, data in ITEMS.items():
    qr = qrcode.QRCode(error_correction=qrcode.constants.ERROR_CORRECT_M, box_size=10, border=2)
    qr.add_data(data)
    qr.make(fit=True)
    qr.make_image(fill_color="black", back_color="white").save(os.path.join(OUT, name))
    print("wrote", name, "->", data)
