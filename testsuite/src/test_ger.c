/*

   BLIS    
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2013, The University of Texas

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    - Neither the name of The University of Texas nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "blis.h"
#include "test_libblis.h"


// Static variables.
static char*     op_str                    = "ger";
static char*     o_types                   = "vvm"; // x y a
static char*     p_types                   = "cc";  // transa conjx
static thresh_t  thresh[BLIS_NUM_FP_TYPES] = { { 1e-04, 1e-05 },   // warn, pass for s
                                               { 1e-04, 1e-05 },   // warn, pass for c
                                               { 1e-13, 1e-14 },   // warn, pass for d
                                               { 1e-13, 1e-14 } }; // warn, pass for z

// Local prototypes.
void libblis_test_ger_deps( test_params_t* params,
                            test_op_t*     op );

void libblis_test_ger_experiment( test_params_t* params,
                                  test_op_t*     op,
                                  mt_impl_t      impl,
                                  num_t          datatype,
                                  char*          pc_str,
                                  char*          sc_str,
                                  dim_t          p_cur,
                                  double*        perf,
                                  double*        resid );

void libblis_test_ger_impl( mt_impl_t impl,
                            obj_t*    alpha,
                            obj_t*    x,
                            obj_t*    y,
                            obj_t*    a );

void libblis_test_ger_check( obj_t*  alpha,
                             obj_t*  x,
                             obj_t*  y,
                             obj_t*  a,
                             obj_t*  a_orig,
                             double* resid );



void libblis_test_ger_deps( test_params_t* params, test_op_t* op )
{
	libblis_test_randv( params, &(op->ops->randv) );
	libblis_test_fnormv( params, &(op->ops->fnormv) );
	libblis_test_subv( params, &(op->ops->subv) );
	libblis_test_scal2v( params, &(op->ops->scal2v) );
	libblis_test_dotv( params, &(op->ops->dotv) );
	libblis_test_gemv( params, &(op->ops->gemv) );
}



void libblis_test_ger( test_params_t* params, test_op_t* op )
{

	// Return early if this test has already been done.
	if ( op->test_done == TRUE ) return;

	// Call dependencies first.
	if ( TRUE ) libblis_test_ger_deps( params, op );

	// Execute the test driver for each implementation requested.
	if ( op->front_seq == ENABLE )
	{
		libblis_test_op_driver( params,
		                        op,
		                        BLIS_TEST_SEQ_FRONT_END,
		                        op_str,
		                        p_types,
		                        o_types,
		                        thresh,
		                        libblis_test_ger_experiment );
	}
}



void libblis_test_ger_experiment( test_params_t* params,
                                  test_op_t*     op,
                                  mt_impl_t      impl,
                                  num_t          datatype,
                                  char*          pc_str,
                                  char*          sc_str,
                                  dim_t          p_cur,
                                  double*        perf,
                                  double*        resid )
{
	unsigned int n_repeats = params->n_repeats;
	unsigned int i;

	double       time_min  = 1e9;
	double       time;

	dim_t        m, n;

	conj_t       conjx, conjy;

	obj_t        alpha, x, y, a;
	obj_t        a_save;


	// Map the dimension specifier to actual dimensions.
	m = libblis_test_get_dim_from_prob_size( op->dim_spec[0], p_cur );
	n = libblis_test_get_dim_from_prob_size( op->dim_spec[1], p_cur );

	// Map parameter characters to BLIS constants.
	bli_param_map_char_to_blis_conj( pc_str[0], &conjx );
	bli_param_map_char_to_blis_conj( pc_str[1], &conjy );

	// Create test scalars.
	bli_obj_init_scalar( datatype, &alpha );

	// Create test operands (vectors and/or matrices).
	libblis_test_vobj_create( params, datatype,
		                      sc_str[0], m,    &x );
	libblis_test_vobj_create( params, datatype,
		                      sc_str[1], n,    &y );
	libblis_test_mobj_create( params, datatype, BLIS_NO_TRANSPOSE,
		                      sc_str[2], m, n, &a );
	libblis_test_mobj_create( params, datatype, BLIS_NO_TRANSPOSE,
		                      sc_str[2], m, n, &a_save );

	// Set alpha.
	if ( bli_obj_is_real( a ) )
		bli_setsc( -1.0,  1.0, &alpha );
	else
		bli_setsc( -1.0,  1.0, &alpha );

	// Randomize x and y.
	bli_randv( &x );
	bli_randv( &y );

	// Initialize A to identity and save.
	bli_setm( &BLIS_ZERO, &a );
	bli_setd( &BLIS_ONE,  &a );
	bli_copym( &a, &a_save );

	// Apply the parameters.
	bli_obj_set_conj( conjx, x );
	bli_obj_set_conj( conjy, y );

	// Repeat the experiment n_repeats times and record results. 
	for ( i = 0; i < n_repeats; ++i )
	{
		bli_copym( &a_save, &a );

		time = bli_clock();

		libblis_test_ger_impl( impl, &alpha, &x, &y, &a );

		time_min = bli_clock_min_diff( time_min, time );
	}

	// Estimate the performance of the best experiment repeat.
	*perf = ( 2.0 * m * n ) / time_min / FLOPS_PER_UNIT_PERF;
	if ( bli_obj_is_complex( a ) ) *perf *= 4.0;

	// Perform checks.
	libblis_test_ger_check( &alpha, &x, &y, &a, &a_save, resid );

	// Zero out performance and residual if output matrix is empty.
	libblis_test_check_empty_problem( &a, perf, resid );

	// Free the test objects.
	bli_obj_free( &x );
	bli_obj_free( &y );
	bli_obj_free( &a );
	bli_obj_free( &a_save );
}



void libblis_test_ger_impl( mt_impl_t impl,
                            obj_t*    alpha,
                            obj_t*    x,
                            obj_t*    y,
                            obj_t*    a )
{
	switch ( impl )
	{
		case BLIS_TEST_SEQ_FRONT_END:
		bli_ger( alpha, x, y, a );
		break;

		default:
		libblis_test_printf_error( "Invalid implementation type.\n" );
	}
}



void libblis_test_ger_check( obj_t*  alpha,
                             obj_t*  x,
                             obj_t*  y,
                             obj_t*  a,
                             obj_t*  a_orig,
                             double* resid )
{
	num_t  dt      = bli_obj_datatype( *a );
	num_t  dt_real = bli_obj_datatype_proj_to_real( *a );

	dim_t  m_a     = bli_obj_length( *a );
	dim_t  n_a     = bli_obj_width( *a );

	obj_t  t, v, w;
	obj_t  tau, rho, norm;

	double junk;

	//
	// Pre-conditions:
	// - x is randomized.
	// - y is randomized.
	// - a is identity.
	// Note:
	// - alpha should have a non-zero imaginary component in the
	//   complex cases in order to more fully exercise the implementation.
	//
	// Under these conditions, we assume that the implementation for
	//
	//   A := A_orig + alpha * conjx(x) * conjy(y)
	//
	// is functioning correctly if
	//
	//   fnorm( v - w )
	//
	// is negligible, where
	//
	//   v = A * t
	//   w = ( A_orig + alpha * conjx(x) * conjy(y)^T ) * t
	//     =   A_orig * t + alpha * conjx(x) * conjy(y)^T * t
	//     =   A_orig * t + alpha * conjx(x) * rho
	//     =   A_orig * t + w
	//

	bli_obj_init_scalar( dt,      &tau );
	bli_obj_init_scalar( dt,      &rho );
	bli_obj_init_scalar( dt_real, &norm );

	bli_obj_create( dt, n_a, 1, 0, 0, &t );
	bli_obj_create( dt, m_a, 1, 0, 0, &v );
	bli_obj_create( dt, m_a, 1, 0, 0, &w );

	bli_setsc( 1.0/( double )n_a, -1.0/( double )n_a, &tau );
	bli_setv( &tau, &t );

	bli_gemv( &BLIS_ONE, a, &t, &BLIS_ZERO, &v );

	bli_dotv( y, &t, &rho );
	bli_mulsc( alpha, &rho );
	bli_scal2v( &rho, x, &w );
	bli_gemv( &BLIS_ONE, a_orig, &t, &BLIS_ONE, &w );

	bli_subv( &w, &v );
	bli_fnormv( &v, &norm );
	bli_getsc( &norm, resid, &junk );

	bli_obj_free( &t );
	bli_obj_free( &v );
	bli_obj_free( &w );
}

