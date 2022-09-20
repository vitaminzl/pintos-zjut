/*-------------------- Added by ZL -----------------------*/
/* 定点实数计算
 * 我们将后14为定义为小数
 * 因此当整数转化为小数时，直接左移14位即可，反之，同理
 * 然后定点实数的加减运算不需要重新定义，与整数保持一致
 * 乘法，则只要两数相乘最后再14位即可
 * 除法，对于定点实数除法，也需要被除数先左移14位再除
 * 对于实数转整数，则有直接截尾和四舍五入两种方式  */

#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#define Q 14
#define F (1 << Q)

#define INT_TO_FP(n) ((n) << Q)
#define FP_TO_INT(x) ((x) >> Q)
#define FP_TO_ROUND_INT(x) ((x) >= 0  ? FP_TO_INT((x) + (F >> 1)) : \
		                              FP_TO_INT((x) - (F >> 1)))
#define FP_MUL(x, y) ((int) ((((int64_t) (x)) * (y)) >> Q))
#define FP_DIV(x, y) ((int) ((((int64_t) (x)) << Q) / (y)))

#endif 

/*--------------------------------------------------------*/
