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

/*****************************************************************************
 *
 * This code implements a class for block compressed sparse row matrices.
 *
 *****************************************************************************/

#ifndef hypre_BCSR_MATRIX_HEADER
#define hypre_BCSR_MATRIX_HEADER

#define hypre_BCSR_MATRIX_USE_DENSE_BLOCKS
#include "bcsr_matrix_dense_block.h"

typedef struct {
  hypre_BCSRMatrixBlock** blocks;
  int* i;
  int* j;
  int num_block_rows;
  int num_block_cols;
  int num_nonzero_blocks;
  int num_rows_per_block;
  int num_cols_per_block;
} hypre_BCSRMatrix;

/*****************************************************************************
 *
 * Accessors
 *
 *****************************************************************************/

#define hypre_BCSRMatrixBlocks(A) ((A) -> blocks)
#define hypre_BCSRMatrixI(A) ((A) -> i)
#define hypre_BCSRMatrixJ(A) ((A) -> j)
#define hypre_BCSRMatrixNumBlockRows(A) ((A) -> num_block_rows)
#define hypre_BCSRMatrixNumBlockCols(A) ((A) -> num_block_cols)
#define hypre_BCSRMatrixNumNonzeroBlocks(A) ((A) -> num_nonzero_blocks)
#define hypre_BCSRMatrixNumRowsPerBlock(A) ((A) -> num_rows_per_block)
#define hypre_BCSRMatrixNumColsPerBlock(A) ((A) -> num_cols_per_block)


#if 0

/*****************************************************************************
 *
 * Prototypes
 *
 *****************************************************************************/

hypre_BCSRMatrix*
hypre_BCSRMatrixCreate(int num_block_rows, int num_block_cols,
		       int num_nonzero_blocks,
		       int num_rows_per_block, int num_cols_per_block);

int
hypre_BCSRMatrixDestroy(hypre_BCSRMatrix* A);

int
hypre_BCSRMatrixInitialise(hypre_BCSRMatrix* A);

int
hypre_BCSRMatrixPrint(hypre_BCSRMatrix* A, char* file_name);

int
hypre_BCSRMatrixTranspose(hypre_BCSRMatrix* A, hypre_BCSRMatrix** AT);

hypre_BCSRMatrix*
hypre_BCSRMatrixFromCSRMatrix(hypre_CSRMatrix* A,
			      int num_rows_per_block, int num_cols_per_block);

hypre_CSRMatrix*
hypre_BCSRMatrixToCSRMatrix(hypre_BCSRMatrix* B);

hypre_CSRMatrix*
hypre_BCSRMatrixCompress(hypre_BCSRMatrix* A);

/*****************************************************************************
 *
 * Auxiliary function prototypes
 *
 *****************************************************************************/

hypre_BCSRMatrix*
hypre_BCSRMatrixBuildInterp(hypre_BCSRMatrix* A, int* CF_marker,
			    hypre_CSRMatrix* S, int coarse_size);

hypre_BCSRMatrix*
hypre_BCSRMatrixBuildInterpD(hypre_BCSRMatrix* A, int* CF_marker,
			     hypre_CSRMatrix* S, int coarse_size);

int
hypre_BCSRMatrixBuildCoarseOperator(hypre_BCSRMatrix* RT,
				    hypre_BCSRMatrix* A,
				    hypre_BCSRMatrix* P,
				    hypre_BCSRMatrix** RAP_ptr);

#endif

#endif
