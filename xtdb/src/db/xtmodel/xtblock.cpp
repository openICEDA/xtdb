#include "xtblock.h"
#include "_xtblock.h"
#include "_xtrectangle.h"
#include "xttable.h"
#include "xtstream.h"
#include <fstream>
#include "xtdm.h"
#include "xtinst.h"
#include "_xtinst.h"
#include "_xtwireseg.h"
#include "_xtportinst.h"
#include "_xtport.h"
#include "_xtvia.h"
#include "_xtline.h"
#include "_xtrectangle.h"
#include "_xtnet.h"
#include "xtpccontainer.h"

namespace xtdb {
xtobject_kind _XtBlock::mKind = xtobject_kind::BLOCK;
XtTable<_XtBlock> _XtBlock::blockTbl;
XtHashTable<_XtBlock> _XtBlock::blockHashTbl(&blockTbl);

_XtBlock::_XtBlock():mName(nullptr), mNext(0),
    mDoubleLLPrev(0), mDoubleLLNext(0),
    mNetTbl(new XtTable<_XtNet>(this)),
    mInstTbl(new XtTable<_XtInst>(this)),
    mWireSegTbl(new XtTable<_XtWireSeg>(this)),
    mPortInstTbl(new XtTable<_XtPortInst>(this)),
    mPortTbl(new XtTable<_XtPort>(this)),
    mViaTbl(new XtTable<_XtVia>(this)),
    mLineTbl(new XtTable<_XtLine>(this)),
    mRectTbl(new XtTable<_XtRectangle>(this)),
    mQuadtree(new XtQuadtree<_XtRectangle*>()),
    mNetWireSegContainer(new XtPCContainer<_XtNet, _XtWireSeg>(mWireSegTbl))
{

}

_XtBlock::~_XtBlock()
{
    delete mNetTbl;
    delete mInstTbl;
    delete mWireSegTbl;
    delete mPortInstTbl;
    delete mPortTbl;
    delete mViaTbl;
    delete mLineTbl;
    delete mRectTbl;
    delete mQuadtree;
    delete mNetWireSegContainer;
}

bool _XtBlock::operator==(const _XtBlock& pRhs) const
{   //TODO: compare more tables
    return (*mNetTbl == *pRhs.mNetTbl) && (*mInstTbl == *pRhs.mInstTbl) &&
            (*mWireSegTbl == *pRhs.mWireSegTbl) && (*mPortInstTbl == *pRhs.mPortInstTbl) &&
            (*mPortTbl == *pRhs.mPortTbl) && (*mViaTbl == *pRhs.mViaTbl) &&
            (*mRectTbl == *pRhs.mRectTbl) && (*mLineTbl == *pRhs.mLineTbl);
}

XtSet<XtShape*> XtBlock::getAllShapes()
{
    //TODO: 定义一个新的数据结构可以串联起多个table，也可以串联起多个自己。这样就可以串联recttable，triangletable...
    return XtSet<XtShape*>(reinterpret_cast<_XtBlock*>(this)->mRectTbl);
}

XtSet<XtInst*> XtBlock::getAllInsts()
{
    return XtSet<XtInst*>(reinterpret_cast<_XtBlock*>(this)->mInstTbl);
}

XtOStream& operator<<(XtOStream& pOS, _XtBlock& pBlock)
{
    pOS << *pBlock.mInstTbl;
    pOS << *pBlock.mNetTbl;
    pOS << *pBlock.mWireSegTbl;
//    pOS << *pBlock.mPortInstTbl;
//    pOS << *pBlock.mPortTbl;
//    pOS << *pBlock.mViaTbl;
    pOS << *pBlock.mLineTbl;
    pOS << *pBlock.mRectTbl;
    return pOS;
}

XtIStream& operator>>(XtIStream& pIS, _XtBlock& pBlock)
{
    pIS >> *pBlock.mInstTbl;
    pIS >> *pBlock.mNetTbl;
    pIS >> *pBlock.mWireSegTbl;
//    pOS << *pBlock.mPortInstTbl;
//    pOS << *pBlock.mPortTbl;
//    pOS << *pBlock.mViaTbl;
    pIS >> *pBlock.mLineTbl;
    pIS >> *pBlock.mRectTbl;
    return pIS;
}
//cell view name example: invertor/layout
//return id of root block
uint XtBlock::loadAllSubBlocks(const char* pCellViewNm)
{
    _XtBlock* foundBlock = _XtBlock::blockHashTbl.find(pCellViewNm);
    if (foundBlock)
    {
        uint blockId = foundBlock->getExtId();
        return blockId;
    }
    XtBlock* block = XtBlock::create();
    block->load(pCellViewNm);
    _XtBlock::blockHashTbl.insert(reinterpret_cast<_XtBlock*>(block)); //TODO: check if it is better to insert inside XtBlock::load

    XtSet<XtInst*> instsSet = block->getAllInsts();
    for (XtIterator<XtInst*> it = instsSet.begin(); it != instsSet.end(); it++)
    {
        XtInst* inst = (XtInst*)(*it);
        std::string cellViewNm = inst->getCellViewName();
        uint loadedBlkId = loadAllSubBlocks(cellViewNm.c_str());
        inst->setBlockId(loadedBlkId);
    }
    return block->getId();
}

//only to be used when loading and writting
void _XtBlock::setName(const char* pName)
{
    if (mName)
    {
        _XtBlock::blockHashTbl.remove(mName);
        free(mName);
    }
    mName = strdup(pName);
    _XtBlock::blockHashTbl.insert(this);
}

//load all objects from disk into memory
void XtBlock::load(const char* pCellViewNm)
{
    if (_XtBlock::blockHashTbl.find(pCellViewNm)) //TODO: is it better to detect lock file on hard disk?
    {
        //TODO: exception, already loaded
        return;
    }
    std::filesystem::path cellViewPath = XtDM::cellViewNmToFilePath(pCellViewNm);
    _XtBlock* block = reinterpret_cast<_XtBlock*>(this);
    XtIStream is(cellViewPath.string());
    is >> *block;
    XtSet<XtInst*> instSet = getAllInsts();
    for (XtIterator<XtInst*> instIt = instSet.begin(); instIt != instSet.end(); instIt++)
    {
        if (instIt->placed())
        {
            XtRectangle* rect = instIt->getShape();
            block->mQuadtree->insert(reinterpret_cast<_XtRectangle*>(rect));
        }
    }
    XtSet<XtShape*> shapesSet = getAllShapes();
    for (XtIterator<XtShape*> it = shapesSet.begin(); it != shapesSet.end(); it++)
    {
        block->mQuadtree->insert(reinterpret_cast<_XtRectangle*>(*it));
    }
    block->setName(pCellViewNm);

    //TODO: construct netwireseg hashset in every netobject
}

XtSet<XtBlock*> XtBlock::getAllBlocks()
{
    return XtSet<XtBlock*>(&_XtBlock::blockTbl);
}

void XtBlock::saveAndClose(const char* pCellViewNm)
{
    if (_XtBlock::blockHashTbl.find(pCellViewNm)) //TODO: is it better to detect lock file on hard disk?
    {
        //TODO: exception, already loaded
        return;
    }
    write(pCellViewNm);
    destroy(this);
}

void XtBlock::write(const char* pCellViewNm)
{
    _XtBlock* block = reinterpret_cast<_XtBlock*>(this);
    std::filesystem::path cellViewPath = XtDM::cellViewNmToFilePath(pCellViewNm);
    std::filesystem::create_directories(cellViewPath.parent_path());

    std::ofstream file(cellViewPath.string(), std::ios::out | std::ios::trunc); //create empty file TODO:create folder if it doesn't exist
    file.close();
    XtOStream os(cellViewPath.string());
    os << *block;

    block->setName(pCellViewNm);
}

bool XtBlock::operator==(const XtBlock& pRhs) const
{
    return (*(_XtBlock*)this) == (_XtBlock&)pRhs;
}

XtBlock* XtBlock::create()
{
    XtBlock* block = reinterpret_cast<XtBlock*>(_XtBlock::blockTbl.create());
    return block;
}

void XtBlock::destroy(XtBlock* pBlock)
{
    _XtBlock* block = reinterpret_cast<_XtBlock*>(pBlock);
    _XtBlock::blockTbl.destroy(block);
    if (_XtBlock::blockHashTbl.find(block->mName))
    {
        _XtBlock::blockHashTbl.remove(block);
    }
}

XtSet<XtWireSeg*> XtBlock::getAllWiresegs()
{
    return XtSet<XtWireSeg*>(reinterpret_cast<_XtBlock*>(this)->mWireSegTbl);
}

char* XtBlock::getName()
{
    _XtBlock* block = reinterpret_cast<_XtBlock*>(this);
    return block->mName;
}
}
