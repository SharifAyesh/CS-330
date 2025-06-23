///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
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




/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();

	CreateGLTexture("textures/wood.jpg", "wood"); //My Loaded Image
	CreateGLTexture("textures/counter.jpg", "counter"); // My Loaded Image
	CreateGLTexture("textures/apple.jpg", "apple"); // My Loaded Image
	CreateGLTexture("textures/stainless.jpg", "stainless"); // My Loaded Image
	CreateGLTexture("textures/plate.jpg", "plate"); // My Loaded Image
	CreateGLTexture("textures/ceramic.jpg", "ceramic"); // My Loaded Image
	
	DefineObjectMaterials();
	SetupSceneLights();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes.
 ***********************************************************/
void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// --- Floor Plane (3D base) ---
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // white floor
	SetShaderTexture("counter");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");
	m_basicMeshes->DrawPlaneMesh();

	// ========== Bowl Setup ==========
	float bowlTilt = 10.0f;

	// Bowl Base (Tapered Cylinder)
	scaleXYZ = glm::vec3(1.85f, 0.15f, 1.85f);
	XrotationDegrees = bowlTilt;
	positionXYZ = glm::vec3(0.0f, 0.25f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Bowl Outer Shell (Inverted Sphere)
	scaleXYZ = glm::vec3(2.0f, -1.0f, 2.0f);
	positionXYZ = glm::vec3(0.0f, 0.9f, 0.0f);
	SetTransformations(scaleXYZ, bowlTilt, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Bowl Inner Scoop (Smaller Inverted Sphere)
	scaleXYZ = glm::vec3(1.65f, -0.95f, 1.65f);
	positionXYZ = glm::vec3(0.0f, 0.95f, 0.0f);
	SetTransformations(scaleXYZ, bowlTilt, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Bowl Rim (Thin Cylinder)
	scaleXYZ = glm::vec3(2.05f, 0.05f, 2.05f);
	positionXYZ = glm::vec3(0.0f, 1.38f, 0.0f);
	SetTransformations(scaleXYZ, bowlTilt, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();


	// ========== Spoon Setup ==========

	// Spoon Handle (Cylinder)
	scaleXYZ = glm::vec3(0.12f, 1.2f, 0.12f);
	XrotationDegrees = 90.0f;                      
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -30.0f;                     
	positionXYZ = glm::vec3(2.4f, 0.1f, 0.1f);      
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Spoon Scoop (Sphere Head)
	scaleXYZ = glm::vec3(0.35f, 0.06f, 0.25f);       
	XrotationDegrees = 180.0f;                       
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.45f, 0.07f, 0.05f);    
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("wood");                        
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();
	
	// --- Mug Body ---
	scaleXYZ = glm::vec3(0.5f, 0.8f, 0.5f);
	positionXYZ = glm::vec3(4.0f, 0.5f, -2.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("ceramic");
	//SetShaderMaterial("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	
	// --- Mug Handle ---
	scaleXYZ = glm::vec3(0.25f, 0.25f, 0.25f);
	positionXYZ = glm::vec3(4.55f, 0.9f, -2.5f);  
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("ceramic");
	//SetShaderMaterial("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawTorusMesh();


	// --- Apple Body ---
	scaleXYZ = glm::vec3(0.6f, 0.6f, 0.6f);  
	positionXYZ = glm::vec3(-4.0f, 0.35f, -2.0f);  
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("apple");
	//SetShaderMaterial("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// --- Apple Stem ---
	scaleXYZ = glm::vec3(0.07f, 0.2f, 0.07f); 
	positionXYZ = glm::vec3(-4.0f, 0.9f, -2.0f);  
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("wood");
	//SetShaderMaterial("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();


	// --- Plate ---
	scaleXYZ = glm::vec3(1.2f, 0.05f, 1.2f);  
	positionXYZ = glm::vec3(0.0f, 0.2f, 4.0f);  
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("plate");
	//SetShaderMaterial("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();
}

//*********************Setup Lights ************************* 

void SceneManager::SetupSceneLights()
{
	// Enable lighting in shader
	m_pShaderManager->setBoolValue("bUseLighting", true);

	// Light 0 – main warm light above 
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(-2.0f, 8.0f, 5.5f)); 
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.12f, 0.08f, 0.05f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(0.4f, 0.3f, 0.15f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(0.4f, 0.3f, 0.2f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 30.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.07f);

	// Light 1 – soft warm fill from front-right
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(4.0f, 1.5f, 3.5f));
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.1f, 0.07f, 0.05f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.4f, 0.25f, 0.2f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.1f));
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 25.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.1f);

	// Light 2 – overhead soft fill to brighten everything 
	m_pShaderManager->setVec3Value("lightSources[2].position", glm::vec3(0.0f, 9.0f, 0.0f));
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", glm::vec3(0.25f)); 
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", glm::vec3(0.3f));  
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", glm::vec3(0.1f));
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 80.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.02f); 

	// Light 3 – bowl highlight from front 
	m_pShaderManager->setVec3Value("lightSources[3].position", glm::vec3(2.0f, 2.5f, 3.0f)); 
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", glm::vec3(0.15f, 0.1f, 0.05f));
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", glm::vec3(0.4f, 0.3f, 0.15f)); 
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", glm::vec3(0.2f, 0.2f, 0.2f));
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 35.0f); 
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.05f); 
}

//*********************Define Objects************************


void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.2f, 0.1f, 0.05f);
	woodMaterial.ambientStrength = 0.3f;
	woodMaterial.diffuseColor = glm::vec3(0.4f, 0.2f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.shininess = 16.0f;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL counterMaterial;
	counterMaterial.ambientColor = glm::vec3(0.4f);
	counterMaterial.ambientStrength = 0.2f;
	counterMaterial.diffuseColor = glm::vec3(0.7f);
	counterMaterial.specularColor = glm::vec3(0.9f);
	counterMaterial.shininess = 8.0f;
	counterMaterial.tag = "counter";
	m_objectMaterials.push_back(counterMaterial);
}