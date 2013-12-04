/**
 * @file oval_resultTest.c
 * \brief Open Vulnerability and Assessment Language
 *
 * See more details at http://oval.mitre.org/
 */

/*
 * Copyright 2009--2013 Red Hat Inc., Durham, North Carolina.
 * All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors:
 *     Tomas Heinrich <theinric@redhat.com>
 *     Šimon Lukašík
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "oval_definitions.h"
#include "oval_types.h"

#include "common/util.h"
#include "common/debug_priv.h"
#include "results/oval_ip_address_impl.h"

oval_result_t ipv4addr_cmp(char *s1, char *s2, oval_operation_t op)
{
	oval_result_t result = OVAL_RESULT_ERROR;
	char *s, *pfx;
	int nm1, nm2;
	struct in_addr addr1, addr2;

	s = strdup(s1);
	pfx = strchr(s, '/');
	if (pfx) {
		int cnt;
		char nm[4];

		*pfx++ = '\0';
		cnt = sscanf(pfx, "%hhu.%hhu.%hhu.%hhu", &nm[0], &nm[1], &nm[2], &nm[3]);
		if (cnt > 1) { /* netmask */
			nm1 = (nm[0] << 24) + (nm[1] << 16) + (nm[2] << 8) + nm[3];
		} else { /* prefix */
			nm1 = (~0) << (32 - nm[0]);
		}
	} else {
		nm1 = ~0;
	}
	if (inet_pton(AF_INET, s, &addr1) <= 0) {
		dW("inet_pton() failed.\n");
		goto cleanup;
	}

	oscap_free(s);
	s = strdup(s2);
	pfx = strchr(s, '/');
	if (pfx) {
		int cnt;
		char nm[4];

		*pfx++ = '\0';
		cnt = sscanf(pfx, "%hhu.%hhu.%hhu.%hhu", &nm[0], &nm[1], &nm[2], &nm[3]);
		if (cnt > 1) { /* netmask */
			nm2 = (nm[0] << 24) + (nm[1] << 16) + (nm[2] << 8) + nm[3];
		} else { /* prefix */
			nm2 = (~0) << (32 - nm[0]);
		}
	} else {
		nm2 = ~0;
	}
	if (inet_pton(AF_INET, s, &addr2) <= 0) {
		dW("inet_pton() failed.\n");
		goto cleanup;
	}

	switch (op) {
	case OVAL_OPERATION_EQUALS:
		if (!memcmp(&addr1, &addr2, sizeof(struct in_addr)) && nm1 == nm2)
			result = OVAL_RESULT_TRUE;
		else
			result = OVAL_RESULT_FALSE;
		break;
	case OVAL_OPERATION_NOT_EQUAL:
		if (memcmp(&addr1, &addr2, sizeof(struct in_addr)) || nm1 != nm2)
			result = OVAL_RESULT_TRUE;
		else
			result = OVAL_RESULT_FALSE;
		break;
	case OVAL_OPERATION_SUBSET_OF:
		if (nm1 <= nm2) {
			result = OVAL_RESULT_FALSE;
			break;
		}
		/* FALLTHROUGH */
	case OVAL_OPERATION_GREATER_THAN:
		addr1.s_addr &= nm1;
		addr2.s_addr &= nm2;
		if (memcmp(&addr1, &addr2, sizeof(struct in_addr)) > 0)
			result = OVAL_RESULT_TRUE;
		else
			result = OVAL_RESULT_FALSE;
		break;
	case OVAL_OPERATION_GREATER_THAN_OR_EQUAL:
		addr1.s_addr &= nm1;
		addr2.s_addr &= nm2;
		if (memcmp(&addr1, &addr2, sizeof(struct in_addr)) >= 0)
			result = OVAL_RESULT_TRUE;
		else
			result = OVAL_RESULT_FALSE;
		break;
	case OVAL_OPERATION_SUPERSET_OF:
		if (nm1 >= nm2) {
			result = OVAL_RESULT_FALSE;
			break;
		}
		/* FALLTHROUGH */
	case OVAL_OPERATION_LESS_THAN:
		addr1.s_addr &= nm1;
		addr2.s_addr &= nm2;
		if (memcmp(&addr1, &addr2, sizeof(struct in_addr)) < 0)
			result = OVAL_RESULT_TRUE;
		else
			result = OVAL_RESULT_FALSE;
		break;
	case OVAL_OPERATION_LESS_THAN_OR_EQUAL:
		addr1.s_addr &= nm1;
		addr2.s_addr &= nm2;
		if (memcmp(&addr1, &addr2, sizeof(struct in_addr)) <= 0)
			result = OVAL_RESULT_TRUE;
		else
			result = OVAL_RESULT_FALSE;
		break;
	default:
		dE("Unexpected compare operation: %d.\n", op);
	}

 cleanup:
	oscap_free(s);

	return result;
}

static void mask_v6_addrs(struct in6_addr *addr1, int p1len, struct in6_addr *addr2, int p2len)
{
	int i;

	for (i = 0; i < 128; ++i) {
		addr1->s6_addr[i / 8] &= (((i < p1len) ? 1 : 0) << (i % 8));
		addr2->s6_addr[i / 8] &= (((i < p2len) ? 1 : 0) << (i % 8));
	}
}

oval_result_t ipv6addr_cmp(char *s1, char *s2, oval_operation_t op)
{
	oval_result_t result = OVAL_RESULT_ERROR;
	char *s, *pfx;
	int p1len, p2len;
	struct in6_addr addr1, addr2;

	s = strdup(s1);
	pfx = strchr(s, '/');
	if (pfx) {
		*pfx++ = '\0';
		p1len = strtol(pfx, NULL, 10);
	} else {
		p1len = 128;
	}
	if (inet_pton(AF_INET6, s, &addr1) <= 0) {
		dW("inet_pton() failed.\n");
		goto cleanup;
	}

	oscap_free(s);
	s = strdup(s2);
	pfx = strchr(s, '/');
	if (pfx) {
		*pfx++ = '\0';
		p2len = strtol(pfx, NULL, 10);
	} else {
		p2len = 128;
	}
	if (inet_pton(AF_INET6, s, &addr2) <= 0) {
		dW("inet_pton() failed.\n");
		goto cleanup;
	}

	switch (op) {
	case OVAL_OPERATION_EQUALS:
		if (!memcmp(&addr1, &addr2, sizeof(struct in6_addr)) && p1len == p2len)
			result = OVAL_RESULT_TRUE;
		else
			result = OVAL_RESULT_FALSE;
		break;
	case OVAL_OPERATION_NOT_EQUAL:
		if (memcmp(&addr1, &addr2, sizeof(struct in6_addr)) || p1len != p2len)
			result = OVAL_RESULT_TRUE;
		else
			result = OVAL_RESULT_FALSE;
		break;
	case OVAL_OPERATION_SUBSET_OF:
		if (p1len <= p2len) {
			result = OVAL_RESULT_FALSE;
			break;
		}
		/* FALLTHROUGH */
	case OVAL_OPERATION_GREATER_THAN:
		mask_v6_addrs(&addr1, p1len, &addr2, p2len);
		if (memcmp(&addr1, &addr2, sizeof(struct in6_addr)) > 0)
			result = OVAL_RESULT_TRUE;
		else
			result = OVAL_RESULT_FALSE;
		break;
	case OVAL_OPERATION_GREATER_THAN_OR_EQUAL:
		mask_v6_addrs(&addr1, p1len, &addr2, p2len);
		if (memcmp(&addr1, &addr2, sizeof(struct in6_addr)) >= 0)
			result = OVAL_RESULT_TRUE;
		else
			result = OVAL_RESULT_FALSE;
		break;
	case OVAL_OPERATION_SUPERSET_OF:
		if (p1len >= p2len) {
			result = OVAL_RESULT_FALSE;
			break;
		}
		/* FALLTHROUGH */
	case OVAL_OPERATION_LESS_THAN:
		mask_v6_addrs(&addr1, p1len, &addr2, p2len);
		if (memcmp(&addr1, &addr2, sizeof(struct in6_addr)) < 0)
			result = OVAL_RESULT_TRUE;
		else
			result = OVAL_RESULT_FALSE;
		break;
	case OVAL_OPERATION_LESS_THAN_OR_EQUAL:
		mask_v6_addrs(&addr1, p1len, &addr2, p2len);
		if (memcmp(&addr1, &addr2, sizeof(struct in6_addr)) <= 0)
			result = OVAL_RESULT_TRUE;
		else
			result = OVAL_RESULT_FALSE;
		break;
	default:
		dE("Unexpected compare operation: %d.\n", op);
	}

 cleanup:
	oscap_free(s);

	return result;
}
