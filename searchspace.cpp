#include "searchspace.h"

SearchSpace::SearchSpace(Board *_b, int _color, int _depth, int _alpha, int _beta) {
	b = _b;
	color = _color;
	depth = _depth;
	isPVNode = (_beta - _alpha != 1);
	isInCheck = b->isInCheck(color);
}