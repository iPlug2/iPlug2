// C port of  http://ldesoras.free.fr/prod.html#src_hiir
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>

const double PI = 3.1415926535897932384626433832795;

void
compute_transition_param(double *kp, double *qp, double transition)
{
	double k = *kp;
	double q = *qp;
	k = tan((1 - transition*2)*PI/4);
	k *= k;
	double kksqrt = pow(1 - k*k, 0.25);
	double e = 0.5*(1 - kksqrt)/(1 + kksqrt);
	double e4 = e*e*e*e;
	q = e*(1 + e4*(2 + e4*(15 + 150*e4)));
	*kp = k;
	*qp = q;
}

double
compute_acc_numden(double q, int order, int c, int den)
{
	den = den == 1;
	int i = den;
	int sign = den ? -1 : 1;
	double sum = 0;
	double q_ii;
	do {
		int i2 = i + 1 - den;
		q_ii = pow(q, i*i2);
		q_ii *= sin((i + i2)*c*PI/order + den*PI/2);
		q_ii *= sign;
		sum += q_ii;

		sign = -sign;
		++i;
	} while (fabs(q_ii) > 1e-100);
	return sum;
}

double
compute_coef(int c, double k, double q, int order)
{
	double num = compute_acc_numden(q, order, c, 0)*pow(q, 0.25);
	double den = compute_acc_numden(q, order, c, 1) + 0.5;
	double ww = num/den;
	ww *= ww;

	double x = sqrt((1 - ww*k)*(1 - ww/k))/(1 + ww);
	double coef = (1 - x)/(1 + x);

	return coef;
}

void
compute_coefs_spec_order_tbw(double coef_arr[], int n, double transition)
{
	double k;
	double q;
	compute_transition_param(&k, &q, transition);
	const int order = n*2 + 1;

	/*printf("k: %.18f\nq: %.18f\n", k, q);*/

	for (int i = 0; i < n; i++)
		coef_arr[i] = compute_coef(i + 1, k, q, order);
}

int
main(int argc, char **argv)
{
	if (argc != 3) {
		fputs("usage: halfband COUNT TRANSITION\n", stderr);
		return 1;
	}

	double tv[2];
	for (int i = 1; i <= 2; i++) {
		tv[i - 1]=strtold(argv[i], NULL);
		if (errno) {
			fprintf(stderr, "arg #%i failed to convert to double\n", i);
			return 1;
		}
	}
	int count = (int) tv[0];

	double *coefs = malloc(count*sizeof(double));
	compute_coefs_spec_order_tbw(coefs, count, tv[1]);
	for (int i = 0; i < count; i++)
		printf("%.18f\n", coefs[i]);
	free(coefs);

	return 0;
}
