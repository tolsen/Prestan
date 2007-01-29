/* 
   WebDAV 207 multi-status response handling
   Copyright (C) 1999-2003, Joe Orton <joe@manyfish.co.uk>

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

/* Generic handling for WebDAV 207 Multi-Status responses. */

#include "config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "ne_alloc.h"
#include "ne_utils.h"
#include "ne_xml.h"
#include "ne_207.h"
#include "ne_uri.h"
#include "ne_basic.h"

#include "ne_i18n.h"

struct ne_207_parser_s {
    ne_207_start_response start_response;
    ne_207_end_response end_response;
    ne_207_start_propstat start_propstat;
    ne_207_end_propstat end_propstat;
    ne_xml_parser *parser;
    void *userdata;

    /* remember whether we are in a response: the validation
     * doesn't encapsulate this since we only count as being 
     * "in a response" when we've seen the href element. */
    int in_response;

    /* current position */
    void *response, *propstat;
    /* caching */
    ne_status status;
    char *description, *href;
};

static const struct ne_xml_elm elements[] = {
    { "DAV:", "multistatus", NE_ELM_multistatus, 0 },
    { "DAV:", "response", NE_ELM_response, 0 },
    { "DAV:", "responsedescription", NE_ELM_responsedescription, 
      NE_XML_CDATA },
    { "DAV:", "href", NE_ELM_href, NE_XML_CDATA },
    { "DAV:", "propstat", NE_ELM_propstat, 0 },
    { "DAV:", "prop", NE_ELM_prop, 0 },
    { "DAV:", "status", NE_ELM_status, NE_XML_CDATA },
    { NULL }
};

/* Set the callbacks for the parser */
void ne_207_set_response_handlers(ne_207_parser *p,
				   ne_207_start_response start,
				   ne_207_end_response end)
{
    p->start_response = start;
    p->end_response = end;
}

void ne_207_set_propstat_handlers(ne_207_parser *p,
				   ne_207_start_propstat start,
				   ne_207_end_propstat end)
{
    p->start_propstat = start;
    p->end_propstat = end;
}

void *ne_207_get_current_response(ne_207_parser *p)
{
    return p->response;
}

void *ne_207_get_current_propstat(ne_207_parser *p)
{
    return p->propstat;
}

static int 
start_element(void *userdata, const struct ne_xml_elm *elm, 
	      const char **atts) 
{
    ne_207_parser *p = userdata;
    
    switch (elm->id) {
    case NE_ELM_response:
	/* Create new response delayed until we get HREF */
	break;
    case NE_ELM_propstat:
	if (p->in_response && p->start_propstat) {
	    p->propstat = p->start_propstat(p->userdata, p->response);
	}
	break;
    }
    return 0;
}

#define GIVE_STATUS(p) ((p)->status.reason_phrase?&(p)->status:NULL)

static int 
end_element(void *userdata, const struct ne_xml_elm *elm, const char *cdata)
{
    ne_207_parser *p = userdata;

    switch (elm->id) {
    case NE_ELM_responsedescription:
	if (cdata != NULL) {
	    NE_FREE(p->description);
	    p->description = ne_strdup(cdata);
	}
	break;
    case NE_ELM_href:
	/* Now we have the href, begin the response */
	if (p->start_response && cdata != NULL) {
	    p->response = p->start_response(p->userdata, cdata);
	    p->in_response = 1;
	}
	break;
    case NE_ELM_status:
	if (cdata) {
	    NE_FREE(p->status.reason_phrase);
	    if (ne_parse_statusline(cdata, &p->status)) {
		char buf[500];
		NE_DEBUG(NE_DBG_HTTP, "Status line: %s\n", cdata);
		ne_snprintf(buf, 500, 
			    _("Invalid HTTP status line in status element at line %d of response:\nStatus line was: %s"),
			    ne_xml_currentline(p->parser), cdata);
		ne_xml_set_error(p->parser, buf);
		return -1;
	    } else {
		NE_DEBUG(NE_DBG_XML, "Decoded status line: %s\n", cdata);
	    }
	}
	break;
    case NE_ELM_propstat:
	if (p->end_propstat)
	    p->end_propstat(p->userdata, p->propstat, GIVE_STATUS(p),
			    p->description);
	p->propstat = NULL;
	NE_FREE(p->description);
	NE_FREE(p->status.reason_phrase);
	break;
    case NE_ELM_response:
	if (p->end_response)
	    p->end_response(p->userdata, p->response, GIVE_STATUS(p),
			    p->description);
	p->response = NULL;
	p->in_response = 0;
	NE_FREE(p->status.reason_phrase);
	NE_FREE(p->description);
	break;
    }
    return 0;
}

/* This should map directly from the DTD... with the addition of
 * ignoring anything we don't understand, being liberal in what we
 * accept. */
static int check_context(void *ud, ne_xml_elmid parent, ne_xml_elmid child) 
{
    NE_DEBUG(NE_DBG_XML, "207cc: %d in %d\n", child, parent);
    switch (parent) {
    case NE_ELM_root:
	switch (child) {
	case NE_ELM_multistatus:
	case NE_ELM_response: /* not sure why this is here... */
	    return NE_XML_VALID;
	default:
	    break;
	}
	break;
    case NE_ELM_multistatus:
	/* <!ELEMENT multistatus (response+, responsedescription?) > */
	switch (child) {
	case NE_ELM_response:
	case NE_ELM_responsedescription:
	    return NE_XML_VALID;
	default:
	    break;
	}
	break;
    case NE_ELM_response:
	/* <!ELEMENT response (href, ((href*, status)|(propstat+)),
	   responsedescription?) > */
	switch (child) {
	case NE_ELM_href:
	case NE_ELM_status:
	case NE_ELM_propstat:
	case NE_ELM_responsedescription:
	    return NE_XML_VALID;
	default:
	    break;
	}
	break;
    case NE_ELM_propstat:
	/* <!ELEMENT propstat (prop, status, responsedescription?) > */
	switch (child) {
	case NE_ELM_prop: 
	case NE_ELM_status:
	case NE_ELM_responsedescription:
	    return NE_XML_VALID;
	default:
	    break;
	}
	break;
    default:
	break;
    }

    return NE_XML_DECLINE;
}

static int ignore_cc(void *ud, ne_xml_elmid parent, ne_xml_elmid child) 
{
    if (child == NE_ELM_unknown || parent == NE_ELM_unknown) {
	NE_DEBUG(NE_DBG_XML, "207 catch-all caught %d in %d\n", child, parent);
	return NE_XML_VALID;
    }

    return NE_XML_DECLINE;
}

void ne_207_ignore_unknown(ne_207_parser *p)
{
    static const struct ne_xml_elm any_elms[] = {
	{ "", "", NE_ELM_unknown, NE_XML_COLLECT },
	{ NULL }
    };
    
    ne_xml_push_handler(p->parser, any_elms,
			 ignore_cc, NULL, NULL, NULL);
    
}

ne_207_parser *ne_207_create(ne_xml_parser *parser, void *userdata)
{
    ne_207_parser *p = ne_calloc(sizeof *p);

    p->parser = parser;
    p->userdata = userdata;
    
    /* Add handler for the standard 207 elements */
    ne_xml_push_handler(parser, elements, check_context, 
			start_element, end_element, p);
    
    return p;
}

void ne_207_destroy(ne_207_parser *p) 
{
    if (p->status.reason_phrase) ne_free(p->status.reason_phrase);
    ne_free(p);
}

int ne_accept_207(void *userdata, ne_request *req, const ne_status *status)
{
    return (status->code == 207);
}

/* Handling of 207 errors: we keep a string buffer, and append
 * messages to it as they come down.
 *
 * Note, 424 means it would have worked but something else went wrong.
 * We will have had the error for "something else", so we display
 * that, and skip 424 errors. */

/* This is passed as userdata to the 207 code. */
struct context {
    char *href;
    ne_buffer *buf;
    unsigned int is_error;
};

static void *start_response(void *userdata, const char *href)
{
    struct context *ctx = userdata;
    NE_FREE(ctx->href);
    ctx->href = ne_strdup(href);
    return NULL;
}

static void handle_error(struct context *ctx, const ne_status *status,
			 const char *description)
{
    if (status && status->klass != 2 && status->code != 424) {
	char buf[50];
	ctx->is_error = 1;
	sprintf(buf, "%d", status->code);
	ne_buffer_concat(ctx->buf, ctx->href, ": ", 
			 buf, " ", status->reason_phrase, "\n", NULL);
	if (description != NULL) {
	    /* TODO: these can be multi-line. Would be good to
	     * word-wrap this at col 80. */
	    ne_buffer_concat(ctx->buf, " -> ", description, "\n", NULL);
	}
    }

}

static void end_response(void *userdata, void *response,
			 const ne_status *status, const char *description)
{
    struct context *ctx = userdata;
    handle_error(ctx, status, description);
}

static void 
end_propstat(void *userdata, void *propstat,
	     const ne_status *status, const char *description)
{
    struct context *ctx = userdata;
    handle_error(ctx, status, description);
}

/* Dispatch a DAV request and handle a 207 error response appropriately */
/* TODO: hook up Content-Type parsing; passing charset to XML parser */
int ne_simple_request(ne_session *sess, ne_request *req)
{
    int ret;
    struct context ctx = {0};
    ne_207_parser *p207;
    ne_xml_parser *p;
    
    p = ne_xml_create();
    p207 = ne_207_create(p, &ctx);
    /* The error string is progressively written into the
     * ne_buffer by the element callbacks */
    ctx.buf = ne_buffer_create();

    ne_207_set_response_handlers(p207, start_response, end_response);
    ne_207_set_propstat_handlers(p207, NULL, end_propstat);
    
    ne_add_response_body_reader(req, ne_accept_207, ne_xml_parse_v, p);

    ne_207_ignore_unknown(p207);

    ret = ne_request_dispatch(req);

    if (ret == NE_OK) {
	if (ne_get_status(req)->code == 207) {
	    if (!ne_xml_valid(p)) { 
		/* The parse was invalid */
		ne_set_error(sess, ne_xml_get_error(p));
		ret = NE_ERROR;
	    } else if (ctx.is_error) {
		/* If we've actually got any error information
		 * from the 207, then set that as the error */
		ne_set_error(sess, ctx.buf->data);
		ret = NE_ERROR;
	    }
	} else if (ne_get_status(req)->klass != 2) {
	    ret = NE_ERROR;
	}
    }

    ne_207_destroy(p207);
    ne_xml_destroy(p);
    ne_buffer_destroy(ctx.buf);
    NE_FREE(ctx.href);

    ne_request_destroy(req);

    return ret;
}
    
