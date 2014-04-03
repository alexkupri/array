//  Copyright (C) 2014 by Aleksandr Kupriianov
//  alexkupri at gmail dot com

//  Purpose: fast array implementation

#include <stdexcept>
//================== memory_operator ====================

//moving elements, while incrementing index
template <typename T,typename Allocator>
void default_memory_operator<T,Allocator>::move_elements_inc(T *dst,T *src,size_k num)
{
	T *limit=src+num;
	while(src!=limit){
		construct(dst,*src);
		destroy(src);
		++dst;
		++src;
	}
}

//moving elements, while decrementing index
template <typename T,typename Allocator>
void default_memory_operator<T,Allocator>::move_elements_dec(T *dst,T *limit,size_k num)
{
	T *src=limit+num;
	dst+=num;
	while(src!=limit){
		--dst;
		--src;
		construct(dst,*src);
		destroy(src);
	}
}

//Filling leaf with elements, elements are read from iterator
template <typename T,typename Allocator> template <class InputIterator>
typename Allocator::size_type default_memory_operator<T,Allocator>::
	fill_elements(T *dst,typename Allocator::size_type num,InputIterator &first,InputIterator last)
{
	T *ptr=dst,*limit=ptr+num;
	try{
		while((ptr!=limit)&&(first!=last)){
			construct(ptr,*first);
			++first;
			++ptr;
		}
	}catch(...){
		T *del_elems=dst;
		while(del_elems!=ptr){
			destroy(del_elems);
			del_elems++;
		}
		throw;
	}		
	return ptr-dst;
}

//destroying elements from leaf
template <typename T,typename Allocator>
void default_memory_operator<T,Allocator>::burn_elements(T *ptr,size_k num) 
{
	while(num){
		destroy(ptr);
		ptr++;
		num--;
	}
}

//Filling elements from iterators for fast iterator.
template <typename T,typename Allocator> template <class InputIterator>
	typename Allocator::size_type fast_memory_operator<T,Allocator>::
	fill_elements(T *dst,typename Allocator::size_type num,InputIterator &first,InputIterator last)
{
	T *ptr=dst,*limit=ptr+num;
	while((ptr!=limit)&&(first!=last)){
		construct(ptr,*first);
		++first;
		++ptr;
	}
	return ptr-dst;
}

//Filling elements from array (special case).
template <typename T,typename Allocator> 
	typename Allocator::size_type fast_memory_operator<T,Allocator>::
	fill_elements(T *dst,typename Allocator::size_type num,const T *&first,const T *last)
{
	typename Allocator::size_type num2=last-first;
	if(num2>num){
		num2=num;
	}
	memmove(dst,first,num2*sizeof(T));
	first+=num2;
	return num2;
}

//============= main class =======================
//============= find and read functions  ==================

//find leaf for reading
template <typename T,int L,int M,typename A>
typename alex_kupriianov_array<T,L,M,A>::size_k alex_kupriianov_array<T,L,M,A>
	::find_leaf(Leaf *&l,size_k pos)const
{
	Node *node=root;
	size_k j=depth,k,val;
	while(j){
		k=0;
		val=((Branch*)node)->nums[0];
		while(pos>=val){
			pos-=val;
			k++;
			assert(k<((Branch*)node)->fillament);
			val=((Branch*)node)->nums[k];
		}
		node=((Branch*)node)->children[k];
		j--;
	}
	l=(Leaf*)node;
	return pos;
}

//Find leaf and increment counter by the way (we are going to add some elements).  
template <typename T,int L,int M,typename A>
typename alex_kupriianov_array<T,L,M,A>::size_k 
	alex_kupriianov_array<T,L,M,A>::find_leaf_inc(Leaf *&l,size_k pos,size_k increment)
{
	Node *node=root;
	size_k j=depth,k,val;
	while(j){
		k=0;
		val=((Branch*)node)->nums[0];
		while(pos>=val){
			pos-=val;
			k++;
			assert(k<((Branch*)node)->fillament);
			val=((Branch*)node)->nums[k];
		}
		((Branch*)node)->nums[k]+=increment;		
		node=((Branch*)node)->children[k];
		j--;
	}
	l=(Leaf*)node;
	count+=increment;
	return pos;
}

//Find leaf and decrement counter by the way (we are going to remove some elements).  
template <typename T,int L,int M,typename A>
typename alex_kupriianov_array<T,L,M,A>::size_k 
	alex_kupriianov_array<T,L,M,A>::find_leaf_dec(Leaf *&l,size_k pos,size_k decrement)
{
	Node *node=root;
	size_k j=depth,k,val;
	while(j){
		k=0;
		val=((Branch*)node)->nums[0];
		while(pos>=val){
			pos-=val;
			k++;
			assert(k<((Branch*)node)->fillament);
			val=((Branch*)node)->nums[k];
		}
		((Branch*)node)->nums[k]-=decrement;		
		node=((Branch*)node)->children[k];
		j--;
	}
	l=(Leaf*)node;
	count-=decrement;
	return pos;
}

//Find child in a branch
template <typename T,int L,int M,typename A>
typename alex_kupriianov_array<T,L,M,A>::size_k alex_kupriianov_array<T,L,M,A>::find_child
	(Branch *b,Node* child)
{
	for(size_k j=0;j<b->fillament;j++){
		if(child==b->children[j]){
			return j;
		}
	}
	assert(false);
	return 0;
}

//================= Leaf and element helpers ==============

//Estimate differences between iterators (but not exceeding limit).
template <typename T,int L,int M,typename A> template <class InputIterator>
typename alex_kupriianov_array<T,L,M,A>::size_k alex_kupriianov_array<T,L,M,A>::
	count_difference(InputIterator first,InputIterator last,size_k limit)
{
	size_k res=0;
	while((first!=last)&&(limit>0)){
		++first;
		++res;
		--limit;
	}
	return res;
}

//Trying to merge leaves (their parent given and index of the left one).
template <typename T,int L,int M,typename A>
bool alex_kupriianov_array<T,L,M,A>::try_merge_leaves(Branch *b,size_k idx)
{
	size_k l=b->nums[idx],r=b->nums[idx+1];
	if(l+r>M){
		return false;
	}
	Leaf *left=(Leaf*)b->children[idx],*right=(Leaf*)b->children[idx+1];
	T_alloc.move_elements_inc(left->elements+l,right->elements,r);
	left->fillament=l+r;
	b->nums[idx]=l+r;
	delete_leaf(right);
	return true;
}

//Trying to balance leaves by copying some elements from left to right (their parent given and index of the left one).
template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::balance_leaves_lr(Branch *b,size_k idx)
{
	Leaf *left=(Leaf*)b->children[idx],*right=(Leaf*)b->children[idx+1];
	size_k l=b->nums[idx],r=b->nums[idx+1];
	size_k moves=l-(r+l)/2;
	assert(moves>0);
	T_alloc.move_elements_dec(right->elements+moves,right->elements,r);
	T_alloc.move_elements_inc(right->elements,left->elements+l-moves,moves);
	left->fillament-=moves;
	right->fillament+=moves;
	b->nums[idx]-=moves;
	b->nums[idx+1]+=moves;		
}

//Trying to balance leaves by copying some elements from right to left (their parent given and index of the left one).
template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::balance_leaves_rl(Branch *b,size_k idx)
{
	Leaf *left=(Leaf*)b->children[idx],*right=(Leaf*)b->children[idx+1];
	size_k l=b->nums[idx],r=b->nums[idx+1];
	size_k moves=r-(r+l)/2;
	assert(moves>0);
	T_alloc.move_elements_inc(left->elements+l,right->elements,moves);
	T_alloc.move_elements_inc(right->elements,right->elements+moves,r-moves);
	left->fillament+=moves;
	right->fillament-=moves;
	b->nums[idx]+=moves;
	b->nums[idx+1]-=moves;	
}

//===================== Branch helpers ====================

//Preparing place for inserting children by moving some children.
template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::insert_children(Branch *b,size_k idx,size_k num)
{	
	size_k j=b->fillament;
	while(j!=idx){
		j--;
		b->children[j+num]=b->children[j];
		b->nums[j+num]=b->nums[j];
	};
	b->fillament+=num;
}

//Delete some children by moving some children.
template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::delete_children(Branch *b,size_k idx,size_k num)
{
	for(size_k j=idx;j<b->fillament-num;j++){
		b->children[j]=b->children[j+num];
		b->nums[j]=b->nums[j+num];
	}
	b->fillament-=num;
}

//Moving some children from old parent (src) to new parent (dst), returning total amount elements in them. 
template <typename T,int L,int M,typename A>
typename alex_kupriianov_array<T,L,M,A>::size_k alex_kupriianov_array<T,L,M,A>::move_children(
	Branch *dst,size_k idst,Branch *src,size_k isrc,size_k num)
{
	size_k j,res=0,cur;
	Node *n;
	for(j=0;j<num;j++){
		n=src->children[isrc+j];
		dst->children[idst+j]=n;
		n->parent=dst;
		cur=src->nums[isrc+j];
		dst->nums[idst+j]=cur;
		res+=cur;
	}
	return res;
}

//Balance branches by moving some nodes from left to right (their parent given and index of the left one).
template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::balance_branch_lr(Branch *b,size_k idx)
{
	Branch *left=(Branch*)b->children[idx],
		*right=(Branch*)b->children[idx+1];
	size_k l=left->fillament,r=right->fillament,
		moves=l-(l+r)/2;
	assert(moves>1);
	insert_children(right,0,moves);
	size_k num=move_children(right,0,left,l-moves,moves);
	left->fillament-=moves;
	b->nums[idx]-=num;
	b->nums[idx+1]+=num;
}

//Balance branches by moving some nodes from right to left (their parent given and index of the left one).
template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::balance_branch_rl(Branch *b,size_k idx)
{
	Branch *left=(Branch*)b->children[idx],
		*right=(Branch*)b->children[idx+1];
	size_k l=left->fillament,r=right->fillament,
		moves=r-(l+r)/2;
	assert(moves>1);
	size_k num=move_children(left,l,right,0,moves);
	delete_children(right,0,moves);
	left->fillament+=moves;
	b->nums[idx]+=num;
	b->nums[idx+1]-=num;
}

//Try to merge two branches into one (their parent given and index of the left one).
template <typename T,int L,int M,typename A>
bool alex_kupriianov_array<T,L,M,A>::try_merge_branches(Branch *b,size_k idx)
{
	Branch *left=(Branch*)b->children[idx],*right=(Branch*)b->children[idx+1];
	size_k l=left->fillament,r=right->fillament;
	if(l+r>L){
		return false;
	}
	move_children(left,left->fillament,right,0,right->fillament);
	left->fillament+=right->fillament;
	b->nums[idx]+=b->nums[idx+1];
	return true;
}

//========== Merge/split and balancing algorithms =========

//We are going to split branches, so we count how many branches we need and reserve 
//them in advance, building a list. This is done for not getting no_mem exception
//in the middle of splitting.
template <typename T,int L,int M,typename A>
typename alex_kupriianov_array<T,L,M,A>::Branch *
	alex_kupriianov_array<T,L,M,A>::reserve_enough_branches_splitting(Node* existing)
{
	Branch *parent,*new_branch;
	Branch *branch_bundle=NULL;
	parent=existing->parent;
	try{
		for(;;){
			if((parent==NULL)||(parent->fillament==L)){
				new_branch=branch_alloc.allocate(1);
				new_branch->parent=branch_bundle;
				branch_bundle=new_branch;
			}
			if((parent==NULL)||(parent->fillament<L)){
				break;
			}
			parent=parent->parent;
		}
	}catch(...){
		while(branch_bundle!=NULL){
			new_branch=branch_bundle;
			branch_bundle=branch_bundle->parent;
			branch_alloc.deallocate(new_branch,1);
		}
		throw;
	}
	return branch_bundle;
}

//Actions necessary to increase depth of the tree by one. 
template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::split_root(Node* existing,Node* right_to_existing,
						size_k right_count,Branch *new_branch)
{
	new_branch->parent=0;
	new_branch->fillament=2;
	new_branch->children[0]=existing;
	new_branch->nums[0]=count-right_count;
	existing->parent=new_branch;
	new_branch->children[1]=right_to_existing;
	new_branch->nums[1]=right_count;
	right_to_existing->parent=new_branch;
	root=new_branch;
	depth++;	
}

//Splitting node. Given are: node to split, node that appears to the right from it and number of elements in the right node.
template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::split(Node* existing,Node* right_to_existing,size_k right_count)
{
	size_k k,num=0;
	Branch *parent,*new_branch,*branch_to_insert;
	Branch *branch_bundle=reserve_enough_branches_splitting(existing);
	while(right_to_existing!=0){
		parent=existing->parent;
		if(parent==0){//special case: existing was root, we increase depth
			split_root(existing,right_to_existing,right_count,branch_bundle);
			break;
		}else{
			//by default we'll insert into the current branch
			k=find_child(parent,existing);
			new_branch=0;//by default, no new iteration takes place
			branch_to_insert=parent;
			if(parent->fillament==L){//if we cannot fit into current branch, splitting
				new_branch=branch_bundle;//we plan to execute new iteration
				branch_bundle=branch_bundle->parent;
				num=move_children(new_branch,0,parent,L-L/2,L/2);
				parent->fillament=L-L/2;
				new_branch->fillament=L/2;
				if(k>=L-L/2){
					branch_to_insert=new_branch;
					k-=L-L/2;
				}
			}
			//inserting 
			right_to_existing->parent=branch_to_insert;
			insert_children(branch_to_insert,k+1,1);
			branch_to_insert->children[k+1]=right_to_existing;
			branch_to_insert->nums[k+1]=right_count;
			branch_to_insert->nums[k]-=right_count;
			//preparing for the next iteration, if valid
			right_count=num;
			right_to_existing=new_branch;//next iteration takes place only if new_branch!=0
			existing=parent;
		}
	}
}

//Inserting the whole leaf at given position.
template <typename T,int L,int M,typename A> 
void alex_kupriianov_array<T,L,M,A>::insert_leaf(Leaf *l,size_k pos)
{
	Leaf *left_neighbour;
	find_leaf_inc(left_neighbour,pos?pos-1:0,l->fillament);
	try{
		split(left_neighbour,l,l->fillament);
	}
	catch(...){
		find_leaf_dec(left_neighbour,pos?pos-1:0,l->fillament);
		throw;
	}
	if(pos==0){//special case
		Branch *parent=l->parent;
		int tmp=parent->nums[0];
		parent->nums[0]=parent->nums[1];
		parent->nums[1]=tmp;
		parent->children[0]=l;
		parent->children[1]=left_neighbour;
	}
}

//The fn for deleting leaf, including all checks and underflows.
template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::delete_leaf(Leaf *l)
{
	//Variable naming: parent gave birth to node (me,this), node gave birth to child
	//child is empty and must be deleted from node, node must be checked for underflow
	Branch *parent=l->parent,*node;
	size_k idx;
	if(parent!=0){//special case: tree can be empty
		idx=find_child(parent,l);
		for(;;){//we'll exit unless we execute "continue" operator
			node=parent;
			delete_children(node,idx,1);
			//checking, whether we need underflow node
			if((node->fillament<L/2-1)||(node->fillament==1)){
				parent=node->parent;
				if(parent==0){//that means that node is root, 
						//which can have at least 2 children, no parent, no brothers
					if(node->fillament==1){//if root has only one child then level down
						root=node->children[0];
						root->parent=0;
						depth--;
						branch_alloc.deallocate(node,1);
					}
				}else{//node is not root: has parent, at least one adjastent brother 
					idx=find_child(parent,node);//and must be balanced
					//if one of node's brothers is thin, we merge and erase some node
					if(idx>0){
						if(try_merge_branches(parent,idx-1)){
							branch_alloc.deallocate(node,1);
							continue;//give all children to the left brother and erase node
						}
					}
					if(idx<parent->fillament-1){
						if(try_merge_branches(parent,idx)){
							idx++;
							branch_alloc.deallocate((Branch*)parent->children[idx],1);
							continue;//take all children from the right brother and erase the brother
						}
					}
					//all our brothers are fat enough for balancing
					if(idx>0){
						balance_branch_lr(parent,idx-1);
					}else{
						balance_branch_rl(parent,idx);
					}
				}
			}
			break;//no "continue": exit;
		}
	}
	leaf_alloc.deallocate(l,1);
}

//Check for underflow of leaf (i.e. if it can be deleted, merged or balanced if too thin).
template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::underflow(Leaf *l)
{
	if(l->fillament<M/2){
		Branch *parent=l->parent;
		if(l->fillament==0){
			delete_leaf(l);
		}else if(parent==0){
			//NOTHING, if the whole tree contains only one leaf, the leaf can be of any size.
		}else{
			size_k idx=find_child(parent,l);
			//Trying to merge with both siblings		
			if(idx>0){
				if(try_merge_leaves(parent,idx-1)){
					return;
				}
			}
			if(idx<parent->fillament-1){
				if(try_merge_leaves(parent,idx)){
					return;
				}
			}
			//all our siblings are fat, otherwise we had succeeded in try_merge_leaves above
			if(idx>0){
				balance_leaves_lr(parent,idx-1);
			}else{
				balance_leaves_rl(parent,idx);
			}
		}
	}
}

//Initialize the empty tree (preparing for insert).
template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::init_tree()
{
	Leaf *l=leaf_alloc.allocate(1);
	l->fillament=0;
	l->parent=0;
	root=l;
	depth=0;
}

//Deleting elements recursively.
template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::burn_subtree(Node *node,size_k depth)
{
	size_k j;
	if(depth==0){
		Leaf *l=(Leaf*)node;
		T_alloc.burn_elements(l->elements,l->fillament);
		leaf_alloc.deallocate(l,1);
	}else{
		Branch *b=(Branch*)node;
		for(j=0;j<b->fillament;j++){
			burn_subtree(b->children[j],depth-1);
		}
		branch_alloc.deallocate(b,1);
	}
}

//After some operations, like multiple delete or multiple insert, 
//thin leaves can occur at certain positions. We need to check and underfow.
template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::sew_together(size_k pos)
{
	Leaf *l;
	size_k found;
	if(pos<count){
		found=find_leaf(l,pos);
		underflow(l);
	}
	if(pos>0){
		find_leaf(l,pos-1);
		underflow(l);
	}	
}

//==================== Public functions =================== 

template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::insert(size_k pos,const T& t)
{
	assert(0<=pos);
	assert(pos<=count);
	if(count==0){
		init_tree();
	}
	Leaf *l,*newleaf=0;
	size_k found,delta=0;
	if(pos!=0){
		delta=1;
	}
	found=find_leaf_inc(l,pos-delta,1)+delta;//find leaf and increment counter
	if(l->fillament==M){//we assume that this branch is executed rarely
		find_leaf_dec(l,pos-delta,1);//undo increment
		newleaf=leaf_alloc.allocate(1);
		T_alloc.move_elements_inc(newleaf->elements,l->elements+M-M/2,M/2);
		newleaf->fillament=M/2;
		try{
			split(l,newleaf,newleaf->fillament);//splitting the leaf, that was overflowed
		}
		catch(...){
			leaf_alloc.deallocate(newleaf,1);
			throw;
		}
		l->fillament=M-M/2;
		found=find_leaf_inc(l,pos-delta,1)+delta; //now finding the leaf again, 
		//which is guaranteed to have space  
	}
	T_alloc.move_elements_dec(l->elements+found+1,l->elements+found,l->fillament-found);
	try{
		T_alloc.move_one_element(l->elements+found,t);
	}catch(...){
		T_alloc.move_elements_inc(l->elements+found,l->elements+found+1,l->fillament-found);
		find_leaf_dec(l,pos-delta,1);
		underflow(l);//if it is empty
		throw;
	}
	l->fillament++;
}

//Helper function for multiple insert. It ensures, that all consequent inserting can be done
//by inserting the whole leafs. Firstly, it cuts leaf in the place of insertion if necessary.
//Secondly, it fills the left leaf with elements until it is full.
template <typename T,int L,int M,typename A> template <class InputIterator>
void alex_kupriianov_array<T,L,M,A>::start_inserting(Leaf *l,size_k found,size_k &pos,InputIterator &first,InputIterator last)
{
	size_k n;
	if((pos!=0)||(count==0)){
		if(found!=l->fillament){//we need to split existing leaf first
			Leaf *new_leaf=leaf_alloc.allocate(1);
			T_alloc.move_elements_inc(new_leaf->elements,l->elements+found,l->fillament-found);
			new_leaf->fillament=l->fillament-found;
			try{
				split(l,new_leaf,new_leaf->fillament);
			}
			catch(...){
				T_alloc.move_elements_inc(l->elements+found,new_leaf->elements,l->fillament-found);
				leaf_alloc.deallocate(new_leaf,1);
				throw;
			}
			l->fillament=found;
		}
		//we fill last leaf before gap as good as we can
		try{ 
			n=T_alloc.fill_elements(l->elements+found,M-found,first,last);
		}
		catch(...){
			underflow(l);//this is for case of empty tree
			sew_together(pos);//this is for case splitting
			throw;
		}
		l->fillament+=n;
		find_leaf_inc(l,pos?pos-1:0,n);
		pos+=n;
	}			
}

//Helper function for multiple insert. Creates and inserts the whole leafs.
template <typename T,int L,int M,typename A> template <class InputIterator>
void alex_kupriianov_array<T,L,M,A>::insert_whole_leafs
		(size_k startpos,size_k &pos,InputIterator first,InputIterator last)
{
	Leaf *l;
	size_k n;
	try{
		while(first!=last){
			l=leaf_alloc.allocate(1);
			l->fillament=0;
			try{
				n=T_alloc.fill_elements(l->elements,M,first,last);
			}catch(...){
				leaf_alloc.deallocate(l,1);
				throw;
			}
			l->fillament=n;
			try{
				insert_leaf(l,pos);
			}catch(...){
				T_alloc.burn_elements(l->elements,n);
				leaf_alloc.deallocate(l,1);
				throw;
			}
			pos+=n;
		}
	}
	catch(...)
	{
		erase(startpos,pos);
		sew_together(startpos);
		throw;
	}
}

template <typename T,int L,int M,typename A> template <class InputIterator>
void alex_kupriianov_array<T,L,M,A>::insert
	(size_k pos,InputIterator first,InputIterator last)
{
	Leaf *l;
	size_k found,n,delta=0,startpos=pos;
	//particular case: empty container
	assert(0<=pos);
	assert(pos<=count);
	if(first==last){
		return;
	}
	if(0==count){
		init_tree();
	}
	//inserting as many elements as we can into existing structure
	if(pos!=0){	
		delta=1; 
	}
	found=find_leaf(l,pos-delta)+delta;
	n=count_difference(first,last,M+1-l->fillament);
	if(n<=M-l->fillament){//we can insert all elements into existing leaf
		T_alloc.move_elements_dec(l->elements+found+n,l->elements+found,l->fillament-found);
		try{		
			T_alloc.fill_elements(l->elements+found,n,first,last);
		}catch(...){
			T_alloc.move_elements_inc(l->elements+found,l->elements+found+n,l->fillament-found);
			underflow(l);
			throw;
		}
		l->fillament+=n;
		find_leaf_inc(l,pos-delta,n);
	}else{
		start_inserting(l,found,pos,first,last);
		insert_whole_leafs(startpos,pos,first,last);
		sew_together(pos);
	}
}

template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::erase(size_k pos)
{
	size_k found;
	Leaf *l;
	assert(0<=pos);
	assert(pos<count);
	found=find_leaf_dec(l,pos,1);
	T_alloc.burn_elements(l->elements+found,1);
	l->fillament--;
	T_alloc.move_elements_inc(l->elements+found,l->elements+found+1,l->fillament-found);
	underflow(l);
}

template <typename T,int L,int M,typename A> 
void alex_kupriianov_array<T,L,M,A>::erase(size_k first,size_k last)
{
	size_k found,del;
	Leaf *l;
	assert(0<=first);
	assert(first<=last);
	assert(last<=count);
	while(first<last){
		found=find_leaf(l,first);
		del=l->fillament-found;
		if(del>last-first){
			del=last-first;
		}
		T_alloc.burn_elements(l->elements+found,del);
		T_alloc.move_elements_inc(l->elements+found,l->elements+found+del,l->fillament-found-del);
		last-=del;
		find_leaf_dec(l,first,del);
		l->fillament-=del;
		if(l->fillament==0){
			delete_leaf(l);			
		}
	}
	sew_together(first);
}

template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::clear()
{
	if(count!=0){
		burn_subtree(root,depth);
		count=depth=0;
	}
}

//==================== Iterator fns =================
//Setting iterator structure in the certain position.
template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::
	set_iterator_struct(IteratorStruct &s,size_k pos)const
{
	s.array=this;
	if(pos<count){//meaningful values
		Leaf *l=(Leaf*)root;
		size_k found=depth?find_leaf(l,pos):pos;
		s.first_valid=l->elements;
		s.last_valid=l->elements+l->fillament-1;
		s.current=l->elements+found;
		s.first_valid_index=pos-found;
	}else if(pos==count){//special case for end
		assert(s.is_forward_iterator);
		s.first_valid_index=pos;
		s.current=s.last_valid=s.first_valid=0;
	}else if(pos==count+1){//special case for rend
		assert(!s.is_forward_iterator);
		s.first_valid_index=pos;
		s.current=s.last_valid=s.first_valid=0;
	}else{
		assert(false);//out of range
	}
}

template <typename T,int L,int M,typename A>
    void alex_kupriianov_array<T,L,M,A>::
	set_iterator_struct_null(IteratorStruct &s)
{
	s.array=0;
	s.first_valid_index=-3;
	s.current=s.first_valid=s.last_valid=0;
}

template <typename T,int L,int M,typename A>
    void alex_kupriianov_array<T,L,M,A>::
	increment_iterator_struct(IteratorStruct &s)
{
	if(s.current==s.last_valid){
		assert(s.array!=0);
		s.array->set_iterator_struct(s,get_position(s)+1);
	}else{
		s.current++;
	}
}

template <typename T,int L,int M,typename A>
    void alex_kupriianov_array<T,L,M,A>::
	decrement_iterator_struct(IteratorStruct &s)
{
	if(s.current==s.first_valid){
		assert(s.array!=0);
		size_k pos=get_position(s);
		if(pos==0){
			s.array->set_iterator_struct(s,s.array->count+1);
		}else{
			s.array->set_iterator_struct(s,get_position(s)-1);
		}
	}else{
		s.current--;
	}
}

template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::my_assert(bool b,const char *msg)
{
	if(!b){
		throw std::runtime_error(msg);
	}
}

template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::check_node(Node* c,size_k sum,bool head,size_k depth,Branch *parent)
{
	size_k j,summ=0;
	if(depth>0){
		Branch *b=(Branch*)c;
		if(!head){
			my_assert(b->fillament>=L/2-1,"Branches must be filled.");
		}
		my_assert(b->fillament>1,"Head branch must be filled.");
		for(j=0;j<b->fillament;j++){
			summ+=b->nums[j];
		}
		my_assert(sum==summ,"Sum of elements must be equal to the node sum.");
		my_assert(b->parent==parent,"Parent must be correct.");
		for(j=0;j<b->fillament;j++){
			check_node(b->children[j],b->nums[j],false,depth-1,b);
		}
	}else{
		Leaf *l=(Leaf*)c;
		my_assert(l->fillament>=1,"Leaf must have at least one element.");
		if(!head){
			my_assert(l->fillament>=M/2,"In multileaf tree, leaves must be at least half-filled.");
		}
		my_assert(l->fillament==sum,"Sum is the number of children in leaf.");
		my_assert(l->parent==parent,"Parent in leaf must be correct.");
	}
} 	

template <typename T,int L,int M,typename A>
void alex_kupriianov_array<T,L,M,A>::__check_consistency()
{
	if(count!=0){
		check_node(root,count,true,depth,0);
	}
}
 	
template <typename T,int L,int M,typename A> template <class output_stream>
void alex_kupriianov_array<T,L,M,A>::
	output_node(output_stream &o,Node* c,size_k tabs,size_k depth)
{
	size_k j;
	for(j=0;j<tabs;j++){
		o<<"  ";
	}
	if(depth){
		Branch *b=(Branch*)c;
		o<<"B "<<b<<":"<<b->parent;
		for(j=0;j<b->fillament;j++){
			o<<" "<<b->nums[j];
		}
		o<<"{\n";
		for(j=0;j<b->fillament;j++){
			output_node(o,b->children[j],tabs+1,depth-1);
		}
		for(j=0;j<tabs;j++){
			o<<"  ";
		}
		o<<"}\n";
	}else{
		Leaf *l=(Leaf*)c;
		for(j=0;j<l->fillament;j++){
			o<<l->elements[j]<<" ";
		}
		o<<" ("<<c<<")\n";
	}
}


template <typename T,int L,int M,typename A> template <class output_stream>
void alex_kupriianov_array<T,L,M,A>::
	__output(output_stream &o,const char *comm)
{
	o<<count<<" "<<comm<<"\n";
	if(count!=0){
		output_node(o,root,0,depth);
	}else{
		o<<root<<" "<<depth<<" Empty\n";
	}
} 	
