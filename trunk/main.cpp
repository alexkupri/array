//  Copyright (C) 2014 by Aleksandr Kupriianov
//  email: alexkupri host: gmail dot com

//  Purpose: fast array tests and usage examples
//  See documentation at http://alexkupri.github.io/array/

// Distributed under the Boost Software License, Version 1.0.
//    (See the file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//#define enable_nomem_tests
//#define use_linux_timer
//#define use_linux_mallinfo
//#define test_vstadnik_container
//#define test_cxx_rope

#include <stdlib.h>
#include <deque> 
#include <vector>
#include <list>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include "btree_seq.h"
 
using namespace std;

int artific_exceptions=0;
enum {MM=4,NN=4};

#define next_rand(aa)  {unsigned long long int val=aa; val=val*val; val=val%4098342361ULL; aa=val;}

#ifdef use_linux_mallinfo
#include <malloc.h>
int UsedMem()
{
	struct mallinfo a=mallinfo();
	return a.uordblks;
}
#define memleak_str "Also checking for memory leaks."
#else
int UsedMem(){return 0;}
#define memleak_str "Not using memory leak detection (currently available on linux/gnuc)."
#endif

#ifdef use_linux_timer
#include <sys/timeb.h>
double MSec()
{
	struct timeb t1;
	ftime(&t1);
	return t1.time*1000.0+t1.millitm;
}
double TimeQuant(){return 1;}
#define time_str "Calculating time, precision is milliseconds."
#else
#include <ctime>
double MSec()
{
	clock_t t=clock();
	return t*1000.0/CLOCKS_PER_SEC;
}
double __time_quant=-1;
double TimeQuant()
{
	if(__time_quant<0){
		int j;
		clock_t prev,cur;
		double val=1e50,tmp;
		for(j=0;j<100;j++){
			prev=clock();
			do{
				cur=clock();
			}while(cur==prev);
			tmp=1000.0*((double)cur-prev)/CLOCKS_PER_SEC;
			if(tmp<val){
				val=tmp;
			}
		}
		__time_quant=val;
	}
	return __time_quant;
}
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
template <typename T,typename Alloc=std::allocator<T> >
class MultipleChecker
{
	btree_seq<T,MM,NN,Alloc> aka;
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
	aka.erase(pos,pos+1);
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
				//cout<<"single ins\n";
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
				t.erase(n,n+1);
			}
		}else if(k<im){
			if((n<=t.size())&&(t.size()<SZ)&&(l<=m)){
				if(p<edge){
					n=0;
				}
				if(q<edge){
					n=t.size();
				}
				//cout<<"multiple ins\n";
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
		"of four operations with vector and btree_seq.");
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

void SubTest_Iterators(vector<int> &vi,btree_seq<int,MM,NN> &aka)
{
	int j;
	size_t k=0;
	vector<int>::iterator vii=vi.begin(),viend=vi.end();
	//aka.__prepare_list();
	btree_seq<int,MM,NN>::iterator akai=aka.begin(),akaend=aka.end(),empty;
	for(;;){
		assert(*akai==*vii);
		assert(akai.get_position()==k);
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
	const btree_seq<int,MM,NN> &caka=aka;
	btree_seq<int,MM,NN>::iterator
		mut_fwd_1=aka.begin(),mut_fwd_2=aka.end();
	btree_seq<int,MM,NN>::const_iterator
		con_fwd_1=caka.begin(),con_fwd_2=caka.end();
	btree_seq<int,MM,NN>::reverse_iterator
		mut_bck_1=aka.rbegin(),mut_bck_2=aka.rend();
	btree_seq<int,MM,NN>::const_reverse_iterator
		con_bck_1=caka.rbegin(),con_bck_2=caka.rend();
	assert(SumIters(mut_fwd_1,mut_fwd_2)==SumIters(vi.begin(),vi.end()));
	assert(SumIters(mut_bck_1,mut_bck_2)==SumIters(vi.begin(),vi.end()));
	assert(SumIters(con_fwd_1,con_fwd_2)==SumIters(vi.begin(),vi.end()));
	assert(SumIters(con_bck_1,con_bck_2)==SumIters(vi.begin(),vi.end()));	
}

class SumVisitor
{
	int sum;
public:
	SumVisitor():sum(0){};
	bool operator()(int v){sum+=v;return false;}
	int get_sum(){return sum;}
};

class FindVisitor
{
	int val;
public:
	FindVisitor(int valval):val(valval){};
	bool operator()(int v){return v==val;}
};

void SubTest_Visitors(btree_seq<int,MM,NN> &aka)
{
	int sum=0,val=rand()%42;
	size_t found,j;
	size_t v1=rand()%(aka.size()+1),v2=rand()%(aka.size()+1);
	if(v1>v2){
		swap(v1,v2);
	}
	if((rand()&7)==7){
		v1=0;
	}
	if((rand()&7)==7){
		v2=aka.size();
	}
	if((rand()&7)==7){
		v1=v2;
	}
	FindVisitor fv(val);
	SumVisitor sv;
	found=aka.visit(v1,v2,fv);
	assert(aka.visit(v1,v2,sv)==v2);
	for(j=v1;j<v2;j++){
		sum+=aka[j];
	}
	assert(sum==sv.get_sum());
	for(j=v1;j<found;j++){
		if(aka[j]==val){
			aka.__output(cout);
			cout<<"v1="<<v1<<" v2="<<v2<<" val="<<val<<" found="<<found<<"\n";
		}
		assert(aka[j]!=val);
	}
	assert(found<=v2);
	if(found<v2){
		assert(aka[found]==val);
	}
}

void IteratorsTest_Int()
{
	TestDescriptor t1("Test with iterators and visitors.");
	{
		int j,k,l,m;
		vector<int> vi1,vi2;
		btree_seq<int,MM,NN> aka1,aka2;
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
			//aka1.__prepare_list();
			aka2.insert(m,aka1.iterator_at(k),aka1.iterator_at(l));
			vi2.insert(vi2.begin()+m,vi1.begin()+k,vi1.begin()+l);
			SubTest_Iterators(vi2,aka2);
			SubTest_Visitors(aka2);
		}
	}	
}

void TestFill_Int()
{
	TestDescriptor t1("Test with fill functions (iterators being checked).");
	{
		size_t j;
		btree_seq<int,MM,NN> aka1,aka2;
		aka1.fill(0,40,1);
		aka1.fill(0,40,0);
		aka1.fill(80,40,2);
		aka2.fill(0,100,3);
		aka2=aka1;
		btree_seq<int,MM,NN> aka3(aka2.rbegin(),aka2.rend());
		btree_seq<int,MM,NN> aka4(aka3);
		assert(aka1.size()==aka2.size());
		assert(aka1.size()==aka3.size());
		assert(aka1.size()==aka4.size());
		for(j=0;j<aka1.size();j++){
			assert(aka1[j]==aka2[j]);
			assert(aka1[j]==aka3[aka1.size()-1-j]);
			assert(aka3[j]==aka4[j]);
		}
		int k[10]={1,2,3,4,5,6,7,8,9,10};
		std::list<int> li(k,k+10);
		btree_seq<int,MM,NN> aka5(li.begin(),li.end());
		assert(aka5.size()==10);
		for(j=0;j<aka5.size();j++){
			assert(aka5[j]==(int)j+1);
		}
		vector<IntContainer> vic;
		vic.resize(50);
		btree_seq<IntContainer,MM,NN> aka6(vic.begin(),vic.end());
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
				btree_seq<IntContainer,MM,NN> aka(vi1.begin(),vi1.end());
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

class NormalTest
{
public:
	typedef btree_seq<IntContainer,4,4> container;
	static void SetProb(double){};
};
	
template <typename TestType>
void AttachTest()
{
	int exceptions=0;
	TestDescriptor t1("Attach test.");
	{
		int n=120,j,k,l;
		vector<IntContainer> vi;
		vi.resize(n);
		for(j=0;j<n;j++){
			vi[j].set(j);
		}
		for(j=0;j<n;j++){
			for(k=0;j+k<n;k++){
				TestType::SetProb(0);
				typename TestType::container a1(vi.begin(),vi.begin()+j),
					a2(vi.begin()+j,vi.begin()+j+k);
				TestType::SetProb(0.05);
			retry:
				try{
					a1.concatenate_right(a2);
				}catch(...){
					a1.__check_consistency();
					a2.__check_consistency();
					exceptions++;
					goto retry;
				}
				a1.__check_consistency();
				a2.__check_consistency();
				assert(a1.size()==j+k);
				assert(a2.size()==0);
				for(l=0;l<j+k;l++){
					assert(a1[l].get()==l);
				}
			}
		}
		cout<<"Exceptions: "<<exceptions<<"\n";
	}
}

template <typename TestType>
void DetachTest()
{
	int exceptions=0;
	TestDescriptor t1("Detach test.");
	{
		int n=120,j,k,l;
		vector<IntContainer> vi;
		vi.resize(n);
		for(j=0;j<n;j++){
			vi[j].set(j);
		}
		for(j=0;j<n;j++){
			for(k=0;k<=j;k++){
				TestType::SetProb(0);
				typename TestType::container a1(vi.begin(),vi.begin()+j),
					a2;
				TestType::SetProb(0.05);
			retry:
				try{
					a1.split_right(a2,k);
				}catch(...){
					a1.__check_consistency();
					exceptions++;
					goto retry;
				}
				TestType::SetProb(0);
				a1.__check_consistency();
				a2.__check_consistency();
				assert(a1.size()==k);
				assert(a2.size()==j-k);
				for(l=0;l<k;l++){
					assert(a1[l].get()==l);
				}								
				for(l=0;l<j-k;l++){
					assert(a2[l].get()==l+k);
				}								
			}
		}
		cout<<"Exceptions: "<<exceptions<<"\n";
	}	
}

void LeftTests()
{
	TestDescriptor t1("Left attach/detach test.");
	{
		int ia[10]={0,1,2,3,4,5,6,7,8,9},m;
		btree_seq<int,MM,NN> a3(ia,ia+5),a4(ia+5,ia+10);
		a4.concatenate_left(a3);
		assert(a4.size()==10);
		for(m=0;m<10;m++){
			assert(a4[m]==m);
		}
		a4.split_left(a3,8);
		assert(a3.size()==8);
		for(m=0;m<8;m++){
			assert(a3[m]==m);
		}
		assert(a4.size()==2);
		for(m=0;m<2;m++){
			assert(a4[m]==m+8);
		}
	}
}

void ResizeTest()
{
	TestDescriptor t1("Resize test.");
	{
		btree_seq<int,MM,NN> a1;
		a1.resize(2);
		a1[0]=a1[1]=1;
		a1.resize(4,2);
		assert((a1.size()==4)&&(a1[0]==1)&&(a1[1]==1)&&(a1[2]==2)&&(a1[3]==2));
		a1.resize(3,5);
		assert((a1.size()==3)&&(a1[0]==1)&&(a1[1]==1)&&(a1[2]==2));
	}
}

void AccessTest()
{
	TestDescriptor t1("Resize test.");
	{
		int ii[6]={0,1,2,3,4,5};
		bool exception_occured=false;
		btree_seq<int,MM,NN> a(ii,ii+6);
		assert(a.at(3)==3);
		assert(a.front()==0);
		assert(a.back()==5);
		try{
			a.at(8)=8;
		}catch(...){
			exception_occured=true;
		}
		assert(exception_occured);
	}
}

struct S2{
	char c;
	int x;
};

void AssignTest()
{
	TestDescriptor t1("Assign test.");
	{
		int ii[6]={0,1,2,3,4,5};
		btree_seq<int,MM,NN> a;
		btree_seq<int> b(3,4);
		assert((b.size()==3)&&(b[0]==4)&&(b[1]==4)&&(b[2]==4));
		a.assign(ii,ii+6);
		assert((a.size()==6)&&(a[0]==0)&&(a[5]==5));
		a.assign(3,-1);
		assert((a.size()==3)&&(a[0]==-1)&&(a[1]==-1)&&(a[2]==-1));
		btree_seq<S2,MM,NN> aks1,aks2;
		S2 s2={'z',6},s3={'q',7};
		aks1.assign(3,s2);
		aks1.assign(3,s3);
		assert((aks1.size()==3)&&(aks1[0].x==7)&&(aks1[1].x==7)&&(aks1[2].x==7));
		aks1.resize(4,s2);
		aks2.assign(aks1.begin(),aks1.end());
		assert((aks2.size()==4)&&(aks2[0].x==7)&&(aks2[1].x==7)&&(aks2[2].x==7)&&(aks2[3].x==6));
	}
}

void PushPopTest()
{
	TestDescriptor t1("PushPop test.");
	{
		int ii[3]={1,2,3};
		btree_seq<int,MM,NN> a(ii,ii+3);
		assert((a.size()==3)&&(a[0]==1)&&(a[1]==2)&&(a[2]==3));
		a.push_front(0);
		assert((a.size()==4)&&(a[0]==0)&&(a[1]==1)&&(a[2]==2)&&(a[3]==3));
		a.push_back(4);
		assert((a.size()==5)&&(a[0]==0)&&(a[1]==1)&&(a[2]==2)&&(a[3]==3)&&(a[4]==4));
		a.pop_front();
		assert((a.size()==4)&&(a[0]==1)&&(a[1]==2)&&(a[2]==3)&&(a[3]==4));
		a.pop_back();
		assert((a.size()==3)&&(a[0]==1)&&(a[1]==2)&&(a[2]==3));
	}
}

void InsertIteratorTest()
{
	TestDescriptor t1("Insert via iterator test.");
	{
		int ii[3]={10,20,30};
		btree_seq<int,MM,NN> a(ii,ii+3);
		btree_seq<int,MM,NN>::iterator it,i2;
		btree_seq<int,MM,NN>::const_iterator cit,ci2;
		it=a.begin();
		i2=it;
		cit=it;
		//it=cit; //must not compile
		btree_seq<int,MM,NN>::iterator it3(it)/*,it4(cit)*/;
		btree_seq<int,MM,NN>::const_iterator ci3(it),ci4(cit);
		assert((a.size()==3)&&(a[0]==10)&&(a[1]==20)&&(a[2]==30));
		//
		it=a.insert(a.begin()+2,21);
		assert(*it==21);
		assert((a.size()==4)&&(a[0]==10)&&(a[1]==20)&&(a[2]==21)&&(a[3]==30));
		//
		it=a.insert(a.begin()+3,2,22);
		assert(*it==22);
		assert((a.size()==6)&&(a[0]==10)&&(a[1]==20)&&(a[2]==21)&&(a[3]==22)&&(a[4]==22)&&(a[5]==30));
		//
		it=a.insert(a.begin(),ii,ii+3);
		assert(*it==10);
		assert((a.size()==9)&&(a[0]==10)&&(a[1]==20)&&(a[2]==30)&&(a[3]==10));
	}
}

void EraseIteratorTest()
{
	TestDescriptor t1("Erase test.");
	{
		int ii[6]={0,1,2,3,4,5};
		btree_seq<int,MM,NN> a(ii,ii+6);
		btree_seq<int,MM,NN>::iterator it;
		it=a.erase(a.begin()+1);
		assert(*it==2);
		it=a.erase(a.begin()+2,a.begin()+4);
		assert(*it==5);
		assert((a.size()==3)&&(a[0]==0)&&(a[1]==2)&&(a[2]==5));
	}
}

void RelationsTest()
{
	TestDescriptor t1("Relations test.");
	{
		btree_seq<int> a(3,0),b(2,1);
		//
		assert((a==a)==true);
		assert((a==b)==false);
		assert((b==a)==false);
		//
		assert((a!=a)==false);
		assert((a!=b)==true);
		assert((b!=a)==true);
		//
		assert((a<a)==false);
		assert((a<b)==true);
		assert((b<a)==false);
		//
		assert((a<=a)==true);
		assert((a<=b)==true);
		assert((b<=a)==false);
		//
		assert((a>a)==false);
		assert((a>b)==false);
		assert((b>a)==true);
		//
		assert((a>=a)==true);
		assert((a>=b)==false);
		assert((b>=a)==true);
	}
}

#ifdef enable_nomem_tests
#include <ext/throw_allocator.h>
void TestNoMemExceptions()
{
	TestDescriptor t1("Test imitating nomemory exception.");
	{
		__gnu_cxx::random_condition::set_probability(0.01);
		//__gnu_cxx::throw_allocator<IntContainer> t;t.init(10);
		MultipleChecker<IntContainer,__gnu_cxx::throw_allocator_random<IntContainer> > mc;
		vector<IntContainer> vi;
		int j,n=60;
		vi.resize(n);
		for(j=0;j<n;j++){
			vi[j].set(j);
		}
		PerformCheck(mc,vi,600000,75,85,95,98,10);
		PerformCheck(mc,vi,600000,75,85,95,98,50);
		PerformCheck(mc,vi,600000,50,60,90,100,10);
		PerformCheck(mc,vi,600000,50,95,95,95,10);
		PerformCheck(mc,vi,600000,75,85,95,98,10);
	}
}
class ExceptionTest
{
public:
	typedef btree_seq<IntContainer,4,4,__gnu_cxx::throw_allocator_random<IntContainer> > container;
	static void SetProb(double d){__gnu_cxx::random_condition::set_probability(d);}
};
void AttachExceptionTest()
{
	AttachTest<ExceptionTest>();
}
void DetachExceptionTest()
{
	DetachTest<ExceptionTest>();
}
#else
void TestNoMemExceptions()
{
	cout<<"The general no-memory test is switched off.\n";
}
void AttachExceptionTest()
{
	cout<<"The attach no-memory test is switched off.\n";
}
void DetachExceptionTest()
{
	cout<<"The detach no-memory test is switched off.\n";
}
#endif

void CorrectnessTestBundle()
{
	//Functionality tests
	BasicTest_Int();
	BasicTest_IntContainer();	
	IteratorsTest_Int();
	TestFill_Int();
	AttachTest<NormalTest>();
	DetachTest<NormalTest>();
	LeftTests();
	cout<<"\n";

	//Syntactic sugar tests
	ResizeTest();
	AccessTest();
	AssignTest();
	PushPopTest();
	InsertIteratorTest();
	EraseIteratorTest();
	RelationsTest();
	cout<<"\n";

	//Exception tests
	TestNoMemExceptions();
	TestCopyExceptions();
	AttachExceptionTest();
	DetachExceptionTest();
	//
	cout<<"\nAll tests passed. Success.\n";
}

//=============  Manual test ===================

void DumpAAI(btree_seq<int,MM,NN> &aai)
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
	btree_seq<int,MM,NN> aai;
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
				aai.erase(idx,idx+1);
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

inline void insert_val(std::vector<int> &vi,int pos,int val)
{
	vi.insert(vi.begin()+pos,val);
} 

inline void insert_val(std::deque<int> &vi,int pos,int val)
{
	vi.insert(vi.begin()+pos,val);
} 

inline void insert_val(btree_seq<int> &akai,int pos,int val)
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

inline void erase_val(btree_seq<int> &akai,int pos)
{
	akai.erase(akai.begin()+pos);
} 

#ifdef test_cxx_rope
#include <ext/rope>
inline void insert_val(__gnu_cxx::rope<int> &vi,int pos,int val)
{
	vi.insert(pos,val);
}
inline void erase_val(__gnu_cxx::rope<int> &vi,int pos)
{
	vi.erase(pos,1);
}
int Visiting(__gnu_cxx::rope<int> &vi)
{
	int res=0;
	__gnu_cxx::rope<int>::const_iterator vb=vi.begin(),ve=vi.end();
	while(vb!=ve){
		res+=*vb;
		vb++;
	}
	return res;
}
#endif

#ifdef test_vstadnik_container
#include "bpt_sequence.hpp"
inline void insert_val(std_ext_adv::sequence<int> &vi,int pos,int val)
{
	vi.insert(vi.begin()+pos,val);
}

inline void erase_val(std_ext_adv::sequence<int> &akai,int pos)
{
	akai.erase(akai.begin()+pos);
}
int Visiting(std_ext_adv::sequence<int> &vi)
{
	int res=0;
	std_ext_adv::sequence<int>::const_iterator vb=vi.begin(),ve=vi.end();
	while(vb!=ve){
		res+=*vb;
		vb++;
	}
	return res;
}
#endif

template <class IntContainer>
int SumArray(IntContainer &ic)
{
	int res=0;
	typename IntContainer::const_iterator it;
	for(it=ic.begin();it!=ic.end();it++){
		res+=(*it);
	}
	return res;
}

int Visiting(btree_seq<int> &aka)
{
	SumVisitor sv;
	aka.visit(0,aka.size(),sv);
	return sv.get_sum();
} 

int Visiting(vector<int> &vi)
{
	int res=0;
	vector<int>::iterator vb=vi.begin(),ve=vi.end();
	while(vb!=ve){
		res+=*vb;
		vb++;
	}
	return res;
} 

int Visiting(deque<int> &vi)
{
	int res=0;
	deque<int>::iterator vb=vi.begin(),ve=vi.end();
	while(vb!=ve){
		res+=*vb;
		vb++;
	}
	return res;
} 

double tolerance()
{
	return 0.01;
}

class TimeVal
{
	double sum,squared_sum;
	int cycles;
	double Error(){return sqrt(fabs(squared_sum/cycles-sum*sum/cycles/cycles))/
			sqrt((double)cycles);}
public:
	void AddValue(double d){sum+=d;squared_sum+=d*d;cycles+=1;/*Dump();*/}
	TimeVal():sum(0),squared_sum(0),cycles(0){};
	double GetResult(){return sum/cycles;}
	bool Valid(){return (cycles>=3)
					&&(sum*tolerance()>TimeQuant())
					/*&&(Error()<GetResult()*tolerance())*/;}
	void Dump(){cout<<setw(15)<<sum<<" "<<setw(15)<<GetResult()<<" "<<setw(15)<<Error()<<(Valid()?"y":"n")<<"\n";}
};

template <class Container >
void DoSingleOperationPerformanceCheck(ostream &os,int elements,int log2)
{
	int  (*func_var)(Container&)=&SumArray;
	void *ptr=(void*)func_var;
	int cursz,j,sum3=0,sum4=0;
	int upper=1<<log2,lower=upper/2;
	unsigned int place=2,idx;
	typename std::vector<Container>::iterator it;
	typename Container::const_iterator subit;
	int sum1=0,sum2=0;
	int cycles=0;
	double t_start1,t_ins,t_readrnd,t_readiter,t_fastsum,t_del;
	TimeVal res_ins,res_del,res_rand,res_sum,res_iter;
	while(!((res_ins.Valid())&&(res_del.Valid())&&(res_rand.Valid())&&
			(res_sum.Valid())&&(res_iter.Valid()))){
		std::vector<Container> vecCon;
		vecCon.resize(elements);
		cursz=0;
		// Growing until lower limit reached
		while(cursz<lower){
			cursz++;
			for(it=vecCon.begin();it!=vecCon.end();it++){
				next_rand(place);
				idx=place%cursz;
				insert_val(*it,idx,idx);
			}
		}
		//Inserting until upper limit reached
		t_start1=MSec();
		while(cursz<upper){
			cursz++;
			for(it=vecCon.begin();it!=vecCon.end();it++){
				next_rand(place);
				idx=place%cursz;
				insert_val(*it,idx,idx);
			}
		}
		t_ins=MSec();
		//Accessing randomly
		for(j=0;j<cursz;j++){
			next_rand(place);
			idx=place%cursz;
			for(it=vecCon.begin();it!=vecCon.end();it++){
				sum1+=(*it)[idx];
			}
		}
		t_readrnd=MSec();
		//Accessing via iterator
		for(it=vecCon.begin();it!=vecCon.end();it++){
			for(subit=(*it).begin();subit!=(*it).end();subit++){
				sum2+=(*subit);
			}
		}
		t_readiter=MSec();
		//Accessing via visitor
		for(it=vecCon.begin();it!=vecCon.end();it++){
			sum4+=Visiting(*it);
		}
		t_fastsum=MSec();
		//Now deleting elements one by one, until lower limit reached
		while(cursz>lower){
			for(it=vecCon.begin();it!=vecCon.end();it++){
				next_rand(place);
				idx=place%cursz;
				erase_val(*it,idx);
			}
			cursz--;
		}
		t_del=MSec();
		// Adding values
		res_ins.AddValue(t_ins-t_start1);
		res_rand.AddValue(t_readrnd-t_ins);
		res_iter.AddValue(t_readiter-t_readrnd);
		res_sum.AddValue(t_fastsum-t_readiter);
		res_del.AddValue(t_del-t_fastsum);
		cycles++;
	}
	cout<<"Our dummies "<<sum1<<" "<<sum2<<" "<<sum3<<" "<<sum4<<" "<<ptr<<"; cycle(s):"<<cycles<<"\n";
	double denom=1.0/((double)(upper))/((double)(elements));//((double)cycles);
	os<<setprecision(2)<<scientific;
	os<<setw(9)<<(upper)<<" "<<setw(12)<<(res_ins.GetResult()*denom*2.0)
			   <<" "<<setw(12)<<(res_del.GetResult()*denom*2.0)
			   <<" "<<setw(12)<<(res_rand.GetResult()*denom)
			   <<" "<<setw(12)<<(res_iter.GetResult()*denom)
			   <<" "<<setw(12)<<(res_sum.GetResult()*denom)<<"\n";
}

template <class Container>
void SingleOperationPerformanceCheck(ostream &os,int elements,int maxsz)
{
	int j=0;
	cout<<"\n\n New series of experiments.\n";
	os.unsetf(ios_base::floatfield);
	os<<"\nAveraged on "<<elements<<" container(s). Timer resolution, msec:"<<TimeQuant()<<"\n";
	os<<"Array size      Insert      Delete     Read_rand     Read_iter    Fast_sum\n";
	while((1<<j)<maxsz){
		DoSingleOperationPerformanceCheck<Container>(os,elements,j);
		j++;
	}
	os<<"\n\n\n";
}

#ifdef test_cxx_rope
void TestRope(ofstream &ofs)
{
	ofs<<"__gnu_cxx::rope<int>\n";
	SingleOperationPerformanceCheck<__gnu_cxx::rope<int> >(ofs,1000,5000);
	ofs<<"__gnu_cxx::rope<int>\n";
	SingleOperationPerformanceCheck<__gnu_cxx::rope<int> >(ofs,10,50000);
}
#else
void TestRope(ofstream &ofs)
{
	ofs<<"Test with __gnu_cxx::rope<int> is disabled. Uncomment line 10 to enable it.\n\n";
}
#endif

#ifdef test_vstadnik_container
void TestVStadnik(ofstream &ofs)
{
	ofs<<"std_ext_adv::sequence<int> (by Vadim Stadnik)\n";
	SingleOperationPerformanceCheck<std_ext_adv::sequence<int> >(ofs,1000,5000);
	ofs<<"std_ext_adv::sequence<int> (by Vadim Stadnik)\n";
	SingleOperationPerformanceCheck<std_ext_adv::sequence<int> >(ofs,10,50000);
}
#else
void TestVStadnik(ofstream &ofs)
{
	ofs<<"Test with Vadim Stadnik's container is disabled. Uncomment line 9 to enable it.\n\n";
}
#endif

void MultipleOperationsTest(ofstream &ofs,int arr,int maxsz)
{
	int j;
	int k=0,l=0;
	size_t m;
	unsigned int place=2;
	btree_seq<int> akai;
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
			next_rand(place);
			k=place%arr;
			next_rand(place);
			l=place%(akai.size()+1);
			akai.insert(l,&vi[k],&vi[k+arr]);
		}
		fwd[j]=MSec();
	}
	bckwd[log2]=MSec();
	for(j=log2-1;j>=0;j--){
		while((int)akai.size()>(1<<(j))){
			next_rand(place);
			l=place%(akai.size()-arr+1);
			akai.erase(l,l+arr);
		}
		bckwd[j]=MSec();
	}
	ofs<<"Test by adding and removing groups of "<<arr<<" elems. Time is given per one element.\n";
	ofs<<"Array_size   Insert   Delete\n";
	ofs<<setprecision(2)<<scientific;
	for(j=1;j<log2;j++){
		if(1<<j<arr){
			continue;
		}
		ofs<<setw(9)<<(1<<j);
		if(fwd[j+1]-fwd[j]>tolerance()*TimeQuant()){
			ofs<<" "<<setw(9)<<((fwd[j+1]-fwd[j])/(1<<(j-1)));
		}else{
			ofs<<"       -  ";
		}
		if(bckwd[j]-bckwd[j+1]>tolerance()*TimeQuant()){
			ofs<<" "<<setw(9)<<((bckwd[j]-bckwd[j+1])/(1<<(j-1)));
		}else{
			ofs<<"       -  ";
		}
		ofs<<"\n";
	}
	ofs<<"\n\n\n";	
	cout<<"Test with "<<arr<<" elements completed.\n";
}

void PerformanceTest()
{
	TestDescriptor t1("Now performance tests.");
	{
		btree_seq<int> akai;
		string s;
		std::ostringstream fname;
		int MMM=akai.__children_in_branch(),NNN=akai.__elements_in_leaf();
		fname<<"results"<<MMM<<"_"<<NNN<<"_r.txt";
		s=fname.str();
		ofstream ofs(s.c_str());
		ofs<<"Experiments with MMM="<<MMM<<" NNN="<<NNN
			<<" sizeof(Branch)="<<akai.__branch_size()
			<<" sizeof(Leaf)="<<akai.__leaf_size()<<"\n";
		ofs<<"vector<int>\n";
		SingleOperationPerformanceCheck<std::vector<int> >(ofs,1000,5000);
		ofs<<"\n\ndeque<int>\n";
		SingleOperationPerformanceCheck<std::deque<int> >(ofs,1000,5000);
		ofs<<"\n\nbtree_seq<int>\n";
 		SingleOperationPerformanceCheck<btree_seq<int> >(ofs,1000,5000);
		ofs<<"\n\nbtree_seq<int>\n";
 		SingleOperationPerformanceCheck<btree_seq<int> >(ofs,10,50000);
 		TestRope(ofs);
 		TestVStadnik(ofs);
		MultipleOperationsTest(ofs,5,10000000);
		MultipleOperationsTest(ofs,50,10000000);
		MultipleOperationsTest(ofs,500,10000000);
	}
}

int main()
{
	cout<<TimeQuant()<<"\n";
	CorrectnessTestBundle();
	PerformanceTest();
	return 0;
}
