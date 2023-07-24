#ifndef __CALLBACK_H__
#define __CALLBACK_H__

/*
================================================================================================
This file defines a set of template functors for generating callbacks, specifically
the OnChange handlers in the CVar system.
================================================================================================
*/

class idCallback {
public:
	virtual ~idCallback() {}
	virtual void Call() = 0;
	virtual idCallback * Clone() const = 0;
};

// Callback class that forwards the call to a c-style function
class idCallbackStatic : public idCallback {
public:
	idCallbackStatic( void ( *f )() ) {
		this->f = f;
	}
	void Call() {
		f();
	}
	idCallback * Clone() const {
		return new ( TAG_FUNC_CALLBACK ) idCallbackStatic( f );
	}
private:
	void ( *f )();
};


// Callback class that forwards the call to a member function
template< class T >
class idCallbackBindMem : public idCallback {
public:
	idCallbackBindMem( T * t, void ( T::*f )() ) {
		this->t = t;
		this->f = f;
	}
	void Call() {
		( t->*f )();
	}
	idCallback * Clone() const {
		return new ( TAG_FUNC_CALLBACK ) idCallbackBindMem( t, f );
	}
private:
	T * t;
	void ( T::*f )();
};

/*
================================================
idCallbackBindMemArg1

Callback class that forwards the call to a member function with an additional constant parameter
================================================
*/
template< class T, typename A1 >
class idCallbackBindMemArg1 : public idCallback {
public:
	idCallbackBindMemArg1( T * t_, void ( T::*f_ )( A1 ), A1 a1_ ) :
		t( t_ ),
		f( f_ ),
		a1( a1_ ) {
	}
	void Call() {
		( t->*f )( a1 );
	}
	idCallback * Clone() const {
		return new ( TAG_FUNC_CALLBACK ) idCallbackBindMemArg1( t, f, a1 );
	}
private:
	T * t;
	void ( T::*f )( A1 );
	A1 a1;
};

/*
================================================================================================

	These are needed because we can't derive the type of an object from the type passed to the
	constructor. If it weren't for these, we'd have to manually specify the type:

		idCallbackBindMem<idFoo>( this, &idFoo::MyFunction );
	becomes:
		MakeCallback( this, &idFoo::MyFunction );

================================================================================================
*/

/*
========================
MakeCallback
========================
*/
ARC_INLINE_EXTERN idCallbackStatic MakeCallback( void ( *f )( void ) ) {
	return idCallbackStatic( f );
}

/*
========================
MakeCallback
========================
*/
template < class T >
ARC_INLINE_EXTERN idCallbackBindMem<T> MakeCallback( T * t, void ( T::*f )( void ) ) {
	return idCallbackBindMem<T>( t, f );
}

/*
========================
MakeCallback
========================
*/
template < class T, typename A1 >
ARC_INLINE_EXTERN idCallbackBindMemArg1<T, A1> MakeCallback( T * t, void ( T::*f )( A1 ), A1 a1 ) {
	return idCallbackBindMemArg1<T, A1>( t, f, a1 );
}

#endif

