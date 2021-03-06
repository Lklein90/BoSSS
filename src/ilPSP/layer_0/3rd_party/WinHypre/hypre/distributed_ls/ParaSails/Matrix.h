/*BHEADER**********************************************************************
 * Copyright (c) 2008,  Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * This file is part of HYPRE.  See file COPYRIGHT for details.
 *
 * HYPRE is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (as published by the Free
 * Software Foundation) version 2.1 dated February 1999.
 *
 * $Revision: 2.5 $
 ***********************************************************************EHEADER*/




/******************************************************************************
 *
 * Matrix.h header file.
 *
 *****************************************************************************/

#include <stdio.h>
#include "Common.h"
#include "Mem.h"

#ifndef _MATRIX_H
#define _MATRIX_H

typedef struct
{
    MPI_Comm comm;

    int      beg_row;
    int      end_row;
    int     *beg_rows;
    int     *end_rows;

    Mem     *mem;

    int     *lens;
    int    **inds;
    double **vals;

    int     num_recv;
    int     num_send;

    int     sendlen;
    int     recvlen;

    int    *sendind;
    double *sendbuf;
    double *recvbuf;

    MPI_Request *recv_req;
    MPI_Request *send_req;
    MPI_Request *recv_req2;
    MPI_Request *send_req2;
    MPI_Status  *statuses;

    struct numbering *numb;
}
Matrix;

Matrix *MatrixCreate(MPI_Comm comm, int beg_row, int end_row);
Matrix *MatrixCreateLocal(int beg_row, int end_row);
void MatrixDestroy(Matrix *mat);
void MatrixSetRow(Matrix *mat, int row, int len, int *ind, double *val);
void MatrixGetRow(Matrix *mat, int row, int *lenp, int **indp, double **valp);
int  MatrixRowPe(Matrix *mat, int row);
void MatrixPrint(Matrix *mat, char *filename);
void MatrixRead(Matrix *mat, char *filename);
void RhsRead(double *rhs, Matrix *mat, char *filename);
int  MatrixNnz(Matrix *mat);

void MatrixComplete(Matrix *mat);
void MatrixMatvec(Matrix *mat, double *x, double *y);
void MatrixMatvecSerial(Matrix *mat, double *x, double *y);
void MatrixMatvecTrans(Matrix *mat, double *x, double *y);

#endif /* _MATRIX_H */
