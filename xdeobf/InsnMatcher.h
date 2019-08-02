#pragma once

constexpr mcode_t m_any = (mcode_t)m_max;

struct CmpStr {
	bool operator()(const char *l, const char *r) const {
		return strcmp(l, r) < 0;
	}
};

struct MatchingContext {
	std::map<const char*, minsn_t*> insns;
	std::map<const char*, mop_t*> ops;
};

struct OpPattern {
	const char *name;

	virtual bool matchSubImpl(MatchingContext &ctx, mop_t *op) = 0;

	bool matchSub(MatchingContext &ctx, mop_t *op) {
		if (matchSubImpl(ctx, op)) {
			if (name) {
				ctx.ops[name] = op;
			}
			return true;
		}
		return false;
	}

	bool match(MatchingContext& ctx, mop_t *op) {
		ctx.insns.clear();
		ctx.ops.clear();
		return matchSub(ctx, op);
	}
};

struct InsnPattern {
	const char *name;
	mcode_t code;
	std::unique_ptr<OpPattern> left, right;
	bool commutative;

	bool matchSubImpl(MatchingContext &ctx, minsn_t *insn) {
		if (code != m_any && code != insn->opcode) {
			return false;
		}

		if (commutative) {
			MatchingContext copy = ctx;
			if (left->matchSub(copy, &insn->r) && right->matchSub(copy, &insn->l)) {
				return true;
			}
		}
		return left->matchSub(ctx, &insn->l) && right->matchSub(ctx, &insn->r);
	}

	bool matchSub(MatchingContext &ctx, minsn_t *insn) {
		if (matchSubImpl(ctx, insn)) {
			if (name) {
				ctx.insns[name] = insn;
			}
			return true;
		}
		return false;
	}
};

struct OpAnyPattern : public OpPattern {
	virtual bool matchSubImpl(MatchingContext &ctx, mop_t *op) {
		return true;
	}
};

struct OpNumberPattern : public OpPattern {
	uint64 value;

	virtual bool matchSubImpl(MatchingContext &ctx, mop_t *op) {
		return op->t == mop_n && op->nnn->value == value;
	}
};

struct OpInsnPattern : public OpPattern {
	InsnPattern insn;

	virtual bool matchSubImpl(MatchingContext &ctx, mop_t *op) {
		return op->t == mop_d && insn.matchSub(ctx, op->d);
	}
};

static std::unique_ptr<OpPattern> opAny(const char *name = nullptr) {
	auto op = std::make_unique<OpAnyPattern>();
	op->name = name;
	return op;
}

static std::unique_ptr<OpPattern> opNumber(const char *name, uint64 value) {
	auto op = std::make_unique<OpNumberPattern>();
	op->value = value;
	return op;
}

static std::unique_ptr<OpPattern> opNumber(uint64 value) {
	return opNumber(nullptr, value);
}

static std::unique_ptr<OpPattern> opInsn(const char *name, InsnPattern&& insn) {
	auto op = std::make_unique<OpInsnPattern>();
	op->insn = std::move(insn);
	return op;
}

static std::unique_ptr<OpPattern> opInsn(InsnPattern&& insn) {
	return opInsn(nullptr, std::move(insn));
}

static InsnPattern insnPat(const char *name, mcode_t code = m_any, std::unique_ptr<OpPattern>&& left = opAny(), std::unique_ptr<OpPattern>&& right = opAny(), bool commutative = false) {
	return InsnPattern{ name, code, std::move(left), std::move(right), commutative };
}

static InsnPattern insnPat(mcode_t code = m_any, std::unique_ptr<OpPattern>&& left = opAny(), std::unique_ptr<OpPattern>&& right = opAny(), bool commutative = false) {
	return insnPat(nullptr, code, std::move(left), std::move(right), commutative);
}
