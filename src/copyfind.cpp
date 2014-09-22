// copyfind.cpp : Compares the text in a set of documents pairwise, looking for matching phrases.
//
// Version 1.2
//
// Copyright (C) 2002 Louis A. Bloomfieldword
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// Louis A. Bloomfield, Department of Physics, University of Virginia
// 382 McCormick Road, P.O. Box 400714, Charlottesville, VA 22904-4714
// email: bloomfield@virginia.edu
// web site: plagiarism.phys.virginia.edu (see this web site for latest version of copyfind)
//
// If you significantly improve this program, please let me know about it and I
// will consider distributing your version from my web site.
//
//TODO: check "anchors" out
//TODO: think about how to compare one document against corpus of documents
//TODO: replace heapsort by stable sort, to avoid artificial flaws when Chinese characters are repeated??
//TODO: handle character variants and mixed-in traditional/simplified characters

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "document.h"
#include "util.h"

using namespace std;

#define WORD_UNMATCHED -1
#define WORD_PERFECT 0
#define WORD_FLAW 1
#define WORD_FILTERED 2

// Program: copyfind
// Purpose: Searches through a set of documents for pairs that share identical text

// Description: Reads in documents listed in file "doclist.txt", then looks through them pairwise for matching
//				text in phrases of at least a specified minimum length. When it find more than a threshold
//				value of matching text, it generates two html files. These html files contain the text of
//				the two documents, with the matching text underlined. The program also generates a file 
//				"comparisons.txt" that lists the number of matching words in each pair of documents.

// Details:	Reads in the list of documents and dynamically allocates a table of document entries, one for
//			each document. It then reads in each document, converts each word to a 32-bit hash-coded value
//			and dynamically allocates space to store those lists of hash-coded word values. Each document
//			entry ends up with pointers to (1) a sequential list of the hash-coded word values, (2) a sorted
//			list of those same hash-coded word values, and (3) a list of the word numbers associated with
//			the sorted hash-coded word values (so that the program can figure out where each of the
//			sorted hash-coded words actually appears in the document).

//			After it reads in all the documents and creates the hash-coded lists, the program begins to
//			compare documents. It selects two documents, left and right, and goes through their sorted
//			hash-coded word lists, beginning with the first entries for words of more than 3 characters.
//			It ignores words that don't appear in both documents. When it finds words common to both
//			documents, it searches around those words in the normal-order hash-coded word lists to see
//			how long the matching phrases are. If they are long enough, it marks the matches in its lists.
//			The program must treat redundant words carefully--words that appear more than once in either or
//			both documents. For each copy of a redundant word in the left document, it looks through all
//			copies of that redundant word in the right document. In general, the right document's counter
//			changes first and recycles when redundant words are encountered.

//			If the program finds more than the threshold number of matching words in two documents, it
//			generates two html files. It embeds the document text in each file, with html codes to
//			underline the matching text.

bool comp0 (vector<int> i, vector<int> j) { return (i[0] < j[0]); }
bool comp1 (vector<int> i, vector<int> j) { return (i[1] < j[1]); }
bool comp4 (vector<int> i, vector<int> j) { return (i[4] < j[4]); }

class matchfind {
	
		// instance variables
		int D; // total number of documents	
		document *pdoc, *pxdoc;	// normal and temporary document entry pointers
		int docL, docR; // index of left and right document being currently compared
		FILE *fcomp;
		FILE *fhtmlp;
		FILE *fdocp;
		FILE *fparallelsp;
		FILE *fflawsp;
		int *matchL;
		int *matchR;
		int *matchAnchorL;
		int *matchAnchorR;
		vector<vector<int> > matchcandidates; // quadruples of start indices and lengths of matched phrases in both documents, candidates for longest matches
		int matchedwordsL;
		int matchedwordsR;
		int docmax;
		int docinc;
		int wordmax;
		int wordinc;
		int matchmax;
		int matchinc;
		int mismatchtolerance;
		int mismatchpercentage;
		int phraselength;
		int minstring;
		int wordthreshold;
		
		// TODO function for checking length of array and extending it if necessary

	public:
		void docprint(int *match, int *matchAnchor, const char *otherDocFileName, long wordcount);
		void findmatches();
		void findlongestmatch(int matchcount);
		void matchprint();
		void printmatches();
		void startdialog();
		matchfind(char *fname, char *minchars);
};	



matchfind::matchfind(char *fname, char *minchars) {
	int max = 1000; // number of arrays entries allocated
	int inc = 1000; // step size when resizing array
	docmax = max;
	docinc = inc;
	wordmax = max;
	wordinc = inc;
	matchmax = max;
	matchinc = inc;
	mismatchtolerance = 2;
	mismatchpercentage = 80;
	phraselength = atoi(minchars);
	minstring = 4;
	wordthreshold = 10;

	// first step: load documents and hash them
	printf("%s","Loading and Hashing Documents\n");		
	
	// open comparison report file
	if((fcomp=fopen("comparisons.txt", "w")) == NULL)	
	{   
		printf("Cannot open comparisons.txt file.\n");	// if failed, report
		util::quitprogram(1);									// and quit program
	}
	// open list of documents
	FILE *fdocp;
	if((fdocp=fopen(fname, "r")) == NULL)	
	{
		printf("Cannot open doclist.txt file.\n");		// if failed, report
		util::quitprogram(1);									// and quit program
	}
	// allocate the maximum number of document entries
	if((pdoc = new document[docmax]) == NULL)			
	{
		printf("Could not allocate enough memory for the document list.\n");
		util::quitprogram(1);
	}
	
	int doccount=0;											// set count of documents to zero
	
	while(!feof(fdocp))									// loop until the list of documents is finished
	{
		if (doccount==docmax)							// if the document entries are full
		{
			if( (pxdoc =								// allocate a larger array of entries
				new document[docmax+docinc]) == NULL )
			{
				printf("Could not allocate enough memory for the document list.\n");
				util::quitprogram(1);
			}

			for(int lcount=0;lcount<docmax;lcount++)		// loop for all the document entries
			{
				strcpy(pxdoc[lcount].docname,			// copy the document names to new array
					pdoc[lcount].docname);
			}
			
			delete [] pdoc;								// deallocate old array

			pdoc=pxdoc;									// set normal pointer to new, larger array
			pxdoc=NULL;									// null out temporary pointer
			docmax=docmax+docinc;						// set maximum to new, larger value
		}
		char dstring[256];
		fscanf(fdocp,"%255s",dstring);					// read in the next document name
		if(feof(fdocp))									// if eof,
		{
			break;										// stop reading document names
		}
		if(strcmp(dstring,""))							// if not blank document name,
		{
			strcpy(pdoc[doccount].docname, dstring);		// copy document name into document entry
			pdoc[doccount].add();
			doccount++;									// increment count of documents
		}
	}                                                                
	
	D = doccount;
	fclose(fdocp);										// close list of documents	
}	

void matchfind::findmatches() {
	printf("%s","Comparing Documents\n");				// second, compare documents
	int comparecount=0;

	docL = 0;	
	// for all possible right documents		    
	for (docR=1; docR<D; docR++) {			
		if (docR != docL) {
			// allocate match arrays
			matchL = new int[pdoc[docL].words]; // allocate array for left match markers
			if( matchL == NULL )
			{
				printf("Could not allocate enough memory for the word matching arrays.\n");
				util::quitprogram(1);
			}
			matchR = new int[pdoc[docR].words];	// allocate array for right match markers
			if( matchR == NULL )
			{
				printf("Could not allocate enough memory for the word matching arrays.\n");
				util::quitprogram(1);
			}
			// allocate match arrays
			matchAnchorL = new int[pdoc[docL].words]; // allocate array for left match markers
			if( matchAnchorL == NULL )
			{
				printf("Could not allocate enough memory for the word matching arrays.\n");
				util::quitprogram(1);
			}
			matchAnchorR = new int[pdoc[docR].words];	// allocate array for right match markers
			if( matchAnchorR == NULL )
			{
				printf("Could not allocate enough memory for the word matching arrays.\n");
				util::quitprogram(1);
			}
			matchcandidates.clear();	

			// initialize match arrays
			int matchcount = 0;
			int wordcountL, wordcountR;
			wordcountL=0;
			wordcountR=0;
			int wordcountRsave=wordcountR;					// prepare right redundant word pointer
			matchedwordsL=0;							// zero count of left matched words
			matchedwordsR=0;							// zero count of right matched words            
						
			while ( (wordcountL < pdoc[docL].words)		// loop while there are still words to check
				 && (wordcountR < pdoc[docR].words) )
			{
				// check for left word less than right word
				if( pdoc[docL].pswordhash[wordcountL] < pdoc[docR].pswordhash[wordcountR] )
				{
					wordcountL++;						// advance to next left word
					if ( wordcountL >= pdoc[docL].words) break;
					if ( pdoc[docL].pswordhash[wordcountL] == pdoc[docL].pswordhash[wordcountL-1] )
					{
						wordcountR=wordcountRsave;
					}
					else
					{
						wordcountRsave=wordcountR;
					}
					continue;							// and resume looping
				}

				// check for right word less than left word

				if( pdoc[docL].pswordhash[wordcountL] > pdoc[docR].pswordhash[wordcountR] )
				{
					wordcountR++;						// advance to next right word
					wordcountRsave=wordcountR;			// set pointer back to top of redundant words
					continue;							// and resume looping
				}

				// we have a match, so look up and down the hash-coded (not sorted) lists for matches
				
//				matchTempL[wordcountL] = WORD_PERFECT;
//				matchTempR[wordcountR] = WORD_PERFECT;
				//printf("PERFECT MATCH: %s\n", pdoc[docL].pwordchar[pdoc[docL].pswordnum[wordcountL]]);

				int firstL = pdoc[docL].pswordnum[wordcountL]-1; // start left just before current word
				int lastL = pdoc[docL].pswordnum[wordcountL]+1;	// end left just after current word
				int firstR = pdoc[docR].pswordnum[wordcountR]-1; // start right just before current word
				int lastR = pdoc[docR].pswordnum[wordcountR]+1;	// end right just after current word

				int firstLp = firstL+1;						// pointer to first perfect match left
				int firstRp = firstR+1;						// pointer to first perfect match right
				int lastLp = lastL-1;							// pointer to last perfect match left
				int lastRp = lastR-1;							// pointer to last perfect match right
				int matchingwordsperfect = lastLp - firstLp+1;	// save number of perfect matches
			
				int firstLx = firstLp;					// save pointer to word before first perfect match left
				int firstRx = firstRp;					// save pointer to word before first perfect match right
				int lastLx = lastLp;						// save pointer to word after last perfect match left
				int lastRx = lastRp;						// save pointer to word after last perfect match right
				
				
				//printf("\tLook LEFT\n");
				int flaws = 0;
				while( (firstL >= 0) && (firstR >= 0) )	
				{
					// make sure that left and right words haven't been used in a match before and
					// that the two words actually match. If so, move up another word and repeat the test.

//					if( matchL[firstL] != WORD_UNMATCHED ) {
//					    //printf("\t\tBreak, ChL already matched: %s\n", pdoc[docL].pwordchar[firstL]);
//					    break;
//					}
//					if( matchR[firstR] != WORD_UNMATCHED ) {
//					    //printf("\t\tBreak, ChR already matched: %s\n", pdoc[docR].pwordchar[firstR]);
//					    break;
//					}

					// handle perfect match to the left
					if (pdoc[docL].pwordhash[firstL] == pdoc[docR].pwordhash[firstR])
					{
						matchingwordsperfect++;
						flaws = 0;                          // we're back to perfect matching
//						matchTempL[firstL] = WORD_PERFECT;	// markup word in temporary list
//						matchTempR[firstR] = WORD_PERFECT;	// markup word in temporary list
						//printf("\t\tPerfect match on the left: %s\n", pdoc[docL].pwordchar[firstL]);
						firstLp = firstL;					// save pointer to first left perfect match
						firstRp = firstR;					// save pointer to first right perfect match
						firstL--;							// move up on left
						firstR--;							// move up on right
						continue;
					}             

					// we're at a flaw, so increase the flaw count
					flaws++;
					if (flaws > mismatchtolerance) { // check for maximum flaws reached
						//printf("\t\tBreak, maxnum flaws, ChL: %s, ChR: %s\n", pdoc[docL].pwordchar[firstL], pdoc[docR].pwordchar[firstR]);
						break;	
					}        
						
					if ((firstL-1) >= 0)					// check one word earlier in left doc (if it exists), if it matches, we can jump over the flaw
					{
//						if ( matchL[firstL-1] != WORD_UNMATCHED )  { 	// make sure we haven't already matched this word
//						    //printf("\t\tBreak, word to left in docL already matched\n");
//						    break;  
//						}
						
						if (pdoc[docL].pwordhash[firstL-1] == pdoc[docR].pwordhash[firstR])
						{
							if (util::percentmatching(firstL-1, firstR, lastLx, lastRx, matchingwordsperfect+1) < mismatchpercentage) {// are we getting too imperfect?
								//printf("\t\tBreak, maxpercent flaws, ChL: %s, ChR: %s\n", pdoc[docL].pwordchar[firstL-1], pdoc[docR].pwordchar[firstR]);
								break;
							}	
//							matchTempL[firstL] = WORD_FLAW;	// markup non-matching word in left temporary list
							firstL--;						// move up on left to skip over the flaw
							matchingwordsperfect++;			// increment perfect match count;
							flaws = 0;						// having just found a perfect match, we're back to perfect matching
//							matchTempL[firstL] = WORD_PERFECT;		// markup word in left temporary list
//							matchTempR[firstR] = WORD_PERFECT;		// markup word in right temporary list							
							//printf("\t\tJump over flaw in docR: %s, next: %s\n", pdoc[docL].pwordchar[firstL+1], pdoc[docL].pwordchar[firstL]);
							firstLp = firstL;					// save pointer to first left perfect match
							firstRp = firstR;					// save pointer to first right perfect match
							firstL--;						// move up on left
							firstR--;						// move up on right
							continue;
						}

					}
					if ((firstR-1) >= 0 )					// check one word earlier on right (if it exists)
					{
//						if (matchR[firstR-1] != WORD_UNMATCHED) { 	// make sure we haven't already matched this word
//						    //printf("\t\tBreak, word to left in docR already matched\n");
//						    break;  
//						}

						if (pdoc[docL].pwordhash[firstL] == pdoc[docR].pwordhash[firstR-1])
						{
							if (util::percentmatching(firstL, firstR-1, lastLx, lastRx, matchingwordsperfect+1) < mismatchpercentage) {// are we getting too imperfect?
								//printf("\t\tBreak, maxpercent flaws, ChL: %s, ChR: %s\n", pdoc[docL].pwordchar[firstL], pdoc[docR].pwordchar[firstR-1]);
								break;
							}	
//							matchTempR[firstR] = WORD_FLAW;	// markup non-matching word in right temporary list
							firstR--;						// move up on right to skip over the flaw
							matchingwordsperfect++;			// increment perfect match count;
							flaws = 0;						// having just found a perfect match, we're back to perfect matching
//							matchTempL[firstL] = WORD_PERFECT;		// markup word in left temporary list
//							matchTempR[firstR] = WORD_PERFECT;		// markup word in right temporary list							
							//printf("\t\tJump over flaw in docR: %s, next: %s\n", pdoc[docR].pwordchar[firstR+1], pdoc[docR].pwordchar[firstR]);
							firstLp = firstL;					// save pointer to first left perfect match
							firstRp = firstR;					// save pointer to first right perfect match
							firstL--;						// move up on left
							firstR--;						// move up on right
							continue;
						}
					}

					// if we get here, the imperfections are on both sides
					if (util::percentmatching(firstL-1, firstR-1, lastLx, lastRx, matchingwordsperfect) < mismatchpercentage ) {// are we getting too imperfect?
						//printf("\t\tBreak, maxpercent flaws, ChL: %s, ChR: %s\n", pdoc[docL].pwordchar[firstL-1], pdoc[docR].pwordchar[firstR-1]);
						break;
					}	
//					matchTempL[firstL] = WORD_FLAW;		// markup word in left temporary list
//					matchTempR[firstR] = WORD_FLAW;		// markup word in right temporary list
					//("\t\tFlaws to left on both sides, ChL: %s, ChR: %s\n", pdoc[docL].pwordchar[firstL], pdoc[docR].pwordchar[firstR]);
					firstL--;								// move up on left
					firstR--;								// move up on right			
				}
				
				//printf("\tLook RIGHT\n");
				flaws = 0;							// start with zero flaws
				while ((lastL < pdoc[docL].words) && (lastR < pdoc[docR].words)) // if we aren't at the end of either document
				{
				// Note: when we leave this loop, LastL and LastR will always point one word after last match
						
					// make sure that left and right words haven't been used in a match before and
					// that the two words actually match. If so, move up another word and repeat the test.
//    				if (matchL[lastL] != WORD_UNMATCHED) {
//					    //printf("\t\tBreak, ChL already matched: %s\n", pdoc[docL].pwordchar[lastL]);
//					    break;
//					}
//    				if (matchR[lastR] != WORD_UNMATCHED) {
//					    //printf("\t\tBreak, ChR already matched: %s\n", pdoc[docR].pwordchar[lastR]);
//					    break;
//					}
					if (pdoc[docL].pwordhash[lastL] == pdoc[docR].pwordhash[lastR])
					{
						matchingwordsperfect++;				// increment perfect match count;
						flaws = 0;							// having just found a perfect match, we're back to perfect matching
//						matchTempL[lastL] = WORD_PERFECT;	// markup word in temporary list
//						matchTempR[lastR] = WORD_PERFECT;	// markup word in temporary list
						//printf("\t\tPerfect match on the right: %s\n", pdoc[docL].pwordchar[lastL]);
						lastLp = lastL;						// save pointer to last left perfect match
						lastRp = lastR;						// save pointer to last right perfect match
						lastL++;							// move down on left
						lastR++;							// move down on right
						continue;
					}

					flaws++;
					if (flaws == mismatchtolerance) { // check for maximum flaws reached
						//printf("\t\tBreak, maxnum flaws, ChL: %s, ChR: %s\n", pdoc[docL].pwordchar[lastL], pdoc[docR].pwordchar[lastR]);
						break;	
					}    
							
					if ((lastL+1) < pdoc[docL].words)		// check one word later on left (if it exists)
					{
//						if (matchL[lastL+1] != WORD_UNMATCHED )  { 	// make sure we haven't already matched this word
//						    //printf("\t\tBreak, word to right in docL already matched\n");
//						    break;  
//						}
							
						if (pdoc[docL].pwordhash[lastL+1] == pdoc[docR].pwordhash[lastR])
						{
							if (util::percentmatching(firstLx, firstRx, lastL+1, lastR, matchingwordsperfect+1) < mismatchpercentage) {// are we getting too imperfect?
								//printf("\t\tBreak, maxpercent flaws, ChL: %s, ChR: %s\n", pdoc[docL].pwordchar[lastL+1], pdoc[docR].pwordchar[lastR]);
								break;
							}	
//							matchTempL[lastL] = WORD_FLAW;		// markup non-matching word in left temporary list
							lastL++;						// move down on left to skip over the flaw
							matchingwordsperfect++;			// increment perfect match count;
							flaws=0;						// having just found a perfect match, we're back to perfect matching
//							matchTempL[lastL] = WORD_PERFECT;	// markup word in lefttemporary list
//							matchTempR[lastR] = WORD_PERFECT;	// markup word in right temporary list
							//printf("\t\tJump over flaw in docL: %s, next: %s\n", pdoc[docL].pwordchar[lastL-1], pdoc[docL].pwordchar[lastL]);
							lastLp = lastL;					// save pointer to last left perfect match
							lastRp = lastR;					// save pointer to last right perfect match
							lastL++;						// move down on left
							lastR++;						// move down on right
							continue;
						}
					}

					if ((lastR+1) < pdoc[docR].words)	// check one word later on right (if it exists)
					{
//						if (matchR[lastR+1] != WORD_UNMATCHED)   { 	// make sure we haven't already matched this word
//						    //printf("\t\tBreak, word to right in docR already matched\n");
//						    break;  
//						}
						
						if (pdoc[docL].pwordhash[lastL] == pdoc[docR].pwordhash[lastR+1])
						{
							if (util::percentmatching(firstLx, firstRx, lastL, lastR+1, matchingwordsperfect+1) < mismatchpercentage) {// are we getting too imperfect?
								//printf("\t\tBreak, maxpercent flaws, ChL: %s, ChR: %s\n", pdoc[docL].pwordchar[lastL], pdoc[docR].pwordchar[lastR+1]);
								break;
							}	
//							matchTempR[lastR] = WORD_FLAW;		// markup non-matching word in right temporary list
							lastR++;						// move down on right to skip over the flaw
							matchingwordsperfect++;			// increment perfect match count;
							flaws=0;						// having just found a perfect match, we're back to perfect matching
//							matchTempL[lastL] = WORD_PERFECT;	// markup word in left temporary list
//							matchTempR[lastR] = WORD_PERFECT;	// markup word in right temporary list
							//printf("\t\tJump over flaw in docL: %s, next: %s\n", pdoc[docL].pwordchar[lastL], pdoc[docR].pwordchar[lastR]);
							lastLp = lastL;					// save pointer to last left perfect match
							lastRp = lastR;					// save pointer to last right perfect match
							lastL++;						// move down on left
							lastR++;						// move down on right
							continue;
						}
					}

					// if we get here, imperfection is on both sides
					if (util::percentmatching(firstLx, firstRx, lastL+1, lastR+1, matchingwordsperfect) < mismatchpercentage)  {// are we getting too imperfect?
						//printf("\t\tBreak, maxpercent flaws, ChL: %s, ChR: %s\n", pdoc[docL].pwordchar[lastL+1], pdoc[docR].pwordchar[lastR+1]);


						break;
					}	
//					matchTempL[lastL] = WORD_FLAW;		// markup word in left temporary list
//					matchTempR[lastR] = WORD_FLAW;		// markup word in right temporary list
					//printf("\t\tFlaws to right on both sides, ChL: %s, ChR: %s\n", pdoc[docL].pwordchar[lastL], pdoc[docR].pwordchar[lastR]);
					lastL++;								// move down on left
					lastR++;								// move down on right
				}
			
				//TODO: this could also be done before the breaks after failed checks?	
				firstL++;										// correct for overshoot
				firstR++;										// correct for overshoot
				lastL--;										// correct for overshoot
				lastR--;										// correct for overshoot

				if( matchingwordsperfect >= phraselength )
				{
					matchcount++;  
					vector<int> matchcandidate(5);
					matchcandidate[0] = matchingwordsperfect;
					matchcandidate[1] = firstLp;
					matchcandidate[2] = (lastLp-firstLp+1);
					matchcandidate[3] = firstRp; 
					matchcandidate[4] = (lastRp-firstRp+1);
					matchcandidates.push_back(matchcandidate);
					matchedwordsL += lastLp-firstLp+1;			// update count of left matched words
					matchedwordsR += lastRp-firstRp+1;			// update count of right matched words
				}
				wordcountR++;									// skip to next right sorted hash-coded word
			}
		
			//TODO: check if matchcount < some value?
			//TODO: open xml/html-file for docL and docR and create headers	
			
			std::sort(matchcandidates.begin(), matchcandidates.end(), comp0);
            
			findlongestmatch(matchcount);

			//TODO: close html/xml-files with matching fragments for left and right doc

			// report number of matching words in the two documents
			fprintf(fcomp,"%d\t%d\t%s\t%s\n",matchedwordsL,matchedwordsR,pdoc[docL].docname,pdoc[docR].docname);

			//printmatches();

			comparecount++;										// increment count of comparisons

			if( (comparecount%500)	== 0 )						// if count is divisible by 500,
			{
				printf("%s%d%s","\r",comparecount," document comparisons completed");	// report count
			}

			if( matchL != NULL ) delete [] matchL;	// if allocated, delete left match array
			if( matchR != NULL ) delete [] matchR;	// if allocated, delete right match array
			if( matchAnchorL != NULL ) delete [] matchAnchorL;	// if allocated, delete left match array
			if( matchAnchorR != NULL ) delete [] matchAnchorR;	// if allocated, delete right match array
		}
	}
	fclose(fcomp);												// close comparisons file
	printf("%s","\nComparisons Done\n");							// report completion
}

void matchfind::findlongestmatch(int matchcount) { 
    fparallelsp = fopen("parallels.txt", "a");
    fflawsp = fopen("flaws.txt", "w");
    
	for (int wordcountL = 0; wordcountL < pdoc[docL].words; wordcountL++) // loop for all left words				
	{
		matchL[wordcountL] = WORD_UNMATCHED;					// zero the left match markers
		matchAnchorL[wordcountL] = -1;							// zero the left match anchors
	}
	for (int wordcountR = 0; wordcountR < pdoc[docR].words; wordcountR++) // loop for all left words				
	{
		matchR[wordcountR] = WORD_UNMATCHED;					// zero the right match markers
		matchAnchorR[wordcountR] = -1;							// zero the right match anchors;
	}
    vector<vector<int> > parallels; // quadruples of start indices and lengths of matched phrases in both documents, filtered for longest matches
    vector<vector<int> > flaws;
	int anchor = 0; // parallel count
	int flawcount = 0;
	for (int i = 0; i < matchcount; i++)
	{	
		int marked = 0;
		int startL = matchcandidates[i][1];
		int lenL = matchcandidates[i][2];
		int startR = matchcandidates[i][3];
		int lenR = matchcandidates[i][4];
		for (int j = startL; j < startL+lenL; j++)	// continue if parts of match already marked
		{
			if (matchL[j] != WORD_UNMATCHED)
			{
				marked++;
			}
		}	
		if (marked > 0 && lenL/marked < 2) // too much stuff in L already marked
		{
			while (i < matchcount-1 && matchcandidates[i][0] == matchcandidates[i+1][0] && matchcandidates[i][1] == matchcandidates[i+1][1])	// jump over duplicate entries 
			{
				i++;
			}
			continue;
		}
		else
		{
			marked = 0;
			for (int j = startR; j < startR+lenR; j++)
			{
				if (matchR[j] != WORD_UNMATCHED)
				{
					marked++;
				}
			}
			if (marked > 0 && lenR/marked < 2)
			{
				// too much stuff in R already marked
				while (i < matchcount-1 && matchcandidates[i][0] == matchcandidates[i+1][0] && matchcandidates[i][1] == matchcandidates[i+1][1])	// jump over duplicate entries 
				{
					i++;
				}
				continue;
			}
		}
		// print output for construction of parallels database table
        vector<int> parallel(5);
        parallel[0] = startL;
        parallel[1] = startR;
        parallel[2] = lenL;
        parallel[3] = lenR;
        parallel[4] = anchor;
        parallels.push_back(parallel);
		int L = startL;
		int R = startR;
		while(true)		// reconstruct the marks for the match selected above
		{ 
			if (L >= startL+lenL || R >= startR+lenR)
			{
				//TODO: print fragments to html/xml-files for left and right doc
				break;
			}
			if (strcmp(pdoc[docL].pwordchar[L], pdoc[docR].pwordchar[R]) == 0)
			{
//						 printf("%s equals %s\n", pdoc[docL].pwordchar[L], pdoc[docR].pwordchar[R]);
				 matchL[L] = WORD_PERFECT;
				 matchR[R] = WORD_PERFECT;
				 matchAnchorL[L] = anchor;
				 matchAnchorR[R] = anchor;
			}
			else 
			{
				bool casesolved = false;
				for (int inc=1; inc<=2; inc++)
				{
					// more consecutive flaws to jump in right phrase than in left phrase
					for (int inc1=0; inc1<inc; inc1++)
					{
						if (L < startL+lenL-1-inc1 && R < startR+lenR-1-inc && strcmp(pdoc[docL].pwordchar[L+inc1], pdoc[docR].pwordchar[R+inc]) == 0)	
						{
							for (int idx = 0; idx < inc1; idx++)
							{
								matchL[L+idx] = WORD_FLAW;
								matchAnchorL[L+idx] = anchor;
//								fprintf(fflawsp, "%d %d %d %d\n", docL, docR, startL, L+idx);
                                vector<int> flaw(3);                             
                                flaw[0] = anchor;
                                flaw[1] = 0;
                                flaw[2] = L+idx;
                                flaws.push_back(flaw);
                                flawcount++;
							}
							matchL[L+inc1] = WORD_PERFECT;
							matchAnchorL[L+inc1] = anchor;
							for (int idx = 0; idx < inc; idx++)
							{
								matchR[R+idx] = WORD_FLAW;
								matchAnchorR[R+idx] = anchor; 
//								fprintf(fflawsp, "%d %d %d %d\n", docR, docL, startR, R+idx);
								vector<int> flaw(3);
                                flaw[0] = anchor;
                                flaw[1] = 1;
                                flaw[2] = R+idx;
                                flaws.push_back(flaw);
                                flawcount++;
							}							
							matchR[R+inc] = WORD_PERFECT;
							matchAnchorR[R+inc] = anchor;
							L += inc1;
							R += inc;
							casesolved = true;
							break;
						}
					}
					if (casesolved) break;
					else
					{
						// more consecutive flaws to jump in left phrase than in right phrase
						for (int inc1=0; inc1<inc; inc1++)
						{
							if (L < startL+lenL-1-inc && R < startR+lenR-1-inc1 && strcmp(pdoc[docL].pwordchar[L+inc], pdoc[docR].pwordchar[R+inc1]) == 0)
							{
								for (int idx = 0; idx < inc; idx++)
								{
									matchL[L+idx] = WORD_FLAW;
									matchAnchorL[L+idx] = anchor;
//								    fprintf(fflawsp, "%d %d %d %d\n", docL, docR, startL, L+idx);
								    vector<int> flaw(3);
                                    flaw[0] = anchor;
                                    flaw[1] = 0;
                                    flaw[2] = L+idx;
                                    flaws.push_back(flaw);
                                    flawcount++;
								}
								matchL[L+inc] = WORD_PERFECT;	
								matchAnchorL[L+inc] = anchor;
								for (int idx = 0; idx < inc1; idx++)
								{
									matchR[R+idx] = WORD_FLAW;
									matchAnchorR[R+idx] = anchor;
//								    fprintf(fflawsp, "%d %d %d %d\n", docR, docL, startR, R+idx);
								    vector<int> flaw(3);
                                    flaw[0] = anchor;
                                    flaw[1] = 1;
                                    flaw[2] = R+idx;
                                    flaws.push_back(flaw);
                                    flawcount++;
								}
								matchR[R+inc1] = WORD_PERFECT;
								matchAnchorR[R+inc1] = anchor;
								L += inc;
								R += inc1;
								casesolved = true;
								break;
							}
						}
						if (casesolved) break;
						else
						{
							// same number of flaws to jump in left and right phrase
							if (L < startL+lenL-1-inc && R < startR+lenR-1-inc && strcmp(pdoc[docL].pwordchar[L+inc], pdoc[docR].pwordchar[R+inc]) == 0)
							{
								for (int idx = 0; idx < inc; idx++)
								{
									matchL[L+idx] = WORD_FLAW;
									matchAnchorL[L+idx] = anchor;
//								    fprintf(fflawsp, "%d %d %d %d\n", docL, docR, startL, L+idx);
								    vector<int> flawL(3);
                                    flawL[0] = anchor;
                                    flawL[1] = 0;
                                    flawL[2] = L+idx;
	                                flaws.push_back(flawL);
                                    flawcount++;
									matchR[R+idx] = WORD_FLAW;
									matchAnchorR[R+idx] = anchor;
//								    fprintf(fflawsp, "%d %d %d %d\n", docR, docL, startR, R+idx);
								    vector<int> flawR(3);
                                    flawR[0] = anchor;
                                    flawR[1] = 1;
                                    flawR[2] = R+idx;
	                                flaws.push_back(flawR);
                                    flawcount++;
								}
								matchL[L+inc] = WORD_PERFECT;
								matchAnchorL[L+inc] = anchor;
								matchR[R+inc] = WORD_PERFECT;
								matchAnchorR[R+inc] = anchor;
								L += inc;
								R += inc;
								casesolved = true;
								break;
							}
							else
							{
								matchL[L] = WORD_FLAW;
								matchAnchorL[L] = anchor;
//								fprintf(fflawsp, "%d %d %d %d\n", docL, docR, startL, L);
								vector<int> flawL(3);
                                flawL[0] = anchor;
                                flawL[1] = 0;
                                flawL[2] = L;
	                            flaws.push_back(flawL);
                                flawcount++;
								matchR[R] = WORD_FLAW;
								matchAnchorR[R] = anchor;
//								fprintf(fflawsp, "%d %d %d %d\n", docR, docL, startR, R);
								vector<int> flawR(3);
                                flawR[0] = anchor;
                                flawR[1] = 1;
                                flawR[2] = R;
	                            flaws.push_back(flawR);
                                flawcount++;
							}
						}
					}
				}	
			}
			L++;
			R++;
		}
		anchor++;
		// jump over duplicate entries 
		while (i < matchcount-1 && matchcandidates[i][0] == matchcandidates[i+1][0] && matchcandidates[i][1] == matchcandidates[i+1][1])	
		{
			i++;
		}
	}
	// heapsort and adjust overlaps on left and right
	// have to pass anchor-1, as heapsort routine takes largest index instead of array size
	// also note that this variant of heapsort sort in descending order
	// (couldn't be bothered to provide for both ascending and descending, this is TODO)
//	util::heapsort(parallels, anchor-1, 0);
    std::sort(parallels.begin(), parallels.end(), comp0);
	// first adjust overlaps for left doc
	// TODO: find out why this sorts ascending, should be descending
	for (int i = 1; i < anchor; i++)
	{
	    // if end index of current match is greater than start index of match ahead
	    // startL = parallels[i][0], lenL = parallels[i][2]
	    if (parallels[i-1][0] + parallels[i-1][2] > parallels[i][0])
	    {
	    	int diff = (parallels[i-1][0] + parallels[i-1][2] - parallels[i][0]);
	        // move start index of match ahead one to the right
	        parallels[i][0] += diff;
	        // reduce length of match ahead by one
	        parallels[i][2] -= diff;
	    }
	}
	// then adjust overlaps for right doc
    std::sort(parallels.begin(), parallels.end(), comp1);
	for (int i = 1; i < anchor; i++)
	{
	    // if end index of current match is greater than end index of match ahead
	    // startR = parallels[i][1], lenR = parallels[i][3]
	    if (parallels[i-1][1] + parallels[i-1][3] > parallels[i][1] )
	    {
	        int diff = (parallels[i-1][1] + parallels[i-1][3] - parallels[i][1]);
	        // move start index of match ahead one to the right
	        parallels[i][1] += diff;
	        // reduce length of match ahead by one
	        parallels[i][3] -= diff;
	    }
	}
	for (int i = 0; i < anchor; i++)
	{
//	    fprintf(fparallelsp, "%d %d %d %d %d %d\n", docL, docR, parallels[i][0], parallels[i][1], parallels[i][2], parallels[i][3]);
        int startL = parallels[i][0];
        int endL = parallels[i][0] + parallels[i][2] - 1;
        int startR = parallels[i][1];
        int endR = parallels[i][1] + parallels[i][3] - 1;
        string phraseL, phraseR;
        for (int k = startL; k < endL; k++) 
        {
            if (strcmp(pdoc[docL].pwordchar[k], "\n") != 0)
            {
                phraseL += pdoc[docL].pwordchar[k];
            }
        }
        for (int k = startR; k < endR; k++) 
        {
            if (strcmp(pdoc[docR].pwordchar[k], "\n") != 0)
            {
                phraseR += pdoc[docR].pwordchar[k];
            }
        }
        fprintf(fparallelsp, "%s\t%d\t%d\t%s\t%d\t%d\t%s\n", pdoc[docR].docname, startR, endR, phraseR.c_str(), startL, endL, phraseL.c_str());
	}
//	std::sort(parallels.begin(), parallels.end(), comp4);
//	for (int i = 0; i < flawcount; i++)
//	{
//	    if (flaws[i][1] == 0 && flaws[i][2] >= parallels[flaws[i][0]][0]) // if flaw is in left doc
//	    {
//	        fprintf(fflawsp, "%d %d %d %d %d\n", docL, docR, parallels[flaws[i][0]][0], flaws[i][2], flaws[i][1]);
//	    }
//	    else if ( flaws[i][2] >= parallels[flaws[i][0]][1]) // flaw is in right doc
//	    {
//	        fprintf(fflawsp, "%d %d %d %d %d\n", docR, docL, parallels[flaws[i][0]][1], flaws[i][2], flaws[i][1]);
//	    }	    
//	}
    fclose(fparallelsp);
}



// Function: docprint

// Purpose: Places the body of the document into the html file that is being generated

// Details: Reads the document in just as it was read in while generating the list of hashed words.
//			Each word is copied to the html file, but with underlining and paragraph marks inserted where
//			they are appropriate.

// Description: Tries to produce a readable html file body that has matching words underlined and that breaks
//				paragraphs where appropriate.

void matchfind::docprint(int *match, int *matchAnchor, const char *otherDocFileName, long words)
{
	int c;											// current character value
	int docbuffercount=0;							// number of characters in the character buffer
	int docbuffer[100];								// the character buffer
	//const int LF=0xA;								// ASCII LF character (line feed)
	int xmatch;
	int xanchor;
	int lastmatch = WORD_UNMATCHED;
	int lastanchor = 0;
	//int uline=0;									// underlining is not active

	c = util::docgetchar(fdocp, docbuffer, &docbuffercount, minstring);
	for(int wordcount=0; wordcount<words; wordcount++)	// loop for every word
	{
		xmatch = match[wordcount];
		xanchor = matchAnchor[wordcount];

		if ((lastmatch != xmatch) || (lastanchor != xanchor))	// check for a change of markup or anchor
		{
			if (lastmatch == WORD_PERFECT) fprintf(fhtmlp, "%s", "</font>");	// close out red markups if they were active
			else if(lastmatch == WORD_FLAW) fprintf(fhtmlp, "%s", "</font></i>");	// close out green italics if they were active
			else if(lastmatch == WORD_FILTERED) fprintf(fhtmlp, "%s", "</font>");	// close out blue markups if they were active

			if (lastanchor != xanchor)
			{
				if (lastanchor >= 0)
				{
					fprintf(fhtmlp, "%s", "</a>"); // close out any active anchor
					//lastanchor = 0;
				}
				if (xanchor >= 0)
				{
					fprintf(fhtmlp, "<a name='%i', href='%s#%i'>", matchAnchor[wordcount], otherDocFileName, matchAnchor[wordcount]);
				}
			}

			if(xmatch == WORD_PERFECT) fprintf(fhtmlp, "%s", "<font color='#FF0000'>");	// start red for perfection
			else if(xmatch == WORD_FLAW) fprintf(fhtmlp, "%s", "<i><font color='#007F00'>");	// start green italics for imperfection
			else if(xmatch == WORD_FILTERED) fprintf(fhtmlp, "%s", "<font color='#0000FF'>");	// start blue for filtered
		}

		char outstring[3];						    // string variable used to output a single character
		int charcount = 0;
		outstring[charcount] = (char) c;
		while (true)
		{
			c = util::docgetchar(fdocp, docbuffer, &docbuffercount, minstring);
			charcount++;
			if (util::issignificantbyte(c))
			{
				outstring[charcount] = '\0';					// terminate the string
				fprintf(fhtmlp, "%s", outstring);		// print the character to the html file
				break;
			}
			outstring[charcount] = (char) c;				// prepare to output that character
		}
		
		/*if (xmatch == WORD_FLAW) //printf("FLAW: %s\n", outstring);
		else if (xmatch == WORD_PERFECT) //printf("PERFECT: %s\n", outstring);
		else if (xmatch == WORD_UNMATCHED) //printf("UNMATCHED: %s\n", outstring);*/
		lastmatch = xmatch;
		lastanchor = xanchor;
		if (strcmp(outstring, "\n") == 0)						// have we hit a linefeed?
			fprintf(fhtmlp,"%s","</br>");
//			fprintf(fhtmlp,"%s","</P>\n<P>");	// if so, insert a paragraph boundary into the html file
//			if( charcount>0 ) break;			// if we had a word going, break to begin the next word
//		}
//		else if( charcount>0 )					// have we hit a nonprinting character while in a word?
//		{
//			fprintf(fhtmlp,"%s"," ");			// if so, insert a blank into the html file
//			break;								// break to begin the next word
//		}	*/									// if we aren't in a word and hit a nonprinting character, ignore it
	}
	if (lastmatch == WORD_PERFECT) 
		fprintf(fhtmlp, "%s", "</font>");	// close out red markups if they were active
	else if(lastmatch == WORD_FLAW) 
		fprintf(fhtmlp, "%s", "</font></i>");	// close out green italics if they were active
	else if(lastmatch == WORD_FILTERED) 
		fprintf(fhtmlp, "%s", "</font>");	// close out blue markups if they were active
}

void matchfind::printmatches() {
	//TODO: fix matchedwords counting			
	if(matchedwordsL>=wordthreshold) // if there are enough matches to report,
	{
		string docLFileName(pdoc[docR].docname);
		string docRFileName(pdoc[docL].docname);
		// assemble html file names
		docLFileName = util::basename(docLFileName);
		docLFileName = util::removeExtension(docLFileName);
		docRFileName = util::basename(docRFileName);
		docRFileName = util::removeExtension(docRFileName);
		string docLhtmlFileName = docLFileName + "_" + docRFileName + ".html";
		string docRhtmlFileName = docRFileName + "_" + docLFileName + ".html";

		if((fhtmlp=fopen(docLhtmlFileName.c_str(),"w")) == NULL) // open html file
		{
			printf("Cannot open file %s\n", docLhtmlFileName.c_str());
			util::quitprogram(1);
		}                

		// create header for html file
		fprintf(fhtmlp,"%s%s%s%s%s%d%s","<html><title>Markup of ",pdoc[docL].docname,
			" for Comparison with ",pdoc[docR].docname,
			" (Matched Words = ",matchedwordsL,")</title><body>\n");

		if((fdocp=fopen(pdoc[docL].docname, "rb")) == NULL)	// open left document
		{
			printf("%s%d%s%s%s","Cannot open document #",docL," = ",pdoc[docL].docname,"\n");
			util::quitprogram(1);

		}
		
		// generate text body of html file, with matching words underlined

		docprint(matchL, matchAnchorL, docRhtmlFileName.c_str(), pdoc[docL].words);

		fclose(fdocp);									// close document

		fprintf(fhtmlp,"%s","\n</html></body>\n");		// complete html file
		
		fclose(fhtmlp);									// close html file

		//TODO: produce same output for right file /*

		if((fhtmlp=fopen(docRhtmlFileName.c_str(),"w")) == NULL)			// open html file
		{
			printf("Cannot open file %s \n",docRhtmlFileName.c_str());	// if failed, report
			util::quitprogram(1);								// and quit program
		}

		// create header material for html file

		fprintf(fhtmlp,"%s%s%s%s%s%d%s","<html><title>Markup of ",pdoc[docR].docname,
			" for Comparison with ",pdoc[docL].docname,
			" (Matched Words = ",matchedwordsL,")</title><body>\n");

		if((fdocp=fopen(pdoc[docR].docname, "rb")) == NULL)	// open right document
		{
			printf("%s%d%s%s%s","Cannot open document #",docR," = ",pdoc[docR].docname,"\n");
			util::quitprogram(1);								// if failed, report and quit
		}
		
		// generate text body of html file, with matching words underlined

		docprint(matchR, matchAnchorR, docLhtmlFileName.c_str(), pdoc[docR].words); 

		fclose(fdocp);									// close document

		fprintf(fhtmlp,"%s","\n</html></body>\n");		// complete html file
		
		fclose(fhtmlp);									// close html file 
		//TODO: */
	}
}

int main(int argc, char* argv[]) { // main program
	matchfind *mf = new matchfind(argv[1], argv[2]);
	mf->findmatches();
	delete mf;
	return 0;
}
