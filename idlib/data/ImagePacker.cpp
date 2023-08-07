#include "Lib.h"
#pragma hdrstop

#include "ImagePacker.h"

class anImageCompressorNode {
public:
	anImageCompressorNode () {
		children[0] = children[1] = nullptr;
		//imageIndex = -1;
		atCapacity = false;
	}

	anImageCompressorNode ( const anImageCompressorNode &other ) {
		rect = other.rect;
		atCapacity = other.atCapacity;
		if ( other.children[0] ) {
			children[0] = new anImageCompressorNode( *other.children[0] );
		} else {
			children[0] = nullptr;
		}
		if ( other.children[1] ) {
			children[1] = new anImageCompressorNode( *other.children[1] );
		} else {
			children[1] = nullptr;
		}
	}

	~anImageCompressorNode () {
		delete children[0];
		delete children[1];
	}

	anImageCompressorNode *Insert( const anSubImage &image );
	void StuffUnderRect( const anSubImage &image );

	anImageCompressorNode *		children[2];
	anSubImage					rect;
	int							imageIndex;
	bool						atCapacity;
};

anImageCompressorNode *anImageCompressorNode::Insert( const anSubImage &image ) {
	if ( children[0] ) {
		anImageCompressorNode *newNode = children[0]->Insert( image );
		if ( newNode ) {
			return newNode;
		}
		return children[1]->Insert( image );
	} else {
		// If this leaf is already filled with image data
		// we can't do anything here
		if ( atCapacity ) {
			return nullptr;
		}

		// If there is not enough space we will expand at the highest level by doubling the image so just
		// return null for now
		if ( ( rect.width < image.width ) || ( rect.height < image.height ) ) {
			// Try again with flipped image dimensions
			if ( image.x >= 0 /* && tryFlipping */ ) {
				anSubImage flip;
				flip.x = -1;
				flip.y = -1;
				flip.width = image.height;
				flip.height = image.width;
				return Insert( flip );
			} else {
				return nullptr;
			}
		}

		// Exact match
		if ( ( rect.width == image.width ) && ( rect.height == image.height ) ) {
			atCapacity = true;
			return this;
		}

		// Split
		children[0] = new anImageCompressorNode;
		children[1] = new anImageCompressorNode;

		// Slit along the axis that will leave the biggest continuous part, with a preference for horizontal splits if equal
		int dw = rect.width - image.width;
		int dh = rect.height - image.height;

		if ( dw > dh ) {
			// Vertical split
			children[0]->rect.x = rect.x;
			children[0]->rect.y = rect.y;
			children[0]->rect.width = image.width;
			children[0]->rect.height = rect.height;

			children[1]->rect.x = rect.x + image.width;
			children[1]->rect.y = rect.y;
			children[1]->rect.width = rect.width - image.width;
			children[1]->rect.height = rect.height;
		} else {
			// Horizontal split
			children[0]->rect.x = rect.x;
			children[0]->rect.y = rect.y;
			children[0]->rect.width = rect.width;
			children[0]->rect.height = image.height;

			children[1]->rect.x = rect.x;
			children[1]->rect.y = rect.y + image.height;
			children[1]->rect.width = rect.width;
			children[1]->rect.height = rect.height - image.height;
		}

		// children[0] is the best fitting now so insert it there
		return children[0]->Insert( image );
	}
}

void anImageCompressorNode::StuffUnderRect( const anSubImage &image ) {
	if ( !rect.Overlaps( image ) ) {
		return;
	}

	if ( !children[0] ) {
		// stuff leaves
		atCapacity = true;
	} else {
		children[0]->StuffUnderRect( image );
		children[1]->StuffUnderRect( image );
	}
}

anImageCompressor::anImageCompressor( int width, int height ) {
	usedWidth = usedHeight = 0;
	root = new anImageCompressorNode;
	root->rect.x = 0;
	root->rect.width = anMath::CeilPowerOfTwo( width );
	root->rect.y = 0;
	root->rect.height = anMath::CeilPowerOfTwo( height );
}

anImageCompressor::~anImageCompressor() {
	usedWidth = usedHeight = 0;
	delete root;
}

anImageCompressor::anImageCompressor( const anImageCompressor &other ) {
	root = new anImageCompressorNode( *other.root );
	usedWidth = other.usedWidth;
	usedHeight = other.usedHeight;
}

anImageCompressor &anImageCompressor::operator= ( const anImageCompressor &other ) {
	delete root;
	root = new anImageCompressorNode( *other.root );
	usedWidth = other.usedWidth;
	usedHeight = other.usedHeight;
	return *this;
}

anSubImage anImageCompressor::PackImage( int width, int height, bool expandIfFull ) {
	// First time, create a root node
	if ( !root ) {
		root = new anImageCompressorNode;
		root->rect.x = 0;
		root->rect.width = anMath::CeilPowerOfTwo( width );
		root->rect.y = 0;
		root->rect.height = anMath::CeilPowerOfTwo( height );
	}

	// Insert and optionally enlarge the image
	anSubImage arg;
	arg.x = arg.y = 0;
	arg.width = width;
	arg.height = height;

	anImageCompressorNode *resultNode = root->Insert( arg );

	if ( !resultNode ) {
		// Merge the borders as a last effort and try again
		anImageCompressorNode *newRoot = new anImageCompressorNode;
		anImageCompressorNode *newChild1 = new anImageCompressorNode;
		newRoot->rect = root->rect;
		newChild1->rect = root->rect;

		int dw = root->rect.width - usedWidth;
		int dh = root->rect.height - usedHeight;

		if ( dh > dw ) {
			newChild1->rect.y = usedHeight;
			newChild1->rect.height = newRoot->rect.height - usedHeight;
			root->StuffUnderRect( newChild1->rect ); // Dont let the holes in root use this...
			root->rect.height = usedHeight;

			newRoot->children[0] = root;
			newRoot->children[1] = newChild1;
			root = newRoot;
			//common->Printf( "Recovered %i x %i block\n", newChild1->rect.width, newChild1->rect.height );
		} else {
			newChild1->rect.x = usedWidth;
			newChild1->rect.width = newRoot->rect.width - usedWidth;
			root->StuffUnderRect( newChild1->rect ); // Dont let the holes in root use this...
			root->rect.width = usedWidth;

			newRoot->children[0] = root;
			newRoot->children[1] = newChild1;
			root = newRoot;
			//common->Printf( "Recovered %i x %i block\n", newChild1->rect.width, newChild1->rect.height );
		}

		resultNode = root->Insert( arg );
	}

	// Expand if needed
	while ( expandIfFull && !resultNode ) {
		bool isHorizontal = false;
		// Is the topmost split a horizontal or a vertical one?
		if ( root->children[0] && ( root->children[0]->rect.x == root->children[1]->rect.x ) ) {
			isHorizontal = true;
		}

		if ( isHorizontal ) {
			// double with and make root a vertical split
			anImageCompressorNode *newRoot = new anImageCompressorNode;
			anImageCompressorNode *newChild1 = new anImageCompressorNode;

			newRoot->rect = root->rect;
			newRoot->rect.width *= 2;
            newChild1->rect = root->rect;
			newChild1->rect.x = root->rect.x + root->rect.width;

			newRoot->children[0] = root;
			newRoot->children[1] = newChild1;
			root = newRoot;
		} else {
			// double height and make root a horizontal split
			anImageCompressorNode *newRoot = new anImageCompressorNode;
			anImageCompressorNode *newChild1 = new anImageCompressorNode;

			newRoot->rect = root->rect;
			newRoot->rect.height *= 2;
			newChild1->rect = root->rect;
			newChild1->rect.y = root->rect.y + root->rect.height;

			newRoot->children[0] = root;
			newRoot->children[1] = newChild1;
			root = newRoot;
		}

		// Try inserting in the new expanded tree
		resultNode = root->Insert( arg );
	}

	// Return the inserted rect or the empty rect if no suitable location was found
	anSubImage result;
	if ( resultNode ) {
		result = resultNode->rect;
		if ( ( result.x + result.width ) > usedWidth ) {
			usedWidth = result.x + result.width;
		}
		if ( ( result.y + result.height ) > usedHeight ) {
			usedHeight = result.y + result.height;
		}
	} else {
		result.x = result.y = result.width = result.height = 0;
	}
	return result;
}

static void DrawRect( byte *data, int width, int height, anSubImage rect ) {
	int x2 = rect.x + rect.width - 1;
	int y2 = rect.y + rect.height - 1;

	for ( int i = rect.x; i <= x2; i++ ) {
		byte *pixel = data + ( rect.y * width+i )*4;
		pixel[0] = 255;
		pixel[1] = pixel[2] = pixel[3] = 0;
		pixel = data + ( y2 * width + i ) * 4;
		pixel[0] = 255;
		pixel[1] = pixel[2] = pixel[3] = 0;
	}

	for ( int j = rect.y; j <= y2; j++ ) {
		byte *pixel = data + ( j * width + rect.x ) * 4;
		pixel[0] = 255;
		pixel[1] = pixel[2] = pixel[3] = 0;
		pixel = data + ( j * width + x2 ) * 4;
		pixel[0] = 255;
		pixel[1] = pixel[2] = pixel[3] = 0;
	}
}

static void CheckerRect( byte *data, int width, int height, anSubImage rect ) {
	int x2 = rect.x + rect.width - 1;
	int y2 = rect.y + rect.height - 1;

	for ( int i = r ect.x; i <= x2; i++ ) {
		for ( int j = rect.y; j <= y2; j++ ) {
			byte *pixel = data + ( j * width + i ) * 4;
			if ( ( ( j / 5 ) + ( i / 5 ) ) & 1 ) {
				pixel[2] = 255;
				pixel[0] = pixel[1] = pixel[3] = 0;
			}
		}
	}
}

void anImageCompressor::DrawTree( byte *image, int width, int height ) {
	assert( width >= GetWidth() );
	assert( height >= GetHeight() );
	DrawTree_R( root, image, width, height );
}

void anImageCompressor::DrawTree_R( anImageCompressorNode *node, byte *image, int width, int height ) {
	if ( node->atCapacity ) {
		CheckerRect( image, width, height, node->rect );
	}
	DrawRect( image, width, height, node->rect );

	if ( node->children[0] ) {
		DrawTree_R( node->children[0], image, width, height );
	}
	if ( node->children[1] ) {
		DrawTree_R( node->children[1], image, width, height );
	}
}

anSubImage anImageCompressor::PackImage( anSubImage &image ) {
	return PackImage( image.width, image.height );
}

int anImageCompressor::GetWidth( void ) {
	if ( !root ) {
		return 0;
	}
	return root->rect.width;
}

int anImageCompressor::GetHeight( void ) {
	if ( !root ) {
		return 0;
	}
	return root->rect.height;
}
