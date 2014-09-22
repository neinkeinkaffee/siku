#include "document.h"
#include "util.h"

document::document() {

	minstring = 4;
	wordmax = 1000;
	wordinc = 1000;

	if( (pwordhash =									// allocate array for hash-coded words
		new unsigned long[wordmax]) == NULL )
	{
		printf("Could not allocate enough memory for the word data array.\n");
		util::quitprogram(1);
	}
	
	if( (pwordchar =									// allocate array for hash-coded words
		new char*[wordmax]) == NULL )
	{
		printf("Could not allocate enough memory for the word data array.\n");
		util::quitprogram(1);
	}	
	
	pswordhash=NULL;								// sorted hash-coded word list not allocated yet
	pswordnum=NULL;									// sorted word number list not allocated yet

}

void document::add()					// the constructor method
{	
	FILE *fdocp;
	if((fdocp=fopen(docname,			// open document in binary, readonly mode
		"rb")) == NULL)
	{
		printf("%s%s%s","Cannot open document ", docname, "\n");
		util::quitprogram(1);
	}
	
	// read document and store its content in (a) hash value array pwordhash and (b) character array pwordchar
	int wordcount=0;								// set count of words in document to zero
	int charcount=0;									// set count of characters in word to zero
	unsigned long inhash=0;										// set hash-code to zero
	int docbuffercount=0;								// set number of words in input buffer to zero

	int c = util::docgetchar(fdocp, docbuffer, &docbuffercount, minstring);	
	while (true)
	{
		if (c < 0) break;
		inhash = ((inhash << 7)|(inhash >> 23))^c;
		char *outstring = new char[3];						    // string variable used to output a single character
		charcount = 0;
		outstring[charcount] = (char) c;
		while (true)
		{
			c = util::docgetchar(fdocp, docbuffer, &docbuffercount, minstring);
			charcount++;
			if (util::issignificantbyte(c)) 
			{
				outstring[charcount] = '\0';					// terminate the string
				//printf(" %d \n", inhash);
				break;
			}
			//putchar(c);
			inhash = ((inhash << 7)|(inhash >> 23))^c;            
			outstring[charcount] = (char) c;				// prepare to output that character
		}                
		if (wordcount==wordmax)				// if hash-coded word entries are full
		{
			if( (pxwordhash = new unsigned long[wordmax+wordinc]) == NULL )
			{								// allocate new, larger array of entries
				printf("Could not allocate enough memory for the word data array.\n");
				util::quitprogram(1);
			}
			for(int lcount=0;lcount<wordmax;lcount++)
			{								// loop for all hash-coded word entries
				pxwordhash[lcount]=pwordhash[lcount];
			}								// copy hash-coded word entries to new array
			
			delete [] pwordhash;			// delete old array
			pwordhash=pxwordhash;			// set normal pointer to new, larger array
			pxwordhash=NULL;				// null out temporary pointer
			
			if( (pxwordchar = new char*[wordmax+wordinc]) == NULL )
			{								// allocate new, larger array of entries
				printf("Could not allocate enough memory for the word data array.\n");
				util::quitprogram(1);
			}
			for(int lcount=0;lcount<wordmax;lcount++)
			{								// loop for all hash-coded word entries
				pxwordchar[lcount]=pwordchar[lcount];
			}								// copy hash-coded word entries to new array
			
			delete [] pwordchar;			// delete old array
			pwordchar=pxwordchar;			// set normal pointer to new, larger array
			pxwordchar=NULL;				// null out temporary pointer
			
			wordmax=wordmax+wordinc;		// set maximum to new, larger value
		}
			
		pwordhash[wordcount] = inhash;		// save hash-coded word value
		pwordchar[wordcount] = outstring;   // save character
		wordcount++;						// increment count of words
		charcount=0;						// set count of characters in word to zero
		inhash=0;							// set hash-coded to zero
	}

	words=wordcount;								// save number of words

	if( (pswordhash =				// allocate array for sorted hash-coded words
		new unsigned long[words]) == NULL )
	{
		printf("Could not allocate enough memory for the word data arrays.\n");
		util::quitprogram(1);
	}
	if( (pswordnum =					// allocate array for sorted word numbers
		new int[words]) == NULL )
	{
		printf("Could not allocate enough memory for the word data arrays.\n");
		util::quitprogram(1);
	}
	
	for (int lcount=0;lcount<words;lcount++)			// loop for all the words in the document
	{
		pwordhash[lcount]=			// copy over hash-coded words
			pwordhash[lcount];
		pswordnum[lcount]=			// copy over word numbers
			lcount;
		pswordhash[lcount]=			// copy over hash-coded words
			pwordhash[lcount];
		pwordchar[lcount]=			// copy over hash-coded words
			pwordchar[lcount];		
	}
	util::heapsort(&pswordhash[-1],		// sort hash-coded words (and word numbers)
		&pswordnum[-1],words);
	fclose(fdocp);									// close the document
}

document::~document()								// the destructor method
{
	if( pwordhash != NULL ) delete [] pwordhash;	// if allocated, delete hash-coded word list
	if( pswordhash != NULL ) delete [] pswordhash;	// if allocated, delete sorted hash-coded word list
	if( pswordnum != NULL ) delete [] pswordnum;	// if allocated, delete sorted word number list
	if( pwordchar != NULL ) delete [] pwordchar;
}
