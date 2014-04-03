//  Copyright (C) 2014 by Aleksandr Kupriianov
//  alexkupri at gmail dot com

//  Purpose: fast array declaration

#ifndef ALEXKUPRIIANOVARRAY_H_
#define ALEXKUPRIIANOVARRAY_H_

#include <assert.h>
#include <cstring>

//This memory allocator is used by default. It performs all operations according to the rules.
template <typename T,typename Allocator=std::allocator<T> >
class default_memory_operator:public Allocator
{
public:
	typedef typename Allocator::size_type size_k;
	default_memory_operator():Allocator(){};
	default_memory_operator(const default_memory_operator &that)
		:Allocator(that){};
	template <class U>
	default_memory_operator(const default_memory_operator<U> &that)
		:Allocator(that){};
	//
	void move_elements_inc(T *dst,T *src,size_k num);
	void move_elements_dec(T *dst,T *limit,size_k num);
	void burn_elements(T *ptr,size_k num); 
	template <class InputIterator>
		typename Allocator::size_type fill_elements(T *dst,typename Allocator::size_type num,InputIterator &first,InputIterator last);
	void move_one_element(T *dst,const T &src){construct(dst,src);} 
};

//This memory allocator can be used for simple types (which can be copied as memory). 
template <typename T,typename Allocator=std::allocator<T> >
class fast_memory_operator:public Allocator
{
public:
	typedef typename Allocator::size_type size_k;
	fast_memory_operator():Allocator(){};
	fast_memory_operator(const fast_memory_operator &that)
		:Allocator(that){};
	template <class U>
	fast_memory_operator(const fast_memory_operator<U> &that)
		:Allocator(that){};
	//
	void move_elements_inc(T *dst,T *src,size_k num)
		{memmove(dst,src,num*sizeof(T));}
	void move_elements_dec(T *dst,T *limit,size_k num)
		{memmove(dst,limit,num*sizeof(T));}
	void burn_elements(T *ptr,size_k num){}//nothing 
	template <class InputIterator>
		size_k fill_elements(T *dst,typename Allocator::size_type num,InputIterator &first,InputIterator last);
	size_k fill_elements(T *dst,typename Allocator::size_type num,
		const T *&first,const T *last);
	void move_one_element(T *dst,const T &src){*dst=src;} 
};

template <typename T,int L,int M,typename A=default_memory_operator<T> >
class alex_kupriianov_array
{
public:
	typedef typename A::size_type size_k;
private:
	//data types
	//In the whole library Node* can be cast to either Branch* or Leaf*.
	//This is determined entirely via depth variable.
	struct Branch;
	struct Node
	{
		Branch *parent;
	};
	struct Branch:public Node
	{
		Node* children[L];
		size_k nums[L];
		size_k fillament;
	};
	struct Leaf:public Node
	{
		size_k fillament;
		T elements[M];
	};
    typedef typename A::template rebind<Branch>::other Branch_alloc_type;
    typedef typename A::template rebind<Leaf>::other Leaf_alloc_type;
    typedef A alloc_type;
	A T_alloc;
	Branch_alloc_type branch_alloc;
	Leaf_alloc_type leaf_alloc;
	size_k depth,count;
	Node *root;
	//find and read functions
	size_k  find_leaf(Leaf *&l,size_k pos)const;
	size_k  find_leaf_inc(Leaf *&l,size_k pos,size_k increment);
	size_k  find_leaf_dec(Leaf *&l,size_k pos,size_k decrement);
	size_k  find_child(Branch *b,Node* c);
	//element and leaf helpers
	template <class InputIterator>
		size_k count_difference(InputIterator first,InputIterator last,size_k limit);
	bool try_merge_leaves(Branch *b,size_k idx);
	void balance_leaves_lr(Branch *b,size_k idx);
	void balance_leaves_rl(Branch *b,size_k idx);
	//branch helpers
	void insert_children(Branch *b,size_k idx,size_k num);
	void delete_children(Branch *b,size_k idx,size_k num);
	size_k  move_children(Branch *dst,size_k idst,Branch *src,size_k isrc,size_k num);
	void balance_branch_lr(Branch *b,size_k idx);
	void balance_branch_rl(Branch *b,size_k idx);
	bool try_merge_branches(Branch *b,size_k idx);
	//merge/split and balancing algorithms
	Branch *reserve_enough_branches_splitting(Node* existing);
	void split_root(Node* existing,Node* right_to_existing,size_k right_count,Branch *new_branch);
	void split(Node* existing,Node* right_to_existing,size_k right_count);
	void insert_leaf(Leaf *l,size_k pos);
	void delete_leaf(Leaf *l);
	void underflow(Leaf *l);
	void init_tree();
	void burn_subtree(Node *n,size_k depth);
	void sew_together(size_k pos);
	//helper functions for insert(n)
	template <class InputIterator>
		void start_inserting(Leaf *l,size_k found,size_k &pos,InputIterator &first,InputIterator last);
	template <class InputIterator>
		void insert_whole_leafs(size_k startpos,size_k &pos,InputIterator first,InputIterator last);
	//iterators
	struct IteratorStruct
	{
		const alex_kupriianov_array *array;
		T *current,*first_valid,*last_valid;
		size_k first_valid_index;
		bool is_forward_iterator;
	};
	struct IteratorStructForward:public IteratorStruct
	{
		IteratorStructForward(){this->is_forward_iterator=true;}
	};
	struct IteratorStructBackward:public IteratorStruct
	{
		IteratorStructBackward(){this->is_forward_iterator=false;}
	};
	void set_iterator_struct(IteratorStruct &s,size_k pos)const;
	static void set_iterator_struct_null(IteratorStruct &s);
	static void increment_iterator_struct(IteratorStruct &s);
	static void decrement_iterator_struct(IteratorStruct &s);
	static size_k  get_position(IteratorStruct &s)
		{
			return (s.current-s.first_valid)+s.first_valid_index;
		}
	class FillIterator
	{
		const T* ptr;
		size_k j;
	public:
	//ok with default fns: op=, ~, copy cn 
		FillIterator(const T *ptrptr,size_k jj):ptr(ptrptr),j(jj){};
		const T& operator*(){return *ptr;}
		FillIterator &operator++(){j++;return *this;}
		bool operator==(const FillIterator &that)const
			{ return j==that.j;	}
		bool operator!=(const FillIterator &that)const
			{ return j!=that.j;	}
	};
	//others
	void check_node(Node* c,size_k sum,bool nothead,size_k depth,Branch *parent);
	template <class output_stream>
		void output_node(output_stream &o,Node* c,size_k tabs,size_k depth);
	static void my_assert(bool,const char*);	 	
public:
	//public funcs, except constructors, destruct, oper=
	bool is_empty()const{return count==0;}
	size_k size()const{return count;}
	T& operator [](size_k pos)
	{
		Leaf *l=(Leaf*)root;
		size_k found=depth?find_leaf(l,pos):pos;
		return l->elements[found];
	}
	const T& operator [](size_k pos)const
	{
		Leaf *l=(Leaf*)root;
		size_k found=depth?find_leaf(l,pos):pos;
		return l->elements[found];
	}
	void insert(size_k pos,const T& t);
	template <class InputIterator>
		void insert(size_k pos,InputIterator first,InputIterator last);
	void erase(size_k pos);
	void erase(size_k first,size_k last);
	void clear();
	void fill(size_k pos,size_k repetition,const T& t)
	{
		FillIterator first(&t,0),last(&t,repetition);
		insert(pos,first,last);
	}
	//some debug functions
	void __check_consistency();
	template <class output_stream>
		void __output(output_stream &o,const char *comm="");
	int __branch_size(){return sizeof(Branch);}
	int __leaf_size(){return sizeof(Leaf);}
//=========== This is the end of main part of container itself =============
//=== All the rest is definitions of iterators and constr/destr/op= ====	
	// Iterators : too many syntactic sugar,
	// let us not to get ill with syntactic diabetes
	// iterators are ok with default op= and destructor
	//=============  iterator ==================
	class iterator
	{
		IteratorStructForward s;
		iterator(alex_kupriianov_array<T,L,M,A> *a):s()
			{ s.current=s.last_valid=s.first_valid=0; s.first_valid_index=a->count;}
	public:
	    static iterator end_iterator(alex_kupriianov_array<T,L,M,A> *a)
	        {return iterator(a);}
		iterator():s(){set_iterator_struct_null(s);}
		iterator(alex_kupriianov_array<T,L,M,A> *a,size_k pos):s()
			{ a->set_iterator_struct(s,pos); }
		iterator(const iterator &it)
			:s(it.s) {}
		bool operator==(const iterator &that)const
			{return s.current==that.s.current;}
		bool operator!=(const iterator &that)const
			{return s.current!=that.s.current;}
		T& operator*()
			{return *s.current;}
		T* operator->()
			{return s.current;}
		iterator &operator++()
			{increment_iterator_struct(s);return *this;}
		iterator operator++(int)
			{
				iterator copy(*this);
				increment_iterator_struct(s);
				return copy;
			}
		iterator &operator--()
			{decrement_iterator_struct(s);return *this;}
		iterator operator--(int)
			{
				iterator copy(*this);
				decrement_iterator_struct(s);
				return copy;
			}
		size_k GetAbsolutePosition()
			{return get_position(s);}
	};
	iterator begin()
		{return iterator(this,0);}
	iterator end()
		{return iterator::end_iterator(this);}
	iterator at(size_k pos)
		{return iterator(this,pos);}
	//=============  const_iterator ==================
	class const_iterator
	{
		IteratorStructForward s;
		const_iterator(const alex_kupriianov_array<T,L,M,A> *a):s()
			{ s.current=s.last_valid=s.first_valid=0; s.first_valid_index=a->count;}
	public:
	    static const_iterator end_iterator(const alex_kupriianov_array<T,L,M,A> *a)
	        {return const_iterator(a);}
		const_iterator():s(){SetIteratorStructNull(&s);}
		const_iterator(const alex_kupriianov_array<T,L,M,A> *a,size_k pos):s()
			{ a->set_iterator_struct(s,pos); }
		const_iterator(const const_iterator &it)
			:s(it.s) {}
		bool operator==(const const_iterator &that)const
			{return s.current==that.s.current;}
		bool operator!=(const const_iterator &that)const
			{return s.current!=that.s.current;}
		const T& operator*()
			{return *s.current;}
		const T* operator->()
			{return s.current;}
		const_iterator &operator++()
			{increment_iterator_struct(s);return *this;}
		const_iterator operator++(int)
			{
				const_iterator copy(*this);
				increment_iterator_struct(s);
				return copy;
			}
		const_iterator &operator--()
			{decrement_iterator_struct(s);return *this;}
		const_iterator operator--(int)
			{
				const_iterator copy(*this);
				decrement_iterator_struct(s);
				return copy;
			}
		size_k GetAbsolutePosition()
			{return get_position(s);}
	};
	const_iterator begin()const
		{return const_iterator(this,0);}
	const_iterator end()const
		{return const_iterator::end_iterator(this);}
	const_iterator at(size_k pos)const
		{return iterator(this,pos);}
	//=============  reverse_iterator ==================
	class reverse_iterator
	{
		IteratorStructBackward s;
		reverse_iterator(alex_kupriianov_array<T,L,M,A> *a):s()
			{ s.current=s.last_valid=s.first_valid=0; s.first_valid_index=a->count+1;}
	public:
	    static reverse_iterator end_iterator(alex_kupriianov_array<T,L,M,A> *a)
	        {return reverse_iterator(a);}
		reverse_iterator():s(){SetIteratorStructNull(&s);}
		reverse_iterator(alex_kupriianov_array<T,L,M,A> *a,size_k pos):s()
			{ a->set_iterator_struct(s,pos); }
		reverse_iterator(const reverse_iterator &it)
			:s(it.s) {}
		bool operator==(const reverse_iterator &that)const
			{return s.current==that.s.current;}
		bool operator!=(const reverse_iterator &that)const
			{return s.current!=that.s.current;}
		T& operator*()
			{return *s.current;}
		T* operator->()
			{return s.current;}
		reverse_iterator &operator++()
			{decrement_iterator_struct(s);return *this;}
		reverse_iterator operator++(int)
			{
				reverse_iterator copy(*this);
				decrement_iterator_struct(s);
				return copy;
			}
		reverse_iterator &operator--()
			{increment_iterator_struct(s);return *this;}
		reverse_iterator operator--(int)
			{
				reverse_iterator copy(*this);
				increment_iterator_struct(s);
				return copy;
			}
		size_k GetAbsolutePosition()
			{return get_position(s);}
	};
	reverse_iterator rbegin()
		{return reverse_iterator(this,count-1);}
	reverse_iterator rend()
		{return reverse_iterator::end_iterator(this);}
	reverse_iterator rat(size_k pos)
		{return reverse_iterator(this,pos);}
	//=============  const_reverse_iterator ==================
	class const_reverse_iterator
	{
		IteratorStructBackward s;
		const_reverse_iterator(const alex_kupriianov_array<T,L,M,A> *a):s()
			{ s.current=s.last_valid=s.first_valid=0; s.first_valid_index=a->count+1;}
	public:
	    static const_reverse_iterator end_iterator(const alex_kupriianov_array<T,L,M,A> *a)
	        {return const_reverse_iterator(a);}
		const_reverse_iterator():s(){SetIteratorStructNull(&s);}
		const_reverse_iterator(const alex_kupriianov_array<T,L,M,A> *a,size_k pos):s()
			{ a->set_iterator_struct(s,pos); }
		const_reverse_iterator(const const_reverse_iterator &it)
			:s(it.s) {}
		bool operator==(const const_reverse_iterator &that)const
			{return s.current==that.s.current;}
		bool operator!=(const const_reverse_iterator &that)const
			{return s.current!=that.s.current;}
		const T& operator*()
			{return *s.current;}
		const T* operator->()
			{return s.current;}
		const_reverse_iterator &operator++()
			{decrement_iterator_struct(s);return *this;}
		const_reverse_iterator operator++(int)
			{
				const_reverse_iterator copy(*this);
				decrement_iterator_struct(s);
				return copy;
			}
		const_reverse_iterator &operator--()
			{increment_iterator_struct(s);return *this;}
		const_reverse_iterator operator--(int)
			{
				const_reverse_iterator copy(*this);
				increment_iterator_struct(s);
				return copy;
			}
		size_k GetAbsolutePosition()
			{return get_position(s);}
	};
	const_reverse_iterator rbegin()const
		{return const_reverse_iterator(this,count-1);}
	const_reverse_iterator rend()const
		{return const_reverse_iterator::end_iterator(this);}
	const_reverse_iterator rat(size_k pos)const
		{return const_reverse_iterator(this,pos);}
//========= Constructors, destructor,operator= ==============
	explicit alex_kupriianov_array(const alloc_type &alloc=alloc_type())
		:T_alloc(alloc),branch_alloc(alloc),leaf_alloc(alloc)
	{
		depth=count=0;
	};
	alex_kupriianov_array(const alex_kupriianov_array<T,L,M,A> &that)
		:T_alloc(that.T_alloc),branch_alloc(that.T_alloc),leaf_alloc(that.T_alloc)
	{
		const_iterator first=that.begin(),last=that.end();
		depth=count=0;
		insert(0,first,last);
	}
	template <typename Iterator>
	alex_kupriianov_array(Iterator first,Iterator last,
		const alloc_type &alloc=alloc_type())
		:T_alloc(alloc),branch_alloc(alloc),leaf_alloc(alloc)
	{
		depth=count=0;
		insert(0,first,last);
	}
	alex_kupriianov_array &operator=(const alex_kupriianov_array<T,L,M,A> &that)
	{
		const_iterator first=that.begin(),last=that.end();
		clear();
		insert(0,first,last);
		return *this;
	}
	~alex_kupriianov_array(){clear();}
};

#include "alex_kupriianov_array2.h"

#endif /*ALEXKUPRIIANOVARRAY_H_*/
