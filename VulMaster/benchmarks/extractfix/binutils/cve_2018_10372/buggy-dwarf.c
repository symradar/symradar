/* Process a CU or TU index.  If DO_DISPLAY is true, print the contents.
   These sections are extensions for Fission.
   See http://gcc.gnu.org/wiki/DebugFissionDWP.  */
static int
process_cu_tu_index (struct dwarf_section *section, int do_display)
{
  unsigned char *phdr = section->start;
  unsigned char *limit = phdr + section->size;
  unsigned char *phash;
  unsigned char *pindex;
  unsigned char *ppool;
  unsigned int version;
  unsigned int ncols = 0;
  unsigned int nused;
  unsigned int nslots;
  unsigned int i;
  unsigned int j;
  dwarf_vma signature_high;
  dwarf_vma signature_low;
  char buf[64];

  /* PR 17512: file: 002-168123-0.004.  */
  if (phdr == NULL)
    {
      warn (_("Section %s is empty\n"), section->name);
      return 0;
    }
  /* PR 17512: file: 002-376-0.004.  */
  if (section->size < 24)
    {
      warn (_("Section %s is too small to contain a CU/TU header\n"),
	    section->name);
      return 0;
    }

  SAFE_BYTE_GET (version, phdr, 4, limit);
  if (version >= 2)
    SAFE_BYTE_GET (ncols, phdr + 4, 4, limit);
  SAFE_BYTE_GET (nused, phdr + 8, 4, limit);
  SAFE_BYTE_GET (nslots, phdr + 12, 4, limit);

  phash = phdr + 16;
  pindex = phash + nslots * 8;
  ppool = pindex + nslots * 4;

  /* PR 17531: file: 45d69832.  */
  if (pindex < phash || ppool < phdr || (pindex == phash && nslots != 0))
    {
      warn (ngettext ("Section %s is too small for %d slot\n",
		      "Section %s is too small for %d slots\n",
		      nslots),
	    section->name, nslots);
      return 0;
    }

  if (do_display)
    {
      introduce (section, FALSE);

      printf (_("  Version:                 %d\n"), version);
      if (version >= 2)
	printf (_("  Number of columns:       %d\n"), ncols);
      printf (_("  Number of used entries:  %d\n"), nused);
      printf (_("  Number of slots:         %d\n\n"), nslots);
    }

  if (ppool > limit || ppool < phdr)
    {
      warn (_("Section %s too small for %d hash table entries\n"),
	    section->name, nslots);
      return 0;
    }

  if (version == 1)
    {
      if (!do_display)
	prealloc_cu_tu_list ((limit - ppool) / 4);
      for (i = 0; i < nslots; i++)
	{
	  unsigned char *shndx_list;
	  unsigned int shndx;

	  SAFE_BYTE_GET64 (phash, &signature_high, &signature_low, limit);
	  if (signature_high != 0 || signature_low != 0)
	    {
	      SAFE_BYTE_GET (j, pindex, 4, limit);
	      shndx_list = ppool + j * 4;
	      /* PR 17531: file: 705e010d.  */
	      if (shndx_list < ppool)
		{
		  warn (_("Section index pool located before start of section\n"));
		  return 0;
		}

	      if (do_display)
		printf (_("  [%3d] Signature:  0x%s  Sections: "),
			i, dwarf_vmatoa64 (signature_high, signature_low,
					   buf, sizeof (buf)));
	      for (;;)
		{
		  if (shndx_list >= limit)
		    {
		      warn (_("Section %s too small for shndx pool\n"),
			    section->name);
		      return 0;
		    }
		  SAFE_BYTE_GET (shndx, shndx_list, 4, limit);
		  if (shndx == 0)
		    break;
		  if (do_display)
		    printf (" %d", shndx);
		  else
		    add_shndx_to_cu_tu_entry (shndx);
		  shndx_list += 4;
		}
	      if (do_display)
		printf ("\n");
	      else
		end_cu_tu_entry ();
	    }
	  phash += 8;
	  pindex += 4;
	}
    }
  else if (version == 2)
    {
      unsigned int val;
      unsigned int dw_sect;
      unsigned char *ph = phash;
      unsigned char *pi = pindex;
      unsigned char *poffsets = ppool + ncols * 4;
      unsigned char *psizes = poffsets + nused * ncols * 4;
      unsigned char *pend = psizes + nused * ncols * 4;
      bfd_boolean is_tu_index;
      struct cu_tu_set *this_set = NULL;
      unsigned int row;
      unsigned char *prow;

      is_tu_index = strcmp (section->name, ".debug_tu_index") == 0;

      /* PR 17531: file: 0dd159bf.
	 Check for wraparound with an overlarge ncols value.  */
      if (poffsets < ppool || (unsigned int) ((poffsets - ppool) / 4) != ncols)
	{
	  warn (_("Overlarge number of columns: %x\n"), ncols);
	  return 0;
	}

      if (pend > limit)
	{
	  warn (_("Section %s too small for offset and size tables\n"),
		section->name);
	  return 0;
	}

      if (do_display)
	{
	  printf (_("  Offset table\n"));
	  printf ("  slot  %-16s  ",
		 is_tu_index ? _("signature") : _("dwo_id"));
	}
      else
	{
	  if (is_tu_index)
	    {
	      tu_count = nused;
	      tu_sets = xcalloc2 (nused, sizeof (struct cu_tu_set));
	      this_set = tu_sets;
	    }
	  else
	    {
	      cu_count = nused;
	      cu_sets = xcalloc2 (nused, sizeof (struct cu_tu_set));
	      this_set = cu_sets;
	    }
	}

      if (do_display)
	{
	  for (j = 0; j < ncols; j++)
	    {
	      SAFE_BYTE_GET (dw_sect, ppool + j * 4, 4, limit);
	      printf (" %8s", get_DW_SECT_short_name (dw_sect));
	    }
	  printf ("\n");
	}

      for (i = 0; i < nslots; i++)
	{
	  SAFE_BYTE_GET64 (ph, &signature_high, &signature_low, limit);

	  SAFE_BYTE_GET (row, pi, 4, limit);
	  if (row != 0)
	    {
	      /* PR 17531: file: a05f6ab3.  */
	      if (row > nused)
		{
		  warn (_("Row index (%u) is larger than number of used entries (%u)\n"),
			row, nused);
		  return 0;
		}

		if (!do_display) {
			<vul-start>memcpy (&this_set[row - 1].signature, ph, sizeof (uint64_t));<vul-end>
		  }

	      prow = poffsets + (row - 1) * ncols * 4;
	      /* PR 17531: file: b8ce60a8.  */
	      if (prow < poffsets || prow > limit)
		{
		  warn (_("Row index (%u) * num columns (%u) > space remaining in section\n"),
			row, ncols);
		  return 0;
		}

	      if (do_display)
		printf (_("  [%3d] 0x%s"),
			i, dwarf_vmatoa64 (signature_high, signature_low,
					   buf, sizeof (buf)));
	      for (j = 0; j < ncols; j++)
		{
		  SAFE_BYTE_GET (val, prow + j * 4, 4, limit);
		  if (do_display)
		    printf (" %8d", val);
		  else
		    {
		      SAFE_BYTE_GET (dw_sect, ppool + j * 4, 4, limit);

		      /* PR 17531: file: 10796eb3.  */
		      if (dw_sect >= DW_SECT_MAX)
			warn (_("Overlarge Dwarf section index detected: %u\n"), dw_sect);
		      else
			this_set [row - 1].section_offsets [dw_sect] = val;
		    }
		}

	      if (do_display)
		printf ("\n");
	    }
	  ph += 8;
	  pi += 4;
	}

      ph = phash;
      pi = pindex;
      if (do_display)
	{
	  printf ("\n");
	  printf (_("  Size table\n"));
	  printf ("  slot  %-16s  ",
		 is_tu_index ? _("signature") : _("dwo_id"));
	}

      for (j = 0; j < ncols; j++)
	{
	  SAFE_BYTE_GET (val, ppool + j * 4, 4, limit);
	  if (do_display)
	    printf (" %8s", get_DW_SECT_short_name (val));
	}

      if (do_display)
	printf ("\n");

      for (i = 0; i < nslots; i++)
	{
	  SAFE_BYTE_GET64 (ph, &signature_high, &signature_low, limit);

	  SAFE_BYTE_GET (row, pi, 4, limit);
	  if (row != 0)
	    {
	      prow = psizes + (row - 1) * ncols * 4;

	      if (do_display)
		printf (_("  [%3d] 0x%s"),
			i, dwarf_vmatoa64 (signature_high, signature_low,
					   buf, sizeof (buf)));

	      for (j = 0; j < ncols; j++)
		{
		  SAFE_BYTE_GET (val, prow + j * 4, 4, limit);
		  if (do_display)
		    printf (" %8d", val);
		  else
		    {
		      SAFE_BYTE_GET (dw_sect, ppool + j * 4, 4, limit);
		      if (dw_sect >= DW_SECT_MAX)
			warn (_("Overlarge Dwarf section index detected: %u\n"), dw_sect);
		      else
		      this_set [row - 1].section_sizes [dw_sect] = val;
		    }
		}

	      if (do_display)
		printf ("\n");
	    }

	  ph += 8;
	  pi += 4;
	}
    }
  else if (do_display)
    printf (_("  Unsupported version (%d)\n"), version);

  if (do_display)
      printf ("\n");

  return 1;
}