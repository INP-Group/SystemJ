#ifndef __SUKHCALC_H
#define __SUKHCALC_H


typedef struct
{
    double DAC_min;
    double DAC_max;

    double K1;
    double K2;
    double K3;
    double K4;
    double K5;
    double A;
    double B;

    double Ka_rf;
    double Kb_rf;
    double U0_rf;
    double U1_rf;

    double Kmu;

    double rS;
    double zL;
    double zP;
    double bC;
    double gU;
    double f0;

    double Rs;
    double Zl;
    double Zp;
    double Bc;
    double Gu;
    double Kadp;

    double Uf0;
} sukhcalc_chpars_t;

typedef struct
{
    double  Ka;

    double  Kmult_K1;
    double  Kmult_K0;

    sukhcalc_chpars_t  ch[2];
} sukhcalc_params_t;


#endif /* __SUKHCALC_H */
