#ifndef __PAIR_H__
#define __PAIR_H__

template<class type1, class type2>
class anPair {
public:
	anPair() : first( type1() ), second( type2() ) { }
	anPair( const type1 &T1, const type2 &T2 ) { first = T1; second = T2; };

	anPair( const anPair<type1, type2> &pair ) : first( pair.first ), second( pair.second ) { }
	anPair( type1 &f, type2 &s ) : first( f ), second( s ) { }

	const bool operator==( const anPair &pair ) const {
		return ( pair.first == first ) && ( pair.second == second );
	}

	const bool operator!=( const anPair &pair ) const {
		return !(*this == pair);
	}

	const bool operator==( const anPair<type1, type2>& pair ) const {
		return ( pair.first == first ) && ( pair.second == second );
	}

	const type1 &First( void ) const { return first; };
	const type2 &Second( void ) const { return second; };

	static int anPairFirstCompare( const anPair<type1, type2> *a, const anPair<type1, type2> *b ) {
		return b->First() - a->First();
	}

	static int anPairSecondCompare( const anPair<type1, type2> *a, const anPair<type1, type2> *b ) {
		return  b->Second() - a->Second();
	}

	static int anPairFirstCompareDirect( const anPair<type1, type2> *a, const anPair<type1, type2> *b ) {
		if ( b->First() - a->First() < 0.001f || b->First() - a->First() < -0.001f ) {
			return 0;
		}

		if ( a->First() > b->First() ) {
			return -1;
		} else if ( a->First() < b->First() ) {
			return 1;
		}
		return 0;
	}

	static int anPairSecondCompareDirect( const anPair<type1, type2> *a, const anPair<type1, type2> *b ) {
		if ( b->Second() - a->Second() < 0.001f || b->Second() - a->Second() < -0.001f ) {
			return 0;
		}

		if ( a->Second() > b->Second() ) {
			return -1;
		} else if ( a->Second() < b->Second() ) {
			return 1;
		}
		return 0;
	}

private:
	type1 first;
	type2 second;
};

#endif // ! __PAIR_H__
