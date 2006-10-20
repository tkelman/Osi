// Copyright (C) 2006, International Business Machines
// Corporation and others.  All Rights Reserved.
#ifndef OsiBranchingObject_H
#define OsiBranchingObject_H

#include <string>
#include <vector>

class OsiSolverInterface;
class OsiSolverBranch;

class OsiBranchingObject;
class OsiBranchingInformation;

//#############################################################################
//This contains the abstract base class for an object and for branching.
//It also contains a simple integer class
//#############################################################################

/** Abstract base class for `objects'.

  The branching model used in Osi is based on the idea of an <i>object</i>.
  In the abstract, an object is something that has a feasible region, can be
  evaluated for infeasibility, can be branched on (<i>i.e.</i>, there's some
  constructive action to be taken to move toward feasibility), and allows
  comparison of the effect of branching.

  This class (OsiObject) is the base class for an object. To round out the
  branching model, the class OsiBranchingObject describes how to perform a
  branch, and the class OsiBranchDecision describes how to compare two
  OsiBranchingObjects.

  To create a new type of object you need to provide three methods:
  #infeasibility(), #feasibleRegion(), and #createBranch(), described below.

  This base class is primarily virtual to allow for any form of structure.
  Any form of discontinuity is allowed.

  As there is an overhead in getting information from solvers and because
  other useful information is available there is also an OsiBranchingInformation 
  class which can contain pointers to information.
  If used it must at minimum contain pointers to current value of objective,
  maximum allowed objective and pointers to arrays for bounds and solution
  and direction of optimization.  Also integer and primal tolerance.
  
  Classes which inherit might have other information such as depth, number of
  solutions, pseudo-shadow prices etc etc.
  May be easier just to throw in here - as I keep doing
*/
class OsiObject {

public:
  
  /// Default Constructor 
  OsiObject ();
  
  /// Copy constructor 
  OsiObject ( const OsiObject &);
  
  /// Assignment operator 
  OsiObject & operator=( const OsiObject& rhs);
  
  /// Clone
  virtual OsiObject * clone() const=0;
  
  /// Destructor 
  virtual ~OsiObject ();
  
  /** Infeasibility of the object
      
    This is some measure of the infeasibility of the object. 0.0 
    indicates that the object is satisfied.
  
    The preferred branching direction is returned in whichWay, where for
    normal two-way branching 0 is down, 1 is up
  
    This is used to prepare for strong branching but should also think of
    case when no strong branching
  
    The object may also compute an estimate of cost of going "up" or "down".
    This will probably be based on pseudo-cost ideas

    This should also set mutable infeasibility_ and whichWay_
    This is for instant re-use for speed

    Default for this just calls infeasibility with OsiBranchingInformation
  */
  virtual double infeasibility(const OsiSolverInterface * solver,int &whichWay) const ;
  // Faster version when more information available
  virtual double infeasibility(const OsiBranchingInformation * info, int &whichWay) const =0;
  // This does NOT set mutable stuff
  double checkInfeasibility(const OsiBranchingInformation * info) const;
  
  /** For the variable(s) referenced by the object,
      look at the current solution and set bounds to match the solution.
      Returns measure of how much it had to move solution to make feasible
  */
  virtual double feasibleRegion(OsiSolverInterface * solver) const ;
  /** For the variable(s) referenced by the object,
      look at the current solution and set bounds to match the solution.
      Returns measure of how much it had to move solution to make feasible
      Faster version
  */
  virtual double feasibleRegion(OsiSolverInterface * solver, const OsiBranchingInformation * info) const =0;
  
  /** Create a branching object and indicate which way to branch first.
      
      The branching object has to know how to create branches (fix
      variables, etc.)
  */
  virtual OsiBranchingObject * createBranch(OsiSolverInterface * solver, const OsiBranchingInformation * info, int way) const = 0;
  
  /** \brief Return true if object can take part in normal heuristics
  */
  virtual bool canDoHeuristics() const 
  {return true;};
  /** Column number if single column object -1 otherwise,
      Used by heuristics
  */
  virtual int columnNumber() const;
  /// Return Priority - note 1 is highest priority
  inline int priority() const
  { return priority_;};
  /// Set priority
  inline void setPriority(int priority)
  { priority_ = priority;};
  /** \brief Return true if branch should only bound variables
  */
  virtual bool boundBranch() const 
  {return true;};
  /// Return maximum number of ways branch may have
  inline int numberWays() const
  { return numberWays_;};
  /// Set maximum number of ways branch may have
  inline void setNumberWays(int numberWays)
  { numberWays_ = numberWays;};
  /** Return preferred way to branch.  If two
      then way=0 means down and 1 means up, otherwise
      way points to preferred branch
  */
  inline void setWhichWay(int way)
  { whichWay_ = way;};
  /** Return preferred way to branch.  If two
      then way=0 means down and 1 means up, otherwise
      way points to preferred branch
  */
  inline int whichWay() const
  { return whichWay_;};
  /// Return infeasibility
  inline double infeasibility() const
  { return infeasibility_;};
  /// Return "up" estimate (default 1.0e-5)
  virtual double upEstimate() const;
  /// Return "down" estimate (default 1.0e-5)
  virtual double downEstimate() const;
  /** Reset variable bounds to their original values.
    Bounds may be tightened, so it may be good to be able to reset them to
    their original values.
   */
  virtual void resetBounds(const OsiSolverInterface * solver) {};
  /**  Change column numbers after preprocessing
   */
  virtual void resetSequenceEtc(int numberColumns, const int * originalColumns) {};
  

protected:
  /// data

  /// Computed infeasibility
  mutable double infeasibility_;
  /// Computed preferred way to branch 
  mutable int whichWay_;
  /// Priority
  int priority_;
  /// Maximum number of ways on branch
  int numberWays_;

};

/** \brief Abstract branching object base class

  In the abstract, an OsiBranchingObject contains instructions for how to
  branch. We want an abstract class so that we can describe how to branch on
  simple objects (<i>e.g.</i>, integers) and more exotic objects
  (<i>e.g.</i>, cliques or hyperplanes).

  The #branch() method is the crucial routine: it is expected to be able to
  step through a set of branch arms, executing the actions required to create
  each subproblem in turn. The base class is primarily virtual to allow for
  a wide range of problem modifications.

  See OsiObject for an overview of the two classes (OsiObject and
  OsiBranchingObject) which make up Osi's branching
  model.
*/

class OsiBranchingObject {

public:

  /// Default Constructor 
  OsiBranchingObject ();

  /// Constructor 
  OsiBranchingObject (OsiSolverInterface * solver, double value);
  
  /// Copy constructor 
  OsiBranchingObject ( const OsiBranchingObject &);
   
  /// Assignment operator 
  OsiBranchingObject & operator=( const OsiBranchingObject& rhs);

  /// Clone
  virtual OsiBranchingObject * clone() const=0;

  /// Destructor 
  virtual ~OsiBranchingObject ();

  /// The number of branch arms created for this branching object
  inline int numberBranches() const
  {return numberBranches_;};

  /// The number of branch arms left for this branching object
  inline int numberBranchesLeft() const
  {return numberBranches_-branchIndex_;};

  /** Set the number of branch arms left for this branching object
      Just for forcing
  */
  inline void setNumberBranchesLeft(int value)
  {assert (value==1&&!branchIndex_); numberBranches_=1;};

  /// Decrement the number of branch arms left for this branching object
  inline void decrementNumberBranchesLeft()
  {branchIndex_++;};

  /** \brief Execute the actions required to branch, as specified by the
	     current state of the branching object, and advance the object's
	     state. 
	     Returns change in guessed objective on next branch
  */
  virtual double branch(OsiSolverInterface * solver)=0;
  /** \brief Execute the actions required to branch, as specified by the
	     current state of the branching object, and advance the object's
	     state. 
	     Returns change in guessed objective on next branch
  */
  virtual double branch() {return branch(NULL);};
  /** \brief Return true if branch should fix variables
  */
  virtual bool boundBranch() const 
  {return true;};
  /** Get the state of the branching object
      This is just the branch index
  */
  inline int branchIndex() const
  {return branchIndex_;};

  /** Set the state of the branching object.
  */
  inline void setBranchingIndex(int branchIndex)
  {branchIndex_=branchIndex;};

  /// Current value
  inline double value() const
  {return value_;};
  
  /// Return pointer back to object which created
  inline const OsiObject * originalObject() const
  {return  originalObject_;};
  /// Set pointer back to object which created
  inline void setOriginalObject(const OsiObject * object)
  {originalObject_=object;};
  /// For debug
  int columnNumber() const;
  /** \brief Print something about branch - only if log level high
  */
  virtual void print(const OsiSolverInterface * solver=NULL) const {};

protected:

  /// Current value - has some meaning about branch
  double value_;

  /// Pointer back to object which created
  const OsiObject * originalObject_;

  /** Number of branches
  */
  int numberBranches_;

  /** The state of the branching object. i.e. branch index
      This starts at 0 when created
  */
  short branchIndex_;

};
/* This contains information
   This could also contain pseudo shadow prices
   or information for dealing with computing and trusting pseudo-costs
*/
class OsiBranchingInformation {

public:
  
  /// Default Constructor 
  OsiBranchingInformation ();
  
  /// Useful Constructor 
  OsiBranchingInformation (const OsiSolverInterface * solver);
  
  /// Copy constructor 
  OsiBranchingInformation ( const OsiBranchingInformation &);
  
  /// Assignment operator 
  OsiBranchingInformation & operator=( const OsiBranchingInformation& rhs);
  
  /// Clone
  virtual OsiBranchingInformation * clone() const;
  
  /// Destructor 
  virtual ~OsiBranchingInformation ();
  
  // Note public
public:
  /// data

  /// Value of objective function (in minimization sense)
  double objectiveValue_;
  /// Value of objective cutoff (in minimization sense)
  double cutoff_;
  /// Direction 1.0 for minimization, -1.0 for maximization
  double direction_;
  /// Integer tolerance
  double integerTolerance_;
  /// Primal tolerance
  double primalTolerance_;
  /// Maximum time remaining before stopping on time
  double timeRemaining_;
  /// Pointer to solver
  const OsiSolverInterface * solver_;
  /// Pointer to current lower bounds on columns
  const double * lower_;
  /// Pointer to current solution
  mutable const double * solution_;
  /// Pointer to current upper bounds on columns
  const double * upper_;
  /// Highly optional target (hot start) solution
  const double * hotstartSolution_;
  /// Number of solutions found
  int numberSolutions_;
  /// Number of branching solutions found (i.e. exclude heuristics)
  int numberBranchingSolutions_;
  /// Depth in tree
  int depth_;
};

/// This just adds two-wayness to a branching object

class OsiTwoWayBranchingObject : public OsiBranchingObject {

public:

  /// Default constructor 
  OsiTwoWayBranchingObject ();

  /** Create a standard tw0-way branch object

    Specifies a simple two-way branch.
    Specify way = -1 to set the object state to perform the down arm first,
    way = 1 for the up arm.
  */
  OsiTwoWayBranchingObject (OsiSolverInterface *solver,const OsiObject * originalObject,
			     int way , double value) ;
    
  /// Copy constructor 
  OsiTwoWayBranchingObject ( const OsiTwoWayBranchingObject &);
   
  /// Assignment operator 
  OsiTwoWayBranchingObject & operator= (const OsiTwoWayBranchingObject& rhs);

  /// Destructor 
  virtual ~OsiTwoWayBranchingObject ();
  
  /** \brief Sets the bounds for the variable according to the current arm
	     of the branch and advances the object state to the next arm.
	     state. 
	     Returns change in guessed objective on next branch
  */
  virtual double branch(OsiSolverInterface * solver)=0;

protected:
  /// Which way was first branch -1 = down, +1 = up
  int firstBranch_;
};
/// Define a single integer class


class OsiSimpleInteger : public OsiObject {

public:

  /// Default Constructor 
  OsiSimpleInteger ();

  /// Useful constructor - passed solver index
  OsiSimpleInteger (const OsiSolverInterface * solver, int iColumn);
  
  /// Useful constructor - passed solver index and original bounds
  OsiSimpleInteger (int iColumn, double lower, double upper);
  
  /// Copy constructor 
  OsiSimpleInteger ( const OsiSimpleInteger &);
   
  /// Clone
  virtual OsiObject * clone() const;

  /// Assignment operator 
  OsiSimpleInteger & operator=( const OsiSimpleInteger& rhs);

  /// Destructor 
  ~OsiSimpleInteger ();
  
  /// Infeasibility - large is 0.5
  virtual double infeasibility(const OsiBranchingInformation * info, int & whichWay) const;

  /** Set bounds to fix the variable at the current (integer) value.

    Given an integer value, set the lower and upper bounds to fix the
    variable. Returns amount it had to move variable.
  */
  virtual double feasibleRegion(OsiSolverInterface * solver, const OsiBranchingInformation * info) const;

  /** Creates a branching object

    The preferred direction is set by \p way, 0 for down, 1 for up.
  */
  virtual OsiBranchingObject * createBranch(OsiSolverInterface * solver, const OsiBranchingInformation * info, int way) const;


  /// Set solver column number
  inline void setColumnNumber(int value)
  {columnNumber_=value;};
  
  /** Column number if single column object -1 otherwise,
      so returns >= 0
      Used by heuristics
  */
  virtual int columnNumber() const;

  /// Original bounds
  inline double originalLowerBound() const
  { return originalLower_;};
  inline void setOriginalLowerBound(double value)
  { originalLower_=value;};
  inline double originalUpperBound() const
  { return originalUpper_;};
  inline void setOriginalUpperBound(double value)
  { originalUpper_=value;};
  /** Reset variable bounds to their original values.
    Bounds may be tightened, so it may be good to be able to reset them to
    their original values.
   */
  virtual void resetBounds(const OsiSolverInterface * solver) ;
  /**  Change column numbers after preprocessing
   */
  virtual void resetSequenceEtc(int numberColumns, const int * originalColumns);
  /// Return "up" estimate (default 1.0e-5)
  virtual double upEstimate() const;
  /// Return "down" estimate (default 1.0e-5)
  virtual double downEstimate() const;
  

protected:
  /// data

  /// Column number in solver
  int columnNumber_;
  /// Original lower bound
  double originalLower_;
  /// Original upper bound
  double originalUpper_;
  
};
/** Simple branching object for an integer variable

  This object can specify a two-way branch on an integer variable. For each
  arm of the branch, the upper and lower bounds on the variable can be
  independently specified. 0 -> down, 1-> up.
*/

class OsiIntegerBranchingObject : public OsiTwoWayBranchingObject {

public:

  /// Default constructor 
  OsiIntegerBranchingObject ();

  /** Create a standard floor/ceiling branch object

    Specifies a simple two-way branch. Let \p value = x*. One arm of the
    branch will be lb <= x <= floor(x*), the other ceil(x*) <= x <= ub.
    Specify way = -1 to set the object state to perform the down arm first,
    way = 1 for the up arm.
  */
  OsiIntegerBranchingObject (OsiSolverInterface *solver,const OsiSimpleInteger * originalObject,
			     int way , double value) ;
    
  /// Copy constructor 
  OsiIntegerBranchingObject ( const OsiIntegerBranchingObject &);
   
  /// Assignment operator 
  OsiIntegerBranchingObject & operator= (const OsiIntegerBranchingObject& rhs);

  /// Clone
  virtual OsiBranchingObject * clone() const;

  /// Destructor 
  virtual ~OsiIntegerBranchingObject ();
  
  /** \brief Sets the bounds for the variable according to the current arm
	     of the branch and advances the object state to the next arm.
	     state. 
	     Returns change in guessed objective on next branch
  */
  virtual double branch(OsiSolverInterface * solver);

  /** \brief Print something about branch - only if log level high
  */
  virtual void print(const OsiSolverInterface * solver=NULL);

protected:
  // Probably could get away with just value which is already stored 
  /// Lower [0] and upper [1] bounds for the down arm (way_ = -1)
  double down_[2];
  /// Lower [0] and upper [1] bounds for the up arm (way_ = 1)
  double up_[2];
};


/** Define Special Ordered Sets of type 1 and 2.  These do not have to be
    integer - so do not appear in lists of integers.
    
    which_ points columns of matrix
*/


class OsiSOS : public OsiObject {

public:

  // Default Constructor 
  OsiSOS ();

  /** Useful constructor - which are indices
      and  weights are also given.  If null then 0,1,2..
      type is SOS type
  */
  OsiSOS (const OsiSolverInterface * solver, int numberMembers,
	   const int * which, const double * weights, int type=1);
  
  // Copy constructor 
  OsiSOS ( const OsiSOS &);
   
  /// Clone
  virtual OsiObject * clone() const;

  // Assignment operator 
  OsiSOS & operator=( const OsiSOS& rhs);

  // Destructor 
  ~OsiSOS ();
  
  /// Infeasibility - large is 0.5
  virtual double infeasibility(const OsiBranchingInformation * info,int & whichWay) const;

  /** Set bounds to fix the variable at the current (integer) value.

    Given an integer value, set the lower and upper bounds to fix the
    variable. Returns amount it had to move variable.
  */
  virtual double feasibleRegion(OsiSolverInterface * solver, const OsiBranchingInformation * info) const;

  /** Creates a branching object

    The preferred direction is set by \p way, 0 for down, 1 for up.
  */
  virtual OsiBranchingObject * createBranch(OsiSolverInterface * solver, const OsiBranchingInformation * info, int way) const;
  /// Return "up" estimate (default 1.0e-5)
  virtual double upEstimate() const;
  /// Return "down" estimate (default 1.0e-5)
  virtual double downEstimate() const;
  
  /// Redoes data when sequence numbers change
  virtual void resetSequenceEtc(int numberColumns, const int * originalColumns);
  
  /// Number of members
  inline int numberMembers() const
  {return numberMembers_;};

  /// Members (indices in range 0 ... numberColumns-1)
  inline const int * members() const
  {return members_;};

  /// SOS type
  inline int sosType() const
  {return sosType_;};

  /** Array of weights */
  inline const double * weights() const
  { return weights_;};

  /** \brief Return true if object can take part in normal heuristics
  */
  virtual bool canDoHeuristics() const 
  {return (sosType_==1&&integerValued_);};
  /// Set whether set is integer valued or not
  inline void setIntegerValued(bool yesNo)
  { integerValued_=yesNo;};
private:
  /// data

  /// Members (indices in range 0 ... numberColumns-1)
  int * members_;
  /// Weights
  double * weights_;

  /// Number of members
  int numberMembers_;
  /// SOS type
   int sosType_;
  /// Whether integer valued
  bool integerValued_;
};

/** Branching object for Special ordered sets

 */
class OsiSOSBranchingObject : public OsiTwoWayBranchingObject {

public:

  // Default Constructor 
  OsiSOSBranchingObject ();

  // Useful constructor
  OsiSOSBranchingObject (OsiSolverInterface * solver,  const OsiSOS * originalObject,
			    int way,
			 double separator);
  
  // Copy constructor 
  OsiSOSBranchingObject ( const OsiSOSBranchingObject &);
   
  // Assignment operator 
  OsiSOSBranchingObject & operator=( const OsiSOSBranchingObject& rhs);

  /// Clone
  virtual OsiBranchingObject * clone() const;

  // Destructor 
  virtual ~OsiSOSBranchingObject ();
  
  /// Does next branch and updates state
  virtual double branch(OsiSolverInterface * solver);

  /** \brief Print something about branch - only if log level high
  */
  virtual void print(const OsiSolverInterface * solver=NULL);
private:
  /// data
};
#endif
