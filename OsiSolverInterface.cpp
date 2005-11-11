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
#include <cassert>
#include "CoinFinite.hpp"
#include "CoinBuild.hpp"
#include "CoinModel.hpp"
#include "CoinLpIO.hpp"

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
#if 0
// Return name of row if one exists or Rnnnnnnn
std::string 
OsiSolverInterface::getRowName(int rowIndex) const
{
  char name[9];
  sprintf(name,"R%7.7d",rowIndex);
  std::string rowName(name);
  return rowName;
}
    
// Return name of column if one exists or Cnnnnnnn
std::string 
OsiSolverInterface::getColName(int colIndex) const
{
  char name[9];
  sprintf(name,"C%7.7d",colIndex);
  std::string colName(name);
  return colName;
}
#endif    

//#############################################################################
// Built-in (i.e., slow) methods for problem modification
//#############################################################################

void
OsiSolverInterface::setObjCoeffSet(const int* indexFirst,
				  const int* indexLast,
				  const double* coeffList)
{
   const int cnt = indexLast - indexFirst;
   for (int i = 0; i < cnt; ++i) {
      setObjCoeff(indexFirst[i], coeffList[i]);
   }
}
//-----------------------------------------------------------------------------
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
/* Set the objective coefficients for all columns
    array [getNumCols()] is an array of values for the objective.
    This defaults to a series of set operations and is here for speed.
*/
void OsiSolverInterface::setObjective(const double * array)
{
  int n=getNumCols();
  for (int i=0;i<n;i++)
    setObjCoeff(i,array[i]);
}
/* Set the lower bounds for all columns
    array [getNumCols()] is an array of values for the objective.
    This defaults to a series of set operations and is here for speed.
*/
void OsiSolverInterface::setColLower(const double * array)
{
  int n=getNumCols();
  for (int i=0;i<n;i++)
    setColLower(i,array[i]);
}
/* Set the upper bounds for all columns
    array [getNumCols()] is an array of values for the objective.
    This defaults to a series of set operations and is here for speed.
*/
void OsiSolverInterface::setColUpper(const double * array)
{
  int n=getNumCols();
  for (int i=0;i<n;i++)
    setColUpper(i,array[i]);
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
/* Add a column (primal variable) to the problem. */
void 
OsiSolverInterface::addCol(int numberElements, const int * rows, const double * elements,
			   const double collb, const double colub,   
			   const double obj) 
{
  CoinPackedVector column(numberElements, rows, elements);
  addCol(column,collb,colub,obj);
}
/* Add a set of columns (primal variables) to the problem.
   
This default implementation simply makes repeated calls to addCol().
*/
void OsiSolverInterface::addCols(const int numcols,
				 const int * columnStarts, const int * rows, const double * elements,
				 const double* collb, const double* colub,   
				 const double* obj)
{
  double infinity = getInfinity();
  for (int i = 0; i < numcols; ++i) {
    int start = columnStarts[i];
    int number = columnStarts[i+1]-start;
    assert (number>=0);
    addCol(number, rows+start, elements+start, collb ? collb[i] : 0.0, 
	   colub ? colub[i] : infinity, 
	   obj ? obj[i] : 0.0);
  }
}
// Add columns from a build object
void 
OsiSolverInterface::addCols(const CoinBuild & buildObject)
{
  assert (buildObject.type()==1); // check correct
  int number = buildObject.numberColumns();
  if (number) {
    CoinPackedVectorBase ** columns=
      new CoinPackedVectorBase * [number];
    int iColumn;
    double * objective = new double [number];
    double * lower = new double [number];
    double * upper = new double [number];
    for (iColumn=0;iColumn<number;iColumn++) {
      const int * rows;
      const double * elements;
      int numberElements = buildObject.column(iColumn,lower[iColumn],
                                              upper[iColumn],objective[iColumn],
                                              rows,elements);
      columns[iColumn] = 
	new CoinPackedVector(numberElements,
			     rows,elements);
    }
    addCols(number, columns, lower, upper,objective);
    for (iColumn=0;iColumn<number;iColumn++) 
      delete columns[iColumn];
    delete [] columns;
    delete [] objective;
    delete [] lower;
    delete [] upper;
  }
  return;
}
// Add columns from a model object
int 
OsiSolverInterface::addCols( CoinModel & modelObject)
{
  bool goodState=true;
  if (modelObject.rowLowerArray()) {
    // some row information exists
    int numberRows2 = modelObject.numberRows();
    const double * rowLower = modelObject.rowLowerArray();
    const double * rowUpper = modelObject.rowUpperArray();
    for (int i=0;i<numberRows2;i++) {
      if (rowLower[i]!=-COIN_DBL_MAX) 
        goodState=false;
      if (rowUpper[i]!=COIN_DBL_MAX) 
        goodState=false;
    }
  }
  if (goodState) {
    // can do addColumns
    int numberErrors = 0;
    // Set arrays for normal use
    double * rowLower = modelObject.rowLowerArray();
    double * rowUpper = modelObject.rowUpperArray();
    double * columnLower = modelObject.columnLowerArray();
    double * columnUpper = modelObject.columnUpperArray();
    double * objective = modelObject.objectiveArray();
    int * integerType = modelObject.integerTypeArray();
    double * associated = modelObject.associatedArray();
    // If strings then do copies
    if (modelObject.stringsExist()) {
      numberErrors = modelObject.createArrays(rowLower, rowUpper, columnLower, columnUpper,
                                 objective, integerType,associated);
    }
    CoinPackedMatrix matrix;
    modelObject.createPackedMatrix(matrix,associated);
    int numberColumns = getNumCols(); // save number of columns
    int numberColumns2 = modelObject.numberColumns();
    if (numberColumns2&&!numberErrors) {
      const int * row = matrix.getIndices();
      const int * columnLength = matrix.getVectorLengths();
      const CoinBigIndex * columnStart = matrix.getVectorStarts();
      const double * element = matrix.getElements();
      CoinPackedVectorBase ** columns=
        new CoinPackedVectorBase * [numberColumns2];
      int iColumn;
      assert (columnLower);
      for (iColumn=0;iColumn<numberColumns2;iColumn++) {
        int start = columnStart[iColumn];
        columns[iColumn] = 
          new CoinPackedVector(columnLength[iColumn],
                               row+start,element+start);
      }
      addCols(numberColumns2, columns, columnLower, columnUpper,objective);
      for (iColumn=0;iColumn<numberColumns2;iColumn++) 
        delete columns[iColumn];
      delete [] columns;
      // Do integers if wanted
      assert(integerType);
      for (iColumn=0;iColumn<numberColumns2;iColumn++) {
        if (integerType[iColumn])
          setInteger(iColumn+numberColumns);
      }
    }
    if (columnLower!=modelObject.columnLowerArray()) {
      delete [] rowLower;
      delete [] rowUpper;
      delete [] columnLower;
      delete [] columnUpper;
      delete [] objective;
      delete [] integerType;
      delete [] associated;
      //if (numberErrors)
      //handler_->message(CLP_BAD_STRING_VALUES,messages_)
      //  <<numberErrors
      //  <<CoinMessageEol;
    }
    return numberErrors;
  } else {
    // not suitable for addColumns
    //handler_->message(CLP_COMPLICATED_MODEL,messages_)
    //<<modelObject.numberRows()
    //<<modelObject.numberColumns()
    //<<CoinMessageEol;
    return -1;
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
OsiSolverInterface::addRows(const CoinBuild & buildObject)
{
  int number = buildObject.numberRows();
  if (number) {
    CoinPackedVectorBase ** rows=
      new CoinPackedVectorBase * [number];
    int iRow;
    double * lower = new double [number];
    double * upper = new double [number];
    for (iRow=0;iRow<number;iRow++) {
      const int * columns;
      const double * elements;
      int numberElements = buildObject.row(iRow,lower[iRow],upper[iRow],
                                           columns,elements);
      rows[iRow] = 
	new CoinPackedVector(numberElements,
			     columns,elements);
    }
    addRows(number, rows, lower, upper);
    for (iRow=0;iRow<number;iRow++) 
      delete rows[iRow];
    delete [] rows;
    delete [] lower;
    delete [] upper;
  }
}
//-----------------------------------------------------------------------------
int
OsiSolverInterface::addRows( CoinModel & modelObject)
{
  bool goodState=true;
  if (modelObject.columnLowerArray()) {
    // some column information exists
    int numberColumns2 = modelObject.numberColumns();
    const double * columnLower = modelObject.columnLowerArray();
    const double * columnUpper = modelObject.columnUpperArray();
    const double * objective = modelObject.objectiveArray();
    const int * integerType = modelObject.integerTypeArray();
    for (int i=0;i<numberColumns2;i++) {
      if (columnLower[i]!=0.0) 
        goodState=false;
      if (columnUpper[i]!=COIN_DBL_MAX) 
        goodState=false;
      if (objective[i]!=0.0) 
        goodState=false;
      if (integerType[i]!=0)
        goodState=false;
    }
  }
  if (goodState) {
    // can do addRows
    int numberErrors = 0;
    // Set arrays for normal use
    double * rowLower = modelObject.rowLowerArray();
    double * rowUpper = modelObject.rowUpperArray();
    double * columnLower = modelObject.columnLowerArray();
    double * columnUpper = modelObject.columnUpperArray();
    double * objective = modelObject.objectiveArray();
    int * integerType = modelObject.integerTypeArray();
    double * associated = modelObject.associatedArray();
    // If strings then do copies
    if (modelObject.stringsExist()) {
      numberErrors = modelObject.createArrays(rowLower, rowUpper, columnLower, columnUpper,
                                 objective, integerType,associated);
    }
    CoinPackedMatrix matrix;
    modelObject.createPackedMatrix(matrix,associated);
    int numberRows2 = modelObject.numberRows();
    if (numberRows2&&!numberErrors) {
      // matrix by rows
      matrix.reverseOrdering();
      const int * column = matrix.getIndices();
      const int * rowLength = matrix.getVectorLengths();
      const CoinBigIndex * rowStart = matrix.getVectorStarts();
      const double * element = matrix.getElements();
      CoinPackedVectorBase ** rows=
        new CoinPackedVectorBase * [numberRows2];
      int iRow;
      assert (rowLower);
      for (iRow=0;iRow<numberRows2;iRow++) {
        int start = rowStart[iRow];
        rows[iRow] = 
          new CoinPackedVector(rowLength[iRow],
                               column+start,element+start);
      }
      addRows(numberRows2, rows, rowLower, rowUpper);
      for (iRow=0;iRow<numberRows2;iRow++) 
        delete rows[iRow];
      delete [] rows;
    }
    if (rowLower!=modelObject.rowLowerArray()) {
      delete [] rowLower;
      delete [] rowUpper;
      delete [] columnLower;
      delete [] columnUpper;
      delete [] objective;
      delete [] integerType;
      delete [] associated;
      //if (numberErrors)
      //handler_->message(CLP_BAD_STRING_VALUES,messages_)
      //  <<numberErrors
      //  <<CoinMessageEol;
    }
    return numberErrors;
  } else {
    // not suitable for addRows
    //handler_->message(CLP_COMPLICATED_MODEL,messages_)
    //<<modelObject.numberRows()
    //<<modelObject.numberColumns()
    //<<CoinMessageEol;
    return -1;
  }
}
// This loads a model from a coinModel object - returns number of errors
int 
OsiSolverInterface::loadFromCoinModel (  CoinModel & modelObject, bool keepSolution)
{
  int numberErrors = 0;
  // Set arrays for normal use
  double * rowLower = modelObject.rowLowerArray();
  double * rowUpper = modelObject.rowUpperArray();
  double * columnLower = modelObject.columnLowerArray();
  double * columnUpper = modelObject.columnUpperArray();
  double * objective = modelObject.objectiveArray();
  int * integerType = modelObject.integerTypeArray();
  double * associated = modelObject.associatedArray();
  // If strings then do copies
  if (modelObject.stringsExist()) {
    numberErrors = modelObject.createArrays(rowLower, rowUpper, columnLower, columnUpper,
                                            objective, integerType,associated);
  }
  CoinPackedMatrix matrix;
  modelObject.createPackedMatrix(matrix,associated);
  int numberRows = modelObject.numberRows();
  int numberColumns = modelObject.numberColumns();
  CoinWarmStart * ws = getWarmStart();
  bool restoreBasis = keepSolution && numberRows&&numberRows==getNumRows()&&
    numberColumns==getNumCols();
  loadProblem(matrix, 
              columnLower, columnUpper, objective, rowLower, rowUpper);
  if (restoreBasis)
    setWarmStart(ws);
  delete ws;
  // Do integers if wanted
  assert(integerType);
  for (int iColumn=0;iColumn<numberColumns;iColumn++) {
    if (integerType[iColumn])
      setInteger(iColumn);
  }
  if (rowLower!=modelObject.rowLowerArray()||
      columnLower!=modelObject.columnLowerArray()) {
    delete [] rowLower;
    delete [] rowUpper;
    delete [] columnLower;
    delete [] columnUpper;
    delete [] objective;
    delete [] integerType;
    delete [] associated;
    //if (numberErrors)
    //  handler_->message(CLP_BAD_STRING_VALUES,messages_)
    //    <<numberErrors
    //    <<CoinMessageEol;
  }
  return numberErrors;
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
/* Add a row (constraint) to the problem. */
void 
OsiSolverInterface::addRow(int numberElements, const int * columns, const double * elements,
			   const double rowlb, const double rowub) 
{
  CoinPackedVector row(numberElements, columns, elements);
  addRow(row,rowlb,rowub);
}
/* Add a set of rows (constraints) to the problem.
   
The default implementation simply makes repeated calls to addRow().
*/
void 
OsiSolverInterface::addRows(const int numrows,
			    const int * rowStarts, const int * columns, const double * elements,
			    const double* rowlb, const double* rowub)
{
  double infinity = getInfinity();
  for (int i = 0; i < numrows; ++i) {
    int start = rowStarts[i];
    int number = rowStarts[i+1]-start;
    assert (number>=0);
    addRow(number, columns+start, elements+start, rowlb ? rowlb[i] : -infinity, 
	   rowub ? rowub[i] : infinity);
  }
}


//#############################################################################
// Implement getObjValue in a simple way that the derived solver interfaces
// can use if the choose.
//#############################################################################
double OsiSolverInterface::getObjValue() const
{
  int nc = getNumCols();
  const double * objCoef = getObjCoefficients();
  const double * colSol  = getColSolution();
  double objOffset=0.0;
  getDblParam(OsiObjOffset,objOffset);
  
  // Compute dot product of objCoef and colSol and then adjust by offset
  // Jean-Sebastien pointed out this is overkill - lets just do it simply
  //double retVal = CoinPackedVector(nc,objCoef).dotProduct(colSol)-objOffset;
  double retVal = -objOffset;
  for ( int i=0 ; i<nc ; i++ )
    retVal += objCoef[i]*colSol[i];
  return retVal;
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
// And alternatively
void 
OsiSolverInterface::applyRowCuts(int numberCuts, const OsiRowCut ** cuts)
{
  int i;
  for (i=0;i<numberCuts;i++) {
    applyRowCut(*cuts[i]);
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
/* Activate debugger using full solution array.
   Only integer values need to be correct.
   Up to user to get it correct.
*/
void OsiSolverInterface::activateRowCutDebugger (const double * solution)
{
  delete rowCutDebugger_;
  rowCutDebugger_ = new OsiRowCutDebugger(*this,solution);
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
// If you want to get debugger object even if not on optimal path then use this
const OsiRowCutDebugger * OsiSolverInterface::getRowCutDebuggerAlways() const
{
  if (rowCutDebugger_&&rowCutDebugger_->active()) {
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
  rowCutDebugger_(NULL),
  appData_(NULL), 
  ws_(NULL),
  handler_(NULL),
  defaultHandler_(true)
{
  setInitialData();
}
// Set data for default constructor
void 
OsiSolverInterface::setInitialData()
{
  delete rowCutDebugger_;
  rowCutDebugger_ = NULL;
  delete ws_;
  ws_ = NULL;
  if (defaultHandler_) {
    delete handler_;
    handler_ = NULL;
  }
  defaultHandler_=true;
  appData_=NULL;
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

  // initialize all hints
  int hint;
  for (hint=OsiDoPresolveInInitial;hint<OsiLastHintParam;hint++) {
    hintParam_[hint] = false;
    hintStrength_[hint] = OsiHintIgnore;
  }
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
OsiSolverInterface::OsiSolverInterface (const OsiSolverInterface & rhs) :
  rowCutDebugger_(NULL),
  appData_(rhs.appData_),
  ws_(NULL)
{  
  if ( rhs.rowCutDebugger_!=NULL )
    rowCutDebugger_ = new OsiRowCutDebugger(*rhs.rowCutDebugger_);
  defaultHandler_ = rhs.defaultHandler_;
  if (defaultHandler_) {
    handler_ = new CoinMessageHandler(*rhs.handler_);
  } else {
    handler_ = rhs.handler_;
  }
  messages_ = CoinMessages(rhs.messages_);
  CoinDisjointCopyN(rhs.intParam_, OsiLastIntParam, intParam_);
  CoinDisjointCopyN(rhs.dblParam_, OsiLastDblParam, dblParam_);
  CoinDisjointCopyN(rhs.strParam_, OsiLastStrParam, strParam_);
  CoinDisjointCopyN(rhs.hintParam_, OsiLastHintParam, hintParam_);
  CoinDisjointCopyN(rhs.hintStrength_, OsiLastHintParam, hintStrength_);
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
    CoinDisjointCopyN(rhs.hintParam_, OsiLastHintParam, hintParam_);
    CoinDisjointCopyN(rhs.hintStrength_, OsiLastHintParam, hintStrength_);
    delete ws_;
    ws_ = NULL;
     if (defaultHandler_) {
       delete handler_;
       handler_ = NULL;
     }
    defaultHandler_ = rhs.defaultHandler_;
    if (defaultHandler_) {
      handler_ = new CoinMessageHandler(*rhs.handler_);
    } else {
      handler_ = rhs.handler_;
    }
 }
  return *this;
}

//-----------------------------------------------------------------------------
// Read mps files
//-----------------------------------------------------------------------------

int OsiSolverInterface::readMps(const char * filename,
    const char * extension)
{
  CoinMpsIO m;
  m.setInfinity(getInfinity());
  
  int numberErrors = m.readMps(filename,extension);
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
/* Read a problem in GMPL format from the given filenames.
   
Will only work if glpk installed
*/
int 
OsiSolverInterface::readGMPL(const char *filename, const char * dataname)
{
  CoinMpsIO m;
  m.setInfinity(getInfinity());
  m.passInMessageHandler(handler_);

  int numberErrors = m.readGMPL(filename,dataname,false);
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
 /* Read a problem in MPS format from the given full filename.
   
This uses CoinMpsIO::readMps() to read
the MPS file and returns the number of errors encountered.
It also may return an array of set information
*/
int 
OsiSolverInterface::readMps(const char *filename, const char*extension,
			    int & numberSets, CoinSet ** & sets)
{
  CoinMpsIO m;
  m.setInfinity(getInfinity());
  
  int numberErrors = m.readMps(filename,extension,numberSets,sets);
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
OsiSolverInterface::writeMpsNative(const char *filename, 
			     const char ** rowNames, 
			     const char ** columnNames,
			     int formatType,
			     int numberAcross,
			     double objSense) const
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

   // Get multiplier for objective function - default 1.0
   double * objective = new double[numcols];
   memcpy(objective,getObjCoefficients(),numcols*sizeof(double));
   if (objSense*getObjSense()<0.0) {
     for (int i = 0; i < numcols; ++i) 
       objective [i] = - objective[i];
   }

   CoinMpsIO writer;
   writer.setInfinity(getInfinity());
   writer.passInMessageHandler(handler_);
   writer.setMpsData(*getMatrixByCol(), getInfinity(),
		     getColLower(), getColUpper(),
		     objective, hasInteger ? integrality : 0,
		     getRowLower(), getRowUpper(),
		     columnNames,rowNames);
   double objOffset=0.0;
   getDblParam(OsiObjOffset,objOffset);
   writer.setObjectiveOffset(objOffset);
   delete [] objective;
   delete[] integrality;
   return writer.writeMps(filename, 1 /*gzip it*/, formatType, numberAcross);
}
/***********************************************************************/
int
OsiSolverInterface::writeLpNative(const char *filename, 
				  char const * const * const rowNames,
				  char const * const * const columnNames,
				  const double epsilon,
				  const int numberAcross,
				  const int decimals,
				  const double objSense,
                                  bool changeNameOnRange) const
{
   const int numcols = getNumCols();
   char *integrality = new char[numcols];
   bool hasInteger = false;

   for (int i=0; i<numcols; i++) {
     if (isInteger(i)) {
       integrality[i] = 1;
       hasInteger = true;
     } else {
       integrality[i] = 0;
     }
   }

   // Get multiplier for objective function - default 1.0
   double *objective = new double[numcols];
   const double *curr_obj = getObjCoefficients();

   if (objSense * getObjSense() < 0.0) {
     for (int i=0; i<numcols; i++) {
       objective[i] = - curr_obj[i];
     }
   }
   else {
     for (int i=0; i<numcols; i++) {
       objective[i] = curr_obj[i];
     }
   }

   CoinLpIO writer;
   writer.setEpsilon(epsilon);
   writer.setNumberAcross(numberAcross);
   writer.setDecimals(decimals);

   writer.setLpDataWithoutRowAndColNames(*getMatrixByRow(),
		     getColLower(), getColUpper(),
		     objective, hasInteger ? integrality : 0,
		     getRowLower(), getRowUpper());

   writer.setLpDataRowAndColNames(columnNames, rowNames);

   //writer.print();
   delete [] objective;
   delete[] integrality;
   return writer.writeLp(filename, epsilon, numberAcross, decimals, changeNameOnRange);

} /*writeLpNative */

/*************************************************************************/
int OsiSolverInterface::readLp(const char * filename, const double epsilon)
{
  CoinLpIO m;
  m.readLp(filename, epsilon);

  // set objective function offest
  setDblParam(OsiObjOffset, 0);

  // set problem name
  setStrParam(OsiProbName, m.getProblemName());

  // no errors
  loadProblem(*m.getMatrixByRow(), m.getColLower(), m.getColUpper(),
	      m.getObjCoefficients(), m.getRowLower(), m.getRowUpper());

  const char *integer = m.integerColumns();
  if (integer) {
    int i, n = 0;
    int nCols = m.getNumCols();
    int *index = new int [nCols];
    for (i=0; i<nCols; i++) {
      if (integer[i]) {
	index[n++] = i;
      }
    }
    setInteger(index,n);
    delete [] index;
  }
  return(0);
} /* readLp */


void OsiSolverInterface::writeLp(const char * filename,
				    const char * extension,
				    const double epsilon,
				    const int numberAcross,
				    const int decimals,
				    const double objSense,
                                 bool changeNameOnRange) const
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
  OsiSolverInterface::writeLpNative(fullname.c_str(), 
				    NULL, NULL, epsilon, numberAcross,
				    decimals, objSense,changeNameOnRange);
}

// Pass in Message handler (not deleted at end)
void 
OsiSolverInterface::passInMessageHandler(CoinMessageHandler * handler)
{
  if (defaultHandler_) {
    delete handler_;
    handler_ = NULL;
  }
  defaultHandler_=false;
  handler_=handler;
}
// Set language
void 
OsiSolverInterface::newLanguage(CoinMessages::Language language)
{
  messages_ = CoinMessage(language);
}
// copy all parameters in this section from one solver to another
void 
OsiSolverInterface::copyParameters(OsiSolverInterface & rhs)
{
  appData_ = rhs.appData_;
  delete rowCutDebugger_;
  if ( rhs.rowCutDebugger_!=NULL )
    rowCutDebugger_ = new OsiRowCutDebugger(*rhs.rowCutDebugger_);
  else
    rowCutDebugger_ = NULL;
  if (defaultHandler_) {
    delete handler_;
  }
  defaultHandler_ = rhs.defaultHandler_;
  if (defaultHandler_) {
    handler_ = new CoinMessageHandler(*rhs.handler_);
  } else {
    handler_ = rhs.handler_;
  }
   CoinDisjointCopyN(rhs.intParam_, OsiLastIntParam, intParam_);
  CoinDisjointCopyN(rhs.dblParam_, OsiLastDblParam, dblParam_);
  CoinDisjointCopyN(rhs.strParam_, OsiLastStrParam, strParam_);
  CoinDisjointCopyN(rhs.hintParam_, OsiLastHintParam, hintParam_);
  CoinDisjointCopyN(rhs.hintStrength_, OsiLastHintParam, hintStrength_);
}
// Resets as if default constructor
void 
OsiSolverInterface::reset()
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "reset",
		  "OsiSolverInterface");
}
/*Enables normal operation of subsequent functions.
  This method is supposed to ensure that all typical things (like
  reduced costs, etc.) are updated when individual pivots are executed
  and can be queried by other methods.  says whether will be
  doing primal or dual
*/
void 
OsiSolverInterface::enableSimplexInterface(bool doingPrimal) {}

//Undo whatever setting changes the above method had to make
void 
OsiSolverInterface::disableSimplexInterface() {}
/* Returns 1 if can just do getBInv etc
   2 if has all OsiSimplex methods
   and 0 if it has none */
int 
OsiSolverInterface::canDoSimplexInterface() const
{
  return 0;
}

/* Tells solver that calls to getBInv etc are about to take place.
   Underlying code may need mutable as this may be called from 
   CglCut:;generateCuts which is const.  If that is too horrific then
   each solver e.g. BCP or CBC will have to do something outside
   main loop.
*/
void 
OsiSolverInterface::enableFactorization() const
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "enableFactorization",
		  "OsiSolverInterface");
}
// and stop
void 
OsiSolverInterface::disableFactorization() const
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "disableFactorization",
		  "OsiSolverInterface");
}

//Returns true if a basis is available
bool 
OsiSolverInterface::basisIsAvailable() const 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "basisIsAvailable",
		  "OsiSolverInterface");
}

/* The following two methods may be replaced by the
   methods of OsiSolverInterface using OsiWarmStartBasis if:
   1. OsiWarmStartBasis resize operation is implemented
   more efficiently and
   2. It is ensured that effects on the solver are the same
   
   Returns a basis status of the structural/artificial variables 
   At present as warm start i.e 0 free, 1 basic, 2 upper, 3 lower
   
   NOTE  artificials are treated as +1 elements so for <= rhs
   artificial will be at lower bound if constraint is tight
*/
void 
OsiSolverInterface::getBasisStatus(int* cstat, int* rstat) const 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "getBasisStatus",
		  "OsiSolverInterface");
}

/* Set the status of structural/artificial variables and
   factorize, update solution etc 
   
   NOTE  artificials are treated as +1 elements so for <= rhs
   artificial will be at lower bound if constraint is tight
*/
int 
OsiSolverInterface::setBasisStatus(const int* cstat, const int* rstat) 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "setBasisStatus",
		  "OsiSolverInterface");
}

/* Perform a pivot by substituting a colIn for colOut in the basis. 
   The status of the leaving variable is given in statOut. Where
   1 is to upper bound, -1 to lower bound
*/
int 
OsiSolverInterface::pivot(int colIn, int colOut, int outStatus) 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "pivot",
		  "OsiSolverInterface");
}

/* Obtain a result of the primal pivot 
   Outputs: colOut -- leaving column, outStatus -- its status,
   t -- step size, and, if dx!=NULL, *dx -- primal ray direction.
   Inputs: colIn -- entering column, sign -- direction of its change (+/-1).
   Both for colIn and colOut, artificial variables are index by
   the negative of the row index minus 1.
   Return code (for now): 0 -- leaving variable found, 
   -1 -- everything else?
   Clearly, more informative set of return values is required 
*/
int 
OsiSolverInterface::primalPivotResult(int colIn, int sign, 
                                      int& colOut, int& outStatus, 
                                      double& t, CoinPackedVector* dx) 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "primalPivotResult",
		  "OsiSolverInterface");
}

/* Obtain a result of the dual pivot (similar to the previous method)
   Differences: entering variable and a sign of its change are now
   the outputs, the leaving variable and its statuts -- the inputs
   If dx!=NULL, then *dx contains dual ray
   Return code: same
*/
int 
OsiSolverInterface::dualPivotResult(int& colIn, int& sign, 
                                    int colOut, int outStatus, 
                                    double& t, CoinPackedVector* dx) 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "dualPivotResult",
		  "OsiSolverInterface");
}

//Get the reduced gradient for the cost vector c 
void 
OsiSolverInterface::getReducedGradient(double* columnReducedCosts, 
                                       double * duals,
                                       const double * c) 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "getReducedGradient",
		  "OsiSolverInterface");
}

/* Set a new objective and apply the old basis so that the
   reduced costs are properly updated */
void 
OsiSolverInterface::setObjectiveAndRefresh(double* c) 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "setObjectiveAndRefresh",
		  "OsiSolverInterface");
}

//Get a row of the tableau (slack part in slack if not NULL)
void 
OsiSolverInterface::getBInvARow(int row, double* z, double * slack) const 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "getBInvARow",
		  "OsiSolverInterface");
}

//Get a row of the basis inverse
void OsiSolverInterface::getBInvRow(int row, double* z) const 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "getBInvRow",
		  "OsiSolverInterface");
}

//Get a column of the tableau
void 
OsiSolverInterface::getBInvACol(int col, double* vec) const 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "getBInvACol",
		  "OsiSolverInterface");
}

//Get a column of the basis inverse
void 
OsiSolverInterface::getBInvCol(int col, double* vec) const 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "getBInvCol",
		  "OsiSolverInterface");
}

/* Get basic indices (order of indices corresponds to the
   order of elements in a vector retured by getBInvACol() and
   getBInvCol()).
*/
void 
OsiSolverInterface::getBasics(int* index) const 
{
  // Throw an exception
  throw CoinError("Needs coding for this interface", "getBasics",
		  "OsiSolverInterface");
}
