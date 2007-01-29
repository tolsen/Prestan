/* 
   Higher Level Interface to XML Parsers.
   Copyright (C) 1999-2002, Joe Orton <joe@manyfish.co.uk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA

*/

#ifndef NE_XML_H
#define NE_XML_H

#include <sys/types.h> /* for size_t */

#include "ne_defs.h"

BEGIN_NEON_DECLS

/* Generic XML parsing interface...

   See doc/parsing-xml.txt for documentation.
   
   Basic principle is that you provide these things:
     1) a list of elements which you wish to handle 
     2) a validation callback which tells the parser whether you
     want to handle this element.
     3) a callback function which is called when a complete element
     has been parsed (provided you are handling the element).
    
   This code then deals with the boring stuff like:
     - Dealing with XML namespaces   
     - Collecting CDATA

   The element list is stored in a 'ne_xml_elm' array.  For each
   element, you specify the element name, an element id, and some
   flags for that element.

const static struct ne_xml_elm[] = {
   { "DAV:", "resourcetype", ELM_resourcetype, 0 },
   { "DAV:", "collection", ELM_collection, NE_ELM_CDATA },
   { NULL }
};

   This list contains two elements, resourcetype and collection, both in 
   the "DAV:" namespace.  The collection element can contain cdata.
   
*/

/* Reserved element id's */
#define NE_ELM_unknown -1
#define NE_ELM_root 0

#define NE_ELM_UNUSED (100)

typedef int ne_xml_elmid;

struct ne_xml_elm;

/* An element */
struct ne_xml_elm {
    const char *nspace, *name;
    ne_xml_elmid id;
    unsigned int flags;
};

/* Function to check element context... 
   This callback must return:
     NE_XML_VALID ->
	Yes, this is valid XML, and I want to handle this element.
     NE_XML_INVALID ->
	No, this is NOT valid XML, and parsing should stop.
     NE_XML_DECLINE ->
	I don't know anything about this element, someone else
	can handle it.
*/


#define NE_XML_VALID (0)
#define NE_XML_INVALID (-1)
#define NE_XML_DECLINE (-2)

/* Validate a new child element. */
typedef int (*ne_xml_validate_cb)
    (void *userdata, ne_xml_elmid parent, ne_xml_elmid child);

typedef int (*ne_xml_startelm_cb)
    (void *userdata, const struct ne_xml_elm *elm, const char **atts);

/* Called when a complete element is parsed */
typedef int (*ne_xml_endelm_cb)
    (void *userdata, const struct ne_xml_elm *s, const char *cdata);

typedef void (*ne_xml_cdata_cb)
    (void *userdata, const struct ne_xml_elm *s, 
     const char *cdata, int len);

struct ne_xml_parser_s;
typedef struct ne_xml_parser_s ne_xml_parser;

/* Flags */
/* This element has no children */
#define NE_XML_CDATA (1<<1)
/* Collect complete contents of this node as cdata */
#define NE_XML_COLLECT ((1<<2) | NE_XML_CDATA)
/* This element uses MIXED mode */
#define NE_XML_MIXED (1<<4)
/* Strip leading whitespace in cdata. */
#define NE_XML_STRIPWS (1<<5)

/* Initialise the parser */
ne_xml_parser *ne_xml_create(void);

/* Push a handler onto the handler stack for the given list of elements.
 * elements must be an array, with the last element .nspace being NULL.
 * Callbacks are called in order:
 *   1. validate_cb
 *   2. startelm_cb
 *   3. endelm_cb
 * (then back to the beginning again).
 * If any of the callbacks ever return non-zero, the parse will STOP.
 * userdata is passed as the first argument to startelm_cb and endelm_cb.
 */
void ne_xml_push_handler(ne_xml_parser *p,
			  const struct ne_xml_elm *elements, 
			  ne_xml_validate_cb validate_cb, 
			  ne_xml_startelm_cb startelm_cb, 
			  ne_xml_endelm_cb endelm_cb,
			  void *userdata);

/* Add a handler which uses a mixed-mode cdata callback */
void ne_xml_push_mixed_handler(ne_xml_parser *p,
				const struct ne_xml_elm *elements,
				ne_xml_validate_cb validate_cb,
				ne_xml_startelm_cb startelm_cb,
				ne_xml_cdata_cb cdata_cb,
				ne_xml_endelm_cb endelm_cb,
				void *userdata);

/* Returns non-zero if the parse was valid, zero if it failed (e.g.,
 * any of the callbacks return non-zero, the XML was not well-formed,
 * etc).  Use ne_xml_get_error to retrieve the error message if it
 * failed. */
int ne_xml_valid(ne_xml_parser *p);

/* Destroys the parser. Any operations on it then have 
 * undefined results. */
void ne_xml_destroy(ne_xml_parser *p);

/* Parse the given block of input of length len. Block does 
 * not need to be NULL-terminated. */
void ne_xml_parse(ne_xml_parser *p, const char *block, size_t len);

/* As above, casting (ne_xml_parser *)userdata internally.
 * (This function can be passed to ne_add_response_body_reader) */
void ne_xml_parse_v(void *userdata, const char *block, size_t len);

/* Return current parse line for errors */
int ne_xml_currentline(ne_xml_parser *p);

/* Set error message for parser */
void ne_xml_set_error(ne_xml_parser *p, const char *msg);

const char *ne_xml_get_error(ne_xml_parser *p);

/* From a start_element callback which was passed 'attrs' using given
 * parser, return attribute of given name and namespace.  If nspace is
 * NULL, no namespace resolution is performed. */
const char *ne_xml_get_attr(ne_xml_parser *parser,
			    const char **attrs, const char *nspace, 
			    const char *name);

/* Return the encoding of the document being parsed.  May return NULL
 * if no encoding is defined or if the XML declaration has not been
 * parsed. */
const char *ne_xml_doc_encoding(const ne_xml_parser *p);

/* media type, appropriate for adding to a Content-Type header */
#define NE_XML_MEDIA_TYPE "application/xml"

END_NEON_DECLS

#endif /* NE_XML_H */
