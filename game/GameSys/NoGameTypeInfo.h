// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAMETYPEINFO_H__
#define __GAMETYPEINFO_H__

/*
===================================================================================

	This file has been generated with the Type Info Generator v1.0 (c) 2004 id Software

===================================================================================
*/

typedef struct {
	const char *name;
	const char *type;
	const char *value;
} constantInfo_t;

typedef struct {
	const char *name;
	int value;
} enumValueInfo_t;

typedef struct {
	const char *typeName;
	const enumValueInfo_t * values;
} enumTypeInfo_t;

typedef struct {
	const char *type;
	const char *name;
	int offset;
	int size;
} classVariableInfo_t;

typedef struct {
	const char *typeName;
	const char *superType;
	int size;
	const classVariableInfo_t * variables;
} classTypeInfo_t;


static constantInfo_t constantInfo[] = {
	{ nullptr, nullptr, nullptr }
};

static enumTypeInfo_t enumTypeInfo[] = {
	{ nullptr, nullptr }
};

static classTypeInfo_t classTypeInfo[] = {
	{ nullptr, nullptr, 0, nullptr }
};

#endif /* !__GAMETYPEINFO_H__ */
