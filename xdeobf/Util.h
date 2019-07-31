#pragma once

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

inline int bitCount(uint32 v) {
	// https://graphics.stanford.edu/~seander/bithacks.html
	v = v - ((v >> 1) & 0x55555555);
	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
	return ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
}
