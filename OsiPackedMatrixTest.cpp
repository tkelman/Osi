// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <cassert>

#include "OsiFloatEqual.hpp"
#include "OsiPackedVector.hpp"
#include "OsiPackedMatrix.hpp"

//#############################################################################

#ifdef NDEBUG
#undef NDEBUG
#endif

void
OsiPackedMatrixUnitTest()
{
  
  OsiRelFltEq eq;
  
  {
    // Test construction on empty matrices
    OsiPackedMatrix m;      
    OsiPackedMatrix lhs = m;    
    OsiPackedMatrix mCopy(m);
    
    assert( eq(m.getExtraGap(),.25) );
    assert( eq(lhs.getExtraGap(),.25) );
    assert( eq(mCopy.getExtraGap(),.25) );
    
    assert( eq(m.getExtraMajor(),.25) );
    assert( eq(lhs.getExtraMajor(),.25) );
    assert( eq(mCopy.getExtraMajor(),.25) );
    
    assert(       m.isColOrdered() );
    assert(     lhs.isColOrdered() );
    assert(   mCopy.isColOrdered() );
    
    assert(       m.getNumElements() == 0 );
    assert(     lhs.getNumElements() == 0 );
    assert(   mCopy.getNumElements() == 0 );
    
    assert(       m.getNumCols() == 0 );
    assert(     lhs.getNumCols() == 0 );
    assert(   mCopy.getNumCols() == 0 );
    
    assert(       m.getNumRows() == 0 );
    assert(     lhs.getNumRows() == 0 );
    assert(   mCopy.getNumRows() == 0 );
    
    assert(       m.getElements() == 0 );
    assert(     lhs.getElements() == 0 );
    assert(   mCopy.getElements() == 0 );
    
    assert(       m.getIndices() == 0 );
    assert(     lhs.getIndices() == 0 );
    assert(   mCopy.getIndices() == 0 );
    
    assert( m.getSizeVectorStarts()==0 );
    assert( lhs.getSizeVectorStarts()==0 );
    assert( mCopy.getSizeVectorStarts()==0 );
    
    assert( m.getSizeVectorLengths()==0 );
    assert( lhs.getSizeVectorLengths()==0 );
    assert( mCopy.getSizeVectorLengths()==0 );
    
    assert( m.getVectorStarts()==NULL );
    assert( lhs.getVectorStarts()==NULL );
    assert( mCopy.getVectorStarts()==NULL );
    
    assert( m.getVectorLengths()==NULL );
    assert( lhs.getVectorLengths()==NULL );
    assert( mCopy.getVectorLengths()==NULL );
    
    assert(       m.getMajorDim() == 0 );
    assert(     lhs.getMajorDim() == 0 );
    assert(   mCopy.getMajorDim() == 0 );
    
    assert(       m.getMinorDim() == 0 );
    assert(     lhs.getMinorDim() == 0 );
    assert(   mCopy.getMinorDim() == 0 );
    
  }
  
  
  {
    OsiPackedMatrix * globalP;  
    
    { 
    /*************************************************************************
    *   Setup data to represent this matrix by rows
    *
    *    3x1 +  x2         -  2x4 - x5               -    x8
    *          2x2 + 1.1x3
    *                   x3              +  x6               
    *                       2.8x4             -1.2x7
    *  5.6x1                      + x5               + 1.9x8
    *
    *************************************************************************/
#if 0
      // By columns
      const int minor=5;
      const int major=8;
      const int numels=14;
      const double elemBase[numels]={3., 5.6, 1., 2., 1.1, 1., -2., 2.8, -1., 1., 1., -1.2, -1., 1.9};
      const int indBase[numels]={0,4,0,1,1,2,0,3,0,4,2,3,0,4};
      const int startsBase[major+1]={0,2,4,6,8,10,11,12,14};
      const int lenBase[major]={2,2,2,2,2,1,1,2};
#else
      // By rows
      const int minor=8;
      const int major=5;
      const int numels=14;
      const double elemBase[numels]={3., 1., -2., -1., -1., 2., 1.1, 1., 1., 2.8, -1.2, 5.6, 1., 1.9 };
      const int indBase[numels]={0,1,3,4,7,1,2,2,5,3,6,0,4,7};
      const int startsBase[major+1]={0,5,7,9,11,14};
      const int lenBase[major]={5,2,2,2,3};
#endif
      double * elem = new double[numels];
      int * ind = new int[numels];
      int * starts = new int[major+1];
      int * lens = new int[major];
      std::copy(elemBase,elemBase+numels,elem);
      std::copy(indBase,indBase+numels,ind);
      std::copy(startsBase,startsBase+major+1,starts);
      std::copy(lenBase,lenBase+major,lens);
      
      OsiPackedMatrix pm(false,minor,major,numels,elem,ind,starts,lens,
			 .25,.25);
      
      assert( elem!=NULL );
      assert( ind!=NULL );
      assert( starts!=NULL );
      assert( lens!=NULL );
      
      delete[] elem;
      delete[] ind;
      delete[] starts;
      delete[] lens;
      
      assert( eq(pm.getExtraGap(),.25) );
      assert( eq(pm.getExtraMajor(),.25) );
      assert( !pm.isColOrdered() );
      assert( pm.getNumElements()==numels );
      assert( pm.getNumCols()==minor );
      assert( pm.getNumRows()==major);
      assert( pm.getSizeVectorStarts()==major+1 );
      assert( pm.getSizeVectorLengths()==major );
      
      const double * ev = pm.getElements();
      assert( eq(ev[0],   3.0) );
      assert( eq(ev[1],   1.0) );
      assert( eq(ev[2],  -2.0) );
      assert( eq(ev[3],  -1.0) );
      assert( eq(ev[4],  -1.0) );
      assert( eq(ev[7],   2.0) );
      assert( eq(ev[8],   1.1) );
      assert( eq(ev[10],   1.0) );
      assert( eq(ev[11],   1.0) );
      assert( eq(ev[13],   2.8) );
      assert( eq(ev[14], -1.2) );
      assert( eq(ev[16],  5.6) );
      assert( eq(ev[17],  1.0) );
      assert( eq(ev[18],  1.9) );
      
      const int * mi = pm.getVectorStarts();
      assert( mi[0]==0 );
      assert( mi[1]==7 );
      assert( mi[2]==10 );
      assert( mi[3]==13 );
      assert( mi[4]==16 );
      assert( mi[5]==20 ); 
      
      const int * vl = pm.getVectorLengths();
      assert( vl[0]==5 );
      assert( vl[1]==2 );
      assert( vl[2]==2 );
      assert( vl[3]==2 );
      assert( vl[4]==3 );
      
      const int * ei = pm.getIndices();
      assert( ei[0]  ==  0 );
      assert( ei[1]  ==  1 );
      assert( ei[2]  ==  3 );
      assert( ei[3]  ==  4 );
      assert( ei[4]  ==  7 );
      assert( ei[7]  ==  1 );
      assert( ei[8]  ==  2 );
      assert( ei[10]  ==  2 );
      assert( ei[11]  ==  5 );
      assert( ei[13]  ==  3 );
      assert( ei[14] ==  6 );
      assert( ei[16] ==  0 );
      assert( ei[17] ==  4 );
      assert( ei[18] ==  7 );  
      
      assert( pm.getMajorDim() == 5 ); 
      assert( pm.getMinorDim() == 8 ); 
      assert( pm.getNumElements() == 14 ); 
      assert( pm.getSizeVectorStarts()==6 );
      
      {
        // Test copy constructor
        OsiPackedMatrix pmC(pm);
        
        assert( eq(pmC.getExtraGap(),.25) );
        assert( eq(pmC.getExtraMajor(),.25) );
        assert( !pmC.isColOrdered() );
        assert( pmC.getNumElements()==numels );
        assert( pmC.getNumCols()==minor );
        assert( pmC.getNumRows()==major);
        assert( pmC.getSizeVectorStarts()==major+1 );
        assert( pmC.getSizeVectorLengths()==major );
        
        // Test that osm has the correct values
        assert( pm.getElements() != pmC.getElements() );
        const double * ev = pmC.getElements();
        assert( eq(ev[0],   3.0) );
        assert( eq(ev[1],   1.0) );
        assert( eq(ev[2],  -2.0) );
        assert( eq(ev[3],  -1.0) );
        assert( eq(ev[4],  -1.0) );
        assert( eq(ev[7],   2.0) );
        assert( eq(ev[8],   1.1) );
        assert( eq(ev[10],   1.0) );
        assert( eq(ev[11],   1.0) );
        assert( eq(ev[13],   2.8) );
        assert( eq(ev[14], -1.2) );
        assert( eq(ev[16],  5.6) );
        assert( eq(ev[17],  1.0) );
        assert( eq(ev[18],  1.9) );
        
        assert( pm.getVectorStarts() != pmC.getVectorStarts() );
        const int * mi = pmC.getVectorStarts();
        assert( mi[0]==0 );
        assert( mi[1]==7 );
        assert( mi[2]==10 );
        assert( mi[3]==13 );
        assert( mi[4]==16 );
        assert( mi[5]==20 ); 
        
        assert( pm.getVectorLengths() != pmC.getVectorLengths() );     
        const int * vl = pmC.getVectorLengths();
        assert( vl[0]==5 );
        assert( vl[1]==2 );
        assert( vl[2]==2 );
        assert( vl[3]==2 );
        assert( vl[4]==3 );
        
        assert( pm.getIndices() != pmC.getIndices() );
        const int * ei = pmC.getIndices();
        assert( ei[0]  ==  0 );
        assert( ei[1]  ==  1 );
        assert( ei[2]  ==  3 );
        assert( ei[3]  ==  4 );
        assert( ei[4]  ==  7 );
        assert( ei[7]  ==  1 );
        assert( ei[8]  ==  2 );
        assert( ei[10]  ==  2 );
        assert( ei[11]  ==  5 );
        assert( ei[13]  ==  3 );
        assert( ei[14] ==  6 );
        assert( ei[16] ==  0 );
        assert( ei[17] ==  4 );
        assert( ei[18] ==  7 );  
        
        assert( pmC.isEquivalent(pm) );      
        
        // Test assignment
        {
          OsiPackedMatrix pmA;
          // Gap should be 0.25
          assert( eq(pmA.getExtraGap(),0.25) );
          assert( eq(pmA.getExtraMajor(),0.25) );
          
          pmA = pm;
          
          assert( eq(pmA.getExtraGap(),0.25) );
          assert( eq(pmA.getExtraMajor(),0.25) );
          assert( !pmA.isColOrdered() );
          assert( pmA.getNumElements()==numels );
          assert( pmA.getNumCols()==minor );
          assert( pmA.getNumRows()==major);
          assert( pmA.getSizeVectorStarts()==major+1 );
          assert( pmA.getSizeVectorLengths()==major );
          
          // Test that osm1 has the correct values
          assert( pm.getElements() != pmA.getElements() );
          const double * ev = pmA.getElements();
          assert( eq(ev[0],   3.0) );
          assert( eq(ev[1],   1.0) );
          assert( eq(ev[2],  -2.0) );
          assert( eq(ev[3],  -1.0) );
          assert( eq(ev[4],  -1.0) );
          assert( eq(ev[7],   2.0) );
          assert( eq(ev[8],   1.1) );
          assert( eq(ev[10],   1.0) );
          assert( eq(ev[11],   1.0) );
          assert( eq(ev[13],   2.8) );
          assert( eq(ev[14], -1.2) );
          assert( eq(ev[16],  5.6) );
          assert( eq(ev[17],  1.0) );
          assert( eq(ev[18],  1.9) );
          
          assert( pm.getVectorStarts() != pmA.getVectorStarts() );
          const int * mi = pmA.getVectorStarts();
          assert( mi[0]==0 );
          assert( mi[1]==7 );
          assert( mi[2]==10 );
          assert( mi[3]==13 );
          assert( mi[4]==16 );
          assert( mi[5]==20 ); 
          
          assert( pm.getVectorLengths() != pmA.getVectorLengths() );     
          const int * vl = pmC.getVectorLengths();
          assert( vl[0]==5 );
          assert( vl[1]==2 );
          assert( vl[2]==2 );
          assert( vl[3]==2 );
          assert( vl[4]==3 );
          
          assert( pm.getIndices() != pmA.getIndices() );
          const int * ei = pmA.getIndices();
          assert( ei[0]  ==  0 );
          assert( ei[1]  ==  1 );
          assert( ei[2]  ==  3 );
          assert( ei[3]  ==  4 );
          assert( ei[4]  ==  7 );
          assert( ei[7]  ==  1 );
          assert( ei[8]  ==  2 );
          assert( ei[10]  ==  2 );
          assert( ei[11]  ==  5 );
          assert( ei[13]  ==  3 );
          assert( ei[14] ==  6 );
          assert( ei[16] ==  0 );
          assert( ei[17] ==  4 );
          assert( ei[18] ==  7 );  
          
          assert( pmA.isEquivalent(pm) );
          assert( pmA.isEquivalent(pmC) );
          
          // Test new to global
          globalP = new OsiPackedMatrix(pmA);
          assert( eq(globalP->getElements()[0],   3.0) );
          assert( globalP->isEquivalent(pmA) );
        }
        assert( eq(globalP->getElements()[0],   3.0) );
      }
      assert( eq(globalP->getElements()[0],   3.0) );
    }
  
    // Test that cloned matrix contains correct values
    const double * ev = globalP->getElements();
    assert( eq(ev[0],   3.0) );
    assert( eq(ev[1],   1.0) );
    assert( eq(ev[2],  -2.0) );
    assert( eq(ev[3],  -1.0) );
    assert( eq(ev[4],  -1.0) );
    assert( eq(ev[7],   2.0) );
    assert( eq(ev[8],   1.1) );
    assert( eq(ev[10],   1.0) );
    assert( eq(ev[11],   1.0) );
    assert( eq(ev[13],   2.8) );
    assert( eq(ev[14], -1.2) );
    assert( eq(ev[16],  5.6) );
    assert( eq(ev[17],  1.0) );
    assert( eq(ev[18],  1.9) );
    
    const int * mi = globalP->getVectorStarts();
    assert( mi[0]==0 );
    assert( mi[1]==7 );
    assert( mi[2]==10 );
    assert( mi[3]==13 );
    assert( mi[4]==16 );
    assert( mi[5]==20 ); 
    
    const int * ei = globalP->getIndices();
    assert( ei[0]  ==  0 );
    assert( ei[1]  ==  1 );
    assert( ei[2]  ==  3 );
    assert( ei[3]  ==  4 );
    assert( ei[4]  ==  7 );
    assert( ei[7]  ==  1 );
    assert( ei[8]  ==  2 );
    assert( ei[10] ==  2 );
    assert( ei[11] ==  5 );
    assert( ei[13] ==  3 );
    assert( ei[14] ==  6 );
    assert( ei[16] ==  0 );
    assert( ei[17] ==  4 );
    assert( ei[18] ==  7 );  
    
    assert( globalP->getMajorDim() == 5 ); 
    assert( globalP->getMinorDim() == 8 ); 
    assert( globalP->getNumElements() == 14 ); 
    assert( globalP->getSizeVectorStarts()==6 );
    
    // Test method which returns length of vectors
    assert( globalP->getVectorSize(0)==5 );
    assert( globalP->getVectorSize(1)==2 );
    assert( globalP->getVectorSize(2)==2 );
    assert( globalP->getVectorSize(3)==2 );
    assert( globalP->getVectorSize(4)==3 );
    
    // Test getVectorSize exceptions
    {
      bool errorThrown = false;
      try {
        globalP->getVectorSize(-1);
      }
      catch (CoinError e) {
	errorThrown = true;
      }
      assert( errorThrown );
    }
    {
      bool errorThrown = false;
      try {
        globalP->getVectorSize(5);
      }
      catch (CoinError e) {
        errorThrown = true;
      }
      assert( errorThrown );
    }
    
    // Test vector method
    {
      // 3x1 +  x2         -  2x4 - x5               -    x8       
      OsiShallowPackedVector pv = globalP->getVector(0);
      assert( pv.getNumElements() == 5 );
      assert( eq(pv[0], 3.0) );
      assert( eq(pv[1], 1.0) );
      assert( eq(pv[3],-2.0) );
      assert( eq(pv[4],-1.0) );
      assert( eq(pv[7],-1.0) );
      
      //          2x2 + 1.1x3               
      pv = globalP->getVector(1);
      assert( pv.getNumElements() == 2 );
      assert( eq(pv[1], 2.0) );
      assert( eq(pv[2], 1.1) );
      
      //                   x3              +  x6             
      pv = globalP->getVector(2);
      assert( pv.getNumElements() == 2 );
      assert( eq(pv[2], 1.0) );
      assert( eq(pv[5], 1.0) ); 
      
      //                      2.8x4             -1.2x7             
      pv = globalP->getVector(3);
      assert( pv.getNumElements() == 2 );
      assert( eq(pv[3], 2.8) );
      assert( eq(pv[6],-1.2) );
      
      //  5.6x1                      + x5               + 1.9x8              
      pv = globalP->getVector(4);
      assert( pv.getNumElements() == 3 );
      assert( eq(pv[0], 5.6) );
      assert( eq(pv[4], 1.0) ); 
      assert( eq(pv[7], 1.9) ); 
    }
    
    // Test vector method exceptions
    {
      bool errorThrown = false;
      try {
        OsiShallowPackedVector v = globalP->getVector(-1);
      }
      catch (CoinError e) {
        errorThrown = true;
      }
      assert( errorThrown );
    }
    {
      bool errorThrown = false;
      try {
        OsiShallowPackedVector vs = globalP->getVector(5);
      }
      catch (CoinError e) {
        errorThrown = true;
      }
      assert( errorThrown );
    }

    {
      OsiPackedMatrix pm(*globalP);
      
      assert( pm.getExtraGap() != 0.0 );
      assert( pm.getExtraMajor() != 0.0 );
      
      pm.setExtraGap(0.0);
      pm.setExtraMajor(0.0);
      
      assert( pm.getExtraGap() == 0.0 );
      assert( pm.getExtraMajor() == 0.0 );
      
      pm.reverseOrdering();
      
      assert( pm.getExtraGap() == 0.0 );
      assert( pm.getExtraMajor() == 0.0 );
    }
    
    delete globalP;
  }
  
#if 0
  {
    // test append
    OsiPackedMatrix pm;
    
    const int ne = 4;
    int inx[ne] =   {  1,  -4,  0,   2 };
    double el[ne] = { 10., 40., 1., 50. };
    OsiPackedVector r(ne,inx,el);

    pm.appendRow(r);  // This line fails

  }
#endif 
  
}
