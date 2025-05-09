/* Decode the line number information for UNIT.  */
static struct line_info_table*
decode_line_info (struct comp_unit *unit, struct dwarf2_debug *stash)
{
  bfd *abfd = unit->abfd;
  struct line_info_table* table;
  bfd_byte *line_ptr;
  bfd_byte *line_end;
  struct line_head lh;
  unsigned int i, bytes_read, offset_size;
  char *cur_file, *cur_dir;
  unsigned char op_code, extended_op, adj_opcode;
  unsigned int exop_len;
  bfd_size_type amt;

  if (! read_section (abfd, &stash->debug_sections[debug_line],
		      stash->syms, unit->line_offset,
		      &stash->dwarf_line_buffer, &stash->dwarf_line_size))
    return NULL;

  amt = sizeof (struct line_info_table);
  table = (struct line_info_table *) bfd_alloc (abfd, amt);
  if (table == NULL)
    return NULL;
  table->abfd = abfd;
  table->comp_dir = unit->comp_dir;

  table->num_files = 0;
  table->files = NULL;

  table->num_dirs = 0;
  table->dirs = NULL;

  table->num_sequences = 0;
  table->sequences = NULL;

  table->lcl_head = NULL;

  if (stash->dwarf_line_size < 16)
    {
      _bfd_error_handler
	(_("Dwarf Error: Line info section is too small (%Ld)"),
	 stash->dwarf_line_size);
      bfd_set_error (bfd_error_bad_value);
      return NULL;
    }
  line_ptr = stash->dwarf_line_buffer + unit->line_offset;
  line_end = stash->dwarf_line_buffer + stash->dwarf_line_size;

  /* Read in the prologue.  */
  lh.total_length = read_4_bytes (abfd, line_ptr, line_end);
  line_ptr += 4;
  offset_size = 4;
  if (lh.total_length == 0xffffffff)
    {
      lh.total_length = read_8_bytes (abfd, line_ptr, line_end);
      line_ptr += 8;
      offset_size = 8;
    }
  else if (lh.total_length == 0 && unit->addr_size == 8)
    {
      /* Handle (non-standard) 64-bit DWARF2 formats.  */
      lh.total_length = read_4_bytes (abfd, line_ptr, line_end);
      line_ptr += 4;
      offset_size = 8;
    }

  if (lh.total_length > (size_t) (line_end - line_ptr))
    {
      _bfd_error_handler
	/* xgettext: c-format */
	(_("Dwarf Error: Line info data is bigger (%#Lx)"
	   " than the space remaining in the section (%#lx)"),
	 lh.total_length, (unsigned long) (line_end - line_ptr));
      bfd_set_error (bfd_error_bad_value);
      return NULL;
    }

  line_end = line_ptr + lh.total_length;

  lh.version = read_2_bytes (abfd, line_ptr, line_end);
  if (lh.version < 2 || lh.version > 5)
    {
      _bfd_error_handler
	(_("Dwarf Error: Unhandled .debug_line version %d."), lh.version);
      bfd_set_error (bfd_error_bad_value);
      return NULL;
    }
  line_ptr += 2;

  if (line_ptr + offset_size + (lh.version >= 5 ? 8 : (lh.version >= 4 ? 6 : 5))
      >= line_end)
    {
      _bfd_error_handler
	(_("Dwarf Error: Ran out of room reading prologue"));
      bfd_set_error (bfd_error_bad_value);
      return NULL;
    }

  if (lh.version >= 5)
    {
      unsigned int segment_selector_size;

      /* Skip address size.  */
      read_1_byte (abfd, line_ptr, line_end);
      line_ptr += 1;

      segment_selector_size = read_1_byte (abfd, line_ptr, line_end);
      line_ptr += 1;
      if (segment_selector_size != 0)
	{
	  _bfd_error_handler
	    (_("Dwarf Error: Line info unsupported segment selector size %u."),
	     segment_selector_size);
	  bfd_set_error (bfd_error_bad_value);
	  return NULL;
	}
    }

  if (offset_size == 4)
    lh.prologue_length = read_4_bytes (abfd, line_ptr, line_end);
  else
    lh.prologue_length = read_8_bytes (abfd, line_ptr, line_end);
  line_ptr += offset_size;

  lh.minimum_instruction_length = read_1_byte (abfd, line_ptr, line_end);
  line_ptr += 1;

  if (lh.version >= 4)
    {
      lh.maximum_ops_per_insn = read_1_byte (abfd, line_ptr, line_end);
      line_ptr += 1;
    }
  else
    lh.maximum_ops_per_insn = 1;

  if (lh.maximum_ops_per_insn == 0)
    {
      _bfd_error_handler
	(_("Dwarf Error: Invalid maximum operations per instruction."));
      bfd_set_error (bfd_error_bad_value);
      return NULL;
    }

  lh.default_is_stmt = read_1_byte (abfd, line_ptr, line_end);
  line_ptr += 1;

  lh.line_base = read_1_signed_byte (abfd, line_ptr, line_end);
  line_ptr += 1;

  lh.line_range = read_1_byte (abfd, line_ptr, line_end);
  line_ptr += 1;

  lh.opcode_base = read_1_byte (abfd, line_ptr, line_end);
  line_ptr += 1;

  if (line_ptr + (lh.opcode_base - 1) >= line_end)
    {
      _bfd_error_handler (_("Dwarf Error: Ran out of room reading opcodes"));
      bfd_set_error (bfd_error_bad_value);
      return NULL;
    }

  amt = lh.opcode_base * sizeof (unsigned char);
  lh.standard_opcode_lengths = (unsigned char *) bfd_alloc (abfd, amt);

  lh.standard_opcode_lengths[0] = 1;

  for (i = 1; i < lh.opcode_base; ++i)
    {
      lh.standard_opcode_lengths[i] = read_1_byte (abfd, line_ptr, line_end);
      line_ptr += 1;
    }

  if (lh.version >= 5)
    {
      /* Read directory table.  */
      if (!read_formatted_entries (unit, &line_ptr, line_end, table,
				   line_info_add_include_dir_stub))
	goto fail;

      /* Read file name table.  */
      if (!read_formatted_entries (unit, &line_ptr, line_end, table,
				   line_info_add_file_name))
	goto fail;
    }
  else
    {
      /* Read directory table.  */
      while ((cur_dir = read_string (abfd, line_ptr, line_end, &bytes_read)) != NULL)
	{
	  line_ptr += bytes_read;

	  if (!line_info_add_include_dir (table, cur_dir))
	    goto fail;
	}

      line_ptr += bytes_read;

      /* Read file name table.  */
      while ((cur_file = read_string (abfd, line_ptr, line_end, &bytes_read)) != NULL)
	{
	  unsigned int dir, xtime, size;

	  line_ptr += bytes_read;

	  dir = _bfd_safe_read_leb128 (abfd, line_ptr, &bytes_read, FALSE, line_end);
	  line_ptr += bytes_read;
	  xtime = _bfd_safe_read_leb128 (abfd, line_ptr, &bytes_read, FALSE, line_end);
	  line_ptr += bytes_read;
	  size = _bfd_safe_read_leb128 (abfd, line_ptr, &bytes_read, FALSE, line_end);
	  line_ptr += bytes_read;

	  if (!line_info_add_file_name (table, cur_file, dir, xtime, size))
	    goto fail;
	}

      line_ptr += bytes_read;
    }

  /* Read the statement sequences until there's nothing left.  */
  while (line_ptr < line_end)
    {
      /* State machine registers.  */
      bfd_vma address = 0;
      unsigned char op_index = 0;
      char * filename = table->num_files ? concat_filename (table, 1) : NULL;
      unsigned int line = 1;
      unsigned int column = 0;
      unsigned int discriminator = 0;
      int is_stmt = lh.default_is_stmt;
      int end_sequence = 0;
      /* eraxxon@alumni.rice.edu: Against the DWARF2 specs, some
	 compilers generate address sequences that are wildly out of
	 order using DW_LNE_set_address (e.g. Intel C++ 6.0 compiler
	 for ia64-Linux).  Thus, to determine the low and high
	 address, we must compare on every DW_LNS_copy, etc.  */
      bfd_vma low_pc  = (bfd_vma) -1;
      bfd_vma high_pc = 0;

      /* Decode the table.  */
      while (! end_sequence)
	{
	  op_code = read_1_byte (abfd, line_ptr, line_end);
	  line_ptr += 1;

	  if (op_code >= lh.opcode_base)
	    {
	      /* Special operand.  */
	      adj_opcode = op_code - lh.opcode_base;
	      if (lh.line_range == 0)
		goto line_fail;
	      if (lh.maximum_ops_per_insn == 1)
		address += (adj_opcode / lh.line_range
			    * lh.minimum_instruction_length);
	      else
		{
		  address += ((op_index + adj_opcode / lh.line_range)
			      / lh.maximum_ops_per_insn
			      * lh.minimum_instruction_length);
		  op_index = ((op_index + adj_opcode / lh.line_range)
			      % lh.maximum_ops_per_insn);
		}
	      line += lh.line_base + (adj_opcode % lh.line_range);
	      /* Append row to matrix using current values.  */
	      if (!add_line_info (table, address, op_index, filename,
				  line, column, discriminator, 0))
		goto line_fail;
	      discriminator = 0;
	      if (address < low_pc)
		low_pc = address;
	      if (address > high_pc)
		high_pc = address;
	    }
	  else switch (op_code)
	    {
	    case DW_LNS_extended_op:
	      exop_len = _bfd_safe_read_leb128 (abfd, line_ptr, &bytes_read,
						FALSE, line_end);
	      line_ptr += bytes_read;
	      extended_op = read_1_byte (abfd, line_ptr, line_end);
	      line_ptr += 1;

	      switch (extended_op)
		{
		case DW_LNE_end_sequence:
		  end_sequence = 1;
		  if (!add_line_info (table, address, op_index, filename, line,
				      column, discriminator, end_sequence))
		    goto line_fail;
		  discriminator = 0;
		  if (address < low_pc)
		    low_pc = address;
		  if (address > high_pc)
		    high_pc = address;
		  if (!arange_add (unit, &unit->arange, low_pc, high_pc))
		    goto line_fail;
		  break;
		case DW_LNE_set_address:
		  address = read_address (unit, line_ptr, line_end);
		  op_index = 0;
		  line_ptr += unit->addr_size;
		  break;
		case DW_LNE_define_file:
		  cur_file = read_string (abfd, line_ptr, line_end, &bytes_read);
		  line_ptr += bytes_read;
		  if ((table->num_files % FILE_ALLOC_CHUNK) == 0)
		    {
		      struct fileinfo *tmp;

		      amt = table->num_files + FILE_ALLOC_CHUNK;
		      amt *= sizeof (struct fileinfo);
		      tmp = (struct fileinfo *) bfd_realloc (table->files, amt);
		      if (tmp == NULL)
			goto line_fail;
		      table->files = tmp;
		    }
		  table->files[table->num_files].name = cur_file;
		  table->files[table->num_files].dir =
		    _bfd_safe_read_leb128 (abfd, line_ptr, &bytes_read,
					   FALSE, line_end);
		  line_ptr += bytes_read;
		  table->files[table->num_files].time =
		    _bfd_safe_read_leb128 (abfd, line_ptr, &bytes_read,
					   FALSE, line_end);
		  line_ptr += bytes_read;
		  table->files[table->num_files].size =
		    _bfd_safe_read_leb128 (abfd, line_ptr, &bytes_read,
					   FALSE, line_end);
		  line_ptr += bytes_read;
		  table->num_files++;
		  break;
		case DW_LNE_set_discriminator:
		  discriminator =
		    _bfd_safe_read_leb128 (abfd, line_ptr, &bytes_read,
					   FALSE, line_end);
		  line_ptr += bytes_read;
		  break;
		case DW_LNE_HP_source_file_correlation:
		  line_ptr += exop_len - 1;
		  break;
		default:
		  _bfd_error_handler
		    (_("Dwarf Error: mangled line number section."));
		  bfd_set_error (bfd_error_bad_value);
		line_fail:
		  if (filename != NULL)
		    free (filename);
		  goto fail;
		}
	      break;
	    case DW_LNS_copy:
	      if (!add_line_info (table, address, op_index,
				  filename, line, column, discriminator, 0))
		goto line_fail;
	      discriminator = 0;
	      if (address < low_pc)
		low_pc = address;
	      if (address > high_pc)
		high_pc = address;
	      break;
	    case DW_LNS_advance_pc:
	      if (lh.maximum_ops_per_insn == 1)
		address += (lh.minimum_instruction_length
			    * _bfd_safe_read_leb128 (abfd, line_ptr,
						     &bytes_read,
						     FALSE, line_end));
	      else
		{
		  bfd_vma adjust = _bfd_safe_read_leb128 (abfd, line_ptr,
							  &bytes_read,
							  FALSE, line_end);
		  address = ((op_index + adjust) / lh.maximum_ops_per_insn
			     * lh.minimum_instruction_length);
		  op_index = (op_index + adjust) % lh.maximum_ops_per_insn;
		}
	      line_ptr += bytes_read;
	      break;
	    case DW_LNS_advance_line:
	      line += _bfd_safe_read_leb128 (abfd, line_ptr, &bytes_read,
					     TRUE, line_end);
	      line_ptr += bytes_read;
	      break;
	    case DW_LNS_set_file:
	      {
		unsigned int file;

		/* The file and directory tables are 0
		   based, the references are 1 based.  */
		file = _bfd_safe_read_leb128 (abfd, line_ptr, &bytes_read,
					      FALSE, line_end);
		line_ptr += bytes_read;
		if (filename)
		  free (filename);
		filename = concat_filename (table, file);
		break;
	      }
	    case DW_LNS_set_column:
	      column = _bfd_safe_read_leb128 (abfd, line_ptr, &bytes_read,
					      FALSE, line_end);
	      line_ptr += bytes_read;
	      break;
	    case DW_LNS_negate_stmt:
	      is_stmt = (!is_stmt);
	      break;
	    case DW_LNS_set_basic_block:
	      break;
		  case DW_LNS_const_add_pc:
	      <vul-start>if (lh.maximum_ops_per_insn == 1)<vul-end>
		address += (lh.minimum_instruction_length
			    * ((255 - lh.opcode_base) / lh.line_range));
	      else
		{
		  bfd_vma adjust = ((255 - lh.opcode_base) / lh.line_range);
		  address += (lh.minimum_instruction_length
			      * ((op_index + adjust)
				 / lh.maximum_ops_per_insn));
		  op_index = (op_index + adjust) % lh.maximum_ops_per_insn;
		}
	      break;
	    case DW_LNS_fixed_advance_pc:
	      address += read_2_bytes (abfd, line_ptr, line_end);
	      op_index = 0;
	      line_ptr += 2;
	      break;
	    default:
	      /* Unknown standard opcode, ignore it.  */
	      for (i = 0; i < lh.standard_opcode_lengths[op_code]; i++)
		{
		  (void) _bfd_safe_read_leb128 (abfd, line_ptr, &bytes_read,
						FALSE, line_end);
		  line_ptr += bytes_read;
		}
	      break;
	    }
	}

      if (filename)
	free (filename);
    }

  if (sort_line_sequences (table))
    return table;

 fail:
  if (table->sequences != NULL)
    free (table->sequences);
  if (table->files != NULL)
    free (table->files);
  if (table->dirs != NULL)
    free (table->dirs);
  return NULL;
}