// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <cassert>
#include <cmath>
#include <cfloat>

#include "OsiMpsReader.hpp"
#include <math.h>
#include <string>
#include <stdio.h>
#include <iostream>

// define COIN_USE_ZLIB if you want to be able to read compressed files!
// Look at http://www.gzip.org/zlib/
// zlib is due to Jean-loup Gailly and Mark Adler
// It is not under CPL but zlib.h has the following
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
#ifdef COIN_USE_ZLIB
#include "zlib.h"
#endif

/// The following lengths are in decreasing order (for 64 bit etc)
/// Large enough to contain element index
typedef int OSIElementIndex;

/// Large enough to contain column index
typedef int OSIColumnIndex;

/// Large enough to contain row index (or basis)
typedef int OSIRowIndex;

// We are allowing free format - but there is a limit!
#define MAX_FIELD_LENGTH 100
#define MAX_CARD_LENGTH 5*MAX_FIELD_LENGTH+80

enum OSISectionType { OSI_NO_SECTION, OSI_NAME_SECTION, OSI_ROW_SECTION,
  OSI_COLUMN_SECTION,
  OSI_RHS_SECTION, OSI_RANGE_SECTION, OSI_BOUND_SECTION,
  OSI_ENDATA_SECTION, OSI_EOF_SECTION, OSI_UNKNOWN_SECTION
};

enum OSIMpsType { OSI_N_ROW, OSI_E_ROW, OSI_L_ROW, OSI_G_ROW,
  OSI_BLANK_COLUMN, OSI_S1_COLUMN, OSI_S2_COLUMN, OSI_S3_COLUMN,
  OSI_INTORG, OSI_INTEND, OSI_SOSEND, OSI_UNSET_BOUND,
  OSI_UP_BOUND, OSI_FX_BOUND, OSI_LO_BOUND, OSI_FR_BOUND,
  OSI_MI_BOUND, OSI_PL_BOUND, OSI_BV_BOUND, OSI_UI_BOUND,
  OSI_SC_BOUND, OSI_UNKNOWN_MPS_TYPE
};
static double osi_strtod(char * ptr, char ** output) 
{

  static const double fraction[]=
  {1.0,1.0e-1,1.0e-2,1.0e-3,1.0e-4,1.0e-5,1.0e-6,1.0e-7,1.0e-8,
   1.0e-9,1.0e-10,1.0e-11,1.0e-12,1.0e-13,1.0e-14,1.0e-15,1.0e-16,
   1.0e-17,1.0e-18,1.0e-19};

  static const double exponent[]=
  {1.0e-9,1.0e-8,1.0e-7,1.0e-6,1.0e-5,1.0e-4,1.0e-3,1.0e-2,1.0e-1,
   1.0,1.0e1,1.0e2,1.0e3,1.0e4,1.0e5,1.0e6,1.0e7,1.0e8,1.0e9};

  double value = 0.0;
  char * save = ptr;

  // take off leading white space
  while (*ptr==' '||*ptr=='\t')
    ptr++;
  double sign1=1.0;
  // do + or -
  if (*ptr=='-') {
    sign1=-1.0;
    ptr++;
  } else if (*ptr=='+') {
    ptr++;
  }
  // more white space
  while (*ptr==' '||*ptr=='\t')
    ptr++;
  char thisChar=0;
  while (value<1.0e30) {
    thisChar = *ptr;
    ptr++;
    if (thisChar>='0'&&thisChar<='9') 
      value = value*10.0+thisChar-'0';
    else
      break;
  }
  if (value<1.0e30) {
    if (thisChar=='.') {
      // do fraction
      double value2 = 0.0;
      int nfrac=0;
      while (nfrac<20) {
	thisChar = *ptr;
	ptr++;
	if (thisChar>='0'&&thisChar<='9') {
	  value2 = value2*10.0+thisChar-'0';
	  nfrac++;
	} else {
	  break;
	}
      }
      if (nfrac<20) {
	value += value2*fraction[nfrac];
      } else {
	thisChar='x'; // force error
      }
    }
    if (thisChar=='e'||thisChar=='E') {
      // exponent
      int sign2=1;
      // do + or -
      if (*ptr=='-') {
	sign2=-1;
	ptr++;
      } else if (*ptr=='+') {
	ptr++;
      }
      int value3 = 0;
      while (value3<100) {
	thisChar = *ptr;
	ptr++;
	if (thisChar>='0'&&thisChar<='9') {
	  value3 = value3*10+thisChar-'0';
	} else {
	  break;
	}
      }
      if (value3<200) {
	value3 *= sign2; // power of 10
	if (abs(value3)<10) {
	  // do most common by lookup (for accuracy?)
	  value *= exponent[value3+9];
	} else {
	  value *= pow(10.0,value3);
	}
      } else {
	thisChar='x'; // force error
      }
    } 
    if (thisChar==0||thisChar=='\t'||thisChar==' ') {
      // okay
      *output=ptr;
    } else {
      *output=save;
    }
  } else {
    // bad value
    *output=save;
  }
  return value*sign1;
} 
/// Very simple code for reading MPS data
class OSIMpsio {

public:

  /**@name Constructor and destructor */
  //@{
  /// Constructor expects file to be open - reads down to (and reads) NAME card
  OSIMpsio ( FILE * fp );
#ifdef COIN_USE_ZLIB
  /// This one takes gzFile if fp null
  OSIMpsio ( FILE * fp, gzFile fp );
#endif
  /// Destructor
  ~OSIMpsio (  );
  //@}


  /**@name card stuff */
  //@{
  /// Gets next field and returns section type e.g. OSI_COLUMN_SECTION
  OSISectionType nextField (  );
  /// Returns current section type
  inline OSISectionType whichSection (  ) const {
    return section_;
  };
  /// Only for first field on card otherwise BLANK_COLUMN
  /// e.g. OSI_E_ROW
  inline OSIMpsType mpsType (  ) const {
    return mpsType_;
  };
  /// Reads and cleans card - taking out trailing blanks - return 1 if EOF
  int cleanCard();
  /// Returns row name of current field
  inline const char *rowName (  ) const {
    return rowName_;
  };
  /// Returns column name of current field
  inline const char *columnName (  ) const {
    return columnName_;
  };
  /// Returns value in current field
  inline double value (  ) const {
    return value_;
  };
  /// Whole card (for printing)
  inline const char *card (  ) const {
    return card_;
  };
  /// Returns card number
  inline OSIElementIndex cardNumber (  ) const {
    return cardNumber_;
  };
  //@}

////////////////// data //////////////////
private:

  /**@name data */
  //@{
  /// Current value
  double value_;
  /// Current card image
  char card_[MAX_CARD_LENGTH];
  /// Current position within card image
  char *position_;
  /// End of card
  char *eol_;
  /// Current OSIMpsType
  OSIMpsType mpsType_;
  /// Current row name
  char rowName_[MAX_FIELD_LENGTH];
  /// Current column name
  char columnName_[MAX_FIELD_LENGTH];
  /// File pointer
  FILE *fp_;
#ifdef COIN_USE_ZLIB
  /// Compressed file object
  gzFile gzfp_;
#endif
  /// Which section we think we are in
  OSISectionType section_;
  /// Card number
  OSIElementIndex cardNumber_;
  /// Whether free format.  Just for blank RHS etc
  bool freeFormat_;
  /// If all names <= 8 characters then allow embedded blanks
  bool eightChar_;
  //@}
};

//#############################################################################
// sections
const static char *section[] = {
  "", "NAME", "ROW", "COLUMN", "RHS", "RANGE", "BOUND", "ENDATA", " "
};

// what is allowed in each section - must line up with OSISectionType
const static OSIMpsType startType[] = {
  OSI_UNKNOWN_MPS_TYPE, OSI_UNKNOWN_MPS_TYPE,
  OSI_N_ROW, OSI_BLANK_COLUMN,
  OSI_BLANK_COLUMN, OSI_BLANK_COLUMN,
  OSI_UP_BOUND, OSI_UNKNOWN_MPS_TYPE,
  OSI_UNKNOWN_MPS_TYPE, OSI_UNKNOWN_MPS_TYPE
};
const static OSIMpsType endType[] = {
  OSI_UNKNOWN_MPS_TYPE, OSI_UNKNOWN_MPS_TYPE,
  OSI_BLANK_COLUMN, OSI_UNSET_BOUND,
  OSI_S1_COLUMN, OSI_S1_COLUMN,
  OSI_UNKNOWN_MPS_TYPE, OSI_UNKNOWN_MPS_TYPE,
  OSI_UNKNOWN_MPS_TYPE, OSI_UNKNOWN_MPS_TYPE
};
const static int allowedLength[] = {
  0, 0,
  1, 2,
  0, 0,
  2, 0,
  0, 0
};

// names of types
const static char *mpsTypes[] = {
  "N", "E", "L", "G",
  "  ", "S1", "S2", "S3", "  ", "  ", "  ",
  "  ", "UP", "FX", "LO", "FR", "MI", "PL", "BV", "UI", "SC"
};

int OSIMpsio::cleanCard()
{
  char * getit;
#ifdef COIN_USE_ZLIB
  if (fp_) {
#endif
    // normal file
    getit = fgets ( card_, MAX_CARD_LENGTH, fp_ );
#ifdef COIN_USE_ZLIB
  } else {
    getit = gzgets ( gzfp_, card_, MAX_CARD_LENGTH );
  }
#endif
  
  if ( getit ) {
    cardNumber_++;
    unsigned char * lastNonBlank = (unsigned char *) card_-1;
    unsigned char * image = (unsigned char *) card_;
    while ( *image != '\0' ) {
      if ( *image != '\t' && *image < ' ' ) {
	break;
      } else if ( *image != '\t' && *image != ' ') {
	lastNonBlank = image;
      }
      image++;
    }
    *(lastNonBlank+1)='\0';
    return 0;
  } else {
    return 1;
  }
}

char *
nextBlankOr ( char *image )
{
  char * saveImage=image;
  while ( 1 ) {
    if ( *image == ' ' || *image == '\t' ) {
      break;
    }
    if ( *image == '\0' )
      return NULL;
    image++;
  }
  // Allow for floating - or +.  Will fail if user has that as row name!!
  if (image-saveImage==1&&(*saveImage=='+'||*saveImage=='-')) {
    while ( *image == ' ' || *image == '\t' ) {
      image++;
    }
    image=nextBlankOr(image);
  }
  return image;
}

//  OSIMpsio.  Constructor
OSIMpsio::OSIMpsio (  FILE * fp )
{
  memset ( card_, 0, MAX_CARD_LENGTH );
  position_ = card_;
  eol_ = card_;
  mpsType_ = OSI_UNKNOWN_MPS_TYPE;
  memset ( rowName_, 0, MAX_FIELD_LENGTH );
  memset ( columnName_, 0, MAX_FIELD_LENGTH );
  value_ = 0.0;
  fp_ = fp;
  section_ = OSI_EOF_SECTION;
  cardNumber_ = 0;
  freeFormat_ = false;
  eightChar_ = true;
  bool found = false;

  while ( !found ) {
    // need new image

    if ( cleanCard() ) {
      section_ = OSI_EOF_SECTION;
      break;
    }
    if ( !strncmp ( card_, "NAME", 4 ) ) {
      section_ = OSI_NAME_SECTION;
      char *next = card_ + 4;

      {
	std::cout<<"At line "<< cardNumber_ <<" "<< card_<<std::endl;
      }
      while ( next != eol_ ) {
	if ( *next == ' ' || *next == '\t' ) {
	  next++;
	} else {
	  break;
	}
      }
      if ( next != eol_ ) {
	char *nextBlank = nextBlankOr ( next );
	char save;

	if ( nextBlank ) {
	  save = *nextBlank;
	  *nextBlank = '\0';
	  strcpy ( columnName_, next );
	  *nextBlank = save;
	  if ( strstr ( nextBlank, "FREE" ) )
	    freeFormat_ = true;
	} else {
	  strcpy ( columnName_, next );
	}
      } else {
	strcpy ( columnName_, "no_name" );
      }
      break;
    } else if ( card_[0] != '*' && card_[0] != '#' ) {
      section_ = OSI_UNKNOWN_SECTION;
      break;
    }
  }
}
#ifdef COIN_USE_ZLIB
// This one takes gzFile if fp null
OSIMpsio::OSIMpsio (  FILE * fp , gzFile gzFile)
{
  memset ( card_, 0, MAX_CARD_LENGTH );
  position_ = card_;
  eol_ = card_;
  mpsType_ = OSI_UNKNOWN_MPS_TYPE;
  memset ( rowName_, 0, MAX_FIELD_LENGTH );
  memset ( columnName_, 0, MAX_FIELD_LENGTH );
  value_ = 0.0;
  fp_ = fp;
  gzfp_ = gzFile;
  section_ = OSI_EOF_SECTION;
  cardNumber_ = 0;
  freeFormat_ = false;
  eightChar_ = true;
  bool found = false;

  while ( !found ) {
    // need new image

    if ( cleanCard() ) {
      section_ = OSI_EOF_SECTION;
      break;
    }
    if ( !strncmp ( card_, "NAME", 4 ) ) {
      section_ = OSI_NAME_SECTION;
      char *next = card_ + 4;

      {
	std::cout<<"At line "<< cardNumber_ <<" "<< card_<<std::endl;
      }
      while ( next != eol_ ) {
	if ( *next == ' ' || *next == '\t' ) {
	  next++;
	} else {
	  break;
	}
      }
      if ( next != eol_ ) {
	char *nextBlank = nextBlankOr ( next );
	char save;

	if ( nextBlank ) {
	  save = *nextBlank;
	  *nextBlank = '\0';
	  strcpy ( columnName_, next );
	  *nextBlank = save;
	  if ( strstr ( nextBlank, "FREE" ) )
	    freeFormat_ = true;
	} else {
	  strcpy ( columnName_, next );
	}
      } else {
	strcpy ( columnName_, "no_name" );
      }
      break;
    } else if ( card_[0] != '*' && card_[0] != '#' ) {
      section_ = OSI_UNKNOWN_SECTION;
      break;
    }
  }
}
#endif
//  ~OSIMpsio.  Destructor
OSIMpsio::~OSIMpsio (  )
{
#ifdef COIN_USE_ZLIB
  if (!fp_) {
    // no fp_ so must be compressed read
    gzclose(gzfp_);
  }
#endif
  if (fp_)
    fclose ( fp_ );
}

void
strcpyAndCompress ( char *to, const char *from )
{
  int n = strlen ( from );
  int i;
  int nto = 0;

  for ( i = 0; i < n; i++ ) {
    if ( from[i] != ' ' ) {
      to[nto++] = from[i];
    }
  }
  if ( !nto )
    to[nto++] = ' ';
  to[nto] = '\0';
}

//  nextField
OSISectionType
OSIMpsio::nextField (  )
{
  mpsType_ = OSI_BLANK_COLUMN;
  // find next non blank character
  char *next = position_;

  while ( next != eol_ ) {
    if ( *next == ' ' || *next == '\t' ) {
      next++;
    } else {
      break;
    }
  }
  bool gotCard;

  if ( next == eol_ ) {
    gotCard = false;
  } else {
    gotCard = true;
  }
  while ( !gotCard ) {
    // need new image

    if ( cleanCard() ) {
      return OSI_EOF_SECTION;
    }
    if ( card_[0] == ' ' ) {
      // not a section or comment
      position_ = card_;
      eol_ = card_ + strlen ( card_ );
      // get mps type and column name
      // scan to first non blank
      next = card_;
      while ( next != eol_ ) {
	if ( *next == ' ' || *next == '\t' ) {
	  next++;
	} else {
	  break;
	}
      }
      if ( next != eol_ ) {
	char *nextBlank = nextBlankOr ( next );
	int nchar;

	if ( nextBlank ) {
	  nchar = nextBlank - next;
	} else {
	  nchar = -1;
	}
	mpsType_ = OSI_BLANK_COLUMN;
	// special coding if RHS or RANGE, not free format and blanks
	if ( ( section_ != OSI_RHS_SECTION && section_ != OSI_RANGE_SECTION )
	     || freeFormat_ || strncmp ( card_ + 4, "        ", 8 ) ) {
	  // if columns section only look for first field if MARKER
	  if ( section_ == OSI_COLUMN_SECTION
	       && !strstr ( next, "'MARKER'" ) ) nchar = -1;
	  if ( nchar == allowedLength[section_] ) {
	    //could be a type
	    int i;

	    for ( i = startType[section_]; i < endType[section_]; i++ ) {
	      if ( !strncmp ( next, mpsTypes[i], nchar ) ) {
		mpsType_ = ( OSIMpsType ) i;
		break;
	      }
	    }
	    if ( mpsType_ != OSI_BLANK_COLUMN ) {
	      //we know all we need so we can skip over
	      next = nextBlank;
	      while ( next != eol_ ) {
		if ( *next == ' ' || *next == '\t' ) {
		  next++;
		} else {
		  break;
		}
	      }
	      if ( next == eol_ ) {
		// error
		position_ = eol_;
		mpsType_ = OSI_UNKNOWN_MPS_TYPE;
	      } else {
		nextBlank = nextBlankOr ( next );
	      }
	    }
	  }
	  if ( mpsType_ != OSI_UNKNOWN_MPS_TYPE ) {
	    // special coding if BOUND, not free format and blanks
	    if ( section_ != OSI_BOUND_SECTION ||
		 freeFormat_ || strncmp ( card_ + 4, "        ", 8 ) ) {
	      char save = '?';

	      if ( !freeFormat_ && eightChar_ && next == card_ + 4 ) {
		if ( eol_ - next >= 8 ) {
		  if ( *( next + 8 ) != ' ' && *( next + 8 ) != '\0' ) {
		    eightChar_ = false;
		  } else {
		    nextBlank = next + 8;
		  }
		  save = *nextBlank;
		  *nextBlank = '\0';
		} else {
		  nextBlank = NULL;
		}
	      } else {
		if ( nextBlank ) {
		  save = *nextBlank;
		  *nextBlank = '\0';
		}
	      }
	      strcpyAndCompress ( columnName_, next );
	      if ( nextBlank ) {
		*nextBlank = save;
		// on to next
		next = nextBlank;
	      } else {
		next = eol_;
	      }
	    } else {
	      // blank bounds name
	      strcpy ( columnName_, "        " );
	    }
	    while ( next != eol_ ) {
	      if ( *next == ' ' || *next == '\t' ) {
		next++;
	      } else {
		break;
	      }
	    }
	    if ( next == eol_ ) {
	      // error unless row section
	      position_ = eol_;
	      value_ = -1.0e100;
	      if ( section_ != OSI_ROW_SECTION )
		mpsType_ = OSI_UNKNOWN_MPS_TYPE;
	    } else {
	      nextBlank = nextBlankOr ( next );
	    }
	    if ( section_ != OSI_ROW_SECTION ) {
	      char save = '?';

	      if ( !freeFormat_ && eightChar_ && next == card_ + 14 ) {
		if ( eol_ - next >= 8 ) {
		  if ( *( next + 8 ) != ' ' && *( next + 8 ) != '\0' ) {
		    eightChar_ = false;
		  } else {
		    nextBlank = next + 8;
		  }
		  save = *nextBlank;
		  *nextBlank = '\0';
		} else {
		  nextBlank = NULL;
		}
	      } else {
		if ( nextBlank ) {
		  save = *nextBlank;
		  *nextBlank = '\0';
		}
	      }
	      strcpyAndCompress ( rowName_, next );
	      if ( nextBlank ) {
		*nextBlank = save;
		// on to next
		next = nextBlank;
	      } else {
		next = eol_;
	      }
	      while ( next != eol_ ) {
		if ( *next == ' ' || *next == '\t' ) {
		  next++;
		} else {
		  break;
		}
	      }
	      // special coding for markers
	      if ( section_ == OSI_COLUMN_SECTION &&
		   !strncmp ( rowName_, "'MARKER'", 8 ) && next != eol_ ) {
		if ( !strncmp ( next, "'INTORG'", 8 ) ) {
		  mpsType_ = OSI_INTORG;
		} else if ( !strncmp ( next, "'INTEND'", 8 ) ) {
		  mpsType_ = OSI_INTEND;
		} else if ( !strncmp ( next, "'SOSORG'", 8 ) ) {
		  if ( mpsType_ == OSI_BLANK_COLUMN )
		    mpsType_ = OSI_S1_COLUMN;
		} else if ( !strncmp ( next, "'SOSEND'", 8 ) ) {
		  mpsType_ = OSI_SOSEND;
		} else {
		  mpsType_ = OSI_UNKNOWN_MPS_TYPE;
		}
		position_ = eol_;
		return section_;
	      }
	      if ( next == eol_ ) {
		// error unless bounds
		position_ = eol_;
		value_ = -1.0e100;
		if ( section_ != OSI_BOUND_SECTION )
		  mpsType_ = OSI_UNKNOWN_MPS_TYPE;
	      } else {
		nextBlank = nextBlankOr ( next );
		if ( nextBlank ) {
		  save = *nextBlank;
		  *nextBlank = '\0';
		}
		char * after;
		value_ = osi_strtod(next,&after);
		// see if error
		assert(after>next);
		if ( nextBlank ) {
		  *nextBlank = save;
		  position_ = nextBlank;
		} else {
		  position_ = eol_;
		}
	      }
	    }
	  }
	} else {
	  //blank name in RHS or RANGE
	  strcpy ( columnName_, "        " );
	  char save = '?';

	  if ( !freeFormat_ && eightChar_ && next == card_ + 14 ) {
	    if ( eol_ - next >= 8 ) {
	      if ( *( next + 8 ) != ' ' && *( next + 8 ) != '\0' ) {
		eightChar_ = false;
	      } else {
		nextBlank = next + 8;
	      }
	      save = *nextBlank;
	      *nextBlank = '\0';
	    } else {
	      nextBlank = NULL;
	    }
	  } else {
	    if ( nextBlank ) {
	      save = *nextBlank;
	      *nextBlank = '\0';
	    }
	  }
	  strcpyAndCompress ( rowName_, next );
	  if ( nextBlank ) {
	    *nextBlank = save;
	    // on to next
	    next = nextBlank;
	  } else {
	    next = eol_;
	  }
	  while ( next != eol_ ) {
	    if ( *next == ' ' || *next == '\t' ) {
	      next++;
	    } else {
	      break;
	    }
	  }
	  if ( next == eol_ ) {
	    // error 
	    position_ = eol_;
	    value_ = -1.0e100;
	    mpsType_ = OSI_UNKNOWN_MPS_TYPE;
	  } else {
	    nextBlank = nextBlankOr ( next );
	    value_ = -1.0e100;
	    if ( nextBlank ) {
	      save = *nextBlank;
	      *nextBlank = '\0';
	    }
	    char * after;
	    value_ = osi_strtod(next,&after);
	    // see if error
	    assert(after>next);
	    if ( nextBlank ) {
	      *nextBlank = save;
	      position_ = nextBlank;
	    } else {
	      position_ = eol_;
	    }
	  }
	}
      }
      return section_;
    } else if ( card_[0] != '*' ) {
      // not a comment
      int i;

      {
	std::cout<<"At line "<< cardNumber_ <<" "<< card_<<std::endl;
      }
      for ( i = OSI_ROW_SECTION; i < OSI_UNKNOWN_SECTION; i++ ) {
	if ( !strncmp ( card_, section[i], strlen ( section[i] ) ) ) {
	  break;
	}
      }
      position_ = card_;
      eol_ = card_;
      section_ = ( OSISectionType ) i;
      return section_;
    } else {
      // comment
    }
  }
  // we only get here for second field (we could even allow more???)
  {
    char save = '?';
    char *nextBlank = nextBlankOr ( next );

    if ( !freeFormat_ && eightChar_ && next == card_ + 39 ) {
      if ( eol_ - next >= 8 ) {
	if ( *( next + 8 ) != ' ' && *( next + 8 ) != '\0' ) {
	  eightChar_ = false;
	} else {
	  nextBlank = next + 8;
	}
	save = *nextBlank;
	*nextBlank = '\0';
      } else {
	nextBlank = NULL;
      }
    } else {
      if ( nextBlank ) {
	save = *nextBlank;
	*nextBlank = '\0';
      }
    }
    strcpyAndCompress ( rowName_, next );
    // on to next
    if ( nextBlank ) {
      *nextBlank = save;
      next = nextBlank;
    } else {
      next = eol_;
    }
    while ( next != eol_ ) {
      if ( *next == ' ' || *next == '\t' ) {
	next++;
      } else {
	break;
      }
    }
    if ( next == eol_ ) {
      // error
      position_ = eol_;
      mpsType_ = OSI_UNKNOWN_MPS_TYPE;
    } else {
      nextBlank = nextBlankOr ( next );
    }
    if ( nextBlank ) {
      save = *nextBlank;
      *nextBlank = '\0';
    }
    //value_ = -1.0e100;
    char * after;
    value_ = osi_strtod(next,&after);
    // see if error
    assert(after>next);
    if ( nextBlank ) {
      *nextBlank = save;
      position_ = nextBlank;
    } else {
      position_ = eol_;
    }
  }
  return section_;
}
static int
hash ( const char *name, int maxsiz, int length )
{
  static int mmult[] = {
    262139, 259459, 256889, 254291, 251701, 249133, 246709, 244247,
    241667, 239179, 236609, 233983, 231289, 228859, 226357, 223829,
    221281, 218849, 216319, 213721, 211093, 208673, 206263, 203773,
    201233, 198637, 196159, 193603, 191161, 188701, 186149, 183761,
    181303, 178873, 176389, 173897, 171469, 169049, 166471, 163871,
    161387, 158941, 156437, 153949, 151531, 149159, 146749, 144299,
    141709, 139369, 136889, 134591, 132169, 129641, 127343, 124853,
    122477, 120163, 117757, 115361, 112979, 110567, 108179, 105727,
    103387, 101021, 98639, 96179, 93911, 91583, 89317, 86939, 84521,
    82183, 79939, 77587, 75307, 72959, 70793, 68447, 66103
  };
  int n = 0;
  int j;

  for ( j = 0; j < length; ++j ) {
    int iname = name[j];

    n += mmult[j] * iname;
  }
  return ( abs ( n ) % maxsiz );	/* integer abs */
}

//  startHash.  Creates hash list for names
void
OsiMpsReader::startHash ( char **names, const OSIColumnIndex number , int section )
{
  names_[section] = names;
  numberHash_[section] = number;
  startHash(section);
}
void
OsiMpsReader::startHash ( int section ) const
{
  char ** names = names_[section];
  OSIColumnIndex number = numberHash_[section];
  OSIColumnIndex i;
  OSIColumnIndex maxhash = 4 * number;
  OSIColumnIndex ipos, iput;

  //hash_=(OsiHashLink *) malloc(maxhash*sizeof(OsiHashLink));
  hash_[section] = new OsiHashLink[maxhash];
  
  OsiHashLink * hashThis = hash_[section];

  for ( i = 0; i < maxhash; i++ ) {
    hashThis[i].index = -1;
    hashThis[i].next = -1;
  }

  /*
   * Initialize the hash table.  Only the index of the first name that
   * hashes to a value is entered in the table; subsequent names that
   * collide with it are not entered.
   */
  for ( i = 0; i < number; ++i ) {
    char *thisName = names[i];
    int length = strlen ( thisName );

    ipos = hash ( thisName, maxhash, length );
    if ( hashThis[ipos].index == -1 ) {
      hashThis[ipos].index = i;
    }
  }

  /*
   * Now take care of the names that collided in the preceding loop,
   * by finding some other entry in the table for them.
   * Since there are as many entries in the table as there are names,
   * there must be room for them.
   */
  iput = -1;
  for ( i = 0; i < number; ++i ) {
    char *thisName = names[i];
    int length = strlen ( thisName );

    ipos = hash ( thisName, maxhash, length );

    while ( 1 ) {
      OSIColumnIndex j1 = hashThis[ipos].index;

      if ( j1 == i )
	break;
      else {
	char *thisName2 = names[j1];

	if ( strcmp ( thisName, thisName2 ) == 0 ) {
	  printf ( "** duplicate name %s\n", names[i] );
	  break;
	} else {
	  OSIColumnIndex k = hashThis[ipos].next;

	  if ( k == -1 ) {
	    while ( 1 ) {
	      ++iput;
	      if ( iput > number ) {
		printf ( "** too many names\n" );
		break;
	      }
	      if ( hashThis[iput].index == -1 ) {
		break;
	      }
	    }
	    hashThis[ipos].next = iput;
	    hashThis[iput].index = i;
	    break;
	  } else {
	    ipos = k;
	    /* nothing worked - try it again */
	  }
	}
      }
    }
  }
}

//  stopHash.  Deletes hash storage
void
OsiMpsReader::stopHash ( int section )
{
  delete [] hash_[section];
  hash_[section] = NULL;
}

//  findHash.  -1 not found
OSIColumnIndex
OsiMpsReader::findHash ( const char *name , int section ) const
{
  OSIColumnIndex found = -1;

  char ** names = names_[section];
  OsiHashLink * hashThis = hash_[section];
  OSIColumnIndex maxhash = 4 * numberHash_[section];
  OSIColumnIndex ipos;

  /* default if we don't find anything */
  if ( !maxhash )
    return -1;
  int length = strlen ( name );

  ipos = hash ( name, maxhash, length );
  while ( 1 ) {
    OSIColumnIndex j1 = hashThis[ipos].index;

    if ( j1 >= 0 ) {
      char *thisName2 = names[j1];

      if ( strcmp ( name, thisName2 ) != 0 ) {
	OSIColumnIndex k = hashThis[ipos].next;

	if ( k != -1 )
	  ipos = k;
	else
	  break;
      } else {
	found = j1;
	break;
      }
    } else {
      found = -1;
      break;
    }
  }
  return found;
}

//------------------------------------------------------------------
// Get value for infinity
//------------------------------------------------------------------
double OsiMpsReader::getInfinity() const
{
  return infinity_;
}
//------------------------------------------------------------------
// Set value for infinity
//------------------------------------------------------------------
void OsiMpsReader::setInfinity(double value) 
{
  if ( value >= 1.020 ) {
    infinity_ = value;
  } else {
    std::cout << "Illegal value for infinity of " << value << std::endl;
  }

}
// Set file name
void OsiMpsReader::setFileName(const char * name)
{
  free(fileName_);
  fileName_=strdup(name);
}
// Get file name
const char * OsiMpsReader::getFileName() const
{
  return fileName_;
}
// Test if current file exists and readable
const bool OsiMpsReader::fileReadable() const
{
  // I am opening it to make sure not odd
  FILE *fp;
  if (strcmp(fileName_,"stdin")) {
    fp = fopen ( fileName_, "r" );
  } else {
    fp = stdin;
  }
  if (!fp) {
    return false;
  } else {
    fclose(fp);
    return true;
  }
}
/* objective offset - this is RHS entry for objective row */
double OsiMpsReader::objectiveOffset() const
{
  return objectiveOffset_;
}
#define MAX_INTEGER 1000000
// Sets default upper bound for integer variables
void OsiMpsReader::setDefaultBound(int value)
{
  if ( value >= 1 && value <=MAX_INTEGER ) {
    defaultBound_ = value;
  } else {
    std::cout << "Illegal default integer bound of " << value << std::endl;
  }
}
// gets default upper bound for integer variables
int OsiMpsReader::getDefaultBound() const
{
  return defaultBound_;
}
//------------------------------------------------------------------
// Read mps files
//------------------------------------------------------------------
int OsiMpsReader::readMps(const char * filename,  const char * extension)
{
  // clean up later as pointless using strings
  std::string f(filename);
  std::string fullname;
  free(fileName_);
  if (strcmp(filename,"stdin")&&strcmp(filename,"-")) {
    std::string e(extension);
    if (f.find('.')<=0&&strlen(extension)) 
      fullname = f + "." + e;
    else
      fullname = f;
    fileName_=strdup(fullname.c_str());    
  } else {
    fileName_=strdup("stdin");    
  }
  return readMps();
}
int OsiMpsReader::readMps()
{
  FILE *fp;
  bool goodFile=false;
#ifdef COIN_USE_ZLIB
  gzFile gzFile=NULL;
#endif
  if (strcmp(fileName_,"stdin")) {
#ifdef COIN_USE_ZLIB
    int length=strlen(fileName_);
    if (!strcmp(fileName_+length-3,".gz")) {
      gzFile = gzopen(fileName_,"rb");
      fp = NULL;
      goodFile = (gzFile!=NULL);
    } else {
#endif
      fp = fopen ( fileName_, "r" );
      goodFile = (fp!=NULL);
#ifdef COIN_USE_ZLIB
      if (! goodFile) {
	 // Try to open it with .gz extension
	 std::string fil(fileName_);
	 fil += ".gz";
	 gzFile = gzopen(fil.c_str(),"rb");
	 goodFile = (gzFile!=NULL);
      }
    }
#endif
  } else {
    fp = stdin;
    goodFile = true;
  }
  if (!goodFile) {
    std::cout << "Unable to open file " << fileName_ << std::endl;
    return -1;
  }
  bool ifmps;
#ifdef COIN_USE_ZLIB
  OSIMpsio mpsfile ( fp , gzFile);
#else
  OSIMpsio mpsfile ( fp );
#endif

  if ( mpsfile.whichSection (  ) == OSI_NAME_SECTION ) {
    ifmps = true;
    // save name of section
    free(problemName_);
    problemName_=strdup(mpsfile.columnName());
  } else if ( mpsfile.whichSection (  ) == OSI_UNKNOWN_SECTION ) {
    std::cout << "Unknown image " << mpsfile.
      card (  ) << " at line 1 of file " << fileName_ << std::endl;
#ifdef COIN_USE_ZLIB
    if (!fp) {
      std::cout << "Consider the possibility of a compressed file "
		<< "which zlib is unable to read" << std::endl;
    }
#endif
    return -2;
  } else if ( mpsfile.whichSection (  ) != OSI_EOF_SECTION ) {
    // save name of section
    free(problemName_);
    problemName_=strdup(mpsfile.card());
    ifmps = false;
  } else {
    std::cout << "EOF on file" << fileName_ << std::endl;
    return -3;
  }
  OSIElementIndex *start;
  OSIRowIndex *row;
  double *element;
  objectiveOffset_ = 0.0;

  int numberErrors = 0;
  int i;

  if ( ifmps ) {
    // mps file - always read in free format
    bool gotNrow = false;

    //get ROWS
    assert ( mpsfile.nextField (  ) == OSI_ROW_SECTION );
    //use malloc etc as I don't know how to do realloc in C++
    numberRows_ = 0;
    numberColumns_ = 0;
    numberElements_ = 0;
    OSIRowIndex maxRows = 1000;
    OSIMpsType *rowType =

      ( OSIMpsType * ) malloc ( maxRows * sizeof ( OSIMpsType ) );
    char **rowName = ( char ** ) malloc ( maxRows * sizeof ( char * ) );

    // for discarded free rows
    OSIRowIndex maxFreeRows = 100;
    OSIRowIndex numberOtherFreeRows = 0;
    char **freeRowName =

      ( char ** ) malloc ( maxFreeRows * sizeof ( char * ) );
    while ( mpsfile.nextField (  ) == OSI_ROW_SECTION ) {
      switch ( mpsfile.mpsType (  ) ) {
      case OSI_N_ROW:
	if ( !gotNrow ) {
	  gotNrow = true;
	  // save name of section
	  free(objectiveName_);
	  objectiveName_=strdup(mpsfile.columnName());
	} else {
	  // add to discard list
	  if ( numberOtherFreeRows == maxFreeRows ) {
	    maxFreeRows = ( 3 * maxFreeRows ) / 2 + 100;
	    freeRowName =
	      ( char ** ) realloc ( freeRowName,

				    maxFreeRows * sizeof ( char * ) );
	  }
	  freeRowName[numberOtherFreeRows] =
	    strdup ( mpsfile.columnName (  ) );
	  numberOtherFreeRows++;
	}
	break;
      case OSI_E_ROW:
      case OSI_L_ROW:
      case OSI_G_ROW:
	if ( numberRows_ == maxRows ) {
	  maxRows = ( 3 * maxRows ) / 2 + 1000;
	  rowType =
	    ( OSIMpsType * ) realloc ( rowType,
				       maxRows * sizeof ( OSIMpsType ) );
	  rowName =

	    ( char ** ) realloc ( rowName, maxRows * sizeof ( char * ) );
	}
	rowType[numberRows_] = mpsfile.mpsType (  );
	rowName[numberRows_] = strdup ( mpsfile.columnName (  ) );
	numberRows_++;
	break;
      default:
	numberErrors++;
	if ( numberErrors < 100 ) {
	  std::cout << "Bad image at card " << mpsfile.
	    cardNumber (  ) << " " << mpsfile.card (  ) << std::endl;
	} else if (numberErrors > 100000) {
	  std::cout << "Returning as too many errors"<< std::endl;
	  return numberErrors;
	}
      }
    }
    assert ( mpsfile.whichSection (  ) == OSI_COLUMN_SECTION );
    assert ( gotNrow );
    rowType =
      ( OSIMpsType * ) realloc ( rowType,
				 numberRows_ * sizeof ( OSIMpsType ) );
    // put objective and other free rows at end
    rowName =
      ( char ** ) realloc ( rowName,
			    ( numberRows_ + 1 +

			      numberOtherFreeRows ) * sizeof ( char * ) );
    rowName[numberRows_] = strdup(objectiveName_);
    memcpy ( rowName + numberRows_ + 1, freeRowName,
	     numberOtherFreeRows * sizeof ( char * ) );
    // now we can get rid of this array
    free(freeRowName);

    startHash ( rowName, numberRows_ + 1 + numberOtherFreeRows , 0 );
    OSIColumnIndex maxColumns = 1000 + numberRows_ / 5;
    OSIElementIndex maxElements = 5000 + numberRows_ / 2;
    OSIMpsType *columnType = ( OSIMpsType * )
      malloc ( maxColumns * sizeof ( OSIMpsType ) );
    char **columnName = ( char ** ) malloc ( maxColumns * sizeof ( char * ) );

    objective_ = ( double * ) malloc ( maxColumns * sizeof ( double ) );
    start = ( OSIElementIndex * )
      malloc ( ( maxColumns + 1 ) * sizeof ( OSIElementIndex ) );
    row = ( OSIRowIndex * )
      malloc ( maxElements * sizeof ( OSIRowIndex ) );
    element =
      ( double * ) malloc ( maxElements * sizeof ( double ) );
    // for duplicates
    OSIElementIndex *rowUsed = new OSIElementIndex[numberRows_];

    for (i=0;i<numberRows_;i++) {
      rowUsed[i]=-1;
    }
    bool objUsed = false;

    numberElements_ = 0;
    char lastColumn[200];

    memset ( lastColumn, '\0', 200 );
    OSIColumnIndex column = -1;
    bool inIntegerSet = false;
    OSIColumnIndex numberIntegers = 0;
    const double tinyElement = 1.0e-14;

    while ( mpsfile.nextField (  ) == OSI_COLUMN_SECTION ) {
      switch ( mpsfile.mpsType (  ) ) {
      case OSI_BLANK_COLUMN:
	if ( strcmp ( lastColumn, mpsfile.columnName (  ) ) ) {
	  // new column

	  // reset old column and take out tiny
	  if ( numberColumns_ ) {
	    objUsed = false;
	    OSIElementIndex i;
	    OSIElementIndex k = start[column];

	    for ( i = k; i < numberElements_; i++ ) {
	      OSIRowIndex irow = row[i];
#if 0
	      if ( fabs ( element[i] ) > tinyElement ) {
		element[k++] = element[i];
	      }
#endif
	      rowUsed[irow] = -1;
	    }
	    //numberElements_ = k;
	  }
	  column = numberColumns_;
	  if ( numberColumns_ == maxColumns ) {
	    maxColumns = ( 3 * maxColumns ) / 2 + 1000;
	    columnType = ( OSIMpsType * )
	      realloc ( columnType, maxColumns * sizeof ( OSIMpsType ) );
	    columnName = ( char ** )
	      realloc ( columnName, maxColumns * sizeof ( char * ) );

	    objective_ = ( double * )
	      realloc ( objective_, maxColumns * sizeof ( double ) );
	    start = ( OSIElementIndex * )
	      realloc ( start,
			( maxColumns + 1 ) * sizeof ( OSIElementIndex ) );
	  }
	  if ( !inIntegerSet ) {
	    columnType[column] = OSI_UNSET_BOUND;
	  } else {
	    columnType[column] = OSI_INTORG;
	    numberIntegers++;
	  }
	  columnName[column] = strdup ( mpsfile.columnName (  ) );
	  strcpy ( lastColumn, mpsfile.columnName (  ) );
	  objective_[column] = 0.0;
	  start[column] = numberElements_;
	  numberColumns_++;
	}
	if ( fabs ( mpsfile.value (  ) ) > tinyElement ) {
	  if ( numberElements_ == maxElements ) {
	    maxElements = ( 3 * maxElements ) / 2 + 1000;
	    row = ( OSIRowIndex * )
	      realloc ( row, maxElements * sizeof ( OSIRowIndex ) );
	    element = ( double * )
	      realloc ( element, maxElements * sizeof ( double ) );
	  }
	  // get row number
	  OSIRowIndex irow = findHash ( mpsfile.rowName (  ) , 0 );

	  if ( irow >= 0 ) {
	    double value = mpsfile.value (  );

	    // check for duplicates
	    if ( irow == numberRows_ ) {
	      // objective
	      if ( objUsed ) {
		numberErrors++;
		if ( numberErrors < 100 ) {
		  std::cout << "Duplicate objective at card " <<
		    mpsfile.cardNumber (  )
		    << " " << mpsfile.card (  ) << std::endl;
		} else if (numberErrors > 100000) {
		  std::cout << "Returning as too many errors"<< std::endl;
		  return numberErrors;
		}
	      } else {
		objUsed = true;
	      }
	      value += objective_[column];
	      if ( fabs ( value ) <= tinyElement )
		value = 0.0;
	      objective_[column] = value;
	    } else if ( irow < numberRows_ ) {
	      // other free rows will just be discarded so won't get here
	      if ( rowUsed[irow] >= 0 ) {
		element[rowUsed[irow]] += value;
		numberErrors++;
		if ( numberErrors < 100 ) {
		  std::cout << "Duplicate row at card " << mpsfile.
		    cardNumber (  ) << " " << mpsfile.
		    card (  ) << " " << mpsfile.rowName (  ) << std::endl;
		} else if (numberErrors > 100000) {
		  std::cout << "Returning as too many errors"<< std::endl;
		  return numberErrors;
		}
	      } else {
		row[numberElements_] = irow;
		element[numberElements_] = value;
		rowUsed[irow] = numberElements_;
		numberElements_++;
	      }
	    }
	  } else {
	    numberErrors++;
	    if ( numberErrors < 100 ) {
	      std::cout << "No match for row at card " << mpsfile.
		cardNumber (  ) << " " << mpsfile.card (  ) << " " << mpsfile.
		rowName (  ) << std::endl;
	    } else if (numberErrors > 100000) {
	      std::cout << "Returning as too many errors"<< std::endl;
	      return numberErrors;
	    }
	  }
	}
	break;
      case OSI_INTORG:
	inIntegerSet = true;
	break;
      case OSI_INTEND:
	inIntegerSet = false;
	break;
      case OSI_S1_COLUMN:
      case OSI_S2_COLUMN:
      case OSI_S3_COLUMN:
      case OSI_SOSEND:
	std::cout << "** code sos etc later" << std::endl;
	abort (  );
	break;
      default:
	numberErrors++;
	if ( numberErrors < 100 ) {
	  std::cout << "Bad image at card " << mpsfile.
	    cardNumber (  ) << " " << mpsfile.card (  ) << std::endl;
	} else if (numberErrors > 100000) {
	  std::cout << "Returning as too many errors"<< std::endl;
	  return numberErrors;
	}
      }
    }
    start[numberColumns_] = numberElements_;
    delete[]rowUsed;
    assert ( mpsfile.whichSection (  ) == OSI_RHS_SECTION );
    columnType =
      ( OSIMpsType * ) realloc ( columnType,
				 numberColumns_ * sizeof ( OSIMpsType ) );
    columnName =

      ( char ** ) realloc ( columnName, numberColumns_ * sizeof ( char * ) );
    objective_ = ( double * )
      realloc ( objective_, numberColumns_ * sizeof ( double ) );
    start = ( OSIElementIndex * )
      realloc ( start, ( numberColumns_ + 1 ) * sizeof ( OSIElementIndex ) );
    row = ( OSIRowIndex * )
      realloc ( row, numberElements_ * sizeof ( OSIRowIndex ) );
    element = ( double * )
      realloc ( element, numberElements_ * sizeof ( double ) );

    rowlower_ = ( double * ) malloc ( numberRows_ * sizeof ( double ) );
    rowupper_ = ( double * ) malloc ( numberRows_ * sizeof ( double ) );
    for (i=0;i<numberRows_;i++) {
      rowlower_[i]=-infinity_;
      rowupper_[i]=infinity_;
    }
    objUsed = false;
    memset ( lastColumn, '\0', 200 );
    bool gotRhs = false;

    // need coding for blank rhs
    while ( mpsfile.nextField (  ) == OSI_RHS_SECTION ) {
      OSIRowIndex irow;

      switch ( mpsfile.mpsType (  ) ) {
      case OSI_BLANK_COLUMN:
	if ( strcmp ( lastColumn, mpsfile.columnName (  ) ) ) {

	  // skip rest if got a rhs
	  if ( gotRhs ) {
	    while ( mpsfile.nextField (  ) == OSI_RHS_SECTION ) {
	    }
	  } else {
	    gotRhs = true;
	    strcpy ( lastColumn, mpsfile.columnName (  ) );
	    // save name of section
	    free(rhsName_);
	    rhsName_=strdup(mpsfile.columnName());
	  }
	}
	// get row number
	irow = findHash ( mpsfile.rowName (  ) , 0 );
	if ( irow >= 0 ) {
	  double value = mpsfile.value (  );

	  // check for duplicates
	  if ( irow == numberRows_ ) {
	    // objective
	    if ( objUsed ) {
	      numberErrors++;
	      if ( numberErrors < 100 ) {
		std::cout << "Duplicate objective at card " <<
		  mpsfile.cardNumber (  )
		  << " " << mpsfile.card (  ) << std::endl;
	      } else if (numberErrors > 100000) {
		std::cout << "Returning as too many errors"<< std::endl;
		return numberErrors;
	      }
	    } else {
	      objUsed = true;
	    }
	    objectiveOffset_ += value;
	  } else if ( irow < numberRows_ ) {
	    if ( rowlower_[irow] != -infinity_ ) {
	      numberErrors++;
	      if ( numberErrors < 100 ) {
		std::cout << "Duplicate row at card " << mpsfile.
		  cardNumber (  ) << " " << mpsfile.
		  card (  ) << " " << mpsfile.rowName (  ) << std::endl;
	      } else if (numberErrors > 100000) {
		std::cout << "Returning as too many errors"<< std::endl;
		return numberErrors;
	      }
	    } else {
	      rowlower_[irow] = value;
	    }
	  }
	} else {
	  numberErrors++;
	  if ( numberErrors < 100 ) {
	    std::cout << "No match for row at card " << mpsfile.
	      cardNumber (  ) << " " << mpsfile.card (  ) << " " << mpsfile.
	      rowName (  ) << std::endl;
	  } else if (numberErrors > 100000) {
	    std::cout << "Returning as too many errors"<< std::endl;
	    return numberErrors;
	  }
	}
	break;
      default:
	numberErrors++;
	if ( numberErrors < 100 ) {
	  std::cout << "Bad image at card " << mpsfile.
	    cardNumber (  ) << " " << mpsfile.card (  ) << std::endl;
	} else if (numberErrors > 100000) {
	  std::cout << "Returning as too many errors"<< std::endl;
	  return numberErrors;
	}
      }
    }
    if ( mpsfile.whichSection (  ) == OSI_RANGE_SECTION ) {
      memset ( lastColumn, '\0', 200 );
      bool gotRange = false;
      OSIRowIndex irow;

      // need coding for blank range
      while ( mpsfile.nextField (  ) == OSI_RANGE_SECTION ) {
	switch ( mpsfile.mpsType (  ) ) {
	case OSI_BLANK_COLUMN:
	  if ( strcmp ( lastColumn, mpsfile.columnName (  ) ) ) {

	    // skip rest if got a range
	    if ( gotRange ) {
	      while ( mpsfile.nextField (  ) == OSI_RANGE_SECTION ) {
	      }
	    } else {
	      gotRange = true;
	      strcpy ( lastColumn, mpsfile.columnName (  ) );
	      // save name of section
	      free(rangeName_);
	      rangeName_=strdup(mpsfile.columnName());
	    }
	  }
	  // get row number
	  irow = findHash ( mpsfile.rowName (  ) , 0 );
	  if ( irow >= 0 ) {
	    double value = mpsfile.value (  );

	    // check for duplicates
	    if ( irow == numberRows_ ) {
	      // objective
	      numberErrors++;
	      if ( numberErrors < 100 ) {
		std::cout << "Duplicate objective at card " <<
		  mpsfile.cardNumber (  )
		  << " " << mpsfile.card (  ) << std::endl;
	      } else if (numberErrors > 100000) {
		std::cout << "Returning as too many errors"<< std::endl;
		return numberErrors;
	      }
	    } else {
	      if ( rowupper_[irow] != infinity_ ) {
		numberErrors++;
		if ( numberErrors < 100 ) {
		  std::cout << "Duplicate row at card " << mpsfile.
		    cardNumber (  ) << " " << mpsfile.
		    card (  ) << " " << mpsfile.rowName (  ) << std::endl;
		} else if (numberErrors > 100000) {
		  std::cout << "Returning as too many errors"<< std::endl;
		  return numberErrors;
		}
	      } else {
		rowupper_[irow] = value;
	      }
	    }
	  } else {
	    numberErrors++;
	    if ( numberErrors < 100 ) {
	      std::cout << "No match for row at card " << mpsfile.
		cardNumber (  ) << " " << mpsfile.card (  ) << " " << mpsfile.
		rowName (  ) << std::endl;
	    } else if (numberErrors > 100000) {
	      std::cout << "Returning as too many errors"<< std::endl;
	      return numberErrors;
	    }
	  }
	  break;
	default:
	  numberErrors++;
	  if ( numberErrors < 100 ) {
	    std::cout << "Bad image at card " << mpsfile.
	      cardNumber (  ) << " " << mpsfile.card (  ) << std::endl;
	  } else if (numberErrors > 100000) {
	    std::cout << "Returning as too many errors"<< std::endl;
	    return numberErrors;
	  }
	}
      }
    }
    stopHash ( 0 );
    // massage ranges
    {
      OSIRowIndex irow;

      for ( irow = 0; irow < numberRows_; irow++ ) {
	double lo = rowlower_[irow];
	double up = rowupper_[irow];
	double up2 = rowupper_[irow];	//range

	switch ( rowType[irow] ) {
	case OSI_E_ROW:
	  if ( lo == -infinity_ )
	    lo = 0.0;
	  if ( up == infinity_ ) {
	    up = lo;
	  } else if ( up > 0.0 ) {
	    up += lo;
	  } else {
	    up = lo;
	    lo += up2;
	  }
	  break;
	case OSI_L_ROW:
	  if ( lo == -infinity_ ) {
	    up = 0.0;
	  } else {
	    up = lo;
	    lo = -infinity_;
	  }
	  if ( up2 != infinity_ ) {
	    lo = up - fabs ( up2 );
	  }
	  break;
	case OSI_G_ROW:
	  if ( lo == -infinity_ ) {
	    lo = 0.0;
	    up = infinity_;
	  } else {
	    up = infinity_;
	  }
	  if ( up2 != infinity_ ) {
	    up = lo + fabs ( up2 );
	  }
	  break;
	default:
	  abort();
	}
	rowlower_[irow] = lo;
	rowupper_[irow] = up;
      }
    }
    free ( rowType );
    // default bounds
    collower_ = ( double * ) malloc ( numberColumns_ * sizeof ( double ) );
    colupper_ = ( double * ) malloc ( numberColumns_ * sizeof ( double ) );
    for (i=0;i<numberColumns_;i++) {
      collower_[i]=0.0;
      colupper_[i]=infinity_;
    }
    // set up integer region just in case
    integerType_ = (char *) malloc (numberColumns_*sizeof(char));

    for ( column = 0; column < numberColumns_; column++ ) {
      if ( columnType[column] == OSI_INTORG ) {
	columnType[column] = OSI_UNSET_BOUND;
	integerType_[column] = 1;
      } else {
	integerType_[column] = 0;
      }
    }
    // start hash even if no bound section - to make sure names survive
    startHash ( columnName, numberColumns_ , 1 );
    if ( mpsfile.whichSection (  ) == OSI_BOUND_SECTION ) {
      memset ( lastColumn, '\0', 200 );
      bool gotBound = false;

      while ( mpsfile.nextField (  ) == OSI_BOUND_SECTION ) {
	if ( strcmp ( lastColumn, mpsfile.columnName (  ) ) ) {

	  // skip rest if got a bound
	  if ( gotBound ) {
	    while ( mpsfile.nextField (  ) == OSI_BOUND_SECTION ) {
	    }
	  } else {
	    gotBound = true;;
	    strcpy ( lastColumn, mpsfile.columnName (  ) );
	    // save name of section
	    free(boundName_);
	    boundName_=strdup(mpsfile.columnName());
	  }
	}
	// get column number
	OSIColumnIndex icolumn = findHash ( mpsfile.rowName (  ) , 1 );

	if ( icolumn >= 0 ) {
	  double value = mpsfile.value (  );
	  bool ifError = false;

	  switch ( mpsfile.mpsType (  ) ) {
	  case OSI_UP_BOUND:
	    if ( value == -1.0e100 )
	      ifError = true;
	    if ( columnType[icolumn] == OSI_UNSET_BOUND ) {
	      if ( value < 0.0 ) {
		collower_[icolumn] = -infinity_;
	      }
	    } else if ( columnType[icolumn] == OSI_LO_BOUND ) {
	      if ( value < collower_[icolumn] ) {
		ifError = true;
	      } else if ( value < collower_[icolumn] + tinyElement ) {
		value = collower_[icolumn];
	      }
	    } else if ( columnType[icolumn] == OSI_MI_BOUND ) {
	    } else {
	      ifError = true;
	    }
	    colupper_[icolumn] = value;
	    columnType[icolumn] = OSI_UP_BOUND;
	    break;
	  case OSI_LO_BOUND:
	    if ( value == -1.0e100 )
	      ifError = true;
	    if ( columnType[icolumn] == OSI_UNSET_BOUND ) {
	    } else if ( columnType[icolumn] == OSI_UP_BOUND ||
			columnType[icolumn] == OSI_UI_BOUND ) {
	      if ( value > colupper_[icolumn] ) {
		ifError = true;
	      } else if ( value > colupper_[icolumn] - tinyElement ) {
		value = colupper_[icolumn];
	      }
	    } else {
	      ifError = true;
	    }
	    collower_[icolumn] = value;
	    columnType[icolumn] = OSI_LO_BOUND;
	    break;
	  case OSI_FX_BOUND:
	    if ( value == -1.0e100 )
	      ifError = true;
	    if ( columnType[icolumn] == OSI_UNSET_BOUND ) {
	    } else {
	      ifError = true;
	    }
	    collower_[icolumn] = value;
	    colupper_[icolumn] = value;
	    columnType[icolumn] = OSI_FX_BOUND;
	    break;
	  case OSI_FR_BOUND:
	    if ( columnType[icolumn] == OSI_UNSET_BOUND ) {
	    } else {
	      ifError = true;
	    }
	    collower_[icolumn] = -infinity_;
	    colupper_[icolumn] = infinity_;
	    columnType[icolumn] = OSI_FR_BOUND;
	    break;
	  case OSI_MI_BOUND:
	    if ( columnType[icolumn] == OSI_UNSET_BOUND ) {
	      colupper_[icolumn] = 0.0;
	    } else if ( columnType[icolumn] == OSI_UP_BOUND ) {
	    } else {
	      ifError = true;
	    }
	    collower_[icolumn] = -infinity_;
	    columnType[icolumn] = OSI_MI_BOUND;
	    break;
	  case OSI_PL_BOUND:
	    if ( columnType[icolumn] == OSI_UNSET_BOUND ) {
	    } else {
	      ifError = true;
	    }
	    columnType[icolumn] = OSI_PL_BOUND;
	    break;
	  case OSI_UI_BOUND:
	    if ( value == -1.0e100 )
	      ifError = true;
	    if ( columnType[icolumn] == OSI_UNSET_BOUND ) {
	    } else if ( columnType[icolumn] == OSI_LO_BOUND ) {
	      if ( value < collower_[icolumn] ) {
		ifError = true;
	      } else if ( value < collower_[icolumn] + tinyElement ) {
		value = collower_[icolumn];
	      }
	    } else {
	      ifError = true;
	    }
	    colupper_[icolumn] = value;
	    columnType[icolumn] = OSI_UI_BOUND;
	    if ( !integerType_[icolumn] ) {
	      numberIntegers++;
	      integerType_[icolumn] = 1;
	    }
	    break;
	  case OSI_BV_BOUND:
	    if ( columnType[icolumn] == OSI_UNSET_BOUND ) {
	    } else {
	      ifError = true;
	    }
	    collower_[icolumn] = 0.0;
	    colupper_[icolumn] = 1.0;
	    columnType[icolumn] = OSI_BV_BOUND;
	    if ( !integerType_[icolumn] ) {
	      numberIntegers++;
	      integerType_[icolumn] = 1;
	    }
	    break;
	  default:
	    ifError = true;
	    break;
	  }
	  if ( ifError ) {
	    numberErrors++;
	    if ( numberErrors < 100 ) {
	      std::cout << "Bad image at card " << mpsfile.
		cardNumber (  ) << " " << mpsfile.card (  ) << std::endl;
	    } else if (numberErrors > 100000) {
	      std::cout << "Returning as too many errors"<< std::endl;
	      return numberErrors;
	    }
	  }
	} else {
	  numberErrors++;
	  if ( numberErrors < 100 ) {
	    std::cout << "No match for column at card " << mpsfile.
	      cardNumber (  ) << " " << mpsfile.card (  ) << " " << mpsfile.
	      rowName (  ) << std::endl;
	  } else if (numberErrors > 100000) {
	    std::cout << "Returning as too many errors"<< std::endl;
	    return numberErrors;
	  }
	}
      }
    }
    stopHash ( 1 );
    // clean up integers
    if ( !numberIntegers ) {
      free(integerType_);
      integerType_ = NULL;
    } else {
      OSIColumnIndex icolumn;

      for ( icolumn = 0; icolumn < numberColumns_; icolumn++ ) {
	if ( integerType_[icolumn] ) {
	  assert ( collower_[icolumn] >= -MAX_INTEGER );
	  // if 0 infinity make 0-1 ???
	  if ( columnType[icolumn] == OSI_UNSET_BOUND ) 
	    colupper_[icolumn] = defaultBound_;
	  if ( colupper_[icolumn] > MAX_INTEGER ) 
	    colupper_[icolumn] = MAX_INTEGER;
	}
      }
    }
    free ( columnType );
    assert ( mpsfile.whichSection (  ) == OSI_ENDATA_SECTION );
  } else {
    // This is very simple format - what should we use?
    fscanf ( fp, "%d %d %d\n", &numberRows_, &numberColumns_, &numberElements_ );
    OSIColumnIndex i;

    rowlower_ = ( double * ) malloc ( numberRows_ * sizeof ( double ) );
    rowupper_ = ( double * ) malloc ( numberRows_ * sizeof ( double ) );
    for ( i = 0; i < numberRows_; i++ ) {
      int j;

      fscanf ( fp, "%d %lg %lg\n", &j, &rowlower_[i], &rowupper_[i] );
      assert ( i == j );
    }
    collower_ = ( double * ) malloc ( numberColumns_ * sizeof ( double ) );
    colupper_ = ( double * ) malloc ( numberColumns_ * sizeof ( double ) );
    objective_= ( double * ) malloc ( numberColumns_ * sizeof ( double ) );
    start = ( OSIElementIndex *) malloc ((numberColumns_ + 1) *
					sizeof (OSIElementIndex) );
    row = ( OSIRowIndex * ) malloc (numberElements_ * sizeof (OSIRowIndex));
    element = ( double * ) malloc (numberElements_ * sizeof (double) );

    start[0] = 0;
    numberElements_ = 0;
    for ( i = 0; i < numberColumns_; i++ ) {
      int j;
      int n;

      fscanf ( fp, "%d %d %lg %lg %lg\n", &j, &n, &collower_[i], &colupper_[i],
	       &objective_[i] );
      assert ( i == j );
      for ( j = 0; j < n; j++ ) {
	fscanf ( fp, "       %d %lg\n", &row[numberElements_],
		 &element[numberElements_] );
	numberElements_++;
      }
      start[i + 1] = numberElements_;
    }
  }
  // construct packed matrix
  matrixByColumn_ = 
    new OsiPackedMatrix(true,
			numberRows_,numberColumns_,numberElements_,
			element,row,start,NULL);
  free ( row );
  free ( start );
  free ( element );

  std::cout<<"Problem "<<problemName_<< " has " << numberRows_ << " rows, " << numberColumns_
	   << " columns and " << numberElements_ << " elements" <<std::endl;
  return numberErrors;
}
// Problem name
const char * OsiMpsReader::getProblemName() const
{
  return problemName_;
}
// Objective name
const char * OsiMpsReader::getObjectiveName() const
{
  return objectiveName_;
}
// Rhs name
const char * OsiMpsReader::getRhsName() const
{
  return rhsName_;
}
// Range name
const char * OsiMpsReader::getRangeName() const
{
  return rangeName_;
}
// Bound name
const char * OsiMpsReader::getBoundName() const
{
  return boundName_;
}

//------------------------------------------------------------------
// Get number of rows, columns and elements
//------------------------------------------------------------------
int OsiMpsReader::getNumCols() const
{
  return numberColumns_;
}
int OsiMpsReader::getNumRows() const
{
  return numberRows_;
}
int OsiMpsReader::getNumElements() const
{
  return numberElements_;
}

//------------------------------------------------------------------
// Get pointer to column lower and upper bounds.
//------------------------------------------------------------------  
const double * OsiMpsReader::getColLower() const
{
  return collower_;
}
const double * OsiMpsReader::getColUpper() const
{
  return colupper_;
}

//------------------------------------------------------------------
// Get pointer to row lower and upper bounds.
//------------------------------------------------------------------  
const double * OsiMpsReader::getRowLower() const
{
  return rowlower_;
}
const double * OsiMpsReader::getRowUpper() const
{
  return rowupper_;
}
 
/** A quick inlined function to convert from lb/ub style constraint
    definition to sense/rhs/range style */
inline void
OsiMpsReader::convertBoundToSense(const double lower, const double upper,
					char& sense, double& right,
					double& range) const
{
  range = 0.0;
  if (lower > -infinity_) {
    if (upper < infinity_) {
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
    if (upper < infinity_) {
      sense = 'L';
      right = upper;
    } else {
      sense = 'N';
      right = 0.0;
    }
  }
}

//-----------------------------------------------------------------------------
/** A quick inlined function to convert from sense/rhs/range stryle constraint
    definition to lb/ub style */
inline void
OsiMpsReader::convertSenseToBound(const char sense, const double right,
					const double range,
					double& lower, double& upper) const
{
  switch (sense) {
  case 'E':
    lower = upper = right;
    break;
  case 'L':
    lower = -infinity_;
    upper = right;
    break;
  case 'G':
    lower = right;
    upper = infinity_;
    break;
  case 'R':
    lower = right - range;
    upper = right;
    break;
  case 'N':
    lower = -infinity_;
    upper = infinity_;
    break;
  }
}
//------------------------------------------------------------------
// Get sense of row constraints.
//------------------------------------------------------------------ 
const char * OsiMpsReader::getRowSense() const
{
  if ( rowsense_==NULL ) {

    int nr=numberRows_;
    rowsense_ = (char *) malloc(nr*sizeof(char));


    double dum1,dum2;
    int i;
    for ( i=0; i<nr; i++ ) {
      convertBoundToSense(rowlower_[i],rowupper_[i],rowsense_[i],dum1,dum2);
    }
  }
  return rowsense_;
}

//------------------------------------------------------------------
// Get the rhs of rows.
//------------------------------------------------------------------ 
const double * OsiMpsReader::getRightHandSide() const
{
  if ( rhs_==NULL ) {

    int nr=numberRows_;
    rhs_ = (double *) malloc(nr*sizeof(double));


    char dum1;
    double dum2;
    int i;
    for ( i=0; i<nr; i++ ) {
      convertBoundToSense(rowlower_[i],rowupper_[i],dum1,rhs_[i],dum2);
    }
  }
  return rhs_;
}

//------------------------------------------------------------------
// Get the range of rows.
// Length of returned vector is getNumRows();
//------------------------------------------------------------------ 
const double * OsiMpsReader::getRowRange() const
{
  if ( rowrange_==NULL ) {

    int nr=numberRows_;
    rowrange_ = (double *) malloc(nr*sizeof(double));
    std::fill(rowrange_,rowrange_+nr,0.0);

    char dum1;
    double dum2;
    int i;
    for ( i=0; i<nr; i++ ) {
      convertBoundToSense(rowlower_[i],rowupper_[i],dum1,dum2,rowrange_[i]);
    }
  }
  return rowrange_;
}

const double * OsiMpsReader::getObjCoefficients() const
{
  return objective_;
}
 
//------------------------------------------------------------------
// Create a row copy of the matrix ...
//------------------------------------------------------------------
const OsiPackedMatrix * OsiMpsReader::getMatrixByRow() const
{
  if ( matrixByRow_ == NULL && matrixByColumn_) {
    matrixByRow_ = new OsiPackedMatrix(*matrixByColumn_);
    matrixByRow_->reverseOrdering();
  }
  return matrixByRow_;
}

//------------------------------------------------------------------
// Create a column copy of the matrix ...
//------------------------------------------------------------------
const OsiPackedMatrix * OsiMpsReader::getMatrixByCol() const
{
  return matrixByColumn_;
}



//------------------------------------------------------------------
// Return true if column is a continuous, binary, ...
//------------------------------------------------------------------
bool OsiMpsReader::isContinuous(int columnNumber) const
{
  const char * intType = integerType_;
  if ( intType==NULL ) return true;
  assert (columnNumber>=0 && columnNumber < numberColumns_);
  if ( intType[columnNumber]==0 ) return true;
  return false;
}

/* Return true if column is integer.
   Note: This function returns true if the the column
   is binary or a general integer.
*/
bool OsiMpsReader::isInteger(int columnNumber) const
{
  const char * intType = integerType_;
  if ( intType==NULL ) return false;
  assert (columnNumber>=0 && columnNumber < numberColumns_);
  if ( intType[columnNumber]==0 ) return true;
  return false;
}
// if integer
const char * OsiMpsReader::integerColumns() const
{
  return integerType_;
}
// names - returns NULL if out of range
const char * OsiMpsReader::rowName(int index) const
{
  if (index>=0&&index<numberRows_) {
    return names_[0][index];
  } else {
    return NULL;
  }
}
const char * OsiMpsReader::columnName(int index) const
{
  if (index>=0&&index<numberColumns_) {
    return names_[1][index];
  } else {
    return NULL;
  }
}
// names - returns -1 if name not found
int OsiMpsReader::rowIndex(const char * name) const
{
  if (!hash_[0]) {
    if (numberRows_) {
      startHash(0);
    } else {
      return -1;
    }
  }
  return findHash ( name , 0 );
}
    int OsiMpsReader::columnIndex(const char * name) const
{
  if (!hash_[1]) {
    if (numberColumns_) {
      startHash(1);
    } else {
      return -1;
    }
  }
  return findHash ( name , 1 );
}

// Release all row information (lower, upper)
void OsiMpsReader::releaseRowInformation()
{
  free(rowlower_);
  free(rowupper_);
  rowlower_=NULL;
  rowupper_=NULL;
}
// Release all column information (lower, upper, objective)
void OsiMpsReader::releaseColumnInformation()
{
  free(collower_);
  free(colupper_);
  free(objective_);
  collower_=NULL;
  colupper_=NULL;
  objective_=NULL;
}
// Release integer information
void OsiMpsReader::releaseIntegerInformation()
{
  free(integerType_);
  integerType_=NULL;
}
// Release row names
void OsiMpsReader::releaseRowNames()
{
  releaseRedundantInformation();
  int i;
  for (i=0;i<numberHash_[0];i++) {
    free(names_[0][i]);
  }
  free(names_[0]);
  names_[0]=NULL;
  numberHash_[0]=0;
}
// Release column names
void OsiMpsReader::releaseColumnNames()
{
  releaseRedundantInformation();
  int i;
  for (i=0;i<numberHash_[1];i++) {
    free(names_[1][i]);
  }
  free(names_[1]);
  names_[1]=NULL;
  numberHash_[1]=0;
}
// Release matrix information
void OsiMpsReader::releaseMatrixInformation()
{
  releaseRedundantInformation();
  delete matrixByColumn_;
  matrixByColumn_=NULL;
}
  


//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
OsiMpsReader::OsiMpsReader ()
:
rowsense_(NULL),
rhs_(NULL),
rowrange_(NULL),
matrixByRow_(NULL),
matrixByColumn_(NULL),
rowlower_(NULL),
rowupper_(NULL),
collower_(NULL),
colupper_(NULL),
objective_(NULL),
integerType_(NULL),
fileName_(strdup("stdin")),
numberRows_(0),
numberColumns_(0),
numberElements_(0),
defaultBound_(1),
infinity_(DBL_MAX),
objectiveOffset_(0.0),
problemName_(strdup("")),
objectiveName_(strdup("")),
rhsName_(strdup("")),
rangeName_(strdup("")),
boundName_(strdup(""))
{
  numberHash_[0]=0;
  hash_[0]=NULL;
  names_[0]=NULL;
  numberHash_[1]=0;
  hash_[1]=NULL;
  names_[1]=NULL;
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
OsiMpsReader::OsiMpsReader (
                  const OsiMpsReader & source)
:
rowsense_(NULL),
rhs_(NULL),
rowrange_(NULL),
matrixByRow_(NULL),
matrixByColumn_(NULL),
rowlower_(NULL),
rowupper_(NULL),
collower_(NULL),
colupper_(NULL),
objective_(NULL),
integerType_(NULL),
fileName_(strdup("stdin")),
numberRows_(0),
numberColumns_(0),
numberElements_(0),
defaultBound_(1),
infinity_(DBL_MAX),
objectiveOffset_(0.0),
problemName_(strdup("")),
objectiveName_(strdup("")),
rhsName_(strdup("")),
rangeName_(strdup("")),
boundName_(strdup(""))
{
  numberHash_[0]=0;
  hash_[0]=NULL;
  names_[0]=NULL;
  numberHash_[1]=0;
  hash_[1]=NULL;
  names_[1]=NULL;
  if ( source.rowlower_ !=NULL || source.collower_ != NULL) {
    gutsOfCopy(source);
    // OK and proper to leave rowsense_, rhs_, and
    // rowrange_ (also row copy and hash) to NULL.  They will be constructed
    // if they are required.
  }
}

void OsiMpsReader::gutsOfCopy(const OsiMpsReader & rhs)
{
  if (rhs.matrixByColumn_)
    matrixByColumn_=new OsiPackedMatrix(*(rhs.matrixByColumn_));
  numberElements_=rhs.numberElements_;
  numberRows_=rhs.numberRows_;
  numberColumns_=rhs.numberColumns_;
  if (rhs.rowlower_) {
    rowlower_ = (double *) malloc(numberRows_*sizeof(double));
    rowupper_ = (double *) malloc(numberRows_*sizeof(double));
    memcpy(rowlower_,rhs.rowlower_,numberRows_*sizeof(double));
    memcpy(rowupper_,rhs.rowupper_,numberRows_*sizeof(double));
  }
  if (rhs.collower_) {
    collower_ = (double *) malloc(numberColumns_*sizeof(double));
    colupper_ = (double *) malloc(numberColumns_*sizeof(double));
    objective_ = (double *) malloc(numberColumns_*sizeof(double));
    memcpy(collower_,rhs.collower_,numberColumns_*sizeof(double));
    memcpy(colupper_,rhs.colupper_,numberColumns_*sizeof(double));
    memcpy(objective_,rhs.objective_,numberColumns_*sizeof(double));
  }
  if (rhs.integerType_) {
    integerType_ = (char *) malloc (numberColumns_*sizeof(char));
    memcpy(integerType_,rhs.integerType_,numberColumns_*sizeof(char));
  }
  free(fileName_);
  free(problemName_);
  free(objectiveName_);
  free(rhsName_);
  free(rangeName_);
  free(boundName_);
  fileName_ = strdup(rhs.fileName_);
  problemName_ = strdup(rhs.problemName_);
  objectiveName_ = strdup(rhs.objectiveName_);
  rhsName_ = strdup(rhs.rhsName_);
  rangeName_ = strdup(rhs.rangeName_);
  boundName_ = strdup(rhs.boundName_);
  numberHash_[0]=rhs.numberHash_[0];
  numberHash_[1]=rhs.numberHash_[1];
  defaultBound_=rhs.defaultBound_;
  infinity_=rhs.infinity_;
  objectiveOffset_=rhs.objectiveOffset_;
  int section;
  for (section=0;section<2;section++) {
    if (numberHash_[section]) {
      char ** names2 = rhs.names_[section];
      names_[section] = (char **) malloc(numberHash_[section]*
					 sizeof(char *));
      char ** names = names_[section];
      int i;
      for (i=0;i<numberHash_[section];i++) {
	names[i]=strdup(names2[i]);
      }
    }
  }
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
OsiMpsReader::~OsiMpsReader ()
{
  gutsOfDestructor();
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
OsiMpsReader &
OsiMpsReader::operator=(
                   const OsiMpsReader& rhs)
{
  if (this != &rhs) {    
    gutsOfDestructor();
    if ( rhs.rowlower_ !=NULL || rhs.collower_ != NULL) {
      gutsOfCopy(rhs);
    }
  }
  return *this;
}



//-------------------------------------------------------------------
void OsiMpsReader::gutsOfDestructor()
{  
  freeAll();
}


void OsiMpsReader::freeAll()
{  
  releaseRedundantInformation();
  releaseRowNames();
  releaseColumnNames();
  delete matrixByRow_;
  delete matrixByColumn_;
  matrixByRow_=NULL;
  matrixByColumn_=NULL;
  free(rowlower_);
  free(rowupper_);
  free(collower_);
  free(colupper_);
  free(objective_);
  free(integerType_);
  free(fileName_);
  rowlower_=NULL;
  rowupper_=NULL;
  collower_=NULL;
  colupper_=NULL;
  objective_=NULL;
  integerType_=NULL;
  fileName_=NULL;
  free(problemName_);
  free(objectiveName_);
  free(rhsName_);
  free(rangeName_);
  free(boundName_);
  problemName_=NULL;
  objectiveName_=NULL;
  rhsName_=NULL;
  rangeName_=NULL;
  boundName_=NULL;
}

/* Release all information which can be re-calculated e.g. rowsense
    also any row copies OR hash tables for names */
void OsiMpsReader::releaseRedundantInformation()
{  
  free( rowsense_);
  free( rhs_);
  free( rowrange_);
  rowsense_=NULL;
  rhs_=NULL;
  rowrange_=NULL;
  free (hash_[0]);
  free (hash_[1]);
  hash_[0]=0;
  hash_[1]=0;
  delete matrixByRow_;
  matrixByRow_=NULL;
}

//#############################################################################


