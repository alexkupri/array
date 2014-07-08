//  Copyright (C) 2014 by Aleksandr Kupriianov
//  email: alexkupri host: gmail dot com

//  Purpose: fast array declaration
  
#ifndef __BTREE_SEQ_H
#define __BTREE_SEQ_H

#include <assert.h>
#include <iterator>

/** @file btree_seq.h
 * Declaration of btree_seq container, sequence based on btree.
 */

///  @cond HELPERS
// This is private namespace.
namespace ___alexkupri_helpers
{
	struct my_false_type{};
	struct my_true_type{};
	template<typename _Tp> struct my_is_integer  {  typedef my_false_type __type;  };
	template<> struct my_is_integer<bool>  {  typedef my_true_type __type;  };
	template<> struct my_is_integer<char>  {  typedef my_true_type __type;  };
	template<> struct my_is_integer<signed char>  {  typedef my_true_type __type;  };
	template<> struct my_is_integer<unsigned char>  {  typedef my_true_type __type;  };
	template<> struct my_is_integer<short>  {  typedef my_true_type __type;  };
	template<> struct my_is_integer<unsigned short>  {  typedef my_true_type __type;  };
	template<> struct my_is_integer<int>  {  typedef my_true_type __type;  };
	template<> struct my_is_integer<unsigned int>  {  typedef my_true_type __type;  };
	template<> struct my_is_integer<long>  {  typedef my_true_type __type;  };
	template<> struct my_is_integer<unsigned long>  {  typedef my_true_type __type;  };
}
///  @endcond

/// The fast sequence container, which behaves like std::vector takes O(log(N)) to insert/delete elements.
/** This container implements most of std::vector's members. It inserts/deletes elements
 * much faster than any standart container. However, random access to the element takes O(log(N)) time as well.
 * For sequential access to elements, iterators and <code> visit </code> exist, which
 * require practically constant time per element.
 * The implementation is based on btrees.
 * @tparam T the type of the element
 * @tparam L maximal number of children per branch, default 30 minimum 4. You can change it for better performance.
 * @tparam M maximal number of elements per leaf, default 60 minimum 4. You can change it for better performance.
 * @tparam A allocator.
*/
template <typename T,int L=30,int M=60,typename A=std::allocator<T> >
class btree_seq
{
public:
	///The last template parameter, allocator.
	typedef A allocator_type;
	///Value type, T (the first template parameter).
	typedef typename A::value_type value_type;
	///Reference type, T&.
	typedef typename A::reference reference;
	///Constant reference type, const T&.
	typedef typename A::const_reference const_reference;
	///Pointer type, T*.
	typedef typename A::pointer pointer;
	///Constant pointer type, const T*.
	typedef typename A::const_pointer const_pointer;
	///Unsigned integer type, size_t (unsigned int).
	typedef typename A::size_type size_type;
	///Signed integer type, ptr_diff_t (int).
	typedef typename A::difference_type difference_type;
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
		size_type nums[L];
		size_type fillament;
	};
	struct Leaf:public Node
	{
		value_type elements[M];
		size_type fillament;
	};
    typedef typename allocator_type::template rebind<Branch>::other Branch_alloc_type;
    typedef typename allocator_type::template rebind<Leaf>::other Leaf_alloc_type;
	allocator_type T_alloc;
	Branch_alloc_type branch_alloc;
	Leaf_alloc_type leaf_alloc;
	//Data
	Node *root;
	size_type depth,count;
	//Element helper functions
	void move_elements_inc(pointer dst,pointer src,size_type num);
	void move_elements_dec(pointer dst,pointer limit,size_type num);
	template <class InputIterator>
		size_type fill_elements(pointer dst,size_type num,InputIterator &first,InputIterator last,
			std::random_access_iterator_tag);
	template <class InputIterator>
		size_type fill_elements(pointer dst,size_type num,InputIterator &first,InputIterator last,
			std::input_iterator_tag);
	template <class InputIterator>
		size_type count_difference(InputIterator first,InputIterator last,size_type limit,
		std::input_iterator_tag){return limit+1;}
	template <class InputIterator>
		size_type count_difference(InputIterator first,InputIterator last,size_type,
		std::random_access_iterator_tag){return last-first;}
	void burn_elements(pointer ptr,size_type num);
	//branch helpers
	void insert_children(Branch *b,size_type idx,size_type num);
	void delete_children(Branch *b,size_type idx,size_type num);
	size_type move_children(Branch *dst,size_type idst,Branch *src,size_type isrc,size_type num);
	size_type fill_leaves(Branch *b,size_type place,Leaf **l,size_type num);
	//leaf balancing helpers
	bool try_merge_leaves(Branch *b,size_type idx);
	void balance_leaves_lr(Branch *b,size_type idx);
	void balance_leaves_rl(Branch *b,size_type idx);
	void delete_leaf(Leaf *l);
	void underflow_leaf(Leaf *l);
	//branch balancing helpers
	bool try_merge_branches(Branch *b,size_type idx);
	void balance_branch_lr(Branch *b,size_type idx);
	void balance_branch_rl(Branch *b,size_type idx);
	void underflow_branch(Branch *node);
	//find and read functions
	size_type  find_leaf(Leaf *&l,size_type pos)const;
	size_type  find_leaf(Node *&l,size_type pos,difference_type increment,size_type depth_lim=0);
	static size_type  find_child(Branch *b,Node* c);
	//some initialization functions
	void init_tree();
	void increase_depth(Branch *new_branch);
	//splitting algorithms
	Branch *reserve_enough_branches_splitting(Node* existing);
	template <typename Alloc,typename Node_type>
		void prepare_for_splitting(Branch *&branch_bundle,
				Node_type *&result,Node_type *existing,Alloc &alloc);
	void split(Node* existing,Node* right_to_existing,size_type right_count,Branch *branch_bundle);
	void add_child(Branch* parent,Node* inserted,size_type count,size_type pos,size_type rl,Branch *branch_bundle);
	//underflow and sew functions
	void deep_sew(size_type pos);
	void my_deep_sew(size_type pos);
	void advanced_sew_together(Leaf *last_leaf,size_type pos);
	//helper functions and a class for inserting
	template <class InputIterator>
		void insert_no_more_half_leaf
			(size_type pos,InputIterator first,InputIterator last,size_type num);
	template <class InputIterator>
		Leaf* start_inserting(size_type &pos,InputIterator &first,InputIterator last);
	void insert_leaves(Leaf **l,size_type num_leaves,size_type num_elems,size_type pos);
	template <class InputIterator>
		Leaf* insert_whole_leaves(size_type startpos,size_type &pos,InputIterator first,InputIterator last,Leaf *last_leaf);
	class FillIterator
	{
		const_pointer ptr;
		size_type j;
	public:
	//ok with default fns: op=, ~, copy cn
		typedef std::random_access_iterator_tag iterator_category;
		typedef 
			typename std::iterator<std::random_access_iterator_tag, T>::value_type
			value_type;
		typedef
			typename std::iterator<std::random_access_iterator_tag, T>::difference_type
			difference_type;
		typedef 
			typename std::iterator<std::random_access_iterator_tag, T>::reference
			reference;
		typedef 
			typename std::iterator<std::random_access_iterator_tag, T>::pointer
			pointer;	
	
		// 
		FillIterator(const_pointer ptrptr,size_type jj):ptr(ptrptr),j(jj){};
		const_reference operator*(){return *ptr;}
		FillIterator &operator++(){j++;return *this;}
		bool operator==(const FillIterator &that)const
			{ return j==that.j;	}
		bool operator!=(const FillIterator &that)const
			{ return j!=that.j;	}
		difference_type operator-(const FillIterator &that)const
			{return j-that.j;}
	};
	//helper function and classes for delete and iterative access
	template<typename Action>
		bool recursive_action(Action &act,size_type start,size_type diff,size_type depth,Node *node);
	class erase_helper
	{
		Leaf *last_leaf;
		size_type leaves;
		btree_seq &aka;
	public:
		erase_helper(btree_seq &akaaka):last_leaf(0),leaves(0),aka(akaaka){}
		void decrement_value(size_type &a,size_type b){a-=b;}
		bool shift_array(){return true;}
		bool process_leaf(Leaf *l,size_type start,size_type end);
		Leaf *get_last_leaf(){return last_leaf;}
		size_type num_leaves(){return leaves;}
	};
	template<typename V>
	class visitor_helper
	{
		V &v;
		size_type iters;
	public:
		visitor_helper(V &vv):v(vv),iters(0){};
		void decrement_value(size_type &a,size_type b){}
		bool shift_array(){return false;}
		bool process_leaf(Leaf *l,size_type st,size_type fin);
		size_type get_iters(){return iters;}
	};
	//attach and detach helpers
	void detach_some(btree_seq<T,L,M,A> &that,Branch *b,size_type dep,bool last);
	void insert_tree(btree_seq<T,L,M,A> &that,bool last);
	//assign helpers
	template <class Integer>
		void impl_insert(size_type pos,Integer n,Integer val,___alexkupri_helpers::my_true_type)
	{
		fill(pos,n,val);
	}
	template <class InputIterator>
		void impl_insert(size_type pos,InputIterator first,InputIterator last,___alexkupri_helpers::my_false_type)
	{
		insert(pos,first,last);
	}
	//others
	void check_node(Node* c,size_type sum,bool nothead,size_type depth,Branch *parent);
	template <class output_stream>
		void output_node(output_stream &o,Node* c,size_type tabs,size_type depth);
	static void my_assert(bool,const char*);
	void assert_range(size_type n);
public:
	///Iterator template for const_iterator and iterator
	/** This is a lazy implementation of iterator. In other words,
	 * it takes constant time to construct, increment, decrement,
	 * get_position and relocate operations. When being dereferenced,
	 * it checks its validity and does actual work if necessary.
	 * So dereference operations (operators '*' '[]' '->') do most work and
	 * take from constant time to O(log(N)) time, depending on operation mode.
	 * If iterator is used in sequential mode (solely incrementing/decrementing),
	 * dereference operations require practically constant time amortized. If
	 * iterator is used in random access mode, dereference operations take O(log(N))
	 * time and can be as expensive as sequence->operator[].
	 * Hint: using function 'visit' might be faster than iterator.
	 * If you are modifying only one container at a time,
	 * using 'visit' might be preferable.*/
	template<typename TT>
	class iterator_base
	{ 
	public:
		///Random access iterator category.
	typedef std::random_access_iterator_tag iterator_category;
		///T or const T.
	typedef 
		typename std::iterator<std::random_access_iterator_tag, TT>::value_type
		value_type;
		///ptrdiff_t (int).
	typedef
		typename std::iterator<std::random_access_iterator_tag, TT>::difference_type
		difference_type;
		/// T& or const T&
	typedef 
		typename std::iterator<std::random_access_iterator_tag, TT>::reference
		reference;
		/// T* or const T*
	typedef 
		typename std::iterator<std::random_access_iterator_tag, TT>::pointer
		pointer;	
	private:
		const btree_seq *tree;
		size_type abs_idx;
		mutable pointer elems;
		mutable size_type avail_ptr;
		mutable size_type rel_idx;
		mutable Branch *br;
		mutable size_type idx_in_br;
		pointer rebase()const;
		pointer access()const
			{return (rel_idx<avail_ptr)?(elems+rel_idx):(rebase());}
	public:
		///Default constructor.
	    iterator_base():
	    	tree(0),abs_idx(),elems(),avail_ptr(0),rel_idx(),br(),idx_in_br(){};
	    ///Pointing to specific place in the tree.
	    iterator_base(const btree_seq *t,size_type pos):
	    	tree(t),abs_idx(pos),elems(),avail_ptr(0),rel_idx(),br(0),idx_in_br(){};
	    ///Copy constructor for the same type.
	    iterator_base(const iterator_base &that):
	    	tree(that.tree),
	    	abs_idx(that.abs_idx),
	    	elems(that.elems),
	    	avail_ptr(that.avail_ptr),
	    	rel_idx(that.rel_idx),
	    	br(that.br),
	    	idx_in_br(that.idx_in_br){};
	    ///Constructor for conversion from iterator to const_iterator.
	    template<typename T2>
	    iterator_base(const iterator_base<T2> &that):
	    	tree(that.get_container()),
	    	abs_idx(that.get_position()),
	    	elems(that.__get_null_pointer()),
	    	avail_ptr(0),
	    	rel_idx(),
	    	br(),
	    	idx_in_br(){};
	    /// Operator = for the same type.
	    iterator_base& operator=(const iterator_base/*<T2>*/ &that)
	    	{
	    		elems=that.elems;
	    		tree=that.tree;
	    		abs_idx=that.abs_idx;
	    		rel_idx=that.rel_idx;
	    		avail_ptr=that.avail_ptr;
	    		idx_in_br=that.idx_in_br;
	    		br=that.br;
	    		return *this;
	    	}
	    /// Conversion from iterator to const_iterator.
	    template<typename T2>
	    iterator_base& operator=(const iterator_base<T2> &that)
	    	{
	    		tree=that.get_container();
	    		abs_idx=that.get_position();
	    		elems=that.__get_null_pointer();
	    		avail_ptr=0;
	    		return *this;
	    	}
	    /// Dereferencing
	    reference operator*()const{return *access();}
	    /// Dereferencing
	    pointer   operator->()const{return access();}
	    /// Getting arbitary element
	    reference operator[](difference_type n)const
	    	{iterator_base tmp(this);tmp+=n;return *tmp;};
		//comparison operators, < > = != <= >=
	    /// Comparison
	    template<typename T2>
		bool operator==(const iterator_base<T2>& that)const
			{return get_position()==that.get_position();};
	    /// Comparison
	    template<typename T2>
		bool operator!=(const iterator_base<T2>& that)const
			{return get_position()!=that.get_position();};
	    /// Comparison
	    template<typename T2>
		bool operator>(const iterator_base<T2>& that)const
			{return get_position()>that.get_position();};
	    /// Comparison
	    template<typename T2>
		bool operator<(const iterator_base<T2>& that)const
			{return get_position()<that.get_position();};
	    /// Comparison
	    template<typename T2>
		bool operator>=(const iterator_base<T2>& that)const
			{return get_position()>=that.get_position();};
	    /// Comparison
	    template<typename T2>
		bool operator<=(const iterator_base<T2>& that)const
			{return get_position()<=that.get_position();};
	    /// Comparison
	    template<typename T2>
		difference_type operator-(const iterator_base<T2>& that)const
			{return get_position()-that.get_position();}
	    /// Preincrement
		iterator_base& operator++(){++abs_idx;++rel_idx;return *this;}
	    /// Predecrement
		iterator_base& operator--(){--abs_idx;--rel_idx;return *this;}
	    /// Postincrement
		iterator_base operator++(int){iterator_base tmp(*this);++(*this);return tmp;}
	    /// Postdecrement
		iterator_base operator--(int){iterator_base tmp(*this);--(*this);return tmp;}
		/// Increase position by n
		iterator_base& operator+=(difference_type n){abs_idx+=n;rel_idx+=n;return *this;}
		/// Decrease position by n
		iterator_base& operator-=(difference_type n){abs_idx-=n;rel_idx-=n;return *this;}
		/// Increase position by n
		iterator_base operator+(difference_type n)const
			{iterator_base tmp(*this);tmp+=n;return tmp;}
		/// Decrease position by n
		iterator_base operator-(difference_type n)const
			{iterator_base tmp(*this);tmp-=n;return tmp;}
		/// Returns current position
		size_type get_position()const{return abs_idx;}
		/// Returns current container
		const btree_seq* get_container()const{return tree;}
		/// Returns null pointer
		pointer __get_null_pointer()const{return 0;}
	};
	///Constant forward random-access iterator.
	typedef iterator_base<const T> const_iterator;
	///Modifying forward random-access iterator.
	typedef iterator_base<T> iterator;
	///Constant reverse random-access iterator.
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	///Modifying reverse random-access iterator.
	typedef std::reverse_iterator<iterator> reverse_iterator; 
	///Empty container constructor.
	/** Constructs an empty container with no elements.
	 *  Complexity: constant.
	 * 	@param alloc allocator */
	explicit btree_seq(const allocator_type &alloc=allocator_type())
		:T_alloc(alloc),branch_alloc(alloc),leaf_alloc(alloc),root(),count(0)
	{
	};
	///Copy constructor.
	/** Copies all elements from another container.
	 *  Complexity: O(N*log(N)), N=that.size().
	 * 	@param that another container to be copied */
	btree_seq(const btree_seq<T,L,M,A> &that)
		:T_alloc(that.T_alloc),branch_alloc(that.T_alloc),leaf_alloc(that.T_alloc),
		 root(),count(0)
	{
		const_iterator first=that.begin(),last=that.end();
		insert(0,first,last);
	}
	///Fill constructor.
	/** Constructs a container with n elements, each of them is copy of val.
	 *  Complexity: O(n*log(n)).
	 * 	@param n number of elements
	 * 	@param val fill value
	 * 	@param alloc allocator */
	explicit btree_seq(size_type n,const value_type &val,
			const allocator_type &alloc=allocator_type())
		:T_alloc(alloc),branch_alloc(alloc),leaf_alloc(alloc),root(),count(0)
	{
		fill(0,n,val);
	}
	///Range constructor
	/** Constructs a container filled with elements from range [first,last).
	 * Complexity: O(N*log(N)), N=dist(last,first).
	 * @param first first position in a range
	 * @param last  last  position in a range
	 * @param alloc allocator	 */
	template <typename Iterator>
	btree_seq(Iterator first,Iterator last,
		const allocator_type &alloc=allocator_type())
		:T_alloc(alloc),branch_alloc(alloc),leaf_alloc(alloc),root(),count(0)
	{
		typename ___alexkupri_helpers::my_is_integer<Iterator>::__type is_int_type;
		impl_insert(0,first,last,is_int_type);
	}
	///Destructor
	/** Deletes the contents and frees memory.
	 * Complexity: O(N*log(N)).*/
	~btree_seq(){erase(0,count);}
	/** @name Iterators
	 */
	///@{

	///Return constant iterator to beginning.
	/** Complexity: constant. */
	const_iterator begin()const{return const_iterator(this,0);}
	///Return constant iterator to end.
	/** Complexity: constant. */
	const_iterator end()const{return const_iterator(this,count);}
	///Return constant iterator to beginning.
	/** Complexity: constant. */
	const_iterator cbegin()const{return const_iterator(this,0);}
	///Return constant iterator to end.
	/** Complexity: constant. */
	const_iterator cend()const{return const_iterator(this,count);}
	///Return iterator to beginning.
	/** Complexity: constant. */
	iterator begin(){return iterator(this,0);}
	///Return constant iterator to end.
	/** Complexity: constant. */
	iterator end(){return iterator(this,count);}
	///Return constant reverse iterator to reverse beginning.
	/** Complexity: constant. */
	const_reverse_iterator rbegin()const{return const_reverse_iterator(end());}
	///Return constant reverse iterator to reverse end.
	/** Complexity: constant. */
	const_reverse_iterator rend()const{return const_reverse_iterator(begin());}
	///Return constant reverse iterator to reverse beginning.
	/** Complexity: constant. */
	const_reverse_iterator crbegin()const{return const_reverse_iterator(cend());}
	///Return constant reverse iterator to reverse end.
	/** Complexity: constant. */
	const_reverse_iterator crend()const{return const_reverse_iterator(cbegin());}
	///Return reverse iterator to reverse beginning.
	/** Complexity: constant. */
	reverse_iterator rbegin(){return reverse_iterator(end());}
	///Return reverse iterator to reverse end.
	/** Complexity: constant. */
	reverse_iterator rend(){return reverse_iterator(begin());}
	///Return iterator to a given index.
	/** Complexity: constant. */
	iterator iterator_at(size_type pos){return iterator(this,pos);}
	///Return constant iterator to a given index.
	/** Complexity: constant. */
	const_iterator citerator_at(size_type pos)const{return const_iterator(this,pos);}
	///@}
	/** @name Access
	 * Access of the contents
	 */
	///@{

	///Access to element
	/** Returns a reference to the element at position pos.
	 * No range check is done.
	 * Complexity: O(log(N)).
	 * @param pos index of the element*/
	reference operator [](size_type pos)
	{
		Leaf *l=(Leaf*)root;
		size_type found=depth?find_leaf(l,pos):pos;
		T *ptr=l->elements;
		return *(ptr+found);
	}
	///Constant access to element
	/** Returns a constant reference to the element at position pos.
	 * No range check is done.
	 * Complexity: O(log(N)).
	 * @param pos index of the element*/
	const_reference operator [](size_type pos)const
	{
		Leaf *l=(Leaf*)root;
		size_type found=depth?find_leaf(l,pos):pos;
		const T *ptr=l->elements;
		return *(ptr+found);
	}
	///Number of elements in container.
	size_type size()const{return count;}
	///Access to element with range check
	/** Returns a reference to the element at position pos.
	 * Complexity: O(log(N)).
	 * @param pos index of the element*/
	reference at(size_type pos){
		assert_range(pos);
		return (*this)[pos];
	}
	///Constant access to element with range check
	/** Returns a constant reference to the element at position pos.
	 * Complexity: O(log(N)).
	 * @param pos index of the element*/
	const_reference at(size_type pos)const{
		assert_range(pos);
		return (*this)[pos];
	}
	///Returns a reference to the first element.
	/** Complexity: O(log(N)).*/
	reference front(){return (*this)[0];}
	///Returns a constnt reference to the first element.
	/** Complexity: O(log(N)).*/
	const_reference front()const{return (*this)[0];}
	///Returns a reference to the last element.
	/** Complexity: O(log(N)).*/
	reference back(){return (*this)[size()-1];}
	///Returns a constant reference to the last element.
	/** Complexity: O(log(N)).*/
	const_reference back()const{return (*this)[size()-1];}
	///Returns true if the container contains no elements.
	bool empty()const{return count==0;}
	/// Sequential search/modify operation on the range.
	/** Implements visitor pattern. The function 'visit' calls
	 * v() on the elements in the given range sequentially,
	 * until the end of range is reached or v() returns true (whichever
	 * happens earlier). This function allows to implement patterns like
	 * find_if, for_each and so on, but it behaves significantly (roughly twice) faster than
	 * standard implementation and iterator access.
	 * Complexity: O(log(N)*(end-start))
	 * @param start the first element on which visitor should be called
	 * @param end the element beyond the last element on which visitor should be called
	 * @param v the visitor class, which must have 'bool operator(element&)'
	 * @return the index of the first element when v() returned true, or end if v() never returned true*/
	template<typename V>
		size_type visit(size_type start,size_type end,V& v);
	///@}
	/** @name Modifying certain elements of the sequence
	 */
	///@{

	/// Native function for inserting a single element.
	/** Inserts the element at given position.
	 * Hint: inserting the range is faster then multiple insertions of one element.
	 * Complexity: O(log(N)).
	 * @param pos position to insert
	 * @param val element to insert*/
	void insert(size_type pos,const value_type& val)
		{insert_no_more_half_leaf(pos,&val,(&val)+1,1);}
	/// Native function for inserting a range of elements.
	/** Inserts the range [first,last) of elements into the given position.
	 * Complexity: O((N+M)*log(N+M)), N-existing elements, M - new ones.
	 * @param pos position to insert
	 * @param first first element to insert
	 * @param last element behind the last element to insert */
	template <class InputIterator>
		void insert(size_type pos,InputIterator first,InputIterator last);
	///Compatible function for inserting the single element.
	/** Inserts a single element at a given position.
	 * Complexity: O((N+M)*log(N+M)), N-existing elements, M - new ones.
	 * Hint: native 'insert(size_type, const value_type& t)' might be faster.
	 * @param pos position to insert
	 * @param val value to insert
	 * @return iterator pointing to the newly inserted element	 */
	iterator insert(const_iterator pos,const value_type &val)
	{
		size_type position=pos.get_position();
		insert(position,val);
		return iterator_at(position);
	}
	/// Compatible function for inserting n copies of an element.
	/** Inserts n copiesof an element at a given position.
	 * Complexity: O((n+M)*log(n+M)), M - existing elements, n - new ones.
	 * Hint: native 'fill(size_type, size_type, const value_type& val)' might be faster.
	 * @param pos position to insert
	 * @param n number of copies to insert
	 * @param val value to insert
	 * @return iterator pointing to the first of newly inserted elements	 */
	iterator insert(const_iterator pos,size_type n,const value_type &val)
	{
		size_type position=pos.get_position();
		fill(position,n,val);
		return iterator_at(position);
	}
	/// Compatible function for inserting a range of elements.
	/** Inserts the range [first,last) of elements into the given position.
	 * Complexity: O((N+M)*log(N+M)), N-existing elements, M - new ones.
	 * Hint: native 'insert(size_type, InputIterator, InputIterator)' might be faster.
	 * @param pos position to insert
	 * @param first first element to insert
	 * @param last element behind the last element to insert
	 * @return iterator pointing to the first of newly inserted elements	 */
	template<class InputIterator>
	iterator insert(const_iterator pos,InputIterator first,InputIterator last)
	{
		typename ___alexkupri_helpers::my_is_integer<InputIterator>::__type is_int_type;
		size_type position=pos.get_position();
		impl_insert(position,first,last,is_int_type);
		return iterator_at(position);
	}
	/// Native function for inserting n copies of an element.
	/** Inserts n copiesof an element at a given position.
	 * Complexity: O((n+M)*log(n+M)), M - existing elements, n - new ones.
	 * @param pos position to insert
	 * @param repetition number of copies to insert
	 * @param val value to insert */
	void fill(size_type pos,size_type repetition,const value_type& val)
	{
		FillIterator first(&val,0),last(&val,repetition);
		insert(pos,first,last);
	}
	/// Resize container so that it contains n elements.
	/** If n is greater than container size, copies of the val are added to the end.
	 *  If n is less than container size, some elements at the end of container are deleted. */
	void resize(size_type n,const value_type& val = value_type());
	/// Native function for erasing elements.
	/** Erase the range of [first,last) elements.
	 * Complexity: O(M*log(N)), M-erased elements, N - all elements.
	 * @param first index of the first element to erase
	 * @param last index of the last element to erase + 1 */
	void erase(size_type first,size_type last);
	/// Compatible function for erasing one element.
	/** Erase the element at position pos.
	 * Complexity: O(log(N)).
	 * Hint: native 'erase(size_type, size_type)' might be faster.
	 * @param pos index of the element to erase
	 * @return iterator pointing to the next position after erased element */
	iterator erase(const_iterator pos)
	{
		size_type position=pos.get_position();
		erase(position,position+1);
		return iterator_at(position);
	}
	/// Compatible function for erasing elements.
	/** Erase the range of [first,last) elements.
	 * Complexity: O(M*log(N)), M-erased elements, N - all elements.
	 * Hint: native 'erase(size_type, size_type)' might be faster.
	 * @param first index of the first element to erase
	 * @param last index of the last element to erase + 1
	 * @return iterator pointing to the next position after erased elements */
	iterator erase(const_iterator first,const_iterator last)
	{
		size_type first_position=first.get_position(),last_position=last.get_position();
		erase(first_position,last_position);
		return iterator_at(first_position);
	}
	/// Adds element to the end of the sequence.
	/** Complexity: O(log(N))	 */
	void push_back(const value_type &val)
		{	insert(count,val);	}
	/// Adds element to the beginning of the sequence.
	/** Complexity: O(log(N))	 */
	void push_front(const value_type &val)
		{   insert(0,val); }
	/// Removes the last element.
	/** Complexity: O(log(N))	 */
	void pop_back()
		{	erase(count-1,count); }
	/// Removes the first element.
	/** Complexity: O(log(N))	 */
	void pop_front()
		{	erase(0,1);	}
	///@}
	/** @name Modifying the whole contents of the sequence
	 */
	///@{

	///Assign content
	/** Deletes old contents and replaces it with copy of contents of that.
	 * Complexity: O(N*log(N))+O(M*log(M)), N=this->size(), M=that.size().
	 * @param that container to be assigned	 */
	btree_seq &operator=(const btree_seq<T,L,M,A> &that)
	{
		const_iterator first=that.begin(),last=that.end();
		clear();
		insert(0,first,last);
		return *this;
	}
	/// Swaps contents of two containers.
	/** Complexity: constant.
	 * @param that container to swap with */
	void swap(btree_seq<T,L,M,A> &that);
	/// Erases all contents of the container.
	/** Complexity: O(N*log(N)) */
	void clear(){erase(0,count);}
	/// Replaces the whole contents with n copies of val.
	/** Erases contents of the container, then fills it with n copies of val.
	 *  Complexity: O((n+M)*log(n+M)), M - existing elements, n - new ones
	 * @param n new size of container
	 * @param val element to clone */
	void assign(size_type n,const value_type &val);
	/// Replaces the whole contents with a range.
	/** Erases contents of the container, then fills it with a copy of range [first,last).
	 * Complexity: O((n+M)*log(n+M)), M - existing elements, n - new ones
	 * @param first first element to be inserted
	 * @param last element behind the last element to be inserted */
	template <class InputIterator>
		void assign(InputIterator first,InputIterator last)
	{
		typename ___alexkupri_helpers::my_is_integer<InputIterator>::__type is_int_type;
		clear();
		impl_insert(0,first,last,is_int_type);
	}
	/// Fast concatenate two sequences (that sequence to the right).
	/** Concatenate two sequences (that sequence to the right), put result
	 * into this sequence and leave that sequence empty.
	 * Concatenation is done without copying all elements.
	 * Example: if sequence A contains {0,1,2} and sequeance B contains
	 * {3,4,5}, after a call 'A.concatenate_right(B)' A contains
	 * {0,1,2,3,4,5} and B is empty.
	 * Complexity: O(log(N+M))
	 * @param that container to concatenate	 */
	void concatenate_right(btree_seq<T,L,M,A> &that);
	/// Fast concatenate two sequences (that sequence to the left).
	/** Concatenate two sequences (that sequence to the left), put result
	 * into this sequence and leave that sequence empty.
	 * Concatenation is done without copying all elements.
	 * Example: if sequence A contains {0,1,2} and sequeance B contains
	 * {3,4,5}, after a call 'A.concatenate_left(B)' A contains
	 * {3,4,5,0,1,2} and B is empty.
	 * Complexity: O(log(N+M))
	 * @param that container to concatenate	 */
	void concatenate_left(btree_seq<T,L,M,A> &that);
	///Fast split, leaving right piece in that container.
	/** Split sequence into two parts: [0,pos) is left in this container,
	 * [pos,size) is moved to that container. That container is cleaned before
	 * operation. Example if A contained {0,1,2,3,4}, after A.split_right(B,3)
	 * A contains {0,1,2} and B contains {3,4}. The operation is done without
	 * moving all elements.
	 * Complexity: O(log(N)), if the second container is initially empty.
	 * @param that container for right part of split operation (old contents removed)
	 * @param pos place to split */
	void split_right(btree_seq<T,L,M,A> &that,size_type pos);
	///Fast split, leaving left piece in that container.
	/** Split sequence into two parts: [pos,size) is left in this container,
	 * [0,pos) is moved to that container. That container is cleaned before
	 * operation. Example if A contained {0,1,2,3,4}, after A.split_left(B,3)
	 * A contains {3,4} and B contains {0,1,2}. The operation is done without
	 * moving all elements.
	 * Complexity: O(log(N)), if the second container is initially empty.
	 * @param that container for leftt part of split operation (old contents removed)
	 * @param pos place to split */
	void split_left(btree_seq<T,L,M,A> &that,size_type pos);
	///@}
	/** @name Others
	 * Functions not used in release version: debug, profile and compatibility.
	 */
	///@{

	/// Checks consistency of the container (debug).
	/** Use this function when modifying this code or if you are unsure,
	 * if your program corrupts memory of the container. */
	void __check_consistency();
	/// Output the contents of container into the stream (debug).
	template <class output_stream>
		void __output(output_stream &o,const char *comm="");
	/// Returns L (second parameter of the template) (profile, RTTI).
	size_type __children_in_branch(){return L;}
	/// Returns M (third parameter of the template) (profile, RTTI).
	size_type __elements_in_leaf(){return M;}
	/// Returns size of inner node of the tree (profile, RTTI).
	size_type __branch_size(){return sizeof(Branch);}
	/// Returns size of leaf node of the tree (profile, RTTI).
	size_type __leaf_size(){return sizeof(Leaf);}
	/// Function does nothing (compatibility with std::vector).
	void reserve(size_type n){/*NOTHING*/}
	/// Returns allocator.
	allocator_type get_allocator()const{return T_alloc;}
	///@}
};

/// Swap contents of two containers.
template <typename T,int L,int M,typename A>
void swap(btree_seq<T,L,M,A> &first,btree_seq<T,L,M,A> &second)
{
	first.swap(second);
}

/// Lexicographical comparison
template <typename T,int L,int M,typename A>
bool   operator<(const btree_seq<T,L,M,A> &x,const btree_seq<T,L,M,A> &y)
    { return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end()); }

/// Lexicographical comparison
template <typename T,int L,int M,typename A>
bool   operator>(const btree_seq<T,L,M,A> &x,const btree_seq<T,L,M,A> &y)
    { return (y<x); }

/// Lexicographical comparison
template <typename T,int L,int M,typename A>
bool   operator<=(const btree_seq<T,L,M,A> &x,const btree_seq<T,L,M,A> &y)
    { return !(y<x); }

/// Lexicographical comparison
template <typename T,int L,int M,typename A>
bool   operator>=(const btree_seq<T,L,M,A> &x,const btree_seq<T,L,M,A> &y)
    { return !(x<y); }

/// Equality of size and all elements
template <typename T,int L,int M,typename A>
bool  operator==(const btree_seq<T,L,M,A> &x,const btree_seq<T,L,M,A> &y)
    { return (x.size() == y.size()
	      && std::equal(x.begin(), x.end(), y.begin())); }

/// Inequality of size or any elements
template <typename T,int L,int M,typename A>
bool  operator!=(const btree_seq<T,L,M,A> &x,const btree_seq<T,L,M,A> &y)
    { return !(x==y); }


#include "btree_seq2.h"

#endif /*__BTREE_SEQ_H*/
