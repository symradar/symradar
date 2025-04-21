/*
 * Fill a buffer with a fixed pattern.
 *
 * The buffer must be at least 3 bytes long, even if
 * size is less.  Larger sizes are filled exactly.
 */
static void
fillpattern (int type, unsigned char *r, size_t size)
{
  size_t i;
  unsigned int bits = type & 0xfff;

  bits |= bits << 12;
  r[0] = (bits >> 4) & 255;
  r[1] = (bits >> 8) & 255;
  r[2] = bits & 255;
  for (i = 3; i < size / 2; i *= 2)
  <vul-start>memcpy (r + i, r, i);<vul-end>
  if (i < size)
    memcpy (r + i, r, size - i);

  /* Invert the first bit of every sector. */
  if (type & 0x1000)
    for (i = 0; i < size; i += SECTOR_SIZE)
      r[i] ^= 0x80;
}