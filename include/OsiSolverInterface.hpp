// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#ifndef OsiSolverInterface_H
#define OsiSolverInterface_H

#include <string>
#include <vector>

#include "OsiCollections.hpp"
#include "OsiPackedVectorBase.hpp"

#include "OsiSolverParameters.hpp"
#include "OsiMessageHandler.hpp"

class OsiPackedMatrix;
class OsiCuts;
class OsiRowCutDebugger;
class OsiWarmStart;

//#############################################################################

/** Solver Interface Abstract Base Class

Abstract Base Class for describing an interface to a solver.
Many SolverInterface methods return a const pointer to the
requested read-only data.  If the model data is changed,
then these pointers may no longer be valid and should be 
refreshed by invoking the member function to obtain an
updated copy of the pointer.

For example:
<pre>
    const double * ruBnds = solverInterfacePtr->rowupper();
    solverInterfacePtr->applyCuts(someSetOfCuts);
    // ruBnds is no longer a valid pointer and must be refreshed
    ruBnds = solverInterfacePtr->rowupper();
</pre>
*/

class OsiSolverInterface  {
   friend void OsiSolverInterfaceCommonUnitTest(
      const OsiSolverInterface* emptySi,
      const std::string & mpsDir);
   friend void OsiSolverInterfaceMpsUnitTest(
      const std::vector<OsiSolverInterface*> & vecSiP,
      const std::string & mpsDir);

public:
  /// Internal class for obtaining status from the applyCuts method 
  class ApplyCutsReturnCode {
    friend class OsiSolverInterface;
    friend class OsiOslSolverInterface;

  public:
    ///@name Constructors and desctructors
    //@{
      /// Default constructor
      ApplyCutsReturnCode():
	 intInconsistent_(0),
	 extInconsistent_(0),
	 infeasible_(0),
	 ineffective_(0),
	 applied_(0) {} 
      /// Copy constructor
      ApplyCutsReturnCode(const ApplyCutsReturnCode & rhs):
	 intInconsistent_(rhs.intInconsistent_),
	 extInconsistent_(rhs.extInconsistent_),
	 infeasible_(rhs.infeasible_),
	 ineffective_(rhs.ineffective_),
	 applied_(rhs.applied_) {} 
      /// Assignment operator
      ApplyCutsReturnCode & operator=(const ApplyCutsReturnCode& rhs)
      { 
	if (this != &rhs) { 
	  intInconsistent_ = rhs.intInconsistent_;
	  extInconsistent_ = rhs.extInconsistent_;
	  infeasible_      = rhs.infeasible_;
	  ineffective_     = rhs.ineffective_;
	  applied_         = rhs.applied_;
	}
	return *this;
      }
      /// Destructor
      ~ApplyCutsReturnCode(){}
    //@}

    /**@name Accessing return code attributes */
    //@{
      /// Number of logically inconsistent cuts
      inline int getNumInconsistent(){return intInconsistent_;}
      /// Number of cuts inconsistent with the current model
      inline int getNumInconsistentWrtIntegerModel(){return extInconsistent_;}
      /// Number of cuts that cause obvious infeasibility
      inline int getNumInfeasible(){return infeasible_;}
      /// Number of redundant or ineffective cuts
      inline int getNumIneffective(){return ineffective_;}
      /// Number of cuts applied
      inline int getNumApplied(){return applied_;}
    //@}

  private: 
    /**@name Private methods */
    //@{
      /// Increment logically inconsistent cut counter 
      inline void incrementInternallyInconsistent(){intInconsistent_++;}
      /// Increment model-inconsistent counter
      inline void incrementExternallyInconsistent(){extInconsistent_++;}
      /// Increment infeasible cut counter
      inline void incrementInfeasible(){infeasible_++;}
      /// Increment ineffective cut counter
      inline void incrementIneffective(){ineffective_++;}
      /// Increment applied cut counter
      inline void incrementApplied(){applied_++;}
    //@}

    ///@name Private member data
    //@{
      /// Counter for logically inconsistent cuts
      int intInconsistent_;
      /// Counter for model-inconsistent cuts
      int extInconsistent_;
      /// Counter for infeasible cuts
      int infeasible_;
      /// Counter for ineffective cuts
      int ineffective_;
      /// Counter for applied cuts
      int applied_;
    //@}
  };

  //---------------------------------------------------------------------------

public:
  ///@name Solve methods 
  //@{
    /// Solve initial LP relaxation 
    virtual void initialSolve() = 0; 

    /// Resolve an LP relaxation after problem modification
    virtual void resolve() = 0;

    /// Invoke solver's built-in enumeration algorithm
    virtual void branchAndBound() = 0;
  //@}

  //---------------------------------------------------------------------------
  /**@name Parameter set/get methods

     The set methods return true if the parameter was set to the given value,
     false otherwise. There can be various reasons for failure: the given
     parameter is not applicable for the solver (e.g., refactorization
     frequency for the volume algorithm), the parameter is not yet implemented
     for the solver or simply the value of the parameter is out of the range
     the solver accepts. If a parameter setting call returns false check the
     details of your solver.

     The get methods return true if the given parameter is applicable for the
     solver and is implemented. In this case the value of the parameter is
     returned in the second argument. Otherwise they return false.

     <strong>NOTE:</strong> There is a default implementation of the set/get
     methods, namely to store/retrieve the given value in an array in the base
     class. A specific solver implementation can use this feature, for
     example, to store parameters that should be used later on. Such parameter
     could be a limit on the number of iterations to be executed when doing
     hot start.
  */
  //@{
    // Set an integer parameter
    virtual bool setIntParam(OsiIntParam key, int value) {
      intParam_[key] = value;
      return true;
    }
    // Set an double parameter
    virtual bool setDblParam(OsiDblParam key, double value) {
      dblParam_[key] = value;
      return true;
    }
    // Set an string parameter
    virtual bool setStrParam(OsiStrParam key, const std::string & value) {
      strParam_[key] = value;
      return true;
    }
    // Get an integer parameter
    virtual bool getIntParam(OsiIntParam key, int& value) const {
      value = intParam_[key];
      return true;
    }
    // Get an double parameter
    virtual bool getDblParam(OsiDblParam key, double& value) const {
      value = dblParam_[key];
      return true;
    }
    // Get a string parameter
    virtual bool getStrParam(OsiStrParam key, std::string& value) const {
      value = strParam_[key];
      return true;
    }
  //@}

  //---------------------------------------------------------------------------
  ///@name Methods returning info on how the solution process terminated
  //@{
    /// Are there a numerical difficulties?
    virtual bool isAbandoned() const = 0;
    /// Is optimality proven?
    virtual bool isProvenOptimal() const = 0;
    /// Is primal infeasiblity proven?
    virtual bool isProvenPrimalInfeasible() const = 0;
    /// Is dual infeasiblity proven?
    virtual bool isProvenDualInfeasible() const = 0;
    /// Is the given primal objective limit reached?
    virtual bool isPrimalObjectiveLimitReached() const = 0;
    /// Is the given dual objective limit reached?
    virtual bool isDualObjectiveLimitReached() const = 0;
    /// Iteration limit reached?
    virtual bool isIterationLimitReached() const = 0;
  //@}

  //---------------------------------------------------------------------------
  /**@name WarmStart related methods */
  //@{
    /// Get warmstarting information
    virtual OsiWarmStart* getWarmStart() const = 0;
    /** Set warmstarting information. Return true/false depending on whether
	the warmstart information was accepted or not. */
    virtual bool setWarmStart(const OsiWarmStart* warmstart) = 0;
  //@}

  //---------------------------------------------------------------------------
  /**@name Hotstart related methods (primarily used in strong branching). <br>
     The user can create a hotstart (a snapshot) of the optimization process
     then reoptimize over and over again always starting from there.

     <strong>NOTES:</strong>:
     <ul>
     <li> Between hotstarted optimizations only bound changes are allowed.
     <li> The copy constructor and assignment operator should NOT copy any
          hotstart information.
     <li> The default implementation simply extracts a warmstarting info in
          the first method, always resets to that info in the second method
	  and deletes the info in the third method.<br>
	  <em>Actual solver implementations are encouraged to do better.</em>
     </ul>
  */
  //@{
    /// Create a hotstart point of the optimization process
    virtual void markHotStart();
    /// Optimize starting from the hotstart
    virtual void solveFromHotStart();
    /// Delete the snapshot
    virtual void unmarkHotStart();
  //@}

  //---------------------------------------------------------------------------
  /**@name Problem information methods 
     
     These methods call the solver's query routines to return
     information about the problem referred to by the current object.
     Querying a problem that has no data associated with it result in
     zeros for the number of rows and columns, and NULL pointers from
     the methods that return vectors.
     
     Const pointers returned from any data-query method are valid as
     long as the data is unchanged and the solver is not called.
  */
  //@{
    /**@name Methods related to querying the input data */
    //@{
      /// Get number of columns
      virtual int getNumCols() const = 0;
  
      /// Get number of rows
      virtual int getNumRows() const = 0;
  
      /// Get number of nonzero elements
      virtual int getNumElements() const = 0;
  
      /// Get pointer to array[getNumCols()] of column lower bounds
      virtual const double * getColLower() const = 0;
  
      /// Get pointer to array[getNumCols()] of column upper bounds
      virtual const double * getColUpper() const = 0;
  
      /** Get pointer to array[getNumRows()] of row constraint senses.
  	<ul>
  	<li>'L': <= constraint
  	<li>'E': =  constraint
  	<li>'G': >= constraint
  	<li>'R': ranged constraint
  	<li>'N': free constraint
  	</ul>
      */
      virtual const char * getRowSense() const = 0;
  
      /** Get pointer to array[getNumRows()] of rows right-hand sides
  	<ul>
  	  <li> if rowsense()[i] == 'L' then rhs()[i] == rowupper()[i]
  	  <li> if rowsense()[i] == 'G' then rhs()[i] == rowlower()[i]
  	  <li> if rowsense()[i] == 'R' then rhs()[i] == rowupper()[i]
  	  <li> if rowsense()[i] == 'N' then rhs()[i] == 0.0
  	</ul>
      */
      virtual const double * getRightHandSide() const = 0;
  
      /** Get pointer to array[getNumRows()] of row ranges.
  	<ul>
            <li> if rowsense()[i] == 'R' then
                    rowrange()[i] == rowupper()[i] - rowlower()[i]
            <li> if rowsense()[i] != 'R' then
                    rowrange()[i] is 0.0
          </ul>
      */
      virtual const double * getRowRange() const = 0;
  
      /// Get pointer to array[getNumRows()] of row lower bounds
      virtual const double * getRowLower() const = 0;
  
      /// Get pointer to array[getNumRows()] of row upper bounds
      virtual const double * getRowUpper() const = 0;
  
      /// Get pointer to array[getNumCols()] of objective function coefficients
      virtual const double * getObjCoefficients() const = 0;
  
      /// Get objective function sense (1 for min (default), -1 for max)
      virtual double getObjSense() const = 0;
  
      /// Return true if variable is continuous
      virtual bool isContinuous(int colIndex) const = 0;
  
      /// Return true if variable is binary
      virtual bool isBinary(int colIndex) const;
  
      /** Return true if column is integer.
          Note: This function returns true if the the column
          is binary or a general integer.
      */
      virtual bool isInteger(int colIndex) const;
  
      /// Return true if variable is general integer
      virtual bool isIntegerNonBinary(int colIndex) const;
  
      /// Return true if variable is binary and not fixed at either bound
      virtual bool isFreeBinary(int colIndex) const; 
    
      /// Get pointer to row-wise copy of matrix
      virtual const OsiPackedMatrix * getMatrixByRow() const = 0;
  
      /// Get pointer to column-wise copy of matrix
      virtual const OsiPackedMatrix * getMatrixByCol() const = 0;
  
      /// Get solver's value for infinity
      virtual double getInfinity() const = 0;
    //@}
    
    /**@name Methods related to querying the solution */
    //@{
      /// Get pointer to array[getNumCols()] of primal solution vector
      virtual const double * getColSolution() const = 0;
  
      /// Get pointer to array[getNumRows()] of dual prices
      virtual const double * getRowPrice() const = 0;
  
      /// Get a pointer to array[getNumCols()] of reduced costs
      virtual const double * getReducedCost() const = 0;
  
      /** Get pointer to array[getNumRows()] of row activity levels (constraint
  	matrix times the solution vector */
      virtual const double * getRowActivity() const = 0;
  
      /// Get objective function value
      virtual double getObjValue() const = 0;

      /** Get how many iterations it took to solve the problem (whatever
	  "iteration" mean to the solver. */
      virtual int getIterationCount() const = 0;
  
      /** Get as many dual rays as the solver can provide. (In case of proven
          primal infeasibility there should be at least one.)
     
          <strong>NOTE for implementers of solver interfaces:</strong> <br>
          The double pointers in the vector should point to arrays of length
          getNumRows() and they should be allocated via new[]. <br>
     
          <strong>NOTE for users of solver interfaces:</strong> <br>
          It is the user's responsibility to free the double pointers in the
          vector using delete[].
      */
      virtual std::vector<double*> getDualRays(int maxNumRays) const = 0;
      /** Get as many primal rays as the solver can provide. (In case of proven
          dual infeasibility there should be at least one.)
     
          <strong>NOTE for implementers of solver interfaces:</strong> <br>
          The double pointers in the vector should point to arrays of length
          getNumCols() and they should be allocated via new[]. <br>
     
          <strong>NOTE for users of solver interfaces:</strong> <br>
          It is the user's responsibility to free the double pointers in the
          vector using delete[].
      */
      virtual std::vector<double*> getPrimalRays(int maxNumRays) const = 0;
  
      /** Get vector of indices of solution which are integer variables 
  	presently at fractional values */
      virtual OsiVectorInt getFractionalIndices(const double etol=1.e-05)
	const;
    //@}
  //@}

  //---------------------------------------------------------------------------

  /**@name Problem modifying methods */
  //@{
    //-------------------------------------------------------------------------
    /**@name Changing bounds on variables and constraints */
    //@{
      /** Set an objective function coefficient */
      virtual void setObjCoeff( int elementIndex, double elementValue ) = 0;

      /** Set a single column lower bound<br>
    	  Use -DBL_MAX for -infinity. */
      virtual void setColLower( int elementIndex, double elementValue ) = 0;
      
      /** Set a single column upper bound<br>
    	  Use DBL_MAX for infinity. */
      virtual void setColUpper( int elementIndex, double elementValue ) = 0;
      
      /** Set a single column lower and upper bound<br>
    	  The default implementation just invokes <code>setColLower</code> and
    	  <code>setColUpper</code> */
      virtual void setColBounds( int elementIndex,
    				 double lower, double upper ) {
    	 setColLower(elementIndex, lower);
    	 setColUpper(elementIndex, upper);
      }
    
      /** Set the bounds on a number of columns simultaneously<br>
    	  The default implementation just invokes <code>setCollower</code> and
    	  <code>setColupper</code> over and over again.
    	  @param <code>[indexfirst,indexLast)</code> contains the indices of
    	         the constraints whose </em>either</em> bound changes
    	  @param indexList the indices of those variables
    	  @param boundList the new lower/upper bound pairs for the variables
      */
      virtual void setColSetBounds(const int* indexFirst,
				   const int* indexLast,
				   const double* boundList);
      
      /** Set a single row lower bound<br>
    	  Use -DBL_MAX for -infinity. */
      virtual void setRowLower( int elementIndex, double elementValue ) = 0;
      
      /** Set a single row upper bound<br>
    	  Use DBL_MAX for infinity. */
      virtual void setRowUpper( int elementIndex, double elementValue ) = 0;
    
      /** Set a single row lower and upper bound<br>
    	  The default implementation just invokes <code>setRowUower</code> and
    	  <code>setRowUpper</code> */
      virtual void setRowBounds( int elementIndex,
    				 double lower, double upper ) {
    	 setRowLower(elementIndex, lower);
    	 setRowUpper(elementIndex, upper);
      }
    
      /** Set the type of a single row<br> */
      virtual void setRowType(int index, char sense, double rightHandSide,
    			      double range) = 0;
    
      /** Set the bounds on a number of rows simultaneously<br>
    	  The default implementation just invokes <code>setRowlower</code> and
    	  <code>setRowupper</code> over and over again.
    	  @param <code>[indexfirst,indexLast)</code> contains the indices of
    	         the constraints whose </em>either</em> bound changes
    	  @param boundList the new lower/upper bound pairs for the constraints
      */
      virtual void setRowSetBounds(const int* indexFirst,
    				   const int* indexLast,
    				   const double* boundList);
    
      /** Set the type of a number of rows simultaneously<br>
    	  The default implementation just invokes <code>setRowtype</code> and
    	  over and over again.
    	  @param <code>[indexfirst,indexLast)</code> contains the indices of
    	         the constraints whose type changes
    	  @param senseList the new senses
    	  @param rhsList   the new right hand sides
    	  @param rangeList the new ranges
      */
      virtual void setRowSetTypes(const int* indexFirst,
				  const int* indexLast,
				  const char* senseList,
				  const double* rhsList,
				  const double* rangeList);
    //@}
    
    //-------------------------------------------------------------------------
    /**@name Integrality related changing methods */
    //@{
      /** Set the index-th variable to be a continuous variable */
      virtual void setContinuous(int index) = 0;
      /** Set the index-th variable to be an integer variable */
      virtual void setInteger(int index) = 0;
      /** Set the variables listed in indices (which is of length len) to be
	  continuous variables */
      virtual void setContinuous(const int* indices, int len);
      /** Set the variables listed in indices (which is of length len) to be
	  integer variables */
      virtual void setInteger(const int* indices, int len);
    //@}
    
    //-------------------------------------------------------------------------
    /// Set objective function sense (1 for min (default), -1 for max,)
    virtual void setObjSense(double s) = 0;
    
    /** Set the primal solution column values
    
    	colsol[numcols()] is an array of values of the problem column
    	variables. These values are copied to memory owned by the
    	solver object or the solver.  They will be returned as the
    	result of colsol() until changed by another call to
    	setColsol() or by a call to any solver routine.  Whether the
    	solver makes use of the solution in any way is
    	solver-dependent. 
    */
    virtual void setColSolution(const double * colsol) = 0;
    
    /** Set dual solution vector
    
    	rowprice[numrows()] is an array of values of the problem row
    	dual variables. These values are copied to memory owned by the
    	solver object or the solver.  They will be returned as the
    	result of rowprice() until changed by another call to
    	setRowprice() or by a call to any solver routine.  Whether the
    	solver makes use of the solution in any way is
    	solver-dependent. 
    */
    virtual void setRowPrice(const double * rowprice) = 0;
    
    //-------------------------------------------------------------------------
    /**@name Methods to expand a problem.<br>
       Note that if a column is added then by default it will correspond to a
       continuous variable. */
    //@{
      /** */
      virtual void addCol(const OsiPackedVectorBase& vec,
			  const double collb, const double colub,   
			  const double obj) = 0;
      /** */
      virtual void addCols(const int numcols,
			   const OsiPackedVectorBase * const * cols,
			   const double* collb, const double* colub,   
			   const double* obj);
#if 0
      /** */
      virtual void addCols(const OsiPackedMatrix& matrix,
			   const double* collb, const double* colub,   
			   const double* obj);
#endif
      /** */
      virtual void deleteCols(const int num, const int * colIndices) = 0;
    
      /** */
      virtual void addRow(const OsiPackedVectorBase& vec,
    			  const double rowlb, const double rowub) = 0;
      /** */
      virtual void addRow(const OsiPackedVectorBase& vec,
    			  const char rowsen, const double rowrhs,   
    			  const double rowrng) = 0;
      /** */
      virtual void addRows(const int numrows,
			   const OsiPackedVectorBase * const * rows,
			   const double* rowlb, const double* rowub);
      /** */
      virtual void addRows(const int numrows,
			   const OsiPackedVectorBase * const * rows,
    			   const char* rowsen, const double* rowrhs,   
    			   const double* rowrng);
#if 0
      /** */
      virtual void addRows(const OsiPackedMatrix& matrix,
    			   const double* rowlb, const double* rowub);
      /** */
      virtual void addRows(const OsiPackedMatrix& matrix,
    			   const char* rowsen, const double* rowrhs,   
    			   const double* rowrng);
#endif
      /** */
      virtual void deleteRows(const int num, const int * rowIndices) = 0;
    
      //-----------------------------------------------------------------------
      /** Apply a collection of cuts.<br>
    	  Only cuts which have an <code>effectiveness >= effectivenessLb</code>
    	  are applied.
    	  <ul>
    	    <li> ReturnCode.numberIneffective() -- number of cuts which were
                 not applied because they had an
    	         <code>effectiveness < effectivenessLb</code>
    	    <li> ReturnCode.numberInconsistent() -- number of invalid cuts
    	    <li> ReturnCode.numberInconsistentWrtIntegerModel() -- number of
                 cuts that are invalid with respect to this integer model
            <li> ReturnCode.numberInfeasible() -- number of cuts that would
    	         make this integer model infeasible
            <li> ReturnCode.numberApplied() -- number of integer cuts which
    	         were applied to the integer model
            <li> cs.size() == numberIneffective() +
                              numberInconsistent() +
    			      numberInconsistentWrtIntegerModel() +
    			      numberInfeasible() +
    			      nubmerApplied()
          </ul>
      */
      virtual ApplyCutsReturnCode applyCuts(const OsiCuts & cs,
    					    double effectivenessLb = 0.0);
    //@}
  //@}

  //---------------------------------------------------------------------------

  /**@name Methods to input a problem */
  //@{
    /** Load in an problem by copying the arguments (the constraints on the
        rows are given by lower and upper bounds). If a pointer is 0 then the
        following values are the default:
        <ul>
          <li> <code>colub</code>: all columns have upper bound infinity
          <li> <code>collb</code>: all columns have lower bound 0 
          <li> <code>rowub</code>: all rows have upper bound infinity
          <li> <code>rowlb</code>: all rows have lower bound -infinity
	  <li> <code>obj</code>: all variables have 0 objective coefficient
        </ul>
    */
    virtual void loadProblem(const OsiPackedMatrix& matrix,
			     const double* collb, const double* colub,   
			     const double* obj,
			     const double* rowlb, const double* rowub) = 0;
			    
    /** Load in an problem by assuming ownership of the arguments (the
        constraints on the rows are given by lower and upper bounds). For
        default values see the previous method. <br>
	<strong>WARNING</strong>: The arguments passed to this method will be
	freed using the C++ <code>delete</code> and <code>delete[]</code>
	functions. 
    */
    virtual void assignProblem(OsiPackedMatrix*& matrix,
			       double*& collb, double*& colub, double*& obj,
			       double*& rowlb, double*& rowub) = 0;

    /** Load in an problem by copying the arguments (the constraints on the
	rows are given by sense/rhs/range triplets). If a pointer is 0 then the
	following values are the default:
	<ul>
          <li> <code>colub</code>: all columns have upper bound infinity
          <li> <code>collb</code>: all columns have lower bound 0 
	  <li> <code>obj</code>: all variables have 0 objective coefficient
          <li> <code>rowsen</code>: all rows are >=
          <li> <code>rowrhs</code>: all right hand sides are 0
          <li> <code>rowrng</code>: 0 for the ranged rows
        </ul>
    */
    virtual void loadProblem(const OsiPackedMatrix& matrix,
			     const double* collb, const double* colub,
			     const double* obj,
			     const char* rowsen, const double* rowrhs,   
			     const double* rowrng) = 0;

    /** Load in an problem by assuming ownership of the arguments (the
        constraints on the rows are given by sense/rhs/range triplets). For
        default values see the previous method. <br>
	<strong>WARNING</strong>: The arguments passed to this method will be
	freed using the C++ <code>delete</code> and <code>delete[]</code>
	functions. 
    */
    virtual void assignProblem(OsiPackedMatrix*& matrix,
			       double*& collb, double*& colub, double*& obj,
			       char*& rowsen, double*& rowrhs,
			       double*& rowrng) = 0;

    /** Just like the other loadProblem() methods except that the matrix is
	given in a standard column major ordered format (without gaps). */
    virtual void loadProblem(const int numcols, const int numrows,
			     const int* start, const int* index,
			     const double* value,
			     const double* collb, const double* colub,   
			     const double* obj,
			     const double* rowlb, const double* rowub) = 0;

    /** Just like the other loadProblem() methods except that the matrix is
	given in a standard column major ordered format (without gaps). */
    virtual void loadProblem(const int numcols, const int numrows,
			     const int* start, const int* index,
			     const double* value,
			     const double* collb, const double* colub,   
			     const double* obj,
			     const char* rowsen, const double* rowrhs,   
			     const double* rowrng) = 0;

    /** Read an mps file from the given filename (defaults to Osi reader) - returns
	number of errors (see OsiMpsReader class) */
    virtual int readMps(const char *filename,
			 const char *extension = "mps") ;

    /** Write the problem into an mps file of the given filename */
    virtual void writeMps(const char *filename,
			  const char *extension = "mps") const = 0;
  //@}

  //---------------------------------------------------------------------------

  /**@name Setting/Accessing application data */
  //@{
    /** Set Application Data<br>
	This is a pointer that the application can store into and
	retrieve from the solveInterface.
	This field is available for the application to optionally
	define and use.
    */
    void setApplicationData (void * appData);

    /// Get Application Data<br>
    void * getApplicationData() const;
  //@}
  //---------------------------------------------------------------------------

  /**@name Message handling */
  //@{
  /// Pass in Message handler (not deleted at end)
  void passInMessageHandler(OsiMessageHandler * handler);
  /// Set language
  void newLanguage(OsiMessages::Language language);
  void setLanguage(OsiMessages::Language language)
  {newLanguage(language);};
  /// Return handler
  OsiMessageHandler * messageHandler() const
  {return handler_;};
  /// Return messages
  OsiMessages messages() 
  {return messages_;};
  /// Return pointer to messages
  OsiMessages * messagesPointer() 
  {return &messages_;};
  //@}
  //---------------------------------------------------------------------------

  /**@name Methods related to testing generated cuts */
  //@{
    /** Activate Row Cut Debugger<br>
        If the model name passed is on list of known models
	then all cuts are checked to see that they do NOT cut
	off the known optimal solution.  
    */
    void activateRowCutDebugger (const char * modelName);

    /** Get Row Cut Debugger<br>
	If there is a row cut debugger object associated with
	model AND if the known optimal solution is within the
	current feasible region then a pointer to the object is
	returned which may be used to test validity of cuts.

	Otherwise NULL is returned
    */
    const OsiRowCutDebugger * getRowCutDebugger() const;
  //@} 

  //---------------------------------------------------------------------------

  ///@name Constructors and destructors
  //@{
    /// Default Constructor
    OsiSolverInterface(); 
    
    /// Clone : FIXME: Document argument!!!
    virtual OsiSolverInterface * clone(bool copyData = true) const = 0;
  
    /// Copy constructor 
    OsiSolverInterface(const OsiSolverInterface &);
  
    /// Assignment operator 
    OsiSolverInterface & operator=(const OsiSolverInterface& rhs);
  
    /// Destructor 
    virtual ~OsiSolverInterface ();
  //@}

  //---------------------------------------------------------------------------

protected:
  ///@name Protected methods
  //@{
    /** Apply a row cut (append to constraint matrix). */
    virtual void applyRowCut( const OsiRowCut & rc ) = 0;

    /** Apply a column cut (adjust one or more bounds). */
    virtual void applyColCut( const OsiColCut & cc ) = 0;

    /** A quick inlined function to convert from lb/ub style
	constraint definition to sense/rhs/range style */
    inline void
    convertBoundToSense(const double lower, const double upper,
			char& sense, double& right, double& range) const;
    /** A quick inlined function to convert from sense/rhs/range stryle
	constraint definition to lb/ub style */
    inline void
    convertSenseToBound(const char sense, const double right,
			const double range,
			double& lower, double& upper) const;
    /** A quick inlined function to force a value to be between a minimum and
	a maximum value */
    template <class T> inline T
    forceIntoRange(const T value, const T lower, const T upper) const {
      return value < lower ? lower : (value > upper ? upper : value);
    }
  //@}
  
  //---------------------------------------------------------------------------

private:
  ///@name Private member data 
  //@{
    /// Pointer to user-defined data structure
    void * appData_;
    /// Pointer to row cut debugger object
    OsiRowCutDebugger * rowCutDebugger_;
    /// Array of integer parameters
    int intParam_[OsiLastIntParam];
    /// Array of double parameters
    double dblParam_[OsiLastDblParam];
    /// Array of string parameters
    std::string strParam_[OsiLastStrParam];

    /* The warmstart information used for hotstarting in case the default
       hotstart implementation is used */
    OsiWarmStart* ws_;
  /// Why not just make useful stuff protected
protected:
   /// Message handler
  OsiMessageHandler * handler_;
  /// Flag to say if default handler (so delete)
  bool defaultHandler_;
  /// Messages
  OsiMessages messages_;
 //@}
};

//#############################################################################
/** A function that tests the methods in the OsiSolverInterface class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging. Also, if this method is compiled with
    optimization, the compilation takes 10-15 minutes and the machine pages
    (has 256M core memory!)... */
void
OsiSolverInterfaceCommonUnitTest(
   const OsiSolverInterface* emptySi,
   const std::string & mpsDir);

//#############################################################################
/** A function that tests that a lot of problems given in MPS files (mostly
    the NETLIB problems) solve properly with all the specified solvers. */
void
OsiSolverInterfaceMpsUnitTest(
   const std::vector<OsiSolverInterface*> & vecSiP,
   const std::string & mpsDir);

//#############################################################################
/** A quick inlined function to convert from lb/ub style constraint
    definition to sense/rhs/range style */
inline void
OsiSolverInterface::convertBoundToSense(const double lower, const double upper,
					char& sense, double& right,
					double& range) const
{
  double inf = getInfinity();
  range = 0.0;
  if (lower > -inf) {
    if (upper < inf) {
      right = upper;
      if (upper==lower) {
        sense = 'E';
      } else {
        sense = 'R';
        range = upper - lower;
      }
    } else {
      sense = 'G';
      right = lower;
    }
  } else {
    if (upper < inf) {
      sense = 'L';
      right = upper;
    } else {
      sense = 'N';
      right = 0.0;
    }
  }
}

//-----------------------------------------------------------------------------
/** A quick inlined function to convert from sense/rhs/range style constraint
    definition to lb/ub style */
inline void
OsiSolverInterface::convertSenseToBound(const char sense, const double right,
					const double range,
					double& lower, double& upper) const
{
  double inf=getInfinity();
  switch (sense) {
  case 'E':
    lower = upper = right;
    break;
  case 'L':
    lower = -inf;
    upper = right;
    break;
  case 'G':
    lower = right;
    upper = inf;
    break;
  case 'R':
    lower = right - range;
    upper = right;
    break;
  case 'N':
    lower = -inf;
    upper = inf;
    break;
  }
}

#endif
