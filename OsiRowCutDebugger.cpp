// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <cfloat>
#include <string>
#include <iostream>

#include "CoinHelperFunctions.hpp"
#include "OsiPackedVector.hpp"
#include "OsiPackedMatrix.hpp"
#include "OsiRowCutDebugger.hpp"

/* If we are on the path to the optimal integer solution then
   check if any generated cuts cut off the optimal solution!
   
   If so then print offending cuts and return non-zero code
*/


int OsiRowCutDebugger::validateCuts(const OsiCuts & cs, 
				  int first, int last) const
{
  int nbad=0; 
  int i;
  const double epsilon=1.0e-8;
  const int nRowCuts = CoinMin(cs.sizeRowCuts(),last);
  
  for (i=first; i<nRowCuts; i++){
    
    OsiRowCut rcut = cs.rowCut(i);
    OsiPackedVector rpv = rcut.row();
    const int n = rpv.getNumElements();
    const int * indices = rpv.getIndices();
    const double * elements = rpv.getElements();
    int k;
    double lb=rcut.lb();
    double ub=rcut.ub();
    
    double sum=0.0;
    
    for (k=0; k<n; k++){
      int column=indices[k];
      sum += optimalSolution_[column]*elements[k];
    }
    // is it violated
    if (sum >ub + epsilon ||sum < lb - epsilon) {
      double violation=CoinMax(sum-ub,lb-sum);
      std::cout<<"Cut "<<i<<" with "<<n
	  <<" coefficients, cuts off optimal solutions by "<<violation
          <<", lo="<<lb<<", ub="<<ub<<std::endl;
      for (k=0; k<n; k++){
	int column=indices[k];
        std::cout<<"( "<<column<<" , "<<elements[k]<<" ) ";
      }
      std::cout <<std::endl;
      nbad++;
    }
  }
  return nbad;
}


/* If we are on the path to the optimal integer solution then
   check out if generated cut cuts off the optimal solution!
   
   If so then print offending cut and return non-zero code
*/

bool OsiRowCutDebugger::invalidCut(const OsiRowCut & rcut) const 
{
  bool bad=false; 
  const double epsilon=1.0e-8;
  
  OsiPackedVector rpv = rcut.row();
  const int n = rpv.getNumElements();
  const int * indices = rpv.getIndices();
  const double * elements = rpv.getElements();
  int k;
  
  double lb=rcut.lb();
  double ub=rcut.ub();
  double sum=0.0;
  
  for (k=0; k<n; k++){
    int column=indices[k];
    sum += optimalSolution_[column]*elements[k];
  }
  // is it violated
  if (sum >ub + epsilon ||sum < lb - epsilon) {
    double violation=CoinMax(sum-ub,lb-sum);
    std::cout<<"Cut with "<<n
	<<" coefficients, cuts off optimal solutions by "<<violation
        <<", lo="<<lb<<", ub="<<ub<<std::endl;
    for (k=0; k<n; k++){
      int column=indices[k];
      std::cout<<"( "<<column<<" , "<<elements[k]<<" ) ";
    }
    std::cout<<std::endl;
    bad=true;
  }
  return bad;
}

// Returns true if still on optimal path, false otherwise
bool OsiRowCutDebugger::onOptimalPath(const OsiSolverInterface & si) const
{
  if (integerVariable_) {
    int nCols=si.getNumCols(); 
    assert (nCols==numberColumns_); // check user has not modified problem
    int i;
    const double * collower = si.getColLower();
    const double * colupper = si.getColUpper();
    bool onOptimalPath=true;
    for (i=0;i<numberColumns_;i++) {
      if (integerVariable_[i]) {
	// value of integer variable in solution
	double value=optimalSolution_[i]; 
	if (value>colupper[i]+1.0e-3 || value<collower[i]-1.0e-3) {
	  onOptimalPath=false;
	  break;
	}
      }
    }
    return onOptimalPath;
  } else {
    // no information
    return false;
  }
}
// Returns true if debugger is active 
bool OsiRowCutDebugger::active() const
{
  return (integerVariable_!=NULL);
}

// Activate using name of model
// returns whether debug activated
bool OsiRowCutDebugger::activate( const OsiSolverInterface & si, 
				   const char * model)
{
  int i;
  //get rid of any arrays
  delete [] integerVariable_;
  delete [] optimalSolution_;
  numberColumns_ = 0;


  enum {undefined, pure0_1, continuousWith0_1, generalMip } probType;


  // Convert input parameter model to be lowercase and 
  // only consider charcters between '/' and '.'
  std::string modelL; //name in lowercase 
  int iput=0;
  for (i=0;i<(int) strlen(model);i++) {
    char value=tolower(model[i]);
    if (value=='/') {
      iput=0;
      modelL.erase();
    } else if (value=='.') {
      break;
    } else {
      modelL.append(1,value);
    }
  }


  OsiPackedVector intSoln;
  probType = undefined;

  //--------------------------------------------------------
  //
  // Define additional problems by adding it as an additional
  // "else if ( modelL == '???' ) { ... }" 
  // stanza below.
  //
  // Assign values to probType and intSoln.
  //
  // probType - pure0_1, continuousWith0_1, or generalMip
  // 
  // intSoln -
  //    when probType is pure0_1
  //       intSoln contains the indices of the variables
  //       at 1 in the optimal solution
  //    when probType is continuousWith0_1
  //       intSoln contains the indices of integer
  //       variables at one in the optimal solution
  //    when probType is generalMip
  //       intSoln contains the the indices of the integer
  //       variables and their value in the optimal solution
  //--------------------------------------------------------

  // exmip1
  if ( modelL == "exmip1" ) {
    probType=continuousWith0_1;
    intSoln.insert(2,1.);
    intSoln.insert(3,1.);
  }

  // p0033
  else if ( modelL == "p0033" ) {
    probType=pure0_1;
    int intIndicesAt1[]={ 0,6,7,9,13,17,18,22,24,25,26,27,28,29 };
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // flugpl
  else if ( modelL == "flugpl" ) {
    probType=generalMip;
    int intIndicesV[] = { 1 , 3 , 4 , 6 , 7 , 9 ,10 ,12 ,13 ,15 };
    double intSolnV[] = { 6.,60., 6.,60.,16.,70., 7.,70.,12.,75.};
    int vecLen = sizeof(intIndicesV)/sizeof(int);
    intSoln.setVector(vecLen,intIndicesV,intSolnV);
  }

  // enigma
  else if ( modelL == "enigma" ) {
    probType=pure0_1;
    int intIndicesAt1[]={ 0,18,25,36,44,59,61,77,82,93 };
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // mod011
  else if ( modelL == "mod011" ) {
    probType=continuousWith0_1;
    int intIndicesAt1[]={ 10,29,32,40,58,77,80,88 };
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // ltw3
  else if ( modelL == "ltw3" ) {
    probType=continuousWith0_1;
    int intIndicesAt1[]={ 20,23,24,26,32,33,40,47 };
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // mod008
  else if ( modelL == "mod008" ) {
    probType=pure0_1;
    int intIndicesAt1[]={1,59,83,116,123};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // mod010
  else if ( modelL == "mod010" ) {
    probType=pure0_1;
    int intIndicesAt1[]={2,9,16,22,26,50,65,68,82,86,102,145,
	149,158,181,191,266,296,376,479,555,625,725,851,981,
	1030,1095,1260,1321,1339,1443,1459,1568,1602,1780,1856,
	1951,2332,2352,2380,2471,2555,2577,2610,2646,2647};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // modglob
  else if ( modelL == "modglob" ) {
    probType=continuousWith0_1;
    int intIndicesAt1[]={204,206,208,212,216,218,220,222,230,232,
	234,236,244,248,250,254,256,258,260,262,264,266,268,274,
	278,282,284,286,288};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // p0201
  else if ( modelL == "p0201" ) {
    probType=pure0_1;
    int intIndicesAt1[]={8,10,21,38,39,56,60,74,79,92,94,110,111,
	128,132,146,151,164,166,182,183,200};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // p0282
  else if ( modelL == "p0282" ) {
    probType=pure0_1;
    int intIndicesAt1[]={3,11,91,101,103,117,155,169,191,199,215,
	223,225,237,240,242,243,244,246,248,251,254,256,257,260,
	262,263,273,275,276,277,280,281};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // p0548
  else if ( modelL == "p0548" ) {
    probType=pure0_1;
    int intIndicesAt1[]={2,3,13,14,17,23,24,43,44,47,61,62,74,75,
	81,82,92,93,96,98,105,120,126,129,140,141,153,154,161,162,
	165,177,182,184,189,192,193,194,199,200,209,214,215,218,222,
	226,234,239,247,256,257,260,274,286,301,305,306,314,317,318,
	327,330,332,334,336,340,347,349,354,358,368,369,379,380,385,
	388,389,390,393,394,397,401,402,406,407,417,419,420,423,427,
	428,430,437,439,444,446,447,450,451,452,472,476,477,480,488,
	491,494,500,503,508,509,510,511,512,515,517,518,519,521,522,
	523,525,526,527,528,529,530,531,532,533,536,537,538,539,541,
	542,545,547};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // p2756
  else if ( modelL == "p2756" ) {
    probType=pure0_1;
    int intIndicesAt1[]={7,25,50,63,69,71,81,124,164,208,210,212,214,
	220,266,268,285,299,301,322,362,399,455,464,468,475,518,574,
	588,590,612,632,652,679,751,767,794,819,838,844,892,894,913,
	919,954,966,996,998,1021,1027,1044,1188,1230,1248,1315,1348,
	1366,1367,1420,1436,1473,1507,1509,1521,1555,1558,1607,1659,
	1715,1746,1761,1789,1800,1844,1885,1913,1916,1931,1992,2002,
	2050,2091,2155,2158,2159,2197,2198,2238,2264,2292,2318,2481,
	2496,2497,2522,2531,2573,2583,2587,2588,2596,2635,2637,2639,
	2643,2645,2651,2653,2672,2675,2680,2683,2708,2727,2730,2751};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // bell3a
  else if ( modelL == "bell3a" ) {
    probType=generalMip;
    int intIndicesV[]={61,62,65,66,67,68,69,70};
    double intSolnV[] = {4.,21.,4.,4.,6.,1.,25.,8.};
    int vecLen = sizeof(intIndicesV)/sizeof(int);
    intSoln.setVector(vecLen,intIndicesV,intSolnV);
  }

  // 10teams
  else if ( modelL == "10teams" ) {
    probType=continuousWith0_1;
    int intIndicesAt1[]={236,298,339,379,443,462,520,576,616,646,690,
	749,778,850,878,918,986,996,1065,1102,1164,1177,1232,1281,1338,
	1358,1421,1474,1522,1533,1607,1621,1708,1714,1775,1835,1887,
	1892,1945,1989};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // danoint
  else if ( modelL == "danoint" ) {
    probType=continuousWith0_1;
    int intIndicesAt1[]={3,5,8,11,15,21,24,25,31,34,37,42,46,48,51,56};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // dcmulti
  else if ( modelL == "dcmulti" ) {
    probType=continuousWith0_1;
    int intIndicesAt1[]={2,3,11,14,15,16,21,24,28,34,35,36,39,40,41,42,
	45,52,53,60,61,64,65,66,67};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // egout
  else if ( modelL == "egout" ) {
    probType=continuousWith0_1;
    int intIndicesAt1[]={0,3,5,7,8,9,11,12,13,15,16,17,18,20,21,22,
	23,24,25,26,27,28,29,32,34,36,37,38,39,40,42,43,44,45,46,47,
	48,49,52,53,54};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // fixnet6
  else if ( modelL == "fixnet6" ) {
    probType=continuousWith0_1;
    int intIndicesAt1[]={1,16,23,31,37,51,64,179,200,220,243,287,
	375,413,423,533,537,574,688,690,693,712,753,773,778,783,847};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // khb05250
  else if ( modelL == "khb05250" ) {
    probType=continuousWith0_1;
    int intIndicesAt1[]={1,3,8,11,12,15,16,17,18,21,22,23};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // lseu
  else if ( modelL == "lseu" ) {
    probType=pure0_1;
    int intIndicesAt1[]={0,1,6,13,26,33,38,43,50,52,63,65,85};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // misc03
  else if ( modelL == "misc03" ) {
    probType=continuousWith0_1;
    int intIndicesAt1[]={4,40,62,75,99,114,127,134,147,148,150,
	152,154,155,157};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // misc07
  else if ( modelL == "misc07" ) {
    probType=continuousWith0_1;
    int intIndicesAt1[]={21,27,57,103,118,148,185,195,205,209,243,
	245,247,249,251,253,255,257};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // gen
  else if ( modelL == "gen" ) {
    probType=generalMip;
    int intIndicesV[]={15,34,35,36,37,38,39,40,41,42,43,44,45,57,58,
	59,60,61,62,63,64,65,66,67,68,69,84,85,86,87,88,89,90,91,92,
	93,107,108,109,110,111,112,113,114,120,121,122,123,124,125,
	126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,
		       141,142,143,432,433,434,435,436};
    double intSolnV[] = {1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,
	1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,
	1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,
	1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,23.,12.,11.,14.,16.};
    int vecLen = sizeof(intIndicesV)/sizeof(int);
    intSoln.setVector(vecLen,intIndicesV,intSolnV);
  }

  // gt2
  else if ( modelL == "gt2" ) {
    probType=generalMip;
    int intIndicesV[]={82,85,88,92,94,95,102,103,117,121,122,128,
		       141,146,151,152,165,166,176,179};
    double intSolnV[] = {1.,3.,1.,5.,2.,1.,1.,2.,2.,2.,1.,2.,1.,1.,
	2.,1.,1.,6.,1.,1.};
    int vecLen = sizeof(intIndicesV)/sizeof(int);
    intSoln.setVector(vecLen,intIndicesV,intSolnV);
  }

  // fiber
  else if ( modelL == "fiber" ) {
    probType=continuousWith0_1;
    int intIndicesAt1[]={36,111,190,214,235,270,338,346,372,386,
	421,424,441,470,473,483,484,498,580,594,597,660,689,735,
	742,761,762,776,779,817,860,1044,1067,1122,1238};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // l152lav
  else if ( modelL == "l152lav" ) {
    probType=pure0_1;
    int intIndicesAt1[]={1,16,30,33,67,111,165,192,198,321,411,449,
	906,961,981,1052,1075,1107,1176,1231,1309,1415,1727,1847,
	1902,1917,1948,1950};
    int numIndices = sizeof(intIndicesAt1)/sizeof(int);
    intSoln.setConstant(numIndices,intIndicesAt1,1.0);
  }

  // bell5
  else if ( modelL == "bell5" ) {
    probType=generalMip;
    int intIndicesV[]={0,1,2,3,4,6,12,13,15,33,34,36,47,48,49,50,
		       51,52,53,54,56};
    double intSolnV[] = {1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,11.,
	2.,38.,2.,498.,125.,10.,17.,41.,19.};
    int vecLen = sizeof(intIndicesV)/sizeof(int);
    intSoln.setVector(vecLen,intIndicesV,intSolnV);
  }

  // blend2
  else if ( modelL == "blend2" ) {
    probType=generalMip;
    int intIndicesV[]={24,35,44,45,46,52,63,64,70,71,76,84,85,
	132,134,151,152,159,164,172,173,289,300,309,310,311,
		       317,328,329,335,336,341,349,350};
    double intSolnV[] = {2.,1.,1.,1.,1.,1.,1.,1.,2.,1.,1.,1.,
	2.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,1.,
	1.,1.,1.,1.,1.,1.,1.,1.,1.};
    int vecLen = sizeof(intIndicesV)/sizeof(int);
    intSoln.setVector(vecLen,intIndicesV,intSolnV);
  }


  // check to see if the model parameter is 
  // a known problem.
  if ( probType != undefined ) {

    // Specified model is a known problem
    
    numberColumns_ = si.getNumCols();

    integerVariable_= new bool[numberColumns_];
    optimalSolution_=new double[numberColumns_];
    //CoinFillN(integerVariable_, numberColumns_,0);
    //CoinFillN(optimalSolution_,numberColumns_,0.0);

    if ( probType == pure0_1 ) {
      
      // mark all variables as integer
      CoinFillN(integerVariable_,numberColumns_,true);
      // set solution to 0.0 for all not mentioned
      CoinFillN(optimalSolution_,numberColumns_,0.0);

      // mark column solution that have value 1
      for ( i=0; i<intSoln.getNumElements(); i++ ) {
        int col = intSoln.getIndices()[i];
        optimalSolution_[col] = intSoln.getElements()[i];
        assert( optimalSolution_[col]==1. );
      }

    }
    else {
      // Probtype is continuousWith0_1 or generalMip 
      assert( probType==continuousWith0_1 || probType==generalMip );

      OsiSolverInterface * siCopy = si.clone();
      assert(siCopy->getMatrixByCol()->isEquivalent(*si.getMatrixByCol()));

      // Loop once for each column looking for integer variables
      for (i=0;i<numberColumns_;i++) {

        // Is the this an integer variable?
        if(siCopy->isInteger(i)) {

          // integer variable found
          integerVariable_[i]=true;

    
          // Determine optimal solution value for integer i
          // from values saved in intSoln and probType.
          double soln;
          if ( probType==continuousWith0_1 ) {

            // Since 0_1 problem, had better be binary
            assert( siCopy->isBinary(i) );

            // intSoln only contains integers with optimal value of 1
            if ( intSoln.isExistingIndex(i) ) {
              soln = 1.0;
              assert( intSoln[i]==1. );
            }
            else {
              soln = 0.0;
            }

          } else {
            // intSoln only contains integers with nonzero optimal values
            if ( intSoln.isExistingIndex(i) ) {
              soln = intSoln[i];
              assert( intSoln[i]>=1. );
            }
            else {
              soln = 0.0;
            }
          }
          
          // Set bounds in copyied problem to fix variable to its solution     
          siCopy->setColUpper(i,soln);
          siCopy->setColLower(i,soln);
          
        }
        else {
          // this is not an integer variable
          integerVariable_[i]=false;
        }
      }
 
      // All integers have been fixed at optimal value.
      // Now solve to get continuous values
#if 0
      assert( siCopy->getNumRows()==5);        
      assert( siCopy->getNumCols()==8); 
      int r,c;
      for ( r=0; r<siCopy->getNumRows(); r++ ) {
        std::cerr <<"rhs[" <<r <<"]=" <<(si.rhs())[r] <<" " <<(siCopy->rhs())[r] <<std::endl;
      }
      for ( c=0; c<siCopy->getNumCols(); c++ ) {
        std::cerr <<"collower[" <<c <<"]=" <<(si.collower())[c] <<" " <<(siCopy->collower())[c] <<std::endl;
        std::cerr <<"colupper[" <<c <<"]=" <<(si.colupper())[c] <<" " <<(siCopy->colupper())[c] <<std::endl;
      }
#endif
      siCopy->initialSolve();
#if 0
      for ( c=0; c<siCopy->getNumCols(); c++ ) {
        std::cerr <<"colsol[" <<c <<"]=" <<optimalSolution_[c] <<" " <<(siCopy->colsol())[c] <<std::endl;
      }
      OsiRelFltEq eq;
      assert( eq(siCopy->getObjValue(),3.2368421052632));
#endif
      
      // Save column solution
      CoinCopyN(siCopy->getColSolution(),numberColumns_,optimalSolution_);

      delete siCopy;
    }
  }
 
  //if (integerVariable_!=NULL) si.rowCutDebugger_=this;

  return (integerVariable_!=NULL);
}


//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
OsiRowCutDebugger::OsiRowCutDebugger ()
: numberColumns_(0),
  integerVariable_(NULL),
  optimalSolution_(NULL)
{
  // nothing to do here
}

//-------------------------------------------------------------------
// Alternate Constructor with model name
//-------------------------------------------------------------------
// Constructor with name of model
OsiRowCutDebugger::OsiRowCutDebugger ( 
        const OsiSolverInterface & si, 
        const char * model)
: numberColumns_(0),
  integerVariable_(NULL),
  optimalSolution_(NULL)
{
  activate(si,model);
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
OsiRowCutDebugger::OsiRowCutDebugger (const OsiRowCutDebugger & source)
{  
  // copy 
  numberColumns_=source.numberColumns_;
  integerVariable_=new bool[numberColumns_];
  optimalSolution_=new double[numberColumns_];
  CoinCopyN(source.integerVariable_,  numberColumns_, integerVariable_ );
  CoinCopyN(source.optimalSolution_, numberColumns_, optimalSolution_);
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
OsiRowCutDebugger::~OsiRowCutDebugger ()
{
  // free memory
  delete [] integerVariable_;
  delete [] optimalSolution_;
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
OsiRowCutDebugger &
OsiRowCutDebugger::operator=(const OsiRowCutDebugger& rhs)
{
  if (this != &rhs) {
    delete [] integerVariable_;
    delete [] optimalSolution_;
    // copy 
    numberColumns_=rhs.numberColumns_;
    integerVariable_=new bool[numberColumns_];
    optimalSolution_=new double[numberColumns_];
    CoinCopyN(rhs.integerVariable_,  numberColumns_, integerVariable_ );
    CoinCopyN(rhs.optimalSolution_, numberColumns_, optimalSolution_);
  }
  return *this;
}
// Return optimal solution
const double * 
OsiRowCutDebugger::optimalSolution() const
{
  return optimalSolution_;
}
