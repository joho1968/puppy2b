#include <puppy.h>
#include <pupmem.h>
#include <ascii.h>

/*
	This sets PUPPY.SYS and creates the message file.

	(k) All rights reversed
*/

int sysfile;			/* PUPPY.SYS global handle */
struct _pup pup;		/* thing we build */
int events;			/* events we find */

#define same(s1,s2) (strcmp(s1,s2)==0)

int _stack = 5000;

main(argc,argv)
int argc;
char **argv;
{
char *p,fn[SS],sw[SS];
int i;
FLAG quit,once,writeout;

	printf("Pup configuration, version 2b, 25 Jan 88\r\n");
	printf("Tom Jennings, 164 Shipley\r\n");
	printf("San Francisco CA 94107 USA\r\n");
	printf("(k) all rights reversed\r\n");

	sysfile= open("PUPPY.SYS",2);		/* open it for read/write */
	if (sysfile == -1) {
		printf(" * File \"PUPPY.SYS\" doesn't exist, making a new one\r\n");
		sysfile= creat("PUPPY.SYS",2);	/* make a new one, */
		if (sysfile == -1) {
			printf(" * Can't create it!\r\n");
			exit(1);
		}
		pup.top= 0;			/* empty file */
		pup.msgnbr= 1;			/* first message number */
		pup.quote_pos= 0L;		/* not used yet */
		pup.callers= 0L;

	} else read(sysfile,&pup,sizeof(struct _pup));	/* read current settings, */

	setdefaults();				/* fill in those blanks */
	finit("PUP.SET");
	lseek(sysfile,0L,0);
	write(sysfile,&pup,sizeof(struct _pup));
	close(sysfile);

	setmsg();				/* create a message file */
	setclr();				/* create a caller file */
	exit(0);
}

/* Create a new caller file. */

setclr() {
int f;

	f= open("puppy.clr",0);
	if (f != -1) {
		printf("A caller file already exists\r\n");
		close(f);
		return;
	}
	fffill("PUPPY.CLR",(long)(pup.callsize * sizeof(struct _clr)),0);
}
/* Create an empty message file. */

setmsg() {
int f;

	f= open("puppy.idx",0);
	if (f != -1) {
		close(f);
		printf("A message base already exists\r\n");
		return;
	}
	fffill("PUPPY.IDX",(long)(pup.messages * sizeof(struct _msg)),0);
	fffill("PUPPY.DAT",(long)(pup.messages * pup.msgsize),SUB);
}

/* Fill the specified file with junk. */

fffill(fname,count,c)
char *fname;		/* filename */
long count;		/* how many bytes to write, */
char c;			/* fill character */
{
char buff[1024];
int f,i,n;

	for (i=0; i < sizeof(buff); i++) 	/* fill the buffer with nothings */
		buff[i]= c;

	printf("Creating file %s\r\n",fname);
	f= creat(fname,1);			/* create the file, */
	if (f == -1) {
		printf("Creation error!\r\n");
		return;
	}
	while (count > 0L) {
		n= (count < sizeof(buff)) ? count : sizeof(buff);
		if (write(f,buff,n) != n) {
			printf("Disk full!\r\n");
			break;
		}
		count -= n;
	}
	close(f);
}
/* Set the defaults to put into PUPPY.SYS. */

setdefaults() {
char *p;
int i,i1,i2;
long l1,l2;

	l1= pup.callers;			/* preserve these */
	l2= pup.quote_pos;
	i1= pup.top;
	i2= pup.msgnbr;
	p= (char *) &pup;			/* clear out the structure */
	for (i= sizeof(struct _pup); i--;) *p++= NUL;
	pup.top= i1;
	pup.msgnbr= i2;
	pup.callers= l1;			/* restore */
	pup.quote_pos= l2;

	pup.messages= 50;			/* size of message base */
	pup.msgsize= 2048;			/* msg size */

	pup.callsize= 100;			/* size of caller base */

	pup.nlimit= 60;				/* default= 1 hr time limit, */
	pup.klimit= 100;			/* 100K download limit, */

	pup.cd_bit= 0x80;			/* CD bit on IBM Async Card CTS */
	pup.maxbaud= 1200;			/* 1200 baud max */
	strcpy(pup.mdmstr,"ATX1E0V0M0S0=0");	/* default modem init */

	pup.connects= 1;			/* 1 attempt with connect */
	pup.tries= 10;				/* 10 attempts to dial */
}


/* Fido initializer */

#define KEYLEN 10

char keyword[][KEYLEN] = {

/* 0 */		"time-limi",
		"k-limit",

/* 2 */		"max-baud",
		"cd-bit",
		"modem-str",

/* 5 */		"dial-trie",
		"connect-t",
		"io-port",

/* 8 */		"node",
		"net",
		"zone",

/* 11 */	"file-pref",

/* 12 */	"message-t",
		"message-s",

/* 14 */	"event",

/* 15 */	"topic",

/* 16 */	"callers",

		""	/* end of list */
};

finit(fn)
char *fn;
{
char *process();

int i,f;
char ln[256];		/* LONG raw input line */
char word[sizeof(ln)];	/* word we parse */
char arg[sizeof(ln)];	/* an arg for it */
int value;		/* argument value */
int line;		/* line in file */
FLAG err;		/* error in file */
char *cp;


	f= open(fn,0);
	if (f == -1) {
		printf(" * Can't find Startup File %s\r\n",fn);
		return(0);
	}

	err= 0;
	line= 0;
	while (rline(f,ln,sizeof(ln))) {
		++line;

		clip_cmt(ln);				/* strip off comments */
		cp= skip_delim(ln);			/* skip leading blanks, etc */
		if (*cp == NUL) continue;		/* ignore blank lines */

		if (*cp == '*') {			/* label line */
			puts(ln); puts("\r\n");
			continue;
		}
		cpyatm(word,cp);			/* the key word, */
		word[KEYLEN - 1]= NUL;			/* truncate to match */
		cp= next_arg(cp);			/* ptr to rest of line */
		cpyatm(arg,cp);				/* its (default) arg */

		if (! *arg) {
			inierr(fn,line,ln,"Incomplete command");
			err= 1;
			continue;
		}
		stolower(word);
		stolower(arg);

		value= atoi(arg);			/* atoi() it blindly */
		if (same(arg,"on") || same(arg,"yes"))
			value= 1;			/* else 0 == off == no */

		for (i= 0; *keyword[i]; ++i) {		/* find the word, */
			if (same(keyword[i],word)) {	/* if a match, */
				cp= process(i,arg,cp,value); /* do it, */
				if (*cp) {
					inierr(fn,line,ln,cp);
					err= 1;
				}
				break;
			}
		}
		if (! *keyword[i]) {
			inierr(fn,line,ln,"Not a command");
			err= 1;
		}
	}
	close(f);
	return(1);
}
/* Complain about this line. */

inierr(fn,lineno,ln,error)
char *fn;
int lineno;
char *ln,*error;
{
	printf("%s in file %s at line %d\r\n",error,fn,lineno);
	printf("  \"%s\"\r\n",ln);
}

/* Process the keyword. */

char *process(i,arg,cp,value)
int i;			/* keyword table index */
char *arg;		/* next word at cp, for convenience */
char *cp;		/* ptr to line after keyword */
int value;		/* atoi of arg for convenience */
{
char *rp;
char *build_sched();

	rp= "";
	switch (i) {
		case 0: pup.nlimit= value; break;
		case 1: pup.klimit= value; break;

		case 2: pup.maxbaud= value; break;
		case 3: pup.cd_bit= value; break;
		case 4: stoupper(cp); strcpy(pup.mdmstr,cp); break;
		case 5: pup.tries= value; break;
		case 6: pup.connects= value; break;
		case 7: pup.iodev= value; break;

		case 8: pup.id.number= value; break;
		case 9: pup.id.net= value; break;
		case 10: pup.id.zone= atoi(arg); break;

		case 11: strcpy(pup.filepref,arg); break;

		case 12: pup.messages= value; break;
		case 13: pup.msgsize= value; break;

		case 14: if (events < SCHEDS) {
				rp= build_sched(&pup.sched[events],cp);
				if (! *rp) ++events;
			} else rp= "Too many EVENTS";
			break;

		case 15: for (i= 0; i < 16; i++) {
				if (! *pup.topic[i].name) {
					arg[8]= NUL;
					strcpy(pup.topic[i].name,arg);
					cp= next_arg(cp);
					cp[24]= NUL;
					strcpy(pup.topic[i].desc,cp);
					break;
				}
			}
			if (i == 16) rp= "Too many TOPICs";
			break;

		case 16: pup.callsize= value; break;
	}
	return(rp);
}

/* Fill in the event structure. */

char *build_sched(a,cp)
struct _sched *a;
char *cp;
{
char buff[SS],c;
int n,h,m;
long l;

	a-> hr= atoi(cp);				/* get start time, */
	if (a-> hr > 23) return("Hour must be 0 to 23");
	while (isdigit(*cp)) ++cp;			/* look for a colon */
	if (*cp == ':') a-> min= atoi(++cp);		/* get mins if so, */
	else a-> min= 0;
	if (a-> min > 59) return("Minute must be 0 to 59");

	cp= next_arg(cp);				/* get sched width */
	a-> len= atoi(cp);				/* or is it ERRORLEVEL */

	cp= next_arg(cp);				/* do tag */
	a-> tag= toupper(*cp);
	if ((a-> tag < 'A') || (a-> tag > 'X'))
		return("Event types must be A - X");

	cp= next_arg(cp);				/* options */
	stolower(cp);					/* all lower case */
	while (*cp) switch (*cp++) {
		case 'o': a-> bits |= SCHED_OPTIONAL; break;
	}
	return("");
}

/* Strip comments from a line of text; truncate it at the semicolon, then work
backwards deleting delimiters. */

clip_cmt(cp)
char *cp;
{
char *sp;

	sp= cp;					/* remember where we started */
	while (*cp) {
		if (*cp == ';') {		/* search for a semicolon */
			*cp= NUL;		/* kill it, */
			while (delim(*--cp)) 	/* kill all delims */
				*cp= NUL;
			break;
		}
		++cp;
	}
}

/* Read a line of text from the file, null terminate it. Function returns
zero if EOF. Deletes all CRs and Control-Zs from the stream. Lines are
terminated by LFs. */

rline(file,buf,len)
int file;
char *buf;
int len;
{
int i;
char notempty,c;

	i= 0; notempty= 0;
	--len;						/* compensate for added NUL */
	while (i < len) {
		if (! read(file,&c,1)) break;		/* stop if empty */
		if (c == 0x1a) continue;		/* totally ignore ^Z, */
		notempty= 1;				/* not empty */
		if (c == '\r') continue;		/* skip CR, */
		if (c == '\r' + 128) continue;		/* skip soft CR, */
		if (c == '\n') break;			/* stop if LF */
		buf[i++]= c;
	}
	buf[i]= '\0';
	return(notempty);
}

/* Return the number of args left in the string. */

num_args(s)
char *s;
{
int count;

	count= 0;
	s= skip_delim(s);			/* skip leading blanks, */
	while (*s) {
		++count;			/* count one, */
		s= next_arg(s);			/* find next, */
	}
	return(count);
}

/* Return a pointer to the next argument in the string. */

char *next_arg(s)
char *s;
{
	while ((!delim(*s)) && *s)		/* skip this one, */
		++s;				/* up to delim, */
	s= skip_delim(s);			/* then skip delims, */
	return(s);
}

/* Skip over the leading delimiters in a string. */

char *skip_delim(s)
char *s;
{
	while (delim(*s) && *s) {
		++s;
	}
	return(s);
}

/* Copy the string to the destination array, stopping if we find one
of our delimiters. */

cpyatm(to,from)
char *to;
char *from;
{
	while ( (!delim(*from)) && *from) 
		*to++= *from++;
	*to= '\0';
}

/* Copy the string to the destination array, stopping if we find one
of our delimiters. */

cpyarg(to,from)
char *to;
char *from;
{
	while (*from) {
		if (delim(*from)) break;
		*to++= *from++;
	}
	*to= '\0';
}

/* Return true if the character is a delimiter. */

delim(c)
char c;
{
	switch (c) {
		case ';': return(1);
		case ' ': return(1);
		case ',': return(1);
		case TAB: return(1);
		default: return(0);
	}
}

/* Strip the pathname or disk specifier from a filename, return it in a
seperate array. We do this by initially copying the entire name in, then
searching for the colon or slash. Right after the last one we find,
stuff a null, removing the name part. 

Also return a pointer to the name part in the input name. */

char *strip_path(out,in)
char *out;
char *in;
{
char *name;
char *endpath;

	strcpy(out,in);			/* duplicate, for working, */
	name= in;			/* point to name, */
	endpath= out;			/* and end of path part, */

	while (*in) {			/* look for slashes or colons, */
		if (*in == ':')	{	/* if a colon, */
			endpath= ++out;	/* point to name, */
			name= ++in;

		} else if (*in == '/') {
			endpath= ++out;	/* move the pointer up, */
			name= ++in;
		} else {
			++in;
			++out;
		}
	}
	*endpath= '\0';			/* delete the name part, */
	return(name);			/* return ptr to name part. */
}

/* Convert a string to lower case. */

stolower(s)
char *s;
{
	while (*s) {
		*s= tolower(*s);
		++s;
	}
}
/* Convert a string to upper case. */

stoupper(s)
char *s;
{
	while (*s) {
		*s= toupper(*s);
		++s;
	}
}

/* atoi() function missing from Lattice C. From Kernighan and Richie. */

atoi(s)
char *s;
{
int n;
	n= 0;
	while ((*s >= '0') && (*s <= '9')) {
		n *= 10;
		n += *s - '0';
		++s;
	}
	return(n);
}
