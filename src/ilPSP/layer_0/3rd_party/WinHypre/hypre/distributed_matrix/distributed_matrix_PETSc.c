/*BHEADER**********************************************************************
 * Copyright (c) 2008,  Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * This file is part of HYPRE.  See file COPYRIGHT for details.
 *
 * HYPRE is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (as published by the Free
 * Software Foundation) version 2.1 dated February 1999.
 *
 * $Revision: 2.4 $
 ***********************************************************************EHEADER*/




/******************************************************************************
 *
 * Member functions for hypre_DistributedMatrix class for PETSc storage scheme.
 *
 *****************************************************************************/

#include "./distributed_matrix.h"

/* Public headers and prototypes for PETSc matrix library */
#ifdef PETSC_AVAILABLE
#include "sles.h"
#endif

/*--------------------------------------------------------------------------
 * hypre_DistributedMatrixDestroyPETSc
 *   Internal routine for freeing a matrix stored in PETSc form.
 *--------------------------------------------------------------------------*/

int 
hypre_DistributedMatrixDestroyPETSc( hypre_DistributedMatrix *distributed_matrix )
{
#ifdef PETSC_AVAILABLE
   Mat PETSc_matrix = (Mat) hypre_DistributedMatrixLocalStorage(distributed_matrix);

   MatDestroy( PETSc_matrix );
#endif

   return(0);
}

/*--------------------------------------------------------------------------
 * Optional routines that depend on underlying storage type
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
 * hypre_DistributedMatrixPrintPETSc
 *   Internal routine for printing a matrix stored in PETSc form.
 *--------------------------------------------------------------------------*/

int 
hypre_DistributedMatrixPrintPETSc( hypre_DistributedMatrix *matrix )
{
   int  ierr=0;
#ifdef PETSC_AVAILABLE
   Mat PETSc_matrix = (Mat) hypre_DistributedMatrixLocalStorage(matrix);

   ierr = MatView( PETSc_matrix, VIEWER_STDOUT_WORLD );
#endif
   return(ierr);
}

/*--------------------------------------------------------------------------
 * hypre_DistributedMatrixGetLocalRangePETSc
 *--------------------------------------------------------------------------*/

int 
hypre_DistributedMatrixGetLocalRangePETSc( hypre_DistributedMatrix *matrix,
                             int *start,
                             int *end )
{
   int ierr=0;
#ifdef PETSC_AVAILABLE
   Mat PETSc_matrix = (Mat) hypre_DistributedMatrixLocalStorage(matrix);

   if (!PETSc_matrix) return(-1);


   ierr = MatGetOwnershipRange( PETSc_matrix, start, end ); CHKERRA(ierr);
/*

  Since PETSc's MatGetOwnershipRange actually returns 
  end = "one more than the global index of the last local row",
  we need to subtract one; hypre assumes we return the index
  of the last row itself.

*/
   *end = *end - 1;
#endif

   return(ierr);
}

/*--------------------------------------------------------------------------
 * hypre_DistributedMatrixGetRowPETSc
 *--------------------------------------------------------------------------*/

int 
hypre_DistributedMatrixGetRowPETSc( hypre_DistributedMatrix *matrix,
                             int row,
                             int *size,
                             int **col_ind,
                             double **values )
{
   int ierr=0;
#ifdef PETSC_AVAILABLE
   Mat PETSc_matrix = (Mat) hypre_DistributedMatrixLocalStorage(matrix);

   if (!PETSc_matrix) return(-1);

   ierr = MatGetRow( PETSc_matrix, row, size, col_ind, values); CHKERRA(ierr);
#endif

   return(ierr);
}

/*--------------------------------------------------------------------------
 * hypre_DistributedMatrixRestoreRowPETSc
 *--------------------------------------------------------------------------*/

int 
hypre_DistributedMatrixRestoreRowPETSc( hypre_DistributedMatrix *matrix,
                             int row,
                             int *size,
                             int **col_ind,
                             double **values )
{
   int ierr=0;
#ifdef PETSC_AVAILABLE
   Mat PETSc_matrix = (Mat) hypre_DistributedMatrixLocalStorage(matrix);

   if (PETSc_matrix == NULL) return(-1);

   ierr = MatRestoreRow( PETSc_matrix, row, size, col_ind, values); CHKERRA(ierr);
#endif

   return(ierr);
}
