/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2009-2011  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                2010-2012  hkrn                                    */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the MMDAI project team nor the names of     */
/*   its contributors may be used to endorse or promote products     */
/*   derived from this software without specific prior written       */
/*   permission.                                                     */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

#include "vpvl2/vpvl2.h"

#ifdef VPVL2_LINK_ASSIMP
#include "EngineCommon.h"
#include "vpvl2/gl2/AssetRenderEngine.h"

#include "vpvl2/pmx/Bone.h"
#include "vpvl2/asset/Model.h"

#include <map>

namespace vpvl2
{
namespace gl2
{

class AssetRenderEngine::Program : public ObjectProgram
{
public:
    Program(IRenderDelegate *delegate)
        : ObjectProgram(delegate),
          m_modelMatrixUniformLocation(0),
          m_viewProjectionMatrixUniformLocation(0),
          m_cameraPositionUniformLocation(0),
          m_materialColorUniformLocation(0),
          m_materialDiffuseUniformLocation(0),
          m_materialSpecularUniformLocation(0),
          m_materialShininessUniformLocation(0),
          m_hasSubTextureUniformLocation(0),
          m_isMainSphereMapUniformLocation(0),
          m_isSubSphereMapUniformLocation(0),
          m_isMainAdditiveUniformLocation(0),
          m_isSubAdditiveUniformLocation(0),
          m_subTextureUniformLocation(0)
    {
    }
    ~Program() {
        m_cameraPositionUniformLocation = 0;
        m_modelMatrixUniformLocation = 0;
        m_viewProjectionMatrixUniformLocation = 0;
        m_materialColorUniformLocation = 0;
        m_materialDiffuseUniformLocation = 0;
        m_materialSpecularUniformLocation = 0;
        m_materialShininessUniformLocation = 0;
        m_hasSubTextureUniformLocation = 0;
        m_isMainSphereMapUniformLocation = 0;
        m_isSubSphereMapUniformLocation = 0;
        m_isMainAdditiveUniformLocation = 0;
        m_isSubAdditiveUniformLocation = 0;
        m_subTextureUniformLocation = 0;
    }

    void setCameraPosition(const Vector3 &value) {
        glUniform3fv(m_cameraPositionUniformLocation, 1, value);
    }
    void setModelMatrix(const Scalar *value) {
        glUniformMatrix4fv(m_modelMatrixUniformLocation, 1, GL_FALSE, value);
    }
    void setViewProjectionMatrix(const Scalar *value) {
        glUniformMatrix4fv(m_viewProjectionMatrixUniformLocation, 1, GL_FALSE, value);
    }
    void setMaterialColor(const Color &value) {
        glUniform3fv(m_materialColorUniformLocation, 1, value);
    }
    void setMaterialDiffuse(const Color &value) {
        glUniform4fv(m_materialDiffuseUniformLocation, 1, value);
    }
    void setMaterialSpecular(const Color &value) {
        glUniform4fv(m_materialSpecularUniformLocation, 1, value);
    }
    void setMaterialShininess(float value) {
        glUniform1f(m_materialShininessUniformLocation, value);
    }
    void setIsMainSphereMap(bool value) {
        glUniform1i(m_isMainSphereMapUniformLocation, value ? 1 : 0);
    }
    void setIsSubSphereMap(bool value) {
        glUniform1i(m_isSubSphereMapUniformLocation, value ? 1 : 0);
    }
    void setIsMainAdditive(bool value) {
        glUniform1i(m_isMainAdditiveUniformLocation, value ? 1 : 0);
    }
    void setIsSubAdditive(bool value) {
        glUniform1i(m_isSubAdditiveUniformLocation, value ? 1 : 0);
    }
    void setSubTexture(GLuint value) {
        if (value) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, value);
            glUniform1i(m_subTextureUniformLocation, 1);
            glUniform1i(m_hasSubTextureUniformLocation, 1);
        }
        else {
            glUniform1i(m_hasSubTextureUniformLocation, 0);
        }
    }

protected:
    virtual void getUniformLocations() {
        ObjectProgram::getUniformLocations();
        m_cameraPositionUniformLocation = glGetUniformLocation(m_program, "cameraPosition");
        m_modelMatrixUniformLocation = glGetUniformLocation(m_program, "modelMatrix");
        m_viewProjectionMatrixUniformLocation = glGetUniformLocation(m_program, "viewProjectionMatrix");
        m_materialColorUniformLocation = glGetUniformLocation(m_program, "materialColor");
        m_materialDiffuseUniformLocation = glGetUniformLocation(m_program, "materialDiffuse");
        m_materialSpecularUniformLocation = glGetUniformLocation(m_program, "materialSpecular");
        m_materialShininessUniformLocation = glGetUniformLocation(m_program, "materialShininess");
        m_hasSubTextureUniformLocation = glGetUniformLocation(m_program, "hasSubTexture");
        m_isMainSphereMapUniformLocation = glGetUniformLocation(m_program, "isMainSphereMap");
        m_isSubSphereMapUniformLocation = glGetUniformLocation(m_program, "isSubSphereMap");
        m_isMainAdditiveUniformLocation = glGetUniformLocation(m_program, "isMainAdditive");
        m_isSubAdditiveUniformLocation = glGetUniformLocation(m_program, "isSubAdditive");
        m_subTextureUniformLocation = glGetUniformLocation(m_program, "subTexture");
    }

private:
    GLuint m_modelMatrixUniformLocation;
    GLuint m_viewProjectionMatrixUniformLocation;
    GLuint m_cameraPositionUniformLocation;
    GLuint m_normalMatrixUniformLocation;
    GLuint m_materialColorUniformLocation;
    GLuint m_materialDiffuseUniformLocation;
    GLuint m_materialSpecularUniformLocation;
    GLuint m_materialShininessUniformLocation;
    GLuint m_hasSubTextureUniformLocation;
    GLuint m_isMainSphereMapUniformLocation;
    GLuint m_isSubSphereMapUniformLocation;
    GLuint m_isMainAdditiveUniformLocation;
    GLuint m_isSubAdditiveUniformLocation;
    GLuint m_subTextureUniformLocation;
};

struct AssetVertex
{
    AssetVertex() {}
    vpvl2::Vector4 position;
    vpvl2::Vector3 normal;
    vpvl2::Vector3 texcoord;
};
typedef btAlignedObjectArray<AssetVertex> AssetVertices;

class AssetRenderEngine::PrivateContext
{
public:
    typedef std::map<std::string, GLuint> Textures;
    PrivateContext() : cullFaceState(true) {}
    virtual ~PrivateContext() {}

    Textures textures;
    std::map<const struct aiMesh *, size_t> indices;
    std::map<const struct aiNode *, AssetVertices> vertices;
    std::map<const struct aiNode *, GLuint> vbo;
    std::map<const struct aiNode *, GLuint> vao;
    std::map<const struct aiNode *, AssetRenderEngine::Program *> assetPrograms;
    std::map<const struct aiNode *, ZPlotProgram *> zplotPrograms;
    bool cullFaceState;
};

const std::string CanonicalizePath(const std::string &path)
{
    const std::string from("\\"), to("/");
    std::string ret(path);
    std::string::size_type pos(path.find(from));
    while (pos != std::string::npos) {
        ret.replace(pos, from.length(), to);
        pos = ret.find(from, pos + to.length());
    }
    return ret;
}

bool SplitTexturePath(const std::string &path, std::string &mainTexture, std::string &subTexture)
{
    std::string::size_type pos = path.find_first_of("*");
    if (pos != std::string::npos) {
        mainTexture = CanonicalizePath(path.substr(0, pos));
        subTexture  = CanonicalizePath(path.substr(pos + 1));
        return true;
    }
    else {
        mainTexture = CanonicalizePath(path);
        subTexture = "";
        return false;
    }
}

AssetRenderEngine::AssetRenderEngine(IRenderDelegate *delegate, const Scene *scene, asset::Model *model)
    : BaseRenderEngine(scene, delegate),
      #ifdef VPVL2_LINK_QT
      QGLFunctions(),
      #endif /* VPVL2_LINK_QT */
      m_modelRef(model),
      m_context(0)
{
    m_context = new PrivateContext();
#ifdef VPVL2_LINK_QT
    initializeGLFunctions();
#endif /* VPVL2_LINK_QT */
    initializeExtensions();
}

AssetRenderEngine::~AssetRenderEngine()
{
    if (m_modelRef) {
        const aiScene *scene = m_modelRef->aiScenePtr();
        if (scene) {
            const unsigned int nmaterials = scene->mNumMaterials;
            std::string texture, mainTexture, subTexture;
            aiString texturePath;
            for (unsigned int i = 0; i < nmaterials; i++) {
                aiMaterial *material = scene->mMaterials[i];
                aiReturn found = AI_SUCCESS;
                GLuint textureID;
                int textureIndex = 0;
                while (found == AI_SUCCESS) {
                    found = material->GetTexture(aiTextureType_DIFFUSE, textureIndex, &texturePath);
                    if (found != AI_SUCCESS)
                        break;
                    texture = texturePath.data;
                    if (SplitTexturePath(texture, mainTexture, subTexture)) {
                        PrivateContext::Textures::const_iterator sub = m_context->textures.find(subTexture);
                        if (sub != m_context->textures.end()) {
                            textureID = sub->second;
                            glDeleteTextures(1, &textureID);
                            m_context->textures.erase(subTexture);
                        }
                    }
                    PrivateContext::Textures::const_iterator main = m_context->textures.find(mainTexture);
                    if (main != m_context->textures.end()) {
                        textureID = main->second;
                        glDeleteTextures(1, &textureID);
                        m_context->textures.erase(mainTexture);
                    }
                    textureIndex++;
                }
            }
            deleteRecurse(scene, scene->mRootNode);
        }
    }
    delete m_context;
    m_context = 0;
    m_modelRef = 0;
    m_delegateRef = 0;
    m_sceneRef = 0;
}

void AssetRenderEngine::renderModel()
{
    if (!m_modelRef || !m_modelRef->isVisible())
        return;
    const aiScene *a = m_modelRef->aiScenePtr();
    renderRecurse(a, a->mRootNode);
    if (!m_context->cullFaceState) {
        glEnable(GL_CULL_FACE);
        m_context->cullFaceState = true;
    }
}

void AssetRenderEngine::renderEdge()
{
    /* do nothing */
}

void AssetRenderEngine::renderShadow()
{
    /* do nothing */
}

void AssetRenderEngine::renderZPlot()
{
    if (!m_modelRef || !m_modelRef->isVisible())
        return;
    const aiScene *a = m_modelRef->aiScenePtr();
    renderZPlotRecurse(a, a->mRootNode);
}

IModel *AssetRenderEngine::model() const
{
    return m_modelRef;
}

bool AssetRenderEngine::upload(const IString *dir)
{
    if (!m_modelRef)
        return false;
    bool ret = true;
#ifdef VPVL2_LINK_QT
    initializeGLFunctions(QGLContext::currentContext());
#endif /* VPVL2_LINK_QT */
    const aiScene *scene = m_modelRef->aiScenePtr();
    const unsigned int nmaterials = scene->mNumMaterials;
    void *context = 0;
    aiString texturePath;
    std::string path, mainTexture, subTexture;
    m_delegateRef->allocateContext(m_modelRef, context);
    IRenderDelegate::Texture texture;
    GLuint textureID = 0;
    texture.object = &textureID;
    for (unsigned int i = 0; i < nmaterials; i++) {
        aiMaterial *material = scene->mMaterials[i];
        aiReturn found = AI_SUCCESS;
        int textureIndex = 0;
        while (found == AI_SUCCESS) {
            found = material->GetTexture(aiTextureType_DIFFUSE, textureIndex, &texturePath);
            path = texturePath.data;
            if (SplitTexturePath(path, mainTexture, subTexture)) {
                if (m_context->textures[mainTexture] == 0) {
                    IString *mainTexturePath = m_delegateRef->toUnicode(reinterpret_cast<const uint8_t *>(mainTexture.c_str()));
                    ret = m_delegateRef->uploadTexture(mainTexturePath, dir, IRenderDelegate::kTexture2D, texture, context);
                    if (ret) {
                        m_context->textures[mainTexture] = textureID = *static_cast<const GLuint *>(texture.object);
                        log0(context, IRenderDelegate::kLogInfo, "Loaded a main texture: %s (ID=%d)", mainTexturePath->toByteArray(), textureID);
                        delete mainTexturePath;
                    }
                    else {
                        delete mainTexturePath;
                        m_delegateRef->releaseContext(m_modelRef, context);
                        return ret;
                    }
                }
                if (m_context->textures[subTexture] == 0) {
                    IString *subTexturePath = m_delegateRef->toUnicode(reinterpret_cast<const uint8_t *>(subTexture.c_str()));
                    ret = m_delegateRef->uploadTexture(subTexturePath, dir, IRenderDelegate::kTexture2D, texture, context);
                    if (ret) {
                        m_context->textures[subTexture] = textureID = *static_cast<const GLuint *>(texture.object);
                        log0(context, IRenderDelegate::kLogInfo, "Loaded a sub texture: %s (ID=%d)", subTexturePath->toByteArray(), textureID);
                        delete subTexturePath;
                    }
                    else {
                        delete subTexturePath;
                        m_delegateRef->releaseContext(m_modelRef, context);
                        return ret;
                    }
                }
            }
            else if (m_context->textures[mainTexture] == 0) {
                IString *mainTexturePath = m_delegateRef->toUnicode(reinterpret_cast<const uint8_t *>(mainTexture.c_str()));
                ret = m_delegateRef->uploadTexture(mainTexturePath, dir, IRenderDelegate::kTexture2D, texture, context);
                if (ret) {
                    m_context->textures[mainTexture] = textureID = *static_cast<const GLuint *>(texture.object);
                    log0(context, IRenderDelegate::kLogInfo, "Loaded a main texture: %s (ID=%d)", mainTexturePath->toByteArray(), textureID);
                    delete mainTexturePath;
                }
                else {
                    delete mainTexturePath;
                    m_delegateRef->releaseContext(m_modelRef, context);
                    return ret;
                }
            }
            textureIndex++;
        }
    }
    ret = uploadRecurse(scene, scene->mRootNode, dir, context);
    m_modelRef->setVisible(ret);
    m_delegateRef->releaseContext(m_modelRef, context);
    return ret;
}

void AssetRenderEngine::update()
{
    /* do nothing */
}

bool AssetRenderEngine::hasPreProcess() const
{
    return false;
}

bool AssetRenderEngine::hasPostProcess() const
{
    return false;
}

void AssetRenderEngine::preparePostProcess()
{
    /* do nothing */
}

void AssetRenderEngine::performPreProcess()
{
    /* do nothing */
}

void AssetRenderEngine::performPostProcess()
{
    /* do nothing */
}

IEffect *AssetRenderEngine::effect(IEffect::ScriptOrderType /* type */) const
{
    return 0;
}

void AssetRenderEngine::setEffect(IEffect::ScriptOrderType /* type */, IEffect * /* effect */, const IString * /* dir */)
{
    /* do nothing */
}

bool AssetRenderEngine::uploadRecurse(const aiScene *scene, const aiNode *node, const IString *dir, void *context)
{
    bool ret = true;
    const unsigned int nmeshes = node->mNumMeshes;
    AssetVertex assetVertex;
    Program *assetProgram = m_context->assetPrograms[node] = new Program(m_delegateRef);
    if (!createProgram(assetProgram,
                       dir,
                       IRenderDelegate::kModelVertexShader,
                       IRenderDelegate::kModelFragmentShader,
                       context)) {
        return ret;
    }
    ZPlotProgram *zplotProgram = m_context->zplotPrograms[node] = new ZPlotProgram(m_delegateRef);
    if (!createProgram(zplotProgram,
                       dir,
                       IRenderDelegate::kZPlotVertexShader,
                       IRenderDelegate::kZPlotFragmentShader,
                       context)) {
        return ret;
    }
    AssetVertices &assetVertices = m_context->vertices[node];
    for (unsigned int i = 0; i < nmeshes; i++) {
        const struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        const aiVector3D *vertices = mesh->mVertices;
        const aiVector3D *normals = mesh->mNormals;
        const bool hasNormals = mesh->HasNormals();
        const bool hasTexCoords = mesh->HasTextureCoords(0);
        const aiVector3D *texcoords = hasTexCoords ? mesh->mTextureCoords[0] : 0;
        const unsigned int nfaces = mesh->mNumFaces;
        int index = 0;
        for (unsigned int j = 0; j < nfaces; j++) {
            const struct aiFace &face = mesh->mFaces[j];
            const unsigned int nindices = face.mNumIndices;
            for (unsigned int k = 0; k < nindices; k++) {
                int vertexIndex = face.mIndices[k];
                if (hasTexCoords) {
                    const aiVector3D &p = texcoords[vertexIndex];
                    assetVertex.texcoord.setValue(p.x, p.y, 0.0f);
                }
                else {
                    assetVertex.texcoord.setZero();
                }
                if (hasNormals) {
                    const aiVector3D &n = normals[vertexIndex];
                    assetVertex.normal.setValue(n.x, n.y, n.z);
                }
                else {
                    assetVertex.normal.setZero();
                }
                const aiVector3D &v = vertices[vertexIndex];
                assetVertex.position.setValue(v.x, v.y, v.z, 1.0f);
                assetVertices.push_back(assetVertex);
                index++;
            }
        }
        m_context->indices[mesh] = index;
    }
    GLuint &vao = m_context->vao[node];
    allocateVertexArrayObjects(&vao, 1);
    if (bindVertexArrayObject(vao)) {
        log0(context, IRenderDelegate::kLogInfo, "Created an vertex array object (ID=%d)", vao);
    }
    size_t vsize = assetVertices.size() * sizeof(assetVertices[0]);
    GLuint &vbo = m_context->vbo[node];
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vsize, &assetVertices[0].position, GL_STATIC_DRAW);
    bindStaticVertexAttributePointers();
    log0(context, IRenderDelegate::kLogInfo,
         "Binding asset static vertex buffer to the vertex buffer object (ID=%d)", vbo);
    glEnableVertexAttribArray(IModel::IBuffer::kVertexStride);
    glEnableVertexAttribArray(IModel::IBuffer::kNormalStride);
    glEnableVertexAttribArray(IModel::IBuffer::kTextureCoordStride);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    unbindVertexArrayObject();
    const unsigned int nChildNodes = node->mNumChildren;
    for (unsigned int i = 0; i < nChildNodes; i++) {
        ret = uploadRecurse(scene, node->mChildren[i], dir, context);
        if (!ret)
            return ret;
    }
    return ret;
}

void AssetRenderEngine::deleteRecurse(const aiScene *scene, const aiNode *node)
{
    releaseVertexArrayObjects(&m_context->vao[node], 1);
    GLuint vbo = m_context->vbo[node];
    glDeleteBuffers(1, &vbo);
    delete m_context->assetPrograms[node];
    delete m_context->zplotPrograms[node];
    const unsigned int nChildNodes = node->mNumChildren;
    for (unsigned int i = 0; i < nChildNodes; i++)
        deleteRecurse(scene, node->mChildren[i]);
}

void AssetRenderEngine::setAssetMaterial(const aiMaterial *material, Program *program)
{
    int textureIndex = 0;
    GLuint textureID;
    std::string mainTexture, subTexture;
    aiString texturePath;
    if (material->GetTexture(aiTextureType_DIFFUSE, textureIndex, &texturePath) == aiReturn_SUCCESS) {
        bool isAdditive = false;
        if (SplitTexturePath(texturePath.data, mainTexture, subTexture)) {
            textureID = m_context->textures[subTexture];
            isAdditive = subTexture.find(".spa") != std::string::npos;
            program->setSubTexture(textureID);
            program->setIsSubAdditive(isAdditive);
            program->setIsSubSphereMap(isAdditive || subTexture.find(".sph") != std::string::npos);
        }
        textureID = m_context->textures[mainTexture];
        isAdditive = mainTexture.find(".spa") != std::string::npos;
        program->setIsMainAdditive(isAdditive);
        program->setIsMainSphereMap(isAdditive || mainTexture.find(".sph") != std::string::npos);
        program->setMainTexture(textureID);
    }
    else {
        program->setMainTexture(0);
        program->setSubTexture(0);
    }
    aiColor4D ambient, diffuse, specular;
    const Vector3 &lc = m_sceneRef->light()->color();
    Color la, mc, md, ms;
    aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &ambient);
    aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse);
    la.setValue(0.7f - lc.x(), 0.7f - lc.y(), 0.7f - lc.z(), 1.0);
    mc.setValue(diffuse.r * la.x() + ambient.r, diffuse.g * la.y() + ambient.g, diffuse.b * la.z() + ambient.b, diffuse.a);
    md.setValue(diffuse.r, diffuse.g, diffuse.b, diffuse.a);
    program->setMaterialColor(mc);
    program->setMaterialDiffuse(md);
    aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &specular);
    ms.setValue(specular.r * lc.x(), specular.g * lc.y(), specular.b * lc.z(), specular.a);
    program->setMaterialSpecular(ms);
    float shininess, strength;
    int ret1 = aiGetMaterialFloat(material, AI_MATKEY_SHININESS, &shininess);
    int ret2 = aiGetMaterialFloat(material, AI_MATKEY_SHININESS_STRENGTH, &strength);
    if (ret1 == aiReturn_SUCCESS && ret2 == aiReturn_SUCCESS) {
        program->setMaterialShininess(shininess * strength);
    }
    else if (ret1 == aiReturn_SUCCESS) {
        program->setMaterialShininess(shininess);
    }
    else {
        program->setMaterialShininess(15.0f);
    }
    float opacity;
    if (aiGetMaterialFloat(material, AI_MATKEY_OPACITY, &opacity) == aiReturn_SUCCESS) {
        program->setOpacity(opacity * m_modelRef->opacity());
    }
    else {
        program->setOpacity(m_modelRef->opacity());
    }
    void *texture = m_sceneRef->light()->depthTexture();
    if (texture && !btFuzzyZero(opacity - 0.98f)) {
        GLuint textureID = texture ? *static_cast<GLuint *>(texture) : 0;
        program->setDepthTexture(textureID);
    }
    else {
        program->setDepthTexture(0);
    }
    int twoside;
    if (aiGetMaterialInteger(material, AI_MATKEY_TWOSIDED, &twoside) == aiReturn_SUCCESS && twoside && !m_context->cullFaceState) {
        glEnable(GL_CULL_FACE);
        m_context->cullFaceState = true;
    }
    else if (m_context->cullFaceState) {
        glDisable(GL_CULL_FACE);
        m_context->cullFaceState = false;
    }
}

void AssetRenderEngine::renderRecurse(const aiScene *scene, const aiNode *node)
{
    const unsigned int nmeshes = node->mNumMeshes;
    float matrix4x4[16];
    Program *program = m_context->assetPrograms[node];
    program->bind();
    m_delegateRef->getMatrix(matrix4x4, m_modelRef,
                             IRenderDelegate::kViewMatrix
                             | IRenderDelegate::kProjectionMatrix
                             | IRenderDelegate::kCameraMatrix);
    program->setViewProjectionMatrix(matrix4x4);
    m_delegateRef->getMatrix(matrix4x4, m_modelRef,
                             IRenderDelegate::kWorldMatrix
                             | IRenderDelegate::kViewMatrix
                             | IRenderDelegate::kProjectionMatrix
                             | IRenderDelegate::kLightMatrix);
    program->setLightViewProjectionMatrix(matrix4x4);
    m_delegateRef->getMatrix(matrix4x4, m_modelRef,
                             IRenderDelegate::kWorldMatrix
                             | IRenderDelegate::kCameraMatrix);
    program->setModelMatrix(matrix4x4);
    const ILight *light = m_sceneRef->light();
    program->setLightColor(light->color());
    program->setLightDirection(light->direction());
    program->setOpacity(m_modelRef->opacity());
    program->setCameraPosition(m_sceneRef->camera()->lookAt());
    bindVertexBundle(node);
    size_t offset = 0;
    for (unsigned int i = 0; i < nmeshes; i++) {
        const struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        setAssetMaterial(scene->mMaterials[mesh->mMaterialIndex], program);
        size_t nindices = m_context->indices[mesh];
        glDrawArrays(GL_TRIANGLES, offset, nindices);
        offset += nindices;
    }
    unbindVertexBundle();
    program->unbind();
    const unsigned int nChildNodes = node->mNumChildren;
    for (unsigned int i = 0; i < nChildNodes; i++)
        renderRecurse(scene, node->mChildren[i]);
}

void AssetRenderEngine::renderZPlotRecurse(const aiScene *scene, const aiNode *node)
{
    static const AssetVertex v;
    float matrix4x4[16], opacity;
    const unsigned int nmeshes = node->mNumMeshes;
    Program *program = m_context->assetPrograms[node];
    program->bind();
    m_delegateRef->getMatrix(matrix4x4, m_modelRef,
                             IRenderDelegate::kWorldMatrix
                             | IRenderDelegate::kViewMatrix
                             | IRenderDelegate::kProjectionMatrix
                             | IRenderDelegate::kCameraMatrix);
    program->setModelViewProjectionMatrix(matrix4x4);
    bindVertexBundle(node);
    glCullFace(GL_FRONT);
    size_t offset = 0;
    for (unsigned int i = 0; i < nmeshes; i++) {
        const struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        const struct aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        bool succeeded = aiGetMaterialFloat(material, AI_MATKEY_OPACITY, &opacity) == aiReturn_SUCCESS;
        if (succeeded && btFuzzyZero(opacity - 0.98f))
            continue;
        size_t nindices = m_context->indices[mesh];
        glDrawArrays(GL_TRIANGLES, offset, nindices);
        offset += nindices;
    }
    glCullFace(GL_BACK);
    unbindVertexBundle();
    program->unbind();
    const unsigned int nChildNodes = node->mNumChildren;
    for (unsigned int i = 0; i < nChildNodes; i++)
        renderZPlotRecurse(scene, node->mChildren[i]);
}

void AssetRenderEngine::log0(void *context, IRenderDelegate::LogLevel level, const char *format...)
{
    va_list ap;
    va_start(ap, format);
    m_delegateRef->log(context, level, format, ap);
    va_end(ap);
}

bool AssetRenderEngine::createProgram(BaseShaderProgram *program,
                                      const IString *dir,
                                      IRenderDelegate::ShaderType vertexShaderType,
                                      IRenderDelegate::ShaderType fragmentShaderType,
                                      void *context)
{
    IString *vertexShaderSource = 0;
    IString *fragmentShaderSource = 0;
    vertexShaderSource = m_delegateRef->loadShaderSource(vertexShaderType, m_modelRef, dir, context);
    fragmentShaderSource = m_delegateRef->loadShaderSource(fragmentShaderType, m_modelRef, dir, context);
    program->addShaderSource(vertexShaderSource, GL_VERTEX_SHADER, context);
    program->addShaderSource(fragmentShaderSource, GL_FRAGMENT_SHADER, context);
    bool ok = program->linkProgram(context);
    delete vertexShaderSource;
    delete fragmentShaderSource;
    return ok;
}

void AssetRenderEngine::bindVertexBundle(const aiNode *node)
{
    if (!bindVertexArrayObject(m_context->vao[node])) {
        glBindBuffer(GL_ARRAY_BUFFER, m_context->vbo[node]);
        bindStaticVertexAttributePointers();
    }
}

void AssetRenderEngine::unbindVertexBundle()
{
    if (!unbindVertexArrayObject()) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void AssetRenderEngine::bindStaticVertexAttributePointers()
{
    static const AssetVertex v;
    const void *vertexPtr = 0;
    glVertexAttribPointer(IModel::IBuffer::kVertexStride, 3, GL_FLOAT, GL_FALSE, sizeof(v), vertexPtr);
    const void *normalPtr = reinterpret_cast<const void *>(reinterpret_cast<const uint8_t *>(&v.normal) - reinterpret_cast<const uint8_t *>(&v.position));
    glVertexAttribPointer(IModel::IBuffer::kNormalStride, 3, GL_FLOAT, GL_FALSE, sizeof(v), normalPtr);
    const void *texcoordPtr = reinterpret_cast<const void *>(reinterpret_cast<const uint8_t *>(&v.texcoord) - reinterpret_cast<const uint8_t *>(&v.position));
    glVertexAttribPointer(IModel::IBuffer::kTextureCoordStride, 2, GL_FLOAT, GL_FALSE, sizeof(v), texcoordPtr);
}

} /* namespace gl2 */
} /* namespace vpvl2 */

#endif
