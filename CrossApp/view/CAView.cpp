//
//  CAView.cpp
//  CrossApp
//
//  Created by Li Yuanfeng on 14-5-12.
//  Copyright (c) 2014Âπ?http://9miao.com All rights reserved.
//

#include "CAView.h"
#include "support/CAPointExtension.h"
#include "math/TransformUtils.h"
#include "basics/CACamera.h"
#include "basics/CAApplication.h"
#include "basics/CAScheduler.h"
#include "dispatcher/CATouch.h"
#include "shaders/CAGLProgram.h"
#include "CABatchView.h"
#include "kazmath/GL/matrix.h"
#include <stdarg.h>
#include "dispatcher/CATouchDispatcher.h"
#include "dispatcher/CAKeypadDispatcher.h"
#include "shaders/CAShaderCache.h"
#include "shaders/CAGLProgram.h"
#include "shaders/ccGLStateCache.h"
#include "CCEGLView.h"
#include "cocoa/CCSet.h"
#include "CAImageView.h"
#include "animation/CAViewAnimation.h"
#include "CADrawingPrimitives.h"
#include "platform/CADensityDpi.h"
#include "ccMacros.h"
#include "game/CGNode.h"

#include "script_support/CCScriptSupport.h"
NS_CC_BEGIN;


static int viewCount = 0;

static int s_globalOrderOfArrival = 1;

CAView::CAView(void)
: m_fRotationX(0.0f)
, m_fRotationY(0.0f)
, m_fRotationZ(0.0f)
, m_fScaleX(1.0f)
, m_fScaleY(1.0f)
, m_fScaleZ(1.0f)
, m_fSkewX(0.0f)
, m_fSkewY(0.0f)
, m_obAnchorPointInPoints(DPOINT_FLT_MIN)
, m_obAnchorPoint(DPOINT_FLT_MIN)
, m_obContentSize(DSizeZero)
, m_obPoint(DPOINT_FLT_MIN)
, m_fVertexZ(0.0f)
, m_obRect(DRectZero)
, m_tAdditionalTransform(Mat4::IDENTITY)
, m_bTransformDirty(true)
, m_bInverseDirty(true)
, m_bUseAdditionalTransform(false)
, m_bTransformUpdated(true)
, m_pCamera(NULL)
, m_nZOrder(0)
, m_pSuperview(NULL)
, m_pShaderProgram(NULL)
, m_eGLServerState(ccGLServerState(0))
, m_uOrderOfArrival(0)
, m_bRunning(false)
, m_bVisible(true)
, m_bReorderChildDirty(false)
, _displayedAlpha(1.0f)
, _realAlpha(1.0f)
, _displayedColor(CAColor_white)
, _realColor(CAColor_white)
, m_bOpacityModifyRGB(true)
, m_bDisplayRange(true)
, m_pobImage(NULL)
, m_bShouldBeHidden(false)
, m_bFlipX(false)
, m_bFlipY(false)
, m_bRecursiveDirty(false)
, m_bDirty(false)
, m_bHasChildren(false)
, m_pContentContainer(NULL)
, m_bIsAnimation(false)
, m_pobBatchView(NULL)
, m_pobImageAtlas(NULL)
, m_bLeftShadowed(false)
, m_bRightShadowed(false)
, m_bTopShadowed(false)
, m_bBottomShadowed(false)
, m_pParentCGNode(NULL)
, m_pCGNode(NULL)
, m_obLayout(DLayoutZero)
, m_eLayoutType(0)
{
    this->setShaderProgram(CAShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor));
    m_sBlendFunc.src = CC_BLEND_SRC;
    m_sBlendFunc.dst = CC_BLEND_DST;
    memset(&m_sQuad, 0, sizeof(m_sQuad));
    
    CAColor4B tmpColor = CAColor_white;
    m_sQuad.bl.colors = tmpColor;
    m_sQuad.br.colors = tmpColor;
    m_sQuad.tl.colors = tmpColor;
    m_sQuad.tr.colors = tmpColor;
    
    m_tTransform = m_tInverse = m_tAdditionalTransform = Mat4::IDENTITY;
    
    this->updateRotationQuat();
    this->setAnchorPoint(DPoint(0.5f, 0.5f));
    
    //CCLog("CAView = %d\n", ++viewCount);
}

CAView::~CAView(void)
{
    CAScheduler::getScheduler()->pauseTarget(this);
    
    CC_SAFE_RELEASE(m_pCamera);
    CC_SAFE_RELEASE(m_pShaderProgram);
    
    for (auto& subview : m_obSubviews)
    {
        subview->setSuperview(NULL);
    }
    m_obSubviews.clear();
    
    CC_SAFE_RELEASE(m_pobImage);
    if (m_pCGNode)
    {
        m_pCGNode->m_pSuperviewCAView = NULL;
        m_pCGNode->release();
    }
    //CCLog("~CAView = %d\n", --viewCount);
}

CAView * CAView::create(void)
{
	CAView * pRet = new CAView();
    if (pRet && pRet->init())
    {
        pRet->autorelease();
    }
    else
    {
        CC_SAFE_DELETE(pRet);
    }
	return pRet;
}

bool CAView::init()
{
    this->setImage(CAImage::CC_WHITE_IMAGE());
    return true;
}

CAView* CAView::createWithFrame(const DRect& rect)
{
	CAView * pRet = new CAView();
    if (pRet && pRet->initWithFrame(rect))
    {
        pRet->autorelease();
    }
    else
    {
        CC_SAFE_DELETE(pRet);
    }
	return pRet;
}

CAView* CAView::createWithFrame(const DRect& rect, const CAColor4B& color4B)
{
	CAView * pRet = new CAView();
    if (pRet && pRet->initWithFrame(rect))
    {
        pRet->setColor(color4B);
        pRet->autorelease();
    }
    else
    {
        CC_SAFE_DELETE(pRet);
    }
	return pRet;
}

CAView* CAView::createWithCenter(const DRect& rect)
{
	CAView * pRet = new CAView();
    if (pRet && pRet->initWithCenter(rect))
    {
        pRet->autorelease();
    }
    else
    {
        CC_SAFE_DELETE(pRet);
    }
	return pRet;
}

CAView* CAView::createWithCenter(const DRect& rect, const CAColor4B& color4B)
{
    CAView * pRet = new CAView();
    if (pRet && pRet->initWithCenter(rect))
    {
        pRet->setColor(color4B);
        pRet->autorelease();
    }
    else
    {
        CC_SAFE_DELETE(pRet);
    }
	return pRet;
}

CAView* CAView::createWithLayout(const DLayout& layout)
{
    CAView * pRet = new CAView();
    if (pRet && pRet->initWithLayout(layout))
    {
        pRet->autorelease();
    }
    else
    {
        CC_SAFE_DELETE(pRet);
    }
    return pRet;
}

CAView* CAView::createWithLayout(const DLayout& layout, const CAColor4B& color4B)
{
    CAView * pRet = new CAView();
    if (pRet && pRet->initWithLayout(layout))
    {
        pRet->setColor(color4B);
        pRet->autorelease();
    }
    else
    {
        CC_SAFE_DELETE(pRet);
    }
    return pRet;
}

CAView* CAView::createWithColor(const CAColor4B& color4B)
{
   	CAView * pRet = new CAView();
    if (pRet && pRet->initWithColor(color4B))
    {
        pRet->autorelease();
    }
    else
    {
        CC_SAFE_DELETE(pRet);
    }
	return pRet;
}

bool CAView::initWithColor(const CAColor4B& color4B)
{
    if (!this->init())
    {
        return false;
    }
    this->setColor(color4B);
    
    return true;
}

bool CAView::initWithFrame(const DRect& rect)
{
    if (!this->init())
    {
        return false;
    }
    this->setFrame(rect);
    
    return true;
}

bool CAView::initWithCenter(const DRect& rect)
{
    if (!this->init())
    {
        return false;
    }
    this->setCenter(rect);
    
    return true;
}

bool CAView::initWithLayout(const CrossApp::DLayout &layout)
{
    if (!this->init())
    {
        return false;
    }
    this->setLayout(layout);
    
    return true;
}

float CAView::getSkewX()
{
    return m_fSkewX;
}

void CAView::setSkewX(float newSkewX)
{
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setSkewX(newSkewX, this);
    }
    else if (m_fSkewX != newSkewX)
    {
        m_fSkewX = newSkewX;
        this->updateDraw();
    }
}

float CAView::getSkewY()
{
    return m_fSkewY;
}

void CAView::setSkewY(float newSkewY)
{
    if (CAViewAnimation::areAnimationsEnabled()
         && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setSkewY(newSkewY, this);
    }
    else if (m_fSkewY != newSkewY)
    {
        m_fSkewY = newSkewY;
        this->updateDraw();
    }
}

/// zOrder getter
int CAView::getZOrder()
{
    return m_nZOrder;
}

/// zOrder setter : private method
/// used internally to alter the zOrder variable. DON'T call this method manually
void CAView::_setZOrder(int z)
{
    m_nZOrder = z;
}

void CAView::setZOrder(int z)
{
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setZOrder(z, this);
    }
    else
    {
        if (m_pSuperview)
        {
            m_pSuperview->reorderSubview(this, z);
        }
        else
        {
            this->_setZOrder(z);
        }
    }
}

/// vertexZ getter
float CAView::getVertexZ()
{
    return m_fVertexZ;
}


/// vertexZ setter
void CAView::setVertexZ(float var)
{
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setVertexZ(var, this);
    }
    else if(m_fVertexZ != var)
    {
        m_fVertexZ = var;
        this->updateDraw();
    }
}


/// rotation getter
int CAView::getRotation()
{
    return m_fRotationZ;
}

/// rotation setter
void CAView::setRotation(int newRotation)
{
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setRotation(newRotation, this);
    }
    else if (m_fRotationZ != newRotation)
    {
        m_fRotationZ = newRotation;
        this->updateRotationQuat();
        this->updateDraw();
    }
}

int CAView::getRotationX()
{
    return m_fRotationX;
}

void CAView::setRotationX(int fRotationX)
{
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setRotationX(fRotationX, this);
    }
    else if (m_fRotationX != fRotationX)
    {
        m_fRotationX = fRotationX;
        this->updateRotationQuat();
        this->updateDraw();
    }
}

int CAView::getRotationY()
{
    return m_fRotationY;
}

void CAView::setRotationY(int fRotationY)
{
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setRotationY(fRotationY, this);
    }
    else if (m_fRotationY != fRotationY)
    {
        m_fRotationY = fRotationY;
        this->updateRotationQuat();
        this->updateDraw();
    }
}

void CAView::updateRotationQuat()
{
    // convert Euler angle to quaternion
    // when m_fRotationZ_X == m_fRotationZ_Y, m_fRotationQuat = RotationZ_X * RotationY * RotationX
    // when m_fRotationZ_X != m_fRotationZ_Y, m_fRotationQuat = RotationY * RotationX
    float halfRadx = CC_DEGREES_TO_RADIANS(m_fRotationX / 2.f);
    float halfRady = CC_DEGREES_TO_RADIANS(m_fRotationY / 2.f);
    float halfRadz = -CC_DEGREES_TO_RADIANS(m_fRotationZ / 2.f);
    
    float coshalfRadx = cosf(halfRadx), sinhalfRadx = sinf(halfRadx), coshalfRady = cosf(halfRady), sinhalfRady = sinf(halfRady), coshalfRadz = cosf(halfRadz), sinhalfRadz = sinf(halfRadz);
    
    m_obRotationQuat.x = sinhalfRadx * coshalfRady * coshalfRadz - coshalfRadx * sinhalfRady * sinhalfRadz;
    m_obRotationQuat.y = coshalfRadx * sinhalfRady * coshalfRadz + sinhalfRadx * coshalfRady * sinhalfRadz;
    m_obRotationQuat.z = coshalfRadx * coshalfRady * sinhalfRadz - sinhalfRadx * sinhalfRady * coshalfRadz;
    m_obRotationQuat.w = coshalfRadx * coshalfRady * coshalfRadz + sinhalfRadx * sinhalfRady * sinhalfRadz;
}

/// scale getter
float CAView::getScale(void)
{
    CCAssert( m_fScaleX == m_fScaleY, "CAView#scale. ScaleX != ScaleY. Don't know which one to return");
    return m_fScaleX;
}

/// scale setter
void CAView::setScale(float scale)
{
    this->setScale(scale, scale);
}

/// scale setter
void CAView::setScale(float fScaleX, float fScaleY)
{
    this->setScaleX(fScaleX);
    this->setScaleY(fScaleY);
}

/// scaleX getter
float CAView::getScaleX()
{
    return m_fScaleX;
}

/// scaleX setter
void CAView::setScaleX(float newScaleX)
{
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setScaleX(newScaleX, this);
    }
    else if (m_fScaleX != newScaleX)
    {
        m_fScaleX = newScaleX;
        this->updateDraw();
    }
}

/// scaleY getter
float CAView::getScaleY()
{
    return m_fScaleY;
}

/// scaleY setter
void CAView::setScaleY(float newScaleY)
{
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setScaleY(newScaleY, this);
    }
    else if (m_fScaleY != newScaleY)
    {
        m_fScaleY = newScaleY;
        this->updateDraw();
    }
}

/// position setter
void CAView::setPoint(const DPoint& newPoint)
{
    m_obPoint = newPoint;
    this->updateDraw();
}

/// children getter
const CAVector<CAView*>& CAView::getSubviews()
{
    return m_obSubviews;
}

unsigned int CAView::getSubviewsCount(void) const
{
    return (unsigned int)m_obSubviews.size();
}

/// camera getter: lazy alloc
CACamera* CAView::getCamera()
{
    if (!m_pCamera)
    {
        m_pCamera = new CACamera();
    }
    
    return m_pCamera;
}

/// isVisible getter
bool CAView::isVisible()
{
    return m_bVisible;
}

/// isVisible setter
void CAView::setVisible(bool var)
{
    m_bVisible = var;
    this->updateDraw();
}

const DPoint& CAView::getAnchorPointInPoints()
{
    return m_obAnchorPointInPoints;
}

/// anchorPoint getter
const DPoint& CAView::getAnchorPoint()
{
    return m_obAnchorPoint;
}

void CAView::setAnchorPointInPoints(const DPoint& anchorPointInPoints)
{
    if (!m_obContentSize.equals(DPointZero))
    {
        DPoint anchorPoint = DPoint(anchorPointInPoints.x / m_obContentSize.width,
                                    anchorPointInPoints.y / m_obContentSize.height);

        if (m_bRunning)
        {
            this->getViewToSuperviewTransform();
        }
        
        m_obAnchorPoint = anchorPoint;
        m_obAnchorPointInPoints = anchorPointInPoints;
        
        if (m_bRunning)
        {
            DPoint point = anchorPointInPoints;
            point.y = m_obContentSize.height - point.y;
            point = PointApplyAffineTransform(point, this->getViewToSuperviewAffineTransform());
            point.y = this->m_pSuperview->m_obContentSize.height - point.y;
            this->setPoint(point);
            this->updateDraw();
        }
    }
}

void CAView::setAnchorPoint(const DPoint& anchorPoint)
{
    if( ! anchorPoint.equals(m_obAnchorPoint))
    {
        DPoint anchorPointInPoints = ccpCompMult(m_obContentSize, anchorPoint);
        
        if (m_bRunning)
        {
            this->getViewToSuperviewTransform();
        }
        
        m_obAnchorPoint = anchorPoint;
        m_obAnchorPointInPoints = anchorPointInPoints;
        
        if (m_bRunning)
        {
            DPoint point = anchorPointInPoints;
            point.y = m_obContentSize.height - point.y;
            point = PointApplyAffineTransform(point, this->getViewToSuperviewAffineTransform());
            point.y = this->m_pSuperview->m_obContentSize.height - point.y;
            this->setPoint(point);
            this->updateDraw();
        }
    }
}

void CAView::setContentSize(const DSize & contentSize)
{
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setContentSize(contentSize, this);
    }
    else if (!contentSize.equals(m_obContentSize))
    {
        m_obContentSize = contentSize;
        m_obAnchorPointInPoints.x = m_obContentSize.width * m_obAnchorPoint.x;
        m_obAnchorPointInPoints.y = m_obContentSize.height * m_obAnchorPoint.y;
        
        this->updateImageRect();
        if (m_pContentContainer)
        {
            m_pContentContainer->viewOnSizeTransitionDidChanged();
        }
        
        CAVector<CAView*>::iterator itr;
        for (itr=m_obSubviews.begin(); itr!=m_obSubviews.end(); itr++)
        {
            (*itr)->reViewlayout(m_obContentSize);
        }
        
        this->updateDraw();
    }
}

const DRect& CAView::getFrame()
{
    m_obReturn.setType(DRect::Frame);
    m_obReturn.origin = ccpSub(m_obPoint, m_obAnchorPointInPoints);
    m_obReturn.size = m_obContentSize;
    return m_obReturn;
}

void CAView::setFrame(const DRect &rect)
{
    m_obLayout = DLayoutZero;
    DSize originalSize = m_obContentSize;
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setContentSize(rect.size, this);
        
        m_obContentSize = rect.size;
        m_obAnchorPointInPoints = m_obContentSize;
        m_obAnchorPointInPoints.x *= m_obAnchorPoint.x;
        m_obAnchorPointInPoints.y *= m_obAnchorPoint.y;
    }
    else
    {
        this->setContentSize(rect.size);
    }
    
    this->setFrameOrigin(rect.origin);
    
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        m_obContentSize = originalSize;
        m_obAnchorPointInPoints = m_obContentSize;
        m_obAnchorPointInPoints.x *= m_obAnchorPoint.x;
        m_obAnchorPointInPoints.y *= m_obAnchorPoint.y;
    }
}

const DPoint& CAView::getFrameOrigin()
{
    m_obReturn.origin = ccpSub(m_obPoint, m_obAnchorPointInPoints);
    return m_obReturn.origin;
}

void CAView::setFrameOrigin(const DPoint& point)
{
    m_obLayout = DLayoutZero;
    DPoint p = ccpAdd(point, m_obAnchorPointInPoints);
    
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setPoint(p, this);
    }
    else
    {
        this->setPoint(p);
    }
    m_eLayoutType = 0;
}

const DRect& CAView::getCenter()
{
    m_obReturn.setType(DRect::Center);
    m_obReturn.origin = ccpAdd(ccpSub(m_obPoint, m_obAnchorPointInPoints),
                           ccpMult(m_obContentSize, 0.5f));
    m_obReturn.size = m_obContentSize;
    return m_obReturn;
}

void CAView::setCenter(const DRect& rect)
{
    m_obLayout = DLayoutZero;
    DSize originalSize = m_obContentSize;
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setContentSize(rect.size, this);
        
        m_obContentSize = rect.size;
        m_obAnchorPointInPoints = m_obContentSize;
        m_obAnchorPointInPoints.x *= m_obAnchorPoint.x;
        m_obAnchorPointInPoints.y *= m_obAnchorPoint.y;
    }
    else
    {
        this->setContentSize(rect.size);
    }
    
    this->setCenterOrigin(rect.origin);
    
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        m_obContentSize = originalSize;
        m_obAnchorPointInPoints = m_obContentSize;
        m_obAnchorPointInPoints.x *= m_obAnchorPoint.x;
        m_obAnchorPointInPoints.y *= m_obAnchorPoint.y;
    }
}

const DPoint& CAView::getCenterOrigin()
{
    m_obReturn.origin = ccpAdd(ccpSub(m_obPoint, m_obAnchorPointInPoints),
                  ccpMult(m_obContentSize, 0.5f));
    return m_obReturn.origin;
}

void CAView::setCenterOrigin(const DPoint& point)
{
    m_obLayout = DLayoutZero;
    DPoint p = ccpSub(point, ccpMult(m_obContentSize, 0.5f));
    p = ccpAdd(p, m_obAnchorPointInPoints);

    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setPoint(p, this);
    }
    else
    {
        this->setPoint(p);
    }
    m_eLayoutType = 1;
}

const DRect& CAView::getBounds()
{
    m_obReturn.setType(DRect::Frame);
    m_obReturn.origin = DPointZero;
    m_obReturn.size = m_obContentSize;
    return m_obReturn;
}

void CAView::setBounds(const DRect& rect)
{
    if (!rect.size.equals(DSizeZero))
    {
        this->setContentSize(rect.size);
    }
}

void CAView::setLayout(const CrossApp::DLayout &layout)
{
    m_obLayout = layout;
    m_eLayoutType = 2;
    
    if (m_bRunning)
    {
        if (m_pSuperview)
        {
            this->reViewlayout(m_pSuperview->m_obContentSize, true);
        }
        else if (m_pParentCGNode)
        {
            this->reViewlayout(m_pParentCGNode->m_obContentSize);
        }
    }
}

const DLayout& CAView::getLayout()
{
    return m_obLayout;
}

// isRunning getter
bool CAView::isRunning()
{
    return m_bRunning;
}

/// Superview getter
CAView * CAView::getSuperview()
{
    return m_pSuperview;
}
/// Superview setter
void CAView::setSuperview(CrossApp::CAView * superview)
{
    m_pSuperview = superview;
}

unsigned int CAView::getOrderOfArrival()
{
    return m_uOrderOfArrival;
}

void CAView::setOrderOfArrival(unsigned int uOrderOfArrival)
{
    m_uOrderOfArrival = uOrderOfArrival;
}

ccGLServerState CAView::getGLServerState()
{
    return m_eGLServerState;
}

void CAView::setGLServerState(ccGLServerState glServerState)
{
    m_eGLServerState = glServerState;
}


CAGLProgram* CAView::getShaderProgram()
{
    return m_pShaderProgram;
}

void CAView::setShaderProgram(CAGLProgram *pShaderProgram)
{
    CC_SAFE_RETAIN(pShaderProgram);
    CC_SAFE_RELEASE(m_pShaderProgram);
    m_pShaderProgram = pShaderProgram;
}

void CAView::enabledLeftShadow(bool var)
{
    m_bLeftShadowed = var;
}

void CAView::enabledRightShadow(bool var)
{
    m_bRightShadowed = var;
}

void CAView::enabledTopShadow(bool var)
{
    m_bTopShadowed = var;
}

void CAView::enabledBottomShadow(bool var)
{
    m_bBottomShadowed = var;
}

const char* CAView::description()
{
    return crossapp_format_string("<CAView | TextTag = %s | Tag = %d >", m_sTextTag.c_str(), m_nTag).c_str();
}

void CAView::reViewlayout(const DSize& contentSize, bool allowAnimation)
{
    if (m_eLayoutType == 2)
    {
        DPoint point;
        DSize size;
        
        {
            const DHorizontalLayout& horizontalLayout = m_obLayout.horizontal;
            
            if (horizontalLayout.left < FLOAT_NONE && horizontalLayout.right < FLOAT_NONE)
            {
                size.width = contentSize.width - horizontalLayout.left - horizontalLayout.right;
                point.x = horizontalLayout.left;
            }
            else if (horizontalLayout.left < FLOAT_NONE && horizontalLayout.width < FLOAT_NONE)
            {
                size.width = horizontalLayout.width;
                point.x = horizontalLayout.left;
            }
            else if (horizontalLayout.right < FLOAT_NONE && horizontalLayout.width < FLOAT_NONE)
            {
                size.width = horizontalLayout.width;
                point.x = contentSize.width - horizontalLayout.right - size.width;
            }
            else if (horizontalLayout.width < FLOAT_NONE && horizontalLayout.center < FLOAT_NONE)
            {
                size.width = horizontalLayout.width;
                point.x = contentSize.width * horizontalLayout.center - size.width / 2;
            }
            else if (horizontalLayout.normalizedWidth < FLOAT_NONE && horizontalLayout.center < FLOAT_NONE)
            {
                size.width = contentSize.width * horizontalLayout.normalizedWidth;
                point.x = contentSize.width * horizontalLayout.center - size.width / 2;
            }
        }
        
        {
            const DVerticalLayout& verticalLayout = m_obLayout.vertical;
            
            if (verticalLayout.top < FLOAT_NONE && verticalLayout.bottom < FLOAT_NONE)
            {
                size.height = contentSize.height - verticalLayout.top - verticalLayout.bottom;
                point.y = verticalLayout.top;
            }
            else if (verticalLayout.top < FLOAT_NONE && verticalLayout.height < FLOAT_NONE)
            {
                size.height = verticalLayout.height;
                point.y = verticalLayout.top;
            }
            else if (verticalLayout.bottom < FLOAT_NONE && verticalLayout.height < FLOAT_NONE)
            {
                size.height = verticalLayout.height;
                point.y = contentSize.height - verticalLayout.bottom - size.height;
            }
            else if (verticalLayout.height < FLOAT_NONE && verticalLayout.center < FLOAT_NONE)
            {
                size.height = verticalLayout.height;
                point.y = contentSize.height * verticalLayout.center - size.height / 2;
            }
            else if (verticalLayout.normalizedHeight < FLOAT_NONE && verticalLayout.center < FLOAT_NONE)
            {
                size.height = contentSize.height * verticalLayout.normalizedHeight;
                point.y = contentSize.height * verticalLayout.center - size.height / 2;
            }
        }
        
        if (allowAnimation
            && CAViewAnimation::areAnimationsEnabled()
            && CAViewAnimation::areBeginAnimations())
        {
            CAViewAnimation::getInstance()->setContentSize(size, this);
        }
        else
        {
            this->setContentSize(size);
        }
        
        DPoint p = ccpCompMult(size, m_obAnchorPoint);;
        p = ccpAdd(p, point);
        
        if (allowAnimation
            && CAViewAnimation::areAnimationsEnabled()
            && CAViewAnimation::areBeginAnimations())
        {
            CAViewAnimation::getInstance()->setPoint(p, this);
        }
        else
        {
            this->setPoint(p);
        }
    }
    else
    {
        this->updateDraw();
    }
}

void CAView::updateDraw()
{
    CAView* v = m_pSuperview;
    CC_RETURN_IF(v == NULL);
    while (v == v->getSuperview())
    {
        CC_BREAK_IF(v == NULL);
        CC_RETURN_IF(v->isVisible());
    }
    SET_DIRTY_RECURSIVELY(m_pobBatchView);
    m_bTransformDirty = m_bInverseDirty = true;
    CAApplication::getApplication()->updateDraw();
}

CAView* CAView::getSubviewByTag(int aTag)
{
    CCAssert( aTag != TagInvalid, "Invalid tag");
    
    for (auto& subview : m_obSubviews)
    {
        if (subview->m_nTag == aTag)
        {
            return subview;
        }
    }
    return NULL;
}

CAView* CAView::getSubviewByTextTag(const std::string& textTag)
{
    CCAssert( !textTag.empty(), "Invalid tag");
    
    for (auto& subview : m_obSubviews)
    {
        if (subview->m_sTextTag.compare(textTag) == 0)
        {
            return subview;
        }
    }
    return NULL;
}

void CAView::setTag(int tag)
{
    this->m_nTag = tag;
}

int CAView::getTag()
{
    return this->m_nTag;
}

void CAView::addSubview(CAView *subview)
{
    this->insertSubview(subview, subview->getZOrder());
}

// helper used by reorderChild & add
void CAView::insertSubview(CAView* subview, int z)
{
    CCAssert( subview != NULL, "Argument must be non-nil");
    CCAssert( subview->m_pSuperview == NULL, "child already added. It can't be added again");
    
    if (m_pobBatchView)
    {
        m_pobBatchView->appendSubview(subview);
        
        if (!m_bReorderChildDirty)
        {
            setReorderSubviewDirtyRecursively();
        }
    }
    
    m_bReorderChildDirty = true;
    m_obSubviews.pushBack(subview);
    subview->_setZOrder(z);
    
    subview->setSuperview(this);
    subview->setOrderOfArrival(s_globalOrderOfArrival++);
    
    if(m_bRunning)
    {
        subview->onEnter();
        subview->onEnterTransitionDidFinish();
    }
}

void CAView::removeFromSuperview()
{
    if (m_pSuperview != NULL)
    {
        m_pSuperview->removeSubview(this);
    }
}

void CAView::removeSubview(CAView* subview)
{
    if (m_pobBatchView)
    {
        m_pobBatchView->removeViewFromAtlas(subview);
    }
    
    if (m_obSubviews.contains(subview))
    {
        this->detachSubview(subview);
    }
}

void CAView::removeSubviewByTag(int tag)
{
    CCAssert( tag != TagInvalid, "Invalid tag");
    
    CAView *subview = this->getSubviewByTag(tag);
    
    if (subview == NULL)
    {
        CCLOG("CrossApp: removeSubviewByTag(tag = %d): child not found!", tag);
    }
    else
    {
        this->removeSubview(subview);
    }
}

void CAView::removeSubviewByTextTag(const std::string& textTag)
{
    CCAssert( !textTag.empty(), "Invalid tag");
    
    CAView *subview = this->getSubviewByTextTag(textTag);
    
    if (subview == NULL)
    {
        CCLOG("CrossApp: removeSubviewByTextTag(textTag = %s): child not found!", textTag.c_str());
    }
    else
    {
        this->removeSubview(subview);
    }
}

void CAView::removeAllSubviews()
{
    // not using detachSubview improves speed here
    if (!m_obSubviews.empty())
    {
        if (m_pobBatchView)
        {
            for (auto& subview : m_obSubviews)
            {
                m_pobBatchView->removeViewFromAtlas(subview);
            }
        }

        for (auto& subview : m_obSubviews)
        {
            if(m_bRunning)
            {
                subview->onExitTransitionDidStart();
                subview->onExit();
            }
            
            subview->setSuperview(NULL);
        }
        m_obSubviews.clear();
    }

    m_bHasChildren = false;
}


void CAView::detachSubview(CAView *subview)
{
    // IMPORTANT:
    //  -1st do onExit
    //  -2nd cleanup
    
    if (m_bRunning)
    {
        subview->onExitTransitionDidStart();
        subview->onExit();
    }
    
    // set Superview nil at the end
    subview->setSuperview(NULL);
    
    m_obSubviews.eraseObject(subview);
    
    this->updateDraw();
}

void CAView::reorderSubview(CAView *subview, int zOrder)
{
    if (zOrder == subview->getZOrder())
    {
        return;
    }
    
    if( m_pobBatchView && ! m_bReorderChildDirty)
    {
        setReorderSubviewDirtyRecursively();
        m_pobBatchView->reorderBatch(true);
    }
    
    CCAssert( subview != NULL, "Subview must be non-nil");
    m_bReorderChildDirty = true;
    subview->setOrderOfArrival(s_globalOrderOfArrival++);
    subview->_setZOrder(zOrder);
    this->updateDraw();
}

void CAView::sortAllSubviews()
{
    if (m_bReorderChildDirty && !m_obSubviews.empty())
    {
        std::sort(m_obSubviews.begin(), m_obSubviews.end(), compareSubviewZOrder);
    
        if (m_pobBatchView)
        {
            for (auto& subview : m_obSubviews)
            {
                if(m_bRunning) subview->sortAllSubviews();
            }
        }
        
        m_bReorderChildDirty = false;
    }
}

void CAView::draw()
{
    m_uZLevel = CAApplication::getApplication()->getCurrentNumberOfDraws();
    
    CC_RETURN_IF(m_pobImage == NULL);
    CC_RETURN_IF(m_pShaderProgram == NULL);
    
    CAIMAGE_DRAW_SETUP();
    
    ccGLBlendFunc(m_sBlendFunc.src, m_sBlendFunc.dst);
    ccGLBindTexture2D(m_pobImage->getName());
    ccGLEnableVertexAttribs(kCCVertexAttribFlag_PosColorTex);
    
#define kQuadSize sizeof(m_sQuad.bl)
    
    long offset = (long)&m_sQuad;
    
    // vertex
    int diff = offsetof( ccV3F_C4B_T2F, vertices);
    glVertexAttribPointer(kCCVertexAttrib_Position,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          kQuadSize,
                          (void*) (offset + diff));
    
    // texCoods
    diff = offsetof( ccV3F_C4B_T2F, texCoords);
    glVertexAttribPointer(kCCVertexAttrib_TexCoords,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          kQuadSize,
                          (void*) (offset + diff));
    
    // color
    diff = offsetof( ccV3F_C4B_T2F, colors);
    glVertexAttribPointer(kCCVertexAttrib_Color,
                          4,
                          GL_UNSIGNED_BYTE,
                          GL_TRUE,
                          kQuadSize,
                          (void*)(offset + diff));
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        
#if CC_SPRITE_DEBUG_DRAW
    // draw bounding box
    DPoint vertices[4]=
    {
        DPoint(m_sQuad.tl.vertices.x, m_sQuad.tl.vertices.y),
        DPoint(m_sQuad.bl.vertices.x, m_sQuad.bl.vertices.y),
        DPoint(m_sQuad.br.vertices.x, m_sQuad.br.vertices.y),
        DPoint(m_sQuad.tr.vertices.x, m_sQuad.tr.vertices.y),
    };
    ccDrawPoly(vertices, 4, true);
#endif // CC_SPRITE_DEBUG_DRAW

    CC_PROFILER_STOP_CATEGORY(kCCProfilerCategorySprite, "CAView - draw");
}

void CAView::drawShadow(CAImage* i, const ccV3F_C4B_T2F_Quad& q)
{
    kmGLPushMatrix();
    
    CAIMAGE_DRAW_SETUP();
    
    ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ccGLBindTexture2D(i->getName());
    ccGLEnableVertexAttribs(kCCVertexAttribFlag_PosColorTex);

    long offset = (long)&q;
    // vertex
    int diff = offsetof( ccV3F_C4B_T2F, vertices);
    glVertexAttribPointer(kCCVertexAttrib_Position,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(q.bl),
                          (void*) (offset + diff));
    
    // texCoods
    diff = offsetof( ccV3F_C4B_T2F, texCoords);
    glVertexAttribPointer(kCCVertexAttrib_TexCoords,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(q.bl),
                          (void*) (offset + diff));
    
    // color
    diff = offsetof( ccV3F_C4B_T2F, colors);
    glVertexAttribPointer(kCCVertexAttrib_Color,
                          4,
                          GL_UNSIGNED_BYTE,
                          GL_TRUE,
                          sizeof(q.bl),
                          (void*)(offset + diff));
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    kmGLPopMatrix();
}

void CAView::drawLeftShadow()
{
    if (m_bLeftShadowed)
    {
        ccV3F_C4B_T2F_Quad quad = m_sQuad;
        
        GLfloat x1,x2,y1,y2;
        x1 = -12;
        y1 = 0;
        x2 = 0;
        y2 = m_obContentSize.height;
        
        quad.bl.vertices = DPoint3D(x1, y1, m_fVertexZ);
        quad.br.vertices = DPoint3D(x2, y1, m_fVertexZ);
        quad.tl.vertices = DPoint3D(x1, y2, m_fVertexZ);
        quad.tr.vertices = DPoint3D(x2, y2, m_fVertexZ);
        
        quad.bl.texCoords.u = quad.tl.texCoords.u = quad.tl.texCoords.v = quad.tr.texCoords.v = 0;
        quad.bl.texCoords.v = quad.br.texCoords.u = quad.br.texCoords.v = quad.tr.texCoords.u = 1;

        quad.bl.colors = quad.br.colors = quad.tl.colors = quad.tr.colors = CAColor_white;
        
        this->drawShadow(CAImage::CC_SHADOW_LEFT_IMAGE(), quad);
    }

}

void CAView::drawRightShadow()
{
    if (m_bRightShadowed)
    {
        ccV3F_C4B_T2F_Quad quad = m_sQuad;
        
        GLfloat x1,x2,y1,y2;
        x1 = m_obContentSize.width;
        y1 = 0;
        x2 = m_obContentSize.width + 12;
        y2 = m_obContentSize.height;
        
        quad.bl.vertices = DPoint3D(x1, y1, m_fVertexZ);
        quad.br.vertices = DPoint3D(x2, y1, m_fVertexZ);
        quad.tl.vertices = DPoint3D(x1, y2, m_fVertexZ);
        quad.tr.vertices = DPoint3D(x2, y2, m_fVertexZ);
        
        quad.bl.texCoords.u = quad.tl.texCoords.u = quad.tl.texCoords.v = quad.tr.texCoords.v = 0;
        quad.bl.texCoords.v = quad.br.texCoords.u = quad.br.texCoords.v = quad.tr.texCoords.u = 1;
        
        quad.bl.colors = quad.br.colors = quad.tl.colors = quad.tr.colors = CAColor_white;
        
        this->drawShadow(CAImage::CC_SHADOW_RIGHT_IMAGE(), quad);
    }
    
}

void CAView::drawTopShadow()
{
    if (m_bTopShadowed)
    {
        ccV3F_C4B_T2F_Quad quad = m_sQuad;
        
        GLfloat x1,x2,y1,y2;
        x1 = 0;
        y1 = m_obContentSize.height;
        x2 = m_obContentSize.width;
        y2 = m_obContentSize.height + 6;
        
        quad.bl.vertices = DPoint3D(x1, y1, m_fVertexZ);
        quad.br.vertices = DPoint3D(x2, y1, m_fVertexZ);
        quad.tl.vertices = DPoint3D(x1, y2, m_fVertexZ);
        quad.tr.vertices = DPoint3D(x2, y2, m_fVertexZ);
        
        quad.bl.texCoords.u = quad.tl.texCoords.u = quad.tl.texCoords.v = quad.tr.texCoords.v = 0;
        quad.bl.texCoords.v = quad.br.texCoords.u = quad.br.texCoords.v = quad.tr.texCoords.u = 1;
        
        quad.bl.colors = quad.br.colors = quad.tl.colors = quad.tr.colors = CAColor_white;
        
        this->drawShadow(CAImage::CC_SHADOW_TOP_IMAGE(), quad);
    }
    
}

void CAView::drawBottomShadow()
{
    if (m_bBottomShadowed)
    {
        ccV3F_C4B_T2F_Quad quad = m_sQuad;
        
        GLfloat x1,x2,y1,y2;
        x1 = 0;
        y1 = -6;
        x2 = m_obContentSize.width;
        y2 = 0;
        
        quad.bl.vertices = DPoint3D(x1, y1, m_fVertexZ);
        quad.br.vertices = DPoint3D(x2, y1, m_fVertexZ);
        quad.tl.vertices = DPoint3D(x1, y2, m_fVertexZ);
        quad.tr.vertices = DPoint3D(x2, y2, m_fVertexZ);
        
        quad.bl.texCoords.u = quad.tl.texCoords.u = quad.tl.texCoords.v = quad.tr.texCoords.v = 0;
        quad.bl.texCoords.v = quad.br.texCoords.u = quad.br.texCoords.v = quad.tr.texCoords.u = 1;
        
        quad.bl.colors = quad.br.colors = quad.tl.colors = quad.tr.colors = CAColor_white;
        
        this->drawShadow(CAImage::CC_SHADOW_BOTTOM_IMAGE(), quad);
    }
    
}

void CAView::visit()
{
    CC_RETURN_IF(!m_bVisible);
    
    kmGLPushMatrix();

    this->transform();
    
    this->drawLeftShadow();
    this->drawRightShadow();
    this->drawTopShadow();
    this->drawBottomShadow();
    
    int minX, maxX, minY, maxY;
    bool isScissor = (bool)glIsEnabled(GL_SCISSOR_TEST);
    if (isScissor)
    {
        GLfloat params[4];
        glGetFloatv(GL_SCISSOR_BOX, params);
        minX = params[0];
        minY = params[1];
        maxX = params[0] + params[2];
        maxY = params[1] + params[3];
    }
    //m_bDisplayRange = 1;
    if (!m_bDisplayRange)
    {
        kmMat4 min;     kmGLGetMatrix(KM_GL_MODELVIEW, &min);
        
        kmMat4 tm;      kmMat4Identity(&tm);
        tm.mat[12]  =   m_obContentSize.width;
        tm.mat[13]  =   m_obContentSize.height;
        
        kmMat4 max;     kmMat4Multiply(&max, &min, &tm);
        
        float minX2 = ceilf(s_dip_to_px(min.mat[12] - 0.9));
        float minY2 = ceilf(s_dip_to_px(min.mat[13] - 0.9));
        float maxX2 = ceilf(s_dip_to_px(max.mat[12] + 0.9));
        float maxY2 = ceilf(s_dip_to_px(max.mat[13] + 0.9));
        
        static CAApplication* application = CAApplication::getApplication();
        if (application->getProjection() == CAApplication::P3D)
        {
            minX2 += s_dip_to_px(application->getWinSize().width/2);
            minY2 += s_dip_to_px(application->getWinSize().height/2);
            maxX2 += s_dip_to_px(application->getWinSize().width/2);
            maxY2 += s_dip_to_px(application->getWinSize().height/2);
        }

        if (isScissor)
        {
            float x1 = MAX(minX2, minX);
            float y1 = MAX(minY2, minY);
            float x2 = MIN(maxX2, maxX);
            float y2 = MIN(maxY2, maxY);
            float width = MAX(x2-x1, 0);
            float height = MAX(y2-y1, 0);
            glScissor(x1, y1, width, height);
        }
        else
        {
            glEnable(GL_SCISSOR_TEST);
            glScissor(minX2,
                      minY2,
                      maxX2-minX2,
                      maxY2-minY2);
        }
    }

    this->sortAllSubviews();
    
    CAVector<CAView*>::iterator itr=m_obSubviews.begin();
    while (itr!=m_obSubviews.end())
    {
        CC_BREAK_IF((*itr)->m_nZOrder >= 0);
        (*itr)->visit();
        itr++;
    }
    
    this->draw();
    
    while (itr!=m_obSubviews.end())
    {
        (*itr)->visit();
        itr++;
    }
    
    if (m_pCGNode)
    {
        m_pCGNode->visit();
    }
    
    //m_uOrderOfArrival = 0;
    
    if (!m_bDisplayRange)
    {
        if (isScissor)
        {
            glScissor(minX,
                      minY,
                      maxX-minX,
                      maxY-minY);
        }
        else
        {
            glDisable(GL_SCISSOR_TEST);
        }
    }

    kmGLPopMatrix();
}

void CAView::visitEve(void)
{
    for (auto& subview : m_obSubviews)
    {
        subview->visitEve();
    }
    
    if (m_pCGNode)
    {
        m_pCGNode->visitEve();
    }
}

CAView* CAView::copy()
{
    CAView* view = NULL;
    if (m_eLayoutType == 0)
    {
        view = CAView::createWithFrame(this->getFrame(), this->getColor());
    }
    else if (m_eLayoutType == 1)
    {
        view = CAView::createWithCenter(this->getCenter(), this->getColor());
    }
    else
    {
        view = CAView::createWithLayout(this->getLayout(), this->getColor());
    }
    view->setImage(this->getImage());
    
    return view;
}

CAResponder* CAView::nextResponder()
{
    if (!m_bHaveNextResponder)
    {
        return NULL;
    }
    
    if (m_pContentContainer)
    {
        return dynamic_cast<CAResponder*>(m_pContentContainer);
    }
    return m_pSuperview;
}

void CAView::onEnter()
{
#if CC_ENABLE_SCRIPT_BINDING
    if(CCScriptEngineManager::sharedManager()->getScriptEngine())
    {
        if (CCScriptEngineManager::sharedManager()->getScriptEngine()->getScriptType()== kScriptTypeJavascript)
        {
            if (CCScriptEngineManager::sendNodeEventToJS(this, kNodeOnEnter))
                return;
        }
    }
#endif
    if (m_pSuperview)
    {
        this->reViewlayout(m_pSuperview->m_obContentSize);
    }
    else if (m_pParentCGNode)
    {
        this->reViewlayout(m_pParentCGNode->m_obContentSize);
    }
    
    m_bRunning = true;
    this->updateDraw();
    
    for (auto& subview : m_obSubviews)
    {
        subview->onEnter();
    }
    
    if (m_pCGNode)
    {
        m_pCGNode->onEnter();
    }
}

void CAView::onEnterTransitionDidFinish()
{
#if CC_ENABLE_SCRIPT_BINDING
    if(CCScriptEngineManager::sharedManager()->getScriptEngine())
    {
        if (CCScriptEngineManager::sharedManager()->getScriptEngine()->getScriptType()== kScriptTypeJavascript)
        {
            if (CCScriptEngineManager::sendNodeEventToJS(this, kNodeOnEnterTransitionDidFinish))
                return;
        }
    }
#endif
    if (!m_obSubviews.empty())
    {
        CAVector<CAView*>::iterator itr;
        for (itr=m_obSubviews.begin(); itr!=m_obSubviews.end(); itr++)
            (*itr)->onEnterTransitionDidFinish();
    }
    
    if (m_pCGNode)
    {
        m_pCGNode->onEnterTransitionDidFinish();
    }
    
    if (m_pContentContainer)
    {
        m_pContentContainer->viewOnEnterTransitionDidFinish();
    }
}

void CAView::onExitTransitionDidStart()
{
#if CC_ENABLE_SCRIPT_BINDING
    if(CCScriptEngineManager::sharedManager()->getScriptEngine())
    {
        if (CCScriptEngineManager::sharedManager()->getScriptEngine()->getScriptType()== kScriptTypeJavascript)
        {
            if (CCScriptEngineManager::sendNodeEventToJS(this, kNodeOnExitTransitionDidStart))
                return;
        }
    }
#endif
    
    for (auto& subview : m_obSubviews)
    {
        subview->onExitTransitionDidStart();
    }
    
    if (m_pCGNode)
    {
        m_pCGNode->onExitTransitionDidStart();
    }
    
    if (m_pContentContainer)
    {
        m_pContentContainer->viewOnExitTransitionDidStart();
    }
}

void CAView::onExit()
{
#if CC_ENABLE_SCRIPT_BINDING
    if(CCScriptEngineManager::sharedManager()->getScriptEngine())
    {
        if (CCScriptEngineManager::sharedManager()->getScriptEngine()->getScriptType()== kScriptTypeJavascript)
        {
            if (CCScriptEngineManager::sendNodeEventToJS(this, kNodeOnExit))
                return;
        }
    }
#endif
    m_bRunning = false;
    
    for (auto& subview : m_obSubviews)
    {
        subview->onExit();
    }
    
    if (m_pCGNode)
    {
        m_pCGNode->onExit();
    }
}

// override me
void CAView::update(float fDelta)
{

}

/////
AffineTransform CAView::getViewToSuperviewAffineTransform() const
{
    AffineTransform ret;
    GLToCGAffine(getViewToSuperviewTransform().m.mat, &ret);
    
    return ret;
}


Mat4 CAView::getViewToSuperviewTransform(CAView* ancestor) const
{
    Mat4 t(this->getViewToSuperviewTransform());

    CAView *s = m_pSuperview;
    if (s)
    {
        while (s)
        {
            t = s->getViewToSuperviewTransform() * t;
            CC_BREAK_IF(!s->getSuperview());
            s = s->getSuperview();
            CC_BREAK_IF(s == ancestor);
        }
        
        if (s && s != ancestor)
        {
            if (CGNode *p = s->m_pParentCGNode)
            {
                t = p->getNodeToParentTransform(nullptr) * t;
            }
        }
    }
    else
    {
        if (CGNode *p = m_pParentCGNode)
        {
            t = p->getNodeToParentTransform(nullptr) * t;
        }
    }

    return t;
}

AffineTransform CAView::getViewToSuperviewAffineTransform(CAView* ancestor) const
{
    AffineTransform t(this->getViewToSuperviewAffineTransform());
    
    CAView *s = m_pSuperview;
    if (s)
    {
        while (s)
        {
            t = AffineTransformConcat(t, s->getViewToSuperviewAffineTransform());
            CC_BREAK_IF(!s->getSuperview());
            s = s->getSuperview();
            CC_BREAK_IF(s == ancestor);
        }
        
        if (s && s != ancestor)
        {
            if (CGNode *p = s->m_pParentCGNode)
            {
                t = AffineTransformConcat(t, p->getNodeToParentAffineTransform(nullptr));
            }
        }
    }
    else
    {
        if (CGNode *p = s->m_pParentCGNode)
        {
            t = AffineTransformConcat(t, p->getNodeToParentAffineTransform(nullptr));
        }
    }
    
    return t;
}

const Mat4& CAView::getViewToSuperviewTransform() const
{
    if (m_bTransformDirty)
    {
        // Translate values
        float height = 0;
        
        if (this->m_pSuperview)
        {
            height = this->m_pSuperview->m_obContentSize.height;
        }
        else if (this->m_pParentCGNode)
        {
            height = this->m_pParentCGNode->m_obContentSize.height;
        }
        else
        {
            height = CAApplication::getApplication()->getWinSize().height;
        }
        
        float x = m_obPoint.x;
        float y = height - m_obPoint.y;
        float z = m_fVertexZ;

        bool needsSkewMatrix = ( m_fSkewX || m_fSkewY );
        
        
        DPoint anchorPoint(m_obAnchorPointInPoints.x * m_fScaleX, (m_obContentSize.height - m_obAnchorPointInPoints.y) * m_fScaleY);
        
        // calculate real position
        if (! needsSkewMatrix && !anchorPoint.equals(DSizeZero))
        {
            x += -anchorPoint.x;
            y += -anchorPoint.y;
        }
        
        // Build Transform Matrix = translation * rotation * scale
        Mat4 translation;
        //move to anchor point first, then rotate
        Mat4::createTranslation(x + anchorPoint.x, y + anchorPoint.y, z, &translation);
        
        Mat4::createRotation(m_obRotationQuat, &m_tTransform);
        
        m_tTransform = translation * m_tTransform;
        //move by (-anchorPoint.x, -anchorPoint.y, 0) after rotation
        m_tTransform.translate(-anchorPoint.x, -anchorPoint.y, 0);
        
        
        if (m_fScaleX != 1.f)
        {
            m_tTransform.m.mat[0] *= m_fScaleX, m_tTransform.m.mat[1] *= m_fScaleX, m_tTransform.m.mat[2] *= m_fScaleX;
        }
        if (m_fScaleY != 1.f)
        {
            m_tTransform.m.mat[4] *= m_fScaleY, m_tTransform.m.mat[5] *= m_fScaleY, m_tTransform.m.mat[6] *= m_fScaleY;
        }
        if (m_fScaleZ != 1.f)
        {
            m_tTransform.m.mat[8] *= m_fScaleZ, m_tTransform.m.mat[9] *= m_fScaleZ, m_tTransform.m.mat[10] *= m_fScaleZ;
        }
        
        // FIXME:: Try to inline skew
        // If skew is needed, apply skew and then anchor point
        if (needsSkewMatrix)
        {
            float skewMatArray[16] =
            {
                1, (float)tanf(CC_DEGREES_TO_RADIANS(m_fSkewY)), 0, 0,
                (float)tanf(CC_DEGREES_TO_RADIANS(m_fSkewX)), 1, 0, 0,
                0,  0,  1, 0,
                0,  0,  0, 1
            };
            Mat4 skewMatrix(skewMatArray);
            
            m_tTransform = m_tTransform * skewMatrix;
            
            // adjust anchor point
            if (!m_obAnchorPointInPoints.equals(DSizeZero))
            {
                // FIXME:: Argh, Mat4 needs a "translate" method.
                // FIXME:: Although this is faster than multiplying a vec4 * mat4
                DPoint anchorPoint(m_obAnchorPointInPoints.x * m_fScaleX, (m_obContentSize.height - m_obAnchorPointInPoints.y) * m_fScaleY);
                m_tTransform.m.mat[12] += m_tTransform.m.mat[0] * -anchorPoint.x + m_tTransform.m.mat[4] * -anchorPoint.y;
                m_tTransform.m.mat[13] += m_tTransform.m.mat[1] * -anchorPoint.x + m_tTransform.m.mat[5] * -anchorPoint.y;
            }
        }
        
        if (m_bUseAdditionalTransform)
        {
            m_tTransform = m_tTransform * m_tAdditionalTransform;
        }
        
        m_bTransformDirty = false;
    }
    
    return m_tTransform;
}

void CAView::setViewToSuperviewTransform(const Mat4& transform)
{
    m_tTransform = transform;
    m_bTransformDirty = false;
    m_bTransformUpdated = true;
}

void CAView::setAdditionalTransform(const AffineTransform& additionalTransform)
{
    Mat4 tmp;
    CGAffineToGL(additionalTransform, tmp.m.mat);
    setAdditionalTransform(&tmp);
}

void CAView::setAdditionalTransform(Mat4* additionalTransform)
{
    if (additionalTransform == nullptr)
    {
        m_bUseAdditionalTransform = false;
    }
    else
    {
        m_tAdditionalTransform = *additionalTransform;
        m_bUseAdditionalTransform = true;
    }
    m_bTransformUpdated = m_bTransformDirty = m_bInverseDirty = true;
}


AffineTransform CAView::getSuperviewToViewAffineTransform() const
{
    AffineTransform ret;
    
    GLToCGAffine(getSuperviewToViewTransform().m.mat,&ret);
    return ret;
}

const Mat4& CAView::getSuperviewToViewTransform() const
{
    if ( m_bInverseDirty )
    {
        m_tInverse = getViewToSuperviewTransform().getInversed();
        m_bInverseDirty = false;
    }
    
    return m_tInverse;
}


AffineTransform CAView::getViewToWorldAffineTransform() const
{
    return this->getViewToSuperviewAffineTransform(nullptr);
}

Mat4 CAView::getViewToWorldTransform() const
{
    return this->getViewToSuperviewTransform(nullptr);
}

AffineTransform CAView::getWorldToViewAffineTransform() const
{
    return AffineTransformInvert(this->getViewToWorldAffineTransform());
}

Mat4 CAView::getWorldToViewTransform() const
{
    return getViewToWorldTransform().getInversed();
}

void CAView::transformAncestors()
{
    if( m_pSuperview != NULL  )
    {
        m_pSuperview->transformAncestors();
        m_pSuperview->transform();
    }
}

void CAView::transform()
{
    kmMat4 transfrom4x4 = this->getViewToSuperviewTransform().m;
    
    kmGLMultMatrix( &transfrom4x4 );
    
    // XXX: Expensive calls. Camera should be integrated into the cached affine matrix
    if ( m_pCamera != NULL)
    {
        DPoint anchorPointInPoints = DPoint(m_obAnchorPointInPoints.x,
                                            m_obContentSize.height - m_obAnchorPointInPoints.y);
        
        bool translate = (anchorPointInPoints.x != 0.0f || anchorPointInPoints.y != 0.0f);
        
        if( translate )
            kmGLTranslatef(RENDER_IN_SUBPIXEL(anchorPointInPoints.x),
                           RENDER_IN_SUBPIXEL(anchorPointInPoints.y),
                           0 );
        
        m_pCamera->locate();
        
        if( translate )
            kmGLTranslatef(RENDER_IN_SUBPIXEL(-anchorPointInPoints.x),
                           RENDER_IN_SUBPIXEL(-anchorPointInPoints.y),
                           0 );
    }
    
}

void CAView::updateTransform()
{
    if(isDirty())
    {
        if(!m_bVisible || (m_pSuperview && m_pSuperview != m_pobBatchView && m_pSuperview->m_bShouldBeHidden))
        {
            m_sQuad.br.vertices = m_sQuad.tl.vertices = m_sQuad.tr.vertices = m_sQuad.bl.vertices = DPoint3D(0,0,0);
            m_bShouldBeHidden = true;
        }
        else
        {
            m_bShouldBeHidden = false;
            
            if( !m_pSuperview || m_pSuperview == m_pobBatchView)
            {
                m_tTransformToBatch = getViewToSuperviewTransform();
            }
            else
            {
                const Mat4 &nodeToParent = getViewToSuperviewTransform();
                Mat4 &parentTransform = m_pSuperview->m_tTransformToBatch;
                m_tTransformToBatch = parentTransform * nodeToParent;
            }
            
            DSize size = m_obContentSize;

            float x1 = 0;
            float y1 = 0;
            
            float x2 = x1 + size.width;
            float y2 = y1 + size.height;
            
            float x = m_tTransformToBatch.m.mat[12];
            float y = m_tTransformToBatch.m.mat[13];
            
            float cr = m_tTransformToBatch.m.mat[0];
            float sr = m_tTransformToBatch.m.mat[1];
            float cr2 = m_tTransformToBatch.m.mat[5];
            float sr2 = -m_tTransformToBatch.m.mat[4];
            float ax = x1 * cr - y1 * sr2 + x;
            float ay = x1 * sr + y1 * cr2 + y;
            
            float bx = x2 * cr - y1 * sr2 + x;
            float by = x2 * sr + y1 * cr2 + y;
            
            float cx = x2 * cr - y2 * sr2 + x;
            float cy = x2 * sr + y2 * cr2 + y;
            
            float dx = x1 * cr - y2 * sr2 + x;
            float dy = x1 * sr + y2 * cr2 + y;
            
            m_sQuad.bl.vertices = DPoint3D(RENDER_IN_SUBPIXEL(ax), RENDER_IN_SUBPIXEL(ay), m_fVertexZ);
            m_sQuad.br.vertices = DPoint3D(RENDER_IN_SUBPIXEL(bx), RENDER_IN_SUBPIXEL(by), m_fVertexZ);
            m_sQuad.tl.vertices = DPoint3D(RENDER_IN_SUBPIXEL(dx), RENDER_IN_SUBPIXEL(dy), m_fVertexZ);
            m_sQuad.tr.vertices = DPoint3D(RENDER_IN_SUBPIXEL(cx), RENDER_IN_SUBPIXEL(cy), m_fVertexZ);
        }
        
        if (m_pobImageAtlas)
        {
            m_pobImageAtlas->updateQuad(&m_sQuad, m_uAtlasIndex);
        }
        
        m_bRecursiveDirty = false;
        setDirty(false);
    }
    
    for (auto& subview : m_obSubviews)
    {
        subview->updateTransform();
    }
    
#if CC_SPRITE_DEBUG_DRAW
    // draw bounding box
    DPoint vertices[4] =
    {
        DPoint( m_sQuad.bl.vertices.x, m_sQuad.bl.vertices.y ),
        DPoint( m_sQuad.br.vertices.x, m_sQuad.br.vertices.y ),
        DPoint( m_sQuad.tr.vertices.x, m_sQuad.tr.vertices.y ),
        DPoint( m_sQuad.tl.vertices.x, m_sQuad.tl.vertices.y ),
    };
    ccDrawPoly(vertices, 4, true);
#endif
}
////

DRect CAView::convertRectToNodeSpace(const CrossApp::DRect &worldRect)
{
    DRect ret = worldRect;
    ret.origin = this->convertToNodeSpace(ret.origin);
    ret.size = this->convertToNodeSize(ret.size);
    return ret;
}

DRect CAView::convertRectToWorldSpace(const CrossApp::DRect &nodeRect)
{
    DRect ret = nodeRect;
    ret.origin = this->convertToWorldSpace(ret.origin);
    ret.size = this->convertToWorldSize(ret.size);
    return ret;
}

DPoint CAView::convertToNodeSpace(const DPoint& worldPoint)
{
    Mat4 tmp = getWorldToViewTransform();
    DPoint3D vec3(worldPoint.x, CAApplication::getApplication()->getWinSize().height - worldPoint.y, 0);
    DPoint3D ret;
    tmp.transformPoint(vec3,&ret);
    return DPoint(ret.x, m_obContentSize.height - ret.y);
}

DPoint CAView::convertToWorldSpace(const DPoint& nodePoint)
{
    Mat4 tmp = getViewToWorldTransform();
    DPoint3D vec3(nodePoint.x, m_obContentSize.height - nodePoint.y, 0);
    DPoint3D ret;
    tmp.transformPoint(vec3,&ret);
    return DPoint(ret.x, CAApplication::getApplication()->getWinSize().height - ret.y);
}

DSize CAView::convertToNodeSize(const DSize& worldSize)
{
    DSize ret = worldSize;
    for (CAView* v = this; v; v = v->m_pSuperview)
    {
        ret.width /= v->m_fScaleX;
        ret.height /= v->m_fScaleY;
    }
    return ret;
}

DSize CAView::convertToWorldSize(const DSize& nodeSize)
{
    DSize ret = nodeSize;
    for (CAView* v = this; v; v = v->m_pSuperview)
    {
        ret.width *= v->m_fScaleX;
        ret.height *= v->m_fScaleY;
    }
    return ret;
}

DPoint CAView::convertTouchToNodeSpace(CATouch *touch)
{
    DPoint point = touch->getLocation();
    return this->convertToNodeSpace(point);
}

bool CAView::isDisplayRange()
{
    return m_bDisplayRange;
}

void CAView::setDisplayRange(bool value)
{
    m_bDisplayRange = value;
}


void CAView::setImage(CAImage* image)
{
    if (!m_pobBatchView)
    {
        CC_SAFE_RETAIN(image);
        CC_SAFE_RELEASE(m_pobImage);
        m_pobImage = image;
        this->updateBlendFunc();
        this->updateDraw();
    }
}

CAImage* CAView::getImage(void)
{
    return m_pobImage;
}

void CAView::setImageRect(const DRect& rect)
{
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setImageRect(rect, this);
    }
    else
    {
        m_bRectRotated = false;
        
        this->setVertexRect(rect);
        this->setImageCoords(rect);

        if (m_pobBatchView)
        {
            setDirty(true);
        }
        else
        {
            this->updateImageRect();
        }
    }
}

void CAView::updateImageRect()
{
    // Don't update Z.
    
    GLfloat x1,x2,y1,y2;
    x1 = 0;
    y1 = 0;
    x2 = m_obContentSize.width;
    y2 = m_obContentSize.height;
    
    m_sQuad.bl.vertices = DPoint3D(x1, y1, m_fVertexZ);
    m_sQuad.br.vertices = DPoint3D(x2, y1, m_fVertexZ);
    m_sQuad.tl.vertices = DPoint3D(x1, y2, m_fVertexZ);
    m_sQuad.tr.vertices = DPoint3D(x2, y2, m_fVertexZ);
}

// override this method to generate "double scale" sprites
void CAView::setVertexRect(const DRect& rect)
{
    m_obRect = rect;
}

void CAView::setImageCoords(DRect rect)
{
    CAImage* image = m_pobBatchView ? m_pobImageAtlas->getImage() : m_pobImage;
    CC_RETURN_IF(! image);
    
    float atlasWidth = (float)image->getPixelsWide();
    float atlasHeight = (float)image->getPixelsHigh();
 
    float left, right, top, bottom;
    
    if (m_bRectRotated)
    {
#if CC_FIX_ARTIFACTS_BY_STRECHING_TEXEL
        
        left   = (2 * rect.origin.x + 1) / (2 * atlasWidth);
        right  = left + (rect.size.height * 2 - 2) / (2 * atlasWidth);
        top    = (2 * rect.origin.y + 1) / (2 * atlasHeight);
        bottom = top + (rect.size.width * 2 - 2) / (2 * atlasHeight);
        
#else
        
        left   = rect.origin.x / atlasWidth;
        right  = (rect.origin.x + rect.size.height) / atlasWidth;
        top    = rect.origin.y / atlasHeight;
        bottom = (rect.origin.y + rect.size.width) / atlasHeight;
        
#endif // CC_FIX_ARTIFACTS_BY_STRECHING_TEXEL
        
        if (m_bFlipX)
        {
            CC_SWAP(top, bottom, float);
        }
        
        if (m_bFlipY)
        {
            CC_SWAP(left, right, float);
        }
        
        m_sQuad.bl.texCoords.u = left;
        m_sQuad.bl.texCoords.v = top;
        m_sQuad.br.texCoords.u = left;
        m_sQuad.br.texCoords.v = bottom;
        m_sQuad.tl.texCoords.u = right;
        m_sQuad.tl.texCoords.v = top;
        m_sQuad.tr.texCoords.u = right;
        m_sQuad.tr.texCoords.v = bottom;
    }
    else
    {
#if CC_FIX_ARTIFACTS_BY_STRECHING_TEXEL
        left   = (2 * rect.origin.x + 1) / (2 * atlasWidth);
        right  = left + (rect.size.width * 2 - 2) / (2 * atlasWidth);
        top    = (2 * rect.origin.y + 1) / (2 * atlasHeight);
        bottom = top + (rect.size.height * 2 - 2) / (2 * atlasHeight);
#else
        left   = rect.origin.x / atlasWidth;
        right  = (rect.origin.x + rect.size.width) / atlasWidth;
        top    = rect.origin.y / atlasHeight;
        bottom = (rect.origin.y + rect.size.height) / atlasHeight;
#endif // ! CC_FIX_ARTIFACTS_BY_STRECHING_TEXEL
        
        if(m_bFlipX)
        {
            CC_SWAP(left, right, float);
        }
        
        if(m_bFlipY)
        {
            CC_SWAP(top, bottom, float);
        }
        
        m_sQuad.bl.texCoords.u = left;
        m_sQuad.bl.texCoords.v = bottom;
        m_sQuad.br.texCoords.u = right;
        m_sQuad.br.texCoords.v = bottom;
        m_sQuad.tl.texCoords.u = left;
        m_sQuad.tl.texCoords.v = top;
        m_sQuad.tr.texCoords.u = right;
        m_sQuad.tr.texCoords.v = top;
    }
}

float CAView::getAlpha(void)
{
	return _realAlpha;
}

float CAView::getDisplayedAlpha(void)
{
	return _displayedAlpha;
}

void CAView::setAlpha(float alpha)
{
    alpha = MIN(alpha, 1.0f);
    alpha = MAX(alpha, 0.0f);
    
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setAlpha(alpha, this);
    }
    else if (_displayedAlpha != alpha)
    {
        _realAlpha = alpha;
        
        float superviewAlpha = m_pSuperview ? m_pSuperview->getDisplayedAlpha() : 1.0f;
        
        this->updateDisplayedAlpha(superviewAlpha);
    }
}

void CAView::updateDisplayedAlpha(float superviewAlpha)
{
	_displayedAlpha = _realAlpha * superviewAlpha;
	
    if (!m_obSubviews.empty())
    {
        CAVector<CAView*>::iterator itr;
        for (itr=m_obSubviews.begin(); itr!=m_obSubviews.end(); itr++)
            (*itr)->updateDisplayedAlpha(_displayedAlpha);
    }

    this->updateColor();
}

const CAColor4B& CAView::getColor(void)
{
	return _realColor;
}

const CAColor4B& CAView::getDisplayedColor()
{
	return _displayedColor;
}

void CAView::setColor(const CAColor4B& color)
{
    if (CAViewAnimation::areAnimationsEnabled()
        && CAViewAnimation::areBeginAnimations())
    {
        CAViewAnimation::getInstance()->setColor(color, this);
    }
    else
    {
        _realColor = color;
        _displayedColor = color;
        this->updateColor();
    }
}

void CAView::updateDisplayedColor(const CAColor4B& superviewColor)
{
	_displayedColor.r = _realColor.r * superviewColor.r/255.0;
	_displayedColor.g = _realColor.g * superviewColor.g/255.0;
	_displayedColor.b = _realColor.b * superviewColor.b/255.0;
    _displayedColor.a = _realColor.a * superviewColor.a/255.0;
    
    updateColor();
}

void CAView::updateColor(void)
{
    CAColor4B color4 = _displayedColor;
    color4.a = color4.a * _displayedAlpha;
    
    if (m_bOpacityModifyRGB)
    {
        color4.r = color4.r * color4.a / 255.0f;
        color4.g = color4.g * color4.a / 255.0f;
        color4.b = color4.b * color4.a / 255.0f;
    }
   
    m_sQuad.bl.colors = color4;
    m_sQuad.br.colors = color4;
    m_sQuad.tl.colors = color4;
    m_sQuad.tr.colors = color4;
    
    if (m_pobBatchView && m_pobImage)
    {
        if (m_uAtlasIndex != UINT_NONE)
        {
            m_pobImageAtlas->updateQuad(&m_sQuad, m_uAtlasIndex);
        }
        else
        {
            // no need to set it recursively
            // update dirty_, don't update recursiveDirty_
            setDirty(true);
        }
    }

    this->updateDraw();
}

void CAView::updateBlendFunc(void)
{
    if (!m_pobImage || !m_pobImage->hasPremultipliedAlpha())
    {
        m_sBlendFunc.src = GL_SRC_ALPHA;
        m_sBlendFunc.dst = GL_ONE_MINUS_SRC_ALPHA;
        setOpacityModifyRGB(false);
    }
    else
    {
        m_sBlendFunc.src = CC_BLEND_SRC;
        m_sBlendFunc.dst = CC_BLEND_DST;
        setOpacityModifyRGB(true);
    }
}

void CAView::setOpacityModifyRGB(bool modify)
{
    if (m_bOpacityModifyRGB != modify)
    {
        m_bOpacityModifyRGB = modify;
        this->updateColor();
    }
}

bool CAView::isOpacityModifyRGB(void)
{
    return m_bOpacityModifyRGB;
}

void CAView::setReorderSubviewDirtyRecursively(void)
{
    //only set Superviews flag the first time
    if ( ! m_bReorderChildDirty )
    {
        m_bReorderChildDirty = true;
        CAView* view = (CAView*)m_pSuperview;
        while (view && view != m_pobBatchView)
        {
            view->setReorderSubviewDirtyRecursively();
            view = view->getSuperview();
        }
    }
}

void CAView::setDirtyRecursively(bool bValue)
{
    m_bRecursiveDirty = bValue;
    setDirty(bValue);
    // recursively set dirty
    if (m_bHasChildren)
    {
        for (auto& subview : m_obSubviews)
        {
            subview->setDirtyRecursively(bValue);
        }
    }
}

void CAView::setFlipX(bool bFlipX)
{
    if (m_bFlipX != bFlipX)
    {
        if (CAViewAnimation::areAnimationsEnabled()
            && CAViewAnimation::areBeginAnimations())
        {
            CAViewAnimation::getInstance()->setFlipX(bFlipX, this);
        }
        else
        {
            m_bFlipX = bFlipX;
            if (m_pobImage)
            {
                setImageRect(m_obRect);
            }
        }
    }
}

bool CAView::isFlipX(void)
{
    return m_bFlipX;
}

void CAView::setFlipY(bool bFlipY)
{
    if (m_bFlipY != bFlipY)
    {
        if (CAViewAnimation::areAnimationsEnabled()
            && CAViewAnimation::areBeginAnimations())
        {
            CAViewAnimation::getInstance()->setFlipY(bFlipY, this);
        }
        else
        {
            m_bFlipY = bFlipY;
            if (m_pobImage)
            {
                setImageRect(m_obRect);
            }
        }
    }
}

bool CAView::isFlipY(void)
{
    return m_bFlipY;
}

bool CAView::ccTouchBegan(CATouch *pTouch, CAEvent *pEvent)
{
    return false;
}

void CAView::ccTouchMoved(CATouch *pTouch, CAEvent *pEvent)
{
}

void CAView::ccTouchEnded(CATouch *pTouch, CAEvent *pEvent)
{
}

void CAView::ccTouchCancelled(CATouch *pTouch, CAEvent *pEvent)
{
}

void CAView::setBatch(CABatchView *batchView)
{
    m_pobBatchView = batchView; // weak reference
    
    // self render
    if( ! m_pobBatchView )
    {
        m_uAtlasIndex = UINT_NONE;
        setImageAtlas(NULL);
        m_bRecursiveDirty = false;
        setDirty(false);
        
        this->updateImageRect();
    }
    else
    {
        // using batch
        m_tTransformToBatch = Mat4::IDENTITY;
        setImageAtlas(m_pobBatchView->getImageAtlas()); // weak ref
    }
}

CGNode* CAView::getCGNode()
{
    return m_pCGNode;
}

void CAView::setCGNode(CrossApp::CGNode *var)
{
    if (m_bRunning && m_pCGNode && m_pCGNode->isRunning())
    {
        m_pCGNode->m_pSuperviewCAView = NULL;
        m_pCGNode->onExitTransitionDidStart();
        m_pCGNode->onExit();
    }
    
    CC_SAFE_RETAIN(var);
    CC_SAFE_RELEASE(m_pCGNode);
    m_pCGNode = var;
    
    if (m_bRunning && m_pCGNode && !m_pCGNode->isRunning())
    {
        m_pCGNode->m_pSuperviewCAView = this;
        m_pCGNode->onEnter();
        m_pCGNode->onEnterTransitionDidFinish();
    }
    
    CAApplication::getApplication()->updateDraw();
}




NS_CC_END;