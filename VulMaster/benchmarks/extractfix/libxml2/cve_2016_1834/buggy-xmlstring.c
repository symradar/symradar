/**
 * xmlStrncat:
 * @cur:  the original xmlChar * array
 * @add:  the xmlChar * array added
 * @len:  the length of @add
 *
 * a strncat for array of xmlChar's, it will extend @cur with the len
 * first bytes of @add. Note that if @len < 0 then this is an API error
 * and NULL will be returned.
 *
 * Returns a new xmlChar *, the original @cur is reallocated if needed
 * and should not be freed
 */

xmlChar *
xmlStrncat(xmlChar *cur, const xmlChar *add, int len) {
    int size;
    xmlChar *ret;

    if ((add == NULL) || (len == 0))
        return(cur);
    if (len < 0)
	return(NULL);
    if (cur == NULL)
        return(xmlStrndup(add, len));

    size = xmlStrlen(cur);
    <vul-start>ret = (xmlChar *) xmlRealloc(cur, (size + len + 1) * sizeof(xmlChar));<vul-end>
    if (ret == NULL) {
        xmlErrMemory(NULL, NULL);
        return(cur);
    }
    memcpy(&ret[size], add, len * sizeof(xmlChar));
    ret[size + len] = 0;
    return(ret);
}