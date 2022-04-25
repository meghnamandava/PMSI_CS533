#line 228 "/data/gem5-approx-hpca/Benchmarks/splash2/codes/null_macros/c.m4.null.POSIX"

#line 1 "predcor.C"
/*************************************************************************/
/*                                                                       */
/*  Copyright (c) 1994 Stanford University                               */
/*                                                                       */
/*  All rights reserved.                                                 */
/*                                                                       */
/*  Permission is given to use, copy, and modify this software for any   */
/*  non-commercial purpose as long as this copyright notice is not       */
/*  removed.  All other uses, including redistribution in whole or in    */
/*  part, are forbidden without prior written permission.                */
/*                                                                       */
/*  This software is provided with absolutely no warranty and no         */
/*  support.                                                             */
/*                                                                       */
/*************************************************************************/


#line 17
#include <pthread.h>
#line 17
#include <sys/time.h>
#line 17
#include <unistd.h>
#line 17
#include <stdlib.h>
#line 17
extern pthread_t PThreadTable[];
#line 17

#include "mdvar.h"
#include "parameters.h"
#include "mddata.h"
#include "split.h"
#include "global.h"

/* predicts new values for displacement and its five derivatives
 *
 * NOR1 : NOR1 = NORDER + 1 = 7 (for a sixth-order method)
 */
void PREDIC(double *C, long NOR1, long ProcID)
{
    /*   this routine calculates predicted F(X), F'(X), F''(X), ... */

    long JIZ;
    long  JI;
    long  L;
    double S;
    long func, mol, dir, atom;

    JIZ=2;
    /* .....loop over F(X), F'(X), F''(X), ..... */
    for (func = 0; func < NORDER; func++) {
        for (mol = StartMol[ProcID]; mol < StartMol[ProcID+1]; mol++)
            for ( dir = 0; dir < NDIR; dir++)
                for ( atom = 0; atom < NATOM; atom++ ) {
                    JI = JIZ;
                    /* sum over Taylor Series */
                    S = 0.0;
                    for ( L = func; L < NORDER; L++) {
                        S += C[JI] * VAR[mol].F[L+1][dir][atom];
                        JI++;
                    } /* for */
                    VAR[mol].F[func][dir][atom] += S;
                } /* for atom */
        JIZ += NOR1;
    } /* for func */
} /* end of subroutine PREDIC */

/* corrects the predicted values, based on forces etc. computed in the interim
 *
 * PCC : the predictor-corrector constants
 * NOR1: NORDER + 1 = 7 for a sixth-order method)
 */

void CORREC(double *PCC, long NOR1, long ProcID)
{
    /*
      .....this routine calculates corrected F(X), F'(X), F"(X), ....
      from corrected F(X) = predicted F(X) + PCC(1)*(FR-SD)
      where SD is predicted accl. F"(X) and FR is computed
      accl. (force/mass) at predicted position
      */

    double Y;
    long mol, dir, atom, func;

    for (mol = StartMol[ProcID]; mol < StartMol[ProcID+1]; mol++) {
        for (dir = 0; dir < NDIR; dir++) {
            for (atom = 0; atom < NATOM; atom++) {
                Y = VAR[mol].F[FORCES][dir][atom] - VAR[mol].F[ACC][dir][atom];
                for ( func = 0; func < NOR1; func++)
                    VAR[mol].F[func][dir][atom] += PCC[func] * Y;
            } /* for atom */
        } /* for dir */
    } /* for mol */

} /* end of subroutine CORREC */
