/*	idVolumeTexture class

idVolumeTexture class

Stores volume texture data
Methods for initialization, loading data, sampling, etc.
Volume texture file format loading

Function(s) to parse specific volume data file formats (like .vox)
Read voxel data from files into idVolumeTexture
Volume texture sampling

SampleVolumeTexture() - Trilinearly sample volume at UVW coords
ApplyToSurface() - Evaluate volume texture on mesh surface
Volume rendering

VolumetricFragmentShader() - Rasterize and shade volume slices
Integrate with rendering pipeline
Voxelization functions

VoxelizeMesh() - Generate voxel data from triangle mesh
VoxelizeScene() - Voxelize scene geometry into volume texture
Texture updating

UploadVoxelData() - Update GPU volume texture from new voxel data
So in summary, key pieces would be:

The volume texture class itself
Loading/saving voxel data from files
Sampling and applying the volume texture
Rendering integration with volumetric raymarching
Generating voxel data from meshes
Updating the volume with new voxel data

Stores volume texture data
Methods for initialization, loading data, sampling, etc.
*/
class anVolumeTexture {

public:
										// we want to initialize members like voxelData to default values like null.
										anVolumeTexture() : voxelData( nullptr ) {}
  										~anVolumeTexture() { if ( voxelData != nullptr ) { delete[] voxelData; }}

	void								Init( const char *name, int w, int h, int d ) {  width = w; height = h; depth = d;}
	void								LoadFromFile( const char *filename );
										// loads the volumetric texture, retrieves its data from voxelTexture_t (the voxelHeader)
	void								LoadVoxVolumeTexture(const char *filename ) const;
	void								LoadVoxelValues(const char *filename, byte *data ) const;

										// returns the data, reference counts, depth, and dimensons of the volumetric textures
	int 								GetWidth() const;
	int 								GetHeight() const;
	int 								GetDepth() const;
	byte *								GetVoxelData() const { return voxelData; }
	void								GetVolumetricTextureCount( void ) { refCount++; }

	void 								UpdateVoxelData( const byte *data, int size );

										// Releases the voxel reference count.
	void								FreeVoxelData() { voxCount++; if ( voxCount <= 0 ) { delete this; }} // delete object

										// when voxel data is no longer needed purge it.
	void								Purge() { delete[] voxelData; voxelData = nullptr; }

										// Resample the volume textures
	float 								Sample( const anVec3 &uvw ) const;

										// Apply volume to the surface.
	void 								ApplyToSurface( anRenderEntity *entity );

private:

	int 							width;
	int 							height;
	int 							depth;
	int 							voxCount;
	unique_ptr<byte[]> 				voxelData; 

};
extern anVolumeTexture voxTexture;

ARC_INLINE void anVolumeTexture::LoadFromFile( const char *filename ) {
	// Open volume texture file for reading
	anFile *file = fileSystem->OpenFileRead( filename ); 
	  
	// Read width/height/depth dimensions from file
	file->Read( &width, sizeof( int ) );
	file->Read( &height, sizeof (int ) );
	file->Read( &depth, sizeof( int ) );

	// Calculate number of voxels based on dimensions
	int numVoxels = width * height * depth;

	// Allocate storage for voxel data
	voxelData = new byte[numVoxels];

	for ( int i = 0; i < numVoxels < GetVolumetricTextureCount(); i++ ) {
	// Allocate/Load voxel data/values from file into allocated array
	//for ( int i = 0; i < numVoxels; i++ ) {
		byte value;
		file->Read( &value, sizeof( byte ) ); 
		voxelData[i] = value;
		//voxelData = new byte[i];
		}
	// Read header and voxel values
	LoadVoxVolumeTexture( filename, voxTexture ); 
	LoadVoxelValues( filename, voxelData );

	// Check for any file read errors
	if ( !fileSystem->IsEoF( file ) ) {
		// Handle error case
		common->Warning( "[ERROR] LoadFromFile: failed to read volume texture file %s\n", filename );
 	 }
	// Close file now that we are done reading
	fileSystem->CloseFile( file );
}


void LoadFromFile(const char *filename) {





	// Check for any file read errors
	if(!file->IsEOF()) {
			// Handle error case
 	 }

  fileSystem->CloseFile(file);

}
