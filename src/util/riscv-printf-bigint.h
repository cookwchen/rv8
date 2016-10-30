//
//  riscv-printf-bigint.h
//

#ifndef riscv_printf_bigint_h
#define riscv_printf_bigint_h

/****************************************************************

The author of this software is David M. Gay.

Copyright (C) 1998, 1999 by Lucent Technologies
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name of Lucent or any of its entities
not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.

****************************************************************/

namespace riscv {

	/* forward declarations */

	struct Bigint;

	Bigint* Balloc(int k);
	void Bcopy(Bigint *x, Bigint *y);
	void Bfree(Bigint *v);
	int lo0bits(u32 *y);
	Bigint* multadd(Bigint *b, int m, int a);
	int hi0bits(u32 x);
	Bigint* i2b(int i);
	Bigint* mult(Bigint *a, Bigint *b);
	Bigint* pow5mult(Bigint *b, int k);
	Bigint* lshift(Bigint *b, int k);
	int cmp(Bigint *a, Bigint *b);
	Bigint* diff(Bigint *a, Bigint *b);
	double b2d(Bigint *a, int *e);
	Bigint* d2b(double dd, int *e, int *bits);
	char* rv_alloc(int i);
	char* nrv_alloc(const char *s, char **rve, int n);
	void freedtoa(char *s);
	int quorem(Bigint *b, Bigint *S);

	/* constants shared with dtoa */

	/* Ten_pmax = floor(P*log(2)/log(5)) */
	/* Bletch = (highest power of 2 < DBL_MAX_10_EXP) / 16 */
	/* Quick_max = floor((P-1)*log(FLT_RADIX)/log(10) - 1) */
	/* Int_max = floor(P*log(FLT_RADIX)/log(10) - 1) */

	enum : int {
		Ebits =            11,
		Exp_shift =        20,
		Exp_msk1 =   0x100000,
		Exp_mask = 0x7ff00000,
		Exp_1 =    0x3ff00000,
		P =                53,
		Bias =           1023,
		Frac_mask =   0xfffff,
		Bndry_mask =  0xfffff,
		Ten_pmax =         22,
		Bletch =         0x10,
		LSB =               1,
		Log2P =             1,
		Quick_max =        14,
		Int_max =          14,
		kshift =            5,
		kmask =            31,
		n_bigtens =         5
	};

	enum : unsigned {
		Sign_bit = 0x80000000
	};

	static const double bigtens[] = {
		1e16, 1e32, 1e64, 1e128, 1e256
	};

	static const double tens[] = {
		1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
		1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
		1e20, 1e21, 1e22
	};

	/* bigint structure */

	struct Bigint {
		int k, maxwds, sign, wds;
		u32 x[1];
	};

	/* bigint functions */

	Bigint* Balloc(int k)
	{
		int x = 1 << k;
		Bigint *rv = (Bigint *)malloc(sizeof(Bigint) + (x-1)*sizeof(u32));
		rv->k = k;
		rv->maxwds = x;
		rv->sign = rv->wds = 0;
		return rv;
	}

	void Bcopy(Bigint *x, Bigint *y)
	{
		memcpy(&x->sign, &y->sign, y->wds*sizeof(u32) + 2 * sizeof(int));
	}

	void Bfree(Bigint *v)
	{
		free((void*)v);
	}

	int lo0bits(u32 *y)
	{
		int k;
		u32 x = *y;

		if (x & 7) {
			if (x & 1) {
				return 0;
			}
			if (x & 2) {
				*y = x >> 1;
				return 1;
			}
			*y = x >> 2;
			return 2;
		}
		k = 0;
		if (!(x & 0xffff)) {
			k = 16;
			x >>= 16;
		}
		if (!(x & 0xff)) {
			k += 8;
			x >>= 8;
		}
		if (!(x & 0xf)) {
			k += 4;
			x >>= 4;
		}
		if (!(x & 0x3)) {
			k += 2;
			x >>= 2;
		}
		if (!(x & 1)) {
			k++;
			x >>= 1;
			if (!x) {
				return 32;
			}
		}
		*y = x;
		return k;
	}

	Bigint* multadd(Bigint *b, int m, int a)
	{
		int i, wds;
		u32 *x;
		u64 carry, y;
		Bigint *b1;

		wds = b->wds;
		x = b->x;
		i = 0;
		carry = a;
		do {
			y = *x * (u64)m + carry;
			carry = y >> 32;
			*x++ = y & 0xffffffffUL;
		} while(++i < wds);
		if (carry) {
			if (wds >= b->maxwds) {
				b1 = Balloc(b->k+1);
				Bcopy(b1, b);
				Bfree(b);
				b = b1;
			}
			b->x[wds++] = carry;
			b->wds = wds;
		}
		return b;
	}

	int hi0bits(u32 x)
	{
		int k = 0;

		if (!(x & 0xffff0000)) {
			k = 16;
			x <<= 16;
		}
		if (!(x & 0xff000000)) {
			k += 8;
			x <<= 8;
		}
		if (!(x & 0xf0000000)) {
			k += 4;
			x <<= 4;
		}
		if (!(x & 0xc0000000)) {
			k += 2;
			x <<= 2;
		}
		if (!(x & 0x80000000)) {
			k++;
			if (!(x & 0x40000000)) {
				return 32;
			}
		}
		return k;
	}

	Bigint* i2b(int i)
	{
		Bigint *b;

		b = Balloc(1);
		b->x[0] = i;
		b->wds = 1;
		return b;
	}

	Bigint* mult(Bigint *a, Bigint *b)
	{
		Bigint *c;
		int k, wa, wb, wc;
		u32 *x, *xa, *xae, *xb, *xbe, *xc, *xc0;
		u32 y;
		u64 carry, z;

		if (a->wds < b->wds) {
			c = a;
			a = b;
			b = c;
		}
		k = a->k;
		wa = a->wds;
		wb = b->wds;
		wc = wa + wb;
		if (wc > a->maxwds) {
			k++;
		}
		c = Balloc(k);
		for(x = c->x, xa = x + wc; x < xa; x++) {
			*x = 0;
		}
		xa = a->x;
		xae = xa + wa;
		xb = b->x;
		xbe = xb + wb;
		xc0 = c->x;
		for(; xb < xbe; xc0++) {
			if ( (y = *xb++) !=0) {
				x = xa;
				xc = xc0;
				carry = 0;
				do {
					z = *x++ * (u64)y + *xc + carry;
					carry = z >> 32;
					*xc++ = z & 0xffffffffUL;
				} while(x < xae);
				*xc = carry;
			}
		}
		for(xc0 = c->x, xc = xc0 + wc; wc > 0 && !*--xc; --wc) ;
		c->wds = wc;
		return c;
	}

	Bigint* pow5mult(Bigint *b, int k)
	{
		Bigint *b1, *p5, *p51;
		int i;
		static int p05[3] = { 5, 25, 125 };

		if ( (i = k & 3) !=0) {
			b = multadd(b, p05[i-1], 0);
		}

		if (!(k >>= 2)) {
			return b;
		}

		p5 = i2b(625);
		for(;;) {
			if (k & 1) {
				b1 = mult(b, p5);
				Bfree(b);
				b = b1;
			}
			if (!(k >>= 1)) {
				break;
			}

			p51 = mult(p5,p5);
			Bfree(p5);
			p5 = p51;
		}
		Bfree(p5);

		return b;
	}

	Bigint* lshift(Bigint *b, int k)
	{
		int i, k1, n, n1;
		Bigint *b1;
		u32 *x, *x1, *xe, z;

		n = k >> kshift;
		k1 = b->k;
		n1 = n + b->wds + 1;
		for(i = b->maxwds; n1 > i; i <<= 1)
			k1++;
		b1 = Balloc(k1);
		x1 = b1->x;
		for(i = 0; i < n; i++)
			*x1++ = 0;
		x = b->x;
		xe = x + b->wds;
		if (k &= kmask) {
			k1 = 32 - k;
			z = 0;
			do {
				*x1++ = *x << k | z;
				z = *x++ >> k1;
			} while(x < xe);
			if ((*x1 = z) !=0) {
				++n1;
			}
		}
		else {
			do {
				*x1++ = *x++;
			} while(x < xe);
		}
		b1->wds = n1 - 1;
		Bfree(b);
		return b1;
	}

	int cmp(Bigint *a, Bigint *b)
	{
		u32 *xa, *xa0, *xb, *xb0;
		int i, j;

		i = a->wds;
		j = b->wds;
		assert (!(i > 1 && !a->x[i-1]));
		assert (!(j > 1 && !b->x[j-1]));
		if (i -= j) {
			return i;
		}
		xa0 = a->x;
		xa = xa0 + j;
		xb0 = b->x;
		xb = xb0 + j;
		for(;;) {
			if (*--xa != *--xb) {
				return *xa < *xb ? -1 : 1;
			}
			if (xa <= xa0) {
				break;
			}
		}
		return 0;
	}

	Bigint* diff(Bigint *a, Bigint *b)
	{
		Bigint *c;
		int i, wa, wb;
		u32 *xa, *xae, *xb, *xbe, *xc;
		u64 borrow, y;

		i = cmp(a,b);
		if (!i) {
			c = Balloc(0);
			c->wds = 1;
			c->x[0] = 0;
			return c;
		}
		if (i < 0) {
			c = a;
			a = b;
			b = c;
			i = 1;
		}
		else {
			i = 0;
		}
		c = Balloc(a->k);
		c->sign = i;
		wa = a->wds;
		xa = a->x;
		xae = xa + wa;
		wb = b->wds;
		xb = b->x;
		xbe = xb + wb;
		xc = c->x;
		borrow = 0;
		do {
			y = (u64)*xa++ - *xb++ - borrow;
			borrow = y >> 32 & 1UL;
			*xc++ = y & 0xffffffffUL;
		} while(xb < xbe);
		while(xa < xae) {
			y = *xa++ - borrow;
			borrow = y >> 32 & 1UL;
			*xc++ = y & 0xffffffffUL;
		}
		while(!*--xc) {
			wa--;
		}
		c->wds = wa;
		return c;
	}

	double b2d(Bigint *a, int *e)
	{
		u32 *xa, *xa0, w, y, z;
		int k;
		f64_bits v;
		u64 d0, d1;

		xa0 = a->x;
		xa = xa0 + a->wds;
		y = *--xa;
		k = hi0bits(y);
		*e = 32 - k;
		if (k < Ebits) {
			d0 = Exp_1 | y >> (Ebits - k);
			w = xa > xa0 ? *--xa : 0;
			d1 = y << ((32-Ebits) + k) | w >> (Ebits - k);
			goto ret_d;
		}
		z = xa > xa0 ? *--xa : 0;
		if (k -= Ebits) {
			d0 = Exp_1 | y << k | z >> (32 - k);
			y = xa > xa0 ? *--xa : 0;
			d1 = z << k | y >> (32 - k);
		}
		else {
			d0 = Exp_1 | y;
			d1 = z;
		}
ret_d:
		v.u = d0 << 32 | d1;
		return v.f;
	}

	Bigint* d2b(double dd, int *e, int *bits)
	{
		Bigint *b;
		int i;
		int de, k;
		u32 *x, y, z;
		f64_bits v{ .f = dd };
		u64 d0 = v.u >> 32, d1 = v.u & ((1ULL<<32)-1);

		b = Balloc(1);
		x = b->x;

		z = d0 & Frac_mask;
		d0 &= 0x7fffffff;	/* clear sign bit, which we ignore */
		if ( (de = (int)(d0 >> Exp_shift)) !=0)
			z |= Exp_msk1;
		if ( (y = d1) !=0) {
			if ( (k = lo0bits(&y)) !=0) {
				x[0] = y | z << (32 - k);
				z >>= k;
			}
			else {
				x[0] = y;
			}
			i = b->wds = (x[1] = z) !=0 ? 2 : 1;
		}
		else {
			k = lo0bits(&z);
			x[0] = z;
			i = b->wds = 1;
			k += 32;
		}
		if (de) {
			*e = de - Bias - (P-1) + k;
			*bits = P - k;
		}
		else {
			*e = de - Bias - (P-1) + 1 + k;
			*bits = 32*i - hi0bits(x[i-1]);
		}
		return b;
	}

	char* rv_alloc(int i)
	{
		int j, k, *r;

		j = sizeof(u32);
		for(k = 0; (int)(sizeof(Bigint) - sizeof(u32) - sizeof(int)) + j <= i; j <<= 1) {
			k++;
		}
		r = (int*)Balloc(k);
		*r = k;
		return (char *)(r+1);
	}

	char* nrv_alloc(const char *s, char **rve, int n)
	{
		char *rv, *t;

		t = rv = rv_alloc(n);
		while((*t = *s++) !=0) {
			t++;
		}
		if (rve) {
			*rve = t;
		}
		return rv;
	}

	void freedtoa(char *s)
	{
		Bigint *b = (Bigint *)((int *)s - 1);
		b->maxwds = 1 << (b->k = *(int*)b);
		Bfree(b);
	}

	int quorem(Bigint *b, Bigint *S)
	{
		int n;
		u32 *bx, *bxe, q, *sx, *sxe;
		u64 borrow, carry, y, ys;

		n = S->wds;
		assert(b->wds <= n);
		if (b->wds < n) {
			return 0;
		}
		sx = S->x;
		sxe = sx + --n;
		bx = b->x;
		bxe = bx + n;
		q = *bxe / (*sxe + 1);	/* ensure q <= true quotient */
		assert(q <= 9);
		if (q) {
			borrow = 0;
			carry = 0;
			do {
				ys = *sx++ * (u64)q + carry;
				carry = ys >> 32;
				y = *bx - (ys & 0xffffffffUL) - borrow;
				borrow = y >> 32 & 1UL;
				*bx++ = y & 0xffffffffUL;
			} while(sx <= sxe);
			if (!*bxe) {
				bx = b->x;
				while(--bxe > bx && !*bxe) {
					--n;
				}
				b->wds = n;
			}
		}
		if (cmp(b, S) >= 0) {
			q++;
			borrow = 0;
			carry = 0;
			bx = b->x;
			sx = S->x;
			do {
				ys = *sx++ + carry;
				carry = ys >> 32;
				y = *bx - (ys & 0xffffffffUL) - borrow;
				borrow = y >> 32 & 1UL;
				*bx++ = y & 0xffffffffUL;
			} while(sx <= sxe);
			bx = b->x;
			bxe = bx + n;
			if (!*bxe) {
				while(--bxe > bx && !*bxe) {
					--n;
				}
				b->wds = n;
			}
		}
		return q;
	}

}

#endif
