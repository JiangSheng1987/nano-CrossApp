//
//  CABar.cpp
//  CrossApp
//
//  Created by Li Yuanfeng on 14-4-14.
//  Copyright (c) 2014 http://9miao.com All rights reserved.
//

#include "CABar.h"
#include "view/CAScale9ImageView.h"
#include "view/CALabel.h"
#include "view/CAScrollView.h"
#include "basics/CAApplication.h"
#include "support/CAPointExtension.h"
#include "dispatcher/CATouch.h"
#include "animation/CAViewAnimation.h"
#include "support/CAThemeManager.h"
#include "support/ccUtils.h"
NS_CC_BEGIN

#pragma CANavigationBar

CANavigationBar::CANavigationBar(bool clearance)
:m_pContentView(NULL)
,m_pTitle(NULL)
,m_pDelegate(NULL)
,m_pBackgroundView(NULL)
,m_cTitleColor(CAColor_white)
,m_cButtonColor(CAColor_white)
,m_pItem(NULL)
,m_pGoBackBarButtonItem(nullptr)
,m_bClearance(clearance)
{

}

CANavigationBar::~CANavigationBar()
{
    CC_SAFE_RELEASE_NULL(m_pItem);
    CC_SAFE_RELEASE_NULL(m_pGoBackBarButtonItem);
    CC_SAFE_RELEASE(m_pBackgroundView);
}

CANavigationBar* CANavigationBar::createWithFrame(const DRect& rect, bool clearance)
{
    CANavigationBar* navigationBar = new CANavigationBar(clearance);
    if (navigationBar && navigationBar->initWithFrame(rect))
    {
        navigationBar->autorelease();
        return navigationBar;
    }
    CC_SAFE_DELETE(navigationBar);
    return NULL;
}

CANavigationBar* CANavigationBar::createWithCenter(const DRect& rect, bool clearance)
{
    CANavigationBar* navigationBar = new CANavigationBar(clearance);
    if (navigationBar && navigationBar->initWithFrame(rect))
    {
        navigationBar->autorelease();
        return navigationBar;
    }
    CC_SAFE_DELETE(navigationBar);
    return NULL;
}

CANavigationBar* CANavigationBar::createWithLayout(const CrossApp::DLayout &layout, bool clearance)
{
    CANavigationBar* navigationBar = new CANavigationBar(clearance);
    if (navigationBar && navigationBar->initWithLayout(layout))
    {
        navigationBar->autorelease();
        return navigationBar;
    }
    CC_SAFE_DELETE(navigationBar);
    return NULL;
}

bool CANavigationBar::init()
{
    m_pContentView = new CAView();
    m_pContentView->setLayout(DLayout(DHorizontalLayout_L_R(0, 0), DVerticalLayout_T_B(m_bClearance ? 40 : 0, 0)));
    this->addSubview(m_pContentView);
    m_pContentView->release();
    
    this->enabledBottomShadow(true);
    
    const CAThemeManager::stringMap& map = CAApplication::getApplication()->getThemeManager()->getThemeMap("CANavigationBar");
    
    m_pGoBackBarButtonItem = CABarButtonItem::create("", CAImage::create(map.at("leftButtonImage")), nullptr);
    m_pGoBackBarButtonItem->setItemWidth(80);
    m_pGoBackBarButtonItem->retain();
    
    return true;
}

void CANavigationBar::onEnterTransitionDidFinish()
{
    CAView::onEnterTransitionDidFinish();
    
    if (m_pBackgroundView == NULL || m_pBackgroundView->getSuperview() == NULL)
    {
        this->showBackground();
    }
    this->updateNavigationBar();
}

void CANavigationBar::onExitTransitionDidStart()
{
    CAView::onExitTransitionDidStart();
}

void CANavigationBar::setContentSize(const DSize & var)
{
    CAView::setContentSize(var);
}

void CANavigationBar::setItem(CANavigationBarItem* item)
{
    if (item == NULL)
    {
        item = CANavigationBarItem::create("The Title");
    }
    CC_SAFE_RETAIN(item);
    CC_SAFE_RELEASE_NULL(m_pItem);
    m_pItem = item;
    this->updateNavigationBar();
}

void CANavigationBar::setGoBackBarButtonItem(CABarButtonItem* item)
{
    CC_SAFE_RETAIN(item);
    CC_SAFE_RELEASE_NULL(m_pGoBackBarButtonItem);
    m_pGoBackBarButtonItem = item;
    this->updateNavigationBar();
}

void CANavigationBar::setBackgroundView(CAView* var)
{
    var->setTouchEnabled(false);
    this->removeSubview(m_pBackgroundView);
    CC_SAFE_RETAIN(var);
    CC_SAFE_RELEASE(m_pBackgroundView);
    m_pBackgroundView = var;
    CC_RETURN_IF(!m_bRunning);
    this->showBackground();
}

void CANavigationBar::setTitleColor(const CAColor4B& color)
{
    m_cTitleColor = color;
    CC_RETURN_IF(!m_bRunning);
    this->showTitle();
}

void CANavigationBar::setButtonColor(const CAColor4B& color)
{
    m_cButtonColor = color;
    CC_RETURN_IF(!m_bRunning);
    this->showLeftButton();
    this->showRightButton();
}

void CANavigationBar::updateNavigationBar()
{
    CC_RETURN_IF(m_pItem == NULL);
    this->showTitle();
    this->showLeftButton();
    this->showRightButton();
}

void CANavigationBar::showBackground()
{
    if (m_pBackgroundView == NULL)
    {
        const CAThemeManager::stringMap& map = CAApplication::getApplication()->getThemeManager()->getThemeMap("CANavigationBar");
        m_pBackgroundView = CAScale9ImageView::createWithImage(CAImage::create(map.at("backgroundView")));
        CC_SAFE_RETAIN(m_pBackgroundView);
    }
    m_pBackgroundView->setLayout(DLayoutFill);
    this->insertSubview(m_pBackgroundView, -1);
}

void CANavigationBar::showTitle()
{
    if (m_pTitle)
    {
        m_pContentView->removeSubview(m_pTitle);
        m_pTitle = NULL;
    }
    
    if (CAView* titleView = m_pItem->getTitleView())
    {
        if (titleView->getLayout().equals(DLayoutZero))
        {
            DLayout layout;
            
            if (!titleView->getBounds().size.equals(DSizeZero))
            {
                layout.horizontal.center = 0.5f;
                layout.horizontal.width = titleView->getBounds().size.width;
                layout.vertical.center = 0.5f;
                layout.vertical.height = titleView->getBounds().size.height;
            }
            else
            {
                layout.horizontal = DHorizontalLayout_L_R(100, 100);
                layout.vertical = DVerticalLayout_T_B(5, 5);
            }
            
            titleView->setLayout(layout);
        }
        
        m_pContentView->addSubview(titleView);
        m_pTitle = titleView;
    }
    else if (CAImage* image = m_pItem->getTitleViewImage())
    {
        m_pTitle = CAImageView::createWithImage(image);
        m_pTitle->setLayout(DLayout(DHorizontalLayout_L_R(180, 180), DVerticalLayout_T_B(12, 12)));
        ((CAImageView*)m_pTitle)->setImageViewScaleType(CAImageViewScaleTypeFitImageInside);
        m_pContentView->addSubview(m_pTitle);
    }
    else
    {
        CALabel* title = CALabel::createWithLayout(DLayout(DHorizontalLayout_L_R(180, 180), DVerticalLayoutFill));
        title->setTextAlignment(CATextAlignmentCenter);
        title->setVerticalTextAlignmet(CAVerticalTextAlignmentCenter);
        title->setNumberOfLine(1);
		title->setColor(m_cTitleColor);
        title->setFontSize(36);
        title->setBold(true);
        m_pContentView->addSubview(title);
        m_pTitle = title;
        
        if (m_pItem)
        {
            std::string str = m_pItem->getTitle();
            ((CALabel*)m_pTitle)->setText(str.c_str());
        }
    }
}

void CANavigationBar::showLeftButton()
{
    std::vector<CAButton*>::iterator itr;
    for (itr = m_pLeftButtons.begin(); itr != m_pLeftButtons.end(); itr++)
    {
        (*itr)->removeFromSuperview();
    }
    m_pLeftButtons.clear();
    
    const CAVector<CAObject*>& buttonItems = m_pItem->getLeftButtonItems();

    DLayout layout = DLayout(DHorizontalLayout_L_W(0, 80), DVerticalLayoutFill);

    for (size_t i=0; i<buttonItems.size(); i++)
    {
        CABarButtonItem* item = dynamic_cast<CABarButtonItem*>(buttonItems.at(i));
        
        if (i == 0)
        {
            layout.horizontal.left = 3;
            layout.horizontal.width = item ? item->getItemWidth() : m_pGoBackBarButtonItem->getItemWidth();
        }
        else
        {
            layout.horizontal.width = item->getItemWidth();
        }
        
        CAButton* button = CAButton::createWithLayout(layout, CAButtonTypeCustom);
        m_pContentView->addSubview(button);
        button->setTitleFontSize(32);
        
        if (i == 0 && item == NULL)
        {
            CAImage* image = m_pGoBackBarButtonItem->getImage();

            if (image)
            {
                float ratio = image->getAspectRatio();
                button->setImageSize(DSize(m_pGoBackBarButtonItem->getImageWidth(), m_pGoBackBarButtonItem->getImageWidth() / ratio));
                button->setImageOffset(DSize(m_pGoBackBarButtonItem->getImageOffsetX(), 0));
                button->setImageForState(CAControlStateAll, image);
                
                if (m_pGoBackBarButtonItem->getHighlightedImage())
                {
                    button->setImageForState(CAControlStateHighlighted, m_pGoBackBarButtonItem->getHighlightedImage());
                }
                else
                {
                    button->setImageColorForState(CAControlStateHighlighted, ccc4(127, 127, 127, 255));
                }            }
            
            std::string title = m_pGoBackBarButtonItem->getTitle();
            if (!title.empty())
            {
                button->setTitleForState(CAControlStateAll, title);
                button->setTitleLabelSize(DSize(m_pGoBackBarButtonItem->getLabelWidth(), 44));
                button->setTitleOffset(DSize(m_pGoBackBarButtonItem->getLabelOffsetX(), 0));
                button->setTitleColorForState(CAControlStateNormal, m_cButtonColor);
                button->setTitleColorForState(CAControlStateHighlighted, ccc4(m_cButtonColor.r/2, m_cButtonColor.g/2, m_cButtonColor.b/2, 255));
            }
            
            
            button->addTarget(this, CAControl_selector(CANavigationBar::goBack), CAControlEventTouchUpInSide);
        }
        else if (item)
        {
            CAImage* image = item->getImage();
            
            if (image)
            {
                float ratio = image->getAspectRatio();
                button->setImageSize(DSize(item->getImageWidth(), item->getImageWidth() / ratio));
                button->setImageOffset(DSize(item->getImageOffsetX(), 0));
                button->setImageForState(CAControlStateAll, image);
                
                if (item->getHighlightedImage())
                {
                    button->setImageForState(CAControlStateHighlighted, item->getHighlightedImage());
                }
                else
                {
                    button->setImageColorForState(CAControlStateHighlighted, ccc4(127, 127, 127, 255));
                }
            }
            
            std::string title = item->getTitle();
            if (!title.empty())
            {
                button->setTitleForState(CAControlStateAll, title);
                button->setTitleLabelSize(DSize(item->getLabelWidth(), 44));
                button->setTitleOffset(DSize(item->getLabelOffsetX(), 0));
                button->setTitleForState(CAControlStateNormal, item->getTitle());
                button->setTitleForState(CAControlStateHighlighted, item->getTitle());
                button->setTitleColorForState(CAControlStateNormal, m_cButtonColor);
                button->setTitleColorForState(CAControlStateHighlighted, ccc4(m_cButtonColor.r/2, m_cButtonColor.g/2, m_cButtonColor.b/2, 255));
            }
            
            button->addTarget(item->getTarget(), item->getSel(), CAControlEventTouchUpInSide);
        }
        m_pLeftButtons.push_back(button);
        
        layout.horizontal.left += layout.horizontal.width;
    }
}

void CANavigationBar::showRightButton()
{
    std::vector<CAButton*>::iterator itr;
    for (itr = m_pRightButtons.begin(); itr != m_pRightButtons.end(); itr++)
    {
        (*itr)->removeFromSuperview();
    }
    m_pRightButtons.clear();
    
    const CAVector<CAObject*>& buttonItems = m_pItem->getRightButtonItems();

    DLayout layout = DLayout(DHorizontalLayout_R_W(0, 80), DVerticalLayoutFill);
    
    for (size_t i=0; i<buttonItems.size(); i++)
    {
        CABarButtonItem* item = dynamic_cast<CABarButtonItem*>(buttonItems.at(i));
        
        layout.horizontal.width = item ? item->getItemWidth() : 80;
        
        if (i == 0)
        {
            layout.horizontal.right = 3;
        }
        
        CAButton* button = CAButton::createWithLayout(layout, CAButtonTypeCustom);
        m_pContentView->addSubview(button);
        button->setTitleFontSize(32);
        
        if (item)
        {
            CAImage* image = item->getImage();
            
            if (image)
            {
                float ratio = image->getAspectRatio();
                button->setImageSize(DSize(item->getImageWidth(), item->getImageWidth() / ratio));
                button->setImageOffset(DSize(item->getImageOffsetX(), 0));
                button->setImageForState(CAControlStateAll, image);
                
                if (item->getHighlightedImage())
                {
                    button->setImageForState(CAControlStateHighlighted, item->getHighlightedImage());
                }
                else
                {
                    button->setImageColorForState(CAControlStateHighlighted, ccc4(127, 127, 127, 255));
                }
            }
            
            std::string title = item->getTitle();
            if (!title.empty())
            {
                button->setTitleForState(CAControlStateAll, title);
                button->setTitleLabelSize(DSize(item->getLabelWidth(), 44));
                button->setTitleOffset(DSize(item->getLabelOffsetX(), 0));
                button->setTitleForState(CAControlStateNormal, item->getTitle());
                button->setTitleForState(CAControlStateHighlighted, item->getTitle());
                button->setTitleColorForState(CAControlStateNormal, m_cButtonColor);
                button->setTitleColorForState(CAControlStateHighlighted, ccc4(m_cButtonColor.r/2, m_cButtonColor.g/2, m_cButtonColor.b/2, 255));
            }
            
            button->addTarget(item->getTarget(), item->getSel(), CAControlEventTouchUpInSide);
        }
        m_pRightButtons.push_back(button);
        
        layout.horizontal.right += layout.horizontal.width;
    }
}

void CANavigationBar::goBack(CAControl* btn, DPoint point)
{
    if (m_pDelegate)
    {
        m_pDelegate->navigationPopViewController(this, true);
    }
}

#pragma CABadgeView

CABadgeView::CABadgeView()
:m_pBackground(NULL)
,m_pTextView(NULL)
{

}

CABadgeView::~CABadgeView()
{
    
}

bool CABadgeView::init()
{
    const CAThemeManager::stringMap& map = CAApplication::getApplication()->getThemeManager()->getThemeMap("CABadgeView");
    m_pBackground = CAScale9ImageView::createWithLayout(DLayout(DHorizontalLayout_W_C(46, 0.5f), DVerticalLayout_H_C(46, 0.5f)));
    m_pBackground->setCapInsets(DRect(22.5, 22.5, 1, 1));
    m_pBackground->setImage(CAImage::create(map.at("badgeImage")));
    this->addSubview(m_pBackground);
    
    m_pTextView = CALabel::createWithLayout(DLayout(DHorizontalLayout_W_C(180, 0.5f), DVerticalLayout_H_C(46, 0.5f)));
    m_pTextView->setTextAlignment(CATextAlignmentCenter);
    m_pTextView->setVerticalTextAlignmet(CAVerticalTextAlignmentCenter);
    m_pTextView->setFontSize(30);
    m_pTextView->setColor(CAColor_white);
    m_pTextView->setBold(true);
    this->addSubview(m_pTextView);
    
    this->setScale(1 /1.23f);
    
    return true;
}

void CABadgeView::setBadgeText(const std::string& text)
{
    this->setVisible(!text.empty());
    
    m_pTextView->setLayout(DLayout(DHorizontalLayout_W_C(180, 0.5f), DVerticalLayout_H_C(46, 0.5f)));
    m_pTextView->setText(text);
    
    int width = CAImage::getStringWidth("", 30, text);
    if (width > 30)
    {
        width += 16;
    }
    width = MIN(width, 196);
    width = MAX(width, 46);

    m_pBackground->setLayout(DLayout(DHorizontalLayout_W_C(width, 0.5f), DVerticalLayout_H_C(46, 0.5f)));
}

#pragma CATabBar

CATabBar::CATabBar(bool clearance)
:m_pContentView(NULL)
,m_pBackgroundView(NULL)
,m_pSelectedIndicatorView(NULL)
,m_pBackgroundImage(NULL)
,m_sBackgroundColor(CAColor_white)
,m_pSelectedBackgroundImage(NULL)
,m_sSelectedBackgroundColor(CAColor_white)
,m_pSelectedIndicatorImage(NULL)
,m_sSelectedIndicatorColor(CAColor_white)
,m_pSelectedItem(NULL)
,m_cItemSize(DSizeZero)
,m_nSelectedIndex(-1)
,m_bSelectedTitleBold(false)
,m_bShowIndicator(false)
,m_pDelegate(NULL)
,m_bClearance(clearance)
{
    const CAThemeManager::stringMap& map = CAApplication::getApplication()->getThemeManager()->getThemeMap("CATabBar");
    m_pBackgroundImage = CAImage::create(map.at("backgroundView_normal"));
    CC_SAFE_RETAIN(m_pBackgroundImage);
    
    m_pSelectedBackgroundImage = CAImage::create(map.at("backgroundView_selected"));
    CC_SAFE_RETAIN(m_pSelectedBackgroundImage);
    
    m_pSelectedIndicatorImage = CAImage::create(map.at("bottomLine"));
    CC_SAFE_RETAIN(m_pSelectedIndicatorImage);
    
    m_sTitleColor = ccc4Int(CrossApp::hex2Int(map.at("titleColor_normal")));
    m_sSelectedTitleColor = ccc4Int(CrossApp::hex2Int(map.at("titleColor_selected")));
}

CATabBar::~CATabBar()
{
    std::vector<CATabBarItem*>::iterator itr;
    for (itr=m_pItems.begin(); itr!=m_pItems.end(); itr++)
    {
        (*itr)->autorelease();
    }
    m_pItems.clear();
    m_pButtons.clear();
    m_pBadgeViews.clear();
    CC_SAFE_RELEASE_NULL(m_pBackgroundImage);
    CC_SAFE_RELEASE_NULL(m_pSelectedBackgroundImage);
    CC_SAFE_RELEASE_NULL(m_pSelectedIndicatorImage);
    CC_SAFE_RELEASE_NULL(m_pSelectedBackgroundImage);
    CC_SAFE_RELEASE_NULL(m_pSelectedIndicatorImage);
}

CATabBar* CATabBar::createWithFrame(const DRect& rect, bool clearance)
{
    CATabBar* tabBar = new CATabBar(clearance);
    if (tabBar && tabBar->initWithFrame(rect))
    {
        tabBar->autorelease();
        return tabBar;
    }
    CC_SAFE_DELETE(tabBar);
    return NULL;
}
CATabBar* CATabBar::createWithCenter(const DRect& rect, bool clearance)
{
    CATabBar* tabBar = new CATabBar(clearance);
    if (tabBar && tabBar->initWithCenter(rect))
    {
        tabBar->autorelease();
        return tabBar;
    }
    CC_SAFE_DELETE(tabBar);
    return NULL;
}

CATabBar* CATabBar::createWithLayout(const CrossApp::DLayout &layout, bool clearance)
{
    CATabBar* tabBar = new CATabBar(clearance);
    if (tabBar && tabBar->initWithLayout(layout))
    {
        tabBar->autorelease();
        return tabBar;
    }
    CC_SAFE_DELETE(tabBar);
    return NULL;
}

bool CATabBar::init()
{
    m_pContentView = new CAView();
    this->addSubview(m_pContentView);
    m_pContentView->release();
    
    this->enabledTopShadow(true);
    return true;
}

void CATabBar::setItems(const CAVector<CATabBarItem*>& items)
{
    CC_RETURN_IF(items.empty());
	m_pItems = items;
    
    unsigned int count = (unsigned int)m_pItems.size();
    m_cItemSize = m_pContentView->getBounds().size;
    m_cItemSize.width = MIN(m_cItemSize.width, 1024) / count;
    
    if (m_pButtons.empty())
    {
        for (unsigned int i=0; i<count; i++)
        {
            DRect rect = DRectZero;
            rect.size = m_cItemSize;
            rect.origin.x = m_cItemSize.width * i;
            
            CAButton* btn = CAButton::createWithFrame(rect, CAButtonTypeCustom);
            m_pContentView->addSubview(btn);
            btn->setTouchEventScrollHandOverToSuperview(false);
            btn->setTag(i);
            btn->addTarget(this, CAControl_selector(CATabBar::setTouchSelected), CAControlEventTouchUpInSide);
            m_pButtons.pushBack(btn);
            btn->setTitleForState(CAControlStateAll, m_pItems.at(i)->getTitle());
            btn->setTitleColorForState(CAControlStateAll, m_sTitleColor);
            btn->setTitleColorForState(CAControlStateHighlighted, m_sSelectedTitleColor);
            btn->setTitleColorForState(CAControlStateSelected, m_sSelectedTitleColor);
            btn->setImageForState(CAControlStateNormal, m_pItems.at(i)->getImage());
            CAImage* selectedImage = m_pItems.at(i)->getSelectedImage()
            ? m_pItems.at(i)->getSelectedImage()
            : m_pItems.at(i)->getImage();
            btn->setImageForState(CAControlStateHighlighted, selectedImage);
            btn->setImageForState(CAControlStateSelected, selectedImage);
            btn->setBackgroundViewForState(CAControlStateNormal, CAView::createWithColor(CAColor_clear));
            if (m_pSelectedBackgroundImage)
            {
                btn->setBackgroundViewForState(CAControlStateHighlighted,
                                               CAScale9ImageView::createWithImage(m_pSelectedBackgroundImage));
                btn->setBackgroundViewForState(CAControlStateSelected,
                                               CAScale9ImageView::createWithImage(m_pSelectedBackgroundImage));
            }
            else
            {
                btn->setBackgroundViewForState(CAControlStateHighlighted,
                                               CAView::createWithColor(m_sSelectedBackgroundColor));
                btn->setBackgroundViewForState(CAControlStateSelected,
                                               CAView::createWithColor(m_sSelectedBackgroundColor));
            }
            btn->setAllowsSelected(true);
            
            DRect badgeRect;
            badgeRect.origin = rect.origin + DPoint(rect.size.width - 55, 25);
            
            CABadgeView* badgeView = new CABadgeView();
            badgeView->init();
            badgeView->setCenter(badgeRect);
            m_pContentView->insertSubview(badgeView, 10);
            badgeView->setBadgeText(m_pItems.at(i)->getBadgeValue());
            m_pBadgeViews.pushBack(badgeView);
            badgeView->release();
        }
    }
    

    this->setBackgroundImage(m_pBackgroundImage);
    this->setSelectedBackgroundImage(m_pSelectedBackgroundImage);
    this->setSelectedIndicatorImage(m_pSelectedIndicatorImage);
    
}

void CATabBar::onEnterTransitionDidFinish()
{
    CAView::onEnterTransitionDidFinish();
}

void CATabBar::onExitTransitionDidStart()
{
    CAView::onExitTransitionDidStart();
}

void CATabBar::setContentSize(const DSize & var)
{
    CAView::setContentSize(var);
    
    DRect rect = this->getBounds();
    rect.size.width = MIN(rect.size.width, 1024);
    rect.origin.x = (this->getBounds().size.width - rect.size.width) / 2;
    rect.origin.y = m_bClearance ? 40 : 0;
    rect.size.height = rect.size.height - rect.origin.y;
    
    m_pContentView->setFrame(rect);
    
    unsigned int count = (unsigned int)m_pItems.size();
    m_cItemSize = rect.size;
    m_cItemSize.width /= count;
    for (unsigned int i=0; i<count; i++)
    {
        DRect rect = DRectZero;
        rect.size = m_cItemSize;
        rect.origin.x = m_cItemSize.width * i;
        rect.origin.y = 0;
        m_pButtons.at(i)->setFrame(rect);
        
        DRect badgeRect;
        badgeRect.origin = rect.origin + DPoint(rect.size.width - 55, 25);
        m_pBadgeViews.at(i)->setCenter(badgeRect);
    }
    
    if (m_pSelectedIndicatorView)
    {
        DRect rect;
        rect.size.width = m_cItemSize.width;
        rect.size.height = 8;
        rect.origin.x = m_nSelectedIndex * m_cItemSize.width;
        rect.origin.y = m_cItemSize.height - rect.size.height;
        m_pSelectedIndicatorView->setFrame(rect);
    }
}

void CATabBar::replaceItemAtIndex(size_t index, CATabBarItem* item)
{
    if (index < m_pItems.size())
    {
		m_pItems.replace(index, item);
        if (!m_pButtons.empty())
        {
            CAButton* btn = m_pButtons.at(index);
            btn->setTitleForState(CAControlStateAll, item->getTitle());
            btn->setImageForState(CAControlStateNormal, item->getImage());
            CAImage* selectedImage = item->getSelectedImage()
            ? item->getSelectedImage()
            : item->getImage();
            btn->setImageForState(CAControlStateHighlighted, selectedImage);
            btn->setImageForState(CAControlStateSelected, selectedImage);

            CABadgeView* badgeView = m_pBadgeViews.at(index);
            badgeView->setBadgeText(item->getBadgeValue());
        }
    }
}

DRect CATabBar::getContentViewFrame()
{
    return m_pContentView->getFrame();
}

void CATabBar::setBackgroundImage(CrossApp::CAImage *var)
{
    CC_SAFE_RETAIN(var);
    CC_SAFE_RELEASE_NULL(m_pBackgroundImage);
    m_pBackgroundImage = var;
    m_sBackgroundColor = CAColor_white;
    this->showBackground();
}

CAImage* CATabBar::getBackgroundImage()
{
    return m_pBackgroundImage;
}

void CATabBar::setBackgroundColor(const CAColor4B &var)
{
    m_sBackgroundColor = var;
    CC_SAFE_RELEASE_NULL(m_pBackgroundImage);
    this->showBackground();
}

const CAColor4B& CATabBar::getBackgroundColor()
{
    return m_sBackgroundColor;
}

void CATabBar::setSelectedBackgroundImage(CrossApp::CAImage *var)
{
    CC_SAFE_RETAIN(var);
    CC_SAFE_RELEASE_NULL(m_pSelectedBackgroundImage);
    m_pSelectedBackgroundImage = var;
    m_sSelectedBackgroundColor = CAColor_white;
    
    this->showSelectedBackground();
}

CAImage* CATabBar::getSelectedBackgroundImage()
{
    return m_pSelectedBackgroundImage;
}

void CATabBar::setSelectedBackgroundColor(const CAColor4B &var)
{
    m_sSelectedBackgroundColor = var;
    CC_SAFE_RELEASE_NULL(m_pSelectedBackgroundImage);
    this->showSelectedBackground();
}

const CAColor4B& CATabBar::getSelectedBackgroundColor()
{
    return m_sSelectedBackgroundColor;
};

void CATabBar::setSelectedIndicatorImage(CrossApp::CAImage *var)
{
    CC_SAFE_RETAIN(var);
    CC_SAFE_RELEASE_NULL(m_pSelectedIndicatorImage);
    m_pSelectedIndicatorImage = var;
    m_sSelectedIndicatorColor = CAColor_white;
    CC_RETURN_IF(var == NULL);
    
    this->showSelectedIndicatorView();
}

CAImage* CATabBar::getSelectedIndicatorImage()
{
    return m_pSelectedIndicatorImage;
}


void CATabBar::setSelectedIndicatorColor(const CAColor4B &var)
{
    m_sSelectedIndicatorColor = var;
    CC_SAFE_RELEASE_NULL(m_pSelectedIndicatorImage);
    
    this->showSelectedIndicatorView();
}

const CAColor4B& CATabBar::getSelectedIndicatorColor()
{
    return m_sSelectedIndicatorColor;
}

void CATabBar::setTitleColorForNormal(const CAColor4B &var)
{
    m_sTitleColor = var;
    if (!m_pButtons.empty())
    {
        for (size_t i=0; i<m_pButtons.size(); i++)
        {
            CAButton* btn = m_pButtons.at(i);
            btn->setTitleColorForState(CAControlStateNormal, m_sTitleColor);
        }
    }
}

const CAColor4B& CATabBar::getTitleColorForNormal()
{
    return m_sTitleColor;
}

void CATabBar::setTitleColorForSelected(const CAColor4B &var)
{
    m_sSelectedTitleColor = var;
    if (!m_pButtons.empty())
    {
        for (size_t i=0; i<m_pButtons.size(); i++)
        {
            CAButton* btn = m_pButtons.at(i);
            btn->setTitleColorForState(CAControlStateHighlighted, m_sSelectedTitleColor);
            btn->setTitleColorForState(CAControlStateSelected, m_sSelectedTitleColor);
        }
    }
}

const CAColor4B& CATabBar::getTitleColorForSelected()
{
    return m_sSelectedTitleColor;
}

void CATabBar::setTitleBoldForSelected(bool var)
{
    m_bSelectedTitleBold = var;
    if (!m_pButtons.empty())
    {
        for (size_t i=0; i<m_pButtons.size(); i++)
        {
            CAButton* btn = m_pButtons.at(i);
            if (i == m_nSelectedIndex)
            {
                btn->setTitleBold(var);
            }
            else
            {
                btn->setTitleBold(false);
            }
        }
    }
}

bool CATabBar::getTitleBoldForSelected()
{
    return m_bSelectedTitleBold;
}

void CATabBar::showBackground()
{
    this->removeSubview(m_pBackgroundView);
    
    if (m_pBackgroundImage)
    {
        m_pBackgroundView = CAScale9ImageView::createWithImage(m_pBackgroundImage);
    }
    else
    {
        m_pBackgroundView = CAView::createWithColor(m_sBackgroundColor);
    }
    m_pBackgroundView->setLayout(DLayout(DHorizontalLayout_L_R(0, 0), DVerticalLayout_T_B(0, 0)));
    this->insertSubview(m_pBackgroundView, -1);
}

void CATabBar::showSelectedBackground()
{
    for (size_t i=0; i<m_pButtons.size(); i++)
    {
        CAButton* btn = m_pButtons.at(i);
        btn->setTitleForState(CAControlStateAll, m_pItems.at(i)->getTitle());
        btn->setTitleColorForState(CAControlStateAll, m_sTitleColor);
        btn->setTitleColorForState(CAControlStateHighlighted, m_sSelectedTitleColor);
        btn->setTitleColorForState(CAControlStateSelected, m_sSelectedTitleColor);
        btn->setImageForState(CAControlStateNormal, m_pItems.at(i)->getImage());
        CAImage* selectedImage = m_pItems.at(i)->getSelectedImage()
        ? m_pItems.at(i)->getSelectedImage()
        : m_pItems.at(i)->getImage();
        btn->setImageForState(CAControlStateHighlighted, selectedImage);
        btn->setImageForState(CAControlStateSelected, selectedImage);
        btn->setBackgroundViewForState(CAControlStateNormal, CAView::createWithColor(CAColor_clear));
        if (m_pSelectedBackgroundImage)
        {
            btn->setBackgroundViewForState(CAControlStateHighlighted,
                                           CAScale9ImageView::createWithImage(m_pSelectedBackgroundImage));
            btn->setBackgroundViewForState(CAControlStateSelected,
                                           CAScale9ImageView::createWithImage(m_pSelectedBackgroundImage));
        }
        else
        {
            btn->setBackgroundViewForState(CAControlStateHighlighted,
                                           CAView::createWithColor(m_sSelectedBackgroundColor));
            btn->setBackgroundViewForState(CAControlStateSelected,
                                           CAView::createWithColor(m_sSelectedBackgroundColor));
        }
        btn->setAllowsSelected(true);
        
        CABadgeView* badgeView = m_pBadgeViews.at(i);
        badgeView->setBadgeText(m_pItems.at(i)->getBadgeValue());
    }
}

void CATabBar::showSelectedIndicatorView()
{
    m_pContentView->removeSubview(m_pSelectedIndicatorView);
    if (m_pSelectedIndicatorImage)
    {
        CAScale9ImageView* imageView = CAScale9ImageView::createWithImage(m_pSelectedIndicatorImage);
        DRect insetRect;
        insetRect.origin = m_pSelectedIndicatorImage->getContentSize() / 2;
        insetRect.origin = ccpSub(insetRect.origin, DPoint(1, 1));
        insetRect.size = DPoint(2, 2);
        imageView->setCapInsets(insetRect);
        m_pSelectedIndicatorView = imageView;
    }
    else
    {
        m_pSelectedIndicatorView = CAView::createWithColor(m_sSelectedIndicatorColor);
    }
    
    DRect rect;
    rect.size.width = m_cItemSize.width;
    rect.size.height = 8;
    rect.origin.x = m_nSelectedIndex * m_cItemSize.width;
    rect.origin.y = m_cItemSize.height - rect.size.height;
    m_pSelectedIndicatorView->setFrame(rect);
    m_pContentView->insertSubview(m_pSelectedIndicatorView, 1);
    m_pSelectedIndicatorView->setVisible(m_bShowIndicator);
}

void CATabBar::showSelectedIndicator()
{
    m_bShowIndicator = true;
    if (m_pSelectedIndicatorView)
    {
        m_pSelectedIndicatorView->setVisible(true);
    }
}

void CATabBar::setSelectedAtIndex(int index)
{
    CC_RETURN_IF(index < -1);
    CC_RETURN_IF((size_t)index >= m_pItems.size());
    
    if (m_nSelectedIndex != -1)
    {
        m_pButtons.at(m_nSelectedIndex)->setControlStateNormal();
    }
    
    if (index != -1)
    {
        m_pButtons.at(index)->setControlStateSelected();
        for (size_t i=0; i<m_pButtons.size(); i++)
        {
            CAButton* btn = m_pButtons.at(i);
            if (i == index)
            {
                btn->setTitleBold(m_bSelectedTitleBold);
            }
            else
            {
                btn->setTitleBold(false);
            }
        }
        m_nSelectedIndex = index;
        m_pSelectedItem = m_pItems.at(m_nSelectedIndex);
        
        if (m_pSelectedIndicatorView)
        {
            m_pSelectedIndicatorView->setVisible(m_bShowIndicator);
            DRect rect;
            rect.size.width = m_cItemSize.width;
            rect.size.height = 8;
            rect.origin.x = m_nSelectedIndex * m_cItemSize.width;
            rect.origin.y = m_cItemSize.height - rect.size.height;
            
            CAViewAnimation::beginAnimations("", NULL);
            CAViewAnimation::setAnimationDuration(0.3f);
            CAViewAnimation::setAnimationCurve(CAViewAnimationCurveEaseOut);
            m_pSelectedIndicatorView->setFrame(rect);
            CAViewAnimation::commitAnimations();
        }
    }
    else
    {
        m_pSelectedIndicatorView->setVisible(false);
    }
}

void CATabBar::addForbidSelectedAtIndex(int index)
{
    m_sForbidSelectedIndexs.insert(index);
}

void CATabBar::setTouchSelected(CrossApp::CAControl *control, CrossApp::DPoint point)
{
    int index = control->getTag();
    if (!m_sForbidSelectedIndexs.count(index))
    {
        this->setSelectedAtIndex(control->getTag());
        
        if (m_pDelegate)
        {
            m_pDelegate->tabBarSelectedItem(this, m_pSelectedItem, m_nSelectedIndex);
        }
    }
    else
    {
        if (m_pDelegate)
        {
            m_pDelegate->tabBarClickToForbidSelectedItem(this, m_pSelectedItem, m_nSelectedIndex);
        }
    }
    
}


NS_CC_END
