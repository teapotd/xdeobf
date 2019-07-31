#include "stdafx.h"

int Unflattener::func(mblock_t *blk) {
	mbl_array_t *mba = blk->mba;

	if (mba->maturity == lastMaturity) {
		return 0;
	}

	dbg("[I] Current maturity: %s\n", mmatToString(mba->maturity));
	lastMaturity = mba->maturity;

	return 0;
}
