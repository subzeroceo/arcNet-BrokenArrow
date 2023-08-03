
#include "../idlib/Lib.h"
#pragma hdrstop

#include "RegExp.h"
#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"

int anRegister::REGCOUNT[NUMTYPES] = {4, 1, 1, 1, 0, 2, 3, 4};

/*
====================
anRegister::SetToRegs
====================
*/
void anRegister::SetToRegs( float *registers ) {
	int i;
	anVec4 v;
	anVec2 v2;
	anVec3 v3;
	idRectangle rect;

	if ( !enabled || var == nullptr || ( var && ( var->GetDict() || !var->GetEval() ) ) ) {
		return;
	}

	switch ( type ) {
		case VEC4: {
			v = *static_cast<idWinVec4*>(var);
			break;
		}
		case RECTANGLE: {
			rect = *static_cast<idWinRectangle*>(var);
			v = rect.ToVec4();
			break;
		}
		case VEC2: {
			v2 = *static_cast<idWinVec2*>(var);
			v[0] = v2[0];
			v[1] = v2[1];
			break;
		}
		case VEC3: {
			v3 = *static_cast<idWinVec3*>(var);
			v[0] = v3[0];
			v[1] = v3[1];
			v[2] = v3[2];
			break;
		}
		case FLOAT: {
			v[0] = *static_cast<idWinFloat*>(var);
			break;
		}
		case INT: {
			v[0] = *static_cast<idWinInt*>(var);
			break;
		}
		case BOOL: {
			v[0] = *static_cast<idWinBool*>(var);
			break;
		}
		default: {
			common->FatalError( "anRegister::SetToRegs: bad reg type" );
			break;
		}
	}
	for ( i = 0; i < regCount; i++ ) {
		registers[ regs[i] ] = v[i];
	}
}

/*
=================
anRegister::GetFromRegs
=================
*/
void anRegister::GetFromRegs( float *registers ) {
	anVec4 v;
	idRectangle rect;

	if ( !enabled || var == nullptr || (var && (var->GetDict() || !var->GetEval()))) {
		return;
	}

	for ( int i = 0; i < regCount; i++ ) {
		v[i] = registers[regs[i]];
	}
	
	switch ( type ) {
		case VEC4: {
			*dynamic_cast<idWinVec4*>(var) = v;
			break;
		}
		case RECTANGLE: {
			rect.x = v.x;
			rect.y = v.y;
			rect.w = v.z;
			rect.h = v.w;
			*static_cast<idWinRectangle*>(var) = rect;
			break;
		}
		case VEC2: {
			*static_cast<idWinVec2*>(var) = v.ToVec2();
			break;
		}
		case VEC3: {
			*static_cast<idWinVec3*>(var) = v.ToVec3();
			break;
		}
		case FLOAT: {
			*static_cast<idWinFloat*>(var) = v[0];
			break;
		}
		case INT: {
			*static_cast<idWinInt*>(var) = v[0];
			break;
		}
		case BOOL: {
			*static_cast<idWinBool*>(var) = ( v[0] != 0.0f );
			break;
		}
		default: {
			common->FatalError( "anRegister::GetFromRegs: bad reg type" );
			break;
		}
	}
}

/*
=================
anRegister::ReadFromDemoFile
=================
*/
void anRegister::ReadFromDemoFile(anSavedGamesFile *f) {
	f->ReadBool( enabled );
	f->ReadShort( type );
	f->ReadInt( regCount );
	for ( int i = 0; i < 4; i++ )
		f->ReadUnsignedShort( regs[i] );
	name = f->ReadHashString();
}

/*
=================
anRegister::WriteToDemoFile
=================
*/
void anRegister::WriteToDemoFile( anSavedGamesFile *f ) {
	f->WriteBool( enabled );
	f->WriteShort( type );
	f->WriteInt( regCount );
	for ( int i = 0; i < 4; i++ )
		f->WriteUnsignedShort( regs[i] );
	f->WriteHashString( name );
}

/*
=================
anRegister::WriteToSaveGame
=================
*/
void anRegister::WriteToSaveGame( anFile *savefile ) {
	int len;

	savefile->Write( &enabled, sizeof( enabled ) );
	savefile->Write( &type, sizeof( type ) );
	savefile->Write( &regCount, sizeof( regCount ) );
	savefile->Write( &regs[0], sizeof( regs ) );
	
	len = name.Length();
	savefile->Write( &len, sizeof( len ) );
	savefile->Write( name.c_str(), len );

	var->WriteToSaveGame( savefile );
}

/*
================
anRegister::ReadFromSaveGame
================
*/
void anRegister::ReadFromSaveGame( anFile *savefile ) {
	int len;

	savefile->Read( &enabled, sizeof( enabled ) );
	savefile->Read( &type, sizeof( type ) );
	savefile->Read( &regCount, sizeof( regCount ) );
	savefile->Read( &regs[0], sizeof( regs ) );

	savefile->Read( &len, sizeof( len ) );
	name.Fill( ' ', len );
	savefile->Read( &name[0], len );

	var->ReadFromSaveGame( savefile );
}

/*
====================
anRegisterList::AddReg
====================
*/
void anRegisterList::AddReg( const char *name, int type, anVec4 data, idWindow *win, idWinVar *var ) {
	if ( FindReg( name ) == nullptr ) {
		assert( type >= 0 && type < anRegister::NUMTYPES );
		int numRegs = anRegister::REGCOUNT[type];
		anRegister *reg = new anRegister( name, type );
		reg->var = var;
		for ( int i = 0; i < numRegs; i++ ) {
			reg->regs[i] = win->ExpressionConstant(data[i]);
		}
		int hash = regHash.GenerateKey( name, false );
		regHash.Add( hash, regs.Append( reg ) );
	}
}

/*
====================
anRegisterList::AddReg
====================
*/
void anRegisterList::AddReg( const char *name, int type, anParser *src, idWindow *win, idWinVar *var ) {
	anRegister* reg;

	reg = FindReg( name );

	if ( reg == nullptr ) {
		assert(type >= 0 && type < anRegister::NUMTYPES);
		int numRegs = anRegister::REGCOUNT[type];
		reg = new anRegister( name, type );
		reg->var = var;
		if ( type == anRegister::STRING ) {
			anToken tok;
			if ( src->ReadToken( &tok ) ) {
				tok = common->GetLanguageDict()->GetString( tok );
				var->Init( tok, win );
			}
		} else {
			for ( int i = 0; i < numRegs; i++ ) {
				reg->regs[i] = win->ParseExpression( src, nullptr );
				if ( i < numRegs-1 ) {
					src->ExpectTokenString( "," );
				}
			}
		}
		int hash = regHash.GenerateKey( name, false );
		regHash.Add( hash, regs.Append( reg ) );
	} else {
		int numRegs = anRegister::REGCOUNT[type];
		reg->var = var;
		if ( type == anRegister::STRING ) {
			anToken tok;
			if ( src->ReadToken( &tok ) ) {
				var->Init( tok, win );
			}
		} else {
			for ( int i = 0; i < numRegs; i++ ) {
				reg->regs[i] = win->ParseExpression( src, nullptr );
				if ( i < numRegs-1 ) {
					src->ExpectTokenString( "," );
				}
			}
		}
	}
}

/*
====================
anRegisterList::GetFromRegs
====================
*/
void anRegisterList::GetFromRegs(float *registers) {
	for ( int i = 0; i < regs.Num(); i++ ) {
		regs[i]->GetFromRegs( registers );
	}
}

/*
====================
anRegisterList::SetToRegs
====================
*/

void anRegisterList::SetToRegs( float *registers ) {
	int i;
	for ( i = 0; i < regs.Num(); i++ ) {
		regs[i]->SetToRegs( registers );
	}
}

/*
====================
anRegisterList::FindReg
====================
*/
anRegister *anRegisterList::FindReg( const char *name ) {
	int hash = regHash.GenerateKey( name, false );
	for ( int i = regHash.First( hash ); i != -1; i = regHash.Next( i ) ) {
		if ( regs[i]->name.Icmp( name ) == 0 ) {
			return regs[i];
		}
	}
	return nullptr;
}

/*
====================
anRegisterList::Reset
====================
*/
void anRegisterList::Reset() {
	regs.DeleteContents( true );
	regHash.Clear();
}

/*
====================
anRegisterList::ReadFromSaveGame
====================
*/
void anRegisterList::ReadFromDemoFile(anSavedGamesFile *f) {
	int c;

	f->ReadInt( c );
	regs.DeleteContents( true );
	for ( int i = 0; i < c; i++ ) {
		anRegister *reg = new anRegister;
		reg->ReadFromDemoFile( f );
		regs.Append( reg );
	}
}

/*
====================
anRegisterList::ReadFromSaveGame
====================
*/
void anRegisterList::WriteToDemoFile(anSavedGamesFile *f) {
	int c = regs.Num();

	f->WriteInt( c );
	for ( int i = 0 ; i < c; i++ ) {
		regs[i]->WriteToDemoFile(f);
	}
}

/*
=====================
anRegisterList::WriteToSaveGame
=====================
*/
void anRegisterList::WriteToSaveGame( anFile *savefile ) {
	int i, num;

	num = regs.Num();
	savefile->Write( &num, sizeof( num ) );

	for ( i = 0; i < num; i++ ) {
		regs[i]->WriteToSaveGame( savefile );
	}
}

/*
====================
anRegisterList::ReadFromSaveGame
====================
*/
void anRegisterList::ReadFromSaveGame( anFile *savefile ) {
	int i, num;

	savefile->Read( &num, sizeof( num ) );
	for ( i = 0; i < num; i++ ) {
		regs[i]->ReadFromSaveGame( savefile );
	}
}
