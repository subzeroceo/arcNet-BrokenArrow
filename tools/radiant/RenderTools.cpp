#include "..//idlib/precompiled.h"
#pragma hdrstop

#include "../../renderer/tr_local.h"
#include "../../renderer/RenderProgram.h"
#include "rendertools.h"

#include "../../renderer/ImmediateMode.h"


///==================
class fhTrisBuffer {
public:
  fhTrisBuffer();
  void Add(const fhSimpleVert* vertices, int verticesCount);
  void Add(const fhSimpleVert& a, const fhSimpleVert& b, const fhSimpleVert& c);
  void Add(arcVec3 a, arcVec3 b, arcVec3 c, arcVec4 color = arcVec4( 1,1,1,1 ) );
  void Clear();
  void Commit(ARCImage* texture, const arcVec4& colorModulate, const arcVec4& colorAdd);

  const fhSimpleVert* Vertices() const;
  int TriNum() const;

private:
  arcNetList<fhSimpleVert> vertices;
};

class fhSurfaceBuffer {
public:
  fhSurfaceBuffer();
  ~fhSurfaceBuffer();

  fhTrisBuffer* GetMaterialBuffer(const arcMaterial* material);
  fhTrisBuffer* GetColorBuffer();

  void Clear();
  void Commit(const arcVec4& colorModulate = arcVec4( 1,1,1,1 ), const arcVec4& colorAdd = arcVec4(0,0,0,0 ) );

private:
  struct entry_t {
    const arcMaterial* material;
    fhTrisBuffer trisBuffer;
  };

  arcNetList<entry_t*> entries;
  fhTrisBuffer coloredTrisBuffer;
};

class fhPointBuffer {
public:
  fhPointBuffer();
  ~fhPointBuffer();

  void Add(const arcVec3& xyz, const arcVec4& color, float size);
  void Add(const arcVec3& xyz, const arcVec3& color, float size);
  void Clear();
  void Commit();

private:
  struct entry_t {
    arcNetList<fhSimpleVert> vertices;
    float size;
    void Commit();
  };

  arcNetList<entry_t*> entries;
  const short* indices = nullptr;
};

static const int maxVerticesPerCommit = 1024 * 4 * 3;

static const unsigned short* indices() {
  static unsigned short* p = nullptr;
  if ( !p) {
    p = new unsigned short[maxVerticesPerCommit];
    for ( int i=0; i<maxVerticesPerCommit; ++i)
      p[i] = i;
  }
  return p;
}

fhTrisBuffer::fhTrisBuffer() {
  vertices.Resize(4096);
}

void fhTrisBuffer::Add(const fhSimpleVert* vertices, int verticesCount) {
  assert(verticesCount % 3 == 0 );
  for ( int i=0; i<verticesCount; ++i)
    this->vertices.Append(vertices[i] );
}

void fhTrisBuffer::Add(arcVec3 a, arcVec3 b, arcVec3 c, arcVec4 color) {

  fhSimpleVert vert;
  vert.color[0] = static_cast<byte>(color[0] * 255.0f);
  vert.color[1] = static_cast<byte>(color[1] * 255.0f);
  vert.color[2] = static_cast<byte>(color[2] * 255.0f);
  vert.color[3] = static_cast<byte>(color[3] * 255.0f);

  vert.xyz = a;
  vertices.Append(vert);

  vert.xyz = b;
  vertices.Append(vert);

  vert.xyz = c;
  vertices.Append(vert);
}

void fhTrisBuffer::Add(const fhSimpleVert& a, const fhSimpleVert& b, const fhSimpleVert& c) {
  vertices.Append( a );
  vertices.Append( b );
  vertices.Append( c );
}

void fhTrisBuffer::Clear() {
  vertices.SetNum(0, false);
}

const fhSimpleVert* fhTrisBuffer::Vertices() const {
  return vertices.Ptr();
}

int fhTrisBuffer::TriNum() const {
  assert(vertices.Num() % 3 == 0 );
  return vertices.Num() / 3;
}

void fhTrisBuffer::Commit(ARCImage* texture, const arcVec4& colorModulate, const arcVec4& colorAdd) {
  const int verticesUsed = vertices.Num();

  if (verticesUsed > 0 ) {
    if (texture) {
      texture->Bind(1 );
      if (texture->type == TT_CUBIC)
        GL_UseProgram(skyboxProgram);
      else
        GL_UseProgram(defaultProgram);
    } else {
      GL_UseProgram(vertexColorProgram);
    }

    fhRenderProgram::SetModelViewMatrix( GL_ModelViewMatrix.Top() );
    fhRenderProgram::SetProjectionMatrix( GL_ProjectionMatrix.Top() );
    fhRenderProgram::SetDiffuseColor(arcVec4( 1,1,1,1 ) );
	fhRenderProgram::SetColorAdd(colorAdd);
	fhRenderProgram::SetColorModulate(colorModulate);
	fhRenderProgram::SetBumpMatrix(arcVec4( 1,0,0,0 ), arcVec4(0,1,0,0 ) );

    int verticesCommitted = 0;
    while (verticesCommitted < verticesUsed)
    {
      int verticesToCommit = std::min(maxVerticesPerCommit, verticesUsed - verticesCommitted);

      auto vert = vertexCache.AllocFrameTemp(&vertices[verticesCommitted], verticesToCommit * sizeof(fhSimpleVert) );
      int offset = vertexCache.Bind(vert);

	  GL_SetupVertexAttributes(fhVertexLayout::Simple, offset);

      glDrawElements( GL_TRIANGLES,
        verticesToCommit,
        GL_UNSIGNED_SHORT,
        indices() );

      verticesCommitted += verticesToCommit;
    }
  }
}


fhSurfaceBuffer::fhSurfaceBuffer() {
}

fhSurfaceBuffer::~fhSurfaceBuffer() {
  for ( int i=0; i<entries.Num(); ++i) {
    delete entries[i];
  }
}

fhTrisBuffer* fhSurfaceBuffer::GetMaterialBuffer(const arcMaterial* material) {
  if ( !material)
    return GetColorBuffer();

  for ( int i=0; i<entries.Num(); ++i) {
    if (entries[i]->material == nullptr)  {
      entries[i]->material = material;
      entries[i]->trisBuffer.Clear();
      return &entries[i]->trisBuffer;
    }

    if (entries[i]->material == material)
      return &entries[i]->trisBuffer;
  }

  entry_t* entry = new entry_t();
  entry->material = material;
  entries.Append(entry);

  return &entry->trisBuffer;
}

fhTrisBuffer* fhSurfaceBuffer::GetColorBuffer() {
  return &coloredTrisBuffer;
}

void fhSurfaceBuffer::Clear() {
  for ( int i=0; i<entries.Num(); ++i) {
    const auto& entry = entries[i];
    entry->trisBuffer.Clear();
    entry->material = nullptr;
  }
  coloredTrisBuffer.Clear();
}

void fhSurfaceBuffer::Commit(const arcVec4& colorModulate, const arcVec4& colorAdd) {
  for ( int i=0; i<entries.Num(); ++i) {
    entry_t* entry = entries[i];

    if ( !entry->material)
      break;

    entries[i]->trisBuffer.Commit(entry->material->GetEditorImage(), colorModulate, colorAdd);
  }

  this->coloredTrisBuffer.Commit(nullptr, colorModulate, colorAdd);
}

fhPointBuffer::fhPointBuffer() {
}
fhPointBuffer::~fhPointBuffer() {
	for ( int i=0; i<entries.Num(); ++i) {
		delete entries[i];
	}
}

void fhPointBuffer::Add(const arcVec3& xyz, const arcVec4& color, float size) {
  if (size <= 0.001f)
    return;

  fhSimpleVert vert;
  vert.xyz = xyz;
  vert.SetColor(color);

  for ( int i=0; i<entries.Num(); ++i) {
    entry_t* e = entries[i];
    if (e->size <= 0.0 ) {
      e->size = size;
      e->vertices.Append(vert);
      return;
    }

    if (abs(size - e->size) < 0.001) {
      e->vertices.Append(vert);
      return;
    }
  }

  entry_t* e = new entry_t();
  e->size = size;
  e->vertices.Append(vert);
  entries.Append(e);
}

void fhPointBuffer::Add(const arcVec3& xyz, const arcVec3& color, float size) {
  Add(xyz, arcVec4(color, 1.0f), size);
}

void fhPointBuffer::Clear() {
  for ( int i=0; i<entries.Num(); ++i) {
    entry_t* e = entries[i];
    e->vertices.SetNum(0, false);
    e->size = -1;
  }
}

void fhPointBuffer::Commit() {
  for ( int i = 0; i < entries.Num(); ++i) {
    entry_t* e = entries[i];

    if (e->size <= 0.0f)
      return;

    e->Commit();
    e->size = -1;
    e->vertices.SetNum(0, false);
  }
}

void fhPointBuffer::entry_t::Commit() {
  const int verticesUsed = vertices.Num();

  if (verticesUsed > 0 ) {
    glPointSize(size);
    GL_UseProgram(vertexColorProgram);

    fhRenderProgram::SetModelViewMatrix( GL_ModelViewMatrix.Top() );
    fhRenderProgram::SetProjectionMatrix( GL_ProjectionMatrix.Top() );
	fhRenderProgram::SetDiffuseColor(arcVec4( 1,1,1,1 ) );
	fhRenderProgram::SetColorAdd(arcVec4(0,0,0,0 ) );
	fhRenderProgram::SetColorModulate(arcVec4( 1,1,1,1 ) );

    int verticesCommitted = 0;
    while (verticesCommitted < verticesUsed) {
      int verticesToCommit = std::min(maxVerticesPerCommit, verticesUsed - verticesCommitted);

      auto vert = vertexCache.AllocFrameTemp(&vertices[verticesCommitted], verticesToCommit * sizeof(fhSimpleVert) );
      int offset = vertexCache.Bind(vert);

	  GL_SetupVertexAttributes(fhVertexLayout::DrawPosColorOnly, offset);

      glDrawElements( GL_POINTS,
        verticesToCommit,
        GL_UNSIGNED_SHORT,
        ::indices() );

      verticesCommitted += verticesToCommit;
    }

    glPointSize(1 );
  }
}