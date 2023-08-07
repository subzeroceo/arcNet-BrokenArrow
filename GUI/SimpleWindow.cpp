#include "../idlib/Lib.h"
#pragma hdrstop

#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "SimpleWindow.h"


idSimpleWindow::idSimpleWindow(idWindow *win) {
	gui = win->GetGui();
	dc = win->dc;
	drawRect = win->drawRect;
	clientRect = win->clientRect;
	textRect = win->textRect;
	origin = win->origin;
	fontNum = win->fontNum;
	name = win->name;
	matScalex = win->matScalex;
	matScaley = win->matScaley;
	borderSize = win->borderSize;
	textAlign = win->textAlign;
	textAlignx = win->textAlignx;
	textAligny = win->textAligny;
	background = win->background;
	flags = win->flags;
	textShadow = win->textShadow;

	visible = win->visible;
	text = win->text;
	rect = win->rect;
	backColor = win->backColor;
	matColor = win->matColor;
	foreColor = win->foreColor;
	borderColor = win->borderColor;
	textScale = win->textScale;
	rotate = win->rotate;
	shear = win->shear;
	backGroundName = win->backGroundName;
	if (backGroundName.Length()) {
		background = declManager->FindMaterial(backGroundName);
		background->SetSort( SS_GUI );
		background->SetImageClassifications( 1 );	// just for resource tracking
	}
	backGroundName.SetMaterialPtr(&background);

// 
//  added parent
	mParent = win->GetParent();
// 

	hideCursor = win->hideCursor;

	idWindow *parent = win->GetParent();
	if (parent) {
		if (text.NeedsUpdate()) {
			parent->AddUpdateVar(&text);
		}
		if (visible.NeedsUpdate()) {
			parent->AddUpdateVar(&visible);
		}
		if (rect.NeedsUpdate()) {
			parent->AddUpdateVar(&rect);
		}
		if (backColor.NeedsUpdate()) {
			parent->AddUpdateVar(&backColor);
		}
		if (matColor.NeedsUpdate()) {
			parent->AddUpdateVar(&matColor);
		}
		if (foreColor.NeedsUpdate()) {
			parent->AddUpdateVar(&foreColor);
		}
		if (borderColor.NeedsUpdate()) {
			parent->AddUpdateVar(&borderColor);
		}
		if (textScale.NeedsUpdate()) {
			parent->AddUpdateVar(&textScale);
		}
		if (rotate.NeedsUpdate()) {
			parent->AddUpdateVar(&rotate);
		}
		if ( shear.NeedsUpdate()) {
			parent->AddUpdateVar(&shear);
		}
		if (backGroundName.NeedsUpdate()) {
			parent->AddUpdateVar(&backGroundName);
		}
	}
}

idSimpleWindow::~idSimpleWindow() {

}

void idSimpleWindow::StateChanged( bool redraw ) {
	if ( redraw && background && background->CinematicLength() ) { 
		background->UpdateCinematic( gui->GetTime() );
	}
}

void idSimpleWindow::SetupTransforms(float x, float y) {
	static anMat3 trans;
	static anVec3 org;

	trans.Identity();
	org.Set( origin.x + x, origin.y + y, 0 );
	if ( rotate ) {
		static anRotation rot;
		static anVec3 vec( 0, 0, 1 );
		rot.Set( org, vec, rotate );
		trans = rot.ToMat3();
	}

	static anMat3 smat;
	smat.Identity();
	if ( shear.x() || shear.y()) {
		smat[0][1] = shear.x();
		smat[1][0] = shear.y();
		trans *= smat;
	}

	if ( !trans.IsIdentity() ) {
		dc->SetTransformInfo( org, trans );
	}
}

void idSimpleWindow::DrawBackground(const idRectangle &drawRect) {
	if (backColor.w() > 0) {
		dc->DrawFilledRect(drawRect.x, drawRect.y, drawRect.w, drawRect.h, backColor);
	}

	if (background) {
		if (matColor.w() > 0) {
			float scalex, scaley;
			if ( flags & WIN_NATURALMAT ) {
				scalex = drawRect.w / background->GetImageWidth();
				scaley = drawRect.h / background->GetImageHeight();
			} else {
				scalex = matScalex;
				scaley = matScaley;
			}
			dc->DrawMaterial(drawRect.x, drawRect.y, drawRect.w, drawRect.h, background, matColor, scalex, scaley);
		}
	}
}

void idSimpleWindow::DrawBorderAndCaption(const idRectangle &drawRect) {
	if (flags & WIN_BORDER) {
		if (borderSize) {
			dc->DrawRect(drawRect.x, drawRect.y, drawRect.w, drawRect.h, borderSize, borderColor);
		}
	}
}

void idSimpleWindow::CalcClientRect(float xofs, float yofs) {

	drawRect = rect;

	if ( flags & WIN_INVERTRECT ) {
		drawRect.x = rect.x() - rect.w();
		drawRect.y = rect.y() - rect.h();
	}
	
	drawRect.x += xofs;
	drawRect.y += yofs;

	clientRect = drawRect;
	if (rect.h() > 0.0 && rect.w() > 0.0) {

		if (flags & WIN_BORDER && borderSize != 0.0) {
			clientRect.x += borderSize;
			clientRect.y += borderSize;
			clientRect.w -= borderSize;
			clientRect.h -= borderSize;
		}

		textRect = clientRect;
		textRect.x += 2.0;
	 	textRect.w -= 2.0;
		textRect.y += 2.0;
		textRect.h -= 2.0;
		textRect.x += textAlignx;
		textRect.y += textAligny;

	}
	origin.Set( rect.x() + ( rect.w() / 2 ), rect.y() + ( rect.h() / 2 ) );

}


void idSimpleWindow::Redraw(float x, float y) {
	
	if ( !visible) {
		return;
	}

	CalcClientRect(0, 0);
	dc->SetFont(fontNum);
	drawRect.Offset(x, y);
	clientRect.Offset(x, y);
	textRect.Offset(x, y);
	SetupTransforms(x, y);
	if ( flags & WIN_NOCLIP ) {
		dc->EnableClipping( false );
	}
	DrawBackground(drawRect);
	DrawBorderAndCaption(drawRect);
	if ( textShadow ) {
		anStr shadowText = text;
		idRectangle shadowRect = textRect;

		shadowText.RemoveColors();
		shadowRect.x += textShadow;
		shadowRect.y += textShadow;

		dc->DrawText( shadowText, textScale, textAlign, colorBlack, shadowRect, !( flags & WIN_NOWRAP ), -1 );
	}
	dc->DrawText(text, textScale, textAlign, foreColor, textRect, !( flags & WIN_NOWRAP ), -1);
	dc->SetTransformInfo(vec3_origin, mat3_identity);
	if ( flags & WIN_NOCLIP ) {
		dc->EnableClipping( true );
	}
	drawRect.Offset(-x, -y);
	clientRect.Offset(-x, -y);
	textRect.Offset(-x, -y);
}

int idSimpleWindow::GetWinVarOffset( idWinVar *wv, drawWin_t* owner) {
	int ret = -1;

	if ( wv == &rect ) {
		ret = (int)&( ( idSimpleWindow * ) 0 )->rect;
	}

	if ( wv == &backColor ) {
		ret = (int)&( ( idSimpleWindow * ) 0 )->backColor;
	}

	if ( wv == &matColor ) {
		ret = (int)&( ( idSimpleWindow * ) 0 )->matColor;
	}

	if ( wv == &foreColor ) {
		ret = (int)&( ( idSimpleWindow * ) 0 )->foreColor;
	}

	if ( wv == &borderColor ) {
		ret = (int)&( ( idSimpleWindow * ) 0 )->borderColor;
	}

	if ( wv == &textScale ) {
		ret = (int)&( ( idSimpleWindow * ) 0 )->textScale;
	}

	if ( wv == &rotate ) {
		ret = (int)&( ( idSimpleWindow * ) 0 )->rotate;
	}

	if ( ret != -1 ) {
		owner->simp = this;
	}
	return ret;
}

idWinVar *idSimpleWindow::GetWinVarByName(const char *_name) {
	idWinVar *retVar = nullptr;
	if (anStr::Icmp(_name, "background" ) == 0) {
		retVar = &backGroundName;
	}
	if (anStr::Icmp(_name, "visible" ) == 0) {
		retVar = &visible;
	}
	if (anStr::Icmp(_name, "rect" ) == 0) {
		retVar = &rect;
	}
	if (anStr::Icmp(_name, "backColor" ) == 0) {
		retVar = &backColor;
	}
	if (anStr::Icmp(_name, "matColor" ) == 0) {
		retVar = &matColor;
	}
	if (anStr::Icmp(_name, "foreColor" ) == 0) {
		retVar = &foreColor;
	}
	if (anStr::Icmp(_name, "borderColor" ) == 0) {
		retVar = &borderColor;
	}
	if (anStr::Icmp(_name, "textScale" ) == 0) {
		retVar = &textScale;
	}
	if (anStr::Icmp(_name, "rotate" ) == 0) {
		retVar = &rotate;
	}
	if (anStr::Icmp(_name, "shear" ) == 0) {
		retVar = &shear;
	}
	if (anStr::Icmp(_name, "text" ) == 0) {
		retVar = &text;
	}
	return retVar;
}

/*
========================
idSimpleWindow::WriteToSaveGame
========================
*/
void idSimpleWindow::WriteToSaveGame( anFile *savefile ) {

	savefile->Write( &flags, sizeof( flags ) );
	savefile->Write( &drawRect, sizeof( drawRect ) );
	savefile->Write( &clientRect, sizeof( clientRect ) );
	savefile->Write( &textRect, sizeof( textRect ) );
	savefile->Write( &origin, sizeof( origin ) );
	savefile->Write( &fontNum, sizeof( fontNum ) );
	savefile->Write( &matScalex, sizeof( matScalex ) );
	savefile->Write( &matScaley, sizeof( matScaley ) );
	savefile->Write( &borderSize, sizeof( borderSize ) );
	savefile->Write( &textAlign, sizeof( textAlign ) );
	savefile->Write( &textAlignx, sizeof( textAlignx ) );
	savefile->Write( &textAligny, sizeof( textAligny ) );
	savefile->Write( &textShadow, sizeof( textShadow ) );

	text.WriteToSaveGame( savefile );
	visible.WriteToSaveGame( savefile );
	rect.WriteToSaveGame( savefile );
	backColor.WriteToSaveGame( savefile );
	matColor.WriteToSaveGame( savefile );
	foreColor.WriteToSaveGame( savefile );
	borderColor.WriteToSaveGame( savefile );
	textScale.WriteToSaveGame( savefile );
	rotate.WriteToSaveGame( savefile );
	shear.WriteToSaveGame( savefile );
	backGroundName.WriteToSaveGame( savefile );

	int stringLen;

	if ( background ) {
		stringLen = strlen( background->GetName() );
		savefile->Write( &stringLen, sizeof( stringLen ) );
		savefile->Write( background->GetName(), stringLen );
	} else {
		stringLen = 0;
		savefile->Write( &stringLen, sizeof( stringLen ) );
	}

}

/*
========================
idSimpleWindow::ReadFromSaveGame
========================
*/
void idSimpleWindow::ReadFromSaveGame( anFile *savefile ) {
	savefile->Read( &flags, sizeof( flags ) );
	savefile->Read( &drawRect, sizeof( drawRect ) );
	savefile->Read( &clientRect, sizeof( clientRect ) );
	savefile->Read( &textRect, sizeof( textRect ) );
	savefile->Read( &origin, sizeof( origin ) );
	savefile->Read( &fontNum, sizeof( fontNum ) );
	savefile->Read( &matScalex, sizeof( matScalex ) );
	savefile->Read( &matScaley, sizeof( matScaley ) );
	savefile->Read( &borderSize, sizeof( borderSize ) );
	savefile->Read( &textAlign, sizeof( textAlign ) );
	savefile->Read( &textAlignx, sizeof( textAlignx ) );
	savefile->Read( &textAligny, sizeof( textAligny ) );
	savefile->Read( &textShadow, sizeof( textShadow ) );

	text.ReadFromSaveGame( savefile );
	visible.ReadFromSaveGame( savefile );
	rect.ReadFromSaveGame( savefile );
	backColor.ReadFromSaveGame( savefile );
	matColor.ReadFromSaveGame( savefile );
	foreColor.ReadFromSaveGame( savefile );
	borderColor.ReadFromSaveGame( savefile );
	textScale.ReadFromSaveGame( savefile );
	rotate.ReadFromSaveGame( savefile );
	shear.ReadFromSaveGame( savefile );
	backGroundName.ReadFromSaveGame( savefile );

	int stringLen;

	savefile->Read( &stringLen, sizeof( stringLen ) );
	if ( stringLen > 0 ) {
		anStr backName;

		backName.Fill( ' ', stringLen );
		savefile->Read( &(backName)[0], stringLen );

		background = declManager->FindMaterial( backName );
		background->SetSort( SS_GUI );
	} else {
		background = nullptr;
	}
}

size_t idSimpleWindow::Size() {
	size_t sz = sizeof(* this);
	sz += name.Size();
	sz += text.Size();
	sz += backGroundName.Size();
	return sz;
}
