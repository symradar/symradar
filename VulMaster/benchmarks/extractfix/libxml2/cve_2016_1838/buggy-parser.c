/**
 * xmlParseEndTag2:
 * @ctxt:  an XML parser context
 * @line:  line of the start tag
 * @nsNr:  number of namespaces on the start tag
 *
 * parse an end of tag
 *
 * [42] ETag ::= '</' Name S? '>'
 *
 * With namespace
 *
 * [NS 9] ETag ::= '</' QName S? '>'
 */

static void
xmlParseEndTag2(xmlParserCtxtPtr ctxt, const xmlChar *prefix,
                const xmlChar *URI, int line, int nsNr, int tlen) {
    const xmlChar *name;

    GROW;
    if ((RAW != '<') || (NXT(1) != '/')) {
	xmlFatalErr(ctxt, XML_ERR_LTSLASH_REQUIRED, NULL);
	return;
    }
    SKIP(2);

    if ((tlen > 0) && (xmlStrncmp(ctxt->input->cur, ctxt->name, tlen) == 0)) {
        if (ctxt->input->cur[tlen] == '>') {
        <vul-start>ctxt->input->cur += tlen + 1;<vul-end>
	    ctxt->input->col += tlen + 1;
	    goto done;
	}
	ctxt->input->cur += tlen;
	ctxt->input->col += tlen;
	name = (xmlChar*)1;
    } else {
	if (prefix == NULL)
	    name = xmlParseNameAndCompare(ctxt, ctxt->name);
	else
	    name = xmlParseQNameAndCompare(ctxt, ctxt->name, prefix);
    }

    /*
     * We should definitely be at the ending "S? '>'" part
     */
    GROW;
    if (ctxt->instate == XML_PARSER_EOF)
        return;
    SKIP_BLANKS;
    if ((!IS_BYTE_CHAR(RAW)) || (RAW != '>')) {
	xmlFatalErr(ctxt, XML_ERR_GT_REQUIRED, NULL);
    } else
	NEXT1;

    /*
     * [ WFC: Element Type Match ]
     * The Name in an element's end-tag must match the element type in the
     * start-tag.
     *
     */
    if (name != (xmlChar*)1) {
        if (name == NULL) name = BAD_CAST "unparseable";
        if ((line == 0) && (ctxt->node != NULL))
            line = ctxt->node->line;
        xmlFatalErrMsgStrIntStr(ctxt, XML_ERR_TAG_NAME_MISMATCH,
		     "Opening and ending tag mismatch: %s line %d and %s\n",
		                ctxt->name, line, name);
    }

    /*
     * SAX: End of Tag
     */
done:
    if ((ctxt->sax != NULL) && (ctxt->sax->endElementNs != NULL) &&
	(!ctxt->disableSAX))
	ctxt->sax->endElementNs(ctxt->userData, ctxt->name, prefix, URI);

    spacePop(ctxt);
    if (nsNr != 0)
	nsPop(ctxt, nsNr);
    return;
}