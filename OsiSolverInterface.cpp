// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <iostream>

#include "CoinHelperFunctions.hpp"
#include "CoinMpsIO.hpp"
#include "CoinMessage.hpp"
#include "CoinWarmStart.hpp"

#include "OsiSolverInterface.hpp"
#include "OsiCuts.hpp"
#include "OsiRowCut.hpp"
#include "OsiColCut.hpp"
#include "OsiRowCutDebugger.hpp"

//#############################################################################
// Hotstart related methods (primarily used in strong branching)
// It is assumed that only bounds (on vars/constraints) can change between
// markHotStart() and unmarkHotStart()
//#############################################################################

void OsiSolverInterface::markHotStart()
{
  delete ws_;
  ws_ = getWarmStart();
}

void OsiSolverInterface::solveFromHotStart()
{
  setWarmStart(ws_);
  resolve();
}

void OsiSolverInterface::unmarkHotStart()
{
  delete ws_;
  ws_ = NULL;
}

//#############################################################################
// Get indices of solution vector which are integer variables presently at
// fractional values
//#############################################################################

OsiVectorInt
OsiSolverInterface::getFractionalIndices(const double etol) const
{
   const int colnum = getNumCols();
   OsiVectorInt frac;
   CoinAbsFltEq eq(etol);
   for (int i = 0; i < colnum; ++i) {
      if (isInteger(i)) {
	 const double ci = getColSolution()[i];
	 const double distanceFromInteger = ci - floor(ci + 0.5);
	 if (! eq(distanceFromInteger, 0.0))
	    frac.push_back(i);
      }
   }
   return frac;
}


int OsiSolverInterface::getNumElements() const
{
  return getMatrixByRow()->getNumElements();
}

//#############################################################################
// Methods for determining the type of column variable.
// The method isContinuous() is presently implemented in the derived classes.
// The methods below depend on isContinuous and the values
// stored in the column bounds.
//#############################################################################

bool 
OsiSolverInterface::isBinary(int colIndex) const
{
  if ( isContinuous(colIndex) ) return false; 
  const double * cu = getColUpper();
  const double * cl = getColLower();
  if (
    (cu[colIndex]== 1 || cu[colIndex]== 0) && 
    (cl[colIndex]== 0 || cl[colIndex]==1)
    ) return true;
  else return false;
}
//-----------------------------------------------------------------------------
bool 
OsiSolverInterface::isInteger(int colIndex) const
{
   return !isContinuous(colIndex);
}
//-----------------------------------------------------------------------------
bool 
OsiSolverInterface::isIntegerNonBinary(int colIndex) const
{
  if ( isInteger(colIndex) && !isBinary(colIndex) )
    return true; 
  else return false;
}
//-----------------------------------------------------------------------------
bool 
OsiSolverInterface::isFreeBinary(int colIndex) const
{
  if ( isContinuous(colIndex) ) return false;
  const double * cu = getColUpper();
  const double * cl = getColLower();
  if (
    (cu[colIndex]== 1) &&
    (cl[colIndex]== 0)
    ) return true;
  else return false;
}

//#############################################################################
// Built-in (i.e., slow) methods for problem modification
//#############################################################################

void
OsiSolverInterface::setColSetBounds(const int* indexFirst,
				    const int* indexLast,
				    const double* boundList)
{
  while (indexFirst != indexLast) {
    setColBounds(*indexFirst, boundList[0], boundList[1]);
    ++indexFirst;
    boundList += 2;
  }
}
//-----------------------------------------------------------------------------
void
OsiSolverInterface::setRowSetBounds(const int* indexFirst,
				    const int* indexLast,
				    const double* boundList)
{
  while (indexFirst != indexLast) {
    setRowBounds(*indexFirst, boundList[0], boundList[1]);
    ++indexFirst;
    boundList += 2;
  }
}
//-----------------------------------------------------------------------------
void
OsiSolverInterface::setRowSetTypes(const int* indexFirst,
				   const int* indexLast,
				   const char* senseList,
				   const double* rhsList,
				   const double* rangeList)
{
  while (indexFirst != indexLast) {
    setRowType(*indexFirst++, *senseList++, *rhsList++, *rangeList++);
  }
}
//-----------------------------------------------------------------------------
void
OsiSolverInterface::setContinuous(const int* indices, int len)
{
  for (int i = 0; i < len; ++i) {
    setContinuous(indices[i]);
  }
}
//-----------------------------------------------------------------------------
void
OsiSolverInterface::setInteger(const int* indices, int len)
{
  for (int i = 0; i < len; ++i) {
    setInteger(indices[i]);
  }
}
//-----------------------------------------------------------------------------
void
OsiSolverInterface::addCols(const int numcols,
			    const CoinPackedVectorBase * const * cols,
			    const double* collb, const double* colub,   
			    const double* obj)
{
  for (int i = 0; i < numcols; ++i) {
    addCol(*cols[i], collb[i], colub[i], obj[i]);
  }
}
//-----------------------------------------------------------------------------
void
OsiSolverInterface::addRows(const int numrows,
			    const CoinPackedVectorBase * const * rows,
			    const double* rowlb, const double* rowub)
{
  for (int i = 0; i < numrows; ++i) {
    addRow(*rows[i], rowlb[i], rowub[i]);
  }
}
//-----------------------------------------------------------------------------
void
OsiSolverInterface::addRows(const int numrows,
			    const CoinPackedVectorBase * const * rows,
			    const char* rowsen, const double* rowrhs,   
			    const double* rowrng)
{
  for (int i = 0; i < numrows; ++i) {
    addRow(*rows[i], rowsen[i], rowrhs[i], rowrng[i]);
  }
}

//#############################################################################
// Apply Cuts
//#############################################################################

OsiSolverInterface::ApplyCutsReturnCode
OsiSolverInterface::applyCuts( const OsiCuts & cs, double effectivenessLb ) 
{
  OsiSolverInterface::ApplyCutsReturnCode retVal;
  int i;

  // Loop once for each column cut
  for ( i=0; i<cs.sizeColCuts(); i ++ ) {
    if ( cs.colCut(i).effectiveness() < effectivenessLb ) {
      retVal.incrementIneffective();
      continue;
    }
    if ( !cs.colCut(i).consistent() ) {
      retVal.incrementInternallyInconsistent();
      continue;
    }
    if ( !cs.colCut(i).consistent(*this) ) {
      retVal.incrementExternallyInconsistent();
      continue;
    }
    if ( cs.colCut(i).infeasible(*this) ) {
      retVal.incrementInfeasible();
      continue;
    }
    applyColCut( cs.colCut(i) );
    retVal.incrementApplied();
  }

  // Loop once for each row cut
  for ( i=0; i<cs.sizeRowCuts(); i ++ ) {
    if ( cs.rowCut(i).effectiveness() < effectivenessLb ) {
      retVal.incrementIneffective();
      continue;
    }
    if ( !cs.rowCut(i).consistent() ) {
      retVal.incrementInternallyInconsistent();
      continue;
    }
    if ( !cs.rowCut(i).consistent(*this) ) {
      retVal.incrementExternallyInconsistent();
      continue;
    }
    if ( cs.rowCut(i).infeasible(*this) ) {
      retVal.incrementInfeasible();
      continue;
    }
    applyRowCut( cs.rowCut(i) );
    retVal.incrementApplied();
  }
  
  return retVal;
}
/* Apply a collection of row cuts which are all effective.
   applyCuts seems to do one at a time which seems inefficient.
   The default does slowly, but solvers can override.
*/
void 
OsiSolverInterface::applyRowCuts(int numberCuts, const OsiRowCut * cuts)
{
  int i;
  for (i=0;i<numberCuts;i++) {
    applyRowCut(cuts[i]);
  }
}

//#############################################################################
// Set/Get Application Data
// This is a pointer that the application can store into and retrieve
// from the solverInterface.
// This field is the application to optionally define and use.
//#############################################################################

void OsiSolverInterface::setApplicationData(void * appData)
{
  appData_ = appData;
}
//-----------------------------------------------------------------------------
void * OsiSolverInterface::getApplicationData() const
{
  return appData_;
}

//#############################################################################
// Methods related to Row Cut Debuggers
//#############################################################################

//-------------------------------------------------------------------
// Activate Row Cut Debugger<br>
// If the model name passed is on list of known models
// then all cuts are checked to see that they do NOT cut
// off the known optimal solution.  

void OsiSolverInterface::activateRowCutDebugger (const char * modelName)
{
  delete rowCutDebugger_;
  rowCutDebugger_ = new OsiRowCutDebugger(*this,modelName);
}

//-------------------------------------------------------------------
// Get Row Cut Debugger<br>
// If there is a row cut debugger object associated with
// model AND if the known optimal solution is within the
// current feasible region then a pointer to the object is
// returned which may be used to test validity of cuts.
// Otherwise NULL is returned

const OsiRowCutDebugger * OsiSolverInterface::getRowCutDebugger() const
{
  if (rowCutDebugger_&&rowCutDebugger_->onOptimalPath(*this)) {
    return rowCutDebugger_;
  } else {
    return NULL;
  }
}

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
OsiSolverInterface::OsiSolverInterface () :
  appData_(NULL), 
  rowCutDebugger_(NULL),
  ws_(NULL),
  defaultHandler_(true)
{
  intParam_[OsiMaxNumIteration] = 9999999;
  intParam_[OsiMaxNumIterationHotStart] = 9999999;

  dblParam_[OsiDualObjectiveLimit] = DBL_MAX;
  dblParam_[OsiPrimalObjectiveLimit] = DBL_MAX;
  dblParam_[OsiDualTolerance] = 1e-6;
  dblParam_[OsiPrimalTolerance] = 1e-6;
  dblParam_[OsiObjOffset] = 0.0;

  strParam_[OsiProbName] = "OsiDefaultName";
  strParam_[OsiSolverName] = "Unknown Solver";
  handler_ = new CoinMessageHandler();
  messages_ = CoinMessage();
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
OsiSolverInterface::OsiSolverInterface (const OsiSolverInterface & rhs) :
  appData_(rhs.appData_),
  rowCutDebugger_(NULL),
  ws_(NULL),
  defaultHandler_(true)
{  
  if ( rhs.rowCutDebugger_!=NULL )
    rowCutDebugger_ = new OsiRowCutDebugger(*rhs.rowCutDebugger_);
  defaultHandler_ = rhs.defaultHandler_;
  if (defaultHandler_)
    handler_ = new CoinMessageHandler(*rhs.handler_);
  else
    handler_ = rhs.handler_;
  messages_ = CoinMessage();
  CoinDisjointCopyN(rhs.intParam_, OsiLastIntParam, intParam_);
  CoinDisjointCopyN(rhs.dblParam_, OsiLastDblParam, dblParam_);
  CoinDisjointCopyN(rhs.strParam_, OsiLastStrParam, strParam_);
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
OsiSolverInterface::~OsiSolverInterface ()
{
  // delete debugger - should be safe as only ever returned const
  delete rowCutDebugger_;
  rowCutDebugger_ = NULL;
  delete ws_;
  ws_ = NULL;
  if (defaultHandler_) {
    delete handler_;
    handler_ = NULL;
  }
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
OsiSolverInterface &
OsiSolverInterface::operator=(const OsiSolverInterface& rhs)
{
  if (this != &rhs) {
    appData_ = rhs.appData_;
    delete rowCutDebugger_;
    if ( rhs.rowCutDebugger_!=NULL )
      rowCutDebugger_ = new OsiRowCutDebugger(*rhs.rowCutDebugger_);
    else
      rowCutDebugger_ = NULL;
    CoinDisjointCopyN(rhs.intParam_, OsiLastIntParam, intParam_);
    CoinDisjointCopyN(rhs.dblParam_, OsiLastDblParam, dblParam_);
    CoinDisjointCopyN(rhs.strParam_, OsiLastStrParam, strParam_);
    delete ws_;
    ws_ = NULL;
     if (defaultHandler_) {
      delete handler_;
      handler_ = NULL;
    }
    defaultHandler_ = rhs.defaultHandler_;
    if (defaultHandler_)
      handler_ = new CoinMessageHandler(*rhs.handler_);
    else
      handler_ = rhs.handler_;
 }
  return *this;
}

//-----------------------------------------------------------------------------
// Read mps files
//-----------------------------------------------------------------------------

int OsiSolverInterface::readMps(const char * filename,
    const char * extension)
{
  std::string f(filename);
  std::string e(extension);
  std::string fullname;
  if (e!="") {
    fullname = f + "." + e;
  } else {
    // no extension so no trailing period
    fullname = f;
  }
  CoinMpsIO m;
  int numberErrors;
  m.setInfinity(getInfinity());
  
  numberErrors = m.readMps(filename,extension);
  handler_->message(COIN_SOLVER_MPS,messages_)
    <<m.getProblemName()<< numberErrors <<CoinMessageEol;
  if (!numberErrors) {

    // set objective function offest
    setDblParam(OsiObjOffset,m.objectiveOffset());

    // set problem name
    setStrParam(OsiProbName,m.getProblemName());

    // no errors
    loadProblem(*m.getMatrixByCol(),m.getColLower(),m.getColUpper(),
		m.getObjCoefficients(),m.getRowSense(),m.getRightHandSide(),
		m.getRowRange());
    const char * integer = m.integerColumns();
    if (integer) {
      int i,n=0;
      int nCols=m.getNumCols();
      int * index = new int [nCols];
      for (i=0;i<nCols;i++) {
	if (integer[i]) {
	  index[n++]=i;
	}
      }
      setInteger(index,n);
      delete [] index;
    }
  }
  return numberErrors;
}

int 
OsiSolverInterface::writeMps(const char *filename, 
			     const char ** rowNames, 
			     const char ** columnNames,
			     int formatType,
			     int numberAcross) const
{
   const int numcols = getNumCols();
   char* integrality = new char[numcols];
   bool hasInteger = false;
   for (int i = 0; i < numcols; ++i) {
      if (isInteger(i)) {
	 integrality[i] = 1;
	 hasInteger = true;
      } else {
	 integrality[i] = 0;
      }
   }

   CoinMpsIO writer;
   writer.setInfinity(getInfinity());
   writer.passInMessageHandler(handler_);
   writer.setMpsData(*getMatrixByCol(), getInfinity(),
		     getColLower(), getColUpper(),
		     getObjCoefficients(), hasInteger ? integrality : 0,
		     getRowLower(), getRowUpper(),
		     (const char **)0, (const char **)0);
   delete[] integrality;
   return writer.writeMps(filename, 1 /*gzip it*/, formatType, numberAcross);
}

// Pass in Message handler (not deleted at end)
void 
OsiSolverInterface::passInMessageHandler(CoinMessageHandler * handler)
{
  defaultHandler_=false;
  handler_=handler;
}
// Set language
void 
OsiSolverInterface::newLanguage(CoinMessages::Language language)
{
  messages_ = CoinMessage(language);
}
