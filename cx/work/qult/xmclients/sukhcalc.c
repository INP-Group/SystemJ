#include <math.h>

#include "sukhcalc.h"


static double f_Uref(sukhcalc_params_t *sr, double Uout)
{
  double aUout = fabs(Uout);

    if (aUout < 0.010) return 0;
    return
        aUout * sr->Ka + sr->U0 +
        sr->U1 * log(sr->Kb * aUout + 1);
}

static double f_Uph0(sukhcalc_chpars_t *cr, double ub)
{
    return (cr->B * cr->K1 + cr->a * cr->K2) * fabs(ub);
}

static double f_Urf(sukhcalc_chpars_t *cr, double Uam)
{
    return
        fabs(Uam) * cr->Ka_rf + cr->U0_rf + cr->U1_rf * log(cr->Kb * fabs(Uam) + 1);
}


void zzz()
{
}
