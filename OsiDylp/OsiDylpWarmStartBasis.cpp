/*! \legal
  Copyright (C) 2002, 2003
  Lou Hafer, International Business Machines Corporation and others.
  All Rights Reserved.
*/

#ifdef COIN_USE_DYLP

#ifdef _MSC_VER

/* Turn off compiler warning about long names */

#  pragma warning(disable:4786)

#endif // _MSC_VER

/* Cut name lengths for readability. */

#define ODWSB OsiDylpWarmStartBasis
#define CWSB CoinWarmStartBasis

/*!
   \file OsiDylpWarmStartBasis.cpp

   \brief Implementation of warm start object for dylp.

  This file contains the implementation of a warm start object for dylp. It
  extends the CoinWarmStartBasis class by adding an explicit list of active
  constraints. When operating in dynamic mode, dylp needs to be told which
  constraints are active in the basis.
  
  For ease of use, constraint status is handled like variable status. There's
  an array, one entry per constraint, coded using
  CoinWarmStartBasis::Status.  Inactive constraints are marked with isFree,
  active constraints with atLowerBound. Note that active/inactive is not
  equivalent to tight/loose.  The default behaviour in dynamic mode is to
  purge constraints which are strictly loose, but dylp can be instructed to
  also purge tight constraints when the associated dual variable is zero.
*/

namespace {
  char sccsid[] = "@(#)OsiDylpWarmStartBasis.cpp	1.3	03/18/04" ;
  char cvsid[] = "$Id$" ;
}

#include <vector>
#include <cassert>
#include <iostream>
#include "OsiDylpWarmStartBasis.hpp"

using std::vector ;

/*
  A macro to standardize the size of the status arrays. CWSB::Status is the
  status enum, and currently it occupies 2 bits, packed 4 per char (note the
  implicit assumption that a char is 1 byte).  This macro calculates the
  number of bytes needed to hold ns status entires, rounded up to the nearest
  int.
*/

#define STATPERBYTE 4
#define STATALLOCUNIT sizeof(int)
#define STATBYTES(zz_ns_zz) \
  (((zz_ns_zz+STATALLOCUNIT*STATPERBYTE-1)/(STATALLOCUNIT*STATPERBYTE))* \
   STATALLOCUNIT)
  

/*! \defgroup ODWSBConstructorsDestructors ODWSB Constructors and Destructors
    \brief ODWSB Constructors and destructors

  ODWSB provides a default constructor, several variations on a copy
  constructor, a default destructor, and two variations on assignment.
  There are also routines to change the capacity of the warm start object.
*/
//@{

/*! Creates an empty basis. */

ODWSB::OsiDylpWarmStartBasis ()

  : CWSB(),
    phase_(dyINV),
    constraintStatus_(0)

{ /* intentionally left blank */ }


/*!
  A true copy --- no data structures are shared with the original.
*/

ODWSB::OsiDylpWarmStartBasis (const OsiDylpWarmStartBasis &ws)

  : CWSB(ws),
    phase_(ws.phase_),
    constraintStatus_(0)

{ if (ws.constraintStatus_)
  { int constatsze = STATBYTES(getNumArtificial()) ;
    constraintStatus_ = new char[constatsze] ;
    memcpy(constraintStatus_,ws.constraintStatus_,constatsze) ; }
  
  return ; }


/*!
  `Virtual constructor' method, for when we need to duplicate the ODWSB object
  from code that doesn't know it has an ODSWB object.
*/

CoinWarmStart *ODWSB::clone () const

{ const ODWSB *odwsb_orig = dynamic_cast<const ODWSB *>(this) ;
  ODWSB *odwsb_new = 0 ;
  if (odwsb_orig)
  { odwsb_new = new OsiDylpWarmStartBasis(*odwsb_orig) ; }
  return (dynamic_cast<CoinWarmStart *>(odwsb_new)) ; }

/*!
  Construct by copying a set of status arrays (parameters preserved). If
  no constraint status is provided, the default is to declare all constraints
  active. The phase is set to dyPRIMAL1, which is safe for any basis.

  Consider assignBasisStatus(int,int,char*&,char*&) if the object should
  assume ownership.
*/

ODWSB::OsiDylpWarmStartBasis
  (int ns, int na, const char *sStat, const char *aStat, const char *cStat)

  : CWSB(ns,na,sStat,aStat),
    phase_(dyPRIMAL1),
    constraintStatus_(0)

{ int constatsze = STATBYTES(na) ;
  constraintStatus_ = new char[constatsze] ;
/*
  If a status array is given, copy it in. If there's no array provided, assume
  all constraints are active. Set up a byte with the correct pattern and use it
  to initialize the constraint status.
*/
  if (cStat)
  { memcpy(constraintStatus_,cStat,constatsze) ; }
  else
  { char byteActive = 0 ;
    int i ;
    for (i = 0 ; i <= 3 ; i++) setStatus(&byteActive,i,CWSB::atLowerBound) ;
    memset(constraintStatus_,byteActive,constatsze) ; } }

/*!
  Assignment of structure (parameter preserved)
*/

OsiDylpWarmStartBasis& ODWSB::operator= (const OsiDylpWarmStartBasis &rhs)

{ if (this != &rhs)
  { CWSB::operator=(rhs) ;
    phase_ = rhs.phase_ ;
    delete[] constraintStatus_ ;
    if (rhs.constraintStatus_)
    { int constatsze = STATBYTES(getNumArtificial()) ;
      constraintStatus_ = new char[constatsze] ;
      memcpy(constraintStatus_,rhs.constraintStatus_,constatsze) ; }
    else
    { constraintStatus_ = 0 ; } }
  
  return *this ; }


/*!
  Assignment of status arrays (parameters destroyed). The phase is set to
  dyPRIMAL1, which is safe for any basis.

  In this method the OsiDylpWarmStartBasis object assumes ownership of the
  arrays and upon return the argument pointers will be NULL.
  If copying is desirable, use the
  \link OsiDylpWarmStartBasis(int,int,const char*,const char*,const char*)
	array constructor \endlink
  or the
  \link operator=(const OsiDylpWarmStartBasis&)
	assignment operator \endlink.

  \note
  The pointers passed to this method will be
  freed using delete[], so they must be created using new[].
*/

void ODWSB::assignBasisStatus
  (int ns, int na, char *&sStat, char *&aStat, char *&cStat)

{ CWSB::assignBasisStatus(ns,na,sStat,aStat) ;
  phase_ = dyPRIMAL1 ;
  delete[] constraintStatus_ ;
  constraintStatus_ = cStat ;
  cStat = 0 ; }

/*!
  Assignment of status arrays (parameters destroyed). When no constraint status
  is provided, the default is to create an array which indicates all
  constraints are active. The phase is set to dyPRIMAL1, which is safe for
  any basis.

  In this method the OsiDylpWarmStartBasis object assumes ownership of the
  arrays and upon return the argument pointers will be NULL.
  If copying is desirable, use the
  \link OsiDylpWarmStartBasis(int,int,const char*,const char*,const char*)
	array constructor \endlink
  or the
  \link operator=(const OsiDylpWarmStartBasis&)
	assignment operator \endlink.

  \note
  The pointers passed to this method will be
  freed using delete[], so they must be created using new[].
*/
void ODWSB::assignBasisStatus (int ns, int na, char *&sStat, char *&aStat)

{ int constatsze = STATBYTES(na) ;
  char byteActive = 0 ;
  int i ;

  CWSB::assignBasisStatus(ns,na,sStat,aStat) ;
  phase_ = dyPRIMAL1 ;

  delete[] constraintStatus_ ;
  constraintStatus_ = new char[constatsze] ;
  for (i = 0 ; i <= 3 ; i++) setStatus(&byteActive,i,CWSB::atLowerBound) ;
  memset(constraintStatus_,byteActive,constatsze) ; }

ODWSB::~OsiDylpWarmStartBasis () { delete[] constraintStatus_ ; }

/*!
  This routine sets the capacity of the warm start object.
  Any existing basis information is lost.
*/

void ODWSB::setSize (int ns, int na)

{ CWSB::setSize(ns,na) ;
  phase_ = dyINV ;
  delete[] constraintStatus_ ;
  if (na > 0)
  { int constatsze = STATBYTES(na) ;
    constraintStatus_ = new char[constatsze] ;
    char byteActive = 0 ;
    for (int i = 0 ; i <= 3 ; i++) setStatus(&byteActive,i,CWSB::atLowerBound) ;
    memset(constraintStatus_,byteActive,constatsze) ; }
  else
  { constraintStatus_ = 0 ; }
  
  return ; }

/*!
  This routine sets the capacity of the warm start object.
  Any existing basis information is retained. If the new size is smaller
  than the existing basis, the existing basis is truncated to fit.
  If the new size is larger than the existing basis, additional structural
  variables are given the status nonbasic at lower bound, additional artificial
  variables are given the status basic, and additional constraints
  are given the status active.

  We need to allow for the possibility that the client will create an empty
  basis and then resize() it, rather than using setSize().
*/

void ODWSB::resize (int numRows, int numCols)

{ int concnt = getNumArtificial() ;
  int varcnt = getNumStructural() ;

  if (concnt == 0 && varcnt == 0) setSize(numCols,numRows) ;

  assert(constraintStatus_) ;
  
  CWSB::resize(numRows,numCols) ;

  if (numRows != concnt)
  { int oldsze = STATBYTES(concnt) ;
    int newsze = STATBYTES(numRows) ;
    char *newStat = new char[newsze] ;
/*
  If the new capacity is smaller, truncate the existing basis.
*/
    if (oldsze > newsze)
    { memcpy(newStat,constraintStatus_,newsze) ; }
/*
  If the new capacity is larger, copy the existing basis, then mark the
  additional constraints as active. We need to be a bit careful here, because
  the allocated sizes are rounded out. The strategy is to (possibly) overlap
  one byte, correcting it after initialization.
*/
    else
    { char byteActive = 0 ;
      int i,actualBytes ;
      actualBytes = concnt/STATPERBYTE ;
      for (i = 0 ; i <= 3 ; i++) setStatus(&byteActive,i,CWSB::atLowerBound) ;
      memcpy(newStat,constraintStatus_,oldsze) ;
      memset(newStat+actualBytes,byteActive,newsze-actualBytes) ;
      for (i = 0 ; i < concnt%STATPERBYTE ; i++)
	setStatus(newStat+actualBytes,i,
		  getStatus(constraintStatus_+actualBytes,i)) ; }

    delete [] constraintStatus_ ;
    constraintStatus_ = newStat ; }
  
  return ; }


/*!
   Removal of a tight constraint with a nonbasic logical implies that some
   basic variable must be kicked out of the basis. There's no cheap, efficient
   way to choose this variable, so the choice is left to the client.
*/
/*
  To elaborate on the above comment: When a tight constraint is removed, new
  extreme points are exposed. There's a reasonable likelihood they'll be
  desireable points (since the tight constraint was most likely tight because
  it was blocking movement in a desireable direction). In essence, we want to
  take the next primal pivot and see what comes tight. That's a fair bit of
  work, and on balance best left to the client to decide if it's appropriate,
  or if something else should be done.
*/

void ODWSB::deleteRows (int number, const int *which)

{ int oldconcnt = getNumArtificial() ;
  int i,k ;

  CWSB::deleteRows(number,which) ;

  int delcnt = 0 ;
  vector<bool> deleted = vector<bool>(oldconcnt) ;
  for (k = 0 ; k < number ; k++)
  { i = which[k] ;
    if (i >= 0 && i < oldconcnt && !deleted[i])
    { delcnt++ ;
      deleted[i] = true ; } }

  int newsze = STATBYTES(oldconcnt-delcnt) ;
  char *newStat = new char[newsze] ;

  k = 0 ;
  for (i = 0 ; i < oldconcnt ; i++)
  { Status status = getConStatus(i);
    if (!deleted[i])
    { setStatus(newStat,k,status) ;
      k++ ; } }
  
  delete [] constraintStatus_ ;
  constraintStatus_ = newStat ; }

/*
  The good news is that CWSB::deleteColumns works just fine for ODWSB.
*/

//@}

/*! \defgroup BasisInfo Methods to get and set basis information
    \brief Methods to get and set basis information
*/

//@{

/*!
  Count the number of active constraints by scanning the constraint status
  array.
*/

int ODWSB::numberActiveConstraints () const

{ int i ;
  int concnt = getNumArtificial() ;
  int actcnt = 0 ;

  for (i = 0 ; i < concnt ; i++)
    if (getStatus(constraintStatus_,i) == CWSB::atLowerBound) actcnt++ ;
  
  return (actcnt) ; }

/*! \defgroup BasisDiff Basis `diff' methods
    \brief Methods to calculate and apply a `diff' between two bases.
*/

//@{

/*!
  The capabilities are limited by the CoinWarmStartBasis format: oldBasis can
  be no larger than newBasis (the basis pointed to by \c this), and the
  subset of newBasis covered by oldBasis is assumed to contain the identical
  set of logical and structural variables, in order.

  We use CWSB::generateDiff to deal with the status arrays for the logical and
  structural variables, then add an additional vector for constraint status.
*/
CoinWarmStartDiff *ODWSB::generateDiff
  (const CoinWarmStart *const oldCWS) const
{
/*
  Make sure the parameter is an OsiDylpWarmStartBasis.
*/
  const OsiDylpWarmStartBasis *oldBasis =
      dynamic_cast<const OsiDylpWarmStartBasis *>(oldCWS) ;
  if (!oldBasis)
  { throw CoinError("Old basis not OsiDylpWarmStartBasis.",
		    "generateDiff","OsiDylpWarmStartBasis") ; }
  const OsiDylpWarmStartBasis *newBasis = this ;
/*
  Make sure newBasis is equal or bigger than oldBasis. Calculate the worst case
  number of diffs and allocate vectors to hold them.
*/
  const int oldArtifCnt = oldBasis->getNumArtificial() ;
  const int newArtifCnt = newBasis->getNumArtificial() ;

  assert(newArtifCnt >= oldArtifCnt) ;
/*
  Ok, we're good to go. Call CWSB::generateDiff to deal with the logical and
  structural arrays.
*/
  const CoinWarmStartBasisDiff *cwsbDiff =
    dynamic_cast<const CoinWarmStartBasisDiff *>(CWSB::generateDiff(oldCWS)) ;
/*
  Now generate diffs for the constraint array.
*/

  int sizeOldArtif = (oldArtifCnt+15)>>4 ;
  int sizeNewArtif = (newArtifCnt+15)>>4 ;
  int maxBasisLength = sizeNewArtif ;

  unsigned int *diffNdx = new unsigned int [maxBasisLength]; 
  unsigned int *diffVal = new unsigned int [maxBasisLength]; 
/*
  Scan the constraint status.
  For the portion of the status arrays which overlap, create
  diffs. Then add any additional status from newBasis.
*/
  const unsigned int *oldStatus =
      reinterpret_cast<const unsigned int *>(oldBasis->getConstraintStatus()) ;
  const unsigned int *newStatus = 
      reinterpret_cast<const unsigned int *>(newBasis->getConstraintStatus()) ;
  int numberChanged = 0 ;
  int i ;
  for (i = 0 ; i < sizeOldArtif ; i++)
  { if (oldStatus[i] != newStatus[i])
    { diffNdx[numberChanged] = i ;
      diffVal[numberChanged++] = newStatus[i] ; } }
  for ( ; i < sizeNewArtif ; i++)
  { diffNdx[numberChanged] = i ;
    diffVal[numberChanged++] = newStatus[i] ; }
/*
  Create the object of our desire.
*/
  OsiDylpWarmStartBasisDiff *diff =
    new OsiDylpWarmStartBasisDiff(numberChanged,diffNdx,diffVal,cwsbDiff) ;
/*
  Clean up and return.
*/
  delete[] diffNdx ;
  delete[] diffVal ;

  return (dynamic_cast<CoinWarmStartDiff *>(diff)) ; }

/*!
   Update this basis by applying \p diff. It's assumed that the allocated
   capacity of the basis is sufficiently large.

   We use CWSB::applyDiff to deal with the logical and structural status
   vectors, then apply the diffs for the constraint status vector.
*/

void ODWSB::applyDiff (const CoinWarmStartDiff *const cwsdDiff)
{
/*
  Make sure we have an OsiDylpWarmStartBasisDiff
*/
  const OsiDylpWarmStartBasisDiff *diff =
    dynamic_cast<const OsiDylpWarmStartBasisDiff *>(cwsdDiff) ;
  if (!diff)
  { throw CoinError("Diff not OsiDylpWarmStartBasisDiff.",
		    "applyDiff","OsiDylpWarmStartBasis") ; }
/*
  Call CWSB::applyDiff to deal with the logical and structural status.
*/
  CWSB::applyDiff(cwsdDiff) ;
/*
  Now fix up the constraint status array. Application of the diff is by
  straighforward replacement of words in the status array.
*/
  const int numberChanges = diff->consze_ ;
  const unsigned int *diffNdxs = diff->condiffNdxs_ ;
  const unsigned int *diffVals = diff->condiffVals_ ;
  unsigned int *constraintStatus =
      reinterpret_cast<unsigned int *>(this->getConstraintStatus()) ;

  for (int i = 0 ; i < numberChanges ; i++)
  { unsigned int diffNdx = diffNdxs[i] ;
    unsigned int diffVal = diffVals[i] ;
    constraintStatus[diffNdx] = diffVal ; }

  return ; }

//@}


/*! \defgroup MiscUtil Miscellaneous Utilities
    \brief Miscellaneous utility functions
*/

//@{

/*!
  Print the warm start object in a (more or less) human-readable form.
  Intended for debugging.
*/

void ODWSB::print () const

{ char conlett[] = {'I','?','?','A'} ;
  char statlett[] = {'F','B','U','L'} ;
  int i,basic_logicals,basic_structurals ;

  std::cout << "ODWSB: " ;
  std::cout << getNumArtificial() << " constraints (" <<
	       numberActiveConstraints() << " active), " ;
  std::cout << getNumStructural() << " variables." << std::endl ;

  std::cout << "Rows: " ;
  for (i = 0 ; i < getNumArtificial() ; i++)
  { std::cout << conlett[getConStatus(i)] ; }
  std::cout << std::endl ;

  std::cout << "      " ;
  basic_logicals = 0 ;
  for (i = 0 ; i < getNumArtificial() ; i++)
  { std::cout << statlett[getArtifStatus(i)] ;
    if (getArtifStatus(i) == CWSB::basic) basic_logicals++ ; }
  std::cout << std::endl ;
  
  std::cout << "Cols: " ;
  basic_structurals = 0 ;
  for (i = 0 ; i < getNumStructural() ; i++)
  { std::cout << statlett[getStructStatus(i)] ;
    if (getStructStatus(i) == CWSB::basic) basic_structurals++ ; }

  std::cout << std::endl << "	basic: ("
	    << basic_structurals << " + " << basic_logicals << ")" ;
  std::cout << std::endl ;
  
  return ; }

//@}




/* Routines for OsiDylpWarmStartBasisDiff */

/*
  Constructor given constraint diff data and a pointer to a
  CoinWarmStartBasisDiff object.
*/
OsiDylpWarmStartBasisDiff::OsiDylpWarmStartBasisDiff (int sze,
  const unsigned int *const diffNdxs, const unsigned int *const diffVals,
  const CoinWarmStartBasisDiff *const cwsbd)

  : CoinWarmStartBasisDiff(*cwsbd),
    consze_(sze),
    condiffNdxs_(0),
    condiffVals_(0)

{ if (sze > 0)
  { condiffNdxs_ = new unsigned int[sze] ;
    memcpy(condiffNdxs_,diffNdxs,sze*sizeof(unsigned int)) ;
    condiffVals_ = new unsigned int[sze] ;
    memcpy(condiffVals_,diffVals,sze*sizeof(unsigned int)) ; }
  
  return ; }

/*
  Copy constructor
*/
OsiDylpWarmStartBasisDiff::OsiDylpWarmStartBasisDiff
  (const OsiDylpWarmStartBasisDiff &odwsbd)
  : CoinWarmStartBasisDiff(odwsbd),
    consze_(odwsbd.consze_),
    condiffNdxs_(0),
    condiffVals_(0)
{ if (odwsbd.consze_ > 0)
  { condiffNdxs_ = new unsigned int[odwsbd.consze_] ;
    memcpy(condiffNdxs_,odwsbd.condiffNdxs_,
	   odwsbd.consze_*sizeof(unsigned int)) ;
    condiffVals_ = new unsigned int[odwsbd.consze_] ;
    memcpy(condiffVals_,odwsbd.condiffVals_,
	   odwsbd.consze_*sizeof(unsigned int)) ; }
  
  return ; }

/*
  Assignment --- for convenience when assigning objects containing
  CoinWarmStartBasisDiff objects.
*/
OsiDylpWarmStartBasisDiff&
OsiDylpWarmStartBasisDiff::operator= (const OsiDylpWarmStartBasisDiff &rhs)

{ if (this != &rhs)
  { CoinWarmStartBasisDiff::operator=(rhs) ;
    if (consze_ > 0)
    { delete[] condiffNdxs_ ;
      delete[] condiffVals_ ; }
    consze_ = rhs.consze_ ;
    if (consze_ > 0)
    { condiffNdxs_ = new unsigned int[consze_] ;
      memcpy(condiffNdxs_,rhs.condiffNdxs_,consze_*sizeof(unsigned int)) ;
      condiffVals_ = new unsigned int[consze_] ;
      memcpy(condiffVals_,rhs.condiffVals_,consze_*sizeof(unsigned int)) ; }
    else
    { condiffNdxs_ = 0 ;
      condiffVals_ = 0 ; } }
  
  return (*this) ; }


#endif // COIN_USE_DYLP

