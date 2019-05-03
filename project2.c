#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

struct block {
	char proc[50];
	int start; 
	int size;
};

struct buddy {
	struct block proc;
	int memSize;
	int isAllocated; //-1 = free, 0=Not allocatable, 1=Allocated
};

struct buddy* buddyMem;
int memSize;
int numBuddy = 0;
int lastSearch = 0;
int numProc;
int startFinder = 0;
int printedAssigned = 0;
int printedAvail = 0;
int availStart = 0;
int found = 0;
int availEnd = 0;
int currentSmallest = INT_MAX;
int smallestSpace = -1;
int lastStatus = 0; //Set to zero at first to indicate not ran
//struct block* locations; //Keeps a list of processes and their start/end values ie (A, 
char* type; //Gets the type of memory management like BESTFIT, NEXTFIT etc.
struct block* mem; //Total possible amount of processes.
int main(int argc, char* argv[])
{
	if (argc == 4) //3 args, accept
	{
		//GET LAUNCH ARGUMENTS

		//Get memory size
		memSize = atoi(argv[2]);
		type = argv[1]; 
		
		//Create our buddy system if we need to
		if (strcmp(type, "BUDDY") == 0)
		{
			buddyMem = malloc(sizeof(struct buddy));
			buddyMem[1].memSize = memSize;
			buddyMem[1].isAllocated = -1;
			numBuddy++;
		}

		//Allocate Memory
		mem = (struct block*)malloc(2048 * sizeof(struct block));

		FILE *file; 
		file = fopen(argv[3], "r");
		char temp[65635];
		char contents[256][256];
		int iterator = 0;
		while (fgets(&temp, 65635, file)) //Read the file 
		{
			if (temp[0] != '#') //Drop comments and store the lines in an array
			{
				if (strstr(temp, "REQUEST"))
				{
					//Grab the process name and the size
					char* token;
					token = strtok(temp, " ");
					char* proc = strtok(NULL, " ");
					int size = atoi(strtok(NULL, " "));	
					struct block process;
					strcpy(process.proc, proc);
					process.size = size;
					request(process);
				}
				else if (strstr(temp, "RELEASE"))
				{
					char* token;
					token = strtok(temp, " ");
					char* proc = strtok(NULL, " ");
					proc = strtok(proc, "\n");
					if (strcmp(type, "BUDDY") == 0)
						releaseBuddy(proc);
					else
						release(proc);
				}
				else if (strstr(temp, "LIST AVAILABLE"))
				{
					if (strcmp(type, "BUDDY") == 0)
						listAvailableBuddy();
					else
						listAvailable();
				}
				else if (strstr(temp, "LIST ASSIGNED"))
				{
					if (strcmp(type, "BUDDY") == 0)
						listAssignedBuddy();
					else
						listAssigned();
				}
				else if (strstr(temp, "FIND"))
				{
					char* token;
					token = strtok(temp, " ");
					char* proc = strtok(NULL, " ");
					proc = strtok(proc, "\n");

					if (strcmp(type, "BUDDY") == 0)
						findBuddy(proc);
					else
						find(proc);
				}

				//strcpy(contents[iterator], &temp);
				//iterator++;
			}
		}
	}
	else { //Otherwise, reject
		printf("Error: incorrect arguments. Usage: ./project2 [replacement type] [total memory allocation] file\n");
	}
	free(mem);
	free(buddyMem);
	return 0;
}

int memoryManager(char* type, struct block proc) //Returns relative start
{
	if (strcmp(type, "BESTFIT") == 0)
		return bestFit(proc);
	else if (strcmp(type, "FIRSTFIT") == 0)
		return firstFit(proc);
	else if (strcmp(type, "NEXTFIT") == 0)
		return nextFit(proc);
	else if (strcmp(type, "BUDDY") == 0)
		return buddy(proc);
}



int bestFit(struct block proc)
{
	int bestDiff = INT_MAX; //Best difference between empty space and proc size. Smaller (above 0) is better. 0 is best.
	int bestStart = -1;
	if (numProc == 0) //if nothing is in memory 
	{
		add(proc, 0); //add to start of memory
		return 0; //Get me out of here!
	}
	if ((mem[0].start > proc.size) && mem[0].start != 0) //if there's enough space to fit between 0 and our first process
	{
			bestDiff = mem[0].start - proc.size; //New best diff
			bestStart = 0; //new start
			if (bestDiff == 0) //This is the best possible fit
			{
				add(proc, bestStart);
				return bestStart;
			}
	}
	for (int i = 0; i < numProc - 1; i++) //Try to fit between all our processes
	{
		int diff = (mem[i + 1].start - (mem[i].start+mem[i].size)) - proc.size; //Get the diff between the start of p2 and end of p1 and our proc size
		if (diff < bestDiff && diff >= 0) //If the new diff is better than the last and we can fit
		{
			bestDiff = diff; //Update bestDiff 
			bestStart = mem[i].start + mem[i].size; //new best start is right after the last proc
			if (bestDiff == 0) //This is the bestDiff possible
			{
				add(proc, bestStart);
				return bestStart;
			}
		}
	}
	int diff = memSize - (mem[numProc - 1].start + mem[numProc - 1].size) - proc.size;
	if (diff < bestDiff && diff >= 0) //If there's space at the end better than our current difference and we can fit
	{
		add(proc, mem[numProc - 1].start + mem[numProc - 1].size); //Add after the last processes. There is no better fit than the last one if we satisfied the if condition above
		return mem[numProc - 1].start + mem[numProc - 1].size;
	}
	if (bestStart != INT_MAX) //If we had a match at some point
	{
		add(proc, bestStart);
		return bestStart;
	}
	printf("FAIL REQUEST %s %d\n", proc.proc, proc.size);
	return -1;
}

//Finds first location to fit block into.
int firstFit(struct block proc)
{
	if (numProc == 0) //if nothing is in memory 
	{
		add(proc, 0); //add to start of memory
		return 0; //Get me out of here!
	}	
	if ((mem[0].start > proc.size) && mem[0].start != 0) //if there's enough space to fit between 0 and our first process and our first process isn't at zero
	{
		add(proc, 0); //Add to start of memory
		return 0; //Get me out of here!
	}
	for (int i = 0; i < numProc-1; i++) //Try to fit between all our processes
	{
		int diff = mem[i + 1].start -  (mem[i].start+mem[i].size); //Get the diff between the start of p2 and end of p1.
		if (diff > proc.size) //If the difference (free space) is more than the total size needed for allocation
		{
			add(proc, mem[i].size + mem[i].start);
			return  mem[i].size + mem[i].start;
		}
	}
	if (memSize - mem[numProc - 1].start+mem[numProc-1].size > proc.size) //If there's space at the end
	{
		add(proc, mem[numProc - 1].start + mem[numProc - 1].size); //Add after the last processes. 
		return mem[numProc - 1].start + mem[numProc - 1].size;
	}
	printf("FAIL REQUEST %s %d\n", proc.proc, proc.size);
	return -1;
}

int nextFit(struct block proc)
{
	if (numProc == 0) //if nothing is in memory 
	{
		add(proc, lastSearch); //add to start of memory
		lastSearch = proc.size + lastSearch;
		return lastSearch; //Get me out of here!
	}
	if ((mem[0].start-lastSearch >= proc.size) && lastSearch<mem[0].start) //if there's enough space to fit between lastSearch and our first process and our first process isn't at zero
	{
		add(proc, lastSearch); //Add to start of memory
		lastSearch = lastSearch + proc.size;
		return lastSearch; //Get me out of here!
	}
	for (int i = 0; i < numProc - 1; i++) //Try to fit between all our processes
	{
		int diff = mem[i + 1].start - (mem[i].start + mem[i].size); //Get the diff between the start of p2 and end of p1.
		if (diff > proc.size && lastSearch>=mem[i].size+mem[i].start) //If the difference (free space) is more than the total size needed for allocation and the start of the space is after (or at) last search
		{
			add(proc, mem[i].size + mem[i].start);
			lastSearch = mem[i].size + mem[i].start;
			return lastSearch;
		}
	}
	if (memSize - mem[numProc - 1].start + mem[numProc - 1].size > proc.size && lastSearch>=mem[numProc - 1].start + mem[numProc - 1].size) //If there's space at the end and lastsearch is in that region
	{
		add(proc, mem[numProc - 1].start + mem[numProc - 1].size); //Add after the last processes. 
		lastSearch = mem[numProc - 1].start + mem[numProc - 1].size;
		return lastSearch;
	}
	else if (lastSearch >= mem[numProc - 1].start + mem[numProc - 1].size) //If last search was in the last free block but failed to allocate
	{
		lastSearch = firstFit(proc); //Just run first fit and return the value as our last search.
		if (lastSearch == -1)
		{
			printf("FAIL REQUEST %s %d\n", proc.proc, proc.size);
			lastSearch = 0;
			return -1;
		}
	}
	//If we make it all the way to the end then no spaces were ever found. 
	printf("FAIL REQUEST %s %d\n", proc.proc, proc.size);
	lastSearch = 0;
	return -1;
}

int buddy(struct block proc)
{
	return findSpace(proc);
}

void request(struct block proc) //request n blocks for process A
{
	int result = memoryManager(type, proc);
	if (result !=-1) //If not a failure
	{
		printf("ALLOCATED %s %d\n", proc.proc, result);
	}
}

void release(char* proc) //release process A's blocks 
{
	if (type == "BUDDY")
	{
		releaseBuddy(proc);
		return;
	}
	for (int i = 0; i < numProc; i++)
	{
		if (strcmp(mem[i].proc, proc) == 0)
		{
			printf("FREE %s %d %d)\n", mem[i].proc, mem[i].size, mem[i].start);
			if (i != numProc - 1) //If not the last one
			{
				for (int j = i; j < numProc - 1; j++) //Start at our removal index
				{
					memcpy(&mem[j], &mem[j + 1], sizeof(struct block)); //Take the stuff to the right and move it over to overwrite the value
				}
			}
			numProc--; //Decrease to total number of Proc
			return;
		}
	}
	printf("FAIL RELEASE %s\n", proc);
}

//List available blocks as (n, start)
void listAvailable()
{
	int printed = -1;
	if (numProc == 0)
	{
		printf("$(%d, %d)$\n", 0, memSize);
		return;
	}
	if (mem[0].start != 0) //If 0 is not allocated
	{
		printf("$(%d, %d)$ ", 0, mem[0].start);
		printed = 0;
	}
	for (int i = 0; i < numProc-1; i++)
	{
		if (mem[i + 1].start - (mem[i].start + mem[i].size) != 0)
		{
			printf("$(%d, %d)$ ", mem[i + 1].start - (mem[i].start + mem[i].size)); //Start of next allocation minus the end of the last one = free space
			printed = 0;
		}
	}
	if (memSize - (mem[numProc - 1].start + mem[numProc - 1].size) != 0) //If there's space at the end
	{
		printf("$(%d, %d)$", (mem[numProc - 1].start + mem[numProc - 1].size), memSize);
		printed = 0;
	}
	if(printed==-1)
		printf("FULL\n");
	else
		printf("\n");
}

void listAssigned() //list assigned blocks as (A, n, start)
{
	if (numProc != 0)
	{
		for (int i = 0; i < numProc; i++)
			printf("$(%s, %d, %d)$ ", mem[i].proc, mem[i].size, mem[i].start);
		printf("\n");
		return;
	}
	printf("NONE\n");

}

void find(char* name) //Find the size and start of name as (name, n, start)
{
	for (int i = 0; i < numProc; i++)
	{
		if (strcmp(name, mem[i].proc) == 0)
		{
			printf("$(%s,%d, %d)$\n", mem[i].proc, mem[i].size, mem[i].start);
			return;
		}
	}
	printf("FAULT\n");
}

int compareStart(const void *b1, const void *b2) //Compare two staring values p1 - p2. Used for sorting
{
	const struct block *proc1 = b1;
	const struct block *proc2 = b2;
	return proc1->start - proc2->start;
}

void add(struct block proc, int start) //Adds a process to a starting location
{ //No error checking for starting in a spot to small is done here. That all should be done BEFORE this method is called 
	proc.start = start;
	struct block test;
	memcpy(&(mem[numProc]), &proc, sizeof(struct block)); //append proc to list
	qsort(mem, numProc, sizeof(mem[0]), compareStart); //Sort them by their starting position
	numProc++;
}

void split(int index) //Splits block into two
{
	int currentSize = buddyMem[index].memSize;
	if (currentSize == 1) //We can't divide any further.
		return;
	if (index * 2 >= numBuddy) //If we're at the bottom of the tree we need to double our memory
	{
		buddyMem = realloc(buddyMem, 2 * sizeof(buddyMem) + sizeof(buddyMem));
		numBuddy *= 2; //Increase number of buddy blocks by its theoretical growth.
		numBuddy += 1; //Add one for the right child node (2n+1)
	}
	buddyMem[index].isAllocated = 0; //Mark as parent node. That is, unallocatable
	buddyMem[2 * index].isAllocated = -1; //Open child node
	buddyMem[2 * index].memSize = currentSize / 2; //Split memory size
	buddyMem[2 * index + 1].isAllocated = -1; //Open right child
	buddyMem[2 * index + 1].memSize = currentSize / 2; //Split memory size


}

int merge(int lchild, int rchild) //Merge if possible. Return if it merged or not.
{
	if (buddyMem[lchild].isAllocated == -1 && buddyMem[rchild].isAllocated == -1) //If both are empty
	{
		buddyMem[lchild].isAllocated = 0; //delete lchild
		buddyMem[rchild].isAllocated = 0; //delete rchild

		buddyMem[lchild / 2].isAllocated = -1; //Open parent back up
		buddyMem[lchild / 2].memSize = buddyMem[lchild].memSize * 2; //Double its memory
		
		merge(lchild / 2, lchild / 2 + 1); //Try to merge its parents recursively if possible
		return 1;
	}
	return 0; //Did not merge

}

int findSpace(struct block proc)
{
	found = 0;
	currentSmallest = INT_MAX;
	smallestSpace = -1;
	traverseSystemFindSpace(1, proc); // Traverse tree to find smallest block with the best fit.
	if (currentSmallest != INT_MAX && currentSmallest != 0) //If we actually found a choice and its not a perfect fit
	{
		while (buddyMem[smallestSpace].memSize / 2 > proc.size) //while we can keep splitting without being too small for our mem
		{
			split(smallestSpace);
			smallestSpace = smallestSpace * 2; //Update index to left child
		}
		buddyMem[smallestSpace].isAllocated = 1; //Now allocated.
		memcpy(&(buddyMem[smallestSpace]), &proc, sizeof(struct block));  //Copy contents to memory block
		return getBuddyLocation(smallestSpace);

	}
	if (currentSmallest == 0) //If there's a perfect fit, take it.
	{
		//Alocate to block
		buddyMem[smallestSpace].isAllocated = 1;
		memcpy(&(buddyMem[smallestSpace]), &proc, sizeof(struct block));
		numBuddy += 1;
		return getBuddyLocation(smallestSpace);
	}
	if (currentSmallest == INT_MAX) //We never found a space we could fit in
	{
		printf("FAIL REQUEST %s %d\n", proc.proc, proc.size);
	}

}

void releaseBuddy(char* proc) //Releases block. If it can merge, it will merge all the way up.
{
	//find node
	int node = -1;
	for (int i = 1; i <= numBuddy; i++)
	{
		if (strcmp(buddyMem[i].proc.proc, proc) == 0) //If the proc names match
		{
			node = i;
			break;
		}
	}

	if (node == 1) //If the root node
	{
		printf("FREE %s %d %d", buddyMem[1].proc.proc, buddyMem[1].memSize, 0);
		memcpy(&buddyMem[1].proc, 0, sizeof(struct block)); //Clear block inside
		buddyMem[1].isAllocated = -1; //Now free
		numBuddy -= 1; //One less buddy out there
		return;
	}
	else if (node != -1) //If not root
	{
		printf("FREE %s %d %d\n", buddyMem[node].proc.proc, buddyMem[node].memSize, getBuddyLocation(node));
		//memcpy(&buddyMem[node].proc, 0, sizeof(struct block)); //Clear block
		buddyMem[node].isAllocated = -1; //Free it
		if (node % 2 == 0) //If our node is even (left child)
		{
			if (buddyMem[node + 1].isAllocated == -1) //If its sister block (right) is also empty
			{
				merge(node, node + 1);
			}
		}
		else if (node % 2 == 1) //If our node is odd (right child)
		{
			if (buddyMem[node - 1].isAllocated == -1) //If the sister block (left) is also empty
			{
				merge(node - 1, node);
			}
		}
	}
	if (node == -1)
	{
		printf("FAIL RELEASE %s\n", proc);
	}
}

void listAssignedBuddy()
{
	printedAssigned = 0; //We haven't printed yet
	traverseSystemFindAssigned(1); //Traverse for filled nodes
	if (printedAssigned == 1) //If printed
		printf("\n");
	if (printedAssigned == 0) //If no prints happened
		printf("NONE\n");

}

void listAvailableBuddy()
{
	printedAvail = 0;
	availEnd = 0;
	availStart = 0;
	lastStatus = 0;
	traverseSystemFindAvail(1); 
	if (printedAvail == 1)
		printf("\n");
	if (printedAvail == 0)
		printf("FULL\n");
}

void findBuddy(char* proc)
{
	for (int i = 1; i <= numBuddy; i++)
	{
		if (strcmp(buddyMem[i].proc.proc, proc) == 0) //If process name matches
		{
			printf("$(%s, %d, %d)$\n", proc, buddyMem[i].memSize, getBuddyLocation(i)); //Print out info
			return;
		}
	}
	printf("FAULT\n"); //Fail
}

int getBuddyLocation(int index)
{
	startFinder = 0;
	found = 0;
	traverseSystemFindStart(1, index);
	return startFinder;
}

int traverseSystemFindSpace(int index, struct block proc)
{
	if (index > 0) {
		traverseSystemFindSpace(getLeftChild(index), proc);
		
		int diff = buddyMem[index].memSize - proc.size;
		if (buddyMem[index].isAllocated == -1 && diff < currentSmallest && diff >= 0) //If diff is smaller than our current and positive (can fit) and the block isn't allocated
		{
			currentSmallest = diff; //Update our current smallest
			smallestSpace = index;
			if (currentSmallest == 0) //If perfect fit, stop here.
			{
				smallestSpace = index;
			}
				
		}
		traverseSystemFindSpace(getRightChild(index), proc);
	}
}

int traverseSystemFindStart(int index, int interest) //traversal for finding the start of a node
{
	if (index > 0) {
		traverseSystemFindStart(getLeftChild(index), interest);
		if (index == interest) // if we're at our node of interest
		{
			found = 1;
			return startFinder; //Return what we've gotten
		}
		if ((buddyMem[index].isAllocated == -1 || buddyMem[index].isAllocated == 1) && found == 0) //If either free or taken, its an active node. Also if we haven't found the answer
		{
			startFinder += buddyMem[index].memSize; //Add to our total
		}
		traverseSystemFindStart(getRightChild(index), interest);
	}
}

//traversal for assigned nodes
int traverseSystemFindAssigned(int index)
{
	if (index > 0) {
		traverseSystemFindAssigned(getLeftChild(index));
		if (buddyMem[index].isAllocated == 1) //If taken
		{
			printf("$(%s, %d, %d)$ ", buddyMem[index].proc.proc, buddyMem[index].memSize, getBuddyLocation(index));
			printedAssigned = 1;
		}
		traverseSystemFindAssigned(getRightChild(index));
	}
}

int traverseSystemFindAvail(int index)
{
	if (buddyMem[1].isAllocated == 1)
		return;
	if (buddyMem[1].isAllocated == -1)
	{
		printf("$(%d, %d)$", 0, memSize);
		printedAvail = 1;
		return;
	}
	if (index > 0) {
		traverseSystemFindAvail(getLeftChild(index));
		if (buddyMem[index].isAllocated == -1 && lastStatus == 0) //If we've hit an empty block and we just started
		{
			availStart = getBuddyLocation(index); //Reset total
			availEnd = availStart + buddyMem[index].memSize; //Make the end the end of this block, just in case its the only free block
			lastStatus = -1; //just hit empty
		}
		else if (buddyMem[index].isAllocated == 1 && lastStatus == 0) //If we've hit a full block and we just started
		{
			lastStatus = 1; //just hit full
		}
		else if (buddyMem[index].isAllocated == -1 && lastStatus == -1) //If empty and we're appending from an empty block
		{
			availEnd = getBuddyLocation(index); //Make the 'end' of our free block this space
			lastStatus = -1; //Last encountered was an empty block
		}
		else if (buddyMem[index].isAllocated == -1 && lastStatus == 1) //If we're empty and coming from a full block
		{
			availStart = getBuddyLocation(index); //Reset total
			availEnd = availStart + buddyMem[index].memSize; //Make the end the end of this block, just in case its the only free block
			lastStatus = -1; //Last encountered was an empty block
		}
		else if (buddyMem[index].isAllocated == 1 && lastStatus == -1) //If a taken block and coming from an empty block
		{
			printf("$(%d, %d)$ ", availStart, availEnd); //print the size of the free block
			printedAvail = 1;  //We printed something
			lastStatus = 1; //We've hit a full block
		}
		else if (buddyMem[index].isAllocated == 1 && lastStatus == 1) //If coming from a full block to a full block
		{
			lastStatus = 1; //keep going
		}
		traverseSystemFindAvail(getRightChild(index));
	}
}


int getLeftChild(int index)
{
	if (2 * index <= numBuddy)
		return 2 * index;
	return -1;
}

int getRightChild(int index)
{
	if (2 * index + 1 <= numBuddy)
		return 2 * index + 1;
	return -1;
}
