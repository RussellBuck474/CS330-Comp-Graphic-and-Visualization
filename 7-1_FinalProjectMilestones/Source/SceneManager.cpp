///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//  Modified by: Student
//  Course: CS-330 Computational Graphics and Visualization
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include <iostream>

// ---------------------------------------------------------------------------
// Shader uniform names
// ---------------------------------------------------------------------------
namespace
{
    const char* g_ModelName = "model";
    const char* g_ColorValueName = "objectColor";
    const char* g_TextureValueName = "objectTexture";
    const char* g_UseTextureName = "bUseTexture";
    const char* g_UseLightingName = "bUseLighting";
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
    m_pShaderManager = pShaderManager;
    m_basicMeshes = new ShapeMeshes();
    m_loadedTextures = 0;
}

// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------
SceneManager::~SceneManager()
{
    DestroyGLTextures();
    delete m_basicMeshes;
    m_basicMeshes = nullptr;
    m_pShaderManager = nullptr;
}

// ---------------------------------------------------------------------------
// CreateGLTexture()
// ---------------------------------------------------------------------------
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(true);

    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (!image)
    {
        std::cout << "Failed to load texture: " << filename << std::endl;
        return false;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (channels == 3)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    else if (channels == 4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(image);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_textureIDs[m_loadedTextures].ID = textureID;
    m_textureIDs[m_loadedTextures].tag = tag;
    m_loadedTextures++;

    return true;
}

// ---------------------------------------------------------------------------
void SceneManager::BindGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
    }
}

// ---------------------------------------------------------------------------
void SceneManager::DestroyGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        glDeleteTextures(1, &m_textureIDs[i].ID);
    }
    m_loadedTextures = 0;
}

// ---------------------------------------------------------------------------
int SceneManager::FindTextureSlot(std::string tag)
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        if (m_textureIDs[i].tag == tag)
            return i;
    }
    return -1;
}

// ---------------------------------------------------------------------------
void SceneManager::SetTransformations(
    glm::vec3 scale,
    float xRot,
    float yRot,
    float zRot,
    glm::vec3 position,
    glm::vec3 offset)
{
    glm::mat4 model =
        glm::translate(position + offset) *
        glm::rotate(glm::radians(zRot), glm::vec3(0, 0, 1)) *
        glm::rotate(glm::radians(yRot), glm::vec3(0, 1, 0)) *
        glm::rotate(glm::radians(xRot), glm::vec3(1, 0, 0)) *
        glm::scale(scale);

    m_pShaderManager->setMat4Value(g_ModelName, model);
}

// ---------------------------------------------------------------------------
void SceneManager::SetShaderColor(float r, float g, float b, float a)
{
    m_pShaderManager->setIntValue(g_UseTextureName, false);
    m_pShaderManager->setVec4Value(g_ColorValueName, glm::vec4(r, g, b, a));
}

// ---------------------------------------------------------------------------
void SceneManager::SetShaderTexture(std::string tag)
{
    m_pShaderManager->setIntValue(g_UseTextureName, true);
    m_pShaderManager->setSampler2DValue(g_TextureValueName, FindTextureSlot(tag));
}

// ---------------------------------------------------------------------------
void SceneManager::SetTextureUVScale(float u, float v)
{
    m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
}

// ---------------------------------------------------------------------------
// PrepareScene()
// ---------------------------------------------------------------------------
void SceneManager::PrepareScene()
{
    CreateGLTexture("textures/pebbles1.jpg", "pebbles");
    CreateGLTexture("textures/water1.jpg", "water");
    CreateGLTexture("textures/worn wall.jpg", "wornwall");
    CreateGLTexture("textures/slate1.jpg", "slate");
    CreateGLTexture("textures/wall.jpg", "wall");
    CreateGLTexture("textures/wood.jpg", "wood");
    CreateGLTexture("textures/silver.jpg", "silver");
    CreateGLTexture("textures/silver1.jpg", "silver1");

    m_basicMeshes->LoadPlaneMesh();
    m_basicMeshes->LoadBoxMesh();
    m_basicMeshes->LoadCylinderMesh();
    m_basicMeshes->LoadTaperedCylinderMesh();
    m_basicMeshes->LoadPyramid4Mesh();
    m_basicMeshes->LoadTorusMesh(0.2f);
    m_basicMeshes->LoadConeMesh();

    SetupSceneLights();
}

// ---------------------------------------------------------------------------
// Lighting
// ---------------------------------------------------------------------------

void SceneManager::SetupSceneLights()
{
    m_pShaderManager->setBoolValue("bUseLighting", true);

    // SUNSET SUN (directional)
    m_pShaderManager->setVec3Value(
        "directionalLight.direction",
        -0.3f, -0.4f, -0.85f
    );

    // STRONG ambient floor (texture-safe)
    m_pShaderManager->setVec3Value(
        "directionalLight.ambient",
        0.75f, 0.72f, 0.68f
    );

    // Warm sunset light
    m_pShaderManager->setVec3Value(
        "directionalLight.diffuse",
        1.2f, 0.9f, 0.6f
    );

    m_pShaderManager->setVec3Value(
        "directionalLight.specular",
        0.2f, 0.2f, 0.2f
    );

    m_pShaderManager->setBoolValue(
        "directionalLight.bActive",
        true
    );

    // Disable point lights (don’t mask the issue)
    for (int i = 0; i < 5; i++)
    {
        m_pShaderManager->setBoolValue(
            ("pointLights[" + std::to_string(i) + "].bActive").c_str(),
            false
        );
    }

}


// ---------------------------------------------------------------------------
// DRAW FUNCTIONS (FULL SCENE)
// ---------------------------------------------------------------------------
void SceneManager::DrawTower(glm::vec3 towerOffset, float towerRotationY)
{
    glm::mat4 rot = glm::rotate(glm::radians(towerRotationY), glm::vec3(0, 1, 0));

    SetTransformations(glm::vec3(3, 6, 1.75f), 0, towerRotationY, 0, towerOffset + glm::vec3(0, 3, 0));
    SetShaderTexture("wall");
    m_basicMeshes->DrawBoxMesh();

    glm::vec3 turretOffsets[4] =
    {
        { 1.5f, 4.5f, 0.75f}, {-1.5f, 4.5f, 0.75f},
        { 1.5f, 4.5f,-0.75f}, {-1.5f, 4.5f,-0.75f}
    };

    for (int i = 0; i < 4; i++)
    {
        glm::vec3 pos = towerOffset + glm::vec3(rot * glm::vec4(turretOffsets[i], 1));
        SetTransformations(glm::vec3(0.25f, 2.3f, 0.25f), 0, towerRotationY, 0, pos);
        SetShaderTexture("wall");
        m_basicMeshes->DrawCylinderMesh();

        SetTransformations(glm::vec3(0.18f, 1.2f, 0.18f), 0, towerRotationY, 0, pos + glm::vec3(0, 2.2f, 0));
        SetShaderTexture("slate");
        m_basicMeshes->DrawTaperedCylinderMesh();

        SetTransformations(glm::vec3(0.15f, 1.4f, 0.15f), 0, towerRotationY, 0, pos + glm::vec3(0, 3.4f, 0));
        SetShaderTexture("wornwall");
        m_basicMeshes->DrawConeMesh();
    }
}

// ---------------------------------------------------------------------------
void SceneManager::DrawWalkway()
{
    SetTransformations(glm::vec3(5.0f, 0.24f, 1.0f), 0, 60, 0, glm::vec3(1.5f, 5.0f, -2.0f));
    SetShaderTexture("slate");
    m_basicMeshes->DrawBoxMesh();
}

// ---------------------------------------------------------------------------
void SceneManager::DrawBridgeDeck()
{
    SetTransformations(glm::vec3(10, 0.4f, 3),
        0, 60, 0,
        glm::vec3(-2.5f, 1.3f, 5.0f));
    
SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
m_basicMeshes->DrawBoxMesh();

}

// ---------------------------------------------------------------------------
void SceneManager::DrawRiverbank()
{
    SetTransformations(glm::vec3(10, 4.5f, 1),
        -6, 0, 0,
        glm::vec3(0, 0.34f, 7));
    SetShaderTexture("pebbles");
    SetTextureUVScale(8, 3);
    m_basicMeshes->DrawPlaneMesh();
}

// ---------------------------------------------------------------------------
void SceneManager::DrawRiver()
{
    SetTransformations(glm::vec3(20, 1, 10),
        0, 0, 0,
        glm::vec3(0, 0.1f, -4));
    SetShaderTexture("water");
    SetTextureUVScale(12, 3);
    m_basicMeshes->DrawPlaneMesh();
}

// ---------------------------------------------------------------------------
// RenderScene()
// ---------------------------------------------------------------------------
void SceneManager::RenderScene()
{
    BindGLTextures();

    SetTransformations(glm::vec3(20, 1, 10), 0, 0, 0, glm::vec3(0));
    SetShaderTexture("pebbles");
    SetTextureUVScale(8, 8);
    m_basicMeshes->DrawPlaneMesh();

    DrawRiverbank();
    DrawRiver();
    DrawTower(glm::vec3(3, 0, -5), -30);
    DrawTower(glm::vec3(0, 0, 1), -30);
    DrawWalkway();
    DrawBridgeDeck();
}