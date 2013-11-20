// Copyright (C) 2008, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinUtilsConfig.h"

#include <cassert>
#include "CoinDenseFactorization.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPackedMatrix.hpp"
#include <stdio.h>
//:class CoinDenseFactorization.  Deals with Factorization and Updates
//  CoinDenseFactorization.  Constructor
CoinDenseFactorization::CoinDenseFactorization (  )
{
  gutsOfInitialize();
}

/// Copy constructor 
CoinDenseFactorization::CoinDenseFactorization ( const CoinDenseFactorization &other)
{
  gutsOfInitialize();
  gutsOfCopy(other);
}
/// The real work of constructors etc
void CoinDenseFactorization::gutsOfDestructor()
{
  delete [] elements_;
  delete [] pivotRow_;
  delete [] workArea_;
  elements_ = NULL;
  pivotRow_ = NULL;
  workArea_ = NULL;
  numberRows_ = 0;
  numberColumns_ = 0;
  numberGoodU_ = 0;
  status_ = -1;
  maximumRows_=0;
  maximumSpace_=0;
}
void CoinDenseFactorization::gutsOfInitialize()
{
  pivotTolerance_ = 1.0e-1;
  zeroTolerance_ = 1.0e-13;
  slackValue_ = -1.0;
  maximumPivots_=200;
  relaxCheck_=1.0;
  numberRows_ = 0;
  numberColumns_ = 0;
  numberGoodU_ = 0;
  status_ = -1;
  numberPivots_ = 0;
  maximumRows_=0;
  maximumSpace_=0;
  elements_ = NULL;
  pivotRow_ = NULL;
  workArea_ = NULL;
}
//  ~CoinDenseFactorization.  Destructor
CoinDenseFactorization::~CoinDenseFactorization (  )
{
  gutsOfDestructor();
}
//  =
CoinDenseFactorization & CoinDenseFactorization::operator = ( const CoinDenseFactorization & other ) {
  if (this != &other) {    
    gutsOfDestructor();
    gutsOfInitialize();
    gutsOfCopy(other);
  }
  return *this;
}
void CoinDenseFactorization::gutsOfCopy(const CoinDenseFactorization &other)
{
  pivotTolerance_ = other.pivotTolerance_;
  zeroTolerance_ = other.zeroTolerance_;
  slackValue_ = other.slackValue_;
  relaxCheck_ = other.relaxCheck_;
  numberRows_ = other.numberRows_;
  numberColumns_ = other.numberColumns_;
  maximumRows_ = other.maximumRows_;
  maximumSpace_ = other.maximumSpace_;
  numberGoodU_ = other.numberGoodU_;
  maximumPivots_ = other.maximumPivots_;
  numberPivots_ = other.numberPivots_;
  factorElements_ = other.factorElements_;
  status_ = other.status_;
  if (other.pivotRow_) {
    pivotRow_ = new int [2*maximumRows_+maximumPivots_];
    memcpy(pivotRow_,other.pivotRow_,(2*maximumRows_+numberPivots_)*sizeof(int));
    elements_ = new double [maximumSpace_];
    memcpy(elements_,other.elements_,(maximumRows_+numberPivots_)*maximumRows_*sizeof(double));
    workArea_ = new double [maximumRows_];
  } else {
    elements_ = NULL;
    pivotRow_ = NULL;
    workArea_ = NULL;
  }
}

//  getAreas.  Gets space for a factorization
//called by constructors
void
CoinDenseFactorization::getAreas ( int numberOfRows,
			 int numberOfColumns,
			 CoinBigIndex maximumL,
			 CoinBigIndex maximumU )
{

  numberRows_ = numberOfRows;
  numberColumns_ = numberOfColumns;
  CoinBigIndex size = numberRows_*(numberRows_+CoinMax(maximumPivots_,(numberRows_+1)>>1));
  if (size>maximumSpace_) {
    delete [] elements_;
    elements_ = new double [size];
    maximumSpace_ = size;
  }
  if (numberRows_>maximumRows_) {
    maximumRows_ = numberRows_;
    delete [] pivotRow_;
    delete [] workArea_;
    pivotRow_ = new int [2*maximumRows_+maximumPivots_];
    workArea_ = new double [maximumRows_];
  }
}

//  preProcess.  
void
CoinDenseFactorization::preProcess ()
{
  // could do better than this but this only a demo
  CoinBigIndex put = numberRows_*numberRows_;
  int *indexRow = (int *) (elements_+put);
  CoinBigIndex * starts = (CoinBigIndex *) pivotRow_; 
  put = numberRows_*numberColumns_;
  for (int i=numberColumns_-1;i>=0;i--) {
    put -= numberRows_;
    memset(workArea_,0,numberRows_*sizeof(double));
    assert (starts[i]<=put);
    for (CoinBigIndex j=starts[i];j<starts[i+1];j++) {
      int iRow = indexRow[j];
      workArea_[iRow] = elements_[j];
    }
    // move to correct position
    memcpy(elements_+put,workArea_,numberRows_*sizeof(double));
  }
}

//Does factorization
int
CoinDenseFactorization::factor ( )
{
  numberPivots_=0;
  for (int j=0;j<numberRows_;j++) {
    pivotRow_[j+numberRows_]=j;
  }
  status_= 0;
  double * elements = elements_;
  numberGoodU_=0;
  for (int i=0;i<numberColumns_;i++) {
    int iRow = -1;
    // Find largest
    double largest=zeroTolerance_;
    for (int j=i;j<numberRows_;j++) {
      double value = fabs(elements[j]);
      if (value>largest) {
	largest=value;
	iRow=j;
      }
    }
    if (iRow>=0) {
      if (iRow!=i) {
	// swap
	assert (iRow>i);
	double * elementsA = elements_;
	for (int k=0;k<=i;k++) {
	  // swap
	  double value = elementsA[i];
	  elementsA[i]=elementsA[iRow];
	  elementsA[iRow]=value;
	  elementsA += numberRows_;
	}
	int iPivot = pivotRow_[i+numberRows_];
	pivotRow_[i+numberRows_]=pivotRow_[iRow+numberRows_];
	pivotRow_[iRow+numberRows_]=iPivot;
      }
      double pivotValue = 1.0/elements[i];
      elements[i]=pivotValue;
      for (int j=i+1;j<numberRows_;j++) {
	elements[j] *= pivotValue;
      }
      // Update rest
      double * elementsA = elements;
      for (int k=i+1;k<numberColumns_;k++) {
	elementsA += numberRows_;
	// swap
	if (iRow!=i) {
	  double value = elementsA[i];
	  elementsA[i]=elementsA[iRow];
	  elementsA[iRow]=value;
	}
	double value = elementsA[i];
	for (int j=i+1;j<numberRows_;j++) {
	  elementsA[j] -= value * elements[j];
	}
      }
    } else {
      status_=-1;
      break;
    }
    numberGoodU_++;
    elements += numberRows_;
  }
  for (int j=0;j<numberRows_;j++) {
    int k = pivotRow_[j+numberRows_];
    pivotRow_[k]=j;
  }
  return status_;
}
// Makes a non-singular basis by replacing variables
void 
CoinDenseFactorization::makeNonSingular(int * sequence, int numberColumns)
{
  // Replace bad ones by correct slack
  int * workArea = (int *) workArea_;
  int i;
  for ( i=0;i<numberRows_;i++) 
    workArea[i]=-1;
  for ( i=0;i<numberGoodU_;i++) {
    int iOriginal = pivotRow_[i+numberRows_];
    workArea[iOriginal]=i;
    //workArea[i]=iOriginal;
  }
  int lastRow=-1;
  for ( i=0;i<numberRows_;i++) {
    if (workArea[i]==-1) {
      lastRow=i;
      break;
    }
  }
  assert (lastRow>=0);
  for ( i=numberGoodU_;i<numberRows_;i++) {
    assert (lastRow<numberRows_);
    // Put slack in basis
    sequence[i]=lastRow+numberColumns;
    lastRow++;
    for (;lastRow<numberRows_;lastRow++) {
      if (workArea[lastRow]==-1)
	break;
    }
  }
}
// Does post processing on valid factorization - putting variables on correct rows
void 
CoinDenseFactorization::postProcess(const int * sequence, int * pivotVariable)
{
  for (int i=0;i<numberRows_;i++) {
    int k = sequence[i];
    pivotVariable[pivotRow_[i+numberRows_]]=k;
  }
}
/* Replaces one Column to basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room
   If checkBeforeModifying is true will do all accuracy checks
   before modifying factorization.  Whether to set this depends on
   speed considerations.  You could just do this on first iteration
   after factorization and thereafter re-factorize
   partial update already in U */
int 
CoinDenseFactorization::replaceColumn ( CoinIndexedVector * regionSparse,
					int pivotRow,
					double pivotCheck ,
					bool checkBeforeModifying)
{
  if (numberPivots_==maximumPivots_)
    return 3;
  double * elements = elements_ + numberRows_*(numberColumns_+numberPivots_);
  double *region = regionSparse->denseVector (  );
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero = regionSparse->getNumElements (  );
  int i;
  memset(elements,0,numberRows_*sizeof(double));
  assert (regionSparse->packedMode());
  double pivotValue = pivotCheck;
  if (fabs(pivotValue)<zeroTolerance_)
    return 2;
  pivotValue = 1.0/pivotValue;
  for (i=0;i<numberNonZero;i++) {
    int iRow = regionIndex[i];
    double value = region[i]; //*pivotValue;;
    iRow = pivotRow_[iRow]; // permute
    elements[iRow] = value;;
  }
  int realPivotRow = pivotRow_[pivotRow];
  elements[realPivotRow]=pivotValue;
  pivotRow_[2*numberRows_+numberPivots_]=realPivotRow;
  numberPivots_++;
  return 0;
}
/* This version has same effect as above with FTUpdate==false
   so number returned is always >=0 */
int 
CoinDenseFactorization::updateColumn ( CoinIndexedVector * regionSparse,
				       CoinIndexedVector * regionSparse2,
				       bool noPermute) const
{
  assert (numberRows_==numberColumns_);
  double *region2 = regionSparse2->denseVector (  );
  int *regionIndex = regionSparse2->getIndices (  );
  int numberNonZero = regionSparse2->getNumElements (  );
  double *region = regionSparse->denseVector (  );
  if (!regionSparse2->packedMode()) {
    if (!noPermute) {
      for (int j=0;j<numberRows_;j++) {
	int iRow = pivotRow_[j+numberRows_];
	region[j]=region2[iRow];
	region2[iRow]=0.0;
      }
    } else {
      // can't due to check mode assert (regionSparse==regionSparse2);
      region = regionSparse2->denseVector (  );
    }
  } else {
    // packed mode
    assert (!noPermute);
    for (int j=0;j<numberNonZero;j++) {
      int jRow = regionIndex[j];
      int iRow = pivotRow_[jRow];
      region[iRow]=region2[j];
      region2[j]=0.0;
    }
  }
  int i;
  double * elements = elements_;
  // base factorization L
  for (i=0;i<numberColumns_;i++) {
    double value = region[i];
    for (int j=i+1;j<numberRows_;j++) {
      region[j] -= value*elements[j];
    }
    elements += numberRows_;
  }
  elements = elements_+numberRows_*numberRows_;
  // base factorization U
  for (i=numberColumns_-1;i>=0;i--) {
    elements -= numberRows_;
    double value = region[i]*elements[i];
    region[i] = value;
    for (int j=0;j<i;j++) {
      region[j] -= value*elements[j];
    }
  }
  // now updates
  elements = elements_+numberRows_*numberRows_;
  for (i=0;i<numberPivots_;i++) {
    int iPivot = pivotRow_[i+2*numberRows_];
    double value = region[iPivot]*elements[iPivot];
    for (int j=0;j<numberRows_;j++) {
      region[j] -= value*elements[j];
    }
    region[iPivot] = value;
    elements += numberRows_;
  }
  // permute back and get nonzeros
  numberNonZero=0;
  if (!noPermute) {
    if (!regionSparse2->packedMode()) {
      for (int j=0;j<numberRows_;j++) {
	int iRow = pivotRow_[j];
	double value = region[iRow];
	region[iRow]=0.0;
	if (fabs(value)>zeroTolerance_) {
	  region2[j] = value;
	  regionIndex[numberNonZero++]=j;
	}
      }
    } else {
      // packed mode
      for (int j=0;j<numberRows_;j++) {
	int iRow = pivotRow_[j];
	double value = region[iRow];
	region[iRow]=0.0;
	if (fabs(value)>zeroTolerance_) {
	  region2[numberNonZero] = value;
	  regionIndex[numberNonZero++]=j;
	}
      }
    }
  } else {
    for (int j=0;j<numberRows_;j++) {
      double value = region[j];
      if (fabs(value)>zeroTolerance_) {
	regionIndex[numberNonZero++]=j;
      } else {
	region[j]=0.0;
      }
    }
  }
  regionSparse2->setNumElements(numberNonZero);
  return 0;
}
/* Updates one column (BTRAN) from regionSparse2
   regionSparse starts as zero and is zero at end 
   Note - if regionSparse2 packed on input - will be packed on output
*/
int 
CoinDenseFactorization::updateColumnTranspose ( CoinIndexedVector * regionSparse,
						CoinIndexedVector * regionSparse2) const
{
  assert (numberRows_==numberColumns_);
  double *region2 = regionSparse2->denseVector (  );
  int *regionIndex = regionSparse2->getIndices (  );
  int numberNonZero = regionSparse2->getNumElements (  );
  double *region = regionSparse->denseVector (  );
  if (!regionSparse2->packedMode()) {
    for (int j=0;j<numberRows_;j++) {
      int iRow = pivotRow_[j];
      region[iRow]=region2[j];
      region2[j]=0.0;
    }
  } else {
    for (int j=0;j<numberNonZero;j++) {
      int jRow = regionIndex[j];
      int iRow = pivotRow_[jRow];
      region[iRow]=region2[j];
      region2[j]=0.0;
    }
  }
  int i;
  double * elements = elements_+numberRows_*(numberRows_+numberPivots_);
  // updates
  for (i=numberPivots_-1;i>=0;i--) {
    elements -= numberRows_;
    int iPivot = pivotRow_[i+2*numberRows_];
    double value = region[iPivot]; //*elements[iPivot];
    for (int j=0;j<iPivot;j++) {
      value -= region[j]*elements[j];
    }
    for (int j=iPivot+1;j<numberRows_;j++) {
      value -= region[j]*elements[j];
    }
    region[iPivot] = value*elements[iPivot];
  }
  // base factorization U
  elements = elements_;
  for (i=0;i<numberColumns_;i++) {
    //double value = region[i]*elements[i];
    double value = region[i];
    for (int j=0;j<i;j++) {
      value -= region[j]*elements[j];
    }
    //region[i] = value;
    region[i] = value*elements[i];
    elements += numberRows_;
  }
  // base factorization L
  elements = elements_+numberRows_*numberRows_;
  for (i=numberColumns_-1;i>=0;i--) {
    elements -= numberRows_;
    double value = region[i];
    for (int j=i+1;j<numberRows_;j++) {
      value -= region[j]*elements[j];
    }
    region[i] = value;
  }
  // permute back and get nonzeros
  numberNonZero=0;
  if (!regionSparse2->packedMode()) {
    for (int j=0;j<numberRows_;j++) {
      int iRow = pivotRow_[j+numberRows_];
      double value = region[j];
      region[j]=0.0;
      if (fabs(value)>zeroTolerance_) {
	region2[iRow] = value;
	regionIndex[numberNonZero++]=iRow;
      }
    }
  } else {
    for (int j=0;j<numberRows_;j++) {
      int iRow = pivotRow_[j+numberRows_];
      double value = region[j];
      region[j]=0.0;
      if (fabs(value)>zeroTolerance_) {
	region2[numberNonZero] = value;
	regionIndex[numberNonZero++]=iRow;
      }
    }
  }
  regionSparse2->setNumElements(numberNonZero);
  return 0;
}
void 
CoinDenseFactorization::maximumPivots (  int value )
{
  if (value>maximumPivots_) {
    delete [] pivotRow_;
    pivotRow_ = new int[2*maximumRows_+value];
  }
  maximumPivots_ = value;
}
