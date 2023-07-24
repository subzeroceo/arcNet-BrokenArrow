#ifndef __DEBUGGRAPH_H__
#define __DEBUGGRAPH_H__

/*
================================================================================================
Contains the DebugGraph declaration.
================================================================================================
*/

/*
================================================
The *Debug Graph, arcDebugGraph, contains graphing functionality common to many debug tools.
================================================
*/
class arcDebugGraph {
public:
	arcDebugGraph( int numItems = 0 );

	void	Enable( bool b ) { enable = b; }

	// create a graph with the specified number of bars
	void	Init( int numBars );

	void	AddGridLine( float value, const arcVec4 & color );

	// sets a bar value, pass -1 to append an element
	void	SetValue( int b, float value, const arcVec4 & color );
	float	GetValue( int b ) { return bars[b].value; }

	// sets a bar label
	void	SetLabel( int b, const char * text );

	enum fillMode_t {
		GRAPH_LINE,				// only draw a single top line for each bar
		GRAPH_FILL,				// fill the entire bar from the bottom (or left)
		GRAPH_FILL_REVERSE,		// fill the entire bar from the top (or right)
	};
	void	SetFillMode( fillMode_t m ) { mode = m; }

	// render the graph sideways?
	void	SetSideways( bool s ) { sideways = s; }

	// the background color is what's drawn between bars and in the empty space
	void	SetBackgroundColor( const arcVec4 & color ) { bgColor = color; }
	void	SetLabelColor( const arcVec4 & color ) { fontColor = color; }

	// the border specifies the amount of space between bars as well as the amount of space around the entire graph
	void	SetBorder( float b ) { border = b; }

	// set the screen position for the graph
	void	SetPosition( float x, float y, float w, float h ) { position.Set( x, y, w, h ); }

	void	Render( ARCRenderSystem * gui );

private:
	const class arcMaterial * white;
	const class arcMaterial * font;

	arcVec4	bgColor;
	arcVec4	fontColor;
	fillMode_t mode;
	bool	sideways;
	float	border;
	arcVec4	position;
	bool	enable;

	struct graphPlot_t {
		float	value;
		arcVec4	color;
	};
	arcNetList< graphPlot_t >	bars;
	arcNetList< graphPlot_t >	grid;
	arcNetList< arcNetString >			labels;
};

#endif
