#ifndef __TEXTCOMPILER_H__
#define __TEXTCOMPILER_H__

namespace TextCompiler {
	// Write an indirect value
	template <typename type> inline void WriteValue( type const * const ptr, anFile *out, bool byteSwap = false ) {
		if ( out != nullptr ) {
			if ( !byteSwap ) {
				out->Write( ptr, sizeof( type ) );
			} else {
				byte const * const p = (byte *)ptr;

				for ( int i=sizeof(type)-1;i>=0;i-- )
				{
					out->Write(&(p[i]), 1);
				}
			}

		}
	}

	// Write a direct value
	template <typename type> inline void WriteValue( type const val, anFile *out, bool byteSwap = false ) {
		if ( out != nullptr ) {
			if ( !byteSwap) {
				out->Write( &val, sizeof( type ) );
			} else {
				byte const * const p = (byte *)&val;
				for ( int i=sizeof( type ) -1;i>=0;i--) {
					out->Write( &( p[i] ), 1 );
				}
			}
 		}
	}

	template <typename type> inline type ReadValue( anFile *in ) {
		type ret;

		in->Read( &ret, sizeof( type ) );
		return ret;
	}

	// specialization write for anStr's
	template <> inline void WriteValue( anStr const * const ptr, anFile *out, bool byteSwap ) {
		if (out != nullptr ) {			// if less than 32, then we don't need a trailing null
			// this is because 5 bits can be used to represent string length in the token header
			// zero length strings end up needing to be null terminated
			if ( ( ptr->Length() < 32 ) && ( ptr->Length() != 0 ) )
			{
				out->Write( ptr->c_str(), ptr->Length() );
			} else {
				out->Write( ptr->c_str(), ptr->Length()+1 );
			}
		}
	}

	// specialization read for anStr's
	template <> inline anStr ReadValue( anFile *in ) {
		char c;
		anStr str;

		in->Read( &c, 1 );
		while ( c != '\0' ) {
			str.Append( c );
			in->Read(&c, 1);
		}

		str.Append( c );

		return str;
	}

	template <typename type> inline void WriteArray( type const * const ptr, unsigned int count, anFile *out ) {
		WriteValue<unsigned int>( &count, out );
		for ( unsigned int i = 0; i < count; i++ ) {
			WriteValue<type>( &( ptr[i] ), out );
		}
	}

	template <typename type> inline type *ReadArray( anFile *in, unsigned int *count = nullptr ) {
		unsigned int len = ReadValue<unsigned int>( in );
		type *buffer = (type *)malloc( len*sizeof( type ) );
		type *ptr = buffer;
		for (unsigned int i=0;i<len;i++ ) {
			*ptr = ReadValue<type>( in );
			ptr++;
		}
		if (count != nullptr )
			*count = len;
		return buffer;
	}

	template <typename type> inline void WriteIdList( anList<type> const * const ptr, anFile *out ) {
		WriteArray<type>( ptr->Ptr(), ptr->Num(), out );
	}

	template <typename type> inline anList<type> ReadIdList( anFile *in ) {
		anList<type> ret;
		ret.Clear();
		unsigned int count;
		idAutoPtr<type> array( ReadArray<type>( in, &count ) );

		for (unsigned int i=0;i<count;i++ )
			ret.Append( array[i] );

		return ret;
	}
};

#endif
