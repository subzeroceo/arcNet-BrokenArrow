#ifndef __ATOMIC_H__
#define __ATOMIC_H__

class Atomic {
public:

	// Atomically compares the value at dest with comperand.
	// If dest value is equal to comperand, the value at dest is replaced with exchange.
	static bool CompareAndSwap( volatile int *dest, int comperand, int exchange)  {

#if defined(_WIN32)
	return (comperand == ::InterlockedCompareExchange( dest, exchange, comperand ) );
#elif defined(__linux__)
	int old;
	asm volatile(
		"lock\n"
		"cmpxchgl %2,%1\n"
		: "=&a"(old), "=m"(*dest) 
		: "r"(exchange), "m"(*dest), "0"(comperand)
		: "memory");
	return old == comperand;

#endif

  }

};

#endif // !__ATOMIC_H__
