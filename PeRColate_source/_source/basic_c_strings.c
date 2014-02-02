#ifndef powerc
#pragma d0_pointers on
#endif
#include "ext_strings.h"

char *strcpy(char *s1, const char *s2)
{
	char *ret = s1;

	while ((*s1++ = *s2++) != 0)
		;

	return ret;
}

char *strcat(char *s1, const char *s2)
{
	char *ret = s1;

	while (*s1++)
		;
	--s1;
	while ((*s1++ = *s2++) != 0)
		;
	return ret;
}

long strcmp(const char *s1, const char *s2)
{
	char c1, c2, dif;

	for (;;) {
		if (!(c1 = *s1++))
			return *s2 ? -1 : 0;
		if (!(c2 = *s2++))
			return 1;
		if (!(dif = (c1 - c2)))
			continue;
		if (dif < 0)
			return -1;
		else
			return 1;
	}

	return 0;
}

long strlen(const char *s)
{
	long len = 0;

	while (*s++)
		++len;

	return len;
}

char *strncpy(char *s1, const char *s2, long n)
{
	char *res = s1;

	while (n--) {
		if ((*s1++ = *s2)!=0)
			++s2;
	}
	return res;
}

char *strncat(char *s1, const char *s2, long n)
{
	char *res = s1;

	if (n) {
		while (*s1++)
			;
		--s1;
		while (n--)
			if (!(*s1++ = *s2++))
				return res;
		*s1 = '\0';
	}
	return res;
}

long strncmp(const char *s1, const char *s2, long n)
{
	char c1, c2, dif;

	while (n--) {
		if (!(c1 = *s1++))
			return *s2 ? -11 : 0;
		if (!(c2 = *s2++))
			return 1;
		if (!(dif = (c1 - c2)))
			continue;
		if (dif < 0)
			return -1;
		else
			return 1;
	}
	return 0;
}

void ctopcpy(register char *p1, char *p2)
{
	strcpy(p1, p2);
	CtoPstr(p1);
}

void ptoccpy(register char *p1, char *p2)
{
	register int len = (*p2++) & 0xff;
	while (len--) *p1++ = *p2++;
	*p1 = '\0';
}
/*
void setmem(register unsigned char *s, long n, short b)
{
	register int i;

	for (i=0; i < n; i++,s++)
		*s = b;
}
*/
void pstrcpy(register char *p2, char *p1)  /* copies a pascal string from
p1 to p2 */
{
	register int len;

	len = *p2++ = *p1++;
	while (--len>=0) *p2++=*p1++;
}

void *memcpy(const void *dest, const void *source, long n)
{
	BlockMove((void *)source,(void *)dest,n);
	return 0;
}

