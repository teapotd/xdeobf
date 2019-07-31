#pragma once

struct Unflattener : public optblock_t {
	int func(mblock_t *blk);

	mba_maturity_t lastMaturity = MMAT_ZERO;
};
