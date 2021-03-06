#include <stdio.h>

#include "DllExportPreProc.h"


MAKE_MPIF(MPI_COMM_SIZE   ,PARAM3,CALL3)
MAKE_MPIF(MPI_COMM_RANK   ,PARAM3,CALL3)
MAKE_MPIF(MPI_WAITALL     ,PARAM4,CALL4)
MAKE_MPIF(MPI_IRECV       ,PARAM8,CALL8)
MAKE_MPIF(MPI_ISSEND      ,PARAM8,CALL8)
MAKE_MPIF(MPI_WAITANY     ,PARAM5,CALL5)
MAKE_MPIF(MPI_ALLGATHER   ,PARAM8,CALL8)
MAKE_MPIF(MPI_BARRIER     ,PARAM2,CALL2)
MAKE_MPIF(MPI_BCAST       ,PARAM6,CALL6)
MAKE_MPIF(MPI_REDUCE      ,PARAM8,CALL8)
MAKE_MPIF(MPI_ALLREDUCE   ,PARAM7,CALL7)
MAKE_MPIF(MPI_SEND        ,PARAM7,CALL7)
MAKE_MPIF(MPI_RECV        ,PARAM8,CALL8)
MAKE_MPIF(MPI_INITIALIZED ,PARAM2,CALL2)
MAKE_MPIF(MPI_INIT        ,PARAM1,CALL1)
