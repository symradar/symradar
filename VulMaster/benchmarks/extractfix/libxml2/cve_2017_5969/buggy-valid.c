/**
 * xmlDumpElementContent:
 * @buf:  An XML buffer
 * @content:  An element table
 * @glob: 1 if one must print the englobing parenthesis, 0 otherwise
 *
 * This will dump the content of the element table as an XML DTD definition
 */
static void
xmlDumpElementContent(xmlBufferPtr buf, xmlElementContentPtr content, int glob) {
    if (content == NULL) return;

    if (glob) xmlBufferWriteChar(buf, "(");
    switch (content->type) {
        case XML_ELEMENT_CONTENT_PCDATA:
            xmlBufferWriteChar(buf, "#PCDATA");
	    break;
	case XML_ELEMENT_CONTENT_ELEMENT:
	    if (content->prefix != NULL) {
		xmlBufferWriteCHAR(buf, content->prefix);
		xmlBufferWriteChar(buf, ":");
	    }
	    xmlBufferWriteCHAR(buf, content->name);
	    break;
	case XML_ELEMENT_CONTENT_SEQ:
	    if ((content->c1->type == XML_ELEMENT_CONTENT_OR) ||
	        (content->c1->type == XML_ELEMENT_CONTENT_SEQ))
		xmlDumpElementContent(buf, content->c1, 1);
	    else
		xmlDumpElementContent(buf, content->c1, 0);
            xmlBufferWriteChar(buf, " , ");
		<vul-start>if ((content->c2->type == XML_ELEMENT_CONTENT_OR) ||
	        ((content->c2->type == XML_ELEMENT_CONTENT_SEQ) &&
		 (content->c2->ocur != XML_ELEMENT_CONTENT_ONCE)))<vul-end>
		xmlDumpElementContent(buf, content->c2, 1);
	    else
		xmlDumpElementContent(buf, content->c2, 0);
	    break;
	case XML_ELEMENT_CONTENT_OR:
	    if ((content->c1->type == XML_ELEMENT_CONTENT_OR) ||
	        (content->c1->type == XML_ELEMENT_CONTENT_SEQ))
		xmlDumpElementContent(buf, content->c1, 1);
	    else
		xmlDumpElementContent(buf, content->c1, 0);
            xmlBufferWriteChar(buf, " | ");
	    if ((content->c2->type == XML_ELEMENT_CONTENT_SEQ) ||
	        ((content->c2->type == XML_ELEMENT_CONTENT_OR) &&
		 (content->c2->ocur != XML_ELEMENT_CONTENT_ONCE)))
		xmlDumpElementContent(buf, content->c2, 1);
	    else
		xmlDumpElementContent(buf, content->c2, 0);
	    break;
	default:
	    xmlErrValid(NULL, XML_ERR_INTERNAL_ERROR,
		    "Internal: ELEMENT content corrupted invalid type\n",
		    NULL);
    }
    if (glob)
        xmlBufferWriteChar(buf, ")");
    switch (content->ocur) {
        case XML_ELEMENT_CONTENT_ONCE:
	    break;
        case XML_ELEMENT_CONTENT_OPT:
	    xmlBufferWriteChar(buf, "?");
	    break;
        case XML_ELEMENT_CONTENT_MULT:
	    xmlBufferWriteChar(buf, "*");
	    break;
        case XML_ELEMENT_CONTENT_PLUS:
	    xmlBufferWriteChar(buf, "+");
	    break;
    }
}