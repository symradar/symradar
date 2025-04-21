#!/usr/bin/env python3

def convert(h):
  byte_array = bytearray.fromhex(h.upper())
  decimal_value = int.from_bytes(byte_array, byteorder='little')
  print(decimal_value)

def convert_to_hex(d):
  hex_value = hex(int(d))
  print(hex_value)

def reverse_endian(h):
  byte_array = bytearray.fromhex(h.upper())
  byte_array.reverse()
  hex_value = ''.join(format(x, '02x') for x in byte_array)
  print(hex_value)

while True:
  hex_value = input("Enter a hex value(d for decimal): ").strip()
  if hex_value == "d":
    dec_value = input("Enter a decimal value: ").strip()
    convert_to_hex(dec_value)
  elif hex_value == "r":
    hex_value = input("Enter a hex value: ").strip()
    reverse_endian(hex_value)
  else:
    convert(hex_value)