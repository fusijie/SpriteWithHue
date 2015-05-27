//
//  CCSpriteWithHue.cpp
//  HelloWorld
//
//  Copyright (c) 2013 Alexey Naumov. All rights reserved.
//  Ported by Jacky on 15/5/8.
//

#include "CCSpriteWithHue.h"

const GLchar* colorRotationShaderBody();
void xRotateMat(float mat[3][3], float rs, float rc);
void yRotateMat(float mat[3][3], float rs, float rc);
void zRotateMat(float mat[3][3], float rs, float rc);
void matrixMult(float a[3][3], float b[3][3], float c[3][3]);
void hueMatrix(GLfloat mat[3][3], float angle);
void premultiplyAlpha(GLfloat mat[3][3], float alpha);

SpriteWithHue* SpriteWithHue::create(const std::string& filename)
{
    SpriteWithHue *sprite = new (std::nothrow) SpriteWithHue();
    if (sprite && sprite->initWithFile(filename))
    {
        sprite->autorelease();
        return sprite;
    }
    CC_SAFE_DELETE(sprite);
    return nullptr;
}

SpriteWithHue* SpriteWithHue::create(const std::string& filename, const cocos2d::Rect& rect)
{
    SpriteWithHue *sprite = new (std::nothrow) SpriteWithHue();
    if (sprite && sprite->initWithFile(filename, rect))
    {
        sprite->autorelease();
        return sprite;
    }
    CC_SAFE_DELETE(sprite);
    return nullptr;
}

SpriteWithHue* SpriteWithHue::createWithTexture(cocos2d::Texture2D *texture)
{
    SpriteWithHue *sprite = new (std::nothrow) SpriteWithHue();
    if (sprite && sprite->initWithTexture(texture))
    {
        sprite->autorelease();
        return sprite;
    }
    CC_SAFE_DELETE(sprite);
    return nullptr;
}

SpriteWithHue* SpriteWithHue::createWithTexture(cocos2d::Texture2D *texture, const cocos2d::Rect& rect, bool rotated)
{
    SpriteWithHue *sprite = new (std::nothrow) SpriteWithHue();
    if (sprite && sprite->initWithTexture(texture, rect, rotated))
    {
        sprite->autorelease();
        return sprite;
    }
    CC_SAFE_DELETE(sprite);
    return nullptr;
}

SpriteWithHue* SpriteWithHue::createWithSpriteFrame(cocos2d::SpriteFrame *spriteFrame)
{
    SpriteWithHue *sprite = new (std::nothrow) SpriteWithHue();
    if (sprite && spriteFrame && sprite->initWithSpriteFrame(spriteFrame))
    {
        sprite->autorelease();
        return sprite;
    }
    CC_SAFE_DELETE(sprite);
    return nullptr;
}

SpriteWithHue* SpriteWithHue::createWithSpriteFrameName(const std::string& spriteFrameName)
{
    cocos2d::SpriteFrame *frame = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(spriteFrameName);
    
#if COCOS2D_DEBUG > 0
    char msg[256] = {0};
    sprintf(msg, "Invalid spriteFrameName: %s", spriteFrameName.c_str());
    CCASSERT(frame != nullptr, msg);
#endif
    
    return createWithSpriteFrame(frame);
}

bool SpriteWithHue::initWithTexture(cocos2d::Texture2D *texture, const cocos2d::Rect &rect, bool rotated)
{
    bool ret = Sprite::initWithTexture(texture, rect, rotated);
    if(ret)
    {
        setupDefaultSettings();
        initShader();
    }
    return ret;
}

bool SpriteWithHue::initWithTexture(cocos2d::Texture2D *texture)
{
    CCASSERT(texture != nullptr, "Invalid texture for sprite");
    
    cocos2d::Rect rect = cocos2d::Rect::ZERO;
    rect.size = texture->getContentSize();
    
    return initWithTexture(texture, rect);
}

bool SpriteWithHue::initWithTexture(cocos2d::Texture2D *texture, const cocos2d::Rect& rect)
{
    return initWithTexture(texture, rect, false);
}

bool SpriteWithHue::initWithSpriteFrame(cocos2d::SpriteFrame *spriteFrame)
{
    CCASSERT(spriteFrame != nullptr, "");
    
    bool bRet = initWithTexture(spriteFrame->getTexture(), spriteFrame->getRect());
    setSpriteFrame(spriteFrame);
    
    return bRet;
}

void SpriteWithHue::setupDefaultSettings()
{
    _hue = 0.0f;
}

void SpriteWithHue::initShader()
{
    auto glProgram = cocos2d::GLProgram::createWithByteArrays(cocos2d::ccPositionTextureColor_noMVP_vert, shaderBody());
    glProgram->bindAttribLocation(cocos2d::GLProgram::ATTRIBUTE_NAME_POSITION, cocos2d::GLProgram::VERTEX_ATTRIB_POSITION);
    glProgram->bindAttribLocation(cocos2d::GLProgram::ATTRIBUTE_NAME_TEX_COORD, cocos2d::GLProgram::VERTEX_ATTRIB_TEX_COORD);
    glProgram->link();
    glProgram->updateUniforms();
    setGLProgram(glProgram);
    updateColor();
}

const GLchar* SpriteWithHue::shaderBody()
{
    return colorRotationShaderBody();
}

void SpriteWithHue::updateColor()
{
    Sprite::updateColor();
    updateColorMatrix();
    updateAlpha();
}

void SpriteWithHue::updateColorMatrix()
{
    GLfloat mat[3][3];
    hueMatrix(mat, _hue);
    premultiplyAlpha(mat, getAlpha());

    auto callback = [this, mat](cocos2d::GLProgram *p, cocos2d::Uniform *u)
    {
      glUniformMatrix3fv(u->location, 1, GL_FALSE, (GLfloat*)&mat);
    };
    getGLProgramState()->setUniformCallback("u_hue", callback);
}

void SpriteWithHue::updateAlpha()
{
    getGLProgramState()->setUniformFloat("u_alpha", getAlpha());
}

GLfloat SpriteWithHue::getAlpha()
{
    return _displayedOpacity/255.0f;
}

float SpriteWithHue::getHue()
{
    return _hue;
}

void SpriteWithHue::setHue(float hue)
{
    _hue = hue;
    updateColorMatrix();
}

//shader

const GLchar * colorRotationShaderBody()
{
    return
    "                                                               \n\
    #ifdef GL_ES                                                    \n\
    precision mediump float;                                        \n\
    #endif                                                          \n\
    \n\
    varying vec2 v_texCoord;                                        \n\
    uniform mat3 u_hue;                                             \n\
    uniform float u_alpha;                                          \n\
    \n\
    void main()                                                     \n\
    {                                                               \n\
    vec4 pixColor = texture2D(CC_Texture0, v_texCoord);             \n\
    vec3 rgbColor = u_hue * pixColor.rgb;                           \n\
    gl_FragColor = vec4(rgbColor, pixColor.a * u_alpha);            \n\
    }                                                               \n\
    ";
}

void xRotateMat(float mat[3][3], float rs, float rc)
{
    mat[0][0] = 1.0;
    mat[0][1] = 0.0;
    mat[0][2] = 0.0;
    
    mat[1][0] = 0.0;
    mat[1][1] = rc;
    mat[1][2] = rs;
    
    mat[2][0] = 0.0;
    mat[2][1] = -rs;
    mat[2][2] = rc;
}

void yRotateMat(float mat[3][3], float rs, float rc)
{
    mat[0][0] = rc;
    mat[0][1] = 0.0;
    mat[0][2] = -rs;
    
    mat[1][0] = 0.0;
    mat[1][1] = 1.0;
    mat[1][2] = 0.0;
    
    mat[2][0] = rs;
    mat[2][1] = 0.0;
    mat[2][2] = rc;
}


void zRotateMat(float mat[3][3], float rs, float rc)
{
    mat[0][0] = rc;
    mat[0][1] = rs;
    mat[0][2] = 0.0;
    
    mat[1][0] = -rs;
    mat[1][1] = rc;
    mat[1][2] = 0.0;
    
    mat[2][0] = 0.0;
    mat[2][1] = 0.0;
    mat[2][2] = 1.0;
}

void matrixMult(float a[3][3], float b[3][3], float c[3][3])
{
    int x, y;
    float temp[3][3];
    
    for(y=0; y<3; y++) {
        for(x=0; x<3; x++) {
            temp[y][x] = b[y][0] * a[0][x] + b[y][1] * a[1][x] + b[y][2] * a[2][x];
        }
    }
    for(y=0; y<3; y++) {
        for(x=0; x<3; x++) {
            c[y][x] = temp[y][x];
        }
    }
}

void hueMatrix(GLfloat mat[3][3], float angle)
{
#define SQRT_2      sqrt(2.0)
#define SQRT_3      sqrt(3.0)
    
    float mag, rot[3][3];
    float xrs, xrc;
    float yrs, yrc;
    float zrs, zrc;
    
    // Rotate the grey vector into positive Z
    mag = SQRT_2;
    xrs = 1.0/mag;
    xrc = 1.0/mag;
    xRotateMat(mat, xrs, xrc);
    mag = SQRT_3;
    yrs = -1.0/mag;
    yrc = SQRT_2/mag;
    yRotateMat(rot, yrs, yrc);
    matrixMult(rot, mat, mat);
    
    // Rotate the hue
    zrs = sin(angle);
    zrc = cos(angle);
    zRotateMat(rot, zrs, zrc);
    matrixMult(rot, mat, mat);
    
    // Rotate the grey vector back into place
    yRotateMat(rot, -yrs, yrc);
    matrixMult(rot,  mat, mat);
    xRotateMat(rot, -xrs, xrc);
    matrixMult(rot,  mat, mat);
}

void premultiplyAlpha(GLfloat mat[3][3], float alpha)
{
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            mat[i][j] *= alpha;
        }
    }
}