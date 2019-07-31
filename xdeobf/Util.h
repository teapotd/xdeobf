#pragma once

inline int bitCount(uint32 v) {
	// https://graphics.stanford.edu/~seander/bithacks.html
	v = v - ((v >> 1) & 0x55555555);
	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
	return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
}

static const char *mmatToString(mba_maturity_t mat) {
	switch (mat) {
	case MMAT_ZERO:         return "MMAT_ZERO";
	case MMAT_GENERATED:    return "MMAT_GENERATED";
	case MMAT_PREOPTIMIZED: return "MMAT_PREOPTIMIZED";
	case MMAT_LOCOPT:       return "MMAT_LOCOPT";
	case MMAT_CALLS:        return "MMAT_CALLS";
	case MMAT_GLBOPT1:      return "MMAT_GLBOPT1";
	case MMAT_GLBOPT2:      return "MMAT_GLBOPT2";
	case MMAT_GLBOPT3:      return "MMAT_GLBOPT3";
	case MMAT_LVARS:        return "MMAT_LVARS";
	default:                return "MMAT_UNKNOWN";
	}
}

static const char *moptToString(mopt_t t) {
	switch (t) {
	case mop_z:   return "mop_z";
	case mop_r:   return "mop_r";
	case mop_n:   return "mop_n";
	case mop_str: return "mop_str";
	case mop_d:   return "mop_d";
	case mop_S:   return "mop_S";
	case mop_v:   return "mop_v";
	case mop_b:   return "mop_b";
	case mop_f:   return "mop_f";
	case mop_l:   return "mop_l";
	case mop_a:   return "mop_a";
	case mop_h:   return "mop_h";
	case mop_c:   return "mop_c";
	case mop_fn:  return "mop_fn";
	case mop_p:   return "mop_p";
	case mop_sc:  return "mop_sc";
	default:      return "???";
	};
}

static qstring minsnToString(minsn_t *o) {
	char outBuf[512];

	switch (o->opcode) {
	case m_nop: snprintf(outBuf, sizeof(outBuf), "m_nop"); break;
	case m_stx: snprintf(outBuf, sizeof(outBuf), "m_stx(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_ldx: snprintf(outBuf, sizeof(outBuf), "m_ldx(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_ldc: snprintf(outBuf, sizeof(outBuf), "m_ldc(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_mov: snprintf(outBuf, sizeof(outBuf), "m_mov(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_neg: snprintf(outBuf, sizeof(outBuf), "m_neg(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_lnot: snprintf(outBuf, sizeof(outBuf), "m_lnot(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_bnot: snprintf(outBuf, sizeof(outBuf), "m_bnot(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_xds: snprintf(outBuf, sizeof(outBuf), "m_xds(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_xdu: snprintf(outBuf, sizeof(outBuf), "m_xdu(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_low: snprintf(outBuf, sizeof(outBuf), "m_low(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_high: snprintf(outBuf, sizeof(outBuf), "m_high(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_add: snprintf(outBuf, sizeof(outBuf), "m_add(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_sub: snprintf(outBuf, sizeof(outBuf), "m_sub(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_mul: snprintf(outBuf, sizeof(outBuf), "m_mul(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_udiv: snprintf(outBuf, sizeof(outBuf), "m_udiv(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_sdiv: snprintf(outBuf, sizeof(outBuf), "m_sdiv(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_umod: snprintf(outBuf, sizeof(outBuf), "m_umod(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_smod: snprintf(outBuf, sizeof(outBuf), "m_smod(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_or: snprintf(outBuf, sizeof(outBuf), "m_or(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_and: snprintf(outBuf, sizeof(outBuf), "m_and(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_xor: snprintf(outBuf, sizeof(outBuf), "m_xor(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_shl: snprintf(outBuf, sizeof(outBuf), "m_shl(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_shr: snprintf(outBuf, sizeof(outBuf), "m_shr(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_sar: snprintf(outBuf, sizeof(outBuf), "m_sar(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_cfadd: snprintf(outBuf, sizeof(outBuf), "m_cfadd(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_ofadd: snprintf(outBuf, sizeof(outBuf), "m_ofadd(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_cfshl: snprintf(outBuf, sizeof(outBuf), "m_cfshl(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_cfshr: snprintf(outBuf, sizeof(outBuf), "m_cfshr(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_sets: snprintf(outBuf, sizeof(outBuf), "m_sets(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_seto: snprintf(outBuf, sizeof(outBuf), "m_seto(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_setp: snprintf(outBuf, sizeof(outBuf), "m_setp(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_setnz: snprintf(outBuf, sizeof(outBuf), "m_setnz(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_setz: snprintf(outBuf, sizeof(outBuf), "m_setz(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_setae: snprintf(outBuf, sizeof(outBuf), "m_setae(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_setb: snprintf(outBuf, sizeof(outBuf), "m_setb(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_seta: snprintf(outBuf, sizeof(outBuf), "m_seta(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_setbe: snprintf(outBuf, sizeof(outBuf), "m_setbe(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_setg: snprintf(outBuf, sizeof(outBuf), "m_setg(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_setge: snprintf(outBuf, sizeof(outBuf), "m_setge(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_setl: snprintf(outBuf, sizeof(outBuf), "m_setl(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_setle: snprintf(outBuf, sizeof(outBuf), "m_setle(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_jcnd: snprintf(outBuf, sizeof(outBuf), "m_jcnd(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_jnz: snprintf(outBuf, sizeof(outBuf), "m_jnz(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_jz: snprintf(outBuf, sizeof(outBuf), "m_jz(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_jae: snprintf(outBuf, sizeof(outBuf), "m_jae(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_jb: snprintf(outBuf, sizeof(outBuf), "m_jb(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_ja: snprintf(outBuf, sizeof(outBuf), "m_ja(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_jbe: snprintf(outBuf, sizeof(outBuf), "m_jbe(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_jg: snprintf(outBuf, sizeof(outBuf), "m_jg(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_jge: snprintf(outBuf, sizeof(outBuf), "m_jge(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_jl: snprintf(outBuf, sizeof(outBuf), "m_jl(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_jle: snprintf(outBuf, sizeof(outBuf), "m_jle(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_jtbl: snprintf(outBuf, sizeof(outBuf), "m_jtbl(%s,%s)", moptToString(o->l.t), moptToString(o->r.t)); break;
	case m_ijmp: snprintf(outBuf, sizeof(outBuf), "m_ijmp(%s,%s)", moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_goto: snprintf(outBuf, sizeof(outBuf), "m_goto(%s)", moptToString(o->l.t)); break;
	case m_call: snprintf(outBuf, sizeof(outBuf), "m_call(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_icall: snprintf(outBuf, sizeof(outBuf), "m_icall(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_ret: snprintf(outBuf, sizeof(outBuf), "m_ret"); break;
	case m_push: snprintf(outBuf, sizeof(outBuf), "m_push(%s)", moptToString(o->l.t)); break;
	case m_pop: snprintf(outBuf, sizeof(outBuf), "m_pop(%s)", moptToString(o->d.t)); break;
	case m_und: snprintf(outBuf, sizeof(outBuf), "m_und(%s)", moptToString(o->d.t)); break;
	case m_ext: snprintf(outBuf, sizeof(outBuf), "m_ext(???)"); break;
	case m_f2i: snprintf(outBuf, sizeof(outBuf), "m_f2i(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_f2u: snprintf(outBuf, sizeof(outBuf), "m_f2u(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_i2f: snprintf(outBuf, sizeof(outBuf), "m_i2f(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_u2f: snprintf(outBuf, sizeof(outBuf), "m_u2f(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_f2f: snprintf(outBuf, sizeof(outBuf), "m_f2f(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_fneg: snprintf(outBuf, sizeof(outBuf), "m_fneg(%s,%s)", moptToString(o->l.t), moptToString(o->d.t)); break;
	case m_fadd: snprintf(outBuf, sizeof(outBuf), "m_fadd(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_fsub: snprintf(outBuf, sizeof(outBuf), "m_fsub(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_fmul: snprintf(outBuf, sizeof(outBuf), "m_fmul(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	case m_fdiv: snprintf(outBuf, sizeof(outBuf), "m_fdiv(%s,%s,%s)", moptToString(o->l.t), moptToString(o->r.t), moptToString(o->d.t)); break;
	default: snprintf(outBuf, sizeof(outBuf), "???");
	}

	return outBuf;
}

static void dumpMbaToFile(mbl_array_t *mba, const char *path) {
	FILE *file = qfopen(path, "a");
	file_printer_t printer(file);
	mba->print(printer);
	qfclose(file);
}
