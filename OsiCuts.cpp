// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <algorithm>
#include <cassert>

#include "OsiCuts.hpp"

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
OsiCuts::OsiCuts ()
:
rowCutPtrs_(),
colCutPtrs_()
{
  // nothing to do here
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
OsiCuts::OsiCuts (const OsiCuts & source)
:
rowCutPtrs_(),
colCutPtrs_()     
{  
  gutsOfCopy( source );
}


//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
OsiCuts::~OsiCuts ()
{
  gutsOfDestructor();
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
OsiCuts &
OsiCuts::operator=(const OsiCuts& rhs)
{
  if (this != &rhs) {
    gutsOfDestructor();
    gutsOfCopy( rhs );
  }
  return *this;
}



//-------------------------------------------------------------------
void OsiCuts::gutsOfCopy(const OsiCuts& source)
{
  assert( sizeRowCuts()==0 );
  assert( sizeColCuts()==0 );
  assert( sizeCuts()==0 );
  int i;
  int ne = source.sizeRowCuts();
  for (i=0; i<ne; i++) insert( source.rowCut(i) );
  ne = source.sizeColCuts();
  for (i=0; i<ne; i++) insert( source.colCut(i) );

}

//-------------------------------------------------------------------
void OsiCuts::gutsOfDestructor()
{
  int i;
  
  int ne = rowCutPtrs_.size();
  for (i=0; i<ne; i++) delete rowCutPtrs_[i];
  rowCutPtrs_.clear();
  
  ne = colCutPtrs_.size();
  for (i=0; i<ne; i++) delete colCutPtrs_[i];
  colCutPtrs_.clear();
  
  assert( sizeRowCuts()==0 );
  assert( sizeColCuts()==0 );
  assert( sizeCuts()   ==0 );
}


//------------------------------------------------------------
//
// Embedded iterator class implementation
//
//------------------------------------------------------------
OsiCuts::iterator::iterator(OsiCuts& cuts)
: 
cuts_(cuts),
rowCutIndex_(-1),
colCutIndex_(-1),
cutP_(NULL)
{
  this->operator++();
}

OsiCuts::iterator::iterator(const OsiCuts::iterator &src)
: 
cuts_(src.cuts_),
rowCutIndex_(src.rowCutIndex_),
colCutIndex_(src.colCutIndex_),
cutP_(src.cutP_)
{
  // nothing to do here
}

OsiCuts::iterator & OsiCuts::iterator::operator=(const OsiCuts::iterator &rhs)
{
  if (this != &rhs) {
    cuts_=rhs.cuts_;
    rowCutIndex_=rhs.rowCutIndex_;
    colCutIndex_=rhs.colCutIndex_;
    cutP_=rhs.cutP_;
  }
  return *this;
}

OsiCuts::iterator::~iterator()
{
  //nothing to do
}

OsiCuts::iterator OsiCuts::iterator::begin()
{
  rowCutIndex_=-1;
  colCutIndex_=-1;
  this->operator++();
  return *this;
}

OsiCuts::iterator OsiCuts::iterator::end()
{
  rowCutIndex_=cuts_.sizeRowCuts();
  colCutIndex_=cuts_.sizeColCuts()-1;
  cutP_=NULL;
  return *this;
}

OsiCuts::iterator OsiCuts::iterator::operator++() {
  cutP_ = NULL;

  // Are there any more row cuts to consider?
  if ( (rowCutIndex_+1)>=cuts_.sizeRowCuts() ) {
    // Only column cuts left.
    colCutIndex_++;
    // Only update cutP if there is a column cut.
    // This is necessary for the iterator to work for
    // OsiCuts that don't have any cuts.
    if ( cuts_.sizeRowCuts() > 0 && colCutIndex_<cuts_.sizeRowCuts() ) 
      cutP_= cuts_.colCutPtr(colCutIndex_);
  }

  // Are there any more col cuts to consider?
  else if ( (colCutIndex_+1)>=cuts_.sizeColCuts() ) {
    // Only row cuts left
    rowCutIndex_++;
    if ( rowCutIndex_<cuts_.sizeRowCuts() ) 
      cutP_= cuts_.rowCutPtr(rowCutIndex_);
  }

  // There are still Row & column cuts left to consider
  else {
    double nextColCutE=cuts_.colCut(colCutIndex_+1).effectiveness();
    double nextRowCutE=cuts_.rowCut(rowCutIndex_+1).effectiveness();
    if ( nextColCutE>nextRowCutE ) {
      colCutIndex_++;
      cutP_=cuts_.colCutPtr(colCutIndex_);
    }
    else {
      rowCutIndex_++;
      cutP_=cuts_.rowCutPtr(rowCutIndex_);
    }
  }
  return *this;
}

//------------------------------------------------------------
//
// Embedded const_iterator class implementation
//
//------------------------------------------------------------
OsiCuts::const_iterator::const_iterator(const OsiCuts& cuts)
: 
cutsPtr_(&cuts),
rowCutIndex_(-1),
colCutIndex_(-1),
cutP_(NULL)
{
  this->operator++();
}

OsiCuts::const_iterator::const_iterator(const OsiCuts::const_iterator &src)
: 
cutsPtr_(src.cutsPtr_),
rowCutIndex_(src.rowCutIndex_),
colCutIndex_(src.colCutIndex_),
cutP_(src.cutP_)
{
  // nothing to do here
}

OsiCuts::const_iterator &
OsiCuts::const_iterator::operator=(const OsiCuts::const_iterator &rhs)
{
  if (this != &rhs) {
    cutsPtr_=rhs.cutsPtr_;
    rowCutIndex_=rhs.rowCutIndex_;
    colCutIndex_=rhs.colCutIndex_;
    cutP_=rhs.cutP_;
  }
  return *this;
}

OsiCuts::const_iterator::~const_iterator()
{
  //nothing to do
}

OsiCuts::const_iterator OsiCuts::const_iterator::begin()
{
  rowCutIndex_=-1;
  colCutIndex_=-1;
  this->operator++();
  return *this;
}

OsiCuts::const_iterator OsiCuts::const_iterator::end()
{
  rowCutIndex_=cutsPtr_->sizeRowCuts();
  colCutIndex_=cutsPtr_->sizeColCuts()-1;
  cutP_=NULL;
  return *this;
}


OsiCuts::const_iterator OsiCuts::const_iterator::operator++() { 
  cutP_ = NULL;

  // Are there any more row cuts to consider?
  if ( (rowCutIndex_+1)>=cutsPtr_->sizeRowCuts() ) {
    // Only column cuts left.
    colCutIndex_++;
    // Only update cutP if there is a column cut.
    // This is necessary for the iterator to work for
    // OsiCuts that don't have any cuts.
    if ( cutsPtr_->sizeRowCuts() > 0 && colCutIndex_<cutsPtr_->sizeRowCuts() ) 
      cutP_= cutsPtr_->colCutPtr(colCutIndex_);
  }

  // Are there any more col cuts to consider?
  else if ( (colCutIndex_+1)>=cutsPtr_->sizeColCuts() ) {
    // Only row cuts left
    rowCutIndex_++;
    if ( rowCutIndex_<cutsPtr_->sizeRowCuts() ) 
      cutP_= cutsPtr_->rowCutPtr(rowCutIndex_);
  }

  // There are still Row & column cuts left to consider
  else {
    double nextColCutE=cutsPtr_->colCut(colCutIndex_+1).effectiveness();
    double nextRowCutE=cutsPtr_->rowCut(rowCutIndex_+1).effectiveness();
    if ( nextColCutE>nextRowCutE ) {
      colCutIndex_++;
      cutP_=cutsPtr_->colCutPtr(colCutIndex_);
    }
    else {
      rowCutIndex_++;
      cutP_=cutsPtr_->rowCutPtr(rowCutIndex_);
    }
  }
  return *this;
}

/* Insert a row cut unless it is a duplicate */
void 
OsiCuts::insertIfNotDuplicate( OsiRowCut & rc )
{
  double newLb = rc.lb();
  double newUb = rc.ub();
  CoinPackedVector vector = rc.row();
  int numberElements =vector.getNumElements();
  int * newIndices = vector.getIndices();
  double * newElements = vector.getElements();
  CoinSort_2(newIndices,newIndices+numberElements,newElements);
  bool notDuplicate=true;
  int numberRowCuts = sizeRowCuts();
  for ( int i =0; i<numberRowCuts;i++) {
    const OsiRowCut * cutPtr = rowCutPtr(i);
    if (cutPtr->row().getNumElements()!=numberElements)
      continue;
    if (fabs(cutPtr->lb()-newLb)>1.0e-12)
      continue;
    if (fabs(cutPtr->ub()-newUb)>1.0e-12)
      continue;
    const CoinPackedVector * thisVector = &(cutPtr->row());
    const int * indices = thisVector->getIndices();
    const double * elements = thisVector->getElements();
    int j;
    for(j=0;j<numberElements;j++) {
      if (indices[j]!=newIndices[j])
	break;
      if (fabs(elements[j]-newElements[j])>1.0e-12)
	break;
    }
    if (j==numberElements) {
      notDuplicate=false;
      break;
    }
  }
  if (notDuplicate) {
    OsiRowCut * newCutPtr = new OsiRowCut();
    newCutPtr->setLb(newLb);
    newCutPtr->setUb(newUb);
    newCutPtr->setRow(vector);
    rowCutPtrs_.push_back(newCutPtr);
  }
}
