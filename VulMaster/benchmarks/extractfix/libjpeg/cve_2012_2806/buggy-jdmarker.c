LOCAL(boolean)
get_sos (j_decompress_ptr cinfo)
/* Process a SOS marker */
{
  INT32 length;
  int i, ci, n, c, cc;
  jpeg_component_info * compptr;
  INPUT_VARS(cinfo);

  if (! cinfo->marker->saw_SOF)
    ERREXIT(cinfo, JERR_SOS_NO_SOF);

  INPUT_2BYTES(cinfo, length, return FALSE);

  INPUT_BYTE(cinfo, n, return FALSE); /* Number of components */

  TRACEMS1(cinfo, 1, JTRC_SOS, n);

  if (length != (n * 2 + 6) || n < 1 || n > MAX_COMPS_IN_SCAN)
    ERREXIT(cinfo, JERR_BAD_LENGTH);

  cinfo->comps_in_scan = n;

  /* Collect the component-spec parameters */

  for (i = 0; i < cinfo->num_components; i++)
    <vul-start>cinfo->cur_comp_info[i] = NULL;<vul-end>

  for (i = 0; i < n; i++) {
    INPUT_BYTE(cinfo, cc, return FALSE);
    INPUT_BYTE(cinfo, c, return FALSE);
    
    for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
	 ci++, compptr++) {
      if (cc == compptr->component_id && !cinfo->cur_comp_info[ci])
	goto id_found;
    }

    ERREXIT1(cinfo, JERR_BAD_COMPONENT_ID, cc);

  id_found:

    cinfo->cur_comp_info[i] = compptr;
    compptr->dc_tbl_no = (c >> 4) & 15;
    compptr->ac_tbl_no = (c     ) & 15;
    
    TRACEMS3(cinfo, 1, JTRC_SOS_COMPONENT, cc,
	     compptr->dc_tbl_no, compptr->ac_tbl_no);
  }

  /* Collect the additional scan parameters Ss, Se, Ah/Al. */
  INPUT_BYTE(cinfo, c, return FALSE);
  cinfo->Ss = c;
  INPUT_BYTE(cinfo, c, return FALSE);
  cinfo->Se = c;
  INPUT_BYTE(cinfo, c, return FALSE);
  cinfo->Ah = (c >> 4) & 15;
  cinfo->Al = (c     ) & 15;

  TRACEMS4(cinfo, 1, JTRC_SOS_PARAMS, cinfo->Ss, cinfo->Se,
	   cinfo->Ah, cinfo->Al);

  /* Prepare to scan data & restart markers */
  cinfo->marker->next_restart_num = 0;

  /* Count another SOS marker */
  cinfo->input_scan_number++;

  INPUT_SYNC(cinfo);
  return TRUE;
}