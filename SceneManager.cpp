///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
// Seth Simmons
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// ---------------------------------------------------------
	// LOAD TEXTURES
	// ---------------------------------------------------------
	CreateGLTexture(
		"../../Utilities/textures/rusticwood.jpg",
		"wood");

	CreateGLTexture(
		"../../Utilities/textures/stainless.jpg",
		"metal");

	CreateGLTexture(
		"../../Utilities/textures/backdrop.jpg",
		"screen");

	// Bind loaded textures
	BindGLTextures();


	// ---------------------------------------------------------
	// WOOD MATERIAL
	// Used for the textured desk plane
	// ---------------------------------------------------------
	OBJECT_MATERIAL woodMaterial;

	woodMaterial.ambientColor =
		glm::vec3(0.20f, 0.12f, 0.06f);

	woodMaterial.ambientStrength = 0.20f;

	woodMaterial.diffuseColor =
		glm::vec3(0.55f, 0.40f, 0.25f);

	woodMaterial.specularColor =
		glm::vec3(0.15f, 0.15f, 0.15f);

	woodMaterial.shininess = 0.15f;

	woodMaterial.tag = "woodMaterial";

	m_objectMaterials.push_back(woodMaterial);


	// ---------------------------------------------------------
	// METAL MATERIAL
	// Used for the monitor frame, stand, and base
	// ---------------------------------------------------------
	OBJECT_MATERIAL metalMaterial;

	metalMaterial.ambientColor =
		glm::vec3(0.25f, 0.25f, 0.25f);

	metalMaterial.ambientStrength = 0.20f;

	metalMaterial.diffuseColor =
		glm::vec3(0.60f, 0.60f, 0.60f);

	metalMaterial.specularColor =
		glm::vec3(0.80f, 0.80f, 0.80f);

	metalMaterial.shininess = 0.50f;

	metalMaterial.tag = "metalMaterial";

	m_objectMaterials.push_back(metalMaterial);


	// ---------------------------------------------------------
	// SCREEN MATERIAL
	// Used for the monitor display
	// ---------------------------------------------------------
	OBJECT_MATERIAL screenMaterial;

	screenMaterial.ambientColor =
		glm::vec3(0.20f, 0.20f, 0.20f);

	screenMaterial.ambientStrength = 0.20f;

	screenMaterial.diffuseColor =
		glm::vec3(0.50f, 0.50f, 0.50f);

	screenMaterial.specularColor =
		glm::vec3(0.20f, 0.20f, 0.20f);

	screenMaterial.shininess = 0.20f;

	screenMaterial.tag = "screenMaterial";

	m_objectMaterials.push_back(screenMaterial);


	// ---------------------------------------------------------
	// ENABLE LIGHTING
	// ---------------------------------------------------------
	m_pShaderManager->setIntValue(
		g_UseLightingName,
		true);


	// ---------------------------------------------------------
	// MAIN LIGHT
	// Positioned above and in front of the scene
	// ---------------------------------------------------------
	m_pShaderManager->setVec3Value(
		"lightSources[0].position",
		glm::vec3(2.0f, 8.0f, 5.0f));

	m_pShaderManager->setVec3Value(
		"lightSources[0].ambientColor",
		glm::vec3(0.03f, 0.03f, 0.03f));

	m_pShaderManager->setVec3Value(
		"lightSources[0].diffuseColor",
		glm::vec3(0.25f, 0.25f, 0.25f));

	m_pShaderManager->setVec3Value(
		"lightSources[0].specularColor",
		glm::vec3(0.30f, 0.30f, 0.30f));

	m_pShaderManager->setFloatValue(
		"lightSources[0].focalStrength",
		32.0f);

	m_pShaderManager->setFloatValue(
		"lightSources[0].specularIntensity",
		0.15f);


	// ---------------------------------------------------------
	// SECONDARY FILL LIGHT
	// Helps prevent completely dark shadows
	// ---------------------------------------------------------
	m_pShaderManager->setVec3Value(
		"lightSources[1].position",
		glm::vec3(-4.0f, 5.0f, 3.0f));

	m_pShaderManager->setVec3Value(
		"lightSources[1].ambientColor",
		glm::vec3(0.01f, 0.01f, 0.01f));

	m_pShaderManager->setVec3Value(
		"lightSources[1].diffuseColor",
		glm::vec3(0.10f, 0.10f, 0.10f));

	m_pShaderManager->setVec3Value(
		"lightSources[1].specularColor",
		glm::vec3(0.10f, 0.10f, 0.10f));

	m_pShaderManager->setFloatValue(
		"lightSources[1].focalStrength",
		16.0f);

	m_pShaderManager->setFloatValue(
		"lightSources[1].specularIntensity",
		0.05f);


	// ---------------------------------------------------------
	// UNUSED LIGHTS
	// Shader supports four lights, so make lights 2 and 3 dark
	// ---------------------------------------------------------
	m_pShaderManager->setVec3Value(
		"lightSources[2].ambientColor",
		glm::vec3(0.0f, 0.0f, 0.0f));

	m_pShaderManager->setVec3Value(
		"lightSources[2].diffuseColor",
		glm::vec3(0.0f, 0.0f, 0.0f));

	m_pShaderManager->setVec3Value(
		"lightSources[2].specularColor",
		glm::vec3(0.0f, 0.0f, 0.0f));

	m_pShaderManager->setFloatValue(
		"lightSources[2].specularIntensity",
		0.0f);


	m_pShaderManager->setVec3Value(
		"lightSources[3].ambientColor",
		glm::vec3(0.0f, 0.0f, 0.0f));

	m_pShaderManager->setVec3Value(
		"lightSources[3].diffuseColor",
		glm::vec3(0.0f, 0.0f, 0.0f));

	m_pShaderManager->setVec3Value(
		"lightSources[3].specularColor",
		glm::vec3(0.0f, 0.0f, 0.0f));

	m_pShaderManager->setFloatValue(
		"lightSources[3].specularIntensity",
		0.0f);


	// ---------------------------------------------------------
	// LOAD MESHES
	// ---------------------------------------------------------
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// ---------------------------------------------------------

// DESK PLANE
// Large horizontal plane supports the monitor.
// ---------------------------------------------------------
	scaleXYZ = glm::vec3(12.0f, 1.0f, 8.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("wood");
	SetTextureUVScale(4.0f, 4.0f);

	// Wood material controls how the desk reflects light
	SetShaderMaterial("woodMaterial");

	m_basicMeshes->DrawPlaneMesh();

// MONITOR FRAME
// Large black box forms the outer body of the monitor.
// ---------------------------------------------------------
	scaleXYZ = glm::vec3(6.0f, 3.8f, 0.35f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 4.5f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metalMaterial");
	m_basicMeshes->DrawBoxMesh();


	// ------------------------------------------------------------
// MONITOR SCREEN
// Use a different texture than the monitor frame.
// ------------------------------------------------------------
	scaleXYZ = glm::vec3(5.2f, 3.0f, 0.10f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 4.5f, 0.18f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("screen");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("screenMaterial");

	m_basicMeshes->DrawBoxMesh();


	// ---------------------------------------------------------
	// LOWER SILVER PANEL
	// This box creates the thicker lower portion of the monitor.
	// ---------------------------------------------------------
	scaleXYZ = glm::vec3(6.0f, 0.7f, 0.38f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 2.55f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metalMaterial");
	m_basicMeshes->DrawBoxMesh();


	// ---------------------------------------------------------
	// MONITOR STAND
	// Narrow vertical box supports the monitor.
	// ---------------------------------------------------------
	scaleXYZ = glm::vec3(0.9f, 1.5f, 0.65f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 1.35f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metalMaterial");
	m_basicMeshes->DrawBoxMesh();


	// ---------------------------------------------------------
	// MONITOR BASE
	// Wide flat box forms the base of the stand.
	// ---------------------------------------------------------
	scaleXYZ = glm::vec3(2.6f, 0.25f, 1.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.45f, 0.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("metal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("metalMaterial");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}
