#include <string>
#include "CAApplication.h"
#include "CAFPSImages.h"
#include "view/CADrawingPrimitives.h"
#include "cocoa/CCNS.h"
#include "view/CAWindow.h"
#include "CAScheduler.h"
#include "ccMacros.h"
#include "dispatcher/CATouchDispatcher.h"
#include "support/CAPointExtension.h"
#include "support/CANotificationCenter.h"
#include "images/CAImageCache.h"
#include "basics/CAAutoreleasePool.h"
#include "platform/platform.h"
#include "platform/CAFileUtils.h"
#include "CCApplication.h"
#include "dispatcher/CAKeypadDispatcher.h"
#include "dispatcher/CATouch.h"
#include "support/user_default/CAUserDefault.h"
#include "shaders/ccGLStateCache.h"
#include "shaders/CAShaderCache.h"
#include "kazmath/kazmath.h"
#include "kazmath/GL/matrix.h"
#include "support/CAProfiling.h"
#include "CCEGLView.h"
#include "platform/CADensityDpi.h"
#include "support/netWork/HttpClient.h"
#include "support/netWork/DownloadManager.h"
#include "game/actions/CGActionManager.h"
#include "support/CAThemeManager.h"

NS_CC_BEGIN

static CCDisplayLinkDirector *s_SharedApplication = NULL;

#define kDefaultFPS        60  // 60 frames per second
extern const char* CrossAppVersion(void);

CAApplication* CAApplication::getApplication()
{
    if (!s_SharedApplication)
    {
        s_SharedApplication = new CCDisplayLinkDirector();
        s_SharedApplication->init();
    }

    return s_SharedApplication;
}

CAApplication::CAApplication(void)
{

}

bool CAApplication::init(void)
{
	setDefaultValues();

    // scenes
    m_pRootWindow = NULL;

    m_pNotificationNode = NULL;

    // FPS
    m_fAccumDt = 0.0f;
    m_fFrameRate = 0.0f;
    m_pFPSLabel = NULL;
    m_uTotalFrames = m_uFrames = 0;
    m_pszFPS = new char[10];
    m_pLastUpdate = new struct cc_timeval();
    m_fSecondsPerFrame = 0.0f;

    m_nDrawCount = 60;
    m_dAnimationInterval = 1.0 / 100.0f;
    m_bDisplayStats = false;
    m_uNumberOfDraws = 0;
    // paused ?
    m_bPaused = false;
   
    // purge ?
    m_bPurgeDirecotorInNextLoop = false;

    m_obWinSizeInPoints = DSizeZero;    

    m_pobOpenGLView = NULL;

    // touchDispatcher
    m_pTouchDispatcher = new CATouchDispatcher();
    m_pTouchDispatcher->init();

    // KeypadDispatcher
    m_pKeypadDispatcher = new CAKeypadDispatcher();

    // Accelerometer
    m_pAccelerometer = new CAAccelerometer();

    // create autorelease pool
    CAPoolManager::sharedPoolManager()->push();

    m_fAdaptationRatio = CADensityDpi::getDensityDpi() / DPI_STANDARD;
    
    m_pThemeManager = nullptr;
    this->setThemeManager(CAThemeManager::create("source_material"));
    
    return true;
}
    
CAApplication::~CAApplication(void)
{
    CC_SAFE_RELEASE(m_pFPSLabel);
    
    CC_SAFE_RELEASE(m_pRootWindow);
    CC_SAFE_RELEASE(m_pNotificationNode);
    CC_SAFE_RELEASE(m_pTouchDispatcher);
    CC_SAFE_RELEASE(m_pKeypadDispatcher);
    CC_SAFE_RELEASE(m_pThemeManager);
    CC_SAFE_DELETE(m_pAccelerometer);

    // pop the autorelease pool
    CAPoolManager::sharedPoolManager()->pop();
    CAPoolManager::purgePoolManager();
    CAScheduler::destroyScheduler();
    
    // delete m_pLastUpdate
    CC_SAFE_DELETE(m_pLastUpdate);
    // delete fps string
    delete []m_pszFPS;

    s_SharedApplication = NULL;
}

void CAApplication::setDefaultValues(void)
{
	m_dOldAnimationInterval = m_dAnimationInterval = 1.0 / 100;
    m_eProjection = CAApplication::Default;
}

void CAApplication::setGLDefaultValues(void)
{
    // This method SHOULD be called only after openGLView_ was initialized
    CCAssert(m_pobOpenGLView, "opengl view should not be null");

    setAlphaBlending(true);
    setDepthTest(false);
    setProjection(m_eProjection);

    // set other opengl default values
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void CAApplication::updateDraw()
{
    m_nDrawCount = 30;
}

void CAApplication::drawScene(float dt)
{    
    if (m_nDrawCount > 0)
    {
        --m_nDrawCount;
        
        if (m_pRootWindow)
        {
            m_pRootWindow->visitEve();
        }
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        kmGLPushMatrix();
        
        m_uNumberOfDraws = 0;
        // draw the scene
        if (m_pRootWindow)
        {
            m_pRootWindow->visit();
        }
        
        // draw the notifications node
        if (m_pNotificationNode)
        {
            m_pNotificationNode->visit();
        }
        
        if (m_bDisplayStats)
        {
            showStats();
        }
        
        kmGLPopMatrix();
        
        
        m_uTotalFrames++;
        
        // swap buffers
        if (m_pobOpenGLView)
        {
            m_pobOpenGLView->swapBuffers();
        }
        
        if (m_bDisplayStats)
        {
            calculateMPF();
        }
    }
}

void CAApplication::calculateDeltaTime(void)
{
    struct cc_timeval now;

    if (CCTime::gettimeofdayCrossApp(&now, NULL) != 0)
    {
        CCLOG("error in gettimeofday");
        m_fDeltaTime = 0;
        return;
    }

    // new delta time. Re-fixed issue #1277
    if (m_bNextDeltaTimeZero)
    {
        m_fDeltaTime = 0;
        m_bNextDeltaTimeZero = false;
    }
    else
    {
        m_fDeltaTime = (now.tv_sec - m_pLastUpdate->tv_sec) + (now.tv_usec - m_pLastUpdate->tv_usec) / 1000000.0f;
        m_fDeltaTime = MAX(0, m_fDeltaTime);
    }

#ifdef DEBUG
    // If we are debugging our code, prevent big delta time
    if(m_fDeltaTime > 0.2f)
    {
        m_fDeltaTime = 1 / 60.0f;
    }
#endif

    *m_pLastUpdate = now;
}
float CAApplication::getDeltaTime()
{
	return m_fDeltaTime;
}
void CAApplication::setOpenGLView(CCEGLView *pobOpenGLView)
{
    CCAssert(pobOpenGLView, "opengl view should not be null");

    if (m_pobOpenGLView != pobOpenGLView)
    {

        // EAGLView is not a CAObject
        if(m_pobOpenGLView)
            delete m_pobOpenGLView; // [openGLView_ release]
        m_pobOpenGLView = pobOpenGLView;

        // set size
        m_obWinSizeInPoints = m_pobOpenGLView->getDesignResolutionSize();
        
        createStatsLabel();
        
        if (m_pobOpenGLView)
        {
            setGLDefaultValues();
        }  
        
        CHECK_GL_ERROR_DEBUG();

        m_pobOpenGLView->setTouchDelegate(m_pTouchDispatcher);
    }
}

void CAApplication::setViewport()
{
    if (m_pobOpenGLView)
    {
        m_pobOpenGLView->setViewPortInPoints(0, 0, m_obWinSizeInPoints.width, m_obWinSizeInPoints.height);
    }
}

void CAApplication::setNextDeltaTimeZero(bool bNextDeltaTimeZero)
{
    m_bNextDeltaTimeZero = bNextDeltaTimeZero;
}

void CAApplication::setProjection(CAApplication::Projection kProjection)
{
    DSize size = m_obWinSizeInPoints;

    setViewport();

    switch (kProjection)
    {
    case CAApplication::P2D:
        {
            kmGLMatrixMode(KM_GL_PROJECTION);
            kmGLLoadIdentity();
            kmMat4 orthoMatrix;
            kmMat4OrthographicProjection(&orthoMatrix, 0, size.width, 0, size.height, 0, 1024 );
            kmGLMultMatrix(&orthoMatrix);
            kmGLMatrixMode(KM_GL_MODELVIEW);
            kmGLLoadIdentity();
        }
        break;

    case CAApplication::P3D:
        {
            float zeye = this->getZEye();

            kmMat4 matrixPerspective, matrixLookup;

            kmGLMatrixMode(KM_GL_PROJECTION);
            kmGLLoadIdentity();
            
            // issue #1334
            kmMat4PerspectiveProjection( &matrixPerspective, 60, (GLfloat)size.width/size.height, 0.1f, zeye*2);
            // kmMat4PerspectiveProjection( &matrixPerspective, 60, (GLfloat)size.width/size.height, 0.1f, 1500);

            kmGLMultMatrix(&matrixPerspective);

            kmGLMatrixMode(KM_GL_MODELVIEW);
            kmGLLoadIdentity();
            kmVec3 eye, center, up;
            kmVec3Fill( &eye, size.width/2, size.height/2, zeye );
            kmVec3Fill( &center, size.width/2, size.height/2, 0.0f );
            kmVec3Fill( &up, 0.0f, 1.0f, 0.0f);
            kmMat4LookAt(&matrixLookup, &eye, &center, &up);
            kmGLMultMatrix(&matrixLookup);
        }
        break;
            
    default:
        CCLOG("CrossApp: CAApplication: unrecognized projection");
        break;
    }

    m_eProjection = kProjection;
    ccSetProjectionMatrixDirty();
}

void CAApplication::purgeCachedData(void)
{
    if (s_SharedApplication->getOpenGLView())
    {
        CAImageCache::sharedImageCache()->removeUnusedImages();
    }
    FileUtils::getInstance()->purgeCachedEntries();
}

float CAApplication::getZEye(void)
{
    return (m_obWinSizeInPoints.height / 1.1565f);
}

void CAApplication::setAlphaBlending(bool bOn)
{
    if (bOn)
    {
        ccGLBlendFunc(CC_BLEND_SRC, CC_BLEND_DST);
    }
    else
    {
        ccGLBlendFunc(GL_ONE, GL_ZERO);
    }

    CHECK_GL_ERROR_DEBUG();
}

void CAApplication::reshapeProjection(const DSize& newWindowSize)
{
	CC_UNUSED_PARAM(newWindowSize);
	if (m_pobOpenGLView)
	{
		m_obWinSizeInPoints = DSize(newWindowSize.width, newWindowSize.height);
		setProjection(m_eProjection);
	}
    if (m_pRootWindow)
    {
        DRect rect = DRectZero;
        rect.size = newWindowSize;
        m_pRootWindow->setFrame(rect);
    }

}

void CAApplication::setStatusBarStyle(const CAStatusBarStyle &var)
{
    m_eStatusBarStyle = var;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    CCApplication::sharedApplication()->setStatusBarStyle(var);
#endif
    
}

void CAApplication::setStatusBarHidden(bool isStatusBarHidden)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    CCApplication::sharedApplication()->setStatusBarHidden(isStatusBarHidden);
#endif
}

bool CAApplication::isStatusBarHidden()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    return CCApplication::sharedApplication()->isStatusBarHidden();
#else
    return false;
#endif
}

const CAInterfaceOrientation& CAApplication::getStatusBarOrientation()
{
    return CCEGLView::sharedOpenGLView()->getStatusBarOrientation();
}

void CAApplication::setThemeManager(CAThemeManager* var)
{
    CC_SAFE_RETAIN(var);
    CC_SAFE_RELEASE(m_pThemeManager);
    m_pThemeManager = var;
}


void CAApplication::setDepthTest(bool bOn)
{
    if (bOn)
    {
        glClearDepth(1.0f);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        
#if (CC_TARGET_PLATFORM != CC_PLATFORM_IOS && CC_TARGET_PLATFORM != CC_PLATFORM_ANDROID)
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
#endif
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
    CHECK_GL_ERROR_DEBUG();
}

static void GLToClipTransform(kmMat4 *transformOut)
{
	kmMat4 projection;
	kmGLGetMatrix(KM_GL_PROJECTION, &projection);

	kmMat4 modelview;
	kmGLGetMatrix(KM_GL_MODELVIEW, &modelview);
	
	kmMat4Multiply(transformOut, &projection, &modelview);
}

DSize CAApplication::getWinSize(void)
{
    return m_obWinSizeInPoints;
}

DSize CAApplication::getVisibleSize()
{
    if (m_pobOpenGLView)
    {
        return m_pobOpenGLView->getVisibleSize();
    }
    else 
    {
        return DSizeZero;
    }
}

DPoint CAApplication::getVisibleOrigin()
{
    if (m_pobOpenGLView)
    {
        return m_pobOpenGLView->getVisibleOrigin();
    }
    else 
    {
        return DPointZero;
    }
}

// scene management

void CAApplication::runWindow(CAWindow *pWindow)
{
    if (m_pRootWindow)
    {
        m_pRootWindow->onExitTransitionDidStart();
        m_pRootWindow->onExit();
        m_pRootWindow->release();
        m_pRootWindow = NULL;
    }
    
    CC_SAFE_RETAIN(pWindow);
    m_pRootWindow = pWindow;
    
    startAnimation();
 
    this->run(0);
}

void CAApplication::run(float dt)
{
    if (m_pRootWindow)
    {
        m_pRootWindow->onEnter();
        m_pRootWindow->onEnterTransitionDidFinish();
    }
}

void CAApplication::end()
{
    m_bPurgeDirecotorInNextLoop = true;
}

void CAApplication::purgeDirector()
{
    // cleanup scheduler
    CAScheduler::unscheduleAll();
    
    // don't release the event handlers
    // They are needed in case the director is run again

    if (m_pRootWindow)
    {
        m_pRootWindow->onExitTransitionDidStart();
        m_pRootWindow->onExit();
        m_pRootWindow->release();
    }
    
    m_pRootWindow = NULL;

    stopAnimation();

    CC_SAFE_RELEASE_NULL(m_pFPSLabel);

    CAHttpClient::destroyAllInstance();
    CADownloadManager::destroyInstance();
    ActionManager::destroyInstance();
    CAUserDefault::destroyInstance();
    CANotificationCenter::destroyInstance();
    
    // purge all managed caches
    ccDrawFree();
    CAImageCache::purgeSharedImageCache();
    CAShaderCache::purgeSharedShaderCache();
    FileUtils::destroyInstance();
    ccGLInvalidateStateCache();
    
    // OpenGL view
    m_pobOpenGLView->end();
    m_pobOpenGLView = NULL;
    
    this->release();
}

void CAApplication::pause(void)
{
    if (m_bPaused)
    {
        return;
    }

    m_dOldAnimationInterval = m_dAnimationInterval;

    // when paused, don't consume CPU
    setAnimationInterval(1 / 4.0);
    m_bPaused = true;
}

void CAApplication::resume(void)
{
    if (! m_bPaused)
    {
        return;
    }

    setAnimationInterval(m_dOldAnimationInterval);

    if (CCTime::gettimeofdayCrossApp(m_pLastUpdate, NULL) != 0)
    {
        CCLOG("CrossApp: CAApplication: Error in gettimeofday");
    }

    m_bPaused = false;
    m_fDeltaTime = 0;
}

// display the FPS using a LabelAtlas
// updates the FPS every frame
void CAApplication::showStats(void)
{
    m_uFrames++;
    m_fAccumDt += m_fDeltaTime;
    
    if (m_bDisplayStats && m_pFPSLabel)
    {
        if (m_fAccumDt > CC_DIRECTOR_STATS_INTERVAL)
        {
            m_fFrameRate = m_uFrames / m_fAccumDt;
            m_uFrames = 0;
            m_fAccumDt = 0;
            
            sprintf(m_pszFPS, "%.1f", m_fFrameRate);
            m_pFPSLabel->setText(m_pszFPS);
        }
        m_pFPSLabel->visitEve();
        m_pFPSLabel->visit();
    }
}

void CAApplication::calculateMPF()
{
    struct cc_timeval now;
    CCTime::gettimeofdayCrossApp(&now, NULL);
    
    m_fSecondsPerFrame = (now.tv_sec - m_pLastUpdate->tv_sec) + (now.tv_usec - m_pLastUpdate->tv_usec) / 1000000.0f;
}

// returns the FPS image data pointer and len
void CAApplication::getFPSImageData(unsigned char** datapointer, unsigned int* length)
{
    // XXX fixed me if it should be used 
    *datapointer = cc_fps_images_png;
	*length = cc_fps_images_len();
}

void CAApplication::createStatsLabel()
{
    if( m_pFPSLabel)
    {
        CC_SAFE_RELEASE_NULL(m_pFPSLabel);
        //CAImageCache::sharedImageCache()->removeImageForKey("cc_fps_images");
        FileUtils::getInstance()->purgeCachedEntries();
    }

//    unsigned char *data = NULL;
//    unsigned int data_len = 0;
//    getFPSImageData(&data, &data_len);
//
//    CAImage* image = CAImage::createWithImageData(data, data_len, "cc_fps_images");
    
    m_pFPSLabel = CALabel::createWithFrame(DRect(20, 20, 100, 50));
    m_pFPSLabel->setFontSize(40);
	m_pFPSLabel->setColor(CAColor_yellow);
    CC_SAFE_RETAIN(m_pFPSLabel);
    m_pFPSLabel->onEnter();
    m_pFPSLabel->onEnterTransitionDidFinish();
}

CAView* CAApplication::getNotificationView()
{ 
    return m_pNotificationNode; 
}

void CAApplication::setNotificationView(CAView *view)
{
    if (m_pNotificationNode)
    {
        m_pNotificationNode->resignFirstResponder();
    }
    CC_SAFE_RELEASE(m_pNotificationNode);
    m_pNotificationNode = view;
    if (m_pNotificationNode)
    {
        m_pNotificationNode->becomeFirstResponder();
        m_pNotificationNode->reViewlayout(m_obWinSizeInPoints);
    }
    CC_SAFE_RETAIN(m_pNotificationNode);
    
    
    this->updateDraw();
}

void CAApplication::setTouchDispatcher(CATouchDispatcher* pTouchDispatcher)
{
    if (m_pTouchDispatcher != pTouchDispatcher)
    {
        CC_SAFE_RETAIN(pTouchDispatcher);
        CC_SAFE_RELEASE(m_pTouchDispatcher);
        m_pTouchDispatcher = pTouchDispatcher;
    }    
}

CATouchDispatcher* CAApplication::getTouchDispatcher()
{
    return m_pTouchDispatcher;
}

void CAApplication::setKeypadDispatcher(CAKeypadDispatcher* pKeypadDispatcher)
{
    CC_SAFE_RETAIN(pKeypadDispatcher);
    CC_SAFE_RELEASE(m_pKeypadDispatcher);
    m_pKeypadDispatcher = pKeypadDispatcher;
}

CAKeypadDispatcher* CAApplication::getKeypadDispatcher()
{
    return m_pKeypadDispatcher;
}

void CAApplication::setAccelerometer(CAAccelerometer* pAccelerometer)
{
    if (m_pAccelerometer != pAccelerometer)
    {
        CC_SAFE_DELETE(m_pAccelerometer);
        m_pAccelerometer = pAccelerometer;
    }
}

CAAccelerometer* CAApplication::getAccelerometer()
{
    return m_pAccelerometer;
}

unsigned long CAApplication::getCurrentNumberOfDraws()
{
    return ++m_uNumberOfDraws;
}

/***************************************************
* implementation of DisplayLinkDirector
**************************************************/

// should we implement 4 types of director ??
// I think DisplayLinkDirector is enough
// so we now only support DisplayLinkDirector
void CCDisplayLinkDirector::startAnimation(void)
{
    if (CCTime::gettimeofdayCrossApp(m_pLastUpdate, NULL) != 0)
    {
        CCLOG("CrossApp: DisplayLinkDirector: Error on gettimeofday");
    }

    m_bInvalid = false;
    m_nDrawCount = 180;
}

void CCDisplayLinkDirector::mainLoop(void)
{
    // calculate "global" dt
    calculateDeltaTime();
    
    if (m_bPurgeDirecotorInNextLoop)
    {
		if (!m_bPaused)
		{
			CAScheduler::getScheduler()->update(m_fDeltaTime);
			CAObject::updateDelayTimers(m_fDeltaTime);
		}
		CAPoolManager::sharedPoolManager()->pop();

        m_bPurgeDirecotorInNextLoop = false;
        purgeDirector();
    }
    else if (! m_bInvalid)
     {
         if (! m_bPaused)
         {
             CAScheduler::getScheduler()->update(m_fDeltaTime);
			 CAObject::updateDelayTimers(m_fDeltaTime);
             drawScene();
         }
         CAPoolManager::sharedPoolManager()->pop();
     }
}

void CCDisplayLinkDirector::stopAnimation(void)
{
    m_bInvalid = true;
}

void CCDisplayLinkDirector::setAnimationInterval(double dValue)
{
    m_dAnimationInterval = dValue;
    if (! m_bInvalid)
    {
        stopAnimation();
        startAnimation();
    }    
}

NS_CC_END

