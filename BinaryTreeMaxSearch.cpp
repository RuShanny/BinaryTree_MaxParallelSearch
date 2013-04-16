// BinaryTreeMaxSearch.cpp : Defines the entry point for the console application.
//


#include <string>
#include <iostream>
#include <time.h>
#include <mutex>
#include <list>
#include <thread>
#include <tchar.h>
#include <windows.h>

using namespace std; 

template <typename T>
class Node
{
private:
  Node<T> *_parent;
	Node<T> *_left;
	Node<T> *_right;
	T _key;
public:
	//Constructor initialize components 
	Node(T key, Node<T> *lptr, Node<T> *rptr, Node<T> *parent):
		_key(key), _left(lptr), _right(rptr), _parent(parent) { }

	Node<T>* GetLeft(void) { return _left; }
	void SetLeft(Node<T>* left) { _left = left; }

	Node<T>* GetRight(void) { return _right; }
	void SetRight(Node<T>* right) { _right = right; }

	T GetKey() { return _key; }
	void SetKey(T key) { _key = key; }

	//Gets pointer to current_node's parent_node
	Node<T>* GetParent(void) { return _parent; }
};

template <typename T>
class MutexMaxKey {
private:
	 mutex m;
	 Node<T>* _max;
public:
	 MutexMaxKey() : _max(NULL) { }

	 // For correct access to shared data - max - use mutex
	 void CompareAndSet(Node<T>* candidate) {
		 m.lock();
		 if (this->GetMax() == NULL) _max = candidate;
		 else if (candidate->GetKey() >= (this->GetMax())->GetKey())  _max = candidate;
		 m.unlock();
	 }

	
	 Node<T>* GetMax() { return _max; }
	 
 };

template <typename T>
class BinTree {
private:
	int _nodesnumber;
	Node<T>* _root;
public:
	BinTree(int nodesnumber):_nodesnumber(nodesnumber)
	{
		srand((unsigned int)time(NULL));
		this->SetRoot(RecursiveAddNode(_root, NULL, _nodesnumber));
	}
	Node<T>* GetRoot() { return _root; };
	void SetRoot(Node<T>* root) { _root = root; }

	// RecursiveAddNode returns pointer to just created node
	Node<T>* RecursiveAddNode(Node<T>* newnode, Node<T>* parentptr, int nodesnumber) 
	{
		
		if (nodesnumber == 0) return NULL;
		newnode = new Node<T>(rand()%10000, NULL, NULL, parentptr);
		nodesnumber--;
		if (nodesnumber == 0) return newnode;

		//Check whether it's one left element that should be created
		//If true, add left child of the newnode
		Node<T> *left = NULL, *right = NULL;
		if (nodesnumber == 1) {
			newnode->SetLeft(RecursiveAddNode(left, newnode, nodesnumber));
			return newnode;
		}
		// Divide total number of rest nodes into two parts : 
		// half of them will be created in left part of the tree
		// another half - in the right one
		int numright, numleft;
		numleft = nodesnumber/2;
		numright = nodesnumber - numleft;
		newnode->SetLeft(RecursiveAddNode(left, newnode, numleft));
		newnode->SetRight(RecursiveAddNode(right, newnode, numright));
		return newnode;
	}
	// Returns pointer to the node with greater key's value 
	Node<T>* CompareNodes (Node<T>* Anode, Node<T>* Bnode) 
	{
		if ( Anode->GetKey() >= Bnode->GetKey() ) return Anode;
		else return Bnode;
	}

	// Search the node with a max key's value in the subtree which root_node is input argument; 
	// Returns pointer to the node with a max key
	Node<T>* MaxSearch(Node<T>* currentnode) 
	{
		Node<T>* maxnode;
		if ((currentnode->GetLeft() != NULL)&&(currentnode->GetRight() != NULL))
		{ 
			maxnode = CompareNodes(MaxSearch(currentnode->GetLeft()), MaxSearch(currentnode->GetRight()));
			maxnode = CompareNodes(maxnode, currentnode);
			return maxnode;
		}
		else 
		{ 
			if ((currentnode->GetLeft() == NULL) && (currentnode->GetRight() != NULL)) {
				maxnode = CompareNodes(MaxSearch(currentnode->GetRight()), currentnode);
				return maxnode;
			}
			if ((currentnode->GetLeft() != NULL) && (currentnode->GetRight() == NULL)) {
				maxnode = CompareNodes(MaxSearch(currentnode->GetLeft()), currentnode);
				return maxnode;
			}

			if ((currentnode->GetLeft() == NULL) && (currentnode->GetRight() == NULL))  return currentnode;
			
		}
	} 
	// Using method MaxSearch, search the node with a max key in the subtree which root_node is input argument (node) 
	// And then compare obtained max key's value with key of current_node's parents;
	// Number of parents' nodes with whom it should compare obtained max key is input argument (parentsnum);
	Node<T>* MaxSearchWithParents (Node<T>* node, int parentsnum) 
	{
		Node<T>* maxnode = MaxSearch(node);
		Node<T>* tmpnode = node;
		for ( unsigned i = 0; i < parentsnum; i++ )
		{
			// If there's no parents's nodes return obtained node with max key's value
			if (tmpnode->GetParent() == NULL) 
			{
				return maxnode;
			}
			else {
				maxnode = CompareNodes(tmpnode->GetParent(), maxnode);
				tmpnode = tmpnode->GetParent();
			}
		}
		return maxnode;
	}
	
	void ThreadSearchMaxElement(Node<T>* entrypoint, int parentsnum, MutexMaxKey<T>* M) {
		Node<T>* threadmaxnode = MaxSearchWithParents(entrypoint, parentsnum);
		//cout << threadmaxnode->GetKey() << "    ";
		M->CompareAndSet(threadmaxnode);
	}

	// Set pointer to the node with max key into M 
	void ParallelsMaxSearch(int threadsnum, MutexMaxKey<T>* M) {
		Node<T>* entrypoint = this->GetRoot();
		int parentsnum = 0;
		if (threadsnum == 1) {
			ThreadSearchMaxElement(entrypoint, parentsnum, M);
			return;
		}

		// Create list of instructions for each thread
		// With its help each thread can find its own entry point node for searching max key
		InstructionsForThreads Instructions(threadsnum);
	
		list<thread> ThreadsList;
		list<EntryPointList>::iterator itEntryPoint;
		list<unsigned char>::iterator it;
		list<thread>::iterator itThread;
		
		for (itEntryPoint = Instructions.begin(); itEntryPoint != Instructions.end(); itEntryPoint++) {
			entrypoint = this->GetRoot();
			parentsnum = itEntryPoint->GetParentsNum(); 

			// Find an entry point node for a thread with help of instructions list
			for (it = (*itEntryPoint).begin(); it != (*itEntryPoint).end(); it++) {
				if ((*it) == 'L') entrypoint = entrypoint->GetLeft();
				else entrypoint = entrypoint->GetRight();
			}
			//cout << "entrypoint key = " << entrypoint->key << "   ";

			// Start new thread to find node with max key in its subtree
			ThreadsList.push_back( thread(&BinTree<T>::ThreadSearchMaxElement, this, entrypoint, parentsnum, M) );
		}
		// Wait until all the threads finish its work
		for (itThread = ThreadsList.begin(); itThread != ThreadsList.end(); itThread++ ) {
			(*itThread).join();
		}
	}

	~BinTree() {
		RecursiveRemoveNodes(this->GetRoot());
	}

	void RecursiveRemoveNodes(Node<T>* node) {
		if ((node->GetLeft() == NULL)&&(node->GetRight() == NULL)) {
			if (node->GetParent() != NULL) 
				if ((node->GetParent()->GetRight()) == node) {
					(node->GetParent())->SetRight(NULL);
				} else (node->GetParent())->SetLeft(NULL);
			delete node;
		} 
		else {
			if (node->GetLeft() != NULL) RecursiveRemoveNodes(node->GetLeft());
			if (node->GetRight() != NULL) RecursiveRemoveNodes(node->GetRight());
			if (node->GetParent() != NULL) 
				if ((node->GetParent()->GetRight()) == node) {
					(node->GetParent())->SetRight(NULL);
				} else (node->GetParent())->SetLeft(NULL);
			delete node;
		}
	}

	// Method for tree's output to console
	void ShowTree(Node<T>* node, int num) 
	{
		if ( node->GetRight() != NULL )
			ShowTree(node->GetRight(), num+1);
		else {
			for ( int i = 1; i <= num; i++ ) 
				cout << "  ";
			cout << "  " << endl;
		}
		for ( int i = 1; i <= num; i++ ) 
			cout << "  ";
		cout << node->key << endl;
		if (node->GetLeft() != NULL) 
			ShowTree(node->GetLeft(), num+1);
		else cout << "  " ;
	}
};

// Contains list of 'L' and 'R' chars.
// These instructions: 'L' - left, 'R' - right (moving begins from the root node of the whole tree)
// help find entry point node (root node of subtree) for a thread
// from which it begins searching node with a max key.
// In order to create correct instructions for a thread it's necessairy:
// 1) take number of a thread
// 2) convert it to the binary representation
// 3) each digit of it means certain instruction: 0 - left ('L') , 1 - right ('R').
 class EntryPointList : public list<unsigned char> {
private:
	// Amount of parents' nodes with whom a thread should compare obtained max key
	int _parentsnum;
	// Number of a thread for which this EntryPointList is created
	int _numthread; 
public:
	EntryPointList(int numthread, int instructions_num) : _numthread(numthread) {
		for ( int k = instructions_num - 1; k >= 0; k-- )
			if (Bin(_numthread, k)) list<unsigned char>::push_back('R');  // <--- 1), 2), 3)
			else list<unsigned char>::push_back('L');	

		list<unsigned char>::iterator it;
		it = list<unsigned char>::end();
		_parentsnum = 0; 
		--it;
		while ((*it != 'L')&&(it != list<unsigned char>::begin())) 
		{
				_parentsnum++;
				--it;
		}
			
		if ((it == list<unsigned char>::begin())&&(*it == 'R')) _parentsnum++;

	}

	int GetParentsNum() { return _parentsnum; }
	int GetNumThread() { return _numthread; }
	void SetParentsNum(int parentsnum) { _parentsnum = parentsnum; }
	void SetNumThread(int numthread) { _numthread = numthread; }

	// Returns bit of the number "x", which stays in "bitnum"'s place
	int Bin(int x, int bitnum) 
	{
		int res =  (x>>bitnum) & 1;
		return res;
	}
};
 
 // In order to devide the tree into several parts correctly and evenly
 // (each part of the tree will be given to one of the threads) following ALGORITHM is used : 
 // 1) find an entry point(root node of subtree) for each thread;
 //		for each thread it's necessairy to go through several nodes deep into the tree:
 //		amount of the these nodes is counted as 
 //     "instructions_num_per_thread = ceil(log(total number of threads) to the base 2)";
 //		their amount is equal to the number of instructions (consist of "left" and "right" ) 
 //		which should be followed in order to achieve an entry point for a thread;
 // 2) make CORRECT list with excess amount of instructions for threads;
 //		it's easier to create such list for number of threads = pow (2, num),
 //		which is greater than or equal to the required number of threads;
 //		(it's easier because this amount of threads is a power of two)
 // 3) decrease it to the required quantity of instructions;
 //		3.a) count difference between pow (2, num) and required number of threads
 //		3.b) every pair of elements (counting begins from the end of the list) is modified in a certain way:
 //		3.c) last element of this pair should be removed
 //		3.d) second element of this pair: remove last element in this sublist
 //	 repeat steps 3.b - 3.d amount of times which is equal to difference between pow(2, num) and required number of threads.
class InstructionsForThreads : public list<EntryPointList> {
private:
	// number of instructions for finding an entry point node for EACH thread
	int _instructions_num_per_thread;
	// length of the list is equal to the total number of threads
	int _length;
public:
	InstructionsForThreads (int threadsNum) {
		this->SetInstructionsNum((int)ceil(log10(threadsNum) / log10(2)));
		this->SetLength(pow(2, this->GetInstructionsNum()));
	
		EntryPointList::iterator it;
		for (int j = 0; j < this->GetLength(); j++ ) {		// <--- 2)
			EntryPointList A(j, this->GetInstructionsNum());
			list<EntryPointList>::push_back(A);
		}
		
		list<EntryPointList>::iterator itEntryPoint;
		itEntryPoint = list<EntryPointList>::end();
	
		int shorter = this->GetLength() - threadsNum;		// <--- 3.a) 
	
		for ( int j = 0; j < shorter; j ++ ) {				// <--- 3.b) 
			itEntryPoint--;
			itEntryPoint->SetParentsNum(itEntryPoint->GetParentsNum()- 1);
			int tmpparentsnum = itEntryPoint->GetParentsNum();
			EntryPointList tmp = *itEntryPoint;
			itEntryPoint--;
			list<EntryPointList>::remove(tmp);				// <--- 3.c)
			itEntryPoint->SetParentsNum(tmpparentsnum);
			(*itEntryPoint).pop_back();						// <--- 3.d) 
		}
	}
	int GetLength() { return _length; }
	void SetLength(int length) { _length = length; }

	int GetInstructionsNum() { return _instructions_num_per_thread; }
	void SetInstructionsNum(int instructions_num_per_thread) {
		_instructions_num_per_thread = instructions_num_per_thread;
	}

};

// Using QueryPerformanceCounter in order to compare time of execution:
// when program works with more than 1 thread and when it uses only one thread

double PCFreq = 0.0;
__int64 CounterStart = 0;

void StartCounter()
{
    LARGE_INTEGER li;
    if(!QueryPerformanceFrequency(&li))
	cout << "QueryPerformanceFrequency failed!\n";

    PCFreq = double(li.QuadPart)/1000.0;

    QueryPerformanceCounter(&li);
    CounterStart = li.QuadPart;
}

double GetCounter()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart-CounterStart)/PCFreq;
}

int _tmain(int argc, _TCHAR* argv[])
{
	int nodesnum, threadsnum, permit_num_of_threads, permit_number_of_nodes;
	MutexMaxKey<double> M;

	while (1) {
		cout << "Enter number of nodes, please : " << endl;
		cin >> nodesnum;
		cout << "Enter number of threads, please : " << endl;
		cin >> threadsnum;
		permit_num_of_threads = (int)ceil(log10(threadsnum) / log10(2));
		permit_number_of_nodes = (int)ceil(log10(nodesnum) / log10(2));
		if ( (nodesnum == 1 ) && (threadsnum == 1) ) break;
		if (permit_number_of_nodes >  permit_num_of_threads + 1) break; 
		else {
			cout << "TOO MUCH THREADS for entered number of nodes" << endl;
			cout << "Please, try again" << endl;
			continue; 
		}
		if ((nodesnum <= 0)||(threadsnum <= 0)) {
			cout << "Number should be positive" << endl;
			cout << "Please, try again" << endl;
			continue;
		}

	}

	BinTree<double> Tree(nodesnum);

	
	StartCounter();
	Tree.ParallelsMaxSearch(1, &M) ;
	cout << "Working with 1 thread" << endl;
	cout <<"the number of milliseconds since StartCounter() was last called = " << GetCounter() << endl;
	cout << endl;

	StartCounter();
	Tree.ParallelsMaxSearch(threadsnum, &M) ;
	cout << "Working with " << threadsnum << " threads" << endl;
	cout <<"the number of milliseconds since StartCounter() was last called = " << GetCounter() << endl;
	cout << endl;


	Node<double>* max = M.GetMax();
	cout << endl;
	cout << "Pointer to the node with max key = " << max << endl;
	cout << "Max key in the tree = " << max->GetKey() << endl;
	system("PAUSE");

	return 0;
}

