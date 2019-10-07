/***************************************************************************
 * This program is Copyright (C) 1986, 1987, 1988 by Jonathan Payne.  JOVE *
 * is provided to you without charge, and with no warranty.  You may give  *
 * away copies of JOVE, including sources, provided that this notice is    *
 * included in all the files.                                              *
 ***************************************************************************/

#ifdef TXT_TO_C
#define STDIO
#endif
#ifdef LINT_ARGS
extern int
	abs(int);

extern void
	exit(int),
	_exit(int),
	*calloc(unsigned int, unsigned int),
#ifndef MAC
	free(void *),	/* returns an int on a Mac */
#endif
	*malloc(unsigned int),
	*realloc(void *, unsigned int);

extern char
	*getenv(char *),
	*tgoto(char *, int, int);

extern  char 
	**scanvec(char * *args,char *str),
	*IOerr(char *err,char *file),
	*MakeName(char *command),
	*StrIndex(int dir,char *buf,int charpos,char what),
	*ask(char *, char *, ...),
	*ask_buf(struct buffer *def),
	*ask_file(char *prmt,char *def,char *buf),
	*basename(char *f),
	*copystr(char *str),
	*do_ask(char *, int (*)(), char *, char *, ...),
	*emalloc(int size),
	*filename(struct buffer *b),
	*get_time(long *timep,char *buf,int from,int to),
	*getsearch(void),
	*itoa(int num),
	*lbptr(struct line *line),
	*lcontents(struct line *line),
	*ltobuf(struct line *line,char *buf),
	*pr_name(char *fname,int okay_home),
	*ralloc(char *obj,int size),
	*sprint(char *, ...),
	switchar(void);

extern  int 
	BufSwrite(int linenum),
	FLine(struct window *w),
	LineDist(struct line *nextp,struct line *endp),
	LookingAt(char *pattern,char *buf,int offset),
	ModBufs(int allp),
	ModMacs(void),
	Peekc(void),
	TwoBlank(void),
	UnixToBuf(char *,int ,int ,int , ...),
	addgetc(void),
	alphacomp(char * *a,char * *b),
	arg_type(void),
	arg_value(void),
	ask_int(char *prompt,int base),
	aux_complete(int c),
	blnkp(char *buf),
	calc_pos(char *lp,int c_char),
	casecmp(char *s1,char *s2),
	casencmp(char *s1,char *s2,int n),
	charp(void),
	chr_to_int(char *cp,int base,int allints, int *result),
	complete(char * *possible,char *prompt,int flags),
	do_if(char *cmd),
	dosputc(char c),
	f_getint(struct File *fp),
	f_gets(struct File *fp,char *buf,int max),
	f_match(char *file),
	filbuf(struct File *fp),
	find_pos(struct line *line,int c_char),
	fixorder(struct line * *line1,int *char1,struct line * *line2,int *char2),
	_flush(int c,struct File *fp),
	getch(void),
	getchar(void),
	getrawinchar(void),
	how_far(struct line *line,int col),
	i_blank(struct line *lp),
	i_bsblank(struct line *lp),
	in_window(struct window *windes,struct line *line),
	inlist(struct line *first,struct line *what),
	in_macro(void),
	inorder(struct line *nextp,int char1,struct line *endp,int char2),
	is_an_arg(void),
	ismword(int c),
	joverc(char *file),
	length(struct line *line),
	look_at(char *expr),
#ifdef IBMPC
	lower(char *c),
#endif
	mac_getc(void),
	match(char * *choices,char *what),
	max(int a,int b),
	min(int a,int b),
	numcomp(char *s1,char *s2),
	pnt_line(void),
	rawkey_ready(void),
	re_lindex(struct line *line,int offset,char *expr,char * *alts,int lbuf_okay),
	scandir(char *dir,char * * *nmptr,int (*qualify)(),int (*sorter)()),
	sindex(char *pattern,char *string),
	swrite(char *line,int inversep,int abortable),
	unbind_aux(int c),
	waitchar(int *slow),
	yes_or_no_p(char *, ...);

extern  disk_line 
	f_getputl(struct line *line,struct File *fp),
	putline(char *buf);

extern  struct File 
	*f_open(char *name,int flags,char *buffer,int buf_size),
	*fd_open(char *name,int flags,int fd,char *buffer,int bsize),
	*open_file(char *fname,char *buf,int how,int ifbad,int loudness);

extern  struct buffer 
	*buf_exists(char *name),
	*do_find(struct window *w,char *fname,int force),
	*do_select(struct window *w,char *name),
	*file_exists(char *name),
	*getNMbuf(void);

extern  struct cmd 
	*FindCmd(void (*proc)());
extern  struct data_obj 
	**IsPrefix(struct data_obj *cp),
	*findcom(char *prompt),
	*findmac(char *prompt),
	*findvar(char *prompt);

extern  struct line 
	*lastline(struct line *lp),
	*listput(struct buffer *buf,struct line *after),
	*max_line(struct line *l1,struct line *l2),
	*min_line(struct line *l1,struct line *l2),
	*nbufline(void),
	*next_line(struct line *line,int num),
	*prev_line(struct line *line,int num),
	*reg_delete(struct line *line1,int char1,struct line *line2,int char2);

extern  struct m_thread 
	*alloc_mthread(void);

extern  struct mark 
	*CurMark(void),
	*MakeMark(struct line *line,int column,int type);

extern  struct position 
	*DoYank(struct line *fline,int fchar,struct line *tline,int tchar,struct line *atline,int atchar,struct buffer *whatbuf),
	*c_indent(int incrmt),
	*docompiled(int dir,char *expr,char * *alts),
	*dosearch(char *pattern,int dir,int re),
	*lisp_indent(void),
	*m_paren(char p_type,int dir,int can_mismatch,int can_stop);

extern  struct table 
	*make_table(void);

extern  struct window 
	*div_wind(struct window *wp,int n),
	*w_nam_typ(char *name,int type),
	*windbp(struct buffer *bp);

extern  struct word 
	*word_in_table(char *text,struct table *table);

extern  unsigned char 
	chpl(void),
	lpp(void);

extern  void 
	AbbrevExpand(void),
	AddSpecial(void),
	AllMarkSet(struct buffer *b,struct line *line,int col),
	AppReg(void),
	Apropos(void),
	BList(void),
	BSexpr(void),
	BUpList(void),
	BackChar(void),
	BackPara(void),
	BackWord(void),
	BindAKey(void),
	BindMac(void),
	BindMtoW(void),
	BindSomething(struct data_obj *(*proc)()),
	BindWMap(struct data_obj * *map,int lastkey,struct data_obj *cmd),
	Bof(void),
	Bol(void),
	Bos(void),
	Bow(void),
	Buf10Select(void),
	Buf1Select(void),
	Buf2Select(void),
	Buf3Select(void),
	Buf4Select(void),
	Buf5Select(void),
	Buf6Select(void),
	Buf7Select(void),
	Buf8Select(void),
	Buf9Select(void),
	BufErase(void),
	BufKill(void),
	BufList(void),
	BufPos(void),
	BufSelect(void),
	CAutoExec(void),
	CalcWind(struct window *w),
	CapChar(void),
	CapWord(void),
	CasRegLower(void),
	CasRegUpper(void),
	CaseReg(int up),
	CentWind(struct window *w),
	ChkWindows(struct line *line1,struct line *line2),
	ChrToOct(void),
	ClAndRedraw(void),
	Comment(void),
	CopyRegion(void),
	CtlxPrefix(void),
	DFixMarks(struct line *line1,int char1,struct line *line2,int char2),
	DOTsave(struct position *buf),
	DefAutoExec(struct data_obj *(*proc)()),
	DefGAbbrev(void),
	DefKBDMac(void),
	DefMAbbrev(void),
	DelBlnkLines(void),
	DelCurWindow(void),
	DelMacro(void),
	DelMark(struct mark *m),
	DelNChar(void),
	DelNWord(void),
	DelPChar(void),
	DelPWord(void),
	DelReg(void),
	DelWtSpace(void),
	DescBindings(void),
	DescCom(void),
	DescMap(struct data_obj * *map,char *pref),
	DescWMap(struct data_obj * *map,int key),
	Digit(void),
	Digit0(void),
	Digit1(void),
	Digit2(void),
	Digit3(void),
	Digit4(void),
	Digit5(void),
	Digit6(void),
	Digit7(void),
	Digit8(void),
	Digit9(void),
	DoAutoExec(char *new,char *old),
	DoJustify(struct line *l1,int c1,struct line *l2,int c2,int scrunch,int indent),
	DoKeys(int nocmdline),
	DoNewline(int indentp),
	DoPara(int dir),
	DoParen(void),
	DoWriteReg(int app),
	DotTo(struct line *line,int col),
	DownScroll(void),
	DrawMesg(int abortable),
	EditAbbrevs(void),
	Eof(void),
	Eol(void),
	Eos(void),
	Eow(void),
	ErrFree(void),
	ErrParse(void),
	EscPrefix(void),
	ExecCmd(struct data_obj *cp),
	ExecMacro(void),
	Extend(void),
	FDotTag(void),
	FDownList(void),
	FList(void),
	FSexpr(void),
	FSrchND(void),
	FillComment(char *format),
	FilterRegion(void),
	FindFile(void),
	FindTag(void),
	ForChar(void),
	ForPara(void),
	ForSearch(void),
	ForWord(void),
	Forget(void),
	GCchunks(void),
	GSexpr(void),
	GoLine(void),
	GotoWind(void),
	GrowWindow(void),
	HandlePref(struct data_obj * *map),
	IFixMarks(struct line *line1,int char1,struct line *line2,int char2),
	IncFSearch(void),
	IncRSearch(void),
	InitCM(void),
	InsFile(void),
	Insert(int c),
	Justify(void),
	KeyDesc(void),
	KillBos(void),
	KillEOL(void),
	KillEos(void),
	KillExpr(void),
	KillSome(void),
	Leave(void),
	LineAI(void),
	LineInsert(int num),
	LowWord(void),
	MAutoExec(void),
	MacInter(void),
	MakeErrors(void),
	MarkSet(struct mark *m,struct line *line,int column),
	MiscPrefix(void),
	NameMac(void),
	Newline(void),
	NextError(void),
	NextLine(void),
	NextPage(void),
	NextWindow(void),
	NotModified(void),
	OneWindow(void),
	OpenLine(void),
	PageNWind(void),
	PageScrollDown(void),
	PageScrollUp(void),
	ParseAll(void),
	PathParse(char *name,char *intobuf),
	Placur(int line,int col),
	PopMark(void),
	PrVar(void),
	PrevError(void),
	PrevLine(void),
	PrevPage(void),
	PrevWindow(void),
	PtToMark(void),
	Push(void),
	PushPntp(struct line *line),
	QRepSearch(void),
	QuotChar(void),
	REcompile(char *pattern,int re,char *into_buf,char * *alt_bufp),
	RErecur(void),
	RSrchND(void),
	ReNamBuf(void),
	ReadFile(void),
	Recur(void),
	RedrawDisplay(void),
	RegJustify(void),
	RegReplace(void),
	RegToUnix(struct buffer *outbuf,char *cmd),
	Remember(void),
	RepSearch(void),
	ResetTerm(void),
	RestAbbrevs(void),
	RevSearch(void),
	RunMacro(void),
	SO_off(void),
	SO_on(void),
	SaveAbbrevs(void),
	SaveFile(void),
	SelfInsert(void),
	SetABuf(struct buffer *b),
	SetBuf(struct buffer *newbuf),
	SetDot(struct position *bp),
	SetLMargin(void),
	SetMark(void),
	SetRMargin(void),
	SetTop(struct window *w,struct line *line),
	SetVar(void),
	SetWind(struct window *new),
	ShToBuf(void),
	ShellCom(void),
	ShowErr(void),
	ShowVersion(void),
	ShrWindow(void),
	SitFor(unsigned int delay),
	Source(void),
	SplitWind(void),
	StrLength(void),
	SyncTmp(void),
	TOstart(char *name,int auto_newline),
	TOstop(void),
	Tab(void),
	TimesFour(void),
	ToError(int forward),
	ToFirst(void),
	ToIndent(void),
	ToLast(void),
	ToMark(struct mark *m),
	TogMinor(int bit),
	TransChar(void),
	TransLines(void),
	Typeout(char *, ...),
	UNIX_cmdline(int argc,char * *argv),
	UnbindC(void),
	Ungetc(int c),
	UnsetTerm(char *),
	UpScroll(void),
	UppWord(void),
	WNumLines(void),
	WVisSpace(void),
	WindFind(void),
	WindSize(struct window *w,int inc),
	WriteFile(void),
	WriteMacs(void),
	WrtReg(void),
	WtModBuf(void),
	XParse(void),
	Yank(void),
	YankPop(void),
	add_mess(char *, ...),
	add_stroke(int c),
	add_word(char *wname,struct table *table),
	b_char(int n),
	b_word(int num),
	bufname(struct buffer *b),
	case_reg(struct line *line1,int char1,struct line *line2,int char2,int up),
	case_word(int up),
	cl_eol(void),
	cl_scr(int doit),
	close_file(struct File *fp),
	clr_arg_value(void),
	clrline(char *cp1,char *cp2),
	complain(char *, ...),
	confirm(char *, ...),
	d_cache_init(void),
	del_char(int dir,int num),
	del_wind(struct window *wp),
	dispatch(int c),
	do_macro(struct macro *mac),
	do_rfill(int ulm),
	do_set_mark(struct line *l,int c),
	do_sgtty(void),
	do_space(void),
	dobell(int x),
	dofread(struct File *fp),
	dword(int forward),
	error(char *, ...),
	f_char(int n),
	f_close(struct File *fp),
	f_mess(char *, ...),
	f_readn(struct File *fp,char *addr,int n),
	f_seek(struct File *fp,long offset),
	f_toNL(struct File *fp),
	f_word(int num),
	file_backup(char *fname),
	file_write(char *fname,int app),
	filemunge(char *newname),
	find_para(int how),
	find_tag(char *tag,int localp),
	finish(int code),
	flush(struct File *fp),
	flusho(void),
	format(char *buf,int len,char *fmt,char *ap),
#ifndef STDIO
	fprintf(struct File *,char *, ...),
#endif
	fputnchar(char *s,int n,struct File *fp),
	free_mthread(struct m_thread *t),
	freedir(char * * *nmptr,int nentries),
	freeline(struct line *line),
	gc_openfiles(void),
	getTERM(void),
	getline(disk_line addr,char *buf),
	i_set(int nline,int ncol),
	init_43(void),
	init_strokes(void),
	init_term(void),
	initlist(struct buffer *b),
	ins_c(char c,char *buf,int atchar,int num,int max),
	ins_str(char *str,int ok_nl),
	insert_c(int c,int n),
	isprocbuf(char *bufname),
	len_error(int flag),
	lfreelist(struct line *first),
	lfreereg(struct line *line1,struct line *line2),
	line_move(int dir,int n,int line_cmd),
	linecopy(char *onto,int atchar,char *from),
	lremove(struct line *line1,struct line *line2),
	lsave(void),
	mac_init(void),
	mac_putc(int c),
#ifndef MAC
	main(int argc,char * *argv),
#endif
	make_argv(char * *argv,char *ap),
	make_scr(void),
	message(char *str),
	minib_add(char *str,int movedown),
	modify(void),
	mp_error(void),
	n_indent(int goal),
	negate_arg_value(void),
	null_ncpy(char *to,char *from,int n),
	open_lines(int n),
	patchup(struct line *line1,int char1,struct line *line2,int char2),
	pop_env(jmp_buf),
	pop_wind(char *name,int clobber,int btype),
	prCTIME(void),
	pr_putc(int c,struct File *fp),
#ifndef STDIO
	printf(char *, ...),
#endif
	push_env(jmp_buf),
	put_bufs(int askp),
	putmatch(int which,char *buf,int size),
	putpad(char *str,int lines),
	putreg(struct File *fp,struct line *line1,int char1,struct line *line2,int char2,int makesure),
	putstr(char *s),
	rbell(void),
	re_dosub(char *tobuf,int delp),
	read_file(char *file,int is_insert),
	redisplay(void),
	reg_kill(struct line *line2,int char2,int dot_moved),
	reset_43(void),
	s_mess(char *, ...),
	set_arg_value(int n),
	set_ino(struct buffer *b),
	set_is_an_arg(int there_is),
	set_mark(void),
	set_wsize(int wsize),
	setbname(struct buffer *b,char *name),
	setcolor(unsigned char fg,unsigned char bg),
	setfname(struct buffer *b,char *name),
	setsearch(char *str),
	settout(char *ttbuf),
	slowpoke(void),
#ifndef STDIO
	sprintf(char *,char *, ...),
#endif
	tiewind(struct window *w,struct buffer *bp),
	tmpclose(void),
	tmpinit(void),
	to_word(int dir),
	ttinit(void),
	ttsize(void),
	tty_reset(void),
	ttyset(int n),
	unmodify(void),
	unwind_macro_stack(void),
	updmode(void),
	v_clear(int line1,int line2),
	v_del_line(int num,int top,int bottom),
	v_ins_line(int num,int top,int bottom),
	winit(void),
	write_em(char *s),
	write_emc(char *s,int n),
	write_emif(),
	write_emif(char *s);

#ifdef MAC

extern int
	creat(char *,int),
	open(char *,int),
	close(int),
	read(int,char *,unsigned),
	write(int,char *,unsigned),
	free(char *);
	unlink(char *),
	chdir(char *),
	rawchkc(void),
	getArgs(char ***);
	
	
extern long 
	lseek(int,long,unsigned);

extern char 
	*pwd(void),
	*index(char *,char),
	*mktemp(char *),
	*rindex(char *,char),
	*getwd(char *),
	*pfile(char *),
	*gfile(char *);
	
extern void
	MacInit(void),
	InitBinds(void),
	NPlacur(int,int),
	i_lines(int,int,int),
	d_lines(int,int,int),
	clr_page(void),
	clr_eoln(void),
	docontrols(void),
	RemoveScrollBar(Window *),
	InitEvents(void),
	menus_on(void),
	menus_off(void);

#endif /* MAC */

#else

extern time_t
	time();

extern long	
	lseek();

extern int
	abs(),
	read(),
	write();

extern void
	exit(),
	_exit(),
	free();

extern char
	*getenv(),
	*pwd(),
	*index(),
	*malloc(),
	*mktemp(),
	*realloc(),
	*rindex(),
#ifdef IPROCS
	*pstate(),
#endif
	*tgoto();

extern  char 
	**scanvec(),
	*IOerr(),
	*MakeName(),
	*StrIndex(),
	*ask(),
	*ask_buf(),
	*ask_file(),
	*basename(),
	*copystr(),
	*do_ask(),
	*emalloc(),
	*filename(),
	*get_time(),
	*getsearch(),
	*itoa(),
	*lbptr(),
	*lcontents(),
	*ltobuf(),
	*pr_name(),
	*ralloc(),
	*sprint(),
	switchar();

extern  int 
	BufSwrite(),
	FLine(),
	LineDist(),
	LookingAt(),
	ModBufs(),
	ModMacs(),
	Peekc(),
	TwoBlank(),
	UnixToBuf(),
	addgetc(),
	alphacomp(),
	arg_type(),
	arg_value(),
	ask_int(),
	aux_complete(),
	blnkp(),
	calc_pos(),
	casecmp(),
	casencmp(),
	charp(),
	chr_to_int(),
	complete(),
	do_if(),
	dosputc(),
	f_getint(),
	f_gets(),
	f_match(),
	filbuf(),
	find_pos(),
	fixorder(),
	_flush(),
	getch(),
	getchar(),
	getrawinchar(),
	how_far(),
	i_blank(),
	i_bsblank(),
	in_window(),
	inlist(),
	in_macro(),
	inorder(),
	is_an_arg(),
	ismword(),
	joverc(),
	length(),
	look_at(),
#ifdef IBMPC
	lower(),
#endif	
	mac_getc(),
	match(),
	max(),
	min(),
	numcomp(),
	pnt_line(),
	rawkey_ready(),
	re_lindex(),
	scandir(),
	sindex(),
	swrite(),
	unbind_aux(),
	waitchar(),
	yes_or_no_p();

extern  disk_line 
	f_getputl(),
	putline();

extern  struct File 
	*f_open(),
	*fd_open(),
	*open_file();

extern  struct buffer 
	*buf_exists(),
	*do_find(),
	*do_select(),
	*file_exists(),
	*getNMbuf();

extern  struct cmd 
	*FindCmd();

extern  struct data_obj 
	**IsPrefix(),
	*findcom(),
	*findmac(),
	*findvar();

extern  struct line 
	*lastline(),
	*listput(),
	*max_line(),
	*min_line(),
	*nbufline(),
	*next_line(),
	*prev_line(),
	*reg_delete();

extern  struct m_thread 
	*alloc_mthread();

extern  struct mark 
	*CurMark(),
	*MakeMark();

extern  struct position 
	*DoYank(),
	*c_indent(),
	*docompiled(),
	*dosearch(),
	*lisp_indent(),
	*m_paren();

extern  struct table 
	*make_table();

extern  struct window 
	*div_wind(),
	*w_nam_typ(),
	*windbp();

extern  struct word 
	*word_in_table();

extern  unsigned char 
	chpl(),
	lpp();

extern  void 
	AbbrevExpand(),
	AddSpecial(),
	AllMarkSet(),
	AppReg(),
	Apropos(),
	BList(),
	BSexpr(),
	BUpList(),
	BackChar(),
	BackPara(),
	BackWord(),
	BindAKey(),
	BindMac(),
	BindMtoW(),
	BindSomething(),
	BindWMap(),
	Bof(),
	Bol(),
	Bos(),
	Bow(),
	Buf10Select(),
	Buf1Select(),
	Buf2Select(),
	Buf3Select(),
	Buf4Select(),
	Buf5Select(),
	Buf6Select(),
	Buf7Select(),
	Buf8Select(),
	Buf9Select(),
	BufErase(),
	BufKill(),
	BufList(),
	BufPos(),
	BufSelect(),
	CAutoExec(),
	CalcWind(),
	CapChar(),
	CapWord(),
	CasRegLower(),
	CasRegUpper(),
	CaseReg(),
	CentWind(),
	ChkWindows(),
	ChrToOct(),
	ClAndRedraw(),
	Comment(),
	CopyRegion(),
	CtlxPrefix(),
	DFixMarks(),
	DOTsave(),
	DefAutoExec(),
	DefGAbbrev(),
	DefKBDMac(),
	DefMAbbrev(),
	DelBlnkLines(),
	DelCurWindow(),
	DelMacro(),
	DelMark(),
	DelNChar(),
	DelNWord(),
	DelPChar(),
	DelPWord(),
	DelReg(),
	DelWtSpace(),
	DescBindings(),
	DescCom(),
	DescMap(),
	DescWMap(),
	Digit(),
	Digit0(),
	Digit1(),
	Digit2(),
	Digit3(),
	Digit4(),
	Digit5(),
	Digit6(),
	Digit7(),
	Digit8(),
	Digit9(),
	DoAutoExec(),
	DoJustify(),
	DoKeys(),
	DoNewline(),
	DoPara(),
	DoParen(),
	DoWriteReg(),
	DotTo(),
	DownScroll(),
	DrawMesg(),
	EditAbbrevs(),
	Eof(),
	Eol(),
	Eos(),
	Eow(),
	ErrFree(),
	ErrParse(),
	EscPrefix(),
	ExecCmd(),
	ExecMacro(),
	Extend(),
	FDotTag(),
	FDownList(),
	FList(),
	FSexpr(),
	FSrchND(),
	FillComment(),
	FilterRegion(),
	FindFile(),
	FindTag(),
	ForChar(),
	ForPara(),
	ForSearch(),
	ForWord(),
	Forget(),
	GCchunks(),
	GSexpr(),
	GoLine(),
	GotoWind(),
	GrowWindow(),
	HandlePref(),
	IFixMarks(),
	IncFSearch(),
	IncRSearch(),
	InitCM(),
	InsFile(),
	Insert(),
	Justify(),
	KeyDesc(),
	KillBos(),
	KillEOL(),
	KillEos(),
	KillExpr(),
	KillSome(),
	Leave(),
	LineAI(),
	LineInsert(),
	LowWord(),
	MAutoExec(),
	MacInter(),
	MakeErrors(),
	MarkSet(),
	MiscPrefix(),
	NameMac(),
	Newline(),
	NextError(),
	NextLine(),
	NextPage(),
	NextWindow(),
	NotModified(),
	OneWindow(),
	OpenLine(),
	PageNWind(),
	PageScrollDown(),
	PageScrollUp(),
	ParseAll(),
	PathParse(),
	Placur(),
	PopMark(),
	PrVar(),
	PrevError(),
	PrevLine(),
	PrevPage(),
	PrevWindow(),
	PtToMark(),
	Push(),
	PushPntp(),
	QRepSearch(),
	QuotChar(),
	REcompile(),
	RErecur(),
	RSrchND(),
	ReNamBuf(),
	ReadFile(),
	Recur(),
	RedrawDisplay(),
	RegJustify(),
	RegReplace(),
	RegToUnix(),
	Remember(),
	RepSearch(),
	ResetTerm(),
	RestAbbrevs(),
	RevSearch(),
	RunMacro(),
	SO_off(),
	SO_on(),
	SaveAbbrevs(),
	SaveFile(),
	SelfInsert(),
	SetABuf(),
	SetBuf(),
	SetDot(),
	SetLMargin(),
	SetMark(),
	SetRMargin(),
	SetTop(),
	SetVar(),
	SetWind(),
	ShToBuf(),
	ShellCom(),
	ShowErr(),
	ShowVersion(),
	ShrWindow(),
	SitFor(),
	Source(),
	SplitWind(),
	StrLength(),
	SyncTmp(),
	TOstart(),
	TOstop(),
	Tab(),
	TimesFour(),
	ToError(),
	ToFirst(),
	ToIndent(),
	ToLast(),
	ToMark(),
	TogMinor(),
	TransChar(),
	TransLines(),
	Typeout(),
	UNIX_cmdline(),
	UnbindC(),
	Ungetc(),
	UnsetTerm(),
	UpScroll(),
	UppWord(),
	WNumLines(),
	WVisSpace(),
	WindFind(),
	WindSize(),
	WriteFile(),
	WriteMacs(),
	WrtReg(),
	WtModBuf(),
	XParse(),
	Yank(),
	YankPop(),
	add_mess(),
	add_stroke(),
	add_word(),
	b_char(),
	b_word(),
	bufname(),
	case_reg(),
	case_word(),
	cl_eol(),
	cl_scr(),
	close_file(),
	clr_arg_value(),
	clrline(),
	complain(),
	confirm(),
	d_cache_init(),
	del_char(),
	del_wind(),
	dispatch(),
	do_macro(),
	do_rfill(),
	do_set_mark(),
	do_sgtty(),
	do_space(),
	dobell(),
	dofread(),
	dword(),
	error(),
	f_char(),
	f_close(),
	f_mess(),
	f_readn(),
	f_seek(),
	f_toNL(),
	f_word(),
	file_backup(),
	file_write(),
	filemunge(),
	find_para(),
	find_tag(),
	finish(),
	flush(),
	flusho(),
	format(),
#ifndef STDIO
	fprintf(),
#endif	
	fputnchar(),
	free_mthread(),
	freedir(),
	freeline(),
	gc_openfiles(),
	getTERM(),
	getline(),
	i_set(),
	init_43(),
	init_strokes(),
	init_term(),
	initlist(),
	ins_c(),
	ins_str(),
	insert_c(),
	isprocbuf(),
	len_error(),
	lfreelist(),
	lfreereg(),
	line_move(),
	linecopy(),
	lremove(),
	lsave(),
	mac_init(),
	mac_putc(),
	main(),
	make_argv(),
	make_scr(),
	message(),
	minib_add(),
	modify(),
	mp_error(),
	n_indent(),
	negate_arg_value(),
	null_ncpy(),
	open_lines(),
	patchup(),
	pop_env(),
	pop_wind(),
	prCTIME(),
	pr_putc(),
#ifndef STDIO
	printf(),
#endif	
	push_env(),
	put_bufs(),
	putmatch(),
	putpad(),
	putreg(),
	putstr(),
	rbell(),
	re_dosub(),
	read_file(),
	redisplay(),
	reg_kill(),
	reset_43(),
	s_mess(),
	set_arg_value(),
	set_ino(),
	set_is_an_arg(),
	set_mark(),
	set_wsize(),
	setbname(),
	setcolor(),
	setfname(),
	setsearch(),
	settout(),
	slowpoke(),
#ifndef STDIO
	sprintf(),
#endif	
	tiewind(),
	tmpclose(),
	tmpinit(),
	to_word(),
	ttinit(),
	ttsize(),
	tty_reset(),
	ttyset(),
	unmodify(),
	unwind_macro_stack(),
	updmode(),
	v_clear(),
	v_del_line(),
	v_ins_line(),
	winit(),
	write_em(),
	write_emc(),
	write_emif(),
	write_emif();
#endif
