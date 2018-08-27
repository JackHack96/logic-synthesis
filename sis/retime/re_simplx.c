
#ifdef SIS
#include "sis.h"

/*
 * This file contains all the routines to carry out the simplex
 * procedure described in "Numerical Recipes in C"...
 * pages 329-343
 * The relevant files -- simp1.c, simp2.c, simp3.c and simplx.c --
 * have been merged in this file.... The routines simp[123] have
 * been made static and simplx has been renamed to re_simplx to
 * avoid possible conflicts. Also re_simplx returns a status
 * (1 if error and the error_string() is appended to)
 *
 * Also added the utility routines ivector and free_ivector.
 * The routine nrerror has been replaced by a sis interface
 * The routine nrerror has been replaced by a sis interface
 */

static int *ivector(nl, nh) int nl, nh;
{
  int *v;
  v = ALLOC(int, nh - nl + 1);
  return v - nl;
}

/* ARGSUSED */
static void free_ivector(v, nl, nh) int *v, nl, nh;
{ free((char *)(v + nl)); }

static void simp1(a, mm, ll, nll, iabf, kp, bmax) double **a, *bmax;
int mm, ll[], nll, iabf, *kp;
{
  int k;
  double test;

  *kp = ll[1];
  *bmax = a[mm + 1][*kp + 1];
  for (k = 2; k <= nll; k++) {
    if (iabf == 0)
      test = a[mm + 1][ll[k] + 1] - (*bmax);
    else
      test = fabs(a[mm + 1][ll[k] + 1]) - fabs(*bmax);
    if (test > 0.0) {
      *bmax = a[mm + 1][ll[k] + 1];
      *kp = ll[k];
    }
  }
}

#define EPS 1.0e-6

static void simp2(a, n, l2, nl2, ip, kp, q1) int n, l2[], nl2, *ip, kp;
double **a, *q1;
{
  int k, ii, i;
  double qp, q0, q;

  *ip = 0;
  for (i = 1; i <= nl2; i++) {
    if (a[l2[i] + 1][kp + 1] < -EPS) {
      *q1 = -a[l2[i] + 1][1] / a[l2[i] + 1][kp + 1];
      *ip = l2[i];
      for (i = i + 1; i <= nl2; i++) {
        ii = l2[i];
        if (a[ii + 1][kp + 1] < -EPS) {
          q = -a[ii + 1][1] / a[ii + 1][kp + 1];
          if (q < *q1) {
            *ip = ii;
            *q1 = q;
          } else if (q == *q1) {
            for (k = 1; k <= n; k++) {
              qp = -a[*ip + 1][k + 1] / a[*ip + 1][kp + 1];
              q0 = -a[ii + 1][k + 1] / a[ii + 1][kp + 1];
              if (q0 != qp)
                break;
            }
            if (q0 < qp)
              *ip = ii;
          }
        }
      }
    }
  }
}

static void simp3(a, i1, k1, ip, kp) int i1, k1, ip, kp;
double **a;
{
  int kk, ii;
  double piv;

  piv = 1.0 / a[ip + 1][kp + 1];
  for (ii = 1; ii <= i1 + 1; ii++)
    if (ii - 1 != ip) {
      a[ii][kp + 1] *= piv;
      for (kk = 1; kk <= k1 + 1; kk++)
        if (kk - 1 != kp)
          a[ii][kk] -= a[ip + 1][kk] * a[ii][kp + 1];
    }
  for (kk = 1; kk <= k1 + 1; kk++)
    if (kk - 1 != kp)
      a[ip + 1][kk] *= -piv;
  a[ip + 1][kp + 1] = piv;
}

#define FREEALL                                                                \
  free_ivector(l3, 1, m);                                                      \
  free_ivector(l2, 1, m);                                                      \
  free_ivector(l1, 1, n + 1);

/*
 * The call to re_simplx solves the following
 *
 * MAX z = a[0][1].x1 + a[0][2].x2+ ... + a[0][N].xN
 * s.t
 * x1 >= 0, ...., xN >= 0
 * and also M = m1+m2+m3 constraints of the type
 *
 */
int re_simplx(a, m, n, m1, m2, m3, icase, izrov, iposv) int m, n, m1, m2, m3,
    *icase, izrov[], iposv[];
double **a;
{
  int i, ip, ir, is, k, kh, kp, m12, nl1, nl2;
  int *l1, *l2, *l3;
  double q1, bmax;

  if (m != (m1 + m2 + m3)) {
    error_append("Bad input constraint counts in SIMPLX\n");
    return 1;
  }
  l1 = ivector(1, n + 1);
  l2 = ivector(1, m);
  l3 = ivector(1, m);
  nl1 = n;
  for (k = 1; k <= n; k++)
    l1[k] = izrov[k] = k;
  nl2 = m;
  for (i = 1; i <= m; i++) {
    if (a[i + 1][1] < 0.0) {
      error_append("Bad input tableau in SIMPLX\n");
      return 1;
    }
    l2[i] = i;
    iposv[i] = n + i;
  }
  for (i = 1; i <= m2; i++)
    l3[i] = 1;
  ir = 0;
  if (m2 + m3) {
    ir = 1;
    for (k = 1; k <= (n + 1); k++) {
      q1 = 0.0;
      for (i = m1 + 1; i <= m; i++)
        q1 += a[i + 1][k];
      a[m + 2][k] = -q1;
    }
    do {
      simp1(a, m + 1, l1, nl1, 0, &kp, &bmax);
      if (bmax <= EPS && a[m + 2][1] < -EPS) {
        *icase = -1;
        FREEALL return 0;
      } else if (bmax <= EPS && a[m + 2][1] <= EPS) {
        m12 = m1 + m2 + 1;
        if (m12 <= m) {
          for (ip = m12; ip <= m; ip++) {
            if (iposv[ip] == (ip + n)) {
              simp1(a, ip, l1, nl1, 1, &kp, &bmax);
              if (bmax > 0.0)
                goto one;
            }
          }
        }
        ir = 0;
        --m12;
        if (m1 + 1 <= m12)
          for (i = m1 + 1; i <= m12; i++)
            if (l3[i - m1] == 1)
              for (k = 1; k <= n + 1; k++)
                a[i + 1][k] = -a[i + 1][k];
        break;
      }
      simp2(a, n, l2, nl2, &ip, kp, &q1);
      if (ip == 0) {
        *icase = -1;
        FREEALL return 0;
      }
    one:
      simp3(a, m + 1, n, ip, kp);
      if (iposv[ip] >= (n + m1 + m2 + 1)) {
        for (k = 1; k <= nl1; k++)
          if (l1[k] == kp)
            break;
        --nl1;
        for (is = k; is <= nl1; is++)
          l1[is] = l1[is + 1];
        a[m + 2][kp + 1] += 1.0;
        for (i = 1; i <= m + 2; i++)
          a[i][kp + 1] = -a[i][kp + 1];
      } else {
        if (iposv[ip] >= (n + m1 + 1)) {
          kh = iposv[ip] - m1 - n;
          if (l3[kh]) {
            l3[kh] = 0;
            a[m + 2][kp + 1] += 1.0;
            for (i = 1; i <= m + 2; i++)
              a[i][kp + 1] = -a[i][kp + 1];
          }
        }
      }
      is = izrov[kp];
      izrov[kp] = iposv[ip];
      iposv[ip] = is;
    } while (ir);
  }
  for (;;) {
    simp1(a, 0, l1, nl1, 0, &kp, &bmax);
    if (bmax <= 0.0) {
      *icase = 0;
      FREEALL return 0;
    }
    simp2(a, n, l2, nl2, &ip, kp, &q1);
    if (ip == 0) {
      *icase = 1;
      FREEALL return 0;
    }
    simp3(a, m, n, ip, kp);
    is = izrov[kp];
    izrov[kp] = iposv[ip];
    iposv[ip] = is;
  }
}

#undef EPS
#undef FREEALL
#endif /* SIS */
