/*BHEADER**********************************************************************
 * Copyright (c) 2008,  Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * This file is part of HYPRE.  See file COPYRIGHT for details.
 *
 * HYPRE is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (as published by the Free
 * Software Foundation) version 2.1 dated February 1999.
 *
 * $Revision: 2.30 $
 ***********************************************************************EHEADER*/





#include "headers.h"




#include "_hypre_utilities.h"
#include "../parcsr_mv/_hypre_parcsr_mv.h"
                                                                                                               
/* reference seems necessary to prevent a problem with the
   "headers" script... */

void hypre_ParCSRMatrixExtractBExt_Arrays
(
  int ** pB_ext_i, int ** pB_ext_j, double ** pB_ext_data, int ** pB_ext_row_map,
  int * num_nonzeros,
  int data, int find_row_map, MPI_Comm comm, hypre_ParCSRCommPkg * comm_pkg,
  int num_cols_B, int num_recvs, int num_sends,
  int first_col_diag, int first_row_index,
  int * recv_vec_starts, int * send_map_starts, int * send_map_elmts,
  int * diag_i, int * diag_j, int * offd_i, int * offd_j, int * col_map_offd,
  double * diag_data, double * offd_data
);

/* The following function was formerly part of hypre_ParMatmul
   but was removed so it can also be used for multiplication of
   Boolean matrices
*/

void hypre_ParMatmul_RowSizes
( int ** C_diag_i, int ** C_offd_i, int ** B_marker,
  int * A_diag_i, int * A_diag_j, int * A_offd_i, int * A_offd_j,
  int * B_diag_i, int * B_diag_j, int * B_offd_i, int * B_offd_j,
  int * B_ext_diag_i, int * B_ext_diag_j, 
  int * B_ext_offd_i, int * B_ext_offd_j, int * map_B_to_C,
  int *C_diag_size, int *C_offd_size,
  int num_rows_diag_A, int num_cols_offd_A, int allsquare,
  int num_cols_diag_B, int num_cols_offd_B, int num_cols_offd_C
)
{
   int i1, i2, i3, jj2, jj3;
   int jj_count_diag, jj_count_offd, jj_row_begin_diag, jj_row_begin_offd;
   int start_indexing = 0; /* start indexing for C_data at 0 */
   /* First pass begins here.  Computes sizes of C rows.
      Arrays computed: C_diag_i, C_offd_i, B_marker
      Arrays needed: (11, all int*)
        A_diag_i, A_diag_j, A_offd_i, A_offd_j,
        B_diag_i, B_diag_j, B_offd_i, B_offd_j,
        B_ext_i, B_ext_j, col_map_offd_B,
        col_map_offd_B, B_offd_i, B_offd_j, B_ext_i, B_ext_j,
      Scalars computed: C_diag_size, C_offd_size
      Scalars needed:
      num_rows_diag_A, num_rows_diag_A, num_cols_offd_A, allsquare,
      first_col_diag_B, n_cols_B, num_cols_offd_B, num_cols_diag_B
   */

   *C_diag_i = hypre_CTAlloc(int, num_rows_diag_A+1);
   *C_offd_i = hypre_CTAlloc(int, num_rows_diag_A+1);

   jj_count_diag = start_indexing;
   jj_count_offd = start_indexing;
   for (i1 = 0; i1 < num_cols_diag_B+num_cols_offd_C; i1++)
   {      
      (*B_marker)[i1] = -1;
   }

   /*-----------------------------------------------------------------------
    *  Loop over rows of A
    *-----------------------------------------------------------------------*/
   
   for (i1 = 0; i1 < num_rows_diag_A; i1++)
   {
      
      /*--------------------------------------------------------------------
       *  Set marker for diagonal entry, C_{i1,i1} (for square matrices). 
       *--------------------------------------------------------------------*/
 
      jj_row_begin_diag = jj_count_diag;
      jj_row_begin_offd = jj_count_offd;
      if ( allsquare ) {
         (*B_marker)[i1] = jj_count_diag;
         jj_count_diag++;
      }

         /*-----------------------------------------------------------------
          *  Loop over entries in row i1 of A_offd.
          *-----------------------------------------------------------------*/
         
	 if (num_cols_offd_A)
	 {
           for (jj2 = A_offd_i[i1]; jj2 < A_offd_i[i1+1]; jj2++)
           {
            i2 = A_offd_j[jj2];
 
               /*-----------------------------------------------------------
                *  Loop over entries in row i2 of B_ext.
                *-----------------------------------------------------------*/
 
               for (jj3 = B_ext_offd_i[i2]; jj3 < B_ext_offd_i[i2+1]; jj3++)
               {
                  i3 = num_cols_diag_B+B_ext_offd_j[jj3];
                  
                  /*--------------------------------------------------------
                   *  Check B_marker to see that C_{i1,i3} has not already
                   *  been accounted for. If it has not, mark it and increment
                   *  counter.
                   *--------------------------------------------------------*/

                  if ((*B_marker)[i3] < jj_row_begin_offd)
                  {
                     	(*B_marker)[i3] = jj_count_offd;
                     	jj_count_offd++;
		  } 
               }
               for (jj3 = B_ext_diag_i[i2]; jj3 < B_ext_diag_i[i2+1]; jj3++)
               {
                  i3 = B_ext_diag_j[jj3];
                  
                  if ((*B_marker)[i3] < jj_row_begin_diag)
                  {
                  	(*B_marker)[i3] = jj_count_diag;
                     	jj_count_diag++;
		  } 
               }
            }
         }
         /*-----------------------------------------------------------------
          *  Loop over entries in row i1 of A_diag.
          *-----------------------------------------------------------------*/
         
         for (jj2 = A_diag_i[i1]; jj2 < A_diag_i[i1+1]; jj2++)
         {
            i2 = A_diag_j[jj2];
 
               /*-----------------------------------------------------------
                *  Loop over entries in row i2 of B_diag.
                *-----------------------------------------------------------*/
 
               for (jj3 = B_diag_i[i2]; jj3 < B_diag_i[i2+1]; jj3++)
               {
                  i3 = B_diag_j[jj3];
                  
                  /*--------------------------------------------------------
                   *  Check B_marker to see that C_{i1,i3} has not already
                   *  been accounted for. If it has not, mark it and increment
                   *  counter.
                   *--------------------------------------------------------*/
 
                  if ((*B_marker)[i3] < jj_row_begin_diag)
                  {
                     (*B_marker)[i3] = jj_count_diag;
                     jj_count_diag++;
                  }
               }
               /*-----------------------------------------------------------
                *  Loop over entries in row i2 of B_offd.
                *-----------------------------------------------------------*/

	       if (num_cols_offd_B)
	       { 
                 for (jj3 = B_offd_i[i2]; jj3 < B_offd_i[i2+1]; jj3++)
                 {
                  i3 = num_cols_diag_B+map_B_to_C[B_offd_j[jj3]];
                  
                  /*--------------------------------------------------------
                   *  Check B_marker to see that C_{i1,i3} has not already
                   *  been accounted for. If it has not, mark it and increment
                   *  counter.
                   *--------------------------------------------------------*/
 
                  if ((*B_marker)[i3] < jj_row_begin_offd)
                  {
                     (*B_marker)[i3] = jj_count_offd;
                     jj_count_offd++;
                  }
                 }
            }
      }
            
      /*--------------------------------------------------------------------
       * Set C_diag_i and C_offd_i for this row.
       *--------------------------------------------------------------------*/
 
      (*C_diag_i)[i1] = jj_row_begin_diag;
      (*C_offd_i)[i1] = jj_row_begin_offd;
      
   }
  
   (*C_diag_i)[num_rows_diag_A] = jj_count_diag;
   (*C_offd_i)[num_rows_diag_A] = jj_count_offd;
 
   /*-----------------------------------------------------------------------
    *  Allocate C_diag_data and C_diag_j arrays.
    *  Allocate C_offd_data and C_offd_j arrays.
    *-----------------------------------------------------------------------*/
 
   *C_diag_size = jj_count_diag;
   *C_offd_size = jj_count_offd;

   /* End of First Pass */
}

/*--------------------------------------------------------------------------
 * hypre_ParMatmul : multiplies two ParCSRMatrices A and B and returns
 * the product in ParCSRMatrix C
 * Note that C does not own the partitionings since its row_starts
 * is owned by A and col_starts by B.
 *--------------------------------------------------------------------------*/

hypre_ParCSRMatrix *hypre_ParMatmul( hypre_ParCSRMatrix  *A,
				     hypre_ParCSRMatrix  *B)
{
   MPI_Comm 	   comm = hypre_ParCSRMatrixComm(A);

   hypre_CSRMatrix *A_diag = hypre_ParCSRMatrixDiag(A);
   
   double          *A_diag_data = hypre_CSRMatrixData(A_diag);
   int             *A_diag_i = hypre_CSRMatrixI(A_diag);
   int             *A_diag_j = hypre_CSRMatrixJ(A_diag);

   hypre_CSRMatrix *A_offd = hypre_ParCSRMatrixOffd(A);
   
   double          *A_offd_data = hypre_CSRMatrixData(A_offd);
   int             *A_offd_i = hypre_CSRMatrixI(A_offd);
   int             *A_offd_j = hypre_CSRMatrixJ(A_offd);

   int *row_starts_A = hypre_ParCSRMatrixRowStarts(A);
   int	num_rows_diag_A = hypre_CSRMatrixNumRows(A_diag);
   int	num_cols_diag_A = hypre_CSRMatrixNumCols(A_diag);
   int	num_cols_offd_A = hypre_CSRMatrixNumCols(A_offd);
   
   hypre_CSRMatrix *B_diag = hypre_ParCSRMatrixDiag(B);
   
   double          *B_diag_data = hypre_CSRMatrixData(B_diag);
   int             *B_diag_i = hypre_CSRMatrixI(B_diag);
   int             *B_diag_j = hypre_CSRMatrixJ(B_diag);

   hypre_CSRMatrix *B_offd = hypre_ParCSRMatrixOffd(B);
   int		   *col_map_offd_B = hypre_ParCSRMatrixColMapOffd(B);
   
   double          *B_offd_data = hypre_CSRMatrixData(B_offd);
   int             *B_offd_i = hypre_CSRMatrixI(B_offd);
   int             *B_offd_j = hypre_CSRMatrixJ(B_offd);

   int	first_col_diag_B = hypre_ParCSRMatrixFirstColDiag(B);
   int	last_col_diag_B;
   int *col_starts_B = hypre_ParCSRMatrixColStarts(B);
   int	num_rows_diag_B = hypre_CSRMatrixNumRows(B_diag);
   int	num_cols_diag_B = hypre_CSRMatrixNumCols(B_diag);
   int	num_cols_offd_B = hypre_CSRMatrixNumCols(B_offd);

   hypre_ParCSRMatrix *C;
   int		      *col_map_offd_C;
   int		      *map_B_to_C;

   hypre_CSRMatrix *C_diag;

   double          *C_diag_data;
   int             *C_diag_i;
   int             *C_diag_j;

   hypre_CSRMatrix *C_offd;

   double          *C_offd_data=NULL;
   int             *C_offd_i=NULL;
   int             *C_offd_j=NULL;

   int              C_diag_size;
   int              C_offd_size;
   int		    num_cols_offd_C = 0;
   
   hypre_CSRMatrix *Bs_ext;
   
   double          *Bs_ext_data;
   int             *Bs_ext_i;
   int             *Bs_ext_j;

   double          *B_ext_diag_data;
   int             *B_ext_diag_i;
   int             *B_ext_diag_j;
   int              B_ext_diag_size;

   double          *B_ext_offd_data;
   int             *B_ext_offd_i;
   int             *B_ext_offd_j;
   int              B_ext_offd_size;

   int		   *B_marker;
   int		   *temp;

   int              i, j;
   int              i1, i2, i3;
   int              jj2, jj3;
   
   int              jj_count_diag, jj_count_offd;
   int              jj_row_begin_diag, jj_row_begin_offd;
   int              start_indexing = 0; /* start indexing for C_data at 0 */
   int		    n_rows_A, n_cols_A;
   int		    n_rows_B, n_cols_B;
   int              allsquare = 0;
   int              cnt, cnt_offd, cnt_diag;
   int 		    num_procs;
   int 		    value;

   double           a_entry;
   double           a_b_product;
   
   double           zero = 0.0;

   n_rows_A = hypre_ParCSRMatrixGlobalNumRows(A);
   n_cols_A = hypre_ParCSRMatrixGlobalNumCols(A);
   n_rows_B = hypre_ParCSRMatrixGlobalNumRows(B);
   n_cols_B = hypre_ParCSRMatrixGlobalNumCols(B);

   if (n_cols_A != n_rows_B || num_cols_diag_A != num_rows_diag_B)
   {
        hypre_error_in_arg(1);
	printf(" Error! Incompatible matrix dimensions!\n");
	return NULL;
   }
   if ( num_rows_diag_A==num_cols_diag_B) allsquare = 1;

   /*-----------------------------------------------------------------------
    *  Extract B_ext, i.e. portion of B that is stored on neighbor procs
    *  and needed locally for matrix matrix product 
    *-----------------------------------------------------------------------*/

   MPI_Comm_size(comm, &num_procs);

   if (num_procs > 1)
   {
       /*---------------------------------------------------------------------
    	* If there exists no CommPkg for A, a CommPkg is generated using
    	* equally load balanced partitionings within 
	* hypre_ParCSRMatrixExtractBExt
    	*--------------------------------------------------------------------*/
   	Bs_ext = hypre_ParCSRMatrixExtractBExt(B,A,1);
   	Bs_ext_data = hypre_CSRMatrixData(Bs_ext);
   	Bs_ext_i    = hypre_CSRMatrixI(Bs_ext);
   	Bs_ext_j    = hypre_CSRMatrixJ(Bs_ext);
   }
   B_ext_diag_i = hypre_CTAlloc(int, num_cols_offd_A+1);
   B_ext_offd_i = hypre_CTAlloc(int, num_cols_offd_A+1);
   B_ext_diag_size = 0;
   B_ext_offd_size = 0;
   last_col_diag_B = first_col_diag_B + num_cols_diag_B -1;

   for (i=0; i < num_cols_offd_A; i++)
   {
      for (j=Bs_ext_i[i]; j < Bs_ext_i[i+1]; j++)
         if (Bs_ext_j[j] < first_col_diag_B || Bs_ext_j[j] > last_col_diag_B)
            B_ext_offd_size++;
         else
            B_ext_diag_size++;
      B_ext_diag_i[i+1] = B_ext_diag_size;
      B_ext_offd_i[i+1] = B_ext_offd_size;
   }

   if (B_ext_diag_size)
   {
      B_ext_diag_j = hypre_CTAlloc(int, B_ext_diag_size);
      B_ext_diag_data = hypre_CTAlloc(double, B_ext_diag_size);
   }
   if (B_ext_offd_size)
   {
      B_ext_offd_j = hypre_CTAlloc(int, B_ext_offd_size);
      B_ext_offd_data = hypre_CTAlloc(double, B_ext_offd_size);
   }

   cnt_offd = 0;
   cnt_diag = 0;
   for (i=0; i < num_cols_offd_A; i++)
   {
      for (j=Bs_ext_i[i]; j < Bs_ext_i[i+1]; j++)
         if (Bs_ext_j[j] < first_col_diag_B || Bs_ext_j[j] > last_col_diag_B)
         {
            B_ext_offd_j[cnt_offd] = Bs_ext_j[j];
            B_ext_offd_data[cnt_offd++] = Bs_ext_data[j];
         }
         else
         {
            B_ext_diag_j[cnt_diag] = Bs_ext_j[j] - first_col_diag_B;
            B_ext_diag_data[cnt_diag++] = Bs_ext_data[j];
         }
   }

   if (num_procs > 1)
   {
      hypre_CSRMatrixDestroy(Bs_ext);
      Bs_ext = NULL;
   }

   cnt = 0;
   if (B_ext_offd_size || num_cols_offd_B)
   {
      temp = hypre_CTAlloc(int, B_ext_offd_size+num_cols_offd_B);
      for (i=0; i < B_ext_offd_size; i++)
         temp[i] = B_ext_offd_j[i];
      cnt = B_ext_offd_size;
      for (i=0; i < num_cols_offd_B; i++)
         temp[cnt++] = col_map_offd_B[i];
   }
   if (cnt)
   {
      qsort0(temp, 0, cnt-1);

      num_cols_offd_C = 1;
      value = temp[0];
      for (i=1; i < cnt; i++)
      {
         if (temp[i] > value)
         {
            value = temp[i];
            temp[num_cols_offd_C++] = value;
         }
      }
   }

   if (num_cols_offd_C)
        col_map_offd_C = hypre_CTAlloc(int,num_cols_offd_C);

   for (i=0; i < num_cols_offd_C; i++)
      col_map_offd_C[i] = temp[i];

   if (B_ext_offd_size || num_cols_offd_B)
      hypre_TFree(temp);

   for (i=0 ; i < B_ext_offd_size; i++)
      B_ext_offd_j[i] = hypre_BinarySearch(col_map_offd_C,
                                           B_ext_offd_j[i],
                                           num_cols_offd_C);
   if (num_cols_offd_B)
   {
      map_B_to_C = hypre_CTAlloc(int,num_cols_offd_B);

      cnt = 0;
      for (i=0; i < num_cols_offd_C; i++)
         if (col_map_offd_C[i] == col_map_offd_B[cnt])
         {
            map_B_to_C[cnt++] = i;
            if (cnt == num_cols_offd_B) break;
         }
   }

   /*-----------------------------------------------------------------------
   *  Allocate marker array.
    *-----------------------------------------------------------------------*/

   B_marker = hypre_CTAlloc(int, num_cols_diag_B+num_cols_offd_C);

   /*-----------------------------------------------------------------------
    *  Initialize some stuff.
    *-----------------------------------------------------------------------*/

   for (i1 = 0; i1 < num_cols_diag_B+num_cols_offd_C; i1++)
   {      
      B_marker[i1] = -1;
   }


   hypre_ParMatmul_RowSizes(
      &C_diag_i, &C_offd_i, &B_marker,
      A_diag_i, A_diag_j, A_offd_i, A_offd_j,
      B_diag_i, B_diag_j, B_offd_i, B_offd_j,
      B_ext_diag_i, B_ext_diag_j, B_ext_offd_i, B_ext_offd_j,
      map_B_to_C,
      &C_diag_size, &C_offd_size,
      num_rows_diag_A, num_cols_offd_A, allsquare,
      num_cols_diag_B, num_cols_offd_B,
      num_cols_offd_C
      );


   /*-----------------------------------------------------------------------
    *  Allocate C_diag_data and C_diag_j arrays.
    *  Allocate C_offd_data and C_offd_j arrays.
    *-----------------------------------------------------------------------*/
 
   last_col_diag_B = first_col_diag_B + num_cols_diag_B - 1;
   C_diag_data = hypre_CTAlloc(double, C_diag_size);
   C_diag_j    = hypre_CTAlloc(int, C_diag_size);
   if (C_offd_size)
   { 
   	C_offd_data = hypre_CTAlloc(double, C_offd_size);
   	C_offd_j    = hypre_CTAlloc(int, C_offd_size);
   } 


   /*-----------------------------------------------------------------------
    *  Second Pass: Fill in C_diag_data and C_diag_j.
    *  Second Pass: Fill in C_offd_data and C_offd_j.
    *-----------------------------------------------------------------------*/

   /*-----------------------------------------------------------------------
    *  Initialize some stuff.
    *-----------------------------------------------------------------------*/

   jj_count_diag = start_indexing;
   jj_count_offd = start_indexing;
   for (i1 = 0; i1 < num_cols_diag_B+num_cols_offd_C; i1++)
   {      
      B_marker[i1] = -1;
   }
   
   /*-----------------------------------------------------------------------
    *  Loop over interior c-points.
    *-----------------------------------------------------------------------*/
    
   for (i1 = 0; i1 < num_rows_diag_A; i1++)
   {
      
      /*--------------------------------------------------------------------
       *  Create diagonal entry, C_{i1,i1} 
       *--------------------------------------------------------------------*/

      jj_row_begin_diag = jj_count_diag;
      jj_row_begin_offd = jj_count_offd;
      if ( allsquare ) {
         B_marker[i1] = jj_count_diag;
         C_diag_data[jj_count_diag] = zero;
         C_diag_j[jj_count_diag] = i1;
         jj_count_diag++;
      }

         /*-----------------------------------------------------------------
          *  Loop over entries in row i1 of A_offd.
          *-----------------------------------------------------------------*/
         
	 if (num_cols_offd_A)
	 {
	  for (jj2 = A_offd_i[i1]; jj2 < A_offd_i[i1+1]; jj2++)
          {
            i2 = A_offd_j[jj2];
            a_entry = A_offd_data[jj2];
            
               /*-----------------------------------------------------------
                *  Loop over entries in row i2 of B_ext.
                *-----------------------------------------------------------*/

               for (jj3 = B_ext_offd_i[i2]; jj3 < B_ext_offd_i[i2+1]; jj3++)
               {
                  i3 = num_cols_diag_B+B_ext_offd_j[jj3];
                  a_b_product = a_entry * B_ext_offd_data[jj3];
                  
                  /*--------------------------------------------------------
                   *  Check B_marker to see that C_{i1,i3} has not already
                   *  been accounted for. If it has not, create a new entry.
                   *  If it has, add new contribution.
                   *--------------------------------------------------------*/
                  if (B_marker[i3] < jj_row_begin_offd)
                  {
                     	B_marker[i3] = jj_count_offd;
                     	C_offd_data[jj_count_offd] = a_b_product;
                     	C_offd_j[jj_count_offd] = i3-num_cols_diag_B;
                     	jj_count_offd++;
		  }
		  else
                    	C_offd_data[B_marker[i3]] += a_b_product;
               }
               for (jj3 = B_ext_diag_i[i2]; jj3 < B_ext_diag_i[i2+1]; jj3++)
               {
                  i3 = B_ext_diag_j[jj3];
                  a_b_product = a_entry * B_ext_diag_data[jj3];
                  if (B_marker[i3] < jj_row_begin_diag)
                  {
                     	B_marker[i3] = jj_count_diag;
                     	C_diag_data[jj_count_diag] = a_b_product;
                     	C_diag_j[jj_count_diag] = i3;
                     	jj_count_diag++;
		  }
		  else
                     	C_diag_data[B_marker[i3]] += a_b_product;
               }
            }
         }

         /*-----------------------------------------------------------------
          *  Loop over entries in row i1 of A_diag.
          *-----------------------------------------------------------------*/

         for (jj2 = A_diag_i[i1]; jj2 < A_diag_i[i1+1]; jj2++)
         {
            i2 = A_diag_j[jj2];
            a_entry = A_diag_data[jj2];
            
               /*-----------------------------------------------------------
                *  Loop over entries in row i2 of B_diag.
                *-----------------------------------------------------------*/

               for (jj3 = B_diag_i[i2]; jj3 < B_diag_i[i2+1]; jj3++)
               {
                  i3 = B_diag_j[jj3];
                  a_b_product = a_entry * B_diag_data[jj3];
                  
                  /*--------------------------------------------------------
                   *  Check B_marker to see that C_{i1,i3} has not already
                   *  been accounted for. If it has not, create a new entry.
                   *  If it has, add new contribution.
                   *--------------------------------------------------------*/

                  if (B_marker[i3] < jj_row_begin_diag)
                  {
                     B_marker[i3] = jj_count_diag;
                     C_diag_data[jj_count_diag] = a_b_product;
                     C_diag_j[jj_count_diag] = i3;
                     jj_count_diag++;
                  }
                  else
                  {
                     C_diag_data[B_marker[i3]] += a_b_product;
                  }
               }
               if (num_cols_offd_B)
	       {
		for (jj3 = B_offd_i[i2]; jj3 < B_offd_i[i2+1]; jj3++)
                {
                  i3 = num_cols_diag_B+map_B_to_C[B_offd_j[jj3]];
                  a_b_product = a_entry * B_offd_data[jj3];
                  
                  /*--------------------------------------------------------
                   *  Check B_marker to see that C_{i1,i3} has not already
                   *  been accounted for. If it has not, create a new entry.
                   *  If it has, add new contribution.
                   *--------------------------------------------------------*/

                  if (B_marker[i3] < jj_row_begin_offd)
                  {
                     B_marker[i3] = jj_count_offd;
                     C_offd_data[jj_count_offd] = a_b_product;
                     C_offd_j[jj_count_offd] = i3-num_cols_diag_B;
                     jj_count_offd++;
                  }
                  else
                  {
                     C_offd_data[B_marker[i3]] += a_b_product;
                  }
                }
               }
         }
   }

   C = hypre_ParCSRMatrixCreate(comm, n_rows_A, n_cols_B, row_starts_A,
	col_starts_B, num_cols_offd_C, C_diag_size, C_offd_size);

/* Note that C does not own the partitionings */
   hypre_ParCSRMatrixSetRowStartsOwner(C,0);
   hypre_ParCSRMatrixSetColStartsOwner(C,0);

   C_diag = hypre_ParCSRMatrixDiag(C);
   hypre_CSRMatrixData(C_diag) = C_diag_data; 
   hypre_CSRMatrixI(C_diag) = C_diag_i; 
   hypre_CSRMatrixJ(C_diag) = C_diag_j; 

   C_offd = hypre_ParCSRMatrixOffd(C);
   hypre_CSRMatrixI(C_offd) = C_offd_i; 
   hypre_ParCSRMatrixOffd(C) = C_offd;

   if (num_cols_offd_C)
   {
      hypre_CSRMatrixData(C_offd) = C_offd_data; 
      hypre_CSRMatrixJ(C_offd) = C_offd_j; 
      hypre_ParCSRMatrixColMapOffd(C) = col_map_offd_C;

   }

   /*-----------------------------------------------------------------------
    *  Free various arrays
    *-----------------------------------------------------------------------*/

   hypre_TFree(B_marker);   
   hypre_TFree(B_ext_diag_i);
   if (B_ext_diag_size)
   {
      hypre_TFree(B_ext_diag_j);
      hypre_TFree(B_ext_diag_data);
   }
   hypre_TFree(B_ext_offd_i);
   if (B_ext_offd_size)
   {
      hypre_TFree(B_ext_offd_j);
      hypre_TFree(B_ext_offd_data);
   }
   if (num_cols_offd_B) hypre_TFree(map_B_to_C);

   return C;
   
}            

/* The following function was formerly part of hypre_ParCSRMatrixExtractBExt
   but the code was removed so it can be used for a corresponding function
   for Boolean matrices
*/

void hypre_ParCSRMatrixExtractBExt_Arrays
( int ** pB_ext_i, int ** pB_ext_j, double ** pB_ext_data, int ** pB_ext_row_map,
  int * num_nonzeros,
  int data, int find_row_map, MPI_Comm comm, hypre_ParCSRCommPkg * comm_pkg,
  int num_cols_B, int num_recvs, int num_sends,
  int first_col_diag, int first_row_index,
  int * recv_vec_starts, int * send_map_starts, int * send_map_elmts,
  int * diag_i, int * diag_j, int * offd_i, int * offd_j, int * col_map_offd,
  double * diag_data, double * offd_data
  )
{
   hypre_ParCSRCommHandle *comm_handle;
   hypre_ParCSRCommPkg *tmp_comm_pkg;
   int *B_int_i;
   int *B_int_j;
   int *B_ext_i;
   int * B_ext_j;
   double * B_ext_data;
   double * B_int_data;
   int * B_int_row_map;
   int * B_ext_row_map;
   int num_procs, my_id;
   int *jdata_recv_vec_starts;
   int *jdata_send_map_starts;
 
   int i, j, k, counter;
   int start_index;
   int j_cnt, j_cnt_rm, jrow;
   int num_rows_B_ext;

   MPI_Comm_size(comm,&num_procs);
   MPI_Comm_rank(comm,&my_id);

   num_rows_B_ext = recv_vec_starts[num_recvs];
   if ( num_rows_B_ext < 0 ) {  /* no B_ext, no communication */
      *pB_ext_i = NULL;
      *pB_ext_j = NULL;
      if ( data ) *pB_ext_data = NULL;
      if ( find_row_map ) *pB_ext_row_map = NULL;
      *num_nonzeros = 0;
      return;
   };
   B_int_i = hypre_CTAlloc(int, send_map_starts[num_sends]+1);
   B_ext_i = hypre_CTAlloc(int, num_rows_B_ext+1);
   *pB_ext_i = B_ext_i;
   if ( find_row_map ) {
      B_int_row_map = hypre_CTAlloc( int, send_map_starts[num_sends]+1 );
      B_ext_row_map = hypre_CTAlloc( int, num_rows_B_ext+1 );
      *pB_ext_row_map = B_ext_row_map;
   };

/*--------------------------------------------------------------------------
 * generate B_int_i through adding number of row-elements of offd and diag
 * for corresponding rows. B_int_i[j+1] contains the number of elements of
 * a row j (which is determined through send_map_elmts) 
 *--------------------------------------------------------------------------*/
   B_int_i[0] = 0;
   j_cnt = 0;
   j_cnt_rm = 0;
   *num_nonzeros = 0;
   for (i=0; i < num_sends; i++)
   {
      for (j = send_map_starts[i]; j < send_map_starts[i+1]; j++)
      {
         jrow = send_map_elmts[j];
         B_int_i[++j_cnt] = offd_i[jrow+1] - offd_i[jrow]
			  + diag_i[jrow+1] - diag_i[jrow];
	    *num_nonzeros += B_int_i[j_cnt];
      }
      if ( find_row_map ) {
         for (j = send_map_starts[i]; j < send_map_starts[i+1]; j++) {
            jrow = send_map_elmts[j];
            B_int_row_map[j_cnt_rm++] = jrow + first_row_index;
         }
      }
   }

/*--------------------------------------------------------------------------
 * initialize communication 
 *--------------------------------------------------------------------------*/
   comm_handle = hypre_ParCSRCommHandleCreate(11,comm_pkg,
		&B_int_i[1],&(B_ext_i[1]) );
   if ( find_row_map ) {
      /* scatter/gather B_int row numbers to form array of B_ext row numbers */
      hypre_ParCSRCommHandleDestroy(comm_handle);
      comm_handle = hypre_ParCSRCommHandleCreate
         (11,comm_pkg, B_int_row_map, B_ext_row_map );
   };

   B_int_j = hypre_CTAlloc(int, *num_nonzeros);
   if (data) B_int_data = hypre_CTAlloc(double, *num_nonzeros);

   jdata_send_map_starts = hypre_CTAlloc(int, num_sends+1);
   jdata_recv_vec_starts = hypre_CTAlloc(int, num_recvs+1);
   start_index = B_int_i[0];
   jdata_send_map_starts[0] = start_index;
   counter = 0;
   for (i=0; i < num_sends; i++)
   {
	*num_nonzeros = counter;
	for (j = send_map_starts[i]; j < send_map_starts[i+1]; j++)
	{
	    jrow = send_map_elmts[j];
	    for (k=diag_i[jrow]; k < diag_i[jrow+1]; k++) 
	    {
		B_int_j[counter] = diag_j[k]+first_col_diag;
		if (data) B_int_data[counter] = diag_data[k];
		counter++;
  	    }
	    for (k=offd_i[jrow]; k < offd_i[jrow+1]; k++) 
	    {
		B_int_j[counter] = col_map_offd[offd_j[k]];
		if (data) B_int_data[counter] = offd_data[k];
		counter++;
  	    }
	   
	}
	*num_nonzeros = counter - *num_nonzeros;
	/* if (data) 
	{
		hypre_BuildCSRJDataType(*num_nonzeros, 
			  &B_int_data[start_index], 
			  &B_int_j[start_index], 
			  &send_matrix_types[i]);	
	}
	else
	{
		MPI_Aint displ[1];
		MPI_Datatype type[1];
		type[0] = MPI_INT;
		MPI_Address(&B_int_j[start_index], &displ[0]);
		MPI_Type_struct(1,num_nonzeros,displ,type,
			&send_matrix_types[i]);
		MPI_Type_commit(&send_matrix_types[i]);
	} */
	start_index += *num_nonzeros;
        jdata_send_map_starts[i+1] = start_index;
   }

   tmp_comm_pkg = hypre_CTAlloc(hypre_ParCSRCommPkg,1);
   hypre_ParCSRCommPkgComm(tmp_comm_pkg) = comm;
   hypre_ParCSRCommPkgNumSends(tmp_comm_pkg) = num_sends;
   hypre_ParCSRCommPkgNumRecvs(tmp_comm_pkg) = num_recvs;
   hypre_ParCSRCommPkgSendProcs(tmp_comm_pkg) = hypre_ParCSRCommPkgSendProcs(comm_pkg);
   hypre_ParCSRCommPkgRecvProcs(tmp_comm_pkg) = hypre_ParCSRCommPkgRecvProcs(comm_pkg);
/*   hypre_ParCSRCommPkgSendMPITypes(tmp_comm_pkg) = send_matrix_types;	 */
   hypre_ParCSRCommPkgSendMapStarts(tmp_comm_pkg) = jdata_send_map_starts; 

   hypre_ParCSRCommHandleDestroy(comm_handle);
   comm_handle = NULL;

/*--------------------------------------------------------------------------
 * after communication exchange B_ext_i[j+1] contains the number of elements
 * of a row j ! 
 * evaluate B_ext_i and compute *num_nonzeros for B_ext 
 *--------------------------------------------------------------------------*/

   for (i=0; i < num_recvs; i++)
	for (j = recv_vec_starts[i]; j < recv_vec_starts[i+1]; j++)
		B_ext_i[j+1] += B_ext_i[j];

   *num_nonzeros = B_ext_i[num_rows_B_ext];

   *pB_ext_j = hypre_CTAlloc(int, *num_nonzeros);
   B_ext_j = *pB_ext_j;
   if (data) {
      *pB_ext_data = hypre_CTAlloc(double, *num_nonzeros);
      B_ext_data = *pB_ext_data;
   };

   for (i=0; i < num_recvs; i++)
   {
	start_index = B_ext_i[recv_vec_starts[i]];
	*num_nonzeros = B_ext_i[recv_vec_starts[i+1]]-start_index;
	jdata_recv_vec_starts[i+1] = B_ext_i[recv_vec_starts[i+1]];
/* 	if (data)
	{
		hypre_BuildCSRJDataType(*num_nonzeros, 
			  &B_ext_data[start_index], 
			  &B_ext_j[start_index], 
			  &recv_matrix_types[i]);	
	}
	else
	{
		MPI_Aint displ[1];
		MPI_Datatype type[1];
		type[0] = MPI_INT;
		MPI_Address(&B_ext_j[start_index], &displ[0]);
		MPI_Type_struct(1,num_nonzeros,displ,type,
			&recv_matrix_types[i]);
		MPI_Type_commit(&recv_matrix_types[i]);
	} */
   }

   /* hypre_ParCSRCommPkgRecvMPITypes(tmp_comm_pkg) = recv_matrix_types;

   comm_handle = hypre_ParCSRCommHandleCreate(0,tmp_comm_pkg,NULL,NULL); */

   hypre_ParCSRCommPkgRecvVecStarts(tmp_comm_pkg) = jdata_recv_vec_starts;

   comm_handle = hypre_ParCSRCommHandleCreate(11,tmp_comm_pkg,B_int_j,B_ext_j);
   hypre_ParCSRCommHandleDestroy(comm_handle);
   comm_handle = NULL;

   if (data)
   {
      comm_handle = hypre_ParCSRCommHandleCreate(1,tmp_comm_pkg,B_int_data,
						B_ext_data);
      hypre_ParCSRCommHandleDestroy(comm_handle);
      comm_handle = NULL;
   }

/*
   for (i=0; i < num_sends; i++)
	MPI_Type_free(&send_matrix_types[i]);

   for (i=0; i < num_recvs; i++)
	MPI_Type_free(&recv_matrix_types[i]);

   hypre_TFree(send_matrix_types);
   hypre_TFree(recv_matrix_types); */

   hypre_TFree(jdata_send_map_starts);
   hypre_TFree(jdata_recv_vec_starts);
   hypre_TFree(tmp_comm_pkg);
   hypre_TFree(B_int_i);
   hypre_TFree(B_int_j);
   if (data) hypre_TFree(B_int_data);
   if ( find_row_map ) hypre_TFree(B_int_row_map);

/* end generic part */
}


/*--------------------------------------------------------------------------
 * hypre_ParCSRMatrixExtractBExt : extracts rows from B which are located on 
 * other processors and needed for multiplication with A locally. The rows
 * are returned as CSRMatrix.
 *--------------------------------------------------------------------------*/

hypre_CSRMatrix * 
hypre_ParCSRMatrixExtractBExt( hypre_ParCSRMatrix *B, hypre_ParCSRMatrix *A, int data)
{
   MPI_Comm comm = hypre_ParCSRMatrixComm(B);
   int first_col_diag = hypre_ParCSRMatrixFirstColDiag(B);
   int first_row_index = hypre_ParCSRMatrixFirstRowIndex(B);
   int *col_map_offd = hypre_ParCSRMatrixColMapOffd(B);

   hypre_ParCSRCommPkg *comm_pkg = hypre_ParCSRMatrixCommPkg(A);
   int num_recvs;
   int *recv_vec_starts;
   int num_sends;
   int *send_map_starts;
   int *send_map_elmts;
 
   hypre_CSRMatrix *diag = hypre_ParCSRMatrixDiag(B);

   int *diag_i = hypre_CSRMatrixI(diag);
   int *diag_j = hypre_CSRMatrixJ(diag);
   double *diag_data = hypre_CSRMatrixData(diag);

   hypre_CSRMatrix *offd = hypre_ParCSRMatrixOffd(B);

   int *offd_i = hypre_CSRMatrixI(offd);
   int *offd_j = hypre_CSRMatrixJ(offd);
   double *offd_data = hypre_CSRMatrixData(offd);

   int num_cols_B, num_nonzeros;
   int num_rows_B_ext;

   hypre_CSRMatrix *B_ext;

   int *B_ext_i;
   int *B_ext_j;
   double *B_ext_data;
   int *idummy;

   /*---------------------------------------------------------------------
    * If there exists no CommPkg for A, a CommPkg is generated using
    * equally load balanced partitionings 
    *--------------------------------------------------------------------*/
   if (!hypre_ParCSRMatrixCommPkg(A))
   {
      hypre_MatvecCommPkgCreate(A);
   }
    
   comm_pkg = hypre_ParCSRMatrixCommPkg(A);
   num_recvs = hypre_ParCSRCommPkgNumRecvs(comm_pkg);
   recv_vec_starts = hypre_ParCSRCommPkgRecvVecStarts(comm_pkg);
   num_sends = hypre_ParCSRCommPkgNumSends(comm_pkg);
   send_map_starts = hypre_ParCSRCommPkgSendMapStarts(comm_pkg);
   send_map_elmts = hypre_ParCSRCommPkgSendMapElmts(comm_pkg);
 
   num_cols_B = hypre_ParCSRMatrixGlobalNumCols(B);
   num_rows_B_ext = recv_vec_starts[num_recvs];

   hypre_ParCSRMatrixExtractBExt_Arrays
      ( &B_ext_i, &B_ext_j, &B_ext_data, &idummy,
        &num_nonzeros,
        data, 0, comm, comm_pkg,
        num_cols_B, num_recvs, num_sends,
        first_col_diag, first_row_index,
        recv_vec_starts, send_map_starts, send_map_elmts,
        diag_i, diag_j, offd_i, offd_j, col_map_offd,
        diag_data, offd_data
         );

   B_ext = hypre_CSRMatrixCreate(num_rows_B_ext,num_cols_B,num_nonzeros);
   hypre_CSRMatrixI(B_ext) = B_ext_i;
   hypre_CSRMatrixJ(B_ext) = B_ext_j;
   if (data) hypre_CSRMatrixData(B_ext) = B_ext_data;

   return B_ext;
}


/*--------------------------------------------------------------------------
 * hypre_ParCSRMatrixTranspose
 *--------------------------------------------------------------------------*/

int
hypre_ParCSRMatrixTranspose( hypre_ParCSRMatrix *A,
                 	     hypre_ParCSRMatrix **AT_ptr,
                	     int data) 
{
   hypre_ParCSRCommHandle *comm_handle;
   MPI_Comm comm = hypre_ParCSRMatrixComm(A);
   hypre_ParCSRCommPkg	*comm_pkg = hypre_ParCSRMatrixCommPkg(A);
   hypre_CSRMatrix      *A_diag   = hypre_ParCSRMatrixDiag(A);
   hypre_CSRMatrix      *A_offd   = hypre_ParCSRMatrixOffd(A);
   int  num_cols = hypre_ParCSRMatrixNumCols(A);
   int  first_row_index = hypre_ParCSRMatrixFirstRowIndex(A);
   int *row_starts = hypre_ParCSRMatrixRowStarts(A);
   int *col_starts = hypre_ParCSRMatrixColStarts(A);

   int	      num_cols_offd = hypre_CSRMatrixNumCols(A_offd);
   int        ierr = 0;
   int	      num_sends, num_recvs, num_cols_offd_AT; 
   int	      i, j, k, index, counter, j_row;
   int        value;

   hypre_ParCSRMatrix *AT;
   hypre_CSRMatrix *AT_diag;
   hypre_CSRMatrix *AT_offd;
   hypre_CSRMatrix *AT_tmp;
   

   int first_row_index_AT, first_col_diag_AT;
   int local_num_rows_AT, local_num_cols_AT;
   

   int *AT_tmp_i;
   int *AT_tmp_j;
   double *AT_tmp_data;

   int *AT_buf_i;
   int *AT_buf_j;
   double *AT_buf_data;

   int *AT_offd_i;
   int *AT_offd_j;
   double *AT_offd_data;
   int *col_map_offd_AT;
   int *row_starts_AT;
   int *col_starts_AT;

   int num_procs, my_id;

   int *recv_procs;
   int *send_procs;
   int *recv_vec_starts;
   int *send_map_starts;
   int *send_map_elmts;
   int *tmp_recv_vec_starts;
   int *tmp_send_map_starts;
   hypre_ParCSRCommPkg *tmp_comm_pkg;

   MPI_Comm_size(comm,&num_procs);   
   MPI_Comm_rank(comm,&my_id);
  
   num_cols_offd_AT = 0;
   counter = 0;
   AT_offd_j = NULL;
   AT_offd_data = NULL;
   col_map_offd_AT = NULL;
 
   /*---------------------------------------------------------------------
    * If there exists no CommPkg for A, a CommPkg is generated using
    * equally load balanced partitionings
    *--------------------------------------------------------------------*/
   if (!comm_pkg)
   {
      hypre_MatvecCommPkgCreate(A);
      comm_pkg = hypre_ParCSRMatrixCommPkg(A); 
   }

   if (num_procs > 1)
   {
      hypre_CSRMatrixTranspose (A_offd, &AT_tmp, data);

      AT_tmp_i = hypre_CSRMatrixI(AT_tmp);
      AT_tmp_j = hypre_CSRMatrixJ(AT_tmp);
      if (data) AT_tmp_data = hypre_CSRMatrixData(AT_tmp);

      num_sends = hypre_ParCSRCommPkgNumSends(comm_pkg);
      num_recvs = hypre_ParCSRCommPkgNumRecvs(comm_pkg);
      recv_procs = hypre_ParCSRCommPkgRecvProcs(comm_pkg);
      send_procs = hypre_ParCSRCommPkgSendProcs(comm_pkg);
      recv_vec_starts = hypre_ParCSRCommPkgRecvVecStarts(comm_pkg);
      send_map_starts = hypre_ParCSRCommPkgSendMapStarts(comm_pkg);
      send_map_elmts = hypre_ParCSRCommPkgSendMapElmts(comm_pkg);

      AT_buf_i = hypre_CTAlloc(int,send_map_starts[num_sends]); 

      for (i=0; i < AT_tmp_i[num_cols_offd]; i++)
	 AT_tmp_j[i] += first_row_index;

      for (i=0; i < num_cols_offd; i++)
         AT_tmp_i[i] = AT_tmp_i[i+1]-AT_tmp_i[i];
	
      comm_handle = hypre_ParCSRCommHandleCreate(12, comm_pkg, AT_tmp_i,
							AT_buf_i);
   }

   hypre_CSRMatrixTranspose( A_diag, &AT_diag, data);

   AT_offd_i = hypre_CTAlloc(int, num_cols+1);

   if (num_procs > 1)
   {   
      hypre_ParCSRCommHandleDestroy(comm_handle);
      comm_handle = NULL;

      tmp_send_map_starts = hypre_CTAlloc(int,num_sends+1);
      tmp_recv_vec_starts = hypre_CTAlloc(int,num_recvs+1);

      tmp_send_map_starts[0] = send_map_starts[0];
      for (i=0; i < num_sends; i++)
      {
	 tmp_send_map_starts[i+1] = tmp_send_map_starts[i];
         for (j=send_map_starts[i]; j < send_map_starts[i+1]; j++)
 	 {
	    tmp_send_map_starts[i+1] += AT_buf_i[j];
	    AT_offd_i[send_map_elmts[j]+1] += AT_buf_i[j];
	 }
      }
      for (i=0; i < num_cols; i++)
	 AT_offd_i[i+1] += AT_offd_i[i];

      tmp_recv_vec_starts[0] = recv_vec_starts[0];
      for (i=0; i < num_recvs; i++)
      {
	 tmp_recv_vec_starts[i+1] = tmp_recv_vec_starts[i];
         for (j=recv_vec_starts[i]; j < recv_vec_starts[i+1]; j++)
         {
            tmp_recv_vec_starts[i+1] +=  AT_tmp_i[j];
         }
      }

      tmp_comm_pkg = hypre_CTAlloc(hypre_ParCSRCommPkg,1);
      hypre_ParCSRCommPkgComm(tmp_comm_pkg) = comm;
      hypre_ParCSRCommPkgNumSends(tmp_comm_pkg) = num_sends;
      hypre_ParCSRCommPkgNumRecvs(tmp_comm_pkg) = num_recvs;
      hypre_ParCSRCommPkgRecvProcs(tmp_comm_pkg) = recv_procs;
      hypre_ParCSRCommPkgSendProcs(tmp_comm_pkg) = send_procs;
      hypre_ParCSRCommPkgRecvVecStarts(tmp_comm_pkg) = tmp_recv_vec_starts;
      hypre_ParCSRCommPkgSendMapStarts(tmp_comm_pkg) = tmp_send_map_starts;

      AT_buf_j = hypre_CTAlloc(int,tmp_send_map_starts[num_sends]);
      comm_handle = hypre_ParCSRCommHandleCreate(12, tmp_comm_pkg, AT_tmp_j,
							AT_buf_j);
      hypre_ParCSRCommHandleDestroy(comm_handle);
      comm_handle = NULL;

      if (data)
      {
         AT_buf_data = hypre_CTAlloc(double,tmp_send_map_starts[num_sends]);
         comm_handle = hypre_ParCSRCommHandleCreate(2,tmp_comm_pkg,AT_tmp_data,
							AT_buf_data);
         hypre_ParCSRCommHandleDestroy(comm_handle);
         comm_handle = NULL;
      }

      hypre_TFree(tmp_recv_vec_starts);
      hypre_TFree(tmp_send_map_starts);
      hypre_TFree(tmp_comm_pkg);
      hypre_CSRMatrixDestroy(AT_tmp);

      if (AT_offd_i[num_cols])
      {
         AT_offd_j = hypre_CTAlloc(int, AT_offd_i[num_cols]);
         if (data) AT_offd_data = hypre_CTAlloc(double, AT_offd_i[num_cols]);
      }
      else
      {
         AT_offd_j = NULL;
         AT_offd_data = NULL;
      }
	 
      counter = 0;
      for (i=0; i < num_sends; i++)
      {
         for (j=send_map_starts[i]; j < send_map_starts[i+1]; j++)
	 {
	    j_row = send_map_elmts[j];
	    index = AT_offd_i[j_row];
	    for (k=0; k < AT_buf_i[j]; k++)
	    {
	       if (data) AT_offd_data[index] = AT_buf_data[counter];
	       AT_offd_j[index++] = AT_buf_j[counter++];
	    }
	    AT_offd_i[j_row] = index;
	 }
      }
      for (i=num_cols; i > 0; i--)
	 AT_offd_i[i] = AT_offd_i[i-1];
      AT_offd_i[0] = 0;

      if (counter)
      {
         qsort0(AT_buf_j,0,counter-1);
         num_cols_offd_AT = 1;
	 value = AT_buf_j[0];
         for (i=1; i < counter; i++)
	 {
	    if (value < AT_buf_j[i])
	    {
	       AT_buf_j[num_cols_offd_AT++] = AT_buf_j[i];
	       value = AT_buf_j[i];
	    }
	 }
      }

      if (num_cols_offd_AT)
         col_map_offd_AT = hypre_CTAlloc(int, num_cols_offd_AT);
      else
         col_map_offd_AT = NULL;

      for (i=0; i < num_cols_offd_AT; i++)
	 col_map_offd_AT[i] = AT_buf_j[i];

      hypre_TFree(AT_buf_i);
      hypre_TFree(AT_buf_j);
      if (data) hypre_TFree(AT_buf_data);

      for (i=0; i < counter; i++)
	 AT_offd_j[i] = hypre_BinarySearch(col_map_offd_AT,AT_offd_j[i],
						num_cols_offd_AT);
   }

   AT_offd = hypre_CSRMatrixCreate(num_cols,num_cols_offd_AT,counter);
   hypre_CSRMatrixI(AT_offd) = AT_offd_i;
   hypre_CSRMatrixJ(AT_offd) = AT_offd_j;
   hypre_CSRMatrixData(AT_offd) = AT_offd_data;




#ifdef HYPRE_NO_GLOBAL_PARTITION
   row_starts_AT = hypre_CTAlloc(int, 2);
   for (i=0; i < 2; i++)
      row_starts_AT[i] = col_starts[i];

   if (row_starts != col_starts)
   {
      col_starts_AT = hypre_CTAlloc(int,2);
      for (i=0; i < 2; i++)
         col_starts_AT[i] = row_starts[i];
   }
   else
   {
      col_starts_AT = row_starts_AT;
   }

   first_row_index_AT =  row_starts_AT[0];
   first_col_diag_AT =  col_starts_AT[0];

   local_num_rows_AT = row_starts_AT[1]-first_row_index_AT ;
   local_num_cols_AT = col_starts_AT[1]-first_col_diag_AT;

#else
   row_starts_AT = hypre_CTAlloc(int,num_procs+1);
   for (i=0; i < num_procs+1; i++)
      row_starts_AT[i] = col_starts[i];

   if (row_starts != col_starts)
   {
      col_starts_AT = hypre_CTAlloc(int,num_procs+1);
      for (i=0; i < num_procs+1; i++)
         col_starts_AT[i] = row_starts[i];
   }
   else
   {
      col_starts_AT = row_starts_AT;
   }
   first_row_index_AT =  row_starts_AT[my_id];
   first_col_diag_AT =  col_starts_AT[my_id];

   local_num_rows_AT = row_starts_AT[my_id+1]-first_row_index_AT ;
   local_num_cols_AT = col_starts_AT[my_id+1]-first_col_diag_AT;


#endif


   AT = hypre_CTAlloc(hypre_ParCSRMatrix,1);
   hypre_ParCSRMatrixComm(AT) = comm;
   hypre_ParCSRMatrixDiag(AT) = AT_diag;
   hypre_ParCSRMatrixOffd(AT) = AT_offd;
   hypre_ParCSRMatrixGlobalNumRows(AT) = hypre_ParCSRMatrixGlobalNumCols(A);
   hypre_ParCSRMatrixGlobalNumCols(AT) = hypre_ParCSRMatrixGlobalNumRows(A);
   hypre_ParCSRMatrixRowStarts(AT) = row_starts_AT;
   hypre_ParCSRMatrixColStarts(AT) = col_starts_AT;
   hypre_ParCSRMatrixColMapOffd(AT) = col_map_offd_AT;
 
   hypre_ParCSRMatrixFirstRowIndex(AT) = first_row_index_AT;
   hypre_ParCSRMatrixFirstColDiag(AT) = first_col_diag_AT;

   hypre_ParCSRMatrixLastRowIndex(AT) = first_row_index_AT + local_num_rows_AT - 1;
   hypre_ParCSRMatrixLastColDiag(AT) = first_col_diag_AT + local_num_cols_AT - 1;

   hypre_ParCSRMatrixOwnsData(AT) = 1;
   hypre_ParCSRMatrixOwnsRowStarts(AT) = 1;
   hypre_ParCSRMatrixOwnsColStarts(AT) = 1;
   if (row_starts_AT == col_starts_AT)
      hypre_ParCSRMatrixOwnsColStarts(AT) = 0;

   hypre_ParCSRMatrixCommPkg(AT) = NULL;
   hypre_ParCSRMatrixCommPkgT(AT) = NULL;

   hypre_ParCSRMatrixRowindices(AT) = NULL;
   hypre_ParCSRMatrixRowvalues(AT) = NULL;
   hypre_ParCSRMatrixGetrowactive(AT) = 0;

   *AT_ptr = AT;
  
   return ierr;
}



/* -----------------------------------------------------------------------------
 * generate a parallel spanning tree (for Maxwell Equation)
 * G_csr is the node to edge connectivity matrix
 * ----------------------------------------------------------------------------- */

void hypre_ParCSRMatrixGenSpanningTree(hypre_ParCSRMatrix *G_csr, int **indices,
                                       int G_type)
{
   int nrows_G, ncols_G, *G_diag_i, *G_diag_j, *GT_diag_mat, i, j, k, edge;
   int *nodes_marked, *edges_marked, *queue, queue_tail, queue_head, node;
   int mypid, nprocs, n_children, *children, nsends, *send_procs, *recv_cnts;
   int nrecvs, *recv_procs, n_proc_array, *proc_array, *pgraph_i, *pgraph_j;
   int parent, proc, proc2, node2, found, *t_indices, tree_size, *T_diag_i;
   int *T_diag_j, *counts, offset;
   MPI_Comm            comm;
   hypre_ParCSRCommPkg *comm_pkg;
   hypre_CSRMatrix     *G_diag;

   /* fetch G matrix (G_type = 0 ==> node to edge) */

   if (G_type == 0)
   {
      nrows_G = hypre_ParCSRMatrixGlobalNumRows(G_csr);
      ncols_G = hypre_ParCSRMatrixGlobalNumCols(G_csr);
      G_diag = hypre_ParCSRMatrixDiag(G_csr);
      G_diag_i = hypre_CSRMatrixI(G_diag);
      G_diag_j = hypre_CSRMatrixJ(G_diag);
   }
   else
   {
      nrows_G = hypre_ParCSRMatrixGlobalNumCols(G_csr);
      ncols_G = hypre_ParCSRMatrixGlobalNumRows(G_csr);
      G_diag = hypre_ParCSRMatrixDiag(G_csr);
      T_diag_i = hypre_CSRMatrixI(G_diag);
      T_diag_j = hypre_CSRMatrixJ(G_diag);
      counts = (int *) malloc(nrows_G * sizeof(int));
      for (i = 0; i < nrows_G; i++) counts[i] = 0;
      for (i = 0; i < T_diag_i[ncols_G]; i++) counts[T_diag_j[i]]++;
      G_diag_i = (int *) malloc((nrows_G+1) * sizeof(int));
      G_diag_j = (int *) malloc(T_diag_i[ncols_G] * sizeof(int));
      G_diag_i[0] = 0;
      for (i = 1; i <= nrows_G; i++) G_diag_i[i] = G_diag_i[i-1] + counts[i-1];
      for (i = 0; i < ncols_G; i++)
      {
         for (j = T_diag_i[i]; j < T_diag_i[i+1]; j++)
         {
            k = T_diag_j[j];
            offset = G_diag_i[k]++;
            G_diag_j[offset] = i;
         }
      }
      G_diag_i[0] = 0;
      for (i = 1; i <= nrows_G; i++) G_diag_i[i] = G_diag_i[i-1] + counts[i-1];
      free(counts);
   }

   /* form G transpose in special form (2 nodes per edge max) */

   GT_diag_mat = (int *) malloc(2 * ncols_G * sizeof(int));
   for (i = 0; i < 2 * ncols_G; i++) GT_diag_mat[i] = -1;
   for (i = 0; i < nrows_G; i++)
   {
      for (j = G_diag_i[i]; j < G_diag_i[i+1]; j++)
      {
         edge = G_diag_j[j];
         if (GT_diag_mat[edge*2] == -1) GT_diag_mat[edge*2] = i;
         else                           GT_diag_mat[edge*2+1] = i;
      }
   }

   /* BFS on the local matrix graph to find tree */

   nodes_marked = (int *) malloc(nrows_G * sizeof(int));
   edges_marked = (int *) malloc(ncols_G * sizeof(int));
   for (i = 0; i < nrows_G; i++) nodes_marked[i] = 0; 
   for (i = 0; i < ncols_G; i++) edges_marked[i] = 0; 
   queue = (int *) malloc(nrows_G * sizeof(int));
   queue_head = 0;
   queue_tail = 1;
   queue[0] = 0;
   nodes_marked[0] = 1;
   while ((queue_tail-queue_head) > 0)
   {
      node = queue[queue_tail-1];
      queue_tail--;
      for (i = G_diag_i[node]; i < G_diag_i[node+1]; i++)
      {
         edge = G_diag_j[i]; 
         if (edges_marked[edge] == 0)
         {
            if (GT_diag_mat[2*edge+1] != -1)
            {
               node2 = GT_diag_mat[2*edge];
               if (node2 == node) node2 = GT_diag_mat[2*edge+1];
               if (nodes_marked[node2] == 0)
               {
                  nodes_marked[node2] = 1;
                  edges_marked[edge] = 1;
                  queue[queue_tail] = node2;
                  queue_tail++;
               }
            }
         }
      }
   }
   free(nodes_marked);
   free(queue);
   free(GT_diag_mat);

   /* fetch the communication information from */

   comm = hypre_ParCSRMatrixComm(G_csr);
   MPI_Comm_rank(comm, &mypid);
   MPI_Comm_size(comm, &nprocs);
   comm_pkg = hypre_ParCSRMatrixCommPkg(G_csr);
   if (nprocs == 1 && comm_pkg == NULL)
   {

      hypre_MatvecCommPkgCreate((hypre_ParCSRMatrix *) G_csr);

      comm_pkg = hypre_ParCSRMatrixCommPkg(G_csr);
   }

   /* construct processor graph based on node-edge connection */
   /* (local edges connected to neighbor processor nodes)     */

   n_children = 0;
   nrecvs = nsends = 0;
   if (nprocs > 1)
   {
      nsends     = hypre_ParCSRCommPkgNumSends(comm_pkg);
      send_procs = hypre_ParCSRCommPkgSendProcs(comm_pkg);
      nrecvs     = hypre_ParCSRCommPkgNumRecvs(comm_pkg);
      recv_procs = hypre_ParCSRCommPkgRecvProcs(comm_pkg);
      proc_array = NULL;
      if ((nsends+nrecvs) > 0)
      {
         n_proc_array = 0;
         proc_array = (int *) malloc((nsends+nrecvs) * sizeof(int));
         for (i = 0; i < nsends; i++) proc_array[i] = send_procs[i];
         for (i = 0; i < nrecvs; i++) proc_array[nsends+i] = recv_procs[i];
         qsort0(proc_array, 0, nsends+nrecvs-1); 
         n_proc_array = 1;
         for (i = 1; i < nrecvs+nsends; i++) 
            if (proc_array[i] != proc_array[n_proc_array])
               proc_array[n_proc_array++] = proc_array[i];
      }
      pgraph_i = (int *) malloc((nprocs+1) * sizeof(int));
      recv_cnts = (int *) malloc(nprocs * sizeof(int));
      MPI_Allgather(&n_proc_array, 1, MPI_INT, recv_cnts, 1, MPI_INT, comm);
      pgraph_i[0] = 0;
      for (i = 1; i <= nprocs; i++)
         pgraph_i[i] = pgraph_i[i-1] + recv_cnts[i-1];
      pgraph_j = (int *) malloc(pgraph_i[nprocs] * sizeof(int));
      MPI_Allgatherv(proc_array, n_proc_array, MPI_INT, pgraph_j, recv_cnts, 
                     pgraph_i, MPI_INT, comm);
      free(recv_cnts);

      /* BFS on the processor graph to determine parent and children */

      nodes_marked = (int *) malloc(nprocs * sizeof(int));
      for (i = 0; i < nprocs; i++) nodes_marked[i] = -1; 
      queue = (int *) malloc(nprocs * sizeof(int));
      queue_head = 0;
      queue_tail = 1;
      node = 0;
      queue[0] = node;
      while ((queue_tail-queue_head) > 0)
      {
         proc = queue[queue_tail-1];
         queue_tail--;
         for (i = pgraph_i[proc]; i < pgraph_i[proc+1]; i++)
         {
            proc2 = pgraph_j[i]; 
            if (nodes_marked[proc2] < 0)
            {
               nodes_marked[proc2] = proc;
               queue[queue_tail] = proc2;
               queue_tail++;
            }
         }
      }
      parent = nodes_marked[mypid];
      n_children = 0;
      for (i = 0; i < nprocs; i++) if (nodes_marked[i] == mypid) n_children++;
      if (n_children == 0) {n_children = 0; children = NULL;}
      else
      {
         children = (int *) malloc(n_children * sizeof(int));
         n_children = 0;
         for (i = 0; i < nprocs; i++) 
            if (nodes_marked[i] == mypid) children[n_children++] = i;
      } 
      free(nodes_marked);
      free(queue);
      free(pgraph_i);
      free(pgraph_j);
   }

   /* first, connection with my parent : if the edge in my parent *
    * is incident to one of my nodes, then my parent will mark it */

   found = 0;
   for (i = 0; i < nrecvs; i++)
   {
      proc = hypre_ParCSRCommPkgRecvProc(comm_pkg, i);
      if (proc == parent)
      {
         found = 1;
         break;
      }
   }

   /* but if all the edges connected to my parent are on my side, *
    * then I will just pick one of them as tree edge              */

   if (found == 0)
   {
      for (i = 0; i < nsends; i++)
      {
         proc = hypre_ParCSRCommPkgSendProc(comm_pkg, i);
         if (proc == parent)
         {
            k = hypre_ParCSRCommPkgSendMapStart(comm_pkg,i);
            edge = hypre_ParCSRCommPkgSendMapElmt(comm_pkg,k);
            edges_marked[edge] = 1;
            break;
         }
      }
   }
   
   /* next, if my processor has an edge incident on one node in my *
    * child, put this edge on the tree. But if there is no such    *
    * edge, then I will assume my child will pick up an edge       */

   for (j = 0; j < n_children; i++)
   {
      proc = children[j];
      for (i = 0; i < nsends; i++)
      {
         proc2 = hypre_ParCSRCommPkgSendProc(comm_pkg, i);
         if (proc == proc2)
         {
            k = hypre_ParCSRCommPkgSendMapStart(comm_pkg,i);
            edge = hypre_ParCSRCommPkgSendMapElmt(comm_pkg,k);
            edges_marked[edge] = 1;
            break;
         }
      }
   }
   if (n_children > 0) free(children);

   /* count the size of the tree */

   tree_size = 0;
   for (i = 0; i < ncols_G; i++)
      if (edges_marked[i] == 1) tree_size++;
   t_indices = (int *) malloc((tree_size+1) * sizeof(int));
   t_indices[0] = tree_size;
   tree_size = 1;
   for (i = 0; i < ncols_G; i++)
      if (edges_marked[i] == 1) t_indices[tree_size++] = i;
   (*indices) = t_indices;
   free(edges_marked);
   if (G_type != 0)
   {
      free(G_diag_i);
      free(G_diag_j);
   }
}

/* -----------------------------------------------------------------------------
 * extract submatrices based on given indices
 * ----------------------------------------------------------------------------- */

void hypre_ParCSRMatrixExtractSubmatrices(hypre_ParCSRMatrix *A_csr, int *indices2,
                                          hypre_ParCSRMatrix ***submatrices)
{
   int    nindices, *indices, nrows_A, *A_diag_i, *A_diag_j, mypid, nprocs;
   int    i, j, k, *proc_offsets1, *proc_offsets2, *itmp_array, *exp_indices;
   int    nnz11, nnz12, nnz21, nnz22, col, ncols_offd, nnz_offd, nnz_diag;
   int    global_nrows, global_ncols, *row_starts, *col_starts, nrows, nnz;
   int    *diag_i, *diag_j, row, *offd_i;
   double *A_diag_a, *diag_a;
   hypre_ParCSRMatrix *A11_csr, *A12_csr, *A21_csr, *A22_csr;
   hypre_CSRMatrix    *A_diag, *diag, *offd;
   MPI_Comm           comm;

   /* -----------------------------------------------------
    * first make sure the incoming indices are in order
    * ----------------------------------------------------- */

   nindices = indices2[0];
   indices  = &(indices2[1]);
   qsort0(indices, 0, nindices-1);

   /* -----------------------------------------------------
    * fetch matrix information
    * ----------------------------------------------------- */

   nrows_A = hypre_ParCSRMatrixGlobalNumRows(A_csr);
   A_diag = hypre_ParCSRMatrixDiag(A_csr);
   A_diag_i = hypre_CSRMatrixI(A_diag);
   A_diag_j = hypre_CSRMatrixJ(A_diag);
   A_diag_a = hypre_CSRMatrixData(A_diag);
   comm = hypre_ParCSRMatrixComm(A_csr);
   MPI_Comm_rank(comm, &mypid);
   MPI_Comm_size(comm, &nprocs);
   if (nprocs > 1)
   {
      printf("ExtractSubmatrices: cannot handle nprocs > 1 yet.\n");
      exit(1);
   }

   /* -----------------------------------------------------
    * compute new matrix dimensions
    * ----------------------------------------------------- */

   proc_offsets1 = (int *) malloc((nprocs+1) * sizeof(int));
   proc_offsets2 = (int *) malloc((nprocs+1) * sizeof(int));
   MPI_Allgather(&nindices, 1, MPI_INT, proc_offsets1, 1, MPI_INT, comm);
   k = 0;
   for (i = 0; i < nprocs; i++) 
   {
      j = proc_offsets1[i];
      proc_offsets1[i] = k;
      k += j;
   } 
   proc_offsets1[nprocs] = k;
   itmp_array = hypre_ParCSRMatrixRowStarts(A_csr);
   for (i = 0; i <= nprocs; i++) 
      proc_offsets2[i] = itmp_array[i] - proc_offsets1[i];

   /* -----------------------------------------------------
    * assign id's to row and col for later processing
    * ----------------------------------------------------- */

   exp_indices = (int *) malloc(nrows_A * sizeof(int));
   for (i = 0; i < nrows_A; i++) exp_indices[i] = -1;
   for (i = 0; i < nindices; i++) 
   {
      if (exp_indices[indices[i]] == -1) exp_indices[indices[i]] = i;
      else
      {
         printf("ExtractSubmatrices: wrong index %d %d\n", i, indices[i]);
         exit(1);
      }
   }
   k = 0;
   for (i = 0; i < nrows_A; i++) 
   {
      if (exp_indices[i] < 0)
      {
         exp_indices[i] = - k - 1;
         k++;
      }
   }

   /* -----------------------------------------------------
    * compute number of nonzeros for each block
    * ----------------------------------------------------- */

   nnz11 = nnz12 = nnz21 = nnz22 = 0;
   for (i = 0; i < nrows_A; i++)
   {
      if (exp_indices[i] >= 0)
      {
         for (j = A_diag_i[i]; j < A_diag_i[i+1]; j++)
         {
            col = A_diag_j[j];
            if (exp_indices[col] >= 0) nnz11++;
            else                       nnz12++;
         }
      }
      else
      {
         for (j = A_diag_i[i]; j < A_diag_i[i+1]; j++)
         {
            col = A_diag_j[j];
            if (exp_indices[col] >= 0) nnz21++;
            else                       nnz22++;
         }
      }
   }

   /* -----------------------------------------------------
    * create A11 matrix (assume sequential for the moment)
    * ----------------------------------------------------- */

   ncols_offd = 0;
   nnz_offd   = 0;
   nnz_diag   = nnz11;
#ifdef HYPRE_NO_GLOBAL_PARTITION


#else
   global_nrows = proc_offsets1[nprocs];
   global_ncols = proc_offsets1[nprocs];
   row_starts = hypre_CTAlloc(int, nprocs+1);
   col_starts = hypre_CTAlloc(int, nprocs+1);
   for (i = 0; i <= nprocs; i++)
   {
      row_starts[i] = proc_offsets1[i];
      col_starts[i] = proc_offsets1[i];
   }
#endif
   A11_csr = hypre_ParCSRMatrixCreate(comm, global_nrows, global_ncols,
                    row_starts, col_starts, ncols_offd, nnz_diag, nnz_offd); 
   nrows = nindices;
   diag_i = hypre_CTAlloc(int, nrows+1);
   diag_j = hypre_CTAlloc(int, nnz_diag);
   diag_a = hypre_CTAlloc(double, nnz_diag);
   nnz = 0;
   row = 0;
   diag_i[0] = 0;
   for (i = 0; i < nrows_A; i++)
   {
      if (exp_indices[i] >= 0)
      {
         for (j = A_diag_i[i]; j < A_diag_i[i+1]; j++)
         {
            col = A_diag_j[j];
            if (exp_indices[col] >= 0)
            {
               diag_j[nnz] = exp_indices[col];
               diag_a[nnz++] = A_diag_a[j];
            }
         }
         row++;
         diag_i[row] = nnz;
      }
   }
   diag = hypre_ParCSRMatrixDiag(A11_csr);
   hypre_CSRMatrixI(diag) = diag_i;
   hypre_CSRMatrixJ(diag) = diag_j;
   hypre_CSRMatrixData(diag) = diag_a;

   offd_i = hypre_CTAlloc(int, nrows+1);
   for (i = 0; i <= nrows; i++) offd_i[i] = 0;
   offd = hypre_ParCSRMatrixOffd(A11_csr);
   hypre_CSRMatrixI(offd) = offd_i;
   hypre_CSRMatrixJ(offd) = NULL;
   hypre_CSRMatrixData(offd) = NULL;

   /* -----------------------------------------------------
    * create A12 matrix (assume sequential for the moment)
    * ----------------------------------------------------- */

   ncols_offd = 0;
   nnz_offd   = 0;
   nnz_diag   = nnz12;
   global_nrows = proc_offsets1[nprocs];
   global_ncols = proc_offsets2[nprocs];
   row_starts = hypre_CTAlloc(int, nprocs+1);
   col_starts = hypre_CTAlloc(int, nprocs+1);
   for (i = 0; i <= nprocs; i++)
   {
      row_starts[i] = proc_offsets1[i];
      col_starts[i] = proc_offsets2[i];
   }
   A12_csr = hypre_ParCSRMatrixCreate(comm, global_nrows, global_ncols,
                    row_starts, col_starts, ncols_offd, nnz_diag, nnz_offd); 
   nrows = nindices;
   diag_i = hypre_CTAlloc(int, nrows+1);
   diag_j = hypre_CTAlloc(int, nnz_diag);
   diag_a = hypre_CTAlloc(double, nnz_diag);
   nnz = 0;
   row = 0;
   diag_i[0] = 0;
   for (i = 0; i < nrows_A; i++)
   {
      if (exp_indices[i] >= 0)
      {
         for (j = A_diag_i[i]; j < A_diag_i[i+1]; j++)
         {
            col = A_diag_j[j];
            if (exp_indices[col] < 0)
            {
               diag_j[nnz] = - exp_indices[col] - 1;
               diag_a[nnz++] = A_diag_a[j];
            }
         }
         row++;
         diag_i[row] = nnz;
      }
   }
   if (nnz > nnz_diag) printf("WARNING WARNING WARNING\n");
   diag = hypre_ParCSRMatrixDiag(A12_csr);
   hypre_CSRMatrixI(diag) = diag_i;
   hypre_CSRMatrixJ(diag) = diag_j;
   hypre_CSRMatrixData(diag) = diag_a;

   offd_i = hypre_CTAlloc(int, nrows+1);
   for (i = 0; i <= nrows; i++) offd_i[i] = 0;
   offd = hypre_ParCSRMatrixOffd(A12_csr);
   hypre_CSRMatrixI(offd) = offd_i;
   hypre_CSRMatrixJ(offd) = NULL;
   hypre_CSRMatrixData(offd) = NULL;

   /* -----------------------------------------------------
    * create A21 matrix (assume sequential for the moment)
    * ----------------------------------------------------- */

   ncols_offd = 0;
   nnz_offd   = 0;
   nnz_diag   = nnz21;
   global_nrows = proc_offsets2[nprocs];
   global_ncols = proc_offsets1[nprocs];
   row_starts = hypre_CTAlloc(int, nprocs+1);
   col_starts = hypre_CTAlloc(int, nprocs+1);
   for (i = 0; i <= nprocs; i++)
   {
      row_starts[i] = proc_offsets2[i];
      col_starts[i] = proc_offsets1[i];
   }
   A21_csr = hypre_ParCSRMatrixCreate(comm, global_nrows, global_ncols,
                    row_starts, col_starts, ncols_offd, nnz_diag, nnz_offd); 
   nrows = nrows_A - nindices;
   diag_i = hypre_CTAlloc(int, nrows+1);
   diag_j = hypre_CTAlloc(int, nnz_diag);
   diag_a = hypre_CTAlloc(double, nnz_diag);
   nnz = 0;
   row = 0;
   diag_i[0] = 0;
   for (i = 0; i < nrows_A; i++)
   {
      if (exp_indices[i] < 0)
      {
         for (j = A_diag_i[i]; j < A_diag_i[i+1]; j++)
         {
            col = A_diag_j[j];
            if (exp_indices[col] >= 0)
            {
               diag_j[nnz] = exp_indices[col];
               diag_a[nnz++] = A_diag_a[j];
            }
         }
         row++;
         diag_i[row] = nnz;
      }
   }
   diag = hypre_ParCSRMatrixDiag(A21_csr);
   hypre_CSRMatrixI(diag) = diag_i;
   hypre_CSRMatrixJ(diag) = diag_j;
   hypre_CSRMatrixData(diag) = diag_a;

   offd_i = hypre_CTAlloc(int, nrows+1);
   for (i = 0; i <= nrows; i++) offd_i[i] = 0;
   offd = hypre_ParCSRMatrixOffd(A21_csr);
   hypre_CSRMatrixI(offd) = offd_i;
   hypre_CSRMatrixJ(offd) = NULL;
   hypre_CSRMatrixData(offd) = NULL;

   /* -----------------------------------------------------
    * create A22 matrix (assume sequential for the moment)
    * ----------------------------------------------------- */

   ncols_offd = 0;
   nnz_offd   = 0;
   nnz_diag   = nnz22;
   global_nrows = proc_offsets2[nprocs];
   global_ncols = proc_offsets2[nprocs];
   row_starts = hypre_CTAlloc(int, nprocs+1);
   col_starts = hypre_CTAlloc(int, nprocs+1);
   for (i = 0; i <= nprocs; i++)
   {
      row_starts[i] = proc_offsets2[i];
      col_starts[i] = proc_offsets2[i];
   }
   A22_csr = hypre_ParCSRMatrixCreate(comm, global_nrows, global_ncols,
                    row_starts, col_starts, ncols_offd, nnz_diag, nnz_offd); 
   nrows = nrows_A - nindices;
   diag_i = hypre_CTAlloc(int, nrows+1);
   diag_j = hypre_CTAlloc(int, nnz_diag);
   diag_a = hypre_CTAlloc(double, nnz_diag);
   nnz = 0;
   row = 0;
   diag_i[0] = 0;
   for (i = 0; i < nrows_A; i++)
   {
      if (exp_indices[i] < 0)
      {
         for (j = A_diag_i[i]; j < A_diag_i[i+1]; j++)
         {
            col = A_diag_j[j];
            if (exp_indices[col] < 0)
            {
               diag_j[nnz] = - exp_indices[col] - 1;
               diag_a[nnz++] = A_diag_a[j];
            }
         }
         row++;
         diag_i[row] = nnz;
      }
   }
   diag = hypre_ParCSRMatrixDiag(A22_csr);
   hypre_CSRMatrixI(diag) = diag_i;
   hypre_CSRMatrixJ(diag) = diag_j;
   hypre_CSRMatrixData(diag) = diag_a;

   offd_i = hypre_CTAlloc(int, nrows+1);
   for (i = 0; i <= nrows; i++) offd_i[i] = 0;
   offd = hypre_ParCSRMatrixOffd(A22_csr);
   hypre_CSRMatrixI(offd) = offd_i;
   hypre_CSRMatrixJ(offd) = NULL;
   hypre_CSRMatrixData(offd) = NULL;

   /* -----------------------------------------------------
    * hand the matrices back to the caller and clean up 
    * ----------------------------------------------------- */

   (*submatrices)[0] = A11_csr;
   (*submatrices)[1] = A12_csr;
   (*submatrices)[2] = A21_csr;
   (*submatrices)[3] = A22_csr;
   free(proc_offsets1);
   free(proc_offsets2);
   free(exp_indices);
}

/* -----------------------------------------------------------------------------
 * extract submatrices of a rectangular matrix
 * ----------------------------------------------------------------------------- */

void hypre_ParCSRMatrixExtractRowSubmatrices(hypre_ParCSRMatrix *A_csr, int *indices2,
                                             hypre_ParCSRMatrix ***submatrices)
{
   int    nindices, *indices, nrows_A, *A_diag_i, *A_diag_j, mypid, nprocs;
   int    i, j, k, *proc_offsets1, *proc_offsets2, *itmp_array, *exp_indices;
   int    nnz11, nnz21, col, ncols_offd, nnz_offd, nnz_diag, *A_offd_i, *A_offd_j;
   int    global_nrows, global_ncols, *row_starts, *col_starts, nrows, nnz;
   int    *diag_i, *diag_j, row, *offd_i, *offd_j, nnz11_offd, nnz21_offd;
   double *A_diag_a, *diag_a, *A_offd_a, *offd_a;
   hypre_ParCSRMatrix *A11_csr, *A21_csr;
   hypre_CSRMatrix    *A_diag, *diag, *A_offd, *offd;
   MPI_Comm           comm;

   /* -----------------------------------------------------
    * first make sure the incoming indices are in order
    * ----------------------------------------------------- */

   nindices = indices2[0];
   indices  = &(indices2[1]);
   qsort0(indices, 0, nindices-1);

   /* -----------------------------------------------------
    * fetch matrix information
    * ----------------------------------------------------- */

   nrows_A = hypre_ParCSRMatrixGlobalNumRows(A_csr);
   A_diag = hypre_ParCSRMatrixDiag(A_csr);
   A_diag_i = hypre_CSRMatrixI(A_diag);
   A_diag_j = hypre_CSRMatrixJ(A_diag);
   A_diag_a = hypre_CSRMatrixData(A_diag);
   A_offd = hypre_ParCSRMatrixOffd(A_csr);
   A_offd_i = hypre_CSRMatrixI(A_offd);
   A_offd_j = hypre_CSRMatrixJ(A_offd);
   A_offd_a = hypre_CSRMatrixData(A_offd);
   comm = hypre_ParCSRMatrixComm(A_csr);
   MPI_Comm_rank(comm, &mypid);
   MPI_Comm_size(comm, &nprocs);

   /* -----------------------------------------------------
    * compute new matrix dimensions
    * ----------------------------------------------------- */

   proc_offsets1 = (int *) malloc((nprocs+1) * sizeof(int));
   proc_offsets2 = (int *) malloc((nprocs+1) * sizeof(int));
   MPI_Allgather(&nindices, 1, MPI_INT, proc_offsets1, 1, MPI_INT, comm);
   k = 0;
   for (i = 0; i < nprocs; i++) 
   {
      j = proc_offsets1[i];
      proc_offsets1[i] = k;
      k += j;
   } 
   proc_offsets1[nprocs] = k;
   itmp_array = hypre_ParCSRMatrixRowStarts(A_csr);
   for (i = 0; i <= nprocs; i++) 
      proc_offsets2[i] = itmp_array[i] - proc_offsets1[i];

   /* -----------------------------------------------------
    * assign id's to row and col for later processing
    * ----------------------------------------------------- */

   exp_indices = (int *) malloc(nrows_A * sizeof(int));
   for (i = 0; i < nrows_A; i++) exp_indices[i] = -1;
   for (i = 0; i < nindices; i++) 
   {
      if (exp_indices[indices[i]] == -1) exp_indices[indices[i]] = i;
      else
      {
         printf("ExtractRowSubmatrices: wrong index %d %d\n", i, indices[i]);
         exit(1);
      }
   }
   k = 0;
   for (i = 0; i < nrows_A; i++) 
   {
      if (exp_indices[i] < 0)
      {
         exp_indices[i] = - k - 1;
         k++;
      }
   }

   /* -----------------------------------------------------
    * compute number of nonzeros for each block
    * ----------------------------------------------------- */

   nnz11 = nnz21 = nnz11_offd = nnz21_offd = 0;
   for (i = 0; i < nrows_A; i++)
   {
      if (exp_indices[i] >= 0)
      {
         for (j = A_diag_i[i]; j < A_diag_i[i+1]; j++)
         {
            col = A_diag_j[j];
            if (exp_indices[col] >= 0) nnz11++;
         }
         nnz11_offd += A_offd_i[i+1] - A_offd_i[i];
      }
      else
      {
         for (j = A_diag_i[i]; j < A_diag_i[i+1]; j++)
         {
            col = A_diag_j[j];
            if (exp_indices[col] < 0) nnz21++;
         }
         nnz21_offd += A_offd_i[i+1] - A_offd_i[i];
      }
   }

   /* -----------------------------------------------------
    * create A11 matrix (assume sequential for the moment)
    * ----------------------------------------------------- */

   ncols_offd = hypre_CSRMatrixNumCols(hypre_ParCSRMatrixDiag(A_csr));
   nnz_diag   = nnz11;
   nnz_offd   = nnz11_offd; 

   global_nrows = proc_offsets1[nprocs];
   itmp_array   = hypre_ParCSRMatrixColStarts(A_csr);
   global_ncols = itmp_array[nprocs];
   row_starts = hypre_CTAlloc(int, nprocs+1);
   col_starts = hypre_CTAlloc(int, nprocs+1);
   for (i = 0; i <= nprocs; i++)
   {
      row_starts[i] = proc_offsets1[i];
      col_starts[i] = itmp_array[i];
   }
   A11_csr = hypre_ParCSRMatrixCreate(comm, global_nrows, global_ncols,
                    row_starts, col_starts, ncols_offd, nnz_diag, nnz_offd); 
   nrows = nindices;
   diag_i = hypre_CTAlloc(int, nrows+1);
   diag_j = hypre_CTAlloc(int, nnz_diag);
   diag_a = hypre_CTAlloc(double, nnz_diag);
   nnz = 0;
   row = 0;
   diag_i[0] = 0;
   for (i = 0; i < nrows_A; i++)
   {
      if (exp_indices[i] >= 0)
      {
         for (j = A_diag_i[i]; j < A_diag_i[i+1]; j++)
         {
            col = A_diag_j[j];
            if (exp_indices[col] >= 0)
            {
               diag_j[nnz] = exp_indices[col];
               diag_a[nnz++] = A_diag_a[j];
            }
         }
         row++;
         diag_i[row] = nnz;
      }
   }
   diag = hypre_ParCSRMatrixDiag(A11_csr);
   hypre_CSRMatrixI(diag) = diag_i;
   hypre_CSRMatrixJ(diag) = diag_j;
   hypre_CSRMatrixData(diag) = diag_a;

   offd_i = hypre_CTAlloc(int, nrows+1);
   offd_j = hypre_CTAlloc(int, nnz_offd);
   offd_a = hypre_CTAlloc(double, nnz_offd);
   nnz = 0;
   row = 0;
   offd_i[0] = 0;
   for (i = 0; i < nrows_A; i++)
   {
      if (exp_indices[i] >= 0)
      {
         for (j = A_offd_i[i]; j < A_offd_i[i+1]; j++)
         {
            offd_j[nnz] = A_offd_j[j];
            offd_a[nnz++] = A_diag_a[j];
         }
         row++;
         offd_i[row] = nnz;
      }
   }
   offd = hypre_ParCSRMatrixOffd(A11_csr);
   hypre_CSRMatrixI(offd) = offd_i;
   hypre_CSRMatrixJ(offd) = offd_j;
   hypre_CSRMatrixData(offd) = offd_a;

   /* -----------------------------------------------------
    * create A21 matrix
    * ----------------------------------------------------- */

   ncols_offd = hypre_CSRMatrixNumCols(hypre_ParCSRMatrixDiag(A_csr));
   nnz_offd   = nnz21_offd;
   nnz_diag   = nnz21;
   global_nrows = proc_offsets2[nprocs];
   itmp_array   = hypre_ParCSRMatrixColStarts(A_csr);
   global_ncols = itmp_array[nprocs];
   row_starts = hypre_CTAlloc(int, nprocs+1);
   col_starts = hypre_CTAlloc(int, nprocs+1);
   for (i = 0; i <= nprocs; i++)
   {
      row_starts[i] = proc_offsets2[i];
      col_starts[i] = itmp_array[i];
   }
   A21_csr = hypre_ParCSRMatrixCreate(comm, global_nrows, global_ncols,
                    row_starts, col_starts, ncols_offd, nnz_diag, nnz_offd); 
   nrows = nrows_A - nindices;
   diag_i = hypre_CTAlloc(int, nrows+1);
   diag_j = hypre_CTAlloc(int, nnz_diag);
   diag_a = hypre_CTAlloc(double, nnz_diag);
   nnz = 0;
   row = 0;
   diag_i[0] = 0;
   for (i = 0; i < nrows_A; i++)
   {
      if (exp_indices[i] < 0)
      {
         for (j = A_diag_i[i]; j < A_diag_i[i+1]; j++)
         {
            diag_j[nnz] = A_diag_j[j];
            diag_a[nnz++] = A_diag_a[j];
         }
         row++;
         diag_i[row] = nnz;
      }
   }
   diag = hypre_ParCSRMatrixDiag(A21_csr);
   hypre_CSRMatrixI(diag) = diag_i;
   hypre_CSRMatrixJ(diag) = diag_j;
   hypre_CSRMatrixData(diag) = diag_a;

   offd_i = hypre_CTAlloc(int, nrows+1);
   offd_j = hypre_CTAlloc(int, nnz_offd);
   offd_a = hypre_CTAlloc(double, nnz_offd);
   nnz = 0;
   row = 0;
   offd_i[0] = 0;
   for (i = 0; i < nrows_A; i++)
   {
      if (exp_indices[i] < 0)
      {
         for (j = A_offd_i[i]; j < A_offd_i[i+1]; j++)
         {
            offd_j[nnz] = A_offd_j[j];
            offd_a[nnz++] = A_diag_a[j];
         }
         row++;
         offd_i[row] = nnz;
      }
   }
   offd = hypre_ParCSRMatrixOffd(A21_csr);
   hypre_CSRMatrixI(offd) = offd_i;
   hypre_CSRMatrixJ(offd) = offd_j;
   hypre_CSRMatrixData(offd) = offd_a;

   /* -----------------------------------------------------
    * hand the matrices back to the caller and clean up 
    * ----------------------------------------------------- */

   (*submatrices)[0] = A11_csr;
   (*submatrices)[1] = A21_csr;
   free(proc_offsets1);
   free(proc_offsets2);
   free(exp_indices);
}

/* -----------------------------------------------------------------------------
 * return the sum of all local elements of the matrix
 * ----------------------------------------------------------------------------- */

double hypre_ParCSRMatrixLocalSumElts( hypre_ParCSRMatrix * A )
{
   hypre_CSRMatrix * A_diag = hypre_ParCSRMatrixDiag( A );
   hypre_CSRMatrix * A_offd = hypre_ParCSRMatrixOffd( A );

   return hypre_CSRMatrixSumElts(A_diag) + hypre_CSRMatrixSumElts(A_offd);
}
