#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

#define SERIALIZE_BOOL( ser, x )			( ( x ) = ser.SerializeBoolNonRef( x ) )
#define SERIALIZE_ENUM( ser, x, type, max ) ( ( x ) = (type)ser.SerializeUMaxNonRef( x, max ) )
#define SERIALIZE_CVAR_FLOAT( ser, cvar )	{ float a = cvar.GetFloat(); ser.Serialize( a ); cvar.SetFloat( a ); }
#define SERIALIZE_CVAR_INT( ser, cvar )		{ int a = cvar.GetInteger(); ser.Serialize( a ); cvar.SetInteger( a ); }
#define SERIALIZE_CVAR_BOOL( ser, cvar )	{ bool a = cvar.GetBool(); SERIALIZE_BOOL( ser, a ); cvar.SetBool( a ); }

#define SERIALIZE_MATX( ser, var ) {			\
	int rows = var.GetNumRows();				\
	int cols = var.GetNumColumns();				\
	ser.Serialize( rows );						\
	ser.Serialize( cols );						\
	if ( ser.IsReading() ) {					\
		var.SetSize( rows, cols );				\
	}											\
	for ( int y = 0; y < rows; y++ ) {			\
		for ( int x = 0; x < rows; x++ ) {		\
			ser.Serialize( var[x][y] );			\
		}										\
	}											\
}												\

#define SERIALIZE_VECX( ser, var ) {			\
	int size = var.GetSize();					\
	ser.Serialize( size );						\
	if ( ser.IsReading() ) {					\
		var.SetSize( size );					\
	}											\
	for ( int x = 0; x < size; x++ ) {			\
		ser.Serialize( var[x] );				\
	}											\
}												\

#define SERIALIZE_JOINT( ser, var )	{			\
	uint16 jointIndex = ( var == nullptr_JOINT_INDEX ) ? 65535 : var;	\
	ser.Serialize( jointIndex );					\
	var = ( jointIndex == 65535 ) ? nullptr_JOINT_INDEX : (jointIndex_t)jointIndex; \
}												\

//#define ENABLE_SERIALIZE_CHECKPOINTS
//#define SERIALIZE_SANITYCHECK
//#define SERIALIZE_NO_QUANT

#define SERIALIZE_CHECKPOINT( ser )			\
	ser.SerializeCheckpoint( __FILE__, __LINE__ );

/*
========================
idSerializer
========================
*/
class idSerializer {
public:
	idSerializer( anBitMessage & msg_, bool writing_) : msg( &msg_ ), writing( writing_ )
#ifdef SERIALIZE_SANITYCHECK
	,magic( 0 )
#endif
	{ }

	bool	IsReading() { return !writing; }
	bool	IsWriting() { return writing; }

	// SerializeRange - minSize through maxSize inclusive of all possible values
	void	SerializeRange( int & value, int minSize, int maxSize ) {	// Supports signed types
		SanityCheck();
		if ( writing ) {
			msg->WriteBits( value - minSize, anMath::BitsForInteger( maxSize-minSize ) );
		} else {
			value = minSize + msg->ReadBits( anMath::BitsForInteger( maxSize-minSize ) );
		}
		assert( value >= minSize && value <= maxSize );
	}

	// SerializeUMax - maxSize inclusive, unsigned
	void	SerializeUMax( int & value, int maxSize ) {					// Unsigned only
		SanityCheck();
		if ( writing ) {
			msg->WriteBits( value, anMath::BitsForInteger( maxSize ) );
		} else {
			value = msg->ReadBits( anMath::BitsForInteger( maxSize ) );
		}
		assert( value <= maxSize );
	}

	// SerializeUMaxNonRef - maxSize inclusive, unsigned, no reference
	int	SerializeUMaxNonRef( int value, int maxSize ) {					// Unsigned only
		SanityCheck();
		if ( writing ) {
			msg->WriteBits( value, anMath::BitsForInteger( maxSize ) );
		} else {
			value = msg->ReadBits( anMath::BitsForInteger( maxSize ) );
		}
		assert( value <= maxSize );
		return value;
	}

	//void SerializeBitMsg( anBitMessage & inOutMsg, int numBytes ) { SanityCheck(); if ( writing ) { msg->WriteBitMsg( inOutMsg, numBytes ); } else { msg->ReadBitMsg( inOutMsg, numBytes ); } }

	// this is still needed to compile Rage code
	void	SerializeBytes( void *bytes, int numBytes ) { SanityCheck(); for ( int i = 0; i < numBytes; i++ ) { Serialize( ((uint8 *)bytes )[i] ); } };

	bool	SerializeBoolNonRef( bool value )	{ SanityCheck(); if ( writing ) { msg->WriteBool( value ); }		else { value = msg->ReadBool(); } return value; }		// We return a value so we can support bit fields (can't pass by reference)


#ifdef SERIALIZE_NO_QUANT
	template< int _max_, int _numBits_ >
	void	SerializeQ( anVec3 & value ) { Serialize( value ); }
	template< int _max_, int _numBits_ >
	void	SerializeQ( float & value ) { Serialize( value ); }
	template< int _max_, int _numBits_ >
	void	SerializeUQ( float & value ) { Serialize( value ); }
	void	SerializeQ( anMat3 & axis, int bits = 15 ) { Serialize( axis ); }
#else
	// SerializeQ - Quantizes a float to a variable number of bits (assumes signed, uses simple quantization)
	template< int _max_, int _numBits_ >
	void	SerializeQ( anVec3 & value ) { SanityCheck(); if ( writing ) { msg->WriteQuantizedVector< anVec3, _max_, _numBits_ >( value ); }	else { msg->ReadQuantizedVector< anVec3, _max_, _numBits_ >( value ); } }
	template< int _max_, int _numBits_ >
	void	SerializeQ( float & value ) { SanityCheck(); if ( writing ) { msg->WriteQuantizedFloat< _max_, _numBits_ >( value ); }		else { value = msg->ReadQuantizedFloat< _max_, _numBits_ >(); } }
	template< int _max_, int _numBits_ >
	void	SerializeUQ( float & value ) { SanityCheck(); if ( writing ) { msg->WriteQuantizedUFloat< _max_, _numBits_ >( value ); }	else { value = msg->ReadQuantizedUFloat< _max_, _numBits_ >(); } }
	void	SerializeQ( anMat3 & axis, int bits = 15 );		// Default to 15 bits per component, which has almost unnoticeable quantization
#endif

	void	Serialize( anMat3 & axis);			// Raw 3x3 matrix serialize
	void	SerializeC( anMat3 & axis);			// Uses compressed quaternion

	template<typename _type_>
	void	SerializeListElement( const anList<_type_* > & list, const _type_ *&element );

	void	SerializePacked( int & original);
	void	SerializeSPacked( int & original);

	void	SerializeString( char * s, int bufferSize )	{ SanityCheck(); if ( writing ) { msg->WriteString( s); } else { msg->ReadString( s, bufferSize ); } }
	//void	SerializeString( idAtomicString & s )		{ SanityCheck(); if ( writing ) { msg->WriteString( s); } else { anStr temp; msg->ReadString( temp ); s.Set( temp ); } }
	void	SerializeString( anStr & s )				{ SanityCheck(); if ( writing ) { msg->WriteString( s); } else { msg->ReadString( s ); } }
	//void	SerializeString( anStringId & s )				{ SanityCheck(); if ( writing ) { msg->WriteString( s.GetKey() ); } else { anStr key; msg->ReadString( key ); s.Set( key );} }

	void	SerializeDelta( int32 & value, const int32 & base ) { SanityCheck(); if ( writing ) { msg->WriteDeltaLong( base, value ); } else { value = msg->ReadDeltaLong( base ); } }
	void	SerializeDelta( int16 & value, const int16 & base ) { SanityCheck(); if ( writing ) { msg->WriteDeltaShort( base, value ); } else { value = msg->ReadDeltaShort( base ); } }
	void	SerializeDelta( int8 & value, const int8 & base ) { SanityCheck(); if ( writing ) { msg->WriteDeltaChar( base, value ); } else { value = msg->ReadDeltaChar( base ); } }

	void	SerializeDelta( uint16 & value, const uint16 & base ) { SanityCheck(); if ( writing ) { msg->WriteDeltaUShort( base, value ); } else { value = msg->ReadDeltaUShort( base ); } }
	void	SerializeDelta( uint8 & value, const uint8 & base ) { SanityCheck(); if ( writing ) { msg->WriteDeltaByte( base, value ); } else { value = msg->ReadDeltaByte( base ); } }

	void	SerializeDelta( float & value, const float & base ) { SanityCheck(); if ( writing ) { msg->WriteDeltaFloat( base, value ); } else { value = msg->ReadDeltaFloat( base ); } }

	// arCNet types, no compression
	void	Serialize( int64 & value )		{ SanityCheck(); if ( writing ) { msg->WriteLongLong( value ); }		else { value = msg->ReadLongLong(); } }
	void	Serialize( uint64 & value )		{ SanityCheck(); if ( writing ) { msg->WriteLongLong( value ); }		else { value = msg->ReadLongLong(); } }
	void	Serialize( int32 & value )		{ SanityCheck(); if ( writing ) { msg->WriteLong( value ); }			else { value = msg->ReadLong(); } }
	void	Serialize( uint32 & value )		{ SanityCheck(); if ( writing ) { msg->WriteLong( value ); }			else { value = msg->ReadLong(); } }
	void	Serialize( int16 & value )		{ SanityCheck(); if ( writing ) { msg->WriteShort( value ); }			else { value = msg->ReadShort(); } }
	void	Serialize( uint16 & value )		{ SanityCheck(); if ( writing ) { msg->WriteUShort( value ); }		else { value = msg->ReadUShort(); } }
	void	Serialize( uint8 & value )		{ SanityCheck(); if ( writing ) { msg->WriteByte( value ); }			else { value = msg->ReadByte(); } }
	void	Serialize( int8 & value )		{ SanityCheck(); if ( writing ) { msg->WriteChar( value ); }			else { value = msg->ReadChar(); } }
	void	Serialize( bool & value )		{ SanityCheck(); if ( writing ) { msg->WriteByte(value?1:0 ); }		else { value = msg->ReadByte() != 0; } }
	void	Serialize( float & value )		{ SanityCheck(); if ( writing ) { msg->WriteFloat( value ); }			else { value = msg->ReadFloat(); } }
	void	Serialize( anRandom & value )	{ SanityCheck(); if ( writing ) { msg->WriteLong(value.GetSeed() ); } else { value.SetSeed( msg->ReadLong() ); } }
	void	Serialize( anVec3 & value )		{ SanityCheck(); if ( writing ) { msg->WriteVectorFloat( value ); }	else { msg->ReadVectorFloat( value ); } }
	void	Serialize( anVec2 & value )		{ SanityCheck(); if ( writing ) { msg->WriteVectorFloat( value ); }	else { msg->ReadVectorFloat( value ); } }
	void	Serialize( anVec6 & value )		{ SanityCheck(); if ( writing ) { msg->WriteVectorFloat( value ); }	else { msg->ReadVectorFloat( value ); } }
	void	Serialize( anVec4 & value )		{ SanityCheck(); if ( writing ) { msg->WriteVectorFloat( value ); }	else { msg->ReadVectorFloat( value ); } }

	// serialize an angle, normalized to between 0 to 360 and quantized to 16 bits
	void	SerializeAngle( float & value ) {
				SanityCheck();
				if ( writing ) {
					float nAngle = anMath::AngleNormalize360( value );
					assert( nAngle >= 0.0f ); // should never get a negative angle
					uint16 sAngle = nAngle * ( 65536.0f / 360.0f );
					msg->WriteUShort( sAngle );
				} else {
					uint16 sAngle = msg->ReadUShort();
					value = sAngle * ( 360.0f / 65536.0f );
				}

			}

	//void	Serialize( degrees_t & value )	{
	//			SanityCheck();
	//			float angle = value.Get();
	//			Serialize( angle );
	//			value.Set( angle );
	//		}
	//void	SerializeAngle( degrees_t & value )	{
	//			SanityCheck();
	//			float angle = value.Get();
	//			SerializeAngle( angle );
	//			value.Set( angle );
	//		}
	//void	Serialize( radians_t & value ) {
	//			SanityCheck();
	//			// convert to degrees
	//			degrees_t d( value.Get() * anMath::M_RAD2DEG );
	//			Serialize( d );
	//			if ( !writing ) {
	//				// if reading, get the value we read in degrees and convert back to radians
	//				value.Set( d.Get() * anMath::M_DEG2RAD );
	//			}
	//		}
	//void	SerializeAngle( radians_t & value ) {
	//			SanityCheck();
	//			// convert to degrees
	//			degrees_t d( value.Get() * anMath::M_RAD2DEG );
	//			// serialize as normalized degrees between 0 - 360
	//			SerializeAngle( d );
	//			if ( !writing ) {
	//				// if reading, get the value we read in degrees and convert back to radians
	//				value.Set( d.Get() * anMath::M_DEG2RAD );
	//			}
	//		}
	//
	//void	Serialize( idColor & value ) {
	//	Serialize( value.r );
	//	Serialize( value.g );
	//	Serialize( value.b );
	//	Serialize( value.a );
	//}

	void	SanityCheck() {
#ifdef SERIALIZE_SANITYCHECK
		if ( writing ) {
			msg->WriteUShort( 0xCCCC );
			msg->WriteUShort( magic );
		} else {
			int cccc = msg->ReadUShort();
			int m = msg->ReadUShort();
			assert( cccc == 0xCCCC );
			assert( m == magic );
			// For release builds
			if ( cccc != 0xCCCC ) {
				anLibrary::Error( "idSerializer::SanityCheck - cccc != 0xCCCC" );
			}
			if ( m != magic ) {
				anLibrary::Error( "idSerializer::SanityCheck - m != magic" );
			}
		}
		magic++;
#endif
	}

	void SerializeCheckpoint( const char *file, int line ) {
#ifdef ENABLE_SERIALIZE_CHECKPOINTS
		const uint32 tagValue = 0xABADF00D;
		uint32 tag = tagValue;
		Serialize( tag );
		if ( tag != tagValue ) {
			anLibrary::Error( "SERIALIZE_CHECKPOINT: tag != tagValue (file: %s - line: %i)", file, line );
		}
#endif
	}

	anBitMessage &	GetMsg() { return *msg; }

private:
	bool		writing;
	anBitMessage *	msg;
#ifdef SERIALIZE_SANITYCHECK
	int			magic;
#endif
};

class idSerializerScopedBlock {
public:
	idSerializerScopedBlock( idSerializer &ser_, int maxSizeBytes_ ) {
		ser = &ser_;
		maxSizeBytes = maxSizeBytes_;

		startByte = ser->IsReading() ? ser->GetMsg().GetReadCount() : ser->GetMsg().GetSize();
		startWriteBits = ser->GetMsg().GetWriteBit();
	}

	~idSerializerScopedBlock() {
		// Serialize remaining bits
		while ( ser->GetMsg().GetWriteBit() != startWriteBits ) {
			ser->SerializeBoolNonRef( false );
		}

		// Verify we didn't go over
		int endByte = ser->IsReading() ? ser->GetMsg().GetReadCount() : ser->GetMsg().GetSize();
		int sizeBytes = endByte - startByte;
		if ( !verify( sizeBytes <= maxSizeBytes ) ) {
			anLibrary::Warning( "idSerializerScopedBlock went over maxSize (%d > %d)", sizeBytes, maxSizeBytes );
			return;
		}

		// Serialize remaining bytes
		uint8 b=0;
		while ( sizeBytes < maxSizeBytes ) {
			ser->Serialize( b );
			sizeBytes++;
		}

		int finalSize = ( ( ser->IsReading() ? ser->GetMsg().GetReadCount() : ser->GetMsg().GetSize() ) - startByte );
		verify( maxSizeBytes == finalSize );
	}

private:
	idSerializer * ser;
	int maxSizeBytes;

	int startByte;
	int startWriteBits;
};

/*
========================
idSerializer::SerializeQ
========================
*/
#ifndef SERIALIZE_NO_QUANT
inline void idSerializer::SerializeQ( anMat3 &axis, int bits ) {
	SanityCheck();

	const float scale = ( ( 1 << ( bits - 1 ) ) - 1 );
	if ( IsWriting() ) {
		anQuats quat = axis.ToQuat();

		int maxIndex = 0;
		for ( unsigned int i = 1; i < 4; i++ ) {
			if ( anMath::Fabs( quat[i] ) > anMath::Fabs( quat[maxIndex] ) ) {
				maxIndex = i;
			}
		}

		msg->WriteBits( maxIndex, 2 );

		anVec3 out;

		if ( quat[maxIndex] < 0.0f ) {
			out.x = -quat[( maxIndex + 1 ) & 3];
			out.y = -quat[( maxIndex + 2 ) & 3];
			out.z = -quat[( maxIndex + 3 ) & 3];
		} else {
			out.x = quat[( maxIndex + 1 ) & 3];
			out.y = quat[( maxIndex + 2 ) & 3];
			out.z = quat[( maxIndex + 3 ) & 3];
		}
		msg->WriteBits( anMath::Ftoi( out.x * scale ), -bits );
		msg->WriteBits( anMath::Ftoi( out.y * scale ), -bits );
		msg->WriteBits( anMath::Ftoi( out.z * scale ), -bits );

	} else if ( IsReading() ) {
		anQuats quat;
		anVec3 in;

		int maxIndex = msg->ReadBits(2);

		in.x = ( float )msg->ReadBits( -bits ) / scale;
		in.y = ( float )msg->ReadBits( -bits ) / scale;
		in.z = ( float )msg->ReadBits( -bits ) / scale;

		quat[( maxIndex + 1 ) & 3] = in.x;
		quat[( maxIndex + 2 ) & 3] = in.y;
		quat[( maxIndex + 3 ) & 3] = in.z;

		quat[maxIndex] = anMath::Sqrt( anMath::Fabs( 1.0f - in.x * in.x - in.y * in.y - in.z * in.z ) );

		axis = quat.ToMat3();
	}
}
#endif

/*
========================
idSerializer::Serialize
========================
*/
inline void idSerializer::Serialize( anMat3 & axis ) {
	SanityCheck();
	Serialize( axis[0] );
	Serialize( axis[1] );
	Serialize( axis[2] );
}

/*
========================
idSerializer::SerializeC
========================
*/
inline void idSerializer::SerializeC( anMat3 & axis ) {
	SanityCheck();

	if ( IsWriting() ) {
		anCQuats cquat = axis.ToCQuat();
		Serialize( cquat.x );
		Serialize( cquat.y );
		Serialize( cquat.z );
	} else if ( IsReading() ) {
		anCQuats cquat;

		Serialize( cquat.x );
		Serialize( cquat.y );
		Serialize( cquat.z );

		axis = cquat.ToMat3();
	}
}

/*
========================
idSerializer::SerializeListElement
========================
*/
template<typename _type_>
inline void idSerializer::SerializeListElement( const anList<_type_* > & list, const _type_ *&element ) {
	SanityCheck();

	if ( IsWriting() ) {
		int index = list.FindIndex( const_cast<_type_ *>(element) );
		assert( index >= 0 );
		SerializePacked( index );
	} else if ( IsReading() ) {
		int index = 0;
		SerializePacked( index );
		element = list[index];
	}
}

/*
========================
idSerializer::SerializePacked

Writes out 7 bits at a time, using every 8th bit to signify more bits exist

NOTE - Signed values work with this function, but take up more bytes
Use SerializeSPacked if you anticipate lots of negative values
========================
*/
inline void idSerializer::SerializePacked( int & original) {
	SanityCheck();

	if ( IsWriting() ) {
		uint32 value = original;
		while ( true ) {
			uint8 byte = value & 0x7F;
			value >>= 7;
			byte |= value ? 0x80 : 0;
			msg->WriteByte( byte );		// Emit byte
			if ( value == 0 ) {
				break;
			}
		}
	} else {
		uint8 byte = 0x80;
		uint32 value = 0;
		int32 shift = 0;

		while ( byte & 0x80 && shift < 32 ) {
			byte = msg->ReadByte();
			value |= (byte & 0x7F) << shift;
			shift += 7;
		}

		original = value;
	}
}

/*
========================
idSerializer::SerializeSPacked
Writes out 7 bits at a time, using every 8th bit to signify more bits exist

NOTE - An extra bit of the first byte is used to store the sign
(this function supports negative values, but will use 2 bytes for values greater than 63)
========================
*/
inline void idSerializer::SerializeSPacked( int & value) {
	SanityCheck();

	if ( IsWriting() ) {
		uint32 uvalue = anMath::Abs( value );

		// Write the first byte specifically to handle the sign bit
		uint8 byte = uvalue & 0x3f;
		byte |= value < 0 ? 0x40 : 0;
		uvalue >>= 6;
		byte |= uvalue > 0 ? 0x80 : 0;

		msg->WriteByte( byte );

		while ( uvalue > 0 ) {
			uint8 byte2 = uvalue & 0x7F;
			uvalue >>= 7;
			byte2 |= uvalue ? 0x80 : 0;
			msg->WriteByte( byte2 );		// Emit byte
		}
	} else {
		// Load the first byte specifically to handle the sign bit
		uint8 byte		= msg->ReadByte();
		uint32 uvalue	= byte & 0x3f;
		bool sgn		= ( byte & 0x40 ) ? true : false;
		int32 shift		= 6;

		while ( byte & 0x80 && shift < 32 ) {
			byte = msg->ReadByte();		// Read byte
			uvalue |= (byte & 0x7F) << shift;
			shift += 7;
		}

		value = sgn ? -( ( int )uvalue ) : uvalue;
	}
}

#endif




