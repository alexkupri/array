//  Copyright (C) 2014 by Aleksandr Kupriianov
//  alexkupri at gmail dot com

//  Purpose: fast array tests and usage examples

#include <stdlib.h>
#include <deque>
#include <vector>
#include <list>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include "alex_kupriianov_array.h"

using namespace std;

int artific_exceptions=0;
enum {MM=4,NN=4};

#ifdef __linux__
#ifdef __GNUC__
#define using_mallinfo_memleak_detection
#endif
#endif

#ifdef using_mallinfo_memleak_detection
#include <ext/throw_allocator.h>
#include <malloc.h>
#include <sys/timeb.h>
int UsedMem()
{
	struct mallinfo a=mallinfo();
	return a.uordblks;
}
double MSec()
{
	struct timeb t1;
	ftime(&t1);
	return t1.time*1000.0+t1.millitm;
}
#define memleak_str "Also checking for memory leaks."
#define time_str "Calculating time, precision is milliseconds."
#else
#include <ctime>
int UsedMem(){return 0;}
double MSec()
{
	clock_t t=clock();
	return t*1000.0/CLOCKS_PER_SEC;
}
#define memleak_str "Not using memory leak detection (currently available on linux/gnuc)."
#define time_str "Time prececision is seconds."
#endif

//================ Class for counting memory and time ==============

class TestDescriptor
{
	double time;
	int memory;
	TestDescriptor();
public:
	TestDescriptor(const char *description);
	~TestDescriptor();
};

TestDescriptor::TestDescriptor(const char *description)
{
	cout<<description<<"\n";
	memory=UsedMem();
	time=MSec();
}

TestDescriptor::~TestDescriptor()
{
	assert(memory==UsedMem());
	cout<<"Success : "<<(MSec()-time)<<" ms.\n"; 
}

//========== Container class for comparing results =================
template <typename T,typename Alloc=default_memory_operator<T> >
class MultipleChecker
{
	alex_kupriianov_array<T,MM,NN,Alloc> aka;
	vector<T> vi;
	void Check();
	int i1,im,d1,dm;
public:
	bool b;
	MultipleChecker():aka(),vi()
	{
		b=false;
		i1=im=d1=dm=0;
	}
	void insert(int pos,T val);
	void insert(int pos,const T *start,const T *finish);
	void erase(int pos);
	void erase(int first,int last);
	int  size(){return vi.size();}
	T  operator[](int pos){return vi[pos];}//gentleman agreement - RO
	void Stats()
	{
		cout<<"i1="<<i1<<" d1="<<d1<<" im="<<im<<" dm="<<dm<<"\n";
	}
};

template <typename T,typename Alloc>
void MultipleChecker<T,Alloc>::insert(int pos,T val)
{
	if(b){
		cout<<"Ins "<<pos<<";"<<vi.size()<<"\n";
	}
	vi.insert(vi.begin()+pos,val);
retry:
	try{//This is used only for NoMemoryTest.
		aka.insert(pos,val);
	}
	catch(...){
		artific_exceptions++;
		goto retry;
	}
	Check();
	i1++;
}

template <typename T,typename Alloc>
void MultipleChecker<T,Alloc>::insert(int pos,const T *start,const T *finish)
{
	if(b){
		cout<<"Ins "<<pos<<" "<<(finish-start)<<";"<<vi.size()<<"\n";
	}
	vi.insert(vi.begin()+pos,start,finish);
retry:
	try{//This is used only for NoMemoryTest.
		aka.insert(pos,start,finish);
	}
	catch(...){
		artific_exceptions++;
		goto retry;
	}
	Check();
	im++;
}

template <typename T,typename Alloc>
void MultipleChecker<T,Alloc>::erase(int pos)
{
	if(b){
		cout<<"Del "<<pos<<";"<<vi.size()<<"\n";
	}
	vi.erase(vi.begin()+pos);
	aka.erase(pos);
	Check();
	d1++;
}

template <typename T,typename Alloc>
void MultipleChecker<T,Alloc>::erase(int first,int last)
{
	if(b){
		cout<<"Del "<<first<<" "<<last<<";"<<vi.size()<<"\n";
	}
	vi.erase(vi.begin()+first,vi.begin()+last);
	aka.erase(first,last);
	Check();
	dm++;
}

template <typename T,typename Alloc>
void MultipleChecker<T,Alloc>::Check()
{
	size_t j;
	assert(vi.size()==aka.size());
	for(j=0;j<vi.size();j++){
		assert(vi[j]==aka[j]);
	}
	aka.__check_consistency();
}

//================= Class for imitating complex structure ==============

struct IntVal;

int cn=0,ds=0;
class IntContainer
{
	IntVal *iv;
	void Check()const;
public:
	IntContainer();
	void set(int j);
	int  get()const;
	IntContainer(const IntContainer& ic);
	IntContainer &operator=(const IntContainer&);
	~IntContainer();
};

ostream &operator<<(ostream &os,const IntContainer &ic)
{
	os<<ic.get();
	return os;
}

struct IntVal
{
	IntContainer *ic;
	int value;
};

IntContainer::IntContainer()
{
	iv=new IntVal;
	iv->ic=this;
	iv->value=-1;
	cn++;
}

IntContainer::IntContainer(const IntContainer& ic)
{
	if(ic.iv->value==-100){
		throw "wow";//This is done for CopyException class
	}
	iv=new IntVal;
	iv->ic=this;
	iv->value=ic.iv->value;
	cn++;
}

IntContainer::~IntContainer()
{
	Check();
	delete iv;
	iv=0;
	ds++;
}

void IntContainer::Check()const
{
	assert(iv->ic==this);
}

void IntContainer::set(int j)
{
	Check();
	iv->value=j;
}

int  IntContainer::get()const
{
	Check();
	return iv->value;
}

IntContainer &IntContainer::operator=(const IntContainer& that)
{
	Check();
	iv->value=that.iv->value;
	return *this;
}

bool operator==(const IntContainer& a,const IntContainer& b)
{
	return a.get()==b.get();
}

void MyCheck(int i)
{
	assert(i>=0);
}

void MyCheck(const IntContainer &ic)
{
	assert(ic.get()>=0);
}

//=============== Correctness test functions =================
template <class T,class V>
void PerformCheck(T &t,const V &vi,int cycles,int rd,int i1,int d1,int im,int edge)
{
	int j,k,l,m,n,p,q,SZ;
	SZ=vi.size();
	for(j=0;j<cycles;j++){
		k=rand()%100;
		l=rand()%SZ;
		m=rand()%SZ;
		n=rand()%SZ;
		p=rand()%100;
		q=rand()%100;
		if(k<rd){
			if(n<t.size()){
				MyCheck(t[n]);
				//assert(t[n]==vi[n]);
			}
		}else if(k<i1){
			if((n<=t.size())&&(t.size()<SZ)){
				if(p<edge){
					n=0;
				}
				if(q<edge){
					n=t.size();
				}
				t.insert((unsigned int)n,vi[l]);
			}
		}else if(k<d1){
			if(n<t.size()){
				if(p<edge){
					n=0;
				}
				if(q<edge){
					n=t.size()-1;
				}
				t.erase(n);
			}
		}else if(k<im){
			if((n<=t.size())&&(t.size()<SZ)&&(l<=m)){
				if(p<edge){
					n=0;
				}
				if(q<edge){
					n=t.size();
				}
				t.insert(n,&vi[l],&vi[m]);
			}
		}else{
			if((l<=m)&&(m<=t.size())){
				if(p<edge){
					l=0;
				}
				if(q<edge){
					m=t.size();
				}				
				t.erase(l,m);
			}
		}
	}
}

void Space()
{
	int j;
	for(j=0;j<100;j++){
		cout<<"\n";
	}
}

void SetVec(vector<int> &inpvect,int val,int num)
{
	int j;
	inpvect.resize(num);
	for(j=0;j<num;j++){
		inpvect[j]=val;
		val++;
	}
}

void BasicTest_Int()
{
	TestDescriptor t1("Simple test for correctness by comparing results "
		"of four operations with vector and alex_kupriianov_array.");
	{
		MultipleChecker<int> mc;
		vector<int> vi;
		SetVec(vi,0,600);
		PerformCheck(mc,vi,600000,75,85,95,98,10);
		PerformCheck(mc,vi,600000,75,85,95,98,50);
		PerformCheck(mc,vi,600000,50,60,90,100,10);
		PerformCheck(mc,vi,600000,50,95,95,95,10);
		PerformCheck(mc,vi,600000,75,85,95,98,10);
	}
}

void BasicTest_IntContainer()
{
	TestDescriptor t1("Test of four operations with complex structure.");
	{
		MultipleChecker<IntContainer> mc;
		vector<IntContainer> vi;
		int j;
		vi.resize(600);
		for(j=0;j<600;j++){
			vi[j].set(j);
		}
		PerformCheck(mc,vi,600000,75,85,95,98,10);
		PerformCheck(mc,vi,600000,75,85,95,98,50);
		PerformCheck(mc,vi,600000,50,60,90,100,10);
		PerformCheck(mc,vi,600000,50,95,95,95,10);
		PerformCheck(mc,vi,600000,75,85,95,98,10);
	}
}

template<class Iter>
int SumIters(Iter begin,Iter end)
{
	int res=0;
	while(begin!=end){
		res+=*begin;
		begin++;
	}
	return res;
}

void SubTest_Iterators(vector<int> &vi,alex_kupriianov_array<int,MM,NN> &aka)
{
	int j;
	size_t k=0;
	vector<int>::iterator vii=vi.begin(),viend=vi.end();
	alex_kupriianov_array<int,MM,NN>::iterator akai=aka.begin(),akaend=aka.end(),empty;
	for(;;){
		assert(*akai==*vii);
		assert(akai.GetAbsolutePosition()==k);
		assert(akai!=akaend);
		for(j=0;j<10;j++){
			vii++;
			akai++;
		}
		if(vii==viend){
			break;
		}
		for(j=0;j<9;j++){
			vii--;
			akai--;
		}
		k++;		
	}
	assert(akai==akaend);
	const alex_kupriianov_array<int,MM,NN> &caka=aka;
	alex_kupriianov_array<int,MM,NN>::iterator 
		mut_fwd_1=aka.begin(),mut_fwd_2=aka.end();
	alex_kupriianov_array<int,MM,NN>::const_iterator 
		con_fwd_1=caka.begin(),con_fwd_2=caka.end();
	alex_kupriianov_array<int,MM,NN>::reverse_iterator 
		mut_bck_1=aka.rbegin(),mut_bck_2=aka.rend();
	alex_kupriianov_array<int,MM,NN>::const_reverse_iterator 
		con_bck_1=caka.rbegin(),con_bck_2=caka.rend();
	assert(SumIters(mut_fwd_1,mut_fwd_2)==SumIters(vi.begin(),vi.end()));
	assert(SumIters(mut_bck_1,mut_bck_2)==SumIters(vi.begin(),vi.end()));
	assert(SumIters(con_fwd_1,con_fwd_2)==SumIters(vi.begin(),vi.end()));
	assert(SumIters(con_bck_1,con_bck_2)==SumIters(vi.begin(),vi.end()));	
}

void IteratorsTest_Int()
{
	TestDescriptor t1("Test with iterators.");
	{
		int j,k,l,m;
		vector<int> vi1,vi2;
		alex_kupriianov_array<int,MM,NN> aka1,aka2;
		SetVec(vi1,0,200);
		aka1.insert(0,vi1.begin(),vi1.end());
		for(j=0;j<1000;j++){
			vi2=vi1;
			aka2=aka1;
			k=rand()%(aka1.size()+1);
			l=rand()%(aka1.size()+1);
			m=rand()%(aka1.size()+1);
			if(k>l){
				swap(k,l);
			}
			aka2.insert(m,aka1.at(k),aka1.at(l));
			vi2.insert(vi2.begin()+m,vi1.begin()+k,vi1.begin()+l);
			SubTest_Iterators(vi2,aka2);
		}
	}	
}

void TestFill_Int()
{
	TestDescriptor t1("Test with fill functions (iterators being checked).");
	{
		alex_kupriianov_array<int,MM,NN> aka1,aka2;
		aka1.fill(0,40,1);
		aka1.fill(0,40,0);
		aka1.fill(80,40,2);
		aka2.fill(0,100,3);
		aka2=aka1;
		alex_kupriianov_array<int,MM,NN> aka3(aka2.rbegin(),aka2.rend());
		alex_kupriianov_array<int,MM,NN> aka4(aka3);
	}	
}

void TestCopyExceptions()
{
	TestDescriptor t1("Test for exception handling. When objects are copied, they throw exceptions.");
	{
	int j,k,num,inspl;
	size_t l,m;
	bool exceptioned;
	int sizes[10]={0,1,2,4,8, 15,20,50,70,100};
	{
		for(j=0;j<10;j++){
			for(k=1;k<10;k++){
				vector<IntContainer> vi1,vi2;//vi1 can be empty,vi2 cannot
				IntContainer single;
				single.set(-100);
				vi1.resize(sizes[j]);
				vi2.resize(sizes[k]);
				for(l=0;l<vi1.size();l++){
					vi1[l].set(l);
				}
				for(l=0;l<vi2.size();l++){
					vi2[l].set(l);
				}
				alex_kupriianov_array<IntContainer,MM,NN> aka(vi1.begin(),vi1.end());
				for(l=0;l<50;l++){
					num=rand()%vi2.size();
					inspl=rand()%(vi1.size()+1);
					switch(l){//special cases: begin and end
						case 0:
						case 1:
						case 2:
							num=0;
							break;
						case 3:
						case 4:
						case 5:
							num=vi2.size()-1;
							break;
					}
					switch(l){//special cases: begin and end
						case 0:
						case 3:
						case 6:
							inspl=0;
							break;
						case 1:
						case 4:
						case 7:
							inspl=vi1.size();
							break;
					}
					//multiple insert attempt
					vi2[num].set(-100);
					exceptioned=false;
					try{
						aka.insert(inspl,vi2.begin(),vi2.end());
					}catch(...){
						exceptioned=true;
					}
					vi2[num].set(num);
					assert(exceptioned);
					aka.__check_consistency();
					assert(aka.size()==vi1.size());
					for(m=0;m<aka.size();m++){
						assert(aka[m].get()==(int)m);
					}
					//single insert attempt 
					exceptioned=false;
					try{
						aka.insert(inspl,single);
					}catch(...){
						exceptioned=true;
					}
					assert(exceptioned);
					aka.__check_consistency();
					assert(aka.size()==vi1.size());
					for(m=0;m<aka.size();m++){
						assert(aka[m].get()==(int)m);
					}
				}
			}
		}	
	}
	}	
}

void TestFastAllocator()
{
	TestDescriptor t1("Test with fast allocator (which uses memcpy).");
	{
		MultipleChecker<int,fast_memory_operator<int> > mc;
		vector<int> vi;
		SetVec(vi,0,600);
		PerformCheck(mc,vi,600000,75,85,95,98,10);
		PerformCheck(mc,vi,600000,75,85,95,98,50);
		PerformCheck(mc,vi,600000,50,60,90,100,10);
		PerformCheck(mc,vi,600000,50,95,95,95,10);
		PerformCheck(mc,vi,600000,75,85,95,98,10);
	}	
}

#ifdef using_mallinfo_memleak_detection
void TestNoMemExceptions()
{
	TestDescriptor t1("Test imitating nomemory exception.");
	{
		__gnu_cxx::throw_allocator_base::set_throw_prob(0.01);
		__gnu_cxx::throw_allocator<int> t;t.init(10);
		MultipleChecker<int,fast_memory_operator<int,__gnu_cxx::throw_allocator<int> > > mc;
		vector<int> vi;
		SetVec(vi,0,60);
		PerformCheck(mc,vi,600000,75,85,95,98,10);
		PerformCheck(mc,vi,600000,75,85,95,98,50);
		PerformCheck(mc,vi,600000,50,60,90,100,10);
		PerformCheck(mc,vi,600000,50,95,95,95,10);
		PerformCheck(mc,vi,600000,75,85,95,98,10);
	}	
}
#else
void TestNoMemExceptions()
{
	cout<<"The rare test for correct nomem behaviour is available only for Linux.\n";
}
#endif

void CorrectnessTestBundle()
{
	BasicTest_Int();
	BasicTest_IntContainer();	
	IteratorsTest_Int();
	TestFill_Int();
	TestFastAllocator();
	TestCopyExceptions();
	TestNoMemExceptions();
	cout<<"All tests passed. Success.\n";
}

//=============  Manual test ===================

void DumpAAI(alex_kupriianov_array<int,MM,NN> &aai)
{
	size_t idx;
	for(idx=0;idx<aai.size();idx++){
		cout/*<<" ("<<idx<<") "*/<<aai[idx]<<" ";
	}
	cout<<"\n";	
}

void ManualTest()
{
	int command,idx,val,num,j;
	vector<int> inpvect;
	alex_kupriianov_array<int,MM,NN> aai;
	for(;;){
		cout<<"Options: 0-exit, 1-show all, 2-insert single, 3-insert multiple(the same)\n";
		cout<<"Options: 4-delete, 5-delete multiple\n";
		cin>>command;
		switch(command){
			case 0:return;
			case 1:
			    //aai.__output(cout);
				DumpAAI(aai);				
				aai.__check_consistency();
				break;
			case 2:
				cout<<"Enter position and value\n";
				cin>>idx>>val;
				aai.insert(idx,val);
				break;
			case 3:
				cout<<"Enter position, start value and repetition count:\n";
				cin>>idx>>val>>num;
				inpvect.resize(num);
				for(j=0;j<num;j++){
					inpvect[j]=val;
					val++;
				}
				aai.insert(idx,inpvect.begin(),inpvect.end());
				break;
			case 4:
				cout<<"Enter position for deletion:\n";
				cin>>idx;
				aai.erase(idx);
				break;			
			case 5:
				cout<<"Enter start and end+1 for deletion:\n";
				cin>>idx>>num;
				aai.erase(idx,num);
				break;			
			default:
				cout<<"Try again\n";
				break;
		}
		//aai.__output(cout);
		aai.__check_consistency();
	} 
}

//===================== Performance tests =============================

enum {MMM=14,NNN=124};

inline void insert_val(std::vector<int> &vi,int pos,int val)
{
	vi.insert(vi.begin()+pos,val);
} 

inline void insert_val(std::deque<int> &vi,int pos,int val)
{
	vi.insert(vi.begin()+pos,val);
} 

inline void insert_val(alex_kupriianov_array<int,MMM,NNN> &akai,int pos,int val)
{
	akai.insert(pos,val);
} 

inline void insert_val(alex_kupriianov_array<int,MMM,NNN,fast_memory_operator<int> > &akai,int pos,int val)
{
	akai.insert(pos,val);
} 

inline void erase_val(std::vector<int> &vi,int pos)
{
	vi.erase(vi.begin()+pos);
} 

inline void erase_val(std::deque<int> &vi,int pos)
{
	vi.erase(vi.begin()+pos);
} 

inline void erase_val(alex_kupriianov_array<int,MMM,NNN> &akai,int pos)
{
	akai.erase(pos);
} 

inline void erase_val(alex_kupriianov_array<int,MMM,NNN,fast_memory_operator<int> > &akai,int pos)
{
	akai.erase(pos);
} 

template <class IntContainer>
int SumArray(IntContainer &ic)
{
	int res=0;
/*	int idx,n=ic.size();
	for(idx=0;idx<n;idx++){
		res+=ic[idx];
	}*/
	typename IntContainer::iterator it;
	for(it=ic.begin();it!=ic.end();it++){
		res+=(*it);
	}
	return res;
}
 

template <class Container>
void SingleOperationPerformanceCheck(ofstream &ofs,int elements,int maxsz)
{
	int  (*func_var)(Container&)=&SumArray;
	void *ptr=(void*)func_var;
	int lim=1,iter=0,cursz=0,place=0,j,sum3=0;
	std::vector<Container> vecCon;
	typename std::vector<Container>::iterator it;
	double denom;
	typename Container::iterator subit;
	vecCon.resize(elements);
	int sum1=0,sum2=0,log2=1;
	while(lim<maxsz){
		lim<<=1;
		log2++;
	}
	std::vector<double> vd;
	vd.resize(log2);
	std::vector<double> t_start1=vd,t_ins=vd,t_readrnd=vd,t_readiter=vd,
		t_start2=vd,t_del=vd;
	//Inserting and reading, while measuring performance
	lim=1;
	while(lim<maxsz){
		t_start1[iter]=MSec();
		//Inserting		
		while(cursz<lim){
			cursz++;
			for(it=vecCon.begin();it!=vecCon.end();it++){
				place=(place+101)%cursz;
				insert_val(*it,place,place);
			}
		}
		t_ins[iter]=MSec();
		//Accessing randomly		
		for(j=0;j<cursz;j++){
			for(it=vecCon.begin();it!=vecCon.end();it++){
				sum1+=(*it)[(j*101)%cursz];
			}
		}
		t_readrnd[iter]=MSec();
		//Accessing via iterator
		for(it=vecCon.begin();it!=vecCon.end();it++){
			for(subit=(*it).begin();subit!=(*it).end();subit++){
				sum2+=(*subit);
			}
		}
		t_readiter[iter]=MSec();
		lim<<=1;
		iter++;
		//sum3+=func_var(vecCon[0]);				
	}
	//Deleting and measuring performance
	while(lim!=0){
		lim>>=1;
		t_start2[iter]=MSec();
		while(cursz>lim){
			for(it=vecCon.begin();it!=vecCon.end();it++){
				place=(place+101)%cursz;
				erase_val(*it,place);
			}
			cursz--;
		}
		t_del[iter]=MSec();
		iter--;
	}
	cout<<"Our dummies "<<sum1<<" "<<sum2<<" "<<sum3<<" "<<ptr<<"\n";
	ofs<<"\n Averaged on "<<elements<<" measurements.\n";
	ofs<<"Array size      Insert      Delete     Read_rand     Read_iter\n";	
	ofs<<setprecision(2)<<scientific;
	for(j=0;j<(log2-1);j++){
		denom=1.0/((double)(1<<j))/((double)(elements));
		ofs<<setw(9)<<(1<<j)<<" "<<setw(12)<<((t_ins[j]-t_start1[j])*denom*2.0)
				   <<" "<<setw(12)<<((t_del[j]-t_start2[j])*denom*2.0)
				   <<" "<<setw(12)<<((t_readrnd[j]-t_ins[j])*denom)
				   <<" "<<setw(12)<<((t_readiter[j]-t_readrnd[j])*denom)<<"\n";
		//We have accessed all elems, and added only half of them.
		//Strictly speaking, we measure insertion and deletion not when
		//array size==1<<log2, but when it changes from 1<<(log2-1) to 1<<log2.
		//However, we get a dynamics.
	} 
	ofs<<"\n\n\n";	
}

void MultipleOperationsTest(ofstream &ofs,int arr,int maxsz)
{
	int j;
	int k=0,l=0;
	size_t m;
	alex_kupriianov_array<int,MMM,NNN,fast_memory_operator<int> > akai;
	vector<int> vi;
	vector<double> fwd,bckwd;	
	vi.resize(arr*2);
	for(m=0;m<vi.size();m++){
		vi[m]=m;
	}
	int lim=1,log2=1;
	while(lim<maxsz){
		lim<<=1;
		log2++;
	}
	lim=1;
	fwd.resize(log2+1);
	bckwd.resize(log2+1);
	fwd[0]=MSec();
	for(j=1;j<=log2;j++){
		while((int)akai.size()<(1<<j)){
			k=(k+547)%arr;
			l=(l+547)%(akai.size()+1);
			akai.insert(l,&vi[k],&vi[k+arr]);
		}
		fwd[j]=MSec();
	}
	bckwd[log2]=MSec();
	for(j=log2-1;j>=0;j--){
		while((int)akai.size()>(1<<(j))){
			l=(l+547)%(akai.size()-arr+1);
			akai.erase(l,l+arr);
		}
		bckwd[j]=MSec();
	}
	ofs<<"Test by adding and removing groups of "<<arr<<" elems. Time is given per one element.\n";
	ofs<<"Array_size   Insert   Delete\n";
	ofs<<setprecision(2)<<scientific;
	for(j=1;j<log2;j++){
		ofs<<setw(9)<<(1<<j)<<setw(9)<<((fwd[j+1]-fwd[j])/(1<<(j-1)))<<
			setw(9)<<((bckwd[j]-bckwd[j+1])/(1<<(j-1)))<<"\n";
	}
	ofs<<"\n\n\n";	
	cout<<"Test with "<<arr<<" elements completed.\n";
}

void PerformanceTest()
{
	TestDescriptor t1("Now performance tests.");
	{
		alex_kupriianov_array<int,MMM,NNN> akai;
		string s;
		std::ostringstream fname;
		fname<<"results"<<MMM<<"_"<<NNN<<".txt";
		s=fname.str();
		ofstream ofs(s.c_str());
		ofs<<"Experiments with MMM="<<MMM<<" NNN="<<NNN
			<<" sizeof(Branch)="<<akai.__branch_size()
			<<" sizeof(Leaf)="<<akai.__leaf_size()<<"\n";
		ofs<<"vector<int>\n";
		SingleOperationPerformanceCheck<std::vector<int> >(ofs,100000,1000);
		ofs<<"alex_kupriianov_array<int>\n";
		SingleOperationPerformanceCheck<alex_kupriianov_array<int,MMM,NNN> >(ofs,100000,1000);
		ofs<<"vector<int>\n";
		SingleOperationPerformanceCheck<std::vector<int> >(ofs,500,10000);
		ofs<<"deque<int>\n"; 
		SingleOperationPerformanceCheck<std::deque<int> >(ofs,100,10000);
		ofs<<"alex_kupriianov_array<int>\n";
		SingleOperationPerformanceCheck<alex_kupriianov_array<int,MMM,NNN> >(ofs,500,100000);
		ofs<<"alex_kupriianov_array<int, fast_memory_operator>\n";
		SingleOperationPerformanceCheck<alex_kupriianov_array<int,MMM,NNN,fast_memory_operator<int> > >(ofs,100,100000);
		ofs<<"alex_kupriianov_array<int>\n";
		SingleOperationPerformanceCheck<alex_kupriianov_array<int,MMM,NNN> >(ofs,1,10000000);
		MultipleOperationsTest(ofs,5,10000000);
		MultipleOperationsTest(ofs,50,10000000);
		MultipleOperationsTest(ofs,500,10000000);
	}
}

int main()
{
	CorrectnessTestBundle();
	PerformanceTest();
	return 0;
}
