#ifndef XTQUADTREE_H
#define XTQUADTREE_H
#include <memory>
#include "xtrect.h"
#include <unordered_set>
#include <functional>

namespace xtdb {
class XtShape;

template <typename T>
class XtQuadtreeNode
{
public:
    XtRect mZone;
    std::unordered_set<T> mObjs;
    int mLevel;
    static const int maxLevel;
    XtQuadtreeNode<T>* mChildren[4];
    XtRect mChildrenZones[4];
public:
    XtQuadtreeNode(const XtRect& pZone, int pLevel = 0);
    ~XtQuadtreeNode();
    bool contains(const XtRect& pZone) const;
    bool intersects(const XtRect& pZone) const;
    void insert(const T& pObj);
    void search(const XtRect& pZone, const std::function<void(XtShape*)>& pOnShapeFound, bool ONLY_FULLY_CONTAINED);
//    void search(const XtRect& pZone, std::unordered_set<T>& pFoundObjs); //TODO:is QSet<T&> better?
    void getAllObjsOfSubtree(const std::function<void(XtShape*)>& pOnShapeFound);
    void removeObj(const T& pObj);
    void resize(const XtRect& pZone);
    void removeObjsIn(const XtRect& pZone);
    inline bool hasNoChild()
    {
        return !mChildren[0] && !mChildren[1] && !mChildren[2] && !mChildren[3];
    };
};

template <typename T>
const int XtQuadtreeNode<T>::maxLevel = 1;

template <typename T>
XtQuadtreeNode<T>::XtQuadtreeNode(const XtRect& pZone, int pLevel):
    mLevel(pLevel), mZone(pZone),
    mChildrenZones{
        XtRect(mZone.left() + mZone.width()/2, mZone.top(), mZone.width(), mZone.height()/2),
        XtRect(mZone.left(), mZone.top(), mZone.width()/2, mZone.height()/2),
        XtRect(mZone.left(), mZone.top() + mZone.height()/2, mZone.width()/2, mZone.height()),
        XtRect(mZone.left() + mZone.width()/2, mZone.top() + mZone.height()/2, mZone.width(), mZone.height())
}, mChildren{nullptr, nullptr, nullptr, nullptr}
{
}

template <typename T>
void XtQuadtreeNode<T>::resize(const XtRect& pZone)
{

}

template <typename T>
void XtQuadtreeNode<T>::removeObjsIn(const XtRect& pZone)
{
    for (typename std::unordered_set<T>::iterator it = mObjs.begin(); it != mObjs.end(); it++)
    {
        mObjs.erase(*it);
    }

    for (int i = 0; i < 4; i++)
    {
        if (mChildren[i])
        {
            if (pZone.contains(mChildrenZones[i]))
            {
                mChildren[i]->removeAllObjsOfSubtree();
            }
            else if (pZone.intersects(mChildrenZones[i]))
            {
                mChildren[i]->removeObjsIn(pZone);
            }
        }
    }
}

template <typename T>
void XtQuadtreeNode<T>::removeObj(const T& pObj)
{
    mObjs.erase(pObj);
}

template <typename T>
XtQuadtreeNode<T>::~XtQuadtreeNode()
{

}

template <typename T>
bool XtQuadtreeNode<T>::contains(const XtRect& pZone) const
{
    if (mZone.contains(pZone))
    {
        return true;
    }
    else
    {
        return false;
    }
}

template <typename T>
bool XtQuadtreeNode<T>::intersects(const XtRect& pZone) const
{
    if(mZone.intersects(pZone))
    {
        return true;
    }
    else
    {
        return false;
    }
}

template <typename T>
void XtQuadtreeNode<T>::insert(const T& pObj)
{
    if(!contains(pObj->getZone()))
    {
        return;
    }
    if(mLevel == maxLevel)
    {
        mObjs.insert(pObj);
        pObj->mOwnerNode = this;
        return;
    }
    for (int i = 0; i < 4; i++)
    {
        if (mChildrenZones[i].contains(pObj->getZone()))
        {
            if (!mChildren[i])
            {
                mChildren[i] = new XtQuadtreeNode<T>(mChildrenZones[i], mLevel + 1);
            }
            mChildren[i]->insert(pObj);
            return;
        }
    }
    mObjs.insert(pObj);
    pObj->mOwnerNode = this;
    return;
}

template <typename T>
void XtQuadtreeNode<T>::search(const XtRect& pZone, const std::function<void(XtShape*)>& pOnShapeFound, bool ONLY_FULLY_CONTAINED)
{
    for(typename std::unordered_set<T>::const_iterator it = mObjs.cbegin(); it != mObjs.cend(); it++)
    {
        if (ONLY_FULLY_CONTAINED)
        {
            if(pZone.contains((*it)->getZone()))
            {
                XtShape* shape = reinterpret_cast<XtShape*>(*it);
                pOnShapeFound(shape);
            }
        }
        else
        {
            if ((*it)->getZone().intersects(pZone))
            {
                XtShape* shape = reinterpret_cast<XtShape*>(*it);
                pOnShapeFound(shape);
            }
        }
    }
    for (int i = 0; i < 4; i++)
    {
        if (mChildren[i])
        {    
            if(pZone.contains(mChildrenZones[i]))
            {
                mChildren[i]->getAllObjsOfSubtree(pOnShapeFound);
            }
            if(pZone.intersects(mChildrenZones[i]))
            {
                mChildren[i]->search(pZone, pOnShapeFound, ONLY_FULLY_CONTAINED);
            }
        }
    }
}

template <typename T>
void XtQuadtreeNode<T>::getAllObjsOfSubtree(const std::function<void(XtShape*)>& pOnShapeFound)
{
    for (typename std::unordered_set<T>::iterator it = mObjs.begin(); it != mObjs.end(); it++)
    {
        XtShape* shape = reinterpret_cast<XtShape*>(*it);
        pOnShapeFound(shape);
    }
    for (int i = 0; i < 4; i++)
    {
        if (mChildren[i])
        {
            mChildren[i]->getAllObjsOfSubtree(pOnShapeFound);
        }
    }
}

template <typename T>
class XtQuadtree
{
public:
    XtQuadtreeNode<T>* mRoot;
    XtQuadtree();
    ~XtQuadtree();
    void insert(const T& pObj);
    void search(const XtRect& pZone, const std::function<void(XtShape*)>& pOnShapeFound, bool ONLY_FULLY_CONTAINED = false);
//    void search(const XtRect& pZone, std::unordered_set<T>& pFoundObjs);
    void clear(XtQuadtreeNode<T>* pQuadtreeNode);
};

template <typename T>
XtQuadtree<T>::XtQuadtree():mRoot(new XtQuadtreeNode<T>(XtRect(0, 0, 10000, 10000)))
{

}

template <typename T>
XtQuadtree<T>::~XtQuadtree()
{
    clear(mRoot);
}

template <typename T>
void XtQuadtree<T>::insert(const T& pObj)
{
    mRoot->insert(pObj);
}

template <typename T>
void XtQuadtree<T>::search(const XtRect& pZone, const std::function<void(XtShape*)>& pOnShapeFound, bool ONLY_FULLY_CONTAINED)
{
    mRoot->search(pZone, pOnShapeFound, ONLY_FULLY_CONTAINED);
}

template <typename T>
void XtQuadtree<T>::clear(XtQuadtreeNode<T>* pNode)
{
    if (pNode)
    {
        for (int i = 0; i < 4; i++)
        {
            if (pNode->mChildren[i])
            {
                clear(pNode->mChildren[i]);
            }
        }
        delete pNode;
        pNode = nullptr;
    }
}
}
#endif // XTQUADTREE_H
