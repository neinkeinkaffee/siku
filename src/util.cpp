#include "util.h"

// Function: docgetchar
// Purpose: Gets the next character from the document
// Details: Skips non-printing characters and underlength text strings that lie between non-printing
//			characters. Converts smart-quotes to ordinary ones. Converts TAB characters to spaces and
//			carriage returns to linefeeds.
// Description: Tries to maintain a buffer of text, LF, space, or EOF characters that is as long as the
//				minimum length specification. It throws out non-printing characters and any text
//				strings that aren;t minlength long. It works by filling the buffer whenever it's empty and
//				keeping it full when possible. But as soon as it encounters a non-printing character, it
//				lets the buffer evaporate to zero length. It then refills it, ignoring text strings that
//				that aren't long enough to fill it completely.


std::string util::basename(std::string const& pathname)
{
    return std::string(std::find_if(pathname.rbegin(), pathname.rend(), MatchPathSeparator()).base(), pathname.end());
}

int util::docgetchar(FILE *fdocp, int *docbuffer, int *docbuffercount, int minstring)
{
	const int TAB=0x9;								// The ASCII TAB character
	const int LF=0xA;								// The ASCII LF character (linefeed)
	const int CR=0xD;								// The ASCII CR character (carriage return)
	const int SPACE=0x20;							// The ASCII SPACED character
	int ch, chout;									// ch (current character), chout (output character)
	if( *docbuffercount==0 )						// if the buffer is empty
	{
		while(*docbuffercount < minstring)			// loop until the buffer is full to the minimum string length
		{
			ch=fgetc(fdocp);						// get the next character from the document
			if( ch==0 )								// possibly a double-byte character,
			{
				ch=fgetc(fdocp);					// so get second byte and continue
			}
			if( ch<0 )								// handle an eof
			{
				docbuffer[*docbuffercount]=ch;		// put the eof in the the buffer
				(*docbuffercount)++;				// increment the number of characters in the buffer
				break;								// don't look for any more characters in the document
			}
//				if( (ch==0x93) || (ch==0x94) ) ch=0x22;	// convert fancy double-quotes to regular double-quotes
//				if( (ch==0x91) || (ch==0x92) ) ch=0x27;	// convert fancy single-quotes to regular single-quotes
//			    if( (ch>0x20) && (ch<=0x7E) )			// handle normal text characters
			if( (ch>0x20) && (ch!=0x7F) && (ch!=0xFF) ) // handle normal text characters, including non-English Characters
			{
				docbuffer[*docbuffercount]=ch;		// store the normal text character in the buffer
				(*docbuffercount)++;				// increment the number of characters in the buffer
			}
			else if( ch==LF )						// handle a linefeed character (end of paragraph)
			{
				docbuffer[*docbuffercount]=LF;		// put the linefeed character in the buffer
				(*docbuffercount)++;				// increment the number of characters in the buffer
			}
			else if( ch==SPACE )					// handle a space character
			{
				docbuffer[*docbuffercount]=SPACE;	// put the space character in the buffer
				(*docbuffercount)++;				// increment the number of characters in the buffer
			}
			else if( ch==TAB )						// handle a tab character
			{
				docbuffer[*docbuffercount]=SPACE;	// put a space character in the buffer instead of a tab
				(*docbuffercount)++;				// increment the number of characters in the buffer
			}
			else if( ch==CR )						// handle a carriage return character
			{
				docbuffer[*docbuffercount]=LF;		// put a linefeed character in the buffer instead of a CR
				(*docbuffercount)++;				// increment the number of characters in the buffer
			}
			else									// handle characters that aren't text characters
			{
				(*docbuffercount)=0;				// truncate the buffer (text string less than minimum)
			}
		}
	}
	if( (*docbuffercount == minstring) && (docbuffer[(*docbuffercount)-1] >= 0 ))	// if the buffer is full and there isn't an eof at the end
	{
		chout=docbuffer[0];							// prepare to return the first character in the buffer
		for(int lcount=0;lcount<minstring-1;lcount++)	// prepare to shift the buffer down by one character
		{
			docbuffer[lcount]=docbuffer[lcount+1];	// move the buffer left one character
		}
		(*docbuffercount)--;						// decrement the number of characters in the buffer
		while( (*docbuffercount < minstring) && (docbuffer[(*docbuffercount)-1] >= 0) )	// loop until the buffer is full to the minimum string length
		{
			ch=fgetc(fdocp);						// get another character from the document
			if( ch==0 )								// possibly a double-byte character,
			{
				ch=fgetc(fdocp);					// so get second byte and continue
			}
//				if( (ch==0x93) || (ch==0x94) ) ch=0x22;		// convert fancy double-quotes to regular double-quotes
//				if( (ch==0x91) || (ch==0x92) ) ch=0x27;		// convert fancy single-quotes to regular single-quotes
//			    if( (ch<0) || ((ch>0x20) && (ch<=0x7E)) ||	// handle eof, text characters, linefeed, or space
			if( (ch<0) || ((ch>0x20) && (ch!=0x7F) && (ch!=0xFF)) || // handle eof, text characters, linefeed, or space
				(ch==LF) || (ch==SPACE) )
			{
				docbuffer[*docbuffercount]=ch;			// put the character in the buffer
				(*docbuffercount)++;					// increment the number of characters in the buffer
			}
			else if( ch==TAB )							// handle a tab character
			{
				docbuffer[*docbuffercount]=SPACE;		// put a space character in the buffer instead of a tab
				(*docbuffercount)++;					// increment the number of characters in the buffer
			}
			else if( ch==CR )							// handle a carriage return character
			{
				docbuffer[*docbuffercount]=LF;			// put a linefeed character in the buffer instead of a CR
				(*docbuffercount)++;					// increment the number of characters in the buffer
			}
			else
			{
				break;
			}
		}
		return chout;								// return the character that was at the front of the buffer
	}
	// if the buffer is only partly full
	else											
	{
		chout=docbuffer[0];							// prepare to return the first character in the buffer
		for(int lcount=0;lcount<minstring-1;lcount++)	// prepare to shift the buffer down by one character
		{
			docbuffer[lcount]=docbuffer[lcount+1];	// move the buffer left one character
		}
		(*docbuffercount)--;						// decrement the number of characters in the buffer
		return chout;								// return the character that was at the front of the buffer
	}
}

bool util::issignificantbyte(int c) {
	if ((c & 0xc0) != 0x80) return 1;
	else return 0;   
}

int util::percentmatching(int firstL, int firstR, int lastL, int lastR, int perfectmatchingwords) {
	return (200 * perfectmatchingwords) / (lastL - firstL + lastR - firstR + 2);
}

bool util::equals(char *a, char *b) {
	int count = 0;
	for (int i=0; i<3; i++)
	{	
		if (a[i] == b[i])
		{
			count++;
		}
	}
	if (count == 3)
	{
		return true;
	}
	else
	{
		return false;
	}
}

// Function: heapsort

// Purpose: Sorts two tables together to put the first table in numerical order

// Details: Sorts both tableA and tableB together, putting tableA in numerical order and taking tableB
//			with it.

// Description: An implementation of the classic heapsort algorithm. The first half of the routine puts
// the heap in order so that the value at the top of each two-branch node is greater than either of the two
// values in the branches below it. The second half of the routine takes values off the top of the heap
// and then resorts the heap so as to fill the opening and promote the remaining values. Ultimately, the
// entire heap is put into ascending numerical order

void util::heapsort(unsigned long *tableA,int *tableB,int n)
{
	
	unsigned long tempA;							// tempA can hold a hash-coded word temporarily
	int tempB;										// tempB can hold a word number temporarily
	int nr;											// number of remaining entries in the heap
	int index1,index2,indexc,indexstart;			// pointers into the heap

	indexstart = (n >> 1);							// start at the lowest two levels of the heap	
	index1 = (n >> 1);								// index1 points to second-to-lowest level of heap
	index2 = (index1 << 1);							// index2 points to lowest level of heap

	for ( indexc=indexstart;indexc>=1;indexc--)		// loop and work backwards to top of heap
	{
		index1 = indexc;							// pointer to the current node
		index2 = (indexc << 1);						// pointer to left branch below node
		
		while ( index2 <= n )						// while we haven't gone off bottom of heap
		{
			if( index2 < n )						// if there are two values branching below this node,
			{
				if( tableA[index2]					// choose the largest value
					< tableA[index2+1] )
				{
					index2++;						// and point to it
				}
			}
			if( tableA[index1] < tableA[index2] )	// if the node is less than the largest value below,
			{
				tempA=tableA[index1];				// swap node and largest value below, part 1
				tempB=tableB[index1];				// swap word number table entry to match, part 1
				tableA[index1]=tableA[index2];		// swap part 2
				tableB[index1]=tableB[index2];		// swap part 2
				tableA[index2]=tempA;				// swap part 3
				tableB[index2]=tempB;				// swap part 3
				index1 = index2;					// pointer to the next lower level of heap
				index2 = (index2 << 1);				// pointer to the left branch below that node
			}
			else									// else, this node is properly ordered, so move along
			{
				break;
			}
		}
	}


	for ( nr=n-1;nr>=1;nr-- )						// start at end of heap and move backward to start
	{
		tempA=tableA[1];							// swap top node and value beyond end of shortened heap, part1
		tempB=tableB[1];							// swap word number table entry to match, part 1
		tableA[1]=tableA[nr+1];						// swap part 2
		tableB[1]=tableB[nr+1];						// swap part 2
		tableA[nr+1]=tempA;							// swap part 3
		tableB[nr+1]=tempB;							// swap part 3

		index1 = 1;									// pointer to the current node (top of heap)
		index2 = 2;									// pointer to left branch below node
		
		while ( index2 <= nr )						// while we haven't gone off bottom of heap
		{
			if( index2 < nr )						// if there are two values branching below this node,
			{
				if( tableA[index2]					// choose the largest value
					< tableA[index2+1] )
				{
					index2++;						// and point to it
				}
			}
			if( tableA[index1] < tableA[index2] )	// if the node is less than the largest value below,
			{
				tempA=tableA[index1];				// swap node and largest value below, part 1
				tempB=tableB[index1];				// swap word number table entry to match, part 1
				tableA[index1]=tableA[index2];		// swap part 2
				tableB[index1]=tableB[index2];		// swap part 2
				tableA[index2]=tempA;				// swap part 3
				tableB[index2]=tempB;				// swap part 3
				index1 = index2;					// pointer to the next lower level of heap

				index2 = (index2 << 1);				// pointer to the left branch below that node
			}
			else									// else, this node is properly ordered, so move along
			{
				break;
			}
		}
	}

	return;											// both tables are all sorted, so return
}

// Function: heapsort 

// Purpose: Sorts one table of pointers to arrays in descending order according to c-th item in array

void util::heapsort(int **table, int n, int primary, int secondary)
{
	
	int *temp;								// temp can hold a pointer to an int array temporarily
	int nr;											// number of remaining entries in the heap
	int index1,index2,indexc,indexstart;			// pointers into the heap

	indexstart = (n >> 1);							// start at the lowest two levels of the heap	
	index1 = (n >> 1);								// index1 points to second-to-lowest level of heap
	index2 = (index1 << 1);							// index2 points to lowest level of heap

	for ( indexc=indexstart;indexc>=1;indexc--)		// loop and work backwards to top of heap
	{
		index1 = indexc;							// pointer to the current node
		index2 = (indexc << 1);						// pointer to left branch below node
		
		while ( index2 <= n )						// while we haven't gone off bottom of heap
		{
			if( index2 < n )						// if there are two values branching below this node,
			{
				//TODO: swap < to > ?
				if( table[index2][primary]					// choose the largest value
					> table[index2+1][primary] )
				{
					//printf("%d < %d, point to former\n", table[index2][0], table[index2+1][0]);
					index2++;						// and point to it
				}
				else if ( (table[index2][primary] == table[index2+1][primary]) && (table[index2][primary] < table[index2+1][primary]) )
				{
					index2++;                       // and point to it
				}
			}
			//TODO: swap < to > ?
			if( table[index1][primary] > table[index2][primary] )	// if the node is less than the largest value below,
			{
				//printf("%d < %d, swap\n", table[index1][0], table[index2][0]);
				temp=table[index1];				// swap node and largest value below, part 1
				table[index1]=table[index2];		// swap part 2
				table[index2]=temp;				// swap part 3
				index1 = index2;					// pointer to the next lower level of heap
				index2 = (index2 << 1);				// pointer to the left branch below that node
			}
			else if ( (table[index1][primary] == table[index2][primary]) && (table[index1][secondary] < table[index2][secondary]))
			{
				//printf("%d < %d, swap\n", table[index1][0], table[index2][0]);
				temp=table[index1];				// swap node and largest value below, part 1
				table[index1]=table[index2];		// swap part 2
				table[index2]=temp;				// swap part 3
				index1 = index2;					// pointer to the next lower level of heap
				index2 = (index2 << 1);				// pointer to the left branch below that node
			}
			else									// else, this node is properly ordered, so move along
			{
				break;
			}
		}
	}


	for ( nr=n-1;nr>=1;nr-- )						// start at end of heap and move backward to start
	{
		temp=table[0];							// swap top node and value beyond end of shortened heap, part1
		table[0]=table[nr+1];						// swap part 2
		table[nr+1]=temp;							// swap part 3

		index1 = 0;									// pointer to the current node (top of heap)
		index2 = 1;									// pointer to left branch below node
		
		while ( index2 <= nr )						// while we haven't gone off bottom of heap
		{
			if( index2 < nr )						// if there are two values branching below this node,
			{
				if( table[index2][primary]					// choose the largest value
					> table[index2+1][primary] )
				{
					index2++;						// and point to it
				}
				else if ( (table[index2][primary] == table[index2+1][primary]) && (table[index2][secondary] < table[index2+1][secondary]) )
				{
					index2++;						// and point to it
				}
			}
			if( table[index1][primary] > table[index2][primary] )	// if the node is less than the largest value below,
			{
				//printf("%d < %d, swap\n", table[index1][c], table[index2][c]);
				temp=table[index1];				// swap node and largest value below, part 1
				table[index1]=table[index2];		// swap part 2
				table[index2]=temp;				// swap part 3
				index1 = index2;					// pointer to the next lower level of heap

				index2 = (index2 << 1);				// pointer to the left branch below that node
			}
			else if ( (table[index1][primary] == table[index2][primary]) && (table[index1][secondary] < table[index2][secondary]))
			{
				//printf("%d < %d, swap\n", table[index1][c], table[index2][c]);
				temp=table[index1];				// swap node and largest value below, part 1
				table[index1]=table[index2];		// swap part 2
				table[index2]=temp;				// swap part 3
				index1 = index2;					// pointer to the next lower level of heap

				index2 = (index2 << 1);				// pointer to the left branch below that node
			}
			else									// else, this node is properly ordered, so move along
			{
				break;
			}
		}
	}

	return;											// both tables are all sorted, so return
}

void util::heapsort(int **table, int n)
{
    heapsort(table, n, 0, 1);
}

void util::heapsort(int **table, int n, int primary)
{
    heapsort(table, n, primary, 1);
}

std::string util::removeExtension(std::string const& filename)
{
    std::string::const_reverse_iterator pivot = std::find(filename.rbegin(), filename.rend(), '.');
    return pivot == filename.rend()
        ? filename
        : std::string(filename.begin(), pivot.base()-1);
}

void util::quitprogram(int value)
{
	printf("%s","Press Enter to Quit Program\n");
	fgetc(stdin);
	exit(value);
}
