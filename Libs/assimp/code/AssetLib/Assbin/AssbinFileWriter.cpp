/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/
/** @file  AssbinFileWriter.cpp
 *  @brief Implementation of Assbin file writer.
 */

#include "AssbinFileWriter.h"
#include "Common/assbin_chunks.h"
#include "PostProcessing/ProcessHelper.h"

#include <assimp/Exceptional.h>
#include <assimp/version.h>
#include <assimp/IOStream.hpp>

#include "zlib.h"

#include <ctime>

#if _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4706)
#endif // _MSC_VER

namespace Assimp {

template <typename T>
size_t Write(IOStream *stream, const T &v) {
    return stream->Write(&v, sizeof(T), 1);
}

// -----------------------------------------------------------------------------------
// Serialize an aiString
template <>
inline size_t Write<aiString>(IOStream *stream, const aiString &s) {
    const size_t s2 = (uint32_t)s.length;
    stream->Write(&s, 4, 1);
    stream->Write(s.data, s2, 1);

    return s2 + 4;
}

// -----------------------------------------------------------------------------------
// Serialize an unsigned int as uint32_t
template <>
inline size_t Write<unsigned int>(IOStream *stream, const unsigned int &w) {
    const uint32_t t = (uint32_t)w;
    if (w > t) {
        // this shouldn't happen, integers in Assimp data structures never exceed 2^32
        throw DeadlyExportError("loss of data due to 64 -> 32 bit integer conversion");
    }

    stream->Write(&t, 4, 1);

    return 4;
}

// -----------------------------------------------------------------------------------
// Serialize an unsigned int as uint16_t
template <>
inline size_t Write<uint16_t>(IOStream *stream, const uint16_t &w) {
    static_assert(sizeof(uint16_t) == 2, "sizeof(uint16_t)==2");
    stream->Write(&w, 2, 1);

    return 2;
}

// -----------------------------------------------------------------------------------
// Serialize a float
template <>
inline size_t Write<float>(IOStream *stream, const float &f) {
    static_assert(sizeof(float) == 4, "sizeof(float)==4");
    stream->Write(&f, 4, 1);

    return 4;
}

// -----------------------------------------------------------------------------------
// Serialize a double
template <>
inline size_t Write<double>(IOStream *stream, const double &f) {
    static_assert(sizeof(double) == 8, "sizeof(double)==8");
    stream->Write(&f, 8, 1);

    return 8;
}

// -----------------------------------------------------------------------------------
// Serialize a vec3
template <>
inline size_t Write<aiVector3D>(IOStream *stream, const aiVector3D &v) {
    size_t t = Write<ai_real>(stream, v.x);
    t += Write<float>(stream, v.y);
    t += Write<float>(stream, v.z);

    return t;
}

// -----------------------------------------------------------------------------------
// Serialize a color value
template <>
inline size_t Write<aiColor3D>(IOStream *stream, const aiColor3D &v) {
    size_t t = Write<ai_real>(stream, v.r);
    t += Write<float>(stream, v.g);
    t += Write<float>(stream, v.b);

    return t;
}

// -----------------------------------------------------------------------------------
// Serialize a color value
template <>
inline size_t Write<aiColor4D>(IOStream *stream, const aiColor4D &v) {
    size_t t = Write<ai_real>(stream, v.r);
    t += Write<float>(stream, v.g);
    t += Write<float>(stream, v.b);
    t += Write<float>(stream, v.a);

    return t;
}

// -----------------------------------------------------------------------------------
// Serialize a quaternion
template <>
inline size_t Write<aiQuaternion>(IOStream *stream, const aiQuaternion &v) {
    size_t t = Write<ai_real>(stream, v.w);
    t += Write<float>(stream, v.x);
    t += Write<float>(stream, v.y);
    t += Write<float>(stream, v.z);
    ai_assert(t == 16);

    return t;
}

// -----------------------------------------------------------------------------------
// Serialize a vertex weight
template <>
inline size_t Write<aiVertexWeight>(IOStream *stream, const aiVertexWeight &v) {
    size_t t = Write<unsigned int>(stream, v.mVertexId);

    return t + Write<float>(stream, v.mWeight);
}

constexpr size_t MatrixSize = 64;

// -----------------------------------------------------------------------------------
// Serialize a mat4x4
template <>
inline size_t Write<aiMatrix4x4>(IOStream *stream, const aiMatrix4x4 &m) {
    for (unsigned int i = 0; i < 4; ++i) {
        for (unsigned int i2 = 0; i2 < 4; ++i2) {
            Write<ai_real>(stream, m[i][i2]);
        }
    }

    return MatrixSize;
}

// -----------------------------------------------------------------------------------
// Serialize an aiVectorKey
template <>
inline size_t Write<aiVectorKey>(IOStream *stream, const aiVectorKey &v) {
    const size_t t = Write<double>(stream, v.mTime);
    return t + Write<aiVector3D>(stream, v.mValue);
}

// -----------------------------------------------------------------------------------
// Serialize an aiQuatKey
template <>
inline size_t Write<aiQuatKey>(IOStream *stream, const aiQuatKey &v) {
    const size_t t = Write<double>(stream, v.mTime);
    return t + Write<aiQuaternion>(stream, v.mValue);
}

template <typename T>
inline size_t WriteBounds(IOStream *stream, const T *in, unsigned int size) {
    T minc, maxc;
    ArrayBounds(in, size, minc, maxc);

    const size_t t = Write<T>(stream, minc);
    return t + Write<T>(stream, maxc);
}

// We use this to write out non-byte arrays so that we write using the specializations.
// This way we avoid writing out extra bytes that potentially come from struct alignment.
template <typename T>
inline size_t WriteArray(IOStream *stream, const T *in, unsigned int size) {
    size_t n = 0;
    for (unsigned int i = 0; i < size; i++)
        n += Write<T>(stream, in[i]);

    return n;
}

// ----------------------------------------------------------------------------------
/** @class  AssbinChunkWriter
 *  @brief  Chunk writer mechanism for the .assbin file structure
 *
 *  This is a standard in-memory IOStream (most of the code is based on BlobIOStream),
 *  the difference being that this takes another IOStream as a "container" in the
 *  constructor, and when it is destroyed, it appends the magic number, the chunk size,
 *  and the chunk contents to the container stream. This allows relatively easy chunk
 *  chunk construction, even recursively.
 */
class AssbinChunkWriter : public IOStream {
private:
    uint8_t *buffer;
    uint32_t magic;
    IOStream *container;
    size_t cur_size, cursor, initial;

private:
    // -------------------------------------------------------------------
    void Grow(size_t need = 0) {
        size_t new_size = std::max(initial, std::max(need, cur_size + (cur_size >> 1)));

        const uint8_t *const old = buffer;
        buffer = new uint8_t[new_size];

        if (old) {
            memcpy(buffer, old, cur_size);
            delete[] old;
        }

        cur_size = new_size;
    }

public:
    AssbinChunkWriter(IOStream *container, uint32_t magic, size_t initial = 4096) :
            buffer(nullptr),
            magic(magic),
            container(container),
            cur_size(0),
            cursor(0),
            initial(initial) {
        // empty
    }

    ~AssbinChunkWriter() override {
        if (container) {
            container->Write(&magic, sizeof(uint32_t), 1);
            container->Write(&cursor, sizeof(uint32_t), 1);
            container->Write(buffer, 1, cursor);
        }
        if (buffer) delete[] buffer;
    }

    void *GetBufferPointer() { return buffer; }

    size_t Read(void * /*pvBuffer*/, size_t /*pSize*/, size_t /*pCount*/) override {
        return 0;
    }

    aiReturn Seek(size_t /*pOffset*/, aiOrigin /*pOrigin*/) override {
        return aiReturn_FAILURE;
    }

    size_t Tell() const override {
        return cursor;
    }

    void Flush() override {
        // not implemented
    }

    size_t FileSize() const override {
        return cursor;
    }

    size_t Write(const void *pvBuffer, size_t pSize, size_t pCount) override {
        pSize *= pCount;
        if (cursor + pSize > cur_size) {
            Grow(cursor + pSize);
        }

        memcpy(buffer + cursor, pvBuffer, pSize);
        cursor += pSize;

        return pCount;
    }
};

// ----------------------------------------------------------------------------------
/** @class  AssbinFileWriter
 *  @brief  Assbin file writer class
 *
 *  This class writes an .assbin file, and is responsible for the file layout.
 */
class AssbinFileWriter {
private:
    bool shortened;
    bool compressed;

protected:
    // -----------------------------------------------------------------------------------
    void WriteBinaryNode(IOStream *container, const aiNode *node) {
        AssbinChunkWriter chunk(container, ASSBIN_CHUNK_AINODE);

        unsigned int nb_metadata = (node->mMetaData != nullptr ? node->mMetaData->mNumProperties : 0);

        Write<aiString>(&chunk, node->mName);
        Write<aiMatrix4x4>(&chunk, node->mTransformation);
        Write<unsigned int>(&chunk, node->mNumChildren);
        Write<unsigned int>(&chunk, node->mNumMeshes);
        Write<unsigned int>(&chunk, nb_metadata);

        for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
            Write<unsigned int>(&chunk, node->mMeshes[i]);
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            WriteBinaryNode(&chunk, node->mChildren[i]);
        }

        for (unsigned int i = 0; i < nb_metadata; ++i) {
            const aiString &key = node->mMetaData->mKeys[i];
            aiMetadataType type = node->mMetaData->mValues[i].mType;
            void *value = node->mMetaData->mValues[i].mData;

            Write<aiString>(&chunk, key);
            Write<uint16_t>(&chunk, (uint16_t)type);

            switch (type) {
            case AI_BOOL:
                Write<bool>(&chunk, *((bool *)value));
                break;
            case AI_INT32:
                Write<int32_t>(&chunk, *((int32_t *)value));
                break;
            case AI_UINT64:
                Write<uint64_t>(&chunk, *((uint64_t *)value));
                break;
            case AI_FLOAT:
                Write<float>(&chunk, *((float *)value));
                break;
            case AI_DOUBLE:
                Write<double>(&chunk, *((double *)value));
                break;
            case AI_AISTRING:
                Write<aiString>(&chunk, *((aiString *)value));
                break;
            case AI_AIVECTOR3D:
                Write<aiVector3D>(&chunk, *((aiVector3D *)value));
                break;
#ifdef SWIG
                case FORCE_32BIT:
#endif // SWIG
            default:
                break;
            }
        }
    }

    // -----------------------------------------------------------------------------------
    void WriteBinaryTexture(IOStream *container, const aiTexture *tex) {
        AssbinChunkWriter chunk(container, ASSBIN_CHUNK_AITEXTURE);

        Write<unsigned int>(&chunk, tex->mWidth);
        Write<unsigned int>(&chunk, tex->mHeight);
        // Write the texture format, but don't include the null terminator.
        chunk.Write(tex->achFormatHint, sizeof(char), HINTMAXTEXTURELEN - 1);

        if (!shortened) {
            if (!tex->mHeight) {
                chunk.Write(tex->pcData, 1, tex->mWidth);
            } else {
                chunk.Write(tex->pcData, 1, tex->mWidth * tex->mHeight * 4);
            }
        }
    }

    // -----------------------------------------------------------------------------------
    void WriteBinaryBone(IOStream *container, const aiBone *b) {
        AssbinChunkWriter chunk(container, ASSBIN_CHUNK_AIBONE);

        Write<aiString>(&chunk, b->mName);
        Write<unsigned int>(&chunk, b->mNumWeights);
        Write<aiMatrix4x4>(&chunk, b->mOffsetMatrix);

        // for the moment we write dumb min/max values for the bones, too.
        // maybe I'll add a better, hash-like solution later
        if (shortened) {
            WriteBounds(&chunk, b->mWeights, b->mNumWeights);
        } // else write as usual
        else
            WriteArray<aiVertexWeight>(&chunk, b->mWeights, b->mNumWeights);
    }

    // -----------------------------------------------------------------------------------
    void WriteBinaryMesh(IOStream *container, const aiMesh *mesh) {
        AssbinChunkWriter chunk(container, ASSBIN_CHUNK_AIMESH);

        Write<unsigned int>(&chunk, mesh->mPrimitiveTypes);
        Write<unsigned int>(&chunk, mesh->mNumVertices);
        Write<unsigned int>(&chunk, mesh->mNumFaces);
        Write<unsigned int>(&chunk, mesh->mNumBones);
        Write<unsigned int>(&chunk, mesh->mMaterialIndex);

        // first of all, write bits for all existent vertex components
        unsigned int c = 0;
        if (mesh->mVertices) {
            c |= ASSBIN_MESH_HAS_POSITIONS;
        }
        if (mesh->mNormals) {
            c |= ASSBIN_MESH_HAS_NORMALS;
        }
        if (mesh->mTangents && mesh->mBitangents) {
            c |= ASSBIN_MESH_HAS_TANGENTS_AND_BITANGENTS;
        }
        for (unsigned int n = 0; n < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++n) {
            if (!mesh->mTextureCoords[n]) {
                break;
            }
            c |= ASSBIN_MESH_HAS_TEXCOORD(n);
        }
        for (unsigned int n = 0; n < AI_MAX_NUMBER_OF_COLOR_SETS; ++n) {
            if (!mesh->mColors[n]) {
                break;
            }
            c |= ASSBIN_MESH_HAS_COLOR(n);
        }
        Write<unsigned int>(&chunk, c);

        aiVector3D minVec, maxVec;
        if (mesh->mVertices) {
            if (shortened) {
                WriteBounds(&chunk, mesh->mVertices, mesh->mNumVertices);
            } // else write as usual
            else
                WriteArray<aiVector3D>(&chunk, mesh->mVertices, mesh->mNumVertices);
        }
        if (mesh->mNormals) {
            if (shortened) {
                WriteBounds(&chunk, mesh->mNormals, mesh->mNumVertices);
            } // else write as usual
            else
                WriteArray<aiVector3D>(&chunk, mesh->mNormals, mesh->mNumVertices);
        }
        if (mesh->mTangents && mesh->mBitangents) {
            if (shortened) {
                WriteBounds(&chunk, mesh->mTangents, mesh->mNumVertices);
                WriteBounds(&chunk, mesh->mBitangents, mesh->mNumVertices);
            } // else write as usual
            else {
                WriteArray<aiVector3D>(&chunk, mesh->mTangents, mesh->mNumVertices);
                WriteArray<aiVector3D>(&chunk, mesh->mBitangents, mesh->mNumVertices);
            }
        }
        for (unsigned int n = 0; n < AI_MAX_NUMBER_OF_COLOR_SETS; ++n) {
            if (!mesh->mColors[n])
                break;

            if (shortened) {
                WriteBounds(&chunk, mesh->mColors[n], mesh->mNumVertices);
            } // else write as usual
            else
                WriteArray<aiColor4D>(&chunk, mesh->mColors[n], mesh->mNumVertices);
        }
        for (unsigned int n = 0; n < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++n) {
            if (!mesh->mTextureCoords[n])
                break;

            // write number of UV components
            Write<unsigned int>(&chunk, mesh->mNumUVComponents[n]);

            if (shortened) {
                WriteBounds(&chunk, mesh->mTextureCoords[n], mesh->mNumVertices);
            } // else write as usual
            else
                WriteArray<aiVector3D>(&chunk, mesh->mTextureCoords[n], mesh->mNumVertices);
        }

        // write faces. There are no floating-point calculations involved
        // in these, so we can write a simple hash over the face data
        // to the dump file. We generate a single 32 Bit hash for 512 faces
        // using Assimp's standard hashing function.
        if (shortened) {
            unsigned int processed = 0;
            for (unsigned int job; (job = std::min(mesh->mNumFaces - processed, 512u)); processed += job) {
                uint32_t hash = 0;
                for (unsigned int a = 0; a < job; ++a) {

                    const aiFace &f = mesh->mFaces[processed + a];
                    uint32_t tmp = f.mNumIndices;
                    hash = SuperFastHash(reinterpret_cast<const char *>(&tmp), sizeof tmp, hash);
                    for (unsigned int i = 0; i < f.mNumIndices; ++i) {
                        static_assert(AI_MAX_VERTICES <= 0xffffffff, "AI_MAX_VERTICES <= 0xffffffff");
                        tmp = static_cast<uint32_t>(f.mIndices[i]);
                        hash = SuperFastHash(reinterpret_cast<const char *>(&tmp), sizeof tmp, hash);
                    }
                }
                Write<unsigned int>(&chunk, hash);
            }
        } else // else write as usual
        {
            // if there are less than 2^16 vertices, we can simply use 16 bit integers ...
            for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
                const aiFace &f = mesh->mFaces[i];

                static_assert(AI_MAX_FACE_INDICES <= 0xffff, "AI_MAX_FACE_INDICES <= 0xffff");
                Write<uint16_t>(&chunk, static_cast<uint16_t>(f.mNumIndices));

                for (unsigned int a = 0; a < f.mNumIndices; ++a) {
                    if (mesh->mNumVertices < (1u << 16)) {
                        Write<uint16_t>(&chunk, static_cast<uint16_t>(f.mIndices[a]));
                    } else {
                        Write<unsigned int>(&chunk, f.mIndices[a]);
                    }
                }
            }
        }

        // write bones
        if (mesh->mNumBones) {
            for (unsigned int a = 0; a < mesh->mNumBones; ++a) {
                const aiBone *b = mesh->mBones[a];
                WriteBinaryBone(&chunk, b);
            }
        }
    }

    // -----------------------------------------------------------------------------------
    void WriteBinaryMaterialProperty(IOStream *container, const aiMaterialProperty *prop) {
        AssbinChunkWriter chunk(container, ASSBIN_CHUNK_AIMATERIALPROPERTY);

        Write<aiString>(&chunk, prop->mKey);
        Write<unsigned int>(&chunk, prop->mSemantic);
        Write<unsigned int>(&chunk, prop->mIndex);

        Write<unsigned int>(&chunk, prop->mDataLength);
        Write<unsigned int>(&chunk, (unsigned int)prop->mType);
        chunk.Write(prop->mData, 1, prop->mDataLength);
    }

    // -----------------------------------------------------------------------------------
    void WriteBinaryMaterial(IOStream *container, const aiMaterial *mat) {
        AssbinChunkWriter chunk(container, ASSBIN_CHUNK_AIMATERIAL);

        Write<unsigned int>(&chunk, mat->mNumProperties);
        for (unsigned int i = 0; i < mat->mNumProperties; ++i) {
            WriteBinaryMaterialProperty(&chunk, mat->mProperties[i]);
        }
    }

    // -----------------------------------------------------------------------------------
    void WriteBinaryNodeAnim(IOStream *container, const aiNodeAnim *nd) {
        AssbinChunkWriter chunk(container, ASSBIN_CHUNK_AINODEANIM);

        Write<aiString>(&chunk, nd->mNodeName);
        Write<unsigned int>(&chunk, nd->mNumPositionKeys);
        Write<unsigned int>(&chunk, nd->mNumRotationKeys);
        Write<unsigned int>(&chunk, nd->mNumScalingKeys);
        Write<unsigned int>(&chunk, nd->mPreState);
        Write<unsigned int>(&chunk, nd->mPostState);

        if (nd->mPositionKeys) {
            if (shortened) {
                WriteBounds(&chunk, nd->mPositionKeys, nd->mNumPositionKeys);

            } // else write as usual
            else
                WriteArray<aiVectorKey>(&chunk, nd->mPositionKeys, nd->mNumPositionKeys);
        }
        if (nd->mRotationKeys) {
            if (shortened) {
                WriteBounds(&chunk, nd->mRotationKeys, nd->mNumRotationKeys);

            } // else write as usual
            else
                WriteArray<aiQuatKey>(&chunk, nd->mRotationKeys, nd->mNumRotationKeys);
        }
        if (nd->mScalingKeys) {
            if (shortened) {
                WriteBounds(&chunk, nd->mScalingKeys, nd->mNumScalingKeys);

            } // else write as usual
            else
                WriteArray<aiVectorKey>(&chunk, nd->mScalingKeys, nd->mNumScalingKeys);
        }
    }

    // -----------------------------------------------------------------------------------
    void WriteBinaryAnim(IOStream *container, const aiAnimation *anim) {
        AssbinChunkWriter chunk(container, ASSBIN_CHUNK_AIANIMATION);

        Write<aiString>(&chunk, anim->mName);
        Write<double>(&chunk, anim->mDuration);
        Write<double>(&chunk, anim->mTicksPerSecond);
        Write<unsigned int>(&chunk, anim->mNumChannels);

        for (unsigned int a = 0; a < anim->mNumChannels; ++a) {
            const aiNodeAnim *nd = anim->mChannels[a];
            WriteBinaryNodeAnim(&chunk, nd);
        }
    }

    // -----------------------------------------------------------------------------------
    void WriteBinaryLight(IOStream *container, const aiLight *l) {
        AssbinChunkWriter chunk(container, ASSBIN_CHUNK_AILIGHT);

        Write<aiString>(&chunk, l->mName);
        Write<unsigned int>(&chunk, l->mType);

        Write<aiVector3D>(&chunk, l->mPosition);
        Write<aiVector3D>(&chunk, l->mDirection);
        Write<aiVector3D>(&chunk, l->mUp);

        if (l->mType != aiLightSource_DIRECTIONAL) {
            Write<float>(&chunk, l->mAttenuationConstant);
            Write<float>(&chunk, l->mAttenuationLinear);
            Write<float>(&chunk, l->mAttenuationQuadratic);
        }

        Write<aiColor3D>(&chunk, l->mColorDiffuse);
        Write<aiColor3D>(&chunk, l->mColorSpecular);
        Write<aiColor3D>(&chunk, l->mColorAmbient);

        if (l->mType == aiLightSource_SPOT) {
            Write<float>(&chunk, l->mAngleInnerCone);
            Write<float>(&chunk, l->mAngleOuterCone);
        }
    }

    // -----------------------------------------------------------------------------------
    void WriteBinaryCamera(IOStream *container, const aiCamera *cam) {
        AssbinChunkWriter chunk(container, ASSBIN_CHUNK_AICAMERA);

        Write<aiString>(&chunk, cam->mName);
        Write<aiVector3D>(&chunk, cam->mPosition);
        Write<aiVector3D>(&chunk, cam->mLookAt);
        Write<aiVector3D>(&chunk, cam->mUp);
        Write<float>(&chunk, cam->mHorizontalFOV);
        Write<float>(&chunk, cam->mClipPlaneNear);
        Write<float>(&chunk, cam->mClipPlaneFar);
        Write<float>(&chunk, cam->mAspect);
    }

    // -----------------------------------------------------------------------------------
    void WriteBinaryScene(IOStream *container, const aiScene *scene) {
        AssbinChunkWriter chunk(container, ASSBIN_CHUNK_AISCENE);

        // basic scene information
        Write<unsigned int>(&chunk, scene->mFlags);
        Write<unsigned int>(&chunk, scene->mNumMeshes);
        Write<unsigned int>(&chunk, scene->mNumMaterials);
        Write<unsigned int>(&chunk, scene->mNumAnimations);
        Write<unsigned int>(&chunk, scene->mNumTextures);
        Write<unsigned int>(&chunk, scene->mNumLights);
        Write<unsigned int>(&chunk, scene->mNumCameras);

        // write node graph
        WriteBinaryNode(&chunk, scene->mRootNode);

        // write all meshes
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            const aiMesh *mesh = scene->mMeshes[i];
            WriteBinaryMesh(&chunk, mesh);
        }

        // write materials
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            const aiMaterial *mat = scene->mMaterials[i];
            WriteBinaryMaterial(&chunk, mat);
        }

        // write all animations
        for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
            const aiAnimation *anim = scene->mAnimations[i];
            WriteBinaryAnim(&chunk, anim);
        }

        // write all textures
        for (unsigned int i = 0; i < scene->mNumTextures; ++i) {
            const aiTexture *mesh = scene->mTextures[i];
            WriteBinaryTexture(&chunk, mesh);
        }

        // write lights
        for (unsigned int i = 0; i < scene->mNumLights; ++i) {
            const aiLight *l = scene->mLights[i];
            WriteBinaryLight(&chunk, l);
        }

        // write cameras
        for (unsigned int i = 0; i < scene->mNumCameras; ++i) {
            const aiCamera *cam = scene->mCameras[i];
            WriteBinaryCamera(&chunk, cam);
        }
    }

public:
    AssbinFileWriter(bool shortened, bool compressed) :
            shortened(shortened), compressed(compressed) {
    }

    // -----------------------------------------------------------------------------------
    // Write a binary model dump
    void WriteBinaryDump(const char *pFile, const char *cmd, IOSystem *pIOSystem, const aiScene *pScene) {
        IOStream *out = pIOSystem->Open(pFile, "wb");
        if (!out)
            throw std::runtime_error("Unable to open output file " + std::string(pFile) + '\n');

        auto CloseIOStream = [&]() {
            if (out) {
                pIOSystem->Close(out);
                out = nullptr; // Ensure this is only done once.
            }
        };

        try {
            time_t tt = time(nullptr);
#if _WIN32
            tm *p = gmtime(&tt);
#else
            struct tm now;
            tm *p = gmtime_r(&tt, &now);
#endif

            // header
            char s[64];
            memset(s, 0, 64);
#if _MSC_VER >= 1400
            sprintf_s(s, "ASSIMP.binary-dump.%s", asctime(p));
#else
            ai_snprintf(s, 64, "ASSIMP.binary-dump.%s", asctime(p));
#endif
            out->Write(s, 44, 1);
            // == 44 bytes

            Write<unsigned int>(out, ASSBIN_VERSION_MAJOR);
            Write<unsigned int>(out, ASSBIN_VERSION_MINOR);
            Write<unsigned int>(out, aiGetVersionRevision());
            Write<unsigned int>(out, aiGetCompileFlags());
            Write<uint16_t>(out, shortened);
            Write<uint16_t>(out, compressed);
            // ==  20 bytes

            char buff[256] = { 0 };
            ai_snprintf(buff, 256, "%s", pFile);
            out->Write(buff, sizeof(char), 256);

            memset(buff, 0, sizeof(buff));
            ai_snprintf(buff, 128, "%s", cmd);
            out->Write(buff, sizeof(char), 128);

            // leave 64 bytes free for future extensions
            memset(buff, 0xcd, 64);
            out->Write(buff, sizeof(char), 64);
            // == 435 bytes

            // ==== total header size: 512 bytes
            ai_assert(out->Tell() == ASSBIN_HEADER_LENGTH);

            // Up to here the data is uncompressed. For compressed files, the rest
            // is compressed using standard DEFLATE from zlib.
            if (compressed) {
                AssbinChunkWriter uncompressedStream(nullptr, 0);
                WriteBinaryScene(&uncompressedStream, pScene);

                uLongf uncompressedSize = static_cast<uLongf>(uncompressedStream.Tell());
                uLongf compressedSize = (uLongf)compressBound(uncompressedSize);
                uint8_t *compressedBuffer = new uint8_t[compressedSize];

                int res = compress2(compressedBuffer, &compressedSize, (const Bytef *)uncompressedStream.GetBufferPointer(), uncompressedSize, 9);
                if (res != Z_OK) {
                    delete[] compressedBuffer;
                    throw DeadlyExportError("Compression failed.");
                }

                out->Write(&uncompressedSize, sizeof(uint32_t), 1);
                out->Write(compressedBuffer, sizeof(char), compressedSize);

                delete[] compressedBuffer;
            } else {
                WriteBinaryScene(out, pScene);
            }

            CloseIOStream();
        } catch (...) {
            CloseIOStream();
            throw;
        }
    }
};

void DumpSceneToAssbin(
        const char *pFile, const char *cmd, IOSystem *pIOSystem,
        const aiScene *pScene, bool shortened, bool compressed) {
    AssbinFileWriter fileWriter(shortened, compressed);
    fileWriter.WriteBinaryDump(pFile, cmd, pIOSystem, pScene);
}
#if _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

} // end of namespace Assimp
