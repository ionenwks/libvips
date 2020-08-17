// VIPS image wrapper

/*

    This file is part of VIPS.
    
    VIPS is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA

 */

/*

    These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

#ifndef VIPS_VIMAGE_H
#define VIPS_VIMAGE_H

#include <list>
#include <complex>
#include <vector>

#include <cstring>

#include <vips/vips.h>

VIPS_NAMESPACE_START

/* Small utility things.
 */

VIPS_CPLUSPLUS_API std::vector<double> to_vectorv( int n, ... );
VIPS_CPLUSPLUS_API std::vector<double> to_vector( double value );
VIPS_CPLUSPLUS_API std::vector<double> to_vector( int n, double array[] );
VIPS_CPLUSPLUS_API std::vector<double> negate( std::vector<double> value );
VIPS_CPLUSPLUS_API std::vector<double> invert( std::vector<double> value );

enum VSteal {
	NOSTEAL = 0,
	STEAL = 1
};

/* A smart VipsObject pointer class ... use g_object_ref()/_unref() for
 * lifetime management.
 */
class VObject
{
private:
	// can be NULL, see eg. VObject()
	VipsObject *vobject; 

public:
	VObject( VipsObject *new_vobject, VSteal steal = STEAL ) : 
		vobject( new_vobject )
	{
		// we allow NULL init, eg. "VImage a;"
		g_assert( !new_vobject ||
			VIPS_IS_OBJECT( new_vobject ) ); 

#ifdef VIPS_DEBUG_VERBOSE
		printf( "VObject constructor, obj = %p, steal = %d\n",
			new_vobject, steal ); 
		if( new_vobject ) { 
			printf( "   obj " ); 
			vips_object_print_name( VIPS_OBJECT( new_vobject ) );
			printf( "\n" ); 
		}
#endif /*VIPS_DEBUG_VERBOSE*/

		if( !steal && vobject ) {
#ifdef VIPS_DEBUG_VERBOSE
			printf( "   reffing object\n" ); 
#endif /*VIPS_DEBUG_VERBOSE*/
			g_object_ref( vobject ); 
		}
	}

	VObject() :
		vobject( 0 )
	{
	}

	// copy constructor 
	VObject( const VObject &a ) : 
		vobject( a.vobject )
	{
		g_assert( !vobject ||
			VIPS_IS_OBJECT( vobject ) );

#ifdef VIPS_DEBUG_VERBOSE
		printf( "VObject copy constructor, obj = %p\n", 
			vobject ); 
		printf( "   reffing object\n" ); 
#endif /*VIPS_DEBUG_VERBOSE*/
		if( vobject )
			g_object_ref( vobject );
	}

	// assignment ... we must delete the old ref
	VObject &operator=( const VObject &a )
	{
#ifdef VIPS_DEBUG_VERBOSE
		printf( "VObject assignment\n" );  
		printf( "   reffing %p\n", a.vobject ); 
		printf( "   unreffing %p\n", vobject ); 
#endif /*VIPS_DEBUG_VERBOSE*/

		g_assert( !vobject ||
			VIPS_IS_OBJECT( vobject ) ); 
		g_assert( !a.vobject ||
			VIPS_IS_OBJECT( a.vobject ) ); 

		// delete the old ref at the end ... otherwise "a = a;" could
		// unref before reffing again 
		if( a.vobject )
			g_object_ref( a.vobject );
		if( vobject )
			g_object_unref( vobject );
		vobject = a.vobject;

		return( *this ); 
	}

	// this mustn't be virtual: we want this class to only be a pointer,
	// no vtable allowed
	~VObject()
	{
#ifdef VIPS_DEBUG_VERBOSE
		printf( "VObject destructor\n" );  
		printf( "   unreffing %p\n", vobject ); 
#endif /*VIPS_DEBUG_VERBOSE*/

		g_assert( !vobject ||
			VIPS_IS_OBJECT( vobject ) ); 
		
		if( vobject ) 
			g_object_unref( vobject ); 
	}

	VipsObject *get_object() const
	{
		g_assert( !vobject ||
			VIPS_IS_OBJECT( vobject ) ); 

		return( vobject ); 
	}

	bool is_null() const
	{
		return vobject == 0;
	}

};

class VIPS_CPLUSPLUS_API VImage;
class VIPS_CPLUSPLUS_API VInterpolate;
class VIPS_CPLUSPLUS_API VSource;
class VIPS_CPLUSPLUS_API VTarget;
class VIPS_CPLUSPLUS_API VOption;

class VOption
{
private:
	struct Pair {
		const char *name;

		// the thing we pass to and from our caller
		GValue value; 

		// an input or output parameter ... we guess the direction
		// from the arg to set()
		bool input; 

		// the pointer we write output values to
		union {
			bool *vbool;
			int *vint;
			double *vdouble;
			VImage *vimage;
			std::vector<double> *vvector;
			VipsBlob **vblob;
		}; 

		Pair( const char *name ) : 
			name( name ), input( false ), vimage( 0 )
		{
			// argh = {0} won't work wil vanilla C++
			memset( &value, 0, sizeof( GValue ) ); 
		}

		~Pair()
		{
			g_value_unset( &value );
		}
	};

	std::list<Pair *> options;

public:
	VOption()
	{
	}

	virtual ~VOption();

	VOption *set( const char *name, bool value ); 
	VOption *set( const char *name, int value );
	VOption *set( const char *name, double value );
	VOption *set( const char *name, const char *value );
	VOption *set( const char *name, const VImage value );
	VOption *set( const char *name, const VInterpolate value ); 
	VOption *set( const char *name, const VSource value );
	VOption *set( const char *name, const VTarget value );
	VOption *set( const char *name, std::vector<VImage> value );
	VOption *set( const char *name, std::vector<double> value );
	VOption *set( const char *name, std::vector<int> value );
	VOption *set( const char *name, VipsBlob *value ); 

	VOption *set( const char *name, bool *value ); 
	VOption *set( const char *name, int *value );
	VOption *set( const char *name, double *value );
	VOption *set( const char *name, VImage *value );
	VOption *set( const char *name, std::vector<double> *value );
	VOption *set( const char *name, VipsBlob **blob ); 

	void set_operation( VipsOperation *operation );
	void get_operation( VipsOperation *operation );

};

class VImage : VObject
{
public:
	using VObject::is_null;

	VImage( VipsImage *image, VSteal steal = STEAL ) : 
		VObject( (VipsObject *) image, steal )
	{
	}

	// an empty (NULL) VImage, eg. "VImage a;"
	VImage() :
		VObject( 0 )
	{
	}

	VipsImage * 
	get_image() const
	{
		return( (VipsImage *) VObject::get_object() );
	}

	int 
	width() const
	{
		return( vips_image_get_width( get_image() ) ); 
	}

	int 
	height() const
	{
		return( vips_image_get_height( get_image() ) ); 
	}

	int 
	bands() const
	{
		return( vips_image_get_bands( get_image() ) ); 
	}

	VipsBandFormat 
	format() const
	{
		return( vips_image_get_format( get_image() ) ); 
	}

	VipsCoding 
	coding() const
	{
		return( vips_image_get_coding( get_image() ) ); 
	}

	VipsInterpretation 
	interpretation() const
	{
		return( vips_image_get_interpretation( get_image() ) ); 
	}

	VipsInterpretation 
	guess_interpretation() const
	{
		return( vips_image_guess_interpretation( get_image() ) ); 
	}

	double 
	xres() const
	{
		return( vips_image_get_xres( get_image() ) ); 
	}

	double 
	yres() const
	{
		return( vips_image_get_yres( get_image() ) ); 
	}

	int
	xoffset() const
	{
		return( vips_image_get_xoffset( get_image() ) ); 
	}

	int
	yoffset() const
	{
		return( vips_image_get_yoffset( get_image() ) ); 
	}

	bool
	has_alpha() const
	{
		return( vips_image_hasalpha( get_image() ) );
	}

	const char *
	filename() const
	{
		return( vips_image_get_filename( get_image() ) ); 
	}

	const void *
	data() const
	{
		return( vips_image_get_data( get_image() ) ); 
	}

	void 
	set( const char *field, int value )
	{
		vips_image_set_int( this->get_image(), field, value ); 
	}

	void 
	set( const char *field, int *value, int n )
	{
		vips_image_set_array_int( this->get_image(), field, value, n ); 
	}

	void 
	set( const char *field, std::vector<int> value )
	{
		vips_image_set_array_int( this->get_image(), field, &value[0],
			static_cast<int>( value.size() ) );
	}

	void 
	set( const char *field, double value )
	{
		vips_image_set_double( this->get_image(), field, value ); 
	}

	void 
	set( const char *field, const char *value )
	{
		vips_image_set_string( this->get_image(), field, value ); 
	}

	void 
	set( const char *field, 
		VipsCallbackFn free_fn, void *data, size_t length )
	{
		vips_image_set_blob( this->get_image(), field, 
			free_fn, data, length ); 
	}

	GType 
	get_typeof( const char *field ) const
	{
		return( vips_image_get_typeof( this->get_image(), field ) ); 
	}

	int 
	get_int( const char *field ) const
	{
		int value;

		if( vips_image_get_int( this->get_image(), field, &value ) )
			throw( VError() ); 

		return( value ); 
	}

	void
	get_array_int( const char *field, int **out, int *n ) const
	{
		if( vips_image_get_array_int( this->get_image(), field, out, n ) )
			throw( VError() ); 
	}

	std::vector<int> 
	get_array_int( const char *field ) const
	{
		int length;
		int *array;

		if( vips_image_get_array_int( this->get_image(), field, &array, &length ) )
			throw( VError() ); 

		std::vector<int> vector( array, array + length );

		return( vector );
	}

	double 
	get_double( const char *field ) const
	{
		double value;

		if( vips_image_get_double( this->get_image(), field, &value ) )
			throw( VError() ); 

		return( value ); 
	}

	const char *
	get_string( const char *field ) const
	{
		const char *value; 

		if( vips_image_get_string( this->get_image(), field, &value ) )
			throw( VError() ); 

		return( value ); 
	}

	const void *
	get_blob( const char *field, size_t *length ) const
	{
		const void *value; 

		if( vips_image_get_blob( this->get_image(), field, 
			&value, length ) )
			throw( VError() ); 

		return( value ); 
	}

	bool
	remove( const char *name ) const
	{
		return( vips_image_remove( get_image(), name ) );
	}

	static VOption *
	option()
	{
		return( new VOption() );
	}

	static void call_option_string( const char *operation_name, 
		const char *option_string, VOption *options = 0 );
	static void call( const char *operation_name, VOption *options = 0 );

	static VImage 
	new_memory()
	{
		return( VImage( vips_image_new_memory() ) ); 
	}

	static VImage 
	new_temp_file( const char *file_format = ".v" )
	{
		VipsImage *image;

		if( !(image = vips_image_new_temp_file( file_format )) )
			throw( VError() ); 

		return( VImage( image ) ); 
	}

	static VImage new_from_file( const char *name, VOption *options = 0 );

	static VImage 
	new_from_memory( void *data, size_t size,
		int width, int height, int bands, VipsBandFormat format )
	{
		VipsImage *image;

		if( !(image = vips_image_new_from_memory( data, size, 
			width, height, bands, format )) )
			throw( VError() ); 

		return( VImage( image ) ); 
	}

	static VImage new_from_buffer( const void *buf, size_t len,
		const char *option_string, VOption *options = 0 );

	static VImage new_from_buffer( const std::string &buf,
		const char *option_string, VOption *options = 0 );

	static VImage new_from_source( VSource source, 
		const char *option_string, VOption *options = 0 );

	static VImage new_matrix( int width, int height );

	static VImage 
	new_matrix( int width, int height, double *array, int size )
	{
		VipsImage *image;

		if( !(image = vips_image_new_matrix_from_array( width, height,
			array, size )) )
			throw( VError() ); 

		return( VImage( image ) ); 
	}

	static VImage new_matrixv( int width, int height, ... );

	VImage 
	new_from_image( std::vector<double> pixel ) const
	{
		VipsImage *image;

		if( !(image = vips_image_new_from_image( this->get_image(), 
			&pixel[0], static_cast<int>( pixel.size() ) )) )
			throw( VError() ); 

		return( VImage( image ) ); 
	}

	VImage 
	new_from_image( double pixel ) const
	{
		return( new_from_image( to_vectorv( 1, pixel ) ) ); 
	}

	VImage 
	copy_memory() const
	{
		VipsImage *image;

		if( !(image = vips_image_copy_memory( this->get_image() )) )
			throw( VError() );

		return( VImage( image ) );
	}

	VImage write( VImage out ) const;

	void write_to_file( const char *name, VOption *options = 0 ) const;

	void write_to_buffer( const char *suffix, void **buf, size_t *size, 
		VOption *options = 0 ) const;

	void write_to_target( const char *suffix, VTarget target, 
		VOption *options = 0 ) const;

	void *
	write_to_memory( size_t *size ) const
	{
		void *result;

		if( !(result = vips_image_write_to_memory( this->get_image(), 
			size )) )
			throw( VError() ); 

		return( result ); 
	}

	// a few useful things

	VImage
	linear( double a, double b, VOption *options = 0 ) const
	{
		return( this->linear( to_vector( a ), to_vector( b ), 
			options ) ); 
	}

	VImage
	linear( std::vector<double> a, double b, VOption *options = 0 ) const
	{
		return( this->linear( a, to_vector( b ), options ) ); 
	}

	VImage
	linear( double a, std::vector<double> b, VOption *options = 0 ) const
	{
		return( this->linear( to_vector( a ), b, options ) ); 
	}

	std::vector<VImage> bandsplit( VOption *options = 0 ) const;

	VImage bandjoin( VImage other, VOption *options = 0 ) const;

	VImage
	bandjoin( double other, VOption *options = 0 ) const
	{
		return( bandjoin( to_vector( other ), options ) ); 
	}

	VImage
	bandjoin( std::vector<double> other, VOption *options = 0 ) const
	{
		return( bandjoin_const( other, options ) ); 
	}

	VImage composite( VImage other, VipsBlendMode mode, 
		VOption *options = 0 ) const;

	std::complex<double> minpos( VOption *options = 0 ) const;

	std::complex<double> maxpos( VOption *options = 0 ) const;

	VImage 
	fliphor( VOption *options = 0 ) const
	{
		return( flip( VIPS_DIRECTION_HORIZONTAL, options ) ); 
	}

	VImage 
	flipver( VOption *options = 0 ) const
	{
		return( flip( VIPS_DIRECTION_VERTICAL, options ) ); 
	}

	VImage 
	rot90( VOption *options = 0 ) const
	{
		return( rot( VIPS_ANGLE_D90, options ) ); 
	}

	VImage 
	rot180( VOption *options = 0 ) const
	{
		return( rot( VIPS_ANGLE_D180, options ) ); 
	}

	VImage 
	rot270( VOption *options = 0 ) const
	{
		return( rot( VIPS_ANGLE_D270, options ) ); 
	}

	VImage 
	dilate( VImage mask, VOption *options = 0 ) const
	{
		return( morph( mask, VIPS_OPERATION_MORPHOLOGY_DILATE, 
			options ) ); 
	}

	VImage 
	erode( VImage mask, VOption *options = 0 ) const
	{
		return( morph( mask, VIPS_OPERATION_MORPHOLOGY_ERODE, 
			options ) ); 
	}

	VImage 
	median( int size = 3, VOption *options = 0 ) const
	{
		return( rank( size, size, (size * size) / 2, options ) ); 
	}

	VImage 
	floor( VOption *options = 0 ) const
	{
		return( round( VIPS_OPERATION_ROUND_FLOOR, options ) ); 
	}

	VImage 
	ceil( VOption *options = 0 ) const
	{
		return( round( VIPS_OPERATION_ROUND_CEIL, options ) ); 
	}

	VImage 
	rint( VOption *options = 0 ) const
	{
		return( round( VIPS_OPERATION_ROUND_RINT, options ) ); 
	}

	VImage 
	bandand( VOption *options = 0 ) const
	{
		return( bandbool( VIPS_OPERATION_BOOLEAN_AND, options ) ); 
	}

	VImage 
	bandor( VOption *options = 0 ) const
	{
		return( bandbool( VIPS_OPERATION_BOOLEAN_OR, options ) ); 
	}

	VImage 
	bandeor( VOption *options = 0 ) const
	{
		return( bandbool( VIPS_OPERATION_BOOLEAN_EOR, options ) ); 
	}

	VImage 
	real( VOption *options = 0 ) const
	{
		return( complexget( VIPS_OPERATION_COMPLEXGET_REAL, options ) );
	}

	VImage 
	imag( VOption *options = 0 ) const
	{
		return( complexget( VIPS_OPERATION_COMPLEXGET_IMAG, options ) );
	}

	VImage 
	polar( VOption *options = 0 ) const
	{
		return( complex( VIPS_OPERATION_COMPLEX_POLAR, options ) );
	}

	VImage 
	rect( VOption *options = 0 ) const
	{
		return( complex( VIPS_OPERATION_COMPLEX_RECT, options ) );
	}

	VImage 
	conj( VOption *options = 0 ) const
	{
		return( complex( VIPS_OPERATION_COMPLEX_CONJ, options ) );
	}

	VImage 
	sin( VOption *options = 0 ) const
	{
		return( math( VIPS_OPERATION_MATH_SIN, options ) );
	}

	VImage 
	cos( VOption *options = 0 ) const
	{
		return( math( VIPS_OPERATION_MATH_COS, options ) );
	}

	VImage 
	tan( VOption *options = 0 ) const
	{
		return( math( VIPS_OPERATION_MATH_TAN, options ) );
	}

	VImage 
	asin( VOption *options = 0 ) const
	{
		return( math( VIPS_OPERATION_MATH_ASIN, options ) );
	}

	VImage 
	acos( VOption *options = 0 ) const
	{
		return( math( VIPS_OPERATION_MATH_ACOS, options ) );
	}

	VImage 
	atan( VOption *options = 0 ) const
	{
		return( math( VIPS_OPERATION_MATH_ATAN, options ) );
	}

	VImage 
	log( VOption *options = 0 ) const
	{
		return( math( VIPS_OPERATION_MATH_LOG, options ) );
	}

	VImage 
	log10( VOption *options = 0 ) const
	{
		return( math( VIPS_OPERATION_MATH_LOG10, options ) );
	}

	VImage 
	exp( VOption *options = 0 ) const
	{
		return( math( VIPS_OPERATION_MATH_EXP, options ) );
	}

	VImage 
	exp10( VOption *options = 0 ) const
	{
		return( math( VIPS_OPERATION_MATH_EXP10, options ) );
	}

	VImage 
	pow( VImage other, VOption *options = 0 ) const
	{
		return( math2( other, VIPS_OPERATION_MATH2_POW, options ) );
	}

	VImage 
	pow( double other, VOption *options = 0 ) const
	{
		return( math2_const( VIPS_OPERATION_MATH2_POW, 
			to_vector( other ), options ) );
	}

	VImage 
	pow( std::vector<double> other, VOption *options = 0 ) const
	{
		return( math2_const( VIPS_OPERATION_MATH2_POW, 
			other, options ) );
	}

	VImage 
	wop( VImage other, VOption *options = 0 ) const
	{
		return( math2( other, VIPS_OPERATION_MATH2_WOP, options ) );
	}

	VImage 
	wop( double other, VOption *options = 0 ) const
	{
		return( math2_const( VIPS_OPERATION_MATH2_WOP, 
			to_vector( other ), options ) );
	}

	VImage 
	wop( std::vector<double> other, VOption *options = 0 ) const
	{
		return( math2_const( VIPS_OPERATION_MATH2_WOP, 
			other, options ) );
	}

	VImage 
	ifthenelse( std::vector<double> th, VImage el, 
		VOption *options = 0 ) const
	{
		return( ifthenelse( el.new_from_image( th ), el, options ) ); 
	}

	VImage 
	ifthenelse( VImage th, std::vector<double> el, 
		VOption *options = 0 ) const
	{
		return( ifthenelse( th, th.new_from_image( el ), options ) ); 
	}

	VImage 
	ifthenelse( std::vector<double> th, std::vector<double> el, 
		VOption *options = 0 ) const
	{
		return( ifthenelse( new_from_image( th ), new_from_image( el ),
			options ) ); 
	}

	VImage 
	ifthenelse( double th, VImage el, VOption *options = 0 ) const
	{
		return( ifthenelse( to_vector( th ), el, options ) ); 
	}

	VImage 
	ifthenelse( VImage th, double el, VOption *options = 0 ) const
	{
		return( ifthenelse( th, to_vector( el ), options ) ); 
	}

	VImage 
	ifthenelse( double th, double el, VOption *options = 0 ) const
	{
		return( ifthenelse( to_vector( th ), to_vector( el ), 
			options ) );
	}

	// Operator overloads

	VImage operator[]( int index ) const;

	std::vector<double> operator()( int x, int y ) const;

	friend VIPS_CPLUSPLUS_API VImage 
		operator+( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator+( const double a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator+( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator+( const std::vector<double> a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator+( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage & 
		operator+=( VImage &a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator+=( VImage &a, const double b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator+=( VImage &a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage 
		operator-( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator-( const double a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator-( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator-( const std::vector<double> a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator-( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage & 
		operator-=( VImage &a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator-=( VImage &a, const double b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator-=( VImage &a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage 
		operator-( const VImage a );

	friend VIPS_CPLUSPLUS_API VImage 
		operator*( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator*( const double a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator*( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator*( const std::vector<double> a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator*( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage & 
		operator*=( VImage &a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator*=( VImage &a, const double b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator*=( VImage &a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage 
		operator/( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator/( const double a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator/( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator/( const std::vector<double> a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator/( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage & 
		operator/=( VImage &a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator/=( VImage &a, const double b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator/=( VImage &a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage 
		operator%( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator%( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator%( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage & 
		operator%=( VImage &a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator%=( VImage &a, const double b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator%=( VImage &a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage 
		operator<( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator<( const double a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator<( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator<( const std::vector<double> a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator<( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage 
		operator<=( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator<=( const double a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator<=( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator<=( const std::vector<double> a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator<=( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage 
		operator>( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator>( const double a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator>( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator>( const std::vector<double> a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator>( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage 
		operator>=( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator>=( const double a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator>=( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator>=( const std::vector<double> a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator>=( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage 
		operator==( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator==( const double a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator==( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator==( const std::vector<double> a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator==( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage 
		operator!=( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator!=( const double a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator!=( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator!=( const std::vector<double> a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator!=( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage 
		operator&( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator&( const double a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator&( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator&( const std::vector<double> a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator&( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage & 
		operator&=( VImage &a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator&=( VImage &a, const double b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator&=( VImage &a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage 
		operator|( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator|( const double a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator|( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator|( const std::vector<double> a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator|( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage & 
		operator|=( VImage &a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator|=( VImage &a, const double b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator|=( VImage &a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage 
		operator^( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator^( const double a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator^( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator^( const std::vector<double> a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator^( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage & 
		operator^=( VImage &a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator^=( VImage &a, const double b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator^=( VImage &a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage 
		operator<<( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator<<( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator<<( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage & 
		operator<<=( VImage &a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator<<=( VImage &a, const double b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator<<=( VImage &a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage 
		operator>>( const VImage a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator>>( const VImage a, const double b );
	friend VIPS_CPLUSPLUS_API VImage 
		operator>>( const VImage a, const std::vector<double> b );

	friend VIPS_CPLUSPLUS_API VImage & 
		operator>>=( VImage &a, const VImage b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator>>=( VImage &a, const double b );
	friend VIPS_CPLUSPLUS_API VImage & 
		operator>>=( VImage &a, const std::vector<double> b );

	/* Automatically generated members.
	 *
	 * Rebuild with:
	 *
	 * 	make vips-operators.h
	 *
	 * Then delete from here to the end of the class and paste in
	 * vips-operators.h. We could just #include vips-operators.h, but 
	 * that confuses doxygen.
	 */

// headers for vips operations
// Mon 17 Aug 10:55:59 BST 2020
// this file is generated automatically, do not edit!

/**
 * Transform lch to cmc.
 * @param options Set of options.
 * @return Output image.
 */
VImage CMC2LCh( VOption *options = 0 ) const;

/**
 * Transform cmyk to xyz.
 * @param options Set of options.
 * @return Output image.
 */
VImage CMYK2XYZ( VOption *options = 0 ) const;

/**
 * Transform hsv to srgb.
 * @param options Set of options.
 * @return Output image.
 */
VImage HSV2sRGB( VOption *options = 0 ) const;

/**
 * Transform lch to cmc.
 * @param options Set of options.
 * @return Output image.
 */
VImage LCh2CMC( VOption *options = 0 ) const;

/**
 * Transform lch to lab.
 * @param options Set of options.
 * @return Output image.
 */
VImage LCh2Lab( VOption *options = 0 ) const;

/**
 * Transform lab to lch.
 * @param options Set of options.
 * @return Output image.
 */
VImage Lab2LCh( VOption *options = 0 ) const;

/**
 * Transform float lab to labq coding.
 * @param options Set of options.
 * @return Output image.
 */
VImage Lab2LabQ( VOption *options = 0 ) const;

/**
 * Transform float lab to signed short.
 * @param options Set of options.
 * @return Output image.
 */
VImage Lab2LabS( VOption *options = 0 ) const;

/**
 * Transform cielab to xyz.
 * @param options Set of options.
 * @return Output image.
 */
VImage Lab2XYZ( VOption *options = 0 ) const;

/**
 * Unpack a labq image to float lab.
 * @param options Set of options.
 * @return Output image.
 */
VImage LabQ2Lab( VOption *options = 0 ) const;

/**
 * Unpack a labq image to short lab.
 * @param options Set of options.
 * @return Output image.
 */
VImage LabQ2LabS( VOption *options = 0 ) const;

/**
 * Convert a labq image to srgb.
 * @param options Set of options.
 * @return Output image.
 */
VImage LabQ2sRGB( VOption *options = 0 ) const;

/**
 * Transform signed short lab to float.
 * @param options Set of options.
 * @return Output image.
 */
VImage LabS2Lab( VOption *options = 0 ) const;

/**
 * Transform short lab to labq coding.
 * @param options Set of options.
 * @return Output image.
 */
VImage LabS2LabQ( VOption *options = 0 ) const;

/**
 * Transform xyz to cmyk.
 * @param options Set of options.
 * @return Output image.
 */
VImage XYZ2CMYK( VOption *options = 0 ) const;

/**
 * Transform xyz to lab.
 * @param options Set of options.
 * @return Output image.
 */
VImage XYZ2Lab( VOption *options = 0 ) const;

/**
 * Transform xyz to yxy.
 * @param options Set of options.
 * @return Output image.
 */
VImage XYZ2Yxy( VOption *options = 0 ) const;

/**
 * Transform xyz to scrgb.
 * @param options Set of options.
 * @return Output image.
 */
VImage XYZ2scRGB( VOption *options = 0 ) const;

/**
 * Transform yxy to xyz.
 * @param options Set of options.
 * @return Output image.
 */
VImage Yxy2XYZ( VOption *options = 0 ) const;

/**
 * Absolute value of an image.
 * @param options Set of options.
 * @return Output image.
 */
VImage abs( VOption *options = 0 ) const;

/**
 * Add two images.
 * @param right Right-hand image argument.
 * @param options Set of options.
 * @return Output image.
 */
VImage add( VImage right, VOption *options = 0 ) const;

/**
 * Affine transform of an image.
 * @param matrix Transformation matrix.
 * @param options Set of options.
 * @return Output image.
 */
VImage affine( std::vector<double> matrix, VOption *options = 0 ) const;

/**
 * Load an analyze6 image.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage analyzeload( const char *filename, VOption *options = 0 );

/**
 * Join an array of images.
 * @param in Array of input images.
 * @param options Set of options.
 * @return Output image.
 */
static VImage arrayjoin( std::vector<VImage> in, VOption *options = 0 );

/**
 * Autorotate image by exif tag.
 * @param options Set of options.
 * @return Output image.
 */
VImage autorot( VOption *options = 0 ) const;

/**
 * Find image average.
 * @param options Set of options.
 * @return Output value.
 */
double avg( VOption *options = 0 ) const;

/**
 * Boolean operation across image bands.
 * @param boolean boolean to perform.
 * @param options Set of options.
 * @return Output image.
 */
VImage bandbool( VipsOperationBoolean boolean, VOption *options = 0 ) const;

/**
 * Fold up x axis into bands.
 * @param options Set of options.
 * @return Output image.
 */
VImage bandfold( VOption *options = 0 ) const;

/**
 * Bandwise join a set of images.
 * @param in Array of input images.
 * @param options Set of options.
 * @return Output image.
 */
static VImage bandjoin( std::vector<VImage> in, VOption *options = 0 );

/**
 * Append a constant band to an image.
 * @param c Array of constants to add.
 * @param options Set of options.
 * @return Output image.
 */
VImage bandjoin_const( std::vector<double> c, VOption *options = 0 ) const;

/**
 * Band-wise average.
 * @param options Set of options.
 * @return Output image.
 */
VImage bandmean( VOption *options = 0 ) const;

/**
 * Band-wise rank of a set of images.
 * @param in Array of input images.
 * @param options Set of options.
 * @return Output image.
 */
static VImage bandrank( std::vector<VImage> in, VOption *options = 0 );

/**
 * Unfold image bands into x axis.
 * @param options Set of options.
 * @return Output image.
 */
VImage bandunfold( VOption *options = 0 ) const;

/**
 * Make a black image.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param options Set of options.
 * @return Output image.
 */
static VImage black( int width, int height, VOption *options = 0 );

/**
 * Boolean operation on two images.
 * @param right Right-hand image argument.
 * @param boolean boolean to perform.
 * @param options Set of options.
 * @return Output image.
 */
VImage boolean( VImage right, VipsOperationBoolean boolean, VOption *options = 0 ) const;

/**
 * Boolean operations against a constant.
 * @param boolean boolean to perform.
 * @param c Array of constants.
 * @param options Set of options.
 * @return Output image.
 */
VImage boolean_const( VipsOperationBoolean boolean, std::vector<double> c, VOption *options = 0 ) const;

/**
 * Build a look-up table.
 * @param options Set of options.
 * @return Output image.
 */
VImage buildlut( VOption *options = 0 ) const;

/**
 * Byteswap an image.
 * @param options Set of options.
 * @return Output image.
 */
VImage byteswap( VOption *options = 0 ) const;

/**
 * Cache an image.
 * @param options Set of options.
 * @return Output image.
 */
VImage cache( VOption *options = 0 ) const;

/**
 * Canny edge detector.
 * @param options Set of options.
 * @return Output image.
 */
VImage canny( VOption *options = 0 ) const;

/**
 * Use pixel values to pick cases from an array of images.
 * @param cases Array of case images.
 * @param options Set of options.
 * @return Output image.
 */
VImage case_image( std::vector<VImage> cases, VOption *options = 0 ) const;

/**
 * Cast an image.
 * @param format Format to cast to.
 * @param options Set of options.
 * @return Output image.
 */
VImage cast( VipsBandFormat format, VOption *options = 0 ) const;

/**
 * Convert to a new colorspace.
 * @param space Destination color space.
 * @param options Set of options.
 * @return Output image.
 */
VImage colourspace( VipsInterpretation space, VOption *options = 0 ) const;

/**
 * Convolve with rotating mask.
 * @param mask Input matrix image.
 * @param options Set of options.
 * @return Output image.
 */
VImage compass( VImage mask, VOption *options = 0 ) const;

/**
 * Perform a complex operation on an image.
 * @param cmplx complex to perform.
 * @param options Set of options.
 * @return Output image.
 */
VImage complex( VipsOperationComplex cmplx, VOption *options = 0 ) const;

/**
 * Complex binary operations on two images.
 * @param right Right-hand image argument.
 * @param cmplx binary complex operation to perform.
 * @param options Set of options.
 * @return Output image.
 */
VImage complex2( VImage right, VipsOperationComplex2 cmplx, VOption *options = 0 ) const;

/**
 * Form a complex image from two real images.
 * @param right Right-hand image argument.
 * @param options Set of options.
 * @return Output image.
 */
VImage complexform( VImage right, VOption *options = 0 ) const;

/**
 * Get a component from a complex image.
 * @param get complex to perform.
 * @param options Set of options.
 * @return Output image.
 */
VImage complexget( VipsOperationComplexget get, VOption *options = 0 ) const;

/**
 * Blend an array of images with an array of blend modes.
 * @param in Array of input images.
 * @param mode Array of VipsBlendMode to join with.
 * @param options Set of options.
 * @return Output image.
 */
static VImage composite( std::vector<VImage> in, std::vector<int> mode, VOption *options = 0 );

/**
 * Blend a pair of images with a blend mode.
 * @param overlay Overlay image.
 * @param mode VipsBlendMode to join with.
 * @param options Set of options.
 * @return Output image.
 */
VImage composite2( VImage overlay, VipsBlendMode mode, VOption *options = 0 ) const;

/**
 * Convolution operation.
 * @param mask Input matrix image.
 * @param options Set of options.
 * @return Output image.
 */
VImage conv( VImage mask, VOption *options = 0 ) const;

/**
 * Approximate integer convolution.
 * @param mask Input matrix image.
 * @param options Set of options.
 * @return Output image.
 */
VImage conva( VImage mask, VOption *options = 0 ) const;

/**
 * Approximate separable integer convolution.
 * @param mask Input matrix image.
 * @param options Set of options.
 * @return Output image.
 */
VImage convasep( VImage mask, VOption *options = 0 ) const;

/**
 * Float convolution operation.
 * @param mask Input matrix image.
 * @param options Set of options.
 * @return Output image.
 */
VImage convf( VImage mask, VOption *options = 0 ) const;

/**
 * Int convolution operation.
 * @param mask Input matrix image.
 * @param options Set of options.
 * @return Output image.
 */
VImage convi( VImage mask, VOption *options = 0 ) const;

/**
 * Seperable convolution operation.
 * @param mask Input matrix image.
 * @param options Set of options.
 * @return Output image.
 */
VImage convsep( VImage mask, VOption *options = 0 ) const;

/**
 * Copy an image.
 * @param options Set of options.
 * @return Output image.
 */
VImage copy( VOption *options = 0 ) const;

/**
 * Count lines in an image.
 * @param direction Countlines left-right or up-down.
 * @param options Set of options.
 * @return Number of lines.
 */
double countlines( VipsDirection direction, VOption *options = 0 ) const;

/**
 * Extract an area from an image.
 * @param left Left edge of extract area.
 * @param top Top edge of extract area.
 * @param width Width of extract area.
 * @param height Height of extract area.
 * @param options Set of options.
 * @return Output image.
 */
VImage crop( int left, int top, int width, int height, VOption *options = 0 ) const;

/**
 * Load csv.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage csvload( const char *filename, VOption *options = 0 );

/**
 * Load csv.
 * @param source Source to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage csvload_source( VSource source, VOption *options = 0 );

/**
 * Save image to csv.
 * @param filename Filename to save to.
 * @param options Set of options.
 */
void csvsave( const char *filename, VOption *options = 0 ) const;

/**
 * Save image to csv.
 * @param target Target to save to.
 * @param options Set of options.
 */
void csvsave_target( VTarget target, VOption *options = 0 ) const;

/**
 * Calculate de00.
 * @param right Right-hand input image.
 * @param options Set of options.
 * @return Output image.
 */
VImage dE00( VImage right, VOption *options = 0 ) const;

/**
 * Calculate de76.
 * @param right Right-hand input image.
 * @param options Set of options.
 * @return Output image.
 */
VImage dE76( VImage right, VOption *options = 0 ) const;

/**
 * Calculate decmc.
 * @param right Right-hand input image.
 * @param options Set of options.
 * @return Output image.
 */
VImage dECMC( VImage right, VOption *options = 0 ) const;

/**
 * Find image standard deviation.
 * @param options Set of options.
 * @return Output value.
 */
double deviate( VOption *options = 0 ) const;

/**
 * Divide two images.
 * @param right Right-hand image argument.
 * @param options Set of options.
 * @return Output image.
 */
VImage divide( VImage right, VOption *options = 0 ) const;

/**
 * Draw a circle on an image.
 * @param ink Color for pixels.
 * @param cx Centre of draw_circle.
 * @param cy Centre of draw_circle.
 * @param radius Radius in pixels.
 * @param options Set of options.
 */
void draw_circle( std::vector<double> ink, int cx, int cy, int radius, VOption *options = 0 ) const;

/**
 * Flood-fill an area.
 * @param ink Color for pixels.
 * @param x DrawFlood start point.
 * @param y DrawFlood start point.
 * @param options Set of options.
 */
void draw_flood( std::vector<double> ink, int x, int y, VOption *options = 0 ) const;

/**
 * Paint an image into another image.
 * @param sub Sub-image to insert into main image.
 * @param x Draw image here.
 * @param y Draw image here.
 * @param options Set of options.
 */
void draw_image( VImage sub, int x, int y, VOption *options = 0 ) const;

/**
 * Draw a line on an image.
 * @param ink Color for pixels.
 * @param x1 Start of draw_line.
 * @param y1 Start of draw_line.
 * @param x2 End of draw_line.
 * @param y2 End of draw_line.
 * @param options Set of options.
 */
void draw_line( std::vector<double> ink, int x1, int y1, int x2, int y2, VOption *options = 0 ) const;

/**
 * Draw a mask on an image.
 * @param ink Color for pixels.
 * @param mask Mask of pixels to draw.
 * @param x Draw mask here.
 * @param y Draw mask here.
 * @param options Set of options.
 */
void draw_mask( std::vector<double> ink, VImage mask, int x, int y, VOption *options = 0 ) const;

/**
 * Paint a rectangle on an image.
 * @param ink Color for pixels.
 * @param left Rect to fill.
 * @param top Rect to fill.
 * @param width Rect to fill.
 * @param height Rect to fill.
 * @param options Set of options.
 */
void draw_rect( std::vector<double> ink, int left, int top, int width, int height, VOption *options = 0 ) const;

/**
 * Blur a rectangle on an image.
 * @param left Rect to fill.
 * @param top Rect to fill.
 * @param width Rect to fill.
 * @param height Rect to fill.
 * @param options Set of options.
 */
void draw_smudge( int left, int top, int width, int height, VOption *options = 0 ) const;

/**
 * Save image to deepzoom file.
 * @param filename Filename to save to.
 * @param options Set of options.
 */
void dzsave( const char *filename, VOption *options = 0 ) const;

/**
 * Save image to dz buffer.
 * @param options Set of options.
 * @return Buffer to save to.
 */
VipsBlob *dzsave_buffer( VOption *options = 0 ) const;

/**
 * Embed an image in a larger image.
 * @param x Left edge of input in output.
 * @param y Top edge of input in output.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param options Set of options.
 * @return Output image.
 */
VImage embed( int x, int y, int width, int height, VOption *options = 0 ) const;

/**
 * Extract an area from an image.
 * @param left Left edge of extract area.
 * @param top Top edge of extract area.
 * @param width Width of extract area.
 * @param height Height of extract area.
 * @param options Set of options.
 * @return Output image.
 */
VImage extract_area( int left, int top, int width, int height, VOption *options = 0 ) const;

/**
 * Extract band from an image.
 * @param band Band to extract.
 * @param options Set of options.
 * @return Output image.
 */
VImage extract_band( int band, VOption *options = 0 ) const;

/**
 * Make an image showing the eye's spatial response.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param options Set of options.
 * @return Output image.
 */
static VImage eye( int width, int height, VOption *options = 0 );

/**
 * False-color an image.
 * @param options Set of options.
 * @return Output image.
 */
VImage falsecolour( VOption *options = 0 ) const;

/**
 * Fast correlation.
 * @param ref Input reference image.
 * @param options Set of options.
 * @return Output image.
 */
VImage fastcor( VImage ref, VOption *options = 0 ) const;

/**
 * Fill image zeros with nearest non-zero pixel.
 * @param options Set of options.
 * @return Value of nearest non-zero pixel.
 */
VImage fill_nearest( VOption *options = 0 ) const;

/**
 * Search an image for non-edge areas.
 * @param top Top edge of extract area.
 * @param width Width of extract area.
 * @param height Height of extract area.
 * @param options Set of options.
 * @return Left edge of image.
 */
int find_trim( int *top, int *width, int *height, VOption *options = 0 ) const;

/**
 * Load a fits image.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage fitsload( const char *filename, VOption *options = 0 );

/**
 * Save image to fits file.
 * @param filename Filename to save to.
 * @param options Set of options.
 */
void fitssave( const char *filename, VOption *options = 0 ) const;

/**
 * Flatten alpha out of an image.
 * @param options Set of options.
 * @return Output image.
 */
VImage flatten( VOption *options = 0 ) const;

/**
 * Flip an image.
 * @param direction Direction to flip image.
 * @param options Set of options.
 * @return Output image.
 */
VImage flip( VipsDirection direction, VOption *options = 0 ) const;

/**
 * Transform float rgb to radiance coding.
 * @param options Set of options.
 * @return Output image.
 */
VImage float2rad( VOption *options = 0 ) const;

/**
 * Make a fractal surface.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param fractal_dimension Fractal dimension.
 * @param options Set of options.
 * @return Output image.
 */
static VImage fractsurf( int width, int height, double fractal_dimension, VOption *options = 0 );

/**
 * Frequency-domain filtering.
 * @param mask Input mask image.
 * @param options Set of options.
 * @return Output image.
 */
VImage freqmult( VImage mask, VOption *options = 0 ) const;

/**
 * Forward fft.
 * @param options Set of options.
 * @return Output image.
 */
VImage fwfft( VOption *options = 0 ) const;

/**
 * Gamma an image.
 * @param options Set of options.
 * @return Output image.
 */
VImage gamma( VOption *options = 0 ) const;

/**
 * Gaussian blur.
 * @param sigma Sigma of Gaussian.
 * @param options Set of options.
 * @return Output image.
 */
VImage gaussblur( double sigma, VOption *options = 0 ) const;

/**
 * Make a gaussian image.
 * @param sigma Sigma of Gaussian.
 * @param min_ampl Minimum amplitude of Gaussian.
 * @param options Set of options.
 * @return Output image.
 */
static VImage gaussmat( double sigma, double min_ampl, VOption *options = 0 );

/**
 * Make a gaussnoise image.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param options Set of options.
 * @return Output image.
 */
static VImage gaussnoise( int width, int height, VOption *options = 0 );

/**
 * Read a point from an image.
 * @param x Point to read.
 * @param y Point to read.
 * @param options Set of options.
 * @return Array of output values.
 */
std::vector<double> getpoint( int x, int y, VOption *options = 0 ) const;

/**
 * Load gif with giflib.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage gifload( const char *filename, VOption *options = 0 );

/**
 * Load gif with giflib.
 * @param buffer Buffer to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage gifload_buffer( VipsBlob *buffer, VOption *options = 0 );

/**
 * Load gif with giflib.
 * @param source Source to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage gifload_source( VSource source, VOption *options = 0 );

/**
 * Global balance an image mosaic.
 * @param options Set of options.
 * @return Output image.
 */
VImage globalbalance( VOption *options = 0 ) const;

/**
 * Place an image within a larger image with a certain gravity.
 * @param direction direction to place image within width/height.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param options Set of options.
 * @return Output image.
 */
VImage gravity( VipsCompassDirection direction, int width, int height, VOption *options = 0 ) const;

/**
 * Make a grey ramp image.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param options Set of options.
 * @return Output image.
 */
static VImage grey( int width, int height, VOption *options = 0 );

/**
 * Grid an image.
 * @param tile_height chop into tiles this high.
 * @param across number of tiles across.
 * @param down number of tiles down.
 * @param options Set of options.
 * @return Output image.
 */
VImage grid( int tile_height, int across, int down, VOption *options = 0 ) const;

/**
 * Load a heif image.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage heifload( const char *filename, VOption *options = 0 );

/**
 * Load a heif image.
 * @param buffer Buffer to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage heifload_buffer( VipsBlob *buffer, VOption *options = 0 );

/**
 * Load a heif image.
 * @param source Source to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage heifload_source( VSource source, VOption *options = 0 );

/**
 * Save image in heif format.
 * @param filename Filename to load from.
 * @param options Set of options.
 */
void heifsave( const char *filename, VOption *options = 0 ) const;

/**
 * Save image in heif format.
 * @param options Set of options.
 * @return Buffer to save to.
 */
VipsBlob *heifsave_buffer( VOption *options = 0 ) const;

/**
 * Save image in heif format.
 * @param target Target to save to.
 * @param options Set of options.
 */
void heifsave_target( VTarget target, VOption *options = 0 ) const;

/**
 * Form cumulative histogram.
 * @param options Set of options.
 * @return Output image.
 */
VImage hist_cum( VOption *options = 0 ) const;

/**
 * Estimate image entropy.
 * @param options Set of options.
 * @return Output value.
 */
double hist_entropy( VOption *options = 0 ) const;

/**
 * Histogram equalisation.
 * @param options Set of options.
 * @return Output image.
 */
VImage hist_equal( VOption *options = 0 ) const;

/**
 * Find image histogram.
 * @param options Set of options.
 * @return Output histogram.
 */
VImage hist_find( VOption *options = 0 ) const;

/**
 * Find indexed image histogram.
 * @param index Index image.
 * @param options Set of options.
 * @return Output histogram.
 */
VImage hist_find_indexed( VImage index, VOption *options = 0 ) const;

/**
 * Find n-dimensional image histogram.
 * @param options Set of options.
 * @return Output histogram.
 */
VImage hist_find_ndim( VOption *options = 0 ) const;

/**
 * Test for monotonicity.
 * @param options Set of options.
 * @return true if in is monotonic.
 */
bool hist_ismonotonic( VOption *options = 0 ) const;

/**
 * Local histogram equalisation.
 * @param width Window width in pixels.
 * @param height Window height in pixels.
 * @param options Set of options.
 * @return Output image.
 */
VImage hist_local( int width, int height, VOption *options = 0 ) const;

/**
 * Match two histograms.
 * @param ref Reference histogram.
 * @param options Set of options.
 * @return Output image.
 */
VImage hist_match( VImage ref, VOption *options = 0 ) const;

/**
 * Normalise histogram.
 * @param options Set of options.
 * @return Output image.
 */
VImage hist_norm( VOption *options = 0 ) const;

/**
 * Plot histogram.
 * @param options Set of options.
 * @return Output image.
 */
VImage hist_plot( VOption *options = 0 ) const;

/**
 * Find hough circle transform.
 * @param options Set of options.
 * @return Output image.
 */
VImage hough_circle( VOption *options = 0 ) const;

/**
 * Find hough line transform.
 * @param options Set of options.
 * @return Output image.
 */
VImage hough_line( VOption *options = 0 ) const;

/**
 * Output to device with icc profile.
 * @param options Set of options.
 * @return Output image.
 */
VImage icc_export( VOption *options = 0 ) const;

/**
 * Import from device with icc profile.
 * @param options Set of options.
 * @return Output image.
 */
VImage icc_import( VOption *options = 0 ) const;

/**
 * Transform between devices with icc profiles.
 * @param output_profile Filename to load output profile from.
 * @param options Set of options.
 * @return Output image.
 */
VImage icc_transform( const char *output_profile, VOption *options = 0 ) const;

/**
 * Make a 1d image where pixel values are indexes.
 * @param options Set of options.
 * @return Output image.
 */
static VImage identity( VOption *options = 0 );

/**
 * Ifthenelse an image.
 * @param in1 Source for TRUE pixels.
 * @param in2 Source for FALSE pixels.
 * @param options Set of options.
 * @return Output image.
 */
VImage ifthenelse( VImage in1, VImage in2, VOption *options = 0 ) const;

/**
 * Insert image @sub into @main at @x, @y.
 * @param sub Sub-image to insert into main image.
 * @param x Left edge of sub in main.
 * @param y Top edge of sub in main.
 * @param options Set of options.
 * @return Output image.
 */
VImage insert( VImage sub, int x, int y, VOption *options = 0 ) const;

/**
 * Invert an image.
 * @param options Set of options.
 * @return Output image.
 */
VImage invert( VOption *options = 0 ) const;

/**
 * Build an inverted look-up table.
 * @param options Set of options.
 * @return Output image.
 */
VImage invertlut( VOption *options = 0 ) const;

/**
 * Inverse fft.
 * @param options Set of options.
 * @return Output image.
 */
VImage invfft( VOption *options = 0 ) const;

/**
 * Join a pair of images.
 * @param in2 Second input image.
 * @param direction Join left-right or up-down.
 * @param options Set of options.
 * @return Output image.
 */
VImage join( VImage in2, VipsDirection direction, VOption *options = 0 ) const;

/**
 * Load jpeg from file.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage jpegload( const char *filename, VOption *options = 0 );

/**
 * Load jpeg from buffer.
 * @param buffer Buffer to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage jpegload_buffer( VipsBlob *buffer, VOption *options = 0 );

/**
 * Load image from jpeg source.
 * @param source Source to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage jpegload_source( VSource source, VOption *options = 0 );

/**
 * Save image to jpeg file.
 * @param filename Filename to save to.
 * @param options Set of options.
 */
void jpegsave( const char *filename, VOption *options = 0 ) const;

/**
 * Save image to jpeg buffer.
 * @param options Set of options.
 * @return Buffer to save to.
 */
VipsBlob *jpegsave_buffer( VOption *options = 0 ) const;

/**
 * Save image to jpeg mime.
 * @param options Set of options.
 */
void jpegsave_mime( VOption *options = 0 ) const;

/**
 * Save image to jpeg target.
 * @param target Target to save to.
 * @param options Set of options.
 */
void jpegsave_target( VTarget target, VOption *options = 0 ) const;

/**
 * Label regions in an image.
 * @param options Set of options.
 * @return Mask of region labels.
 */
VImage labelregions( VOption *options = 0 ) const;

/**
 * Calculate (a * in + b).
 * @param a Multiply by this.
 * @param b Add this.
 * @param options Set of options.
 * @return Output image.
 */
VImage linear( std::vector<double> a, std::vector<double> b, VOption *options = 0 ) const;

/**
 * Cache an image as a set of lines.
 * @param options Set of options.
 * @return Output image.
 */
VImage linecache( VOption *options = 0 ) const;

/**
 * Make a laplacian of gaussian image.
 * @param sigma Radius of Logmatian.
 * @param min_ampl Minimum amplitude of Logmatian.
 * @param options Set of options.
 * @return Output image.
 */
static VImage logmat( double sigma, double min_ampl, VOption *options = 0 );

/**
 * Load file with imagemagick.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage magickload( const char *filename, VOption *options = 0 );

/**
 * Load buffer with imagemagick.
 * @param buffer Buffer to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage magickload_buffer( VipsBlob *buffer, VOption *options = 0 );

/**
 * Save file with imagemagick.
 * @param filename Filename to save to.
 * @param options Set of options.
 */
void magicksave( const char *filename, VOption *options = 0 ) const;

/**
 * Save image to magick buffer.
 * @param options Set of options.
 * @return Buffer to save to.
 */
VipsBlob *magicksave_buffer( VOption *options = 0 ) const;

/**
 * Resample with a map image.
 * @param index Index pixels with this.
 * @param options Set of options.
 * @return Output image.
 */
VImage mapim( VImage index, VOption *options = 0 ) const;

/**
 * Map an image though a lut.
 * @param lut Look-up table image.
 * @param options Set of options.
 * @return Output image.
 */
VImage maplut( VImage lut, VOption *options = 0 ) const;

/**
 * Make a butterworth filter.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param order Filter order.
 * @param frequency_cutoff Frequency cutoff.
 * @param amplitude_cutoff Amplitude cutoff.
 * @param options Set of options.
 * @return Output image.
 */
static VImage mask_butterworth( int width, int height, double order, double frequency_cutoff, double amplitude_cutoff, VOption *options = 0 );

/**
 * Make a butterworth_band filter.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param order Filter order.
 * @param frequency_cutoff_x Frequency cutoff x.
 * @param frequency_cutoff_y Frequency cutoff y.
 * @param radius radius of circle.
 * @param amplitude_cutoff Amplitude cutoff.
 * @param options Set of options.
 * @return Output image.
 */
static VImage mask_butterworth_band( int width, int height, double order, double frequency_cutoff_x, double frequency_cutoff_y, double radius, double amplitude_cutoff, VOption *options = 0 );

/**
 * Make a butterworth ring filter.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param order Filter order.
 * @param frequency_cutoff Frequency cutoff.
 * @param amplitude_cutoff Amplitude cutoff.
 * @param ringwidth Ringwidth.
 * @param options Set of options.
 * @return Output image.
 */
static VImage mask_butterworth_ring( int width, int height, double order, double frequency_cutoff, double amplitude_cutoff, double ringwidth, VOption *options = 0 );

/**
 * Make fractal filter.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param fractal_dimension Fractal dimension.
 * @param options Set of options.
 * @return Output image.
 */
static VImage mask_fractal( int width, int height, double fractal_dimension, VOption *options = 0 );

/**
 * Make a gaussian filter.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param frequency_cutoff Frequency cutoff.
 * @param amplitude_cutoff Amplitude cutoff.
 * @param options Set of options.
 * @return Output image.
 */
static VImage mask_gaussian( int width, int height, double frequency_cutoff, double amplitude_cutoff, VOption *options = 0 );

/**
 * Make a gaussian filter.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param frequency_cutoff_x Frequency cutoff x.
 * @param frequency_cutoff_y Frequency cutoff y.
 * @param radius radius of circle.
 * @param amplitude_cutoff Amplitude cutoff.
 * @param options Set of options.
 * @return Output image.
 */
static VImage mask_gaussian_band( int width, int height, double frequency_cutoff_x, double frequency_cutoff_y, double radius, double amplitude_cutoff, VOption *options = 0 );

/**
 * Make a gaussian ring filter.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param frequency_cutoff Frequency cutoff.
 * @param amplitude_cutoff Amplitude cutoff.
 * @param ringwidth Ringwidth.
 * @param options Set of options.
 * @return Output image.
 */
static VImage mask_gaussian_ring( int width, int height, double frequency_cutoff, double amplitude_cutoff, double ringwidth, VOption *options = 0 );

/**
 * Make an ideal filter.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param frequency_cutoff Frequency cutoff.
 * @param options Set of options.
 * @return Output image.
 */
static VImage mask_ideal( int width, int height, double frequency_cutoff, VOption *options = 0 );

/**
 * Make an ideal band filter.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param frequency_cutoff_x Frequency cutoff x.
 * @param frequency_cutoff_y Frequency cutoff y.
 * @param radius radius of circle.
 * @param options Set of options.
 * @return Output image.
 */
static VImage mask_ideal_band( int width, int height, double frequency_cutoff_x, double frequency_cutoff_y, double radius, VOption *options = 0 );

/**
 * Make an ideal ring filter.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param frequency_cutoff Frequency cutoff.
 * @param ringwidth Ringwidth.
 * @param options Set of options.
 * @return Output image.
 */
static VImage mask_ideal_ring( int width, int height, double frequency_cutoff, double ringwidth, VOption *options = 0 );

/**
 * First-order match of two images.
 * @param sec Secondary image.
 * @param xr1 Position of first reference tie-point.
 * @param yr1 Position of first reference tie-point.
 * @param xs1 Position of first secondary tie-point.
 * @param ys1 Position of first secondary tie-point.
 * @param xr2 Position of second reference tie-point.
 * @param yr2 Position of second reference tie-point.
 * @param xs2 Position of second secondary tie-point.
 * @param ys2 Position of second secondary tie-point.
 * @param options Set of options.
 * @return Output image.
 */
VImage match( VImage sec, int xr1, int yr1, int xs1, int ys1, int xr2, int yr2, int xs2, int ys2, VOption *options = 0 ) const;

/**
 * Apply a math operation to an image.
 * @param math math to perform.
 * @param options Set of options.
 * @return Output image.
 */
VImage math( VipsOperationMath math, VOption *options = 0 ) const;

/**
 * Binary math operations.
 * @param right Right-hand image argument.
 * @param math2 math to perform.
 * @param options Set of options.
 * @return Output image.
 */
VImage math2( VImage right, VipsOperationMath2 math2, VOption *options = 0 ) const;

/**
 * Binary math operations with a constant.
 * @param math2 math to perform.
 * @param c Array of constants.
 * @param options Set of options.
 * @return Output image.
 */
VImage math2_const( VipsOperationMath2 math2, std::vector<double> c, VOption *options = 0 ) const;

/**
 * Load mat from file.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage matload( const char *filename, VOption *options = 0 );

/**
 * Invert an matrix.
 * @param options Set of options.
 * @return Output matrix.
 */
VImage matrixinvert( VOption *options = 0 ) const;

/**
 * Load matrix.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage matrixload( const char *filename, VOption *options = 0 );

/**
 * Load matrix.
 * @param source Source to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage matrixload_source( VSource source, VOption *options = 0 );

/**
 * Print matrix.
 * @param options Set of options.
 */
void matrixprint( VOption *options = 0 ) const;

/**
 * Save image to matrix.
 * @param filename Filename to save to.
 * @param options Set of options.
 */
void matrixsave( const char *filename, VOption *options = 0 ) const;

/**
 * Save image to matrix.
 * @param target Target to save to.
 * @param options Set of options.
 */
void matrixsave_target( VTarget target, VOption *options = 0 ) const;

/**
 * Find image maximum.
 * @param options Set of options.
 * @return Output value.
 */
double max( VOption *options = 0 ) const;

/**
 * Measure a set of patches on a color chart.
 * @param h Number of patches across chart.
 * @param v Number of patches down chart.
 * @param options Set of options.
 * @return Output array of statistics.
 */
VImage measure( int h, int v, VOption *options = 0 ) const;

/**
 * Merge two images.
 * @param sec Secondary image.
 * @param direction Horizontal or vertical merge.
 * @param dx Horizontal displacement from sec to ref.
 * @param dy Vertical displacement from sec to ref.
 * @param options Set of options.
 * @return Output image.
 */
VImage merge( VImage sec, VipsDirection direction, int dx, int dy, VOption *options = 0 ) const;

/**
 * Find image minimum.
 * @param options Set of options.
 * @return Output value.
 */
double min( VOption *options = 0 ) const;

/**
 * Morphology operation.
 * @param mask Input matrix image.
 * @param morph Morphological operation to perform.
 * @param options Set of options.
 * @return Output image.
 */
VImage morph( VImage mask, VipsOperationMorphology morph, VOption *options = 0 ) const;

/**
 * Mosaic two images.
 * @param sec Secondary image.
 * @param direction Horizontal or vertical mosaic.
 * @param xref Position of reference tie-point.
 * @param yref Position of reference tie-point.
 * @param xsec Position of secondary tie-point.
 * @param ysec Position of secondary tie-point.
 * @param options Set of options.
 * @return Output image.
 */
VImage mosaic( VImage sec, VipsDirection direction, int xref, int yref, int xsec, int ysec, VOption *options = 0 ) const;

/**
 * First-order mosaic of two images.
 * @param sec Secondary image.
 * @param direction Horizontal or vertical mosaic.
 * @param xr1 Position of first reference tie-point.
 * @param yr1 Position of first reference tie-point.
 * @param xs1 Position of first secondary tie-point.
 * @param ys1 Position of first secondary tie-point.
 * @param xr2 Position of second reference tie-point.
 * @param yr2 Position of second reference tie-point.
 * @param xs2 Position of second secondary tie-point.
 * @param ys2 Position of second secondary tie-point.
 * @param options Set of options.
 * @return Output image.
 */
VImage mosaic1( VImage sec, VipsDirection direction, int xr1, int yr1, int xs1, int ys1, int xr2, int yr2, int xs2, int ys2, VOption *options = 0 ) const;

/**
 * Pick most-significant byte from an image.
 * @param options Set of options.
 * @return Output image.
 */
VImage msb( VOption *options = 0 ) const;

/**
 * Multiply two images.
 * @param right Right-hand image argument.
 * @param options Set of options.
 * @return Output image.
 */
VImage multiply( VImage right, VOption *options = 0 ) const;

/**
 * Load a nifti image.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage niftiload( const char *filename, VOption *options = 0 );

/**
 * Save image to nifti file.
 * @param filename Filename to save to.
 * @param options Set of options.
 */
void niftisave( const char *filename, VOption *options = 0 ) const;

/**
 * Load an openexr image.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage openexrload( const char *filename, VOption *options = 0 );

/**
 * Load file with openslide.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage openslideload( const char *filename, VOption *options = 0 );

/**
 * Load pdf from file.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage pdfload( const char *filename, VOption *options = 0 );

/**
 * Load pdf from buffer.
 * @param buffer Buffer to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage pdfload_buffer( VipsBlob *buffer, VOption *options = 0 );

/**
 * Load pdf from source.
 * @param source Source to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage pdfload_source( VSource source, VOption *options = 0 );

/**
 * Find threshold for percent of pixels.
 * @param percent Percent of pixels.
 * @param options Set of options.
 * @return Threshold above which lie percent of pixels.
 */
int percent( double percent, VOption *options = 0 ) const;

/**
 * Make a perlin noise image.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param options Set of options.
 * @return Output image.
 */
static VImage perlin( int width, int height, VOption *options = 0 );

/**
 * Calculate phase correlation.
 * @param in2 Second input image.
 * @param options Set of options.
 * @return Output image.
 */
VImage phasecor( VImage in2, VOption *options = 0 ) const;

/**
 * Load png from file.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage pngload( const char *filename, VOption *options = 0 );

/**
 * Load png from buffer.
 * @param buffer Buffer to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage pngload_buffer( VipsBlob *buffer, VOption *options = 0 );

/**
 * Load png from source.
 * @param source Source to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage pngload_source( VSource source, VOption *options = 0 );

/**
 * Save image to png file.
 * @param filename Filename to save to.
 * @param options Set of options.
 */
void pngsave( const char *filename, VOption *options = 0 ) const;

/**
 * Save image to png buffer.
 * @param options Set of options.
 * @return Buffer to save to.
 */
VipsBlob *pngsave_buffer( VOption *options = 0 ) const;

/**
 * Save image to target as png.
 * @param target Target to save to.
 * @param options Set of options.
 */
void pngsave_target( VTarget target, VOption *options = 0 ) const;

/**
 * Load ppm from file.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage ppmload( const char *filename, VOption *options = 0 );

/**
 * Load ppm base class.
 * @param source Source to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage ppmload_source( VSource source, VOption *options = 0 );

/**
 * Save image to ppm file.
 * @param filename Filename to save to.
 * @param options Set of options.
 */
void ppmsave( const char *filename, VOption *options = 0 ) const;

/**
 * Save to ppm.
 * @param target Target to save to.
 * @param options Set of options.
 */
void ppmsave_target( VTarget target, VOption *options = 0 ) const;

/**
 * Premultiply image alpha.
 * @param options Set of options.
 * @return Output image.
 */
VImage premultiply( VOption *options = 0 ) const;

/**
 * Find image profiles.
 * @param rows First non-zero pixel in row.
 * @param options Set of options.
 * @return First non-zero pixel in column.
 */
VImage profile( VImage *rows, VOption *options = 0 ) const;

/**
 * Load named icc profile.
 * @param name Profile name.
 * @param options Set of options.
 * @return Loaded profile.
 */
static VipsBlob *profile_load( const char *name, VOption *options = 0 );

/**
 * Find image projections.
 * @param rows Sums of rows.
 * @param options Set of options.
 * @return Sums of columns.
 */
VImage project( VImage *rows, VOption *options = 0 ) const;

/**
 * Resample an image with a quadratic transform.
 * @param coeff Coefficient matrix.
 * @param options Set of options.
 * @return Output image.
 */
VImage quadratic( VImage coeff, VOption *options = 0 ) const;

/**
 * Unpack radiance coding to float rgb.
 * @param options Set of options.
 * @return Output image.
 */
VImage rad2float( VOption *options = 0 ) const;

/**
 * Load a radiance image from a file.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage radload( const char *filename, VOption *options = 0 );

/**
 * Load rad from buffer.
 * @param buffer Buffer to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage radload_buffer( VipsBlob *buffer, VOption *options = 0 );

/**
 * Load rad from source.
 * @param source Source to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage radload_source( VSource source, VOption *options = 0 );

/**
 * Save image to radiance file.
 * @param filename Filename to save to.
 * @param options Set of options.
 */
void radsave( const char *filename, VOption *options = 0 ) const;

/**
 * Save image to radiance buffer.
 * @param options Set of options.
 * @return Buffer to save to.
 */
VipsBlob *radsave_buffer( VOption *options = 0 ) const;

/**
 * Save image to radiance target.
 * @param target Target to save to.
 * @param options Set of options.
 */
void radsave_target( VTarget target, VOption *options = 0 ) const;

/**
 * Rank filter.
 * @param width Window width in pixels.
 * @param height Window height in pixels.
 * @param index Select pixel at index.
 * @param options Set of options.
 * @return Output image.
 */
VImage rank( int width, int height, int index, VOption *options = 0 ) const;

/**
 * Load raw data from a file.
 * @param filename Filename to load from.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param bands Number of bands in image.
 * @param options Set of options.
 * @return Output image.
 */
static VImage rawload( const char *filename, int width, int height, int bands, VOption *options = 0 );

/**
 * Save image to raw file.
 * @param filename Filename to save to.
 * @param options Set of options.
 */
void rawsave( const char *filename, VOption *options = 0 ) const;

/**
 * Write raw image to file descriptor.
 * @param fd File descriptor to write to.
 * @param options Set of options.
 */
void rawsave_fd( int fd, VOption *options = 0 ) const;

/**
 * Linear recombination with matrix.
 * @param m matrix of coefficients.
 * @param options Set of options.
 * @return Output image.
 */
VImage recomb( VImage m, VOption *options = 0 ) const;

/**
 * Reduce an image.
 * @param hshrink Horizontal shrink factor.
 * @param vshrink Vertical shrink factor.
 * @param options Set of options.
 * @return Output image.
 */
VImage reduce( double hshrink, double vshrink, VOption *options = 0 ) const;

/**
 * Shrink an image horizontally.
 * @param hshrink Horizontal shrink factor.
 * @param options Set of options.
 * @return Output image.
 */
VImage reduceh( double hshrink, VOption *options = 0 ) const;

/**
 * Shrink an image vertically.
 * @param vshrink Vertical shrink factor.
 * @param options Set of options.
 * @return Output image.
 */
VImage reducev( double vshrink, VOption *options = 0 ) const;

/**
 * Relational operation on two images.
 * @param right Right-hand image argument.
 * @param relational relational to perform.
 * @param options Set of options.
 * @return Output image.
 */
VImage relational( VImage right, VipsOperationRelational relational, VOption *options = 0 ) const;

/**
 * Relational operations against a constant.
 * @param relational relational to perform.
 * @param c Array of constants.
 * @param options Set of options.
 * @return Output image.
 */
VImage relational_const( VipsOperationRelational relational, std::vector<double> c, VOption *options = 0 ) const;

/**
 * Remainder after integer division of two images.
 * @param right Right-hand image argument.
 * @param options Set of options.
 * @return Output image.
 */
VImage remainder( VImage right, VOption *options = 0 ) const;

/**
 * Remainder after integer division of an image and a constant.
 * @param c Array of constants.
 * @param options Set of options.
 * @return Output image.
 */
VImage remainder_const( std::vector<double> c, VOption *options = 0 ) const;

/**
 * Replicate an image.
 * @param across Repeat this many times horizontally.
 * @param down Repeat this many times vertically.
 * @param options Set of options.
 * @return Output image.
 */
VImage replicate( int across, int down, VOption *options = 0 ) const;

/**
 * Resize an image.
 * @param scale Scale image by this factor.
 * @param options Set of options.
 * @return Output image.
 */
VImage resize( double scale, VOption *options = 0 ) const;

/**
 * Rotate an image.
 * @param angle Angle to rotate image.
 * @param options Set of options.
 * @return Output image.
 */
VImage rot( VipsAngle angle, VOption *options = 0 ) const;

/**
 * Rotate an image.
 * @param options Set of options.
 * @return Output image.
 */
VImage rot45( VOption *options = 0 ) const;

/**
 * Rotate an image by a number of degrees.
 * @param angle Rotate anticlockwise by this many degrees.
 * @param options Set of options.
 * @return Output image.
 */
VImage rotate( double angle, VOption *options = 0 ) const;

/**
 * Perform a round function on an image.
 * @param round rounding operation to perform.
 * @param options Set of options.
 * @return Output image.
 */
VImage round( VipsOperationRound round, VOption *options = 0 ) const;

/**
 * Transform srgb to hsv.
 * @param options Set of options.
 * @return Output image.
 */
VImage sRGB2HSV( VOption *options = 0 ) const;

/**
 * Convert an srgb image to scrgb.
 * @param options Set of options.
 * @return Output image.
 */
VImage sRGB2scRGB( VOption *options = 0 ) const;

/**
 * Convert scrgb to bw.
 * @param options Set of options.
 * @return Output image.
 */
VImage scRGB2BW( VOption *options = 0 ) const;

/**
 * Transform scrgb to xyz.
 * @param options Set of options.
 * @return Output image.
 */
VImage scRGB2XYZ( VOption *options = 0 ) const;

/**
 * Convert an scrgb image to srgb.
 * @param options Set of options.
 * @return Output image.
 */
VImage scRGB2sRGB( VOption *options = 0 ) const;

/**
 * Scale an image to uchar.
 * @param options Set of options.
 * @return Output image.
 */
VImage scale( VOption *options = 0 ) const;

/**
 * Check sequential access.
 * @param options Set of options.
 * @return Output image.
 */
VImage sequential( VOption *options = 0 ) const;

/**
 * Unsharp masking for print.
 * @param options Set of options.
 * @return Output image.
 */
VImage sharpen( VOption *options = 0 ) const;

/**
 * Shrink an image.
 * @param hshrink Horizontal shrink factor.
 * @param vshrink Vertical shrink factor.
 * @param options Set of options.
 * @return Output image.
 */
VImage shrink( double hshrink, double vshrink, VOption *options = 0 ) const;

/**
 * Shrink an image horizontally.
 * @param hshrink Horizontal shrink factor.
 * @param options Set of options.
 * @return Output image.
 */
VImage shrinkh( int hshrink, VOption *options = 0 ) const;

/**
 * Shrink an image vertically.
 * @param vshrink Vertical shrink factor.
 * @param options Set of options.
 * @return Output image.
 */
VImage shrinkv( int vshrink, VOption *options = 0 ) const;

/**
 * Unit vector of pixel.
 * @param options Set of options.
 * @return Output image.
 */
VImage sign( VOption *options = 0 ) const;

/**
 * Similarity transform of an image.
 * @param options Set of options.
 * @return Output image.
 */
VImage similarity( VOption *options = 0 ) const;

/**
 * Make a 2d sine wave.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param options Set of options.
 * @return Output image.
 */
static VImage sines( int width, int height, VOption *options = 0 );

/**
 * Extract an area from an image.
 * @param width Width of extract area.
 * @param height Height of extract area.
 * @param options Set of options.
 * @return Output image.
 */
VImage smartcrop( int width, int height, VOption *options = 0 ) const;

/**
 * Sobel edge detector.
 * @param options Set of options.
 * @return Output image.
 */
VImage sobel( VOption *options = 0 ) const;

/**
 * Spatial correlation.
 * @param ref Input reference image.
 * @param options Set of options.
 * @return Output image.
 */
VImage spcor( VImage ref, VOption *options = 0 ) const;

/**
 * Make displayable power spectrum.
 * @param options Set of options.
 * @return Output image.
 */
VImage spectrum( VOption *options = 0 ) const;

/**
 * Find many image stats.
 * @param options Set of options.
 * @return Output array of statistics.
 */
VImage stats( VOption *options = 0 ) const;

/**
 * Statistical difference.
 * @param width Window width in pixels.
 * @param height Window height in pixels.
 * @param options Set of options.
 * @return Output image.
 */
VImage stdif( int width, int height, VOption *options = 0 ) const;

/**
 * Subsample an image.
 * @param xfac Horizontal subsample factor.
 * @param yfac Vertical subsample factor.
 * @param options Set of options.
 * @return Output image.
 */
VImage subsample( int xfac, int yfac, VOption *options = 0 ) const;

/**
 * Subtract two images.
 * @param right Right-hand image argument.
 * @param options Set of options.
 * @return Output image.
 */
VImage subtract( VImage right, VOption *options = 0 ) const;

/**
 * Sum an array of images.
 * @param in Array of input images.
 * @param options Set of options.
 * @return Output image.
 */
static VImage sum( std::vector<VImage> in, VOption *options = 0 );

/**
 * Load svg with rsvg.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage svgload( const char *filename, VOption *options = 0 );

/**
 * Load svg with rsvg.
 * @param buffer Buffer to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage svgload_buffer( VipsBlob *buffer, VOption *options = 0 );

/**
 * Load svg from source.
 * @param source Source to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage svgload_source( VSource source, VOption *options = 0 );

/**
 * Find the index of the first non-zero pixel in tests.
 * @param tests Table of images to test.
 * @param options Set of options.
 * @return Output image.
 */
static VImage switch_image( std::vector<VImage> tests, VOption *options = 0 );

/**
 * Run an external command.
 * @param cmd_format Command to run.
 * @param options Set of options.
 */
static void system( const char *cmd_format, VOption *options = 0 );

/**
 * Make a text image.
 * @param text Text to render.
 * @param options Set of options.
 * @return Output image.
 */
static VImage text( const char *text, VOption *options = 0 );

/**
 * Generate thumbnail from file.
 * @param filename Filename to read from.
 * @param width Size to this width.
 * @param options Set of options.
 * @return Output image.
 */
static VImage thumbnail( const char *filename, int width, VOption *options = 0 );

/**
 * Generate thumbnail from buffer.
 * @param buffer Buffer to load from.
 * @param width Size to this width.
 * @param options Set of options.
 * @return Output image.
 */
static VImage thumbnail_buffer( VipsBlob *buffer, int width, VOption *options = 0 );

/**
 * Generate thumbnail from image.
 * @param width Size to this width.
 * @param options Set of options.
 * @return Output image.
 */
VImage thumbnail_image( int width, VOption *options = 0 ) const;

/**
 * Generate thumbnail from source.
 * @param source Source to load from.
 * @param width Size to this width.
 * @param options Set of options.
 * @return Output image.
 */
static VImage thumbnail_source( VSource source, int width, VOption *options = 0 );

/**
 * Load tiff from file.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage tiffload( const char *filename, VOption *options = 0 );

/**
 * Load tiff from buffer.
 * @param buffer Buffer to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage tiffload_buffer( VipsBlob *buffer, VOption *options = 0 );

/**
 * Load tiff from source.
 * @param source Source to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage tiffload_source( VSource source, VOption *options = 0 );

/**
 * Save image to tiff file.
 * @param filename Filename to save to.
 * @param options Set of options.
 */
void tiffsave( const char *filename, VOption *options = 0 ) const;

/**
 * Save image to tiff buffer.
 * @param options Set of options.
 * @return Buffer to save to.
 */
VipsBlob *tiffsave_buffer( VOption *options = 0 ) const;

/**
 * Cache an image as a set of tiles.
 * @param options Set of options.
 * @return Output image.
 */
VImage tilecache( VOption *options = 0 ) const;

/**
 * Build a look-up table.
 * @param options Set of options.
 * @return Output image.
 */
static VImage tonelut( VOption *options = 0 );

/**
 * Transpose3d an image.
 * @param options Set of options.
 * @return Output image.
 */
VImage transpose3d( VOption *options = 0 ) const;

/**
 * Unpremultiply image alpha.
 * @param options Set of options.
 * @return Output image.
 */
VImage unpremultiply( VOption *options = 0 ) const;

/**
 * Load vips from file.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage vipsload( const char *filename, VOption *options = 0 );

/**
 * Save image to vips file.
 * @param filename Filename to save to.
 * @param options Set of options.
 */
void vipssave( const char *filename, VOption *options = 0 ) const;

/**
 * Load webp from file.
 * @param filename Filename to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage webpload( const char *filename, VOption *options = 0 );

/**
 * Load webp from buffer.
 * @param buffer Buffer to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage webpload_buffer( VipsBlob *buffer, VOption *options = 0 );

/**
 * Load webp from source.
 * @param source Source to load from.
 * @param options Set of options.
 * @return Output image.
 */
static VImage webpload_source( VSource source, VOption *options = 0 );

/**
 * Save image to webp file.
 * @param filename Filename to save to.
 * @param options Set of options.
 */
void webpsave( const char *filename, VOption *options = 0 ) const;

/**
 * Save image to webp buffer.
 * @param options Set of options.
 * @return Buffer to save to.
 */
VipsBlob *webpsave_buffer( VOption *options = 0 ) const;

/**
 * Save image to webp target.
 * @param target Target to save to.
 * @param options Set of options.
 */
void webpsave_target( VTarget target, VOption *options = 0 ) const;

/**
 * Make a worley noise image.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param options Set of options.
 * @return Output image.
 */
static VImage worley( int width, int height, VOption *options = 0 );

/**
 * Wrap image origin.
 * @param options Set of options.
 * @return Output image.
 */
VImage wrap( VOption *options = 0 ) const;

/**
 * Make an image where pixel values are coordinates.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param options Set of options.
 * @return Output image.
 */
static VImage xyz( int width, int height, VOption *options = 0 );

/**
 * Make a zone plate.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param options Set of options.
 * @return Output image.
 */
static VImage zone( int width, int height, VOption *options = 0 );

/**
 * Zoom an image.
 * @param xfac Horizontal zoom factor.
 * @param yfac Vertical zoom factor.
 * @param options Set of options.
 * @return Output image.
 */
VImage zoom( int xfac, int yfac, VOption *options = 0 ) const;

};

VIPS_NAMESPACE_END

#endif /*VIPS_VIMAGE_H*/
