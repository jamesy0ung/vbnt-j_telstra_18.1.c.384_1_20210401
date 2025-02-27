/* $Id: upnpreplyparse.c,v 1.15 2013/06/06 21:36:40 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2013 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "upnpreplyparse.h"
#include "minixml.h"

static void
NameValueParserStartElt(void * d, const char * name, int l)
{
	struct NameValueParserData * data = (struct NameValueParserData *)d;
	data->topelt = 1;
    if(l>63)
        l = 63;
    memcpy(data->curelt, name, l);
    data->curelt[l] = '\0';
	data->cdata = NULL;
	data->cdatalen = 0;
}

static void
NameValueParserEndElt(void * d, const char * name, int l)
{
    struct NameValueParserData * data = (struct NameValueParserData *)d;
    struct NameValue * nv;
	(void)name;
	(void)l;
	if(!data->topelt)
		return;
	if(strcmp(data->curelt, "NewPortListing") != 0)
	{
		int l;
		/* standard case. Limited to n chars strings */
		l = data->cdatalen;
	    nv = malloc(sizeof(struct NameValue));
	    if(l>=(int)sizeof(nv->value))
	        l = sizeof(nv->value) - 1;
	    strncpy(nv->name, data->curelt, 64);
		nv->name[63] = '\0';
		if(data->cdata != NULL)
		{
			memcpy(nv->value, data->cdata, l);
			nv->value[l] = '\0';
		}
		else
		{
			nv->value[0] = '\0';
		}
	    LIST_INSERT_HEAD( &(data->head), nv, entries);
	}
	data->cdata = NULL;
	data->cdatalen = 0;
	data->topelt = 0;
}

static void
NameValueParserGetData(void * d, const char * datas, int l)
{
    struct NameValueParserData * data = (struct NameValueParserData *)d;
	if(strcmp(data->curelt, "NewPortListing") == 0)
	{
		/* specific case for NewPortListing which is a XML Document */
		data->portListing = malloc(l + 1);
		if(!data->portListing)
		{
			/* malloc error */
			return;
		}
		memcpy(data->portListing, datas, l);
		data->portListing[l] = '\0';
		data->portListingLength = l;
	}
	else
	{
		/* standard case. */
		data->cdata = datas;
		data->cdatalen = l;
	}
}

void
ParseNameValue(const char * buffer, int bufsize,
               struct NameValueParserData * data)
{
    struct xmlparser parser;
    LIST_INIT(&(data->head));
	data->portListing = NULL;
	data->portListingLength = 0;
	data->cdata = NULL;
    /* init xmlparser object */
    parser.xmlstart = buffer;
    parser.xmlsize = bufsize;
    parser.data = data;
    parser.starteltfunc = NameValueParserStartElt;
    parser.endeltfunc = NameValueParserEndElt;
    parser.datafunc = NameValueParserGetData;
	parser.attfunc = 0;
    parsexml(&parser);
}

void
ClearNameValueList(struct NameValueParserData * pdata)
{
    struct NameValue * nv;
	if(pdata->portListing)
	{
		free(pdata->portListing);
		pdata->portListing = NULL;
		pdata->portListingLength = 0;
	}
    while((nv = pdata->head.lh_first) != NULL)
    {
        LIST_REMOVE(nv, entries);
        free(nv);
    }
}

char *
GetValueFromNameValueList(struct NameValueParserData * pdata,
                          const char * Name)
{
    struct NameValue * nv;
    char * p = NULL;
    for(nv = pdata->head.lh_first;
        (nv != NULL) && (p == NULL);
        nv = nv->entries.le_next)
    {
        if(strcmp(nv->name, Name) == 0)
            p = nv->value;
    }
    return p;
}

#if 0
/* useless now that minixml ignores namespaces by itself */
char *
GetValueFromNameValueListIgnoreNS(struct NameValueParserData * pdata,
                                  const char * Name)
{
	struct NameValue * nv;
	char * p = NULL;
	char * pname;
	for(nv = pdata->head.lh_first;
	    (nv != NULL) && (p == NULL);
		nv = nv->entries.le_next)
	{
		pname = strrchr(nv->name, ':');
		if(pname)
			pname++;
		else
			pname = nv->name;
		if(strcmp(pname, Name)==0)
			p = nv->value;
	}
	return p;
}
#endif

/* debug all-in-one function
 * do parsing then display to stdout */
#ifdef DEBUG
void
DisplayNameValueList(char * buffer, int bufsize)
{
    struct NameValueParserData pdata;
    struct NameValue * nv;
    ParseNameValue(buffer, bufsize, &pdata);
    for(nv = pdata.head.lh_first;
        nv != NULL;
        nv = nv->entries.le_next)
    {
        printf("%s = %s\n", nv->name, nv->value);
    }
    ClearNameValueList(&pdata);
}
#endif

