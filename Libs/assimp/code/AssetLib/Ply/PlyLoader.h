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

/** @file  PLYLoader.h
 *  @brief Declaration of the .ply importer class.
 */
#pragma once
#ifndef AI_PLYLOADER_H_INCLUDED
#define AI_PLYLOADER_H_INCLUDED

#include "PlyParser.h"
#include <assimp/BaseImporter.h>
#include <assimp/types.h>
#include <vector>

struct aiNode;
struct aiMaterial;
struct aiMesh;

namespace Assimp {

using namespace PLY;

// ---------------------------------------------------------------------------
/** Importer class to load the stanford PLY file format
*/
class PLYImporter final : public BaseImporter {
public:
    PLYImporter();
    ~PLYImporter() override;

    // -------------------------------------------------------------------
    /** Returns whether the class can handle the format of the given file.
     * See BaseImporter::CanRead() for details.
     */
    bool CanRead(const std::string &pFile, IOSystem *pIOHandler,
            bool checkSig) const override;

    // -------------------------------------------------------------------
    /** Extract a vertex from the DOM
    */
    void LoadVertex(const PLY::Element *pcElement, const PLY::ElementInstance *instElement, unsigned int pos);

    // -------------------------------------------------------------------
    /** Extract a face from the DOM
    */
    void LoadFace(const PLY::Element *pcElement, const PLY::ElementInstance *instElement, unsigned int pos);

protected:
    // -------------------------------------------------------------------
    /** Return importer meta information.
     * See #BaseImporter::GetInfo for the details
     */
    const aiImporterDesc *GetInfo() const override;

    // -------------------------------------------------------------------
    /** Imports the given file into the given scene structure.
    * See BaseImporter::InternReadFile() for details
    */
    void InternReadFile(const std::string &pFile, aiScene *pScene,
            IOSystem *pIOHandler) override;

    // -------------------------------------------------------------------
    /** Extract a material list from the DOM
    */
    void LoadMaterial(std::vector<aiMaterial *> *pvOut, std::string &defaultTexture, const bool pointsOnly);

    // -------------------------------------------------------------------
    /** Static helper to parse a color from four single channels in
    */
    static void GetMaterialColor(
            const std::vector<PLY::PropertyInstance> &avList,
            unsigned int aiPositions[4],
            PLY::EDataType aiTypes[4],
            aiColor4D *clrOut);

    // -------------------------------------------------------------------
    /** Static helper to parse a color channel value. The input value
    *  is normalized to 0-1.
    */
    static ai_real NormalizeColorValue(
            PLY::PropertyInstance::ValueUnion val,
            PLY::EDataType eType);

private:
    unsigned char *mBuffer;
    PLY::DOM *pcDOM;
    aiMesh *mGeneratedMesh;
};

} // end of namespace Assimp

#endif // AI_3DSIMPORTER_H_INC
