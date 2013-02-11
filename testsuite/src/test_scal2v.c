/*

   BLIS    
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2012, The University of Texas

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

#include "blis2.h"
#include "test_libblis.h"


// Static variables.
static char*     op_str                    = "scal2v";
static char*     o_types                   = "vv";  // x y
static char*     p_types                   = "c";   // conjx
static thresh_t  thresh[BLIS_NUM_FP_TYPES] = { { 1e-04, 1e-05 },   // warn, pass for s
                                               { 1e-04, 1e-05 },   // warn, pass for c
                                               { 1e-13, 1e-14 },   // warn, pass for d
                                               { 1e-13, 1e-14 } }; // warn, pass for z

// Local prototypes.
void libblis_test_scal2v_deps( test_params_t* params,
                               test_op_t*     op );

void libblis_test_scal2v_experiment( test_params_t* params,
                                     test_op_t*     op,
                                     mt_impl_t      impl,
                                     num_t          datatype,
                                     char*          pc_str,
                                     char*          sc_str,
                                     dim_t          p_cur,
                                     double*        perf,
                                     double*        resid );

void libblis_test_scal2v_impl( mt_impl_t impl,
                               obj_t*    alpha,
                               obj_t*    x,
                               obj_t*    y );

void libblis_test_scal2v_check( obj_t*  alpha,
                                obj_t*  x,
                                obj_t*  y,
                                obj_t*  y_orig,
                                double* resid );



void libblis_test_scal2v_deps( test_params_t* params, test_op_t* op )
{
	libblis_test_randv( params, &(op->ops->randv) );
	libblis_test_fnormv( params, &(op->ops->fnormv) );
	libblis_test_subv( params, &(op->ops->subv) );
	libblis_test_copyv( params, &(op->ops->copyv) );
	libblis_test_scalv( params, &(op->ops->scalv) );
}



void libblis_test_scal2v( test_params_t* params, test_op_t* op )
{

	// Return early if this test has already been done.
	if ( op->test_done == TRUE ) return;

	// Call dependencies first.
	if ( TRUE ) libblis_test_scal2v_deps( params, op );

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
		                        libblis_test_scal2v_experiment );
	}
}



void libblis_test_scal2v_experiment( test_params_t* params,
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

	dim_t        m;

	conj_t       conjx;

	obj_t        alpha, x, y;
	obj_t        y_save;


	// Map the dimension specifier to an actual dimension.
	m = libblis_test_get_dim_from_prob_size( op->dim_spec[0], p_cur );

	// Map parameter characters to BLIS constants.
	bl2_param_map_char_to_blis_conj( pc_str[0], &conjx );

	// Create test scalars.
	bl2_obj_init_scalar( datatype, &alpha );

	// Create test operands (vectors and/or matrices).
	libblis_test_vobj_create( params, datatype, sc_str[0], m, &x );
	libblis_test_vobj_create( params, datatype, sc_str[1], m, &y );
	libblis_test_vobj_create( params, datatype, sc_str[1], m, &y_save );

	// Set alpha.
	//bl2_setsc( sqrt(2.0)/2.0, sqrt(2.0)/2.0, &alpha );
	//bl2_copysc( &BLIS_TWO, &alpha );
	if ( bl2_obj_is_real( y ) )
		bl2_setsc( -2.0,  0.0, &alpha );
	else
		bl2_setsc(  0.0, -2.0, &alpha );

	// Randomize x and y, and save y.
	bl2_randv( &x );
	bl2_randv( &y );
	bl2_copyv( &y, &y_save );

	// Apply the parameters.
	bl2_obj_set_conj( conjx, x );

	// Repeat the experiment n_repeats times and record results. 
	for ( i = 0; i < n_repeats; ++i )
	{
		bl2_copyv( &y_save, &y );

		time = bl2_clock();

		libblis_test_scal2v_impl( impl, &alpha, &x, &y );

		time_min = bl2_clock_min_diff( time_min, time );
	}

	// Estimate the performance of the best experiment repeat.
	*perf = ( 2.0 * m ) / time_min / FLOPS_PER_UNIT_PERF;
	if ( bl2_obj_is_complex( y ) ) *perf *= 4.0;

	// Perform checks.
	libblis_test_scal2v_check( &alpha, &x, &y, &y_save, resid );

	// Free the test objects.
	bl2_obj_free( &x );
	bl2_obj_free( &y );
	bl2_obj_free( &y_save );
}



void libblis_test_scal2v_impl( mt_impl_t impl,
                               obj_t*    alpha,
                               obj_t*    x,
                               obj_t*    y )
{
	switch ( impl )
	{
		case BLIS_TEST_SEQ_FRONT_END:
		bl2_scal2v( alpha, x, y );
		break;

		default:
		libblis_test_printf_error( "Invalid implementation type.\n" );
	}
}



void libblis_test_scal2v_check( obj_t*  alpha,
                                obj_t*  x,
                                obj_t*  y,
                                obj_t*  y_orig,
                                double* resid )
{
	num_t  dt      = bl2_obj_datatype( *y );
	num_t  dt_real = bl2_obj_datatype_proj_to_real( *y );

	dim_t  m       = bl2_obj_vector_dim( *y );

	obj_t  x_temp;
	obj_t  norm;

	double junk;

	//
	// Pre-conditions:
	// - x is randomized.
	// - y_orig is set to one.
	// Note:
	// - alpha should have a non-zero imaginary component in the complex
	//   cases in order to more fully exercise the implementation.
	//
	// Under these conditions, we assume that the implementation for
	//
	//   y := alpha * conjx(x)
	//
	// is functioning correctly if
	//
	//   fnorm( y - alpha * conjx(x) )
	//
	// is negligible.
	//

	bl2_obj_init_scalar( dt_real, &norm );

	bl2_obj_create( dt, m, 1, 0, 0, &x_temp );

	bl2_copyv( x, &x_temp );

	bl2_scalv( alpha, &x_temp );

	bl2_subv( &x_temp, y );
	bl2_fnormv( y, &norm );
	bl2_getsc( &norm, resid, &junk );

	bl2_obj_free( &x_temp );
}

