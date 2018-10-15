/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sred/action.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:12 $
 *
 */
#include "reductio.h"

char *strcopyall (from)

char *from;

{
	char *to;

	MYCALLOC(char,to,strlen(from)+1);
	strcpy (to, from);

	return (to);
}

yyerror(message)
char *message;

{
	int lp;

	errorcount++;

	if (mylinepos < 0) {
		fprintf(stderr,"unexpected end of file \n");
	}
	else {
		fprintf(stderr, myline);
		for (lp = 0; lp < mylinepos - 1; lp++)
			if (myline[lp] == '\t') fprintf(stderr, "\t");
			else	fprintf(stderr, " ");
		fprintf(stderr, "^\n");

		fprintf(stderr, "%s\n", message);
	}
}

error(message,element)

char *message, *element;

{
	errorcount++;

	fprintf(stderr,"%s%s\n", message, element);
}

faterr (message,element)

char *message, *element;

{
	errorcount++;

	fprintf(stderr,"%s%s\n", message, element);

	abort ();
}

int mygetc(file)

FILE *file;

{
	while ( myline[mylinepos] == '\0' ) {

		if (myline[mylinepos] == '\0')
			if (fgets (myline, 512, file)) {
				mylinepos = 0;
			}
			else {
				mylinepos = -1;
				return (EOF);
			}
		else mylinepos++;
	}

	return ((int) myline[mylinepos++]);
}

addoutputname()

{
	NAMETABLE *temp, *newname ();
	char wire[MAXNAME];

	strcpy (wire, "y");
	strcat (wire, lastnum);

	temp = newname (lastid, wire);

	temp->next = nametable;
	nametable = temp;
}

addinputname()

{
	NAMETABLE *temp, *newname ();
	char wire[MAXNAME];

	strcpy (wire, "x");
	strcat (wire, lastnum);

	temp = newname (lastid, wire);

	temp->next = nametable;
	nametable = temp;
}

addinput()

{
	SYMTABLE *temp, *search(), *newsymbol();

	if (strlen (lastvect) != nis)
		error("wrong length input: ", lastin);

	if (search (inputlist, lastin) != 0)
		error("multiple defined input symbol: ", lastin);

	temp = newsymbol (lastin, lastvect);
	temp->next = inputlist;
	inputlist = temp;
}

addoutput()

{
	SYMTABLE *temp, *search(), *newsymbol();

	if (strlen (lastvect) != nos)
		error("wrong length output: ", lastout);

	if (search (outputlist, lastout) != 0)
		error("multiple defined output symbol: ", lastout);

	temp = newsymbol (lastout, lastvect);
	temp->next = outputlist;
	outputlist = temp;
}

addstate()

{
	SYMTABLE *temp, *search(), *newsymbol();

	if (search (statelist, laststate) != 0)
		error("multiple output associated to state: ", laststate);

	temp = newsymbol (laststate, lastvect);
	temp->next = statelist;
	statelist = temp;
}

NAMETABLE *newname (name, wire)

char *name, *wire;

{
	NAMETABLE *temp;

	MYCALLOC (NAMETABLE, temp, 1);

	temp->symbol = strcopyall(name);
	temp->wire = strcopyall (wire);

	return (temp);
}

SYMTABLE *newsymbol (name, value)

char *name, *value;

{
	SYMTABLE *temp;

	MYCALLOC (SYMTABLE, temp, 1);

	temp->symbol = strcopyall(name);
	temp->vector = strcopyall (value);

	return (temp);
}

SYMTABLE *search (list, string)

SYMTABLE *list;
char *string;

{
	while ( list != (SYMTABLE *) 0 ) {
		if ( strcmp (list->symbol, string) == 0 )
			return (list);
		list = list->next;
	}
	return ( (SYMTABLE *) 0 );
}

moorewithinput()

{
	SYMTABLE *temp;

	if (type!=MOORE) {
		error("Moore type line in Mealy type machine","");
		return;
	}

	MYREALLOC(INPUTTABLE, itable, itable_size, np);
	if ((lastin[0] == 'e') || (lastin[0] == '*')) {

		if (!isymb) {

			temp = search (inputlist, lastin);

			if (temp == (SYMTABLE *) 0)
				error("undefined input: ", lastin);
			else 	itable[np].input = strcopyall (temp->vector);
		}
		else 	itable[np].input = strcopyall (lastin);
	}
	else {
		if (strlen(lastin) != nis) 
			error("wrong length input: x", lastin);
		else	itable[np].input = strcopyall (lastin);
	}

	temp = search (statelist, laststate);

	if ( temp == (SYMTABLE *) 0 )
		error("no output associated with state: ", laststate);
	else 	itable[np].output = strcopyall (temp->vector);

	itable[np].pstate = strcopyall (laststate);
	itable[np].nstate = strcopyall (lastnext);

	np++;
}

moorenoinput()

{
	SYMTABLE *temp;

	if (type!=MOORE) {
		error("Moore type line in Mealy type machine","");
		return;
	}

	MYREALLOC(INPUTTABLE, itable, itable_size, np);
	if (nis!=0) error("missing input specification","");

	temp = search (statelist, laststate);

	if ( temp == (SYMTABLE *) 0 )
		error("no output associated with state: ", laststate);
	else 	itable[np].output = strcopyall (temp->vector);

	itable[np].pstate = strcopyall (laststate);
	itable[np].nstate = strcopyall (lastnext);

	np++;
}

mealy()

{
	SYMTABLE *temp;

	if (type!=MEALY) {
		error("Mealy type line in Moore type machine","");
		return;
	}

	MYREALLOC(INPUTTABLE, itable, itable_size, np);
	if ((lastin[0] == 'e') || (lastin[0] == '*')) {

		if (!isymb) {

			temp = search (inputlist, lastin);

			if (temp == (SYMTABLE *) 0)
				error("undefined input: ", lastin);
			else 	itable[np].input = strcopyall (temp->vector);
		}
		else 	itable[np].input = strcopyall (lastin);
	}
	else {
		if (strlen(lastin) != nis) 
			error("wrong length input: x", lastin);
		else	itable[np].input = strcopyall (lastin);
	}

	if ((lastout[0] == 'a') || (lastout[0] == '*')) {

		if (!osymb) {

			temp = search (outputlist, lastout);

			if (temp == (SYMTABLE *) 0)
				error("undefined output: ", lastout);
			else 	itable[np].output = strcopyall (temp->vector);
		}
		else 	itable[np].output = strcopyall (lastout);
	}
	else {
		if (strlen(lastout) != nos) 
			error("wrong length output: y", lastout);
		else	itable[np].output = strcopyall (lastout);
	}

	itable[np].pstate = strcopyall (laststate);
	itable[np].nstate = strcopyall (lastnext);

	np++;
}
