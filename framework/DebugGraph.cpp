#pragma hdrstop
#include "/idlib/Lib.h"

/*
================================================================================================
Contains the DebugGraph implementation.
================================================================================================
*/

/*
========================
arcDebugGraph::arcDebugGraph
========================
*/
arcDebugGraph::arcDebugGraph( int numItems ) :
	bgColor( 0.0f, 0.0f, 0.0f, 0.5f ),
	fontColor( 1.0f, 1.0f, 1.0f, 1.0f ),
	enable( true ),
	mode( GRAPH_FILL ),
	sideways( false ),
	border( 0.0f ),
	position( 100.0f, 100.0f, 100.0f, 100.0f ) {

	Init( numItems );
}

/*
========================
arcDebugGraph::Init
========================
*/
void arcDebugGraph::Init( int numBars ) {
	bars.SetNum( numBars );
	labels.Clear();

	for ( int i = 0; i < numBars; i++ ) {
		bars[i].value = 0.0f;
	}
}

/*
========================
arcDebugGraph::AddGridLine
========================
*/
void arcDebugGraph::AddGridLine( float value, const anVec4 & color ) {
	graphPlot_t & line = grid.Alloc();
	line.value = value;
	line.color = color;
}

/*
========================
arcDebugGraph::SetValue
========================
*/
void arcDebugGraph::SetValue( int b, float value, const anVec4 & color ) {
	if ( !enable ) {
		return;
	}
	if ( b < 0 ) {
		bars.RemoveIndex( 0 );
		graphPlot_t & graph = bars.Alloc();
		graph.value = value;
		graph.color = color;
	} else {
		bars[b].value = value;
		bars[b].color = color;
	}
}

/*
========================
arcDebugGraph::SetLabel
========================
*/
void arcDebugGraph::SetLabel( int b, const char *text ) {
	if ( labels.Num() != bars.Num() ) {
		labels.SetNum( bars.Num() );
	}
	labels[b] = text;
}

/*
========================
arcDebugGraph::Render
========================
*/
void arcDebugGraph::Render( anRenderSystem * gui ) {
	if ( !enable ) {
		return;
	}

	gui->DrawFilled( bgColor, position.x, position.y, position.z, position.w );

	if ( bars.Num() == 0 ) {
		return;
	}

	if ( sideways ) {
		float barWidth = position.z - border * 2.0f;
		float barHeight = ( ( position.w - border ) / ( float )bars.Num() );
		float barLeft = position.x + border;
		float barTop = position.y + border;
		for ( int i = 0; i < bars.Num(); i++ ) {
			anVec4 rect( vec4_zero );
			if ( mode == GRAPH_LINE ) {
				rect.Set( barLeft + barWidth * bars[i].value, barTop + i * barHeight, 1.0f, barHeight - border );
			} else if ( mode == GRAPH_FILL ) {
				rect.Set( barLeft, barTop + i * barHeight, barWidth * bars[i].value, barHeight - border );
			} else if ( mode == GRAPH_FILL_REVERSE ) {
				rect.Set( barLeft + barWidth, barTop + i * barHeight, barWidth - barWidth * bars[i].value, barHeight - border );
			}
			gui->DrawFilled( bars[i].color, rect.x, rect.y, rect.z, rect.w );
		}
		if ( labels.Num() > 0 ) {
			int maxLen = 0;
			for ( int i = 0; i < labels.Num(); i++ ) {
				maxLen = Max( maxLen, labels[i].Length() );
			}
			anVec4 rect( position );
			rect.x -= SMALLCHAR_WIDTH * maxLen;
			rect.z = SMALLCHAR_WIDTH * maxLen;
			gui->DrawFilled( bgColor, rect.x, rect.y, rect.z, rect.w );
			for ( int i = 0; i < labels.Num(); i++ ) {
				anVec2 pos( barLeft - SMALLCHAR_WIDTH * maxLen, barTop + i * barHeight );
				gui->DrawSmallStringExt( anMath::Ftoi( pos.x ), anMath::Ftoi( pos.y ), labels[i], fontColor, true );
			}
		}
	} else {
		float barWidth = ( ( position.z - border ) / ( float )bars.Num() );
		float barHeight = position.w - border * 2.0f;
		float barLeft = position.x + border;
		float barTop = position.y + border;
		float barBottom = barTop + barHeight;

		for ( int i = 0; i < grid.Num(); i++ ) {
			anVec4 rect( position.x, barBottom - barHeight * grid[i].value, position.z, 1.0f );
			gui->DrawFilled( grid[i].color, rect.x, rect.y, rect.z, rect.w );
		}
		for ( int i = 0; i < bars.Num(); i++ ) {
			anVec4 rect;
			if ( mode == GRAPH_LINE ) {
				rect.Set( barLeft + i * barWidth, barBottom - barHeight * bars[i].value, barWidth - border, 1.0f );
			} else if ( mode == GRAPH_FILL ) {
				rect.Set( barLeft + i * barWidth, barBottom - barHeight * bars[i].value, barWidth - border, barHeight * bars[i].value );
			} else if ( mode == GRAPH_FILL_REVERSE ) {
				rect.Set( barLeft + i * barWidth, barTop, barWidth - border, barHeight * bars[i].value );
			}
			gui->DrawFilled( bars[i].color, rect.x, rect.y, rect.z, rect.w );
		}
		if ( labels.Num() > 0 ) {
			anVec4 rect( position );
			rect.y += barHeight;
			rect.w = SMALLCHAR_HEIGHT;
			gui->DrawFilled( bgColor, rect.x, rect.y, rect.z, rect.w );
			for ( int i = 0; i < labels.Num(); i++ ) {
				anVec2 pos( barLeft + i * barWidth, barBottom + border );
				gui->DrawSmallStringExt( anMath::Ftoi( pos.x ), anMath::Ftoi( pos.y ), labels[i], fontColor, true );
			}
		}
	}
}
