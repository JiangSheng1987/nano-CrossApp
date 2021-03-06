
#include "CGSpriteBatchNode.h"
#include "ccConfig.h"
#include "CGSprite.h"
#include "view/CADrawingPrimitives.h"
#include "support/CAPointExtension.h"
#include "shaders/CAShaderCache.h"
#include "shaders/CAGLProgram.h"
#include "shaders/ccGLStateCache.h"
#include "basics/CAApplication.h"
#include "math/TransformUtils.h"
#include "kazmath/GL/matrix.h"

NS_CC_BEGIN


CGSpriteBatchNode* CGSpriteBatchNode::createWithImage(CAImage* image, unsigned int capacity/* = kDefaultSpriteBatchCapacity*/)
{
    CGSpriteBatchNode *batchNode = new CGSpriteBatchNode();
    batchNode->initWithImage(image, capacity);
    batchNode->autorelease();

    return batchNode;
}

/*
* creation with File Image
*/

CGSpriteBatchNode* CGSpriteBatchNode::create(const std::string& fileImage, unsigned int capacity/* = kDefaultSpriteBatchCapacity*/)
{
    CGSpriteBatchNode *batchNode = new CGSpriteBatchNode();
    batchNode->initWithFile(fileImage, capacity);
    batchNode->autorelease();

    return batchNode;
}

/*
* init with CAImage
*/
bool CGSpriteBatchNode::initWithImage(CAImage *image, unsigned int capacity)
{
    m_blendFunc.src = CC_BLEND_SRC;
    m_blendFunc.dst = CC_BLEND_DST;
    m_pobImageAtlas = new CAImageAtlas();

    if (0 == capacity)
    {
        capacity = kDefaultSpriteBatchCapacity;
    }
    
    m_pobImageAtlas->initWithImage(image, capacity);

    updateBlendFunc();

    setShaderProgram(CAShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor));
    return true;
}

bool CGSpriteBatchNode::init()
{
    CAImage * image = new CAImage();
    image->autorelease();
    return this->initWithImage(image, 0);
}

/*
* init with FileImage
*/
bool CGSpriteBatchNode::initWithFile(const std::string& fileImage, unsigned int capacity)
{
    return initWithImage(CAImage::create(fileImage), capacity);
}

CGSpriteBatchNode::CGSpriteBatchNode()
: m_pobImageAtlas(nullptr)
{
}

CGSpriteBatchNode::~CGSpriteBatchNode()
{
    CC_SAFE_RELEASE(m_pobImageAtlas);
}

// override visit
// don't call visit on it's children
void CGSpriteBatchNode::visit(void)
{
    CC_PROFILER_START_CATEGORY(kCCProfilerCategoryBatchSprite, "CGSpriteBatchNode - visit");

    CC_RETURN_IF(!m_bVisible);
    
    kmGLPushMatrix();
    
    this->transform();
    this->sortAllChildren();
    this->draw();
    
    for (auto& var : m_obChildren)
        var->visit();

    if (m_pCAView)
    {
        m_pCAView->visit();
    }
    
    kmGLPopMatrix();

    CC_PROFILER_STOP_CATEGORY(kCCProfilerCategoryBatchSprite, "CGSpriteBatchNode - visit");
}

void CGSpriteBatchNode::insertChild(CGNode* child, int z)
{
    if (CGSprite* sprite = dynamic_cast<CGSprite*>(child))
    {
        CGNode::insertChild(sprite, z);
        
        this->appendChild(sprite);
    }
}

void CGSpriteBatchNode::addChild(CGNode* child)
{
    this->insertChild(child, child->getZOrder());
}

void CGSpriteBatchNode::removeChild(CGNode* child)
{
    if (CGSprite* sprite = dynamic_cast<CGSprite*>(child))
    {
        this->removeSpriteFromAtlas(sprite);
        
        CGNode::removeChild(sprite);
    }
}

void CGSpriteBatchNode::removeAllChildren()
{
    CGNode::removeAllChildren();
    m_pobImageAtlas->removeAllQuads();
    m_obDescendants.clear();
}

void CGSpriteBatchNode::sortAllChildren()
{
    if (m_bReorderChildDirty)
    {
        std::sort(m_obChildren.begin(), m_obChildren.end(), compareChildrenZOrder);
        
        if (!m_obChildren.empty())
        {
            CAVector<CGNode*>::iterator itr;
            
            for(const auto &child: m_obChildren)
                child->sortAllChildren();
            
            ssize_t index = 0;
            for(const auto &child: m_obChildren)
            {
                CGSprite* sp = static_cast<CGSprite*>(child);
                updateAtlasIndex(sp, &index);
            }
            
        }
        m_bReorderChildDirty = false;
    }
}

void CGSpriteBatchNode::updateAtlasIndex(CGSprite* sprite, ssize_t* curIndex)
{
    auto& array = sprite->getChildren();
    auto count = array.size();
    
    ssize_t oldIndex = 0;
    
    if( count == 0 )
    {
        oldIndex = sprite->getAtlasIndex();
        sprite->setAtlasIndex(*curIndex);
        if (oldIndex != *curIndex)
        {
            swap(oldIndex, *curIndex);
        }
        (*curIndex)++;
    }
    else
    {
        bool needNewIndex = true;
        
        if (array.front()->getZOrder() >= 0)
        {
            oldIndex = sprite->getAtlasIndex();
            sprite->setAtlasIndex(*curIndex);
            if (oldIndex != *curIndex)
            {
                swap(oldIndex, *curIndex);
            }
            (*curIndex)++;
            
            needNewIndex = false;
        }
        
        for (const auto &child: array)
        {
            CGSprite* sp = static_cast<CGSprite*>(child);
            if (needNewIndex && sp->getZOrder() >= 0)
            {
                oldIndex = sprite->getAtlasIndex();
                sprite->setAtlasIndex(*curIndex);
                if (oldIndex != *curIndex)
                {
                    this->swap(oldIndex, *curIndex);
                }
                (*curIndex)++;
                needNewIndex = false;
                
            }
            
            updateAtlasIndex(sp, curIndex);
        }
        
        if (needNewIndex)
        {
            oldIndex = sprite->getAtlasIndex();
            sprite->setAtlasIndex(*curIndex);
            if (oldIndex!=*curIndex)
            {
                swap(oldIndex, *curIndex);
            }
            (*curIndex)++;
        }
    }
}

void CGSpriteBatchNode::swap(ssize_t oldIndex, ssize_t newIndex)
{
    ccV3F_C4B_T2F_Quad* quads = m_pobImageAtlas->getQuads();
    std::swap( quads[oldIndex], quads[newIndex] );
    
    auto oldIt = std::next( m_obDescendants.begin(), oldIndex );
    auto newIt = std::next( m_obDescendants.begin(), newIndex );
    
    (*newIt)->setAtlasIndex(oldIndex);
    //    (*oldIt)->setAtlasIndex(newIndex);
    
    std::swap( *oldIt, *newIt );
}

void CGSpriteBatchNode::reorderBatch(bool reorder)
{
    m_bReorderChildDirty = reorder;
}

void CGSpriteBatchNode::draw(void)
{
    CC_PROFILER_START("CGSpriteBatchNode - draw");

    CC_RETURN_IF (m_pobImageAtlas->getTotalQuads() == 0);

    CAIMAGE_DRAW_SETUP();

    for (const auto& child : m_obChildren)
        child->updateTransform();
    
    ccGLBlendFunc( m_blendFunc.src, m_blendFunc.dst );

    m_pobImageAtlas->drawQuads();

    CC_PROFILER_STOP("CGSpriteBatchNode - draw");
}

void CGSpriteBatchNode::increaseAtlasCapacity(void)
{
    unsigned int quantity = (m_pobImageAtlas->getCapacity() + 1) * 4 / 3;

    if (! m_pobImageAtlas->resizeCapacity(quantity))
    {
        CCAssert(false, "Not enough memory to resize the atlas");
    }
}

ssize_t CGSpriteBatchNode::rebuildIndexInOrder(CGSprite *parent, ssize_t index)
{
    auto& children = parent->getChildren();
    for(const auto &child: children)
    {
        CGSprite* sp = static_cast<CGSprite*>(child);
        if (sp && (sp->getZOrder() < 0))
        {
            index = rebuildIndexInOrder(sp, index);
        }
    }
    
    // ignore self (batch node)
    if (parent != static_cast<CAObject*>(this))
    {
        parent->setAtlasIndex(index);
        index++;
    }
    
    for(const auto &child: children)
    {
        CGSprite* sp = static_cast<CGSprite*>(child);
        if (sp && (sp->getZOrder() >= 0))
        {
            index = rebuildIndexInOrder(sp, index);
        }
    }
    
    return index;
}

ssize_t CGSpriteBatchNode::highestAtlasIndexInChild(CGSprite *pSprite)
{
    auto& children = pSprite->getChildren();

    if (children.empty())
    {
        return pSprite->getAtlasIndex();
    }
    else
    {
        return highestAtlasIndexInChild(static_cast<CGSprite*>(children.back()));
    }
}

ssize_t CGSpriteBatchNode::lowestAtlasIndexInChild(CGSprite *pSprite)
{
    auto& children = pSprite->getChildren();

    if (children.empty())
    {
        return pSprite->getAtlasIndex();
    }
    else
    {
        return lowestAtlasIndexInChild(static_cast<CGSprite*>(children.back()));
    }
}

ssize_t CGSpriteBatchNode::atlasIndexForChild(CGSprite *sprite, int nZ)
{
    auto& pBrothers = sprite->getParent()->getChildren();
    ssize_t childIndex = pBrothers.getIndex(sprite);

    bool bIgnoreParent = (CGSpriteBatchNode*)(sprite->getParent()) == this;
    CGSprite *prev = NULL;
    if (childIndex > 0 && childIndex != -1)
    {
        prev = (CGSprite*)( pBrothers.at(childIndex - 1) );
    }

    if (bIgnoreParent)
    {
        if (childIndex == 0)
        {
            return 0;
        }

        return highestAtlasIndexInChild(prev) + 1;
    }

    if (childIndex == 0)
    {
        CGSprite *p = (CGSprite*)(sprite->getParent());

        if (nZ < 0)
        {
            return p->getAtlasIndex();
        }
        else
        {
            return p->getAtlasIndex() + 1;
        }
    }
    else
    {
        if ((prev->getZOrder() < 0 && nZ < 0) || (prev->getZOrder() >= 0 && nZ >= 0))
        {
            return highestAtlasIndexInChild(prev) + 1;
        }

        CGSprite *p = (CGSprite*)(sprite->getParent());
        return p->getAtlasIndex() + 1;
    }

    CCAssert(0, "should not run here");
    return 0;
}

void CGSpriteBatchNode::appendChild(CGSprite* sprite)
{
    m_bReorderChildDirty=true;
    sprite->setBatchNode(this);
    sprite->setDirty(true);

    if(m_pobImageAtlas->getTotalQuads() == m_pobImageAtlas->getCapacity())
    {
        increaseAtlasCapacity();
    }

    m_obDescendants.pushBack(sprite);

    ssize_t index = static_cast<ssize_t>(m_obDescendants.size() - 1);
    sprite->setAtlasIndex(index);

    ccV3F_C4B_T2F_Quad quad = sprite->getQuad();
    m_pobImageAtlas->insertQuad(&quad, index);
    
    auto& children = sprite->getChildren();
    for(const auto &child: children)
    {
        appendChild(static_cast<CGSprite*>(child));
    }
}

void CGSpriteBatchNode::removeSpriteFromAtlas(CGSprite *sprite)
{
    m_pobImageAtlas->removeQuadAtIndex(sprite->getAtlasIndex());

    sprite->setBatchNode(nullptr);

    auto it = std::find(m_obDescendants.begin(), m_obDescendants.end(), sprite );
    if( it != m_obDescendants.end() )
    {
        auto next = std::next(it);
        
        CGSprite *spr = nullptr;
        for(; next != m_obDescendants.end(); ++next)
        {
            spr = *next;
            spr->setAtlasIndex(spr->getAtlasIndex() - 1);
        }
        
        m_obDescendants.erase(it);
    }
    
    auto& children = sprite->getChildren();
    for(const auto &obj: children)
    {
        CGSprite* child = static_cast<CGSprite*>(obj);
        if (child)
        {
            removeSpriteFromAtlas(child);
        }
    }
}

void CGSpriteBatchNode::updateBlendFunc(void)
{
    if (! m_pobImageAtlas->getImage()->hasPremultipliedAlpha())
    {
        m_blendFunc.src = GL_SRC_ALPHA;
        m_blendFunc.dst = GL_ONE_MINUS_SRC_ALPHA;
    }
    else
    {
        m_blendFunc.src = CC_BLEND_SRC;
        m_blendFunc.dst = CC_BLEND_DST;
    }
}

void CGSpriteBatchNode::setBlendFunc(BlendFunc blendFunc)
{
    m_blendFunc = blendFunc;
}

BlendFunc CGSpriteBatchNode::getBlendFunc(void)
{
    return m_blendFunc;
}

CAImage* CGSpriteBatchNode::getImage(void)
{
    return m_pobImageAtlas->getImage();
}

void CGSpriteBatchNode::setImage(CAImage *image)
{
    m_pobImageAtlas->setImage(image);
    updateBlendFunc();
}

void CGSpriteBatchNode::insertQuadFromSprite(CGSprite *sprite, ssize_t index)
{
    CCAssert( sprite != NULL, "Argument must be non-NULL");
    CCAssert( dynamic_cast<CGSprite*>(sprite), "CGSpriteBatchNode only supports CCSprites as children");

    while(index >= m_pobImageAtlas->getCapacity() || m_pobImageAtlas->getCapacity() == m_pobImageAtlas->getTotalQuads())
    {
        this->increaseAtlasCapacity();
    }

    sprite->setBatchNode(this);
    sprite->setAtlasIndex(index);

    ccV3F_C4B_T2F_Quad quad = sprite->getQuad();
    m_pobImageAtlas->insertQuad(&quad, index);

    sprite->setDirty(true);
    sprite->updateTransform();
}

void CGSpriteBatchNode::updateQuadFromSprite(CGSprite *sprite, ssize_t index)
{
    CCAssert(sprite != NULL, "Argument must be non-nil");
    CCAssert(dynamic_cast<CGSprite*>(sprite) != NULL, "CGSpriteBatchNode only supports CCSprites as children");
    
	while (index >= m_pobImageAtlas->getCapacity() || m_pobImageAtlas->getCapacity() == m_pobImageAtlas->getTotalQuads())
    {
		this->increaseAtlasCapacity();
    }

	sprite->setBatchNode(this);
    sprite->setAtlasIndex(index);
	sprite->setDirty(true);
	sprite->updateTransform();
}

CGSpriteBatchNode * CGSpriteBatchNode::addSpriteWithoutQuad(CGSprite*child, ssize_t z, int aTag)
{
    CCAssert( child != NULL, "Argument must be non-NULL");
    CCAssert( dynamic_cast<CGSprite*>(child), "CGSpriteBatchNode only supports CCSprites as children");

    child->setAtlasIndex(z);

    int i=0;
 
    auto it = m_obDescendants.begin();
    for (; it != m_obDescendants.end(); ++it)
        CC_BREAK_IF ((*it)->getAtlasIndex() >= z);

    m_obDescendants.insert(i, child);

    CGNode::insertChild(child, z);
    child->setTag(aTag);
    reorderBatch(false);

    return this;
}

NS_CC_END
