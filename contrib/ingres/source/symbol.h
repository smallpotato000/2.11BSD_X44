#
/*
**	COPYRIGHT
**
**	The Regents of the University of California
**
**	1977
**
**	This program material is the property of the
**	Regents of the University of California and
**	may not be reproduced or disclosed without
**	the prior written permission of the owner.
*/



# define		AND		0
# define		OR		1
# define		UOP		2		/* UNARY OPERATORS */
# define		opPLUS		0
# define		opMINUS		1
# define		opNOT		2
# define		opATAN		3
# define		opCOS		4
# define		opGAMMA		5
# define		opLOG		6
# define		opASCII		7
# define		opSIN		8
# define		opSQRT		9
# define		opABS		10
# define		opEXP		11
# define		opINT1		12
# define		opINT2		13
# define		opINT4		14
# define		opFLOAT4	15
# define		opFLOAT8	16

# define		BOP			3		/* BINARY OPERATORS */
# define		opADD		0
# define		opSUB		1
# define		opMUL		2
# define		opDIV		3
# define		opPOW		4
# define		opEQ		5
# define		opNE		6
# define		opLT		7
# define		opLE		8
# define		opGT		9
# define		opGE		10
# define		opMOD		11
# define		opCONCAT	12

# define		AOP			4		/* AGGREGATE OPERATORS */
# define		opCOUNT		0
# define		opCOUNTU	1
# define		opSUM		2
# define		opSUMU		3
# define		opAVG		4
# define		opAVGU		5
# define		opMIN		6
# define		opMAX		7
# define		opANY		8

# define		COP			5		/* CONSTANT OPERATORS */
# define		BADCOP		-1		/* error return on lookup */
# define		opDBA		0
# define		opUSERCODE	1
# define		opDATE		2
# define		opTIME		3


# define		RESDOM		11		/* RESULT DOMAIN NUMBER */
# define		VAR			12		/* VARIABLE */
# define		S_VAR		13		/* variable for which
 	 	 	 	 	 	 	 	 	 * a substitution has been done,
 	 	 	 	 	 	 	 	 	 * Only occurs in the ovqp-decomp
 	 	 	 	 	 	 	 	 	 * merged process.
 	 	 	 	 	 	 	 	 	 */

# define		QMODE		16	/* QUERY MODE */
# define		mdRETTERM	0	/* retrieve to terminal, used by decomp-ovqp */
# define		mdRETR		1	/* retrieve, into rel perhaps */
# define		mdAPP		2	/* append to rel */
# define		mdREPL		3	/* replace in rel */
# define		mdDEL		4	/* delete from rel */
# define		mdCOPY		5	/* copy into/from rel */
# define		mdCREATE	6	/* create new relation */
# define		mdDESTROY	7	/* destroy old relation */
# define		mdHELP		8	/* print info on relation */
# define		mdINDEX		9	/* create secondary index */
# define		mdMODIFY	10	/* change access structure of rel */
# define		mdPRINT		11	/* print rel on terminal */
# define		mdRANGE		12	/* declare range variable */
# define		mdSAVE		13	/* save relation date */
# define		mdDEFINE	14	/* "here is a tree", to qrymod */
# define		mdRET_UNI	15	/* retrieve unique */
# define		mdVIEW		16	/* extra info for view def, to qrymod */
# define		mdUPDATE	17	/* update processor */
# define		mdRESETREL	18	/* modify to truncated */
# define		mdDISPLAY	19	/* dump qm decl */
# define		mdNETQRY	20	/* reserved for distr ingres */
# define		mdMOVEREL	21	/* reserved for distr ingres */
# define		mdPROT		22	/* extra info for permit def, to qrymod */
# define		mdINTEG		23	/* extra info for integrity def, to qrymod */
# define		mdDCREATE	24	/* reserved for distr ingres */
# define		mdWAITQRY	25	/* reserved for distr ingres */
# define		mdREMQM		26	/* remove permit integrity definition */
# define		mdDISTRIB	27	/* reserved for distr ingres */

# define		ROOT		20	/* ROOT of QUERY TREE */
# define		QLEND		21	/* NULL branch at end of QUAL */
# define		TREE		25	/* SIGNALS BEGINNING of TREE */
# define		BYHEAD		22	/* BY LIST HEAD */
# define		AGHEAD		23	/* AGGREGATE HEAD */

# define		SITE		26	/* reserved for distr ingres */

# define		INT			30	/* INTEGER CONSTANT */
# define		FLOAT		31	/* FLOATING POINT CONSTANT */
# define		CHAR		32	/* CHARACTER CONSTANT */
# define		BINARY		33	/* BINARY CONSTANT (used in printup */


# define		RESULTVAR	40	/* RESULT RELATION VAR NO. */
# define		SOURCEID	41	/* SOURCE RELATION NAME(S) */
# define		RESULTID	42	/* RESULT RELATION NAME	*/


# define		CID		29		/* FOR INTEGRITY CONSTRAINTS */


# define		CHANGESTRAT	50	/* FOR OVQP TO DEVISE NEW STRATEGY */
# define		USERQRY		51	/* Tells OVQP that the result rel is a user relation */
# define		EXIT		52	/* tells OVQP that query is done */
# define		ACK			1	/* OVQP should write an acknowledging EOP */
# define		NOACK		2	/* OVQP should not acknowledge */
# define		RUBACK		3	/* internal mode for DECOMP */
# define		REOPENRES	54	/* OVQP should reopen result relation */
# define		RETVAL		55	/* RETURN FROM OVQP */
# define		UPDATE		-3	/* tells DECOMP to call update */
# define		NOUPDATE	-2	/* tells DECOMP not to call update */
# define		EMPTY		-1
# define		NONEMPTY	0
# define		EOTUP		56	/* end of tuple symbol send by OVQP to equel */

/*	The following are pattern matching symbols used by ovqp */
# define		PAT_ANY		1	/* matches 0 or more char */
# define		PAT_ONE		2	/* matches any one char */
# define		PAT_LBRAC	3	/* left bracket for char groupings */
# define		PAT_RBRAC	4	/* right bracket for char groupings */
