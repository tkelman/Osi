// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#include <cassert>

#include <time.h>
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#else
#include <sys/times.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

#include "CoinHelperFunctions.hpp"
#include "ClpDualRowSteepest.hpp"
#include "ClpPrimalColumnSteepest.hpp"
#include "ClpFactorization.hpp"
#include "ClpSimplex.hpp"
#include "OsiClpSolverInterface.hpp"
#include "OsiCuts.hpp"
#include "OsiRowCut.hpp"
#include "OsiColCut.hpp"
#include "Presolve.hpp"

static double totalTime=0.0;
static double cpuTime()
{
  double cpu_temp;
#if defined(_MSC_VER)
  unsigned int ticksnow;        /* clock_t is same as int */
  
  ticksnow = (unsigned int)clock();
  
  cpu_temp = (double)((double)ticksnow/CLOCKS_PER_SEC);
#else
  struct rusage usage;
  getrusage(RUSAGE_SELF,&usage);
  cpu_temp = usage.ru_utime.tv_sec;
  cpu_temp += 1.0e-6*((double) usage.ru_utime.tv_usec);
#endif
  return cpu_temp;
}

//#############################################################################
// Solve methods
//#############################################################################
void OsiClpSolverInterface::initialSolve()
{
  ClpSimplex solver;
  double time1 = cpuTime();
  solver.borrowModel(*modelPtr_);
  // Set message handler to have same levels etc
  solver.passInMessageHandler(handler_);
  // set reasonable defaults
  solver.scaling(1);
  solver.setDualBound(1.0e6);
  solver.setDualTolerance(1.0e-7);
  ClpDualRowSteepest steep;
  solver.setDualRowPivotAlgorithm(steep);
  solver.setPrimalTolerance(1.0e-8);
  ClpPrimalColumnSteepest steepP;
  solver.setPrimalColumnPivotAlgorithm(steepP);
  /*
    If basis then do primal (as user could do dual with resolve)
    If not then see if dual feasible (and allow for gubs etc?)
   */
  bool doPrimal = (basis_.numberBasicStructurals()>0);
  setBasis(basis_,&solver);
  //#define PRESOLVE
#ifdef PRESOLVE
  Presolve pinfo;
  ClpSimplex * model2 = pinfo.presolvedModel(solver,1.0e-8);
  // change from 200
  model2->factorization()->maximumPivots(100+model2->numberRows()/50);
  if (!doPrimal) {
    // look further
    if (solver.crash(1.0,0)>0)
      doPrimal=true;
  }
  doPrimal=false;
  if (!doPrimal) {
    // faster if bounds tightened
    //int numberInfeasibilities = model2->tightenPrimalBounds();
     model2->tightenPrimalBounds();
    //if (numberInfeasibilities)
    //std::cout<<"** Analysis indicates model infeasible"
    //       <<std::endl;
     // up dual bound for safety
     model2->setDualBound(1.0e10);
     model2->dual();
      // check if clp thought it was in a loop
    if (model2->status()==3&&
	model2->numberIterations()<model2->maximumIterations()) {
      // switch algorithm
      model2->primal();
      }
  } else {
    // up infeasibility cost for safety
    model2->setInfeasibilityCost(1.0e10);
    model2->primal();
      // check if clp thought it was in a loop
    if (model2->status()==3
	&&model2->numberIterations()<model2->maximumIterations()) {
      // switch algorithm
      model2->dual();
      }
  }
  pinfo.postsolve(true);
  
  delete model2;
  printf("Resolving from postsolved model\n");
  // later try without (1) and check duals before solve
  solver.primal(1);
  lastAlgorithm_=1; // primal
  //if (solver.numberIterations())
  //printf("****** iterated %d\n",solver.numberIterations());
#else
  if (!doPrimal) {
    // look further
    if (solver.crash(1.0,0)>0)
      doPrimal=true;
  }
  if (!doPrimal) {
    //printf("doing dual\n");
    solver.dual();
    lastAlgorithm_=2; // dual
    // check if clp thought it was in a loop
    if (solver.status()==3&&solver.numberIterations()<solver.maximumIterations()) {
      // switch algorithm
      solver.primal();
      lastAlgorithm_=1; // primal
    }
  } else {
    //printf("doing primal\n");
    solver.primal();
    lastAlgorithm_=1; // primal
    // check if clp thought it was in a loop
    if (solver.status()==3&&solver.numberIterations()<solver.maximumIterations()) {
      // switch algorithm
      solver.dual();
      lastAlgorithm_=2; // dual
    }
  }
#endif
  basis_ = getBasis(&solver);
  //basis_.print();
  solver.returnModel(*modelPtr_);
  time1 = cpuTime()-time1;
  totalTime += time1;
  //std::cout<<time1<<" seconds - total "<<totalTime<<std::endl;
}
//-----------------------------------------------------------------------------
void OsiClpSolverInterface::resolve()
{
  ClpSimplex solver;
  solver.borrowModel(*modelPtr_);
  // Set message handler to have same levels etc
  solver.passInMessageHandler(handler_);
  //basis_.print();
  setBasis(basis_,&solver);
  solver.scaling();

  ClpDualRowSteepest steep;
  solver.setDualRowPivotAlgorithm(steep);
  //solver.saveModel("save.bad");
  solver.dual();
  lastAlgorithm_=2; // dual
  // check if clp thought it was in a loop
  if (solver.status()==3&&solver.numberIterations()<solver.maximumIterations()) {
    // switch algorithm
    solver.primal();
    lastAlgorithm_=1; // primal
  }
  basis_ = getBasis(&solver);
  //basis_.print();
  solver.returnModel(*modelPtr_);
}

//#############################################################################
// Parameter related methods
//#############################################################################

bool
OsiClpSolverInterface::setIntParam(OsiIntParam key, int value)
{
   std::map<OsiIntParam, ClpIntParam>::const_iterator clpkey =
      intParamMap_.find(key);
   if (clpkey != intParamMap_.end() ) {
      return modelPtr_->setIntParam(clpkey->second, value);
   }
   return false;
}

//-----------------------------------------------------------------------------

bool
OsiClpSolverInterface::setDblParam(OsiDblParam key, double value)
{
   std::map<OsiDblParam, ClpDblParam>::const_iterator clpkey =
      dblParamMap_.find(key);
   if (clpkey != dblParamMap_.end() ) {
      return modelPtr_->setDblParam(clpkey->second, value);
   }
   return false;
}

//-----------------------------------------------------------------------------

bool
OsiClpSolverInterface::setStrParam(OsiStrParam key, const std::string & value)
{
   std::map<OsiStrParam, ClpStrParam>::const_iterator clpkey =
      strParamMap_.find(key);
   if (clpkey != strParamMap_.end() ) {
      return modelPtr_->setStrParam(clpkey->second, value);
   }
   return false;
}


//-----------------------------------------------------------------------------

bool
OsiClpSolverInterface::getIntParam(OsiIntParam key, int& value) const 
{
   std::map<OsiIntParam, ClpIntParam>::const_iterator clpkey =
      intParamMap_.find(key);
   if (clpkey != intParamMap_.end() ) {
      return modelPtr_->getIntParam(clpkey->second, value);
   }
   return false;
}

//-----------------------------------------------------------------------------

bool
OsiClpSolverInterface::getDblParam(OsiDblParam key, double& value) const
{
   std::map<OsiDblParam, ClpDblParam>::const_iterator clpkey =
      dblParamMap_.find(key);
   if (clpkey != dblParamMap_.end() ) {
      return modelPtr_->getDblParam(clpkey->second, value);
   }
   return false;
}

//-----------------------------------------------------------------------------

bool
OsiClpSolverInterface::getStrParam(OsiStrParam key, std::string & value) const
{
  if ( key==OsiSolverName ) {
    value = "clp";
    return true;
  }
   std::map<OsiStrParam, ClpStrParam>::const_iterator clpkey =
      strParamMap_.find(key);
   if (clpkey != strParamMap_.end() ) {
      return modelPtr_->getStrParam(clpkey->second, value);
   }
   return false;
}


//#############################################################################
// Methods returning info on how the solution process terminated
//#############################################################################

bool OsiClpSolverInterface::isAbandoned() const
{
  // not sure about -1 (should not happen)
  return (modelPtr_->status()==4||modelPtr_->status()==-1);
}

bool OsiClpSolverInterface::isProvenOptimal() const
{

  const int stat = modelPtr_->status();
  return (stat == 0);
}

bool OsiClpSolverInterface::isProvenPrimalInfeasible() const
{

  const int stat = modelPtr_->status();
  if (stat != 1)
     return false;
  return true;
}

bool OsiClpSolverInterface::isProvenDualInfeasible() const
{
  const int stat = modelPtr_->status();
  return stat == 2;
}

bool OsiClpSolverInterface::isPrimalObjectiveLimitReached() const
{
  double limit = 0.0;
  getDblParam(OsiPrimalObjectiveLimit, limit);
  if (limit > 1e30) {
    // was not ever set
    return false;
  }
   
  const double obj = modelPtr_->objectiveValue();
  const int maxmin = modelPtr_->optimizationDirection();

  switch (lastAlgorithm_) {
   case 0: // no simplex was needed
     return maxmin > 0 ? (obj < limit) /*minim*/ : (obj > limit) /*maxim*/;
   case 2: // dual simplex
     if (modelPtr_->status() == 0) // optimal
	return maxmin > 0 ? (obj < limit) /*minim*/ : (obj > limit) /*maxim*/;
     return false;
   case 1: // primal simplex
     return maxmin > 0 ? (obj < limit) /*minim*/ : (obj > limit) /*maxim*/;
  }
  return false; // fake return
}

bool OsiClpSolverInterface::isDualObjectiveLimitReached() const
{

  double limit = 0.0;
  getDblParam(OsiDualObjectiveLimit, limit);
  if (limit > 1e30) {
    // was not ever set
    return false;
  }
   
  const double obj = modelPtr_->objectiveValue();
  const int maxmin = modelPtr_->optimizationDirection();

  switch (lastAlgorithm_) {
   case 0: // no simplex was needed
     return maxmin > 0 ? (obj > limit) /*minim*/ : (obj < limit) /*maxim*/;
   case 1: // primal simplex
     if (modelPtr_->status() == 0) // optimal
	return maxmin > 0 ? (obj > limit) /*minim*/ : (obj < limit) /*maxim*/;
     return false;
   case 2: // dual simplex
     if (modelPtr_->status() != 0 && modelPtr_->status() != 3)
	// over dual limit
	return true;
     return maxmin > 0 ? (obj > limit) /*minim*/ : (obj < limit) /*maxim*/;
  }
  return false; // fake return
}

bool OsiClpSolverInterface::isIterationLimitReached() const
{
  const int stat = modelPtr_->status();
  return (stat == 3);
}

//#############################################################################
// WarmStart related methods
//#############################################################################

CoinWarmStart* OsiClpSolverInterface::getWarmStart() const
{

  return new CoinWarmStartBasis(basis_);
}

//-----------------------------------------------------------------------------

bool OsiClpSolverInterface::setWarmStart(const CoinWarmStart* warmstart)
{

  const CoinWarmStartBasis* ws =
    dynamic_cast<const CoinWarmStartBasis*>(warmstart);

  if (! ws)
    return false;
  basis_ = CoinWarmStartBasis(*ws);
  return true;

}

//#############################################################################
// Hotstart related methods (primarily used in strong branching)
//#############################################################################

void OsiClpSolverInterface::markHotStart()
{
  delete ws_;
  ws_ = dynamic_cast<CoinWarmStartBasis*>(getWarmStart());
  modelPtr_->getIntParam(ClpMaxNumIteration,itlimOrig_);
  int itlim;
  modelPtr_->getIntParam(ClpMaxNumIterationHotStart, itlim);
  modelPtr_->setIntParam(ClpMaxNumIteration,itlim);
  int numberRows = modelPtr_->numberRows();
  rowActivity_= new double[numberRows];
  memcpy(rowActivity_,modelPtr_->primalRowSolution(),
	 numberRows*sizeof(double));
  int numberColumns = modelPtr_->numberColumns();
  columnActivity_= new double[numberColumns];
  memcpy(columnActivity_,modelPtr_->primalColumnSolution(),
	 numberColumns*sizeof(double));

}

void OsiClpSolverInterface::solveFromHotStart()
{
  setWarmStart(ws_);
  int numberRows = modelPtr_->numberRows();
  memcpy(modelPtr_->primalRowSolution(),
	 rowActivity_,numberRows*sizeof(double));
  int numberColumns = modelPtr_->numberColumns();
  memcpy(modelPtr_->primalColumnSolution(),columnActivity_,
	 numberColumns*sizeof(double));
  resolve();
  
}

void OsiClpSolverInterface::unmarkHotStart()
{

  modelPtr_->setIntParam(ClpMaxNumIteration,itlimOrig_);
  delete ws_;
  ws_ = NULL;
  delete [] rowActivity_;
  delete [] columnActivity_;
  rowActivity_=NULL;
  columnActivity_=NULL;
}

//#############################################################################
// Problem information methods (original data)
//#############################################################################

//------------------------------------------------------------------
const char * OsiClpSolverInterface::getRowSense() const
{
  extractSenseRhsRange();
  return rowsense_;
}
//------------------------------------------------------------------
const double * OsiClpSolverInterface::getRightHandSide() const
{
  extractSenseRhsRange();
  return rhs_;
}
//------------------------------------------------------------------
const double * OsiClpSolverInterface::getRowRange() const
{
  extractSenseRhsRange();
  return rowrange_;
}
//------------------------------------------------------------------
// Return information on integrality
//------------------------------------------------------------------
bool OsiClpSolverInterface::isContinuous(int colNumber) const
{
  if ( integerInformation_==NULL ) return true;
  if ( integerInformation_[colNumber]==0 ) return true;
  return false;
}
//------------------------------------------------------------------

//------------------------------------------------------------------
// Row and column copies of the matrix ...
//------------------------------------------------------------------
const CoinPackedMatrix * OsiClpSolverInterface::getMatrixByRow() const
{
  if ( matrixByRow_ == NULL ) {
    matrixByRow_ = new CoinPackedMatrix(); 
    matrixByRow_->reverseOrderedCopyOf(*modelPtr_->matrix());
    matrixByRow_->removeGaps();
#if 0
    CoinPackedMatrix back;
    std::cout<<"start check"<<std::endl;
    back.reverseOrderedCopyOf(*matrixByRow_);
    modelPtr_->matrix()->isEquivalent2(back);
    std::cout<<"stop check"<<std::endl;
#endif
  }
  return matrixByRow_;
}

const CoinPackedMatrix * OsiClpSolverInterface::getMatrixByCol() const
{
  return modelPtr_->matrix();
}

//------------------------------------------------------------------
std::vector<double*> OsiClpSolverInterface::getDualRays(int maxNumRays) const
{
  return std::vector<double*>(1, modelPtr_->infeasibilityRay());
}
//------------------------------------------------------------------
std::vector<double*> OsiClpSolverInterface::getPrimalRays(int maxNumRays) const
{
  return std::vector<double*>(1, modelPtr_->unboundedRay());
}
//------------------------------------------------------------------

//-----------------------------------------------------------------------------
void OsiClpSolverInterface::setColSetBounds(const int* indexFirst,
					    const int* indexLast,
					    const double* boundList)
{
  double * lower = modelPtr_->columnLower();
  double * upper = modelPtr_->columnUpper();
  while (indexFirst != indexLast) {
    const int iCol=*indexFirst++;
    lower[iCol]= forceIntoRange(*boundList++, -OsiClpInfinity, OsiClpInfinity);
    upper[iCol]= forceIntoRange(*boundList++, -OsiClpInfinity, OsiClpInfinity);
  }
}
//-----------------------------------------------------------------------------
void
OsiClpSolverInterface::setRowType(int i, char sense, double rightHandSide,
				  double range)
{
  // *TEST*
  double lower, upper;
  convertSenseToBound(sense, rightHandSide, range, lower, upper);
  setRowBounds(i, lower, upper);
}
//-----------------------------------------------------------------------------
void OsiClpSolverInterface::setRowSetBounds(const int* indexFirst,
					    const int* indexLast,
					    const double* boundList)
{
  double * lower = modelPtr_->rowLower();
  double * upper = modelPtr_->rowUpper();
  const int len = indexLast - indexFirst;
  while (indexFirst != indexLast) {
    const int iRow=*indexFirst++;
    lower[iRow]= forceIntoRange(*boundList++, -OsiClpInfinity, OsiClpInfinity);
    upper[iRow]= forceIntoRange(*boundList++, -OsiClpInfinity, OsiClpInfinity);
  }
  if (rowsense_ != NULL) {
    assert ((rhs_ != NULL) && (rowrange_ != NULL));
    indexFirst -= len;
    while (indexFirst != indexLast) {
      const int iRow=*indexFirst++;
      convertBoundToSense(lower[iRow], upper[iRow],
			  rowsense_[iRow], rhs_[iRow], rowrange_[iRow]);
    }
  }
}
//-----------------------------------------------------------------------------
void
OsiClpSolverInterface::setRowSetTypes(const int* indexFirst,
				      const int* indexLast,
				      const char* senseList,
				      const double* rhsList,
				      const double* rangeList)
{
  double * lower = modelPtr_->rowLower();
  double * upper = modelPtr_->rowUpper();
  const int len = indexLast - indexFirst;
  while (indexFirst != indexLast) {
    const int iRow= *indexFirst++;
    convertSenseToBound(*senseList++, *rhsList++, *rangeList++,
			lower[iRow], upper[iRow]);
  }
  if (rowsense_ != NULL) {
    assert ((rhs_ != NULL) && (rowrange_ != NULL));
    indexFirst -= len;
    senseList -= len;
    rhsList -= len;
    rangeList -= len;
    while (indexFirst != indexLast) {
      const int iRow=*indexFirst++;
      rowsense_[iRow] = *senseList++;
      rhs_[iRow] = *rhsList++;
      rowrange_[iRow] = *rangeList++;
    }
  }
}
//#############################################################################
void
OsiClpSolverInterface::setContinuous(int index)
{

  if (integerInformation_) {
    integerInformation_[index]=0;
  }
}
//-----------------------------------------------------------------------------
void
OsiClpSolverInterface::setInteger(int index)
{
  if (!integerInformation_) {
    integerInformation_ = new char[modelPtr_->numberColumns()];
    CoinFillN ( integerInformation_, modelPtr_->numberColumns(),(char) 0);
  }
  integerInformation_[index]=1;
}
//-----------------------------------------------------------------------------
void
OsiClpSolverInterface::setContinuous(const int* indices, int len)
{
  if (integerInformation_) {
    int i;
    for (i=0; i<len;i++) {
      integerInformation_[i]=0;
    }
  }
}
//-----------------------------------------------------------------------------
void
OsiClpSolverInterface::setInteger(const int* indices, int len)
{
  if (!integerInformation_) {
    integerInformation_ = new char[modelPtr_->numberColumns()];
    CoinFillN ( integerInformation_, modelPtr_->numberColumns(),(char) 0);
  }
  int i;
  for (i=0; i<len;i++) {
    integerInformation_[indices[i]]=1;
  }
}
//-----------------------------------------------------------------------------
void OsiClpSolverInterface::setColSolution(const double * cs) 
{
  CoinDisjointCopyN(cs,modelPtr_->numberColumns(),
		    modelPtr_->primalColumnSolution());
}
//-----------------------------------------------------------------------------
void OsiClpSolverInterface::setRowPrice(const double * rs) 
{
  CoinDisjointCopyN(rs,modelPtr_->numberRows(),
		    modelPtr_->dualRowSolution());
}

//#############################################################################
// Problem modifying methods (matrix)
//#############################################################################
void 
OsiClpSolverInterface::addCol(const CoinPackedVectorBase& vec,
			      const double collb, const double colub,   
			      const double obj)
{
  int numberColumns = modelPtr_->numberColumns();
  modelPtr_->resize(modelPtr_->numberRows(),numberColumns+1);
  basis_.resize(modelPtr_->numberRows(),numberColumns+1);
  setColBounds(numberColumns,collb,colub);
  setObjCoeff(numberColumns,obj);
  if (!modelPtr_->clpMatrix())
    modelPtr_->createEmptyMatrix();
  modelPtr_->matrix()->appendCol(vec);
  if (integerInformation_) {
    char * temp = new char[numberColumns+1];
    memcpy(temp,integerInformation_,numberColumns*sizeof(char));
    delete [] integerInformation_;
    integerInformation_ = temp;
    integerInformation_[numberColumns]=0;
  }
  freeCachedResults();
}
//-----------------------------------------------------------------------------
void 
OsiClpSolverInterface::addCols(const int numcols,
			       const CoinPackedVectorBase * const * cols,
			       const double* collb, const double* colub,   
			       const double* obj)
{
  int numberColumns = modelPtr_->numberColumns();
  modelPtr_->resize(modelPtr_->numberRows(),numberColumns+numcols);
  basis_.resize(modelPtr_->numberRows(),numberColumns+numcols);
  double * lower = modelPtr_->columnLower()+numberColumns;
  double * upper = modelPtr_->columnUpper()+numberColumns;
  double * objective = modelPtr_->objective()+numberColumns;
  int iCol;
  for (iCol = 0; iCol < numcols; iCol++) {
    lower[iCol]= forceIntoRange(collb[iCol], -OsiClpInfinity, OsiClpInfinity);
    upper[iCol]= forceIntoRange(colub[iCol], -OsiClpInfinity, OsiClpInfinity);
    objective[iCol] = obj[iCol];
  }
  if (!modelPtr_->clpMatrix())
    modelPtr_->createEmptyMatrix();
  modelPtr_->matrix()->appendCols(numcols,cols);
  if (integerInformation_) {
    char * temp = new char[numberColumns+numcols];
    memcpy(temp,integerInformation_,numberColumns*sizeof(char));
    delete [] integerInformation_;
    integerInformation_ = temp;
    for (iCol = 0; iCol < numcols; iCol++) 
      integerInformation_[numberColumns+iCol]=0;
  }
  freeCachedResults();
}
//-----------------------------------------------------------------------------
void 
OsiClpSolverInterface::deleteCols(const int num, const int * columnIndices)
{
  modelPtr_->deleteColumns(num,columnIndices);
  basis_.deleteColumns(num,columnIndices);
  freeCachedResults();
}
//-----------------------------------------------------------------------------
void 
OsiClpSolverInterface::addRow(const CoinPackedVectorBase& vec,
			      const double rowlb, const double rowub)
{
  int numberRows = modelPtr_->numberRows();
  modelPtr_->resize(numberRows+1,modelPtr_->numberColumns());
  basis_.resize(numberRows+1,modelPtr_->numberColumns());
  setRowBounds(numberRows,rowlb,rowub);
  if (!modelPtr_->clpMatrix())
    modelPtr_->createEmptyMatrix();
  modelPtr_->matrix()->appendRow(vec);
  freeCachedResults();
}
//-----------------------------------------------------------------------------
void 
OsiClpSolverInterface::addRow(const CoinPackedVectorBase& vec,
			      const char rowsen, const double rowrhs,   
			      const double rowrng)
{
  int numberRows = modelPtr_->numberRows();
  modelPtr_->resize(numberRows+1,modelPtr_->numberColumns());
  basis_.resize(numberRows+1,modelPtr_->numberColumns());
  double rowlb, rowub;
  convertSenseToBound(rowsen, rowrhs, rowrng, rowlb, rowub);
  setRowBounds(numberRows,rowlb,rowub);
  if (!modelPtr_->clpMatrix())
    modelPtr_->createEmptyMatrix();
  modelPtr_->matrix()->appendRow(vec);
  freeCachedResults();
}
//-----------------------------------------------------------------------------
void 
OsiClpSolverInterface::addRows(const int numrows,
			       const CoinPackedVectorBase * const * rows,
			       const double* rowlb, const double* rowub)
{
  int numberRows = modelPtr_->numberRows();
  modelPtr_->resize(numberRows+numrows,modelPtr_->numberColumns());
  basis_.resize(numberRows+numrows,modelPtr_->numberColumns());
  double * lower = modelPtr_->rowLower()+numberRows;
  double * upper = modelPtr_->rowUpper()+numberRows;
  int iRow;
  for (iRow = 0; iRow < numrows; iRow++) {
    lower[iRow]= forceIntoRange(rowlb[iRow], -OsiClpInfinity, OsiClpInfinity);
    upper[iRow]= forceIntoRange(rowub[iRow], -OsiClpInfinity, OsiClpInfinity);
  }
  if (!modelPtr_->clpMatrix())
    modelPtr_->createEmptyMatrix();
  modelPtr_->matrix()->appendRows(numrows,rows);
  freeCachedResults();
}
//-----------------------------------------------------------------------------
void 
OsiClpSolverInterface::addRows(const int numrows,
			       const CoinPackedVectorBase * const * rows,
			       const char* rowsen, const double* rowrhs,   
			       const double* rowrng)
{
  int numberRows = modelPtr_->numberRows();
  modelPtr_->resize(numberRows+numrows,modelPtr_->numberColumns());
  basis_.resize(numberRows+numrows,modelPtr_->numberColumns());
  double * lower = modelPtr_->rowLower()+numberRows;
  double * upper = modelPtr_->rowUpper()+numberRows;
  int iRow;
  for (iRow = 0; iRow < numrows; iRow++) {
    double rowlb, rowub;
    convertSenseToBound(rowsen[iRow], rowrhs[iRow], rowrng[iRow], 
			rowlb, rowub);
    lower[iRow]= forceIntoRange(rowlb, -OsiClpInfinity, OsiClpInfinity);
    upper[iRow]= forceIntoRange(rowub, -OsiClpInfinity, OsiClpInfinity);
  }
  if (!modelPtr_->clpMatrix())
    modelPtr_->createEmptyMatrix();
  modelPtr_->matrix()->appendRows(numrows,rows);
  freeCachedResults();
}
//-----------------------------------------------------------------------------
void 
OsiClpSolverInterface::deleteRows(const int num, const int * rowIndices)
{
  modelPtr_->deleteRows(num,rowIndices);
  basis_.deleteRows(num,rowIndices);
  freeCachedResults();
}

//#############################################################################
// Methods to input a problem
//#############################################################################

void
OsiClpSolverInterface::loadProblem(const CoinPackedMatrix& matrix,
				   const double* collb, const double* colub,   
				   const double* obj,
				   const double* rowlb, const double* rowub)
{
  modelPtr_->loadProblem(matrix, collb, colub, obj, rowlb, rowub);
  freeCachedResults();

}

//-----------------------------------------------------------------------------

void
OsiClpSolverInterface::assignProblem(CoinPackedMatrix*& matrix,
				     double*& collb, double*& colub,
				     double*& obj,
				     double*& rowlb, double*& rowub)
{
   modelPtr_->loadProblem(*matrix, collb, colub, obj, rowlb, rowub);
   freeCachedResults();
   delete matrix;   matrix = NULL;
   delete[] collb;  collb = NULL;
   delete[] colub;  colub = NULL;
   delete[] obj;    obj = NULL;
   delete[] rowlb;  rowlb = NULL;
   delete[] rowub;  rowub = NULL;
}

//-----------------------------------------------------------------------------

void
OsiClpSolverInterface::loadProblem(const CoinPackedMatrix& matrix,
				   const double* collb, const double* colub,
				   const double* obj,
				   const char* rowsen, const double* rowrhs,   
				   const double* rowrng)
{
   assert( rowsen != NULL );
   assert( rowrhs != NULL );
   int numrows = matrix.getNumRows();
   double * rowlb = new double[numrows];
   double * rowub = new double[numrows];
   for (int i = numrows-1; i >= 0; --i) {   
      convertSenseToBound(rowsen[i],rowrhs[i],rowrng[i],rowlb[i],rowub[i]);
   }
   modelPtr_->loadProblem(matrix, collb, colub, obj, rowlb, rowub);
   freeCachedResults();
   delete [] rowlb;
   delete [] rowub;
}

//-----------------------------------------------------------------------------

void
OsiClpSolverInterface::assignProblem(CoinPackedMatrix*& matrix,
				     double*& collb, double*& colub,
				     double*& obj,
				     char*& rowsen, double*& rowrhs,
				     double*& rowrng)
{
   loadProblem(*matrix, collb, colub, obj, rowsen, rowrhs, rowrng);
   delete matrix;   matrix = NULL;
   delete[] collb;  collb = NULL;
   delete[] colub;  colub = NULL;
   delete[] obj;    obj = NULL;
   delete[] rowsen; rowsen = NULL;
   delete[] rowrhs; rowrhs = NULL;
   delete[] rowrng; rowrng = NULL;
}

//-----------------------------------------------------------------------------

void
OsiClpSolverInterface::loadProblem(const int numcols, const int numrows,
				   const int* start, const int* index,
				   const double* value,
				   const double* collb, const double* colub,
				   const double* obj,
				   const double* rowlb, const double* rowub)
{
  modelPtr_->loadProblem(numcols, numrows, start,  index,
	    value, collb, colub, obj,
	    rowlb,  rowub);
  freeCachedResults();
}
//-----------------------------------------------------------------------------

void
OsiClpSolverInterface::loadProblem(const int numcols, const int numrows,
				   const int* start, const int* index,
				   const double* value,
				   const double* collb, const double* colub,
				   const double* obj,
				   const char* rowsen, const double* rowrhs,   
				   const double* rowrng)
{
   assert( rowsen != NULL );
   assert( rowrhs != NULL );
   double * rowlb = new double[numrows];
   double * rowub = new double[numrows];
   for (int i = numrows-1; i >= 0; --i) {   
      convertSenseToBound(rowsen[i],rowrhs[i],rowrng[i],rowlb[i],rowub[i]);
   }
   modelPtr_->loadProblem(numcols, numrows, start,  index,
	     value, collb, colub, obj,
	     rowlb,  rowub);
   freeCachedResults();
   delete[] rowlb;
   delete[] rowub;
}

//-----------------------------------------------------------------------------
// Write mps files
//-----------------------------------------------------------------------------

void OsiClpSolverInterface::writeMps(const char * filename,
				     const char * extension) const
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
  // Fall back on Osi version - without names
  OsiSolverInterface::writeMps(fullname.c_str(), NULL, NULL);
}

int 
OsiClpSolverInterface::writeMps(const char *filename, 
		  const char ** rowNames, const char ** columnNames,
		  int formatType,int numberAcross) const 
{
  return OsiSolverInterface::writeMps(filename, rowNames, columnNames,
			       formatType, numberAcross);
}

//#############################################################################
// CLP specific public interfaces
//#############################################################################

ClpSimplex * OsiClpSolverInterface::getModelPtr() const
{
  freeCachedResults();
  return modelPtr_;
}

//------------------------------------------------------------------- 

//#############################################################################
// Constructors, destructors clone and assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
OsiClpSolverInterface::OsiClpSolverInterface ()
:
OsiSolverInterface(),
rowsense_(NULL),
rhs_(NULL),
rowrange_(NULL),
ws_(NULL),
rowActivity_(NULL),
columnActivity_(NULL),
basis_(),  
itlimOrig_(9999999),
lastAlgorithm_(0),
notOwned_(false),
matrixByRow_(NULL),
integerInformation_(NULL)
{
   modelPtr_ = new ClpSimplex();
   fillParamMaps();
}

//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
OsiSolverInterface * OsiClpSolverInterface::clone(bool CopyData) const
{
   if (CopyData) {
      return new OsiClpSolverInterface(*this);
   } else {
      return new OsiClpSolverInterface();
   }
}


//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
OsiClpSolverInterface::OsiClpSolverInterface (
                  const OsiClpSolverInterface & rhs)
:
OsiSolverInterface(rhs),
rowsense_(NULL),
rhs_(NULL),
rowrange_(NULL),
ws_(NULL),
rowActivity_(NULL),
columnActivity_(NULL),
basis_(),
itlimOrig_(9999999),
lastAlgorithm_(0),
notOwned_(false),
matrixByRow_(NULL),
integerInformation_(NULL)
{
  if ( rhs.modelPtr_  ) 
    modelPtr_ = new ClpSimplex(*rhs.modelPtr_);
  else
    modelPtr_ = new ClpSimplex();
  if ( rhs.ws_ ) 
    ws_ = new CoinWarmStartBasis(*rhs.ws_);
  basis_ = rhs.basis_;
  if (rhs.integerInformation_) {
    int numberColumns = modelPtr_->numberColumns();
    integerInformation_ = new char[numberColumns];
    memcpy(integerInformation_,rhs.integerInformation_,
	   numberColumns*sizeof(char));
  }
  fillParamMaps();
}

// Borrow constructor - only delete one copy
OsiClpSolverInterface::OsiClpSolverInterface (ClpSimplex * rhs)
:
OsiSolverInterface(),
rowsense_(NULL),
rhs_(NULL),
rowrange_(NULL),
ws_(NULL),
rowActivity_(NULL),
columnActivity_(NULL),
basis_(),
itlimOrig_(9999999),
lastAlgorithm_(0),
notOwned_(false),
matrixByRow_(NULL),
integerInformation_(NULL)
{
  modelPtr_ = rhs;
  if (rhs) {
    notOwned_=true;

    if (rhs->integerInformation()) {
      int numberColumns = modelPtr_->numberColumns();
      integerInformation_ = new char[numberColumns];
      memcpy(integerInformation_,rhs->integerInformation(),
	     numberColumns*sizeof(char));
    }
  }
  fillParamMaps();
}
    
// Releases so won't error
void 
OsiClpSolverInterface::releaseClp()
{
  modelPtr_=NULL;
  notOwned_=false;
}
    

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
OsiClpSolverInterface::~OsiClpSolverInterface ()
{
  freeCachedResults();
  if (!notOwned_)
    delete modelPtr_;
  delete ws_;
  delete [] rowActivity_;
  delete [] columnActivity_;
  delete [] integerInformation_;
}

//-------------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
OsiClpSolverInterface &
OsiClpSolverInterface::operator=(const OsiClpSolverInterface& rhs)
{
  if (this != &rhs) {    
    OsiSolverInterface::operator=(rhs);
    freeCachedResults();
    if (!notOwned_)
      delete modelPtr_;
    delete ws_;
    if ( rhs.modelPtr_  ) 
      modelPtr_ = new ClpSimplex(*rhs.modelPtr_);
    notOwned_=false;
    
    if ( rhs.ws_ ) 
      ws_ = new CoinWarmStartBasis(*rhs.ws_);
    delete [] rowActivity_;
    delete [] columnActivity_;
    rowActivity_=NULL;
    columnActivity_=NULL;
    basis_ = rhs.basis_;
    intParamMap_ = rhs.intParamMap_;
    dblParamMap_ = rhs.dblParamMap_;
    strParamMap_ = rhs.strParamMap_;
  }
  return *this;
}

//#############################################################################
// Applying cuts
//#############################################################################

void OsiClpSolverInterface::applyRowCut( const OsiRowCut & rowCut )
{
  const CoinPackedVector & row=rowCut.row();
  addRow(row ,  rowCut.lb(),rowCut.ub());
}
/* Apply a collection of row cuts which are all effective.
   applyCuts seems to do one at a time which seems inefficient.
*/
void 
OsiClpSolverInterface::applyRowCuts(int numberCuts, const OsiRowCut * cuts)
{
  int i;
  if (!numberCuts)
    return;

  const CoinPackedVectorBase * * rows
    =     new const CoinPackedVectorBase * [numberCuts];
  double * rowlb = new double [numberCuts];
  double * rowub = new double [numberCuts];
  for (i=0;i<numberCuts;i++) {
    rowlb[i] = cuts[i].lb();
    rowub[i] = cuts[i].ub();
    rows[i] = &cuts[i].row();
#ifdef TAKEOUT
    if (rows[i]->getNumElements()==10||rows[i]->getNumElements()==15)
      printf("ApplyCuts %d size %d\n",getNumRows()+i,rows[i]->getNumElements());
#endif
  }
  addRows(numberCuts,rows,rowlb,rowub);
  delete [] rows;
  delete [] rowlb;
  delete [] rowub;

}

//-----------------------------------------------------------------------------

void OsiClpSolverInterface::applyColCut( const OsiColCut & cc )
{
  double * lower = modelPtr_->columnLower();
  double * upper = modelPtr_->columnUpper();
  const CoinPackedVector & lbs = cc.lbs();
  const CoinPackedVector & ubs = cc.ubs();
  int i;

  for ( i=0; i<lbs.getNumElements(); i++ ) {
    int iCol = lbs.getIndices()[i];
    double value = lbs.getElements()[i];
    if ( value > lower[iCol] )
      lower[iCol]= value;
  }
  for ( i=0; i<ubs.getNumElements(); i++ ) {
    int iCol = ubs.getIndices()[i];
    double value = ubs.getElements()[i];
    if ( value < upper[iCol] )
      upper[iCol]= value;
  }
}
//#############################################################################
// Private methods
//#############################################################################


//------------------------------------------------------------------- 

void OsiClpSolverInterface::freeCachedResults() const
{  
  delete [] rowsense_;
  delete [] rhs_;
  delete [] rowrange_;
  delete matrixByRow_;
  delete ws_;
  rowsense_=NULL;
  rhs_=NULL;
  rowrange_=NULL;
  matrixByRow_=NULL;
  ws_ = NULL;
}

//------------------------------------------------------------------
void OsiClpSolverInterface::extractSenseRhsRange() const
{
  if (rowsense_ == NULL) {
    // all three must be NULL
    assert ((rhs_ == NULL) && (rowrange_ == NULL));
    
    int nr=modelPtr_->numberRows();
    if ( nr!=0 ) {
      rowsense_ = new char[nr];
      rhs_ = new double[nr];
      rowrange_ = new double[nr];
      std::fill(rowrange_,rowrange_+nr,0.0);
      
      const double * lb = modelPtr_->rowLower();
      const double * ub = modelPtr_->rowUpper();
      
      int i;
      for ( i=0; i<nr; i++ ) {
        convertBoundToSense(lb[i], ub[i], rowsense_[i], rhs_[i], rowrange_[i]);
      }
    }
  }
}
// Set language
void 
OsiClpSolverInterface::newLanguage(CoinMessages::Language language)
{
  modelPtr_->newLanguage(language);
  OsiSolverInterface::newLanguage(language);
}
//#############################################################################

void
OsiClpSolverInterface::fillParamMaps()
{
   intParamMap_[OsiMaxNumIteration]         = ClpMaxNumIteration;
   intParamMap_[OsiMaxNumIterationHotStart] = ClpMaxNumIterationHotStart;
   intParamMap_[OsiLastIntParam]            = ClpLastIntParam;

   dblParamMap_[OsiDualObjectiveLimit]   = ClpDualObjectiveLimit;
   dblParamMap_[OsiPrimalObjectiveLimit] = ClpPrimalObjectiveLimit;
   dblParamMap_[OsiDualTolerance]        = ClpDualTolerance;
   dblParamMap_[OsiPrimalTolerance]      = ClpPrimalTolerance;
   dblParamMap_[OsiObjOffset]            = ClpObjOffset;
   dblParamMap_[OsiLastDblParam]         = ClpLastDblParam;

   strParamMap_[OsiProbName]     = ClpProbName;
   strParamMap_[OsiLastStrParam] = ClpLastStrParam;
}
// Warm start
CoinWarmStartBasis
OsiClpSolverInterface::getBasis(ClpSimplex * model) const
{
  int iRow,iColumn;
  int numberRows = model->numberRows();
  int numberColumns = model->numberColumns();
  CoinWarmStartBasis basis;
  basis.setSize(numberColumns,numberRows);

  if (model->statusExists()) {
    for (iRow=0;iRow<numberRows;iRow++) {
      basis.setArtifStatus(iRow,
			   (CoinWarmStartBasis::Status) model->getRowStatus(iRow));
    }
    for (iColumn=0;iColumn<numberColumns;iColumn++) {
      basis.setStructStatus(iColumn,
		       (CoinWarmStartBasis::Status) model->getColumnStatus(iColumn));
    }
  }
  //basis.print();
  return basis;
}
// Sets up basis
void 
OsiClpSolverInterface::setBasis ( const CoinWarmStartBasis & basis,
				  ClpSimplex * model)
{
  // transform basis to status arrays
  int iRow,iColumn;
  int numberRows = model->numberRows();
  int numberColumns = model->numberColumns();
  if (!model->statusExists()) {
    /*
      get status arrays
      ClpBasis would seem to have overheads and we will need
      extra bits anyway.
    */
    model->createStatus();
  }
  CoinWarmStartBasis basis2 = basis;
  // resize if necessary
  basis2.resize(numberRows,numberColumns);
  // move status
  model->createStatus();
  for (iRow=0;iRow<numberRows;iRow++) {
    model->setRowStatus(iRow,
		 (ClpSimplex::Status) basis2.getArtifStatus(iRow));
  }
  for (iColumn=0;iColumn<numberColumns;iColumn++) {
    model->setColumnStatus(iColumn,
		    (ClpSimplex::Status) basis2.getStructStatus(iColumn));
  }
}
