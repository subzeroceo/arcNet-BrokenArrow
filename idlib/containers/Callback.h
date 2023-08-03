#ifndef __CALLBACK_H__
#define __CALLBACK_H__

/*
================================================================================================
This file defines a set of template functors for generating callbacks, specifically
the OnChange handlers in the CVar system.
================================================================================================
*/

class anCallBack {
public:
	virtual ~anCallBack() {}
	virtual void Call() = 0;
	virtual anCallBack * Clone() const = 0;
};

// Callback class that forwards the call to a c-style function
class anCallBackStatic : public anCallBack {
public:
	anCallBackStatic( void ( *f )() ) {
		this->f = f;
	}
	void Call() {
		f();
	}
	anCallBack *Clone() const {
		return new ( TAG_FUNC_CALLBACK ) anCallBackStatic( f );
	}
private:
	void (* f)();
};

// Callback class that forwards the call to a member function
template<class T>
class anCallBackBindMem : public anCallBack {
public:
	anCallBackBindMem( T * t, void ( T::*f )() ) {
		this->t = t;
		this->f = f;
	}
	void Call() {
		( t->*f )();
	}
	anCallBack * Clone() const {
		return new ( TAG_FUNC_CALLBACK ) anCallBackBindMem( t, f );
	}
private:
	T * t;
	void ( T::*f )();
};

/*
================================================
anCallBackBindMemArg1

Callback class that forwards the call to a member function with an additional constant parameter
================================================
*/
template<class T, typename A1>
class anCallBackBindMemArg1 : public anCallBack {
public:
	anCallBackBindMemArg1( T * t_, void ( T::*f_ )( A1 ), A1 a1_ ) :
		t( t_ ),
		f( f_ ),
		a1( a1_ ) {
	}
	void Call() {
		( t->*f )( a1 );
	}
	anCallBack * Clone() const {
		return new ( TAG_FUNC_CALLBACK ) anCallBackBindMemArg1( t, f, a1 );
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

		anCallBackBindMem<idFoo>( this, &idFoo::MyFunction );
	becomes:
		MakeCallback( this, &idFoo::MyFunction );

================================================================================================
*/

/*
========================
MakeCallback
========================
*/
ARC_INLINE_EXTERN anCallBackStatic MakeCallback( void ( *f )( void ) ) {
	return anCallBackStatic( f );
}

/*
========================
MakeCallback
========================
*/
template <class T>
ARC_INLINE_EXTERN anCallBackBindMem<T> MakeCallback( T * t, void ( T::*f )( void ) ) {
	return anCallBackBindMem<T>( t, f );
}

/*
========================
MakeCallback
========================
*/
template <class T, typename A1>
ARC_INLINE_EXTERN anCallBackBindMemArg1<T, A1> MakeCallback( T * t, void ( T::*f )( A1 ), A1 a1 ) {
	return anCallBackBindMemArg1<T, A1>( t, f, a1 );
}

#endif

