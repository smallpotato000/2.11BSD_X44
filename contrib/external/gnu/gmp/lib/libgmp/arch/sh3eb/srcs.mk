
C_SRCS_LIST= \
	random.c		mpn/generic/random.c \
	toom_interpolate_7pts.c		mpn/generic/toom_interpolate_7pts.c \
	divrem_2.c		mpn/generic/divrem_2.c \
	sbpi1_divappr_q.c		mpn/generic/sbpi1_divappr_q.c \
	random2.c		mpn/generic/random2.c \
	mu_bdiv_q.c		mpn/generic/mu_bdiv_q.c \
	mulmid_basecase.c		mpn/generic/mulmid_basecase.c \
	jacobi_2.c		mpn/generic/jacobi_2.c \
	toom32_mul.c		mpn/generic/toom32_mul.c \
	toom2_sqr.c		mpn/generic/toom2_sqr.c \
	toom8h_mul.c		mpn/generic/toom8h_mul.c \
	toom44_mul.c		mpn/generic/toom44_mul.c \
	zero.c		mpn/generic/zero.c \
	mod_1_4.c		mpn/generic/mod_1_4.c \
	gcdext.c		mpn/generic/gcdext.c \
	hamdist.c		mpn/generic/popham.c \
	sec_powm.c		mpn/generic/sec_powm.c \
	add_err3_n.c		mpn/generic/add_err3_n.c \
	binvert.c		mpn/generic/binvert.c \
	mu_div_q.c		mpn/generic/mu_div_q.c \
	and_n.c		mpn/generic/logops_n.c \
	invertappr.c		mpn/generic/invertappr.c \
	add_n_sub_n.c		mpn/generic/add_n_sub_n.c \
	dump.c		mpn/generic/dump.c \
	mu_divappr_q.c		mpn/generic/mu_divappr_q.c \
	dcpi1_div_qr.c		mpn/generic/dcpi1_div_qr.c \
	hgcd_reduce.c		mpn/generic/hgcd_reduce.c \
	matrix22_mul1_inverse_vector.c		mpn/generic/matrix22_mul1_inverse_vector.c \
	toom6_sqr.c		mpn/generic/toom6_sqr.c \
	divrem_1.c		mpn/generic/divrem_1.c \
	hgcd_step.c		mpn/generic/hgcd_step.c \
	sub_err3_n.c		mpn/generic/sub_err3_n.c \
	mod_1.c		mpn/generic/mod_1.c \
	toom42_mulmid.c		mpn/generic/toom42_mulmid.c \
	sec_div_qr.c		mpn/generic/sec_div.c \
	andn_n.c		mpn/generic/logops_n.c \
	divexact.c		mpn/generic/divexact.c \
	jacobi.c		mpn/generic/jacobi.c \
	powlo.c		mpn/generic/powlo.c \
	mul.c		mpn/generic/mul.c \
	set_str.c		mpn/generic/set_str.c \
	toom42_mul.c		mpn/generic/toom42_mul.c \
	toom_interpolate_6pts.c		mpn/generic/toom_interpolate_6pts.c \
	toom54_mul.c		mpn/generic/toom54_mul.c \
	dcpi1_divappr_q.c		mpn/generic/dcpi1_divappr_q.c \
	copyd.c		mpn/generic/copyd.c \
	toom_eval_dgr3_pm2.c		mpn/generic/toom_eval_dgr3_pm2.c \
	mod_1_3.c		mpn/generic/mod_1_3.c \
	com.c		mpn/generic/com.c \
	xor_n.c		mpn/generic/logops_n.c \
	sec_tabselect.c		mpn/generic/sec_tabselect.c \
	copyi.c		mpn/generic/copyi.c \
	nior_n.c		mpn/generic/logops_n.c \
	toom_couple_handling.c		mpn/generic/toom_couple_handling.c \
	lshift.c		mpn/generic/lshift.c \
	sbpi1_bdiv_r.c		mpn/generic/sbpi1_bdiv_r.c \
	add.c		mpn/generic/add.c \
	div_qr_2.c		mpn/generic/div_qr_2.c \
	toom_interpolate_12pts.c		mpn/generic/toom_interpolate_12pts.c \
	perfsqr.c		mpn/generic/perfsqr.c \
	toom53_mul.c		mpn/generic/toom53_mul.c \
	toom_eval_pm2exp.c		mpn/generic/toom_eval_pm2exp.c \
	mu_div_qr.c		mpn/generic/mu_div_qr.c \
	toom_interpolate_16pts.c		mpn/generic/toom_interpolate_16pts.c \
	mod_34lsub1.c		mpn/generic/mod_34lsub1.c \
	strongfibo.c		mpn/generic/strongfibo.c \
	bdiv_q.c		mpn/generic/bdiv_q.c \
	sec_invert.c		mpn/generic/sec_invert.c \
	toom22_mul.c		mpn/generic/toom22_mul.c \
	bsqrtinv.c		mpn/generic/bsqrtinv.c \
	toom4_sqr.c		mpn/generic/toom4_sqr.c \
	rshift.c		mpn/generic/rshift.c \
	div_q.c		mpn/generic/div_q.c \
	jacbase.c		mpn/generic/jacbase.c \
	sec_sqr.c		mpn/generic/sec_sqr.c \
	hgcd_matrix.c		mpn/generic/hgcd_matrix.c \
	toom_eval_dgr3_pm1.c		mpn/generic/toom_eval_dgr3_pm1.c \
	toom33_mul.c		mpn/generic/toom33_mul.c \
	mullo_n.c		mpn/generic/mullo_n.c \
	mod_1_2.c		mpn/generic/mod_1_2.c \
	gcd_22.c		mpn/generic/gcd_22.c \
	sqrlo.c		mpn/generic/sqrlo.c \
	sub_1.c		mpn/generic/sub_1.c \
	add_err2_n.c		mpn/generic/add_err2_n.c \
	trialdiv.c		mpn/generic/trialdiv.c \
	add_1.c		mpn/generic/add_1.c \
	sqr_basecase.c		mpn/generic/sqr_basecase.c \
	toom_interpolate_5pts.c		mpn/generic/toom_interpolate_5pts.c \
	sbpi1_bdiv_q.c		mpn/generic/sbpi1_bdiv_q.c \
	pre_mod_1.c		mpn/generic/pre_mod_1.c \
	hgcd.c		mpn/generic/hgcd.c \
	bdiv_dbm1c.c		mpn/generic/bdiv_dbm1c.c \
	div_qr_1.c		mpn/generic/div_qr_1.c \
	sqrtrem.c		mpn/generic/sqrtrem.c \
	bdiv_q_1.c		mpn/generic/bdiv_q_1.c \
	toom63_mul.c		mpn/generic/toom63_mul.c \
	gcdext_1.c		mpn/generic/gcdext_1.c \
	div_qr_2u_pi1.c		mpn/generic/div_qr_2u_pi1.c \
	toom8_sqr.c		mpn/generic/toom8_sqr.c \
	mul_basecase.c		mpn/generic/mul_basecase.c \
	addmul_1.c		mpn/generic/addmul_1.c \
	neg.c		mpn/generic/neg.c \
	gcdext_lehmer.c		mpn/generic/gcdext_lehmer.c \
	divis.c		mpn/generic/divis.c \
	dcpi1_div_q.c		mpn/generic/dcpi1_div_q.c \
	sec_div_r.c		mpn/generic/sec_div.c \
	mul_1.c		mpn/generic/mul_1.c \
	toom_eval_pm2.c		mpn/generic/toom_eval_pm2.c \
	toom62_mul.c		mpn/generic/toom62_mul.c \
	hgcd2.c		mpn/generic/hgcd2.c \
	comb_tables.c		mpn/generic/comb_tables.c \
	sbpi1_bdiv_qr.c		mpn/generic/sbpi1_bdiv_qr.c \
	sub_err2_n.c		mpn/generic/sub_err2_n.c \
	scan1.c		mpn/generic/scan1.c \
	brootinv.c		mpn/generic/brootinv.c \
	pre_divrem_1.c		mpn/generic/pre_divrem_1.c \
	perfpow.c		mpn/generic/perfpow.c \
	get_str.c		mpn/generic/get_str.c \
	mulmod_bnm1.c		mpn/generic/mulmod_bnm1.c \
	mullo_basecase.c		mpn/generic/mullo_basecase.c \
	ior_n.c		mpn/generic/logops_n.c \
	tdiv_qr.c		mpn/generic/tdiv_qr.c \
	sec_pi1_div_qr.c		mpn/generic/sec_pi1_div.c \
	div_qr_2n_pi1.c		mpn/generic/div_qr_2n_pi1.c \
	toom43_mul.c		mpn/generic/toom43_mul.c \
	mod_1_1.c		mpn/generic/mod_1_1.c \
	matrix22_mul.c		mpn/generic/matrix22_mul.c \
	sec_pi1_div_r.c		mpn/generic/sec_pi1_div.c \
	xnor_n.c		mpn/generic/logops_n.c \
	iorn_n.c		mpn/generic/logops_n.c \
	divrem.c		mpn/generic/divrem.c \
	bsqrt.c		mpn/generic/bsqrt.c \
	gcd_1.c		mpn/generic/gcd_1.c \
	dcpi1_bdiv_qr.c		mpn/generic/dcpi1_bdiv_qr.c \
	mul_n.c		mpn/generic/mul_n.c \
	redc_2.c		mpn/generic/redc_2.c \
	submul_1.c		mpn/generic/submul_1.c \
	gcd_11.c		mpn/generic/gcd_11.c \
	toom6h_mul.c		mpn/generic/toom6h_mul.c \
	sqrmod_bnm1.c		mpn/generic/sqrmod_bnm1.c \
	mul_fft.c		mpn/generic/mul_fft.c \
	mulmid.c		mpn/generic/mulmid.c \
	powm.c		mpn/generic/powm.c \
	compute_powtab.c		mpn/generic/compute_powtab.c \
	rootrem.c		mpn/generic/rootrem.c \
	cnd_sub_n.c		mpn/generic/cnd_sub_n.c \
	mode1o.c		mpn/generic/mode1o.c \
	cnd_add_n.c		mpn/generic/cnd_add_n.c \
	toom_interpolate_8pts.c		mpn/generic/toom_interpolate_8pts.c \
	remove.c		mpn/generic/remove.c \
	lshiftc.c		mpn/generic/lshiftc.c \
	sec_mul.c		mpn/generic/sec_mul.c \
	dive_1.c		mpn/generic/dive_1.c \
	cmp.c		mpn/generic/cmp.c \
	toom_eval_pm1.c		mpn/generic/toom_eval_pm1.c \
	nand_n.c		mpn/generic/logops_n.c \
	hgcd_appr.c		mpn/generic/hgcd_appr.c \
	cnd_swap.c		mpn/generic/cnd_swap.c \
	scan0.c		mpn/generic/scan0.c \
	gcd_subdiv_step.c		mpn/generic/gcd_subdiv_step.c \
	sbpi1_div_qr.c		mpn/generic/sbpi1_div_qr.c \
	invert.c		mpn/generic/invert.c \
	sub.c		mpn/generic/sub.c \
	sqrlo_basecase.c		mpn/generic/sqrlo_basecase.c \
	toom_eval_pm2rexp.c		mpn/generic/toom_eval_pm2rexp.c \
	sec_sub_1.c		mpn/generic/sec_aors_1.c \
	broot.c		mpn/generic/broot.c \
	sec_add_1.c		mpn/generic/sec_aors_1.c \
	popcount.c		mpn/generic/popham.c \
	dcpi1_bdiv_q.c		mpn/generic/dcpi1_bdiv_q.c \
	hgcd2_jacobi.c		mpn/generic/hgcd2_jacobi.c \
	add_err1_n.c		mpn/generic/add_err1_n.c \
	mulmid_n.c		mpn/generic/mulmid_n.c \
	redc_1.c		mpn/generic/redc_1.c \
	sqr.c		mpn/generic/sqr.c \
	nussbaumer_mul.c		mpn/generic/nussbaumer_mul.c \
	zero_p.c		mpn/generic/zero_p.c \
	mu_bdiv_qr.c		mpn/generic/mu_bdiv_qr.c \
	fib2m.c		mpn/generic/fib2m.c \
	pow_1.c		mpn/generic/pow_1.c \
	get_d.c		mpn/generic/get_d.c \
	toom52_mul.c		mpn/generic/toom52_mul.c \
	sbpi1_div_q.c		mpn/generic/sbpi1_div_q.c \
	diveby3.c		mpn/generic/diveby3.c \
	fib2_ui.c		mpn/generic/fib2_ui.c \
	bdiv_qr.c		mpn/generic/bdiv_qr.c \
	hgcd_jacobi.c		mpn/generic/hgcd_jacobi.c \
	div_qr_1n_pi1.c		mpn/generic/div_qr_1n_pi1.c \
	toom3_sqr.c		mpn/generic/toom3_sqr.c \
	sizeinbase.c		mpn/generic/sizeinbase.c \
	gcd.c		mpn/generic/gcd.c \
	redc_n.c		mpn/generic/redc_n.c \
	sub_err1_n.c		mpn/generic/sub_err1_n.c \

ASM_SRCS_LIST= \
	add_n.asm		mpn/sh/add_n.asm \
	sub_n.asm		mpn/sh/sub_n.asm \
