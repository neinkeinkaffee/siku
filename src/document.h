#if ! defined _DOCUMENT_H
#define _DOCUMENT_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class document { // data structure for a document
	private:
		int docbuffer[100];	// buffer for getting document characters
		int minstring;
		int wordmax, wordinc;
	
	// data is not hidden so as to maximize speed
	public:											
		document();		// dummy constructor
		//document(char* fname);										// a constructor method
		~document();									// a destructor method
		void add();
		char docname[256];								// an entry for the document name
		unsigned long *pwordhash, *pxwordhash;			// a pointer to the hash-coded word list
		unsigned long *pswordhash;						// a pointer to the sorted hash-coded word list
		char **pwordchar, **pxwordchar;                	// a pointer to the doc-sorted character list
		int *pswordnum;									// a pointer to the sorted word number list
		int words;										// an entry for the number of words in the lists
};

#endif
