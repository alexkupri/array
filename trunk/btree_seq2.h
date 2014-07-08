//  Copyright (C) 2014 by Aleksandr Kupriianov
//  email: alexkupri host: gmail dot com

#include <stdexcept>
#include <algorithm>

/** @file btree_seq2.h
 * Implementation of btree_seq container, sequence based on btree.
 */

/// Moving elements while incrementing pointers
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::move_elements_inc(pointer dst,pointer src,size_type num)
{
	pointer limit=src+num;
	while(src!=limit){
		T_alloc.construct(dst,*src);
		T_alloc.destroy(src);
		++dst;
		++src;
	}
}

/// Moving elements while decrementing pointers
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::move_elements_dec(pointer dst,pointer limit,size_type num)
{
	pointer src=limit+num;
	dst+=num;
	while(src!=limit){
		--dst;
		--src;
		T_alloc.construct(dst,*src);
		T_alloc.destroy(src);
	}
}

///Filling leaf with elements, elements are read from general-type iterator
template <typename T,int L,int M,typename A>
template <class InputIterator>
typename btree_seq<T,L,M,A>::size_type btree_seq<T,L,M,A>::
	fill_elements(pointer dst,size_type num,InputIterator &first,InputIterator last,
	std::input_iterator_tag)
{
	pointer ptr=dst,limit=ptr+num;
	try{
		while((ptr!=limit)&&(first!=last)){
			T_alloc.construct(ptr,*first);
			++first;
			++ptr;
		}
	}catch(...){
		T *del_elems=dst;
		while(del_elems!=ptr){
			T_alloc.destroy(del_elems);
			del_elems++;
		}
		throw;
	}		
	return ptr-dst;
}

///Filling leaf with elements, elements are read from random access iterator
template <typename T,int L,int M,typename A>
template <class InputIterator>
typename btree_seq<T,L,M,A>::size_type btree_seq<T,L,M,A>::
	fill_elements(pointer dst,size_type num,InputIterator &first,InputIterator last,
	std::random_access_iterator_tag)
{
	size_type num2=last-first;
	if(num2<num){
		num=num2;
	}
	pointer ptr=dst,limit=ptr+num;
	try{
		while(ptr!=limit){
			T_alloc.construct(ptr,*first);
			++first;
			++ptr;
		}
	}catch(...){
		T *del_elems=dst;
		while(del_elems!=ptr){
			T_alloc.destroy(del_elems);
			del_elems++;
		}
		throw;
	}
	return ptr-dst;
}

///Destroying elements from leaf
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::burn_elements(pointer ptr,size_type num)
{
	while(num){
		T_alloc.destroy(ptr);
		ptr++;
		num--;
	}
}

///Preparing place for inserting children by moving some children.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::insert_children(Branch *b,size_type idx,size_type num)
{
	size_type j=b->fillament;
	while(j!=idx){
		j--;
		b->children[j+num]=b->children[j];
		b->nums[j+num]=b->nums[j];
	};
	b->fillament+=num;
}

///Delete some free places by moving some children.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::delete_children(Branch *b,size_type idx,size_type num)
{
	Node **children=b->children;
	size_type *nums=b->nums;
	for(size_type j=idx;j<b->fillament-num;j++){
		children[j]=children[j+num];
		nums[j]=nums[j+num];
	}
	b->fillament-=num;
}

///Moving some children from old parent (src) to new parent (dst),
///returning total amount elements in them.
template <typename T,int L,int M,typename A>
typename btree_seq<T,L,M,A>::size_type btree_seq<T,L,M,A>::move_children(
	Branch *dst,size_type idst,Branch *src,size_type isrc,size_type num)
{
	size_type j,res=0,cur;
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

/// Inserting leaves into branch, returning total number of elements in leaves
template <typename T,int L,int M,typename A>
typename btree_seq<T,L,M,A>::size_type
	btree_seq<T,L,M,A>::fill_leaves(Branch *b,size_type place,Leaf **l,size_type num)
{
	size_type j,res=0;
	for(j=0;j<num;j++){
		b->children[j+place]=l[j];
		b->nums[j+place]=l[j]->fillament;
		res+=l[j]->fillament;
		l[j]->parent=b;
	}
	return res;
}

///Trying to merge leaves (parent of the leaves and index of the left leaf are given).
template <typename T,int L,int M,typename A>
bool btree_seq<T,L,M,A>::try_merge_leaves(Branch *b,size_type idx)
{
	size_type l=b->nums[idx],r=b->nums[idx+1];
	if(l+r>M){
		return false;
	}
	Leaf *left=static_cast<Leaf*>(b->children[idx]),
			*right=static_cast<Leaf*>(b->children[idx+1]);
	move_elements_inc(left->elements+l,right->elements,r);
	left->fillament=l+r;
	b->nums[idx]=l+r;
	delete_leaf(right);
	return true;
}

///Balancing leaves by copying some elements from left to right
///(parent of leaves and index of the left leaf are given).
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::balance_leaves_lr(Branch *b,size_type idx)
{
	Leaf *left=static_cast<Leaf*>(b->children[idx]),
		  *right=static_cast<Leaf*>(b->children[idx+1]);
	size_type l=b->nums[idx],r=b->nums[idx+1];
	size_type moves=l-(r+l)/2;
	move_elements_dec(right->elements+moves,right->elements,r);
	move_elements_inc(right->elements,left->elements+l-moves,moves);
	left->fillament-=moves;
	right->fillament+=moves;
	b->nums[idx]-=moves;
	b->nums[idx+1]+=moves;
}

///Balancing leaves by copying some elements from right to left
///(parent of leaves and index of the left leaf are given).
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::balance_leaves_rl(Branch *b,size_type idx)
{
	Leaf *left=static_cast<Leaf*>(b->children[idx]),
			*right=static_cast<Leaf*>(b->children[idx+1]);
	size_type l=b->nums[idx],r=b->nums[idx+1];
	size_type moves=r-(r+l)/2;
	move_elements_inc(left->elements+l,right->elements,moves);
	move_elements_inc(right->elements,right->elements+moves,r-moves);
	left->fillament+=moves;
	right->fillament-=moves;
	b->nums[idx]+=moves;
	b->nums[idx+1]-=moves;
}

///Deleting the empty leaf.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::delete_leaf(Leaf *l)
{
	Branch *parent=l->parent;
	size_type idx;
	if(parent!=0){//special case: tree can be empty
		idx=find_child(parent,l);
		delete_children(parent,idx,1);
		underflow_branch(parent);
	}
	leaf_alloc.deallocate(l,1);
}

///Check for underflow of leaf (i.e. if it can be deleted, merged or balanced if too thin).
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::underflow_leaf(Leaf *l)
{
	if(l->fillament<M/2){
		Branch *parent=l->parent;
		if(l->fillament==0){
			delete_leaf(l);
		}else if(parent==0){
			//NOTHING, if the whole tree contains only one leaf, the leaf can be of any size.
		}else{
			size_type idx=find_child(parent,l);
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

///Trying to merge two branches into one (parent of branches and index of the left branch are given).
template <typename T,int L,int M,typename A>
bool btree_seq<T,L,M,A>::try_merge_branches(Branch *b,size_type idx)
{
	Branch *left=static_cast<Branch*>(b->children[idx]),
		   *right=static_cast<Branch*>(b->children[idx+1]);
	size_type l=left->fillament,r=right->fillament;
	if(l+r>L){
		return false;
	}
	move_children(left,left->fillament,right,0,right->fillament);
	left->fillament+=right->fillament;
	b->nums[idx]+=b->nums[idx+1];
	branch_alloc.deallocate(right,1);
	delete_children(b,idx+1,1);
	return true;
}

///Balancing branches by moving some nodes from left to right
///(parent of branches and index of the left branch are given).
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::balance_branch_lr(Branch *b,size_type idx)
{
	Branch *left=static_cast<Branch*>(b->children[idx]),
		*right=static_cast<Branch*>(b->children[idx+1]);
	size_type l=left->fillament,r=right->fillament,
		moves=l-(l+r)/2;
	insert_children(right,0,moves);
	size_type num=move_children(right,0,left,l-moves,moves);
	left->fillament-=moves;
	b->nums[idx]-=num;
	b->nums[idx+1]+=num;
}

///Balancing branches by moving some nodes from right to left
///(parent of branches and index of the left branch are given).
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::balance_branch_rl(Branch *b,size_type idx)
{
	Branch *left=static_cast<Branch*>(b->children[idx]),
		*right=static_cast<Branch*>(b->children[idx+1]);
	size_type l=left->fillament,r=right->fillament,
		moves=r-(l+r)/2;
	size_type num=move_children(left,l,right,0,moves);
	delete_children(right,0,moves);
	left->fillament+=moves;
	b->nums[idx]+=num;
	b->nums[idx+1]-=num;
}

///Deleting branch and underflow ancestors if necessary.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::underflow_branch(Branch *node)
{
	Branch *parent=node;
	size_type idx;
	while(parent->fillament<L/2){
		node=parent;
		parent=node->parent;
		if(parent==0){//that means that node is root,
				//which can have at least 2 children, no parent, no brothers
			if(node->fillament==1){//if root has only one child then level down
				root=node->children[0];
				root->parent=0;
				depth--;
				branch_alloc.deallocate(node,1);
			}
			break;
		}else{//node is not root: has parent, at least one adjastent brother
			idx=find_child(parent,node);//and must be balanced
			//if one of node's brothers is thin, we merge and erase some node
			if(idx>0){
				if(try_merge_branches(parent,idx-1)){
					//branch_alloc.deallocate(node,1);
					continue;
				}
			}
			if(idx<parent->fillament-1){
				if(try_merge_branches(parent,idx)){
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
}

///Find leaf and position of element in leaf, having position of the element.
template <typename T,int L,int M,typename A>
typename btree_seq<T,L,M,A>::size_type btree_seq<T,L,M,A>
	::find_leaf(Leaf *&l,size_type pos)const
{
	Node *node=root;
	Branch *br;
	size_type j=depth,k,val;
	while(j){
		k=0;
		br=static_cast<Branch*>(node);
		val=br->nums[0];
		while(pos>=val){
			pos-=val;
			k++;
			val=br->nums[k];
		}
		node=br->children[k];
		j--;
	}
	l=static_cast<Leaf*>(node);
	return pos;
}

///Find leaf and position of element in leaf, the position is given,
/// and increment counters by the way (we are going to add or remove
///some elements at this position).
///We can stop at depth_lim, not to go to the leaf, if we a going to
///insert/remove the whole subtree.
template <typename T,int L,int M,typename A>
typename btree_seq<T,L,M,A>::size_type
	btree_seq<T,L,M,A>::find_leaf(Node *&l,size_type pos,difference_type increment,size_type depth_lim)
{
	Node *node=root;
	Branch *br;
	size_type j=depth-depth_lim,k,val;
	while(j){
		k=0;
		br=static_cast<Branch*>(node);
		val=br->nums[0];
		while(pos>=val){
			pos-=val;
			k++;
			val=br->nums[k];
		}
		br->nums[k]+=increment;
		node=br->children[k];
		j--;
	}
	l=node;
	count+=increment;
	return pos;
}

///Find child in a branch
template <typename T,int L,int M,typename A>
typename btree_seq<T,L,M,A>::size_type btree_seq<T,L,M,A>::find_child
	(Branch *b,Node* child)
{
	for(size_type j=0;j<b->fillament;j++){
		if(child==b->children[j]){
			return j;
		}
	}
	assert(false);
	return 0;
}

///Initialize the empty tree (preparing for insert).
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::init_tree()
{
	Leaf *l=leaf_alloc.allocate(1);
	l->fillament=0;
	l->parent=0;
	root=l;
	depth=0;
}

///Actions necessary to increase the depth of the tree by one.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::increase_depth(Branch *new_branch)
{
	new_branch->fillament=1;
	new_branch->children[0]=root;
	new_branch->nums[0]=count;
	new_branch->parent=0;
	root->parent=new_branch;
	root=new_branch;
	depth++;
}


///We are going to split branches, so we count how many branches we need and reserve
///them in advance, building a list. This is done for not getting no_mem exception
///in the middle of splitting.
template <typename T,int L,int M,typename A>
typename btree_seq<T,L,M,A>::Branch *
	btree_seq<T,L,M,A>::reserve_enough_branches_splitting(Node* existing)
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

///Allocate the node (branch or leaf) and branches enough for splitting.
template <typename T,int L,int M,typename A>
template <typename Alloc,typename Node_type>
void btree_seq<T,L,M,A>::prepare_for_splitting(Branch *&branch_bundle,
		Node_type *&result,Node_type *existing,Alloc &alloc)
{
	result=alloc.allocate(1);
	try{
		branch_bundle=reserve_enough_branches_splitting(existing);
	}catch(...){
		alloc.deallocate(result,1);
		throw;
	}
}

///Splitting node. Given are: node to split, node that appears to the right from it and number of elements in the right node.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::split(Node* existing,Node* right_to_existing,
		size_type right_count,Branch *branch_bundle)
{
	size_type k,num=0;
	Branch *parent;
	//Branch *branch_bundle=reserve_enough_branches_splitting(existing);
	parent=existing->parent;
	if(parent==0){//special case: existing was root, we increase depth
		increase_depth(branch_bundle);
		parent=existing->parent;
	}
	k=find_child(parent,existing);
	add_child(parent,right_to_existing,right_count,k,1,branch_bundle);
}

///Adding child to the branch (while splitting its existing child),
///from left or right side (lr).
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::add_child(Branch* parent,Node* inserted,
		size_type count,size_type pos,size_type rl,Branch *branch_bundle)
{
	Branch *new_branch=0, *branch_to_insert=parent;
	size_type num=0;
	if(parent->fillament==L){//if we cannot fit into current branch, splitting
		new_branch=branch_bundle;//we plan to execute new iteration
		branch_bundle=branch_bundle->parent;
		num=move_children(new_branch,0,parent,L-L/2,L/2);
		parent->fillament=L-L/2;
		new_branch->fillament=L/2;
		if(pos>=L-L/2){
			branch_to_insert=new_branch;
			pos-=L-L/2;
		}
	}
	//inserting
	inserted->parent=branch_to_insert;
	insert_children(branch_to_insert,pos+rl,1);
	branch_to_insert->children[pos+rl]=inserted;
	branch_to_insert->nums[pos+rl]=count;
	branch_to_insert->nums[pos+1-rl]-=count;
	//performing next splitting, if necessary
	if(new_branch!=0){
		split(parent,new_branch,num,branch_bundle);
	}
}

///After some operations, like multiple delete or multiple insert,
///thin leaves and branches can occur at certain positions.
///We need to check and underfow.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::deep_sew(size_type pos)
{
	Node *n=root;
	size_type dep=depth;
	size_type j,cur=pos;
	for(;;){
		if(dep==0){
			Leaf* l=static_cast<Leaf*>(n);
			if((depth>0)&&(l->fillament<M/2)){
				underflow_leaf(l);
			}
			return;
		}else{
			Branch *b=static_cast<Branch*>(n);
			if((b->fillament==1)){
				underflow_branch(b);
				dep=depth;
				n=root;
				cur=pos;
				//start from the beginning
			}else{
				j=0;
				//find child for next iteration
				while(cur>=b->nums[j]){
					cur-=b->nums[j];
					j++;
				}
				n=b->children[j];
				dep--;
				//underflow
				if((b->parent!=0)&&(b->fillament<L/2)){
					underflow_branch(b);
				}
			}
		}
	}
}

///
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::my_deep_sew(size_type pos)
{
	if(pos!=0){
		deep_sew(pos-1);
	}
	if(pos!=count){
		deep_sew(pos);
	}
}

///Some special cases for quick sewing together.
///We are given leaf to the left of isertion and position.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::advanced_sew_together
	(Leaf *last_leaf,size_type pos)
{
	Branch *parent=last_leaf->parent;
	if(pos==count){
		underflow_leaf(last_leaf);
		return;
	}else if(parent!=0){
		size_type j=find_child(parent,last_leaf);
		if(j!=parent->fillament-1){
			if(try_merge_leaves(parent,j)){//both leaves are small
				underflow_leaf(last_leaf);//underflow merged leaf
			}else if(last_leaf->fillament<M/2){//left leaf is small
				underflow_leaf(last_leaf);
			}else{//right or no leaf is small
				underflow_leaf(static_cast<Leaf*>(parent->children[j+1]));
			}
			return;
		}
	}
	my_deep_sew(pos);
}

///Fast function for inserting few amount of elements (no more than one splitting).
template <typename T,int L,int M,typename A> template <class InputIterator>
void btree_seq<T,L,M,A>::insert_no_more_half_leaf
	(size_type pos,InputIterator first,InputIterator last,size_type num)
{
	Leaf *l,*l2=0;
	Node *nod;
	size_type found,delta=0,fill,place_of_splitting=pos;
	if(count==0){
		init_tree();
	}
	if(pos!=0){
		delta=1;
	}
	found=find_leaf(nod,pos-delta,num)+delta;//find leaf and increment counter
	l=static_cast<Leaf*>(nod);
	fill=l->fillament;
	if(fill+num>M){
		Leaf *newleaf,*leaf_to_ins=l;
		Branch *branch_bundle=0;
		size_type new_found=found,old_leaf=fill-fill/2,addition=0;
		place_of_splitting=pos-found+old_leaf;
		try{
			prepare_for_splitting(branch_bundle,newleaf,l,leaf_alloc);
		}
		catch(...){
			find_leaf(nod,pos-delta,-num);
			throw;
		}
		l2=newleaf;
		newleaf->fillament=fill-old_leaf;
		if(new_found>old_leaf){
			leaf_to_ins=newleaf;
			found-=old_leaf;
			addition=num;
			l2=l;
		}
		split(l,newleaf,newleaf->fillament+addition,branch_bundle);//splitting the leaf, that was overflowed
		move_elements_inc(newleaf->elements,l->elements+old_leaf,fill-old_leaf);
		l->fillament=old_leaf;
		l=leaf_to_ins;
	}
	move_elements_dec(l->elements+found+num,l->elements+found,l->fillament-found);
	l->fillament+=num;
	try{
		fill_elements(l->elements+found,num,first,last,
				typename std::iterator_traits<InputIterator>::iterator_category());
	}catch(...){
		l->fillament-=num;
		move_elements_inc(l->elements+found,l->elements+found+num,l->fillament-found);
		find_leaf(nod,pos-delta,-num);
		underflow_leaf(l);//necessary: if l is empty
		my_deep_sew(place_of_splitting);//necessary if both sides are small
		throw;
	}
	if(l2!=0){
		underflow_leaf(l2);//Check if the smallest leaf needs underflow
	}
}

///Helper function for mass insert. It ensures, that all consequent inserting can be done
///by inserting the whole leafs. Firstly, it cuts leaf in the place of insertion if necessary.
///Secondly, it fills the left leaf with elements until it is full.
template <typename T,int L,int M,typename A> template <class InputIterator>
typename btree_seq<T,L,M,A>::Leaf
	*btree_seq<T,L,M,A>::start_inserting
	(size_type &pos,InputIterator &first,InputIterator last)
{
	size_type n=0,found;
	Leaf *new_leaf=0,*l=0;
	Node *dummy;
	Branch *branch_bundle=0;
	if(count==0){
		init_tree();
	}else if(pos==0){
		return 0;
	}
	found=find_leaf(l,pos-1)+1;
	if(found!=l->fillament){//we need to split existing leaf first
		prepare_for_splitting(branch_bundle,new_leaf,l,leaf_alloc);
		move_elements_inc(new_leaf->elements,l->elements+found,l->fillament-found);
		new_leaf->fillament=l->fillament-found;
		split(l,new_leaf,new_leaf->fillament,branch_bundle);
		l->fillament=found;
	}
	//we fill last leaf before gap as good as we can
	try{
		n=fill_elements(l->elements+found,M-found,first,last,
			typename std::iterator_traits<InputIterator>::iterator_category());
	}
	catch(...){
		underflow_leaf(l);//this is for case of empty tree
		my_deep_sew(pos);//this is for case splitting
		throw;
	}
	l->fillament+=n;
	find_leaf(dummy,pos-1,n);
	pos+=n;
	return l;
}

///Inserting multiple leaves (not more than L-1) at given position.
///Assuming that position is between leaves.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::insert_leaves(Leaf **l,size_type num_leaves,
	size_type num_elems,size_type pos)
{
	if(depth==0){
		Branch *new_branch=branch_alloc.allocate(1);
		increase_depth(new_branch);
	}
	Node *left_neighbour;
	find_leaf(left_neighbour,pos?pos-1:0,num_elems);
	Branch *parent=left_neighbour->parent;
	Branch *new_branch=0,*branch_bundle=0;
	size_type k;
	int place=find_child(parent,left_neighbour),oldplace=place;
	if(pos!=0){
		place++;
	}
	if(parent->fillament+num_leaves>L){
		//Refactor these cycles
		Leaf *nodes[L*2];
		Leaf **dst=nodes;
		try{
			prepare_for_splitting(branch_bundle,new_branch,parent,branch_alloc);
		}catch(...){
			find_leaf(left_neighbour,pos?pos-1:0,-num_elems);
			throw;
		}
		int j,n=parent->fillament,first,sum=0;
		for(j=0;j<place;j++){
			*dst++=static_cast<Leaf*>(parent->children[j]);
		}
		for(j=0;j<num_leaves;j++){
			*dst++=l[j];
		}
		for(j=place;j<n;j++){
			*dst++=static_cast<Leaf*>(parent->children[j]);
		}
		n+=num_leaves;
		first=n/2;
		fill_leaves(parent,0,nodes,first);
		sum=fill_leaves(new_branch,0,nodes+first,n-first);
		parent->fillament=first;
		new_branch->fillament=n-first;
		split(parent,new_branch,sum,branch_bundle);
	}else{
		parent->nums[oldplace]-=num_elems;
		insert_children(parent,place,num_leaves);
		fill_leaves(parent,place,l,num_leaves);
	}
}

///Helper function for multiple insert. Creates and inserts the whole leaves.
template <typename T,int L,int M,typename A> template <class InputIterator>
typename btree_seq<T,L,M,A>::Leaf
	*btree_seq<T,L,M,A>::insert_whole_leaves
		(size_type startpos,size_type &pos,InputIterator first,InputIterator last,Leaf *last_leaf)
{
	Leaf *l[L];
	size_type n,total_sum,leaf_num;
	try{
		while(first!=last){
			total_sum=0;
			leaf_num=0;
			while((first!=last)&&(leaf_num<L-1)){
				last_leaf=leaf_alloc.allocate(1);
				last_leaf->fillament=0;
				l[leaf_num]=last_leaf;
				leaf_num++;
				n=fill_elements(last_leaf->elements,M,first,last,
					typename std::iterator_traits<InputIterator>::iterator_category());
				last_leaf->fillament=n;
				total_sum+=n;
			}
			insert_leaves(l,leaf_num,total_sum,pos);
			pos+=total_sum;
		}
	}
	catch(...)
	{
		size_type k;
		for(k=0;k<leaf_num;k++){
			burn_elements(l[k]->elements,l[k]->fillament);
			leaf_alloc.deallocate(l[k],1);
		}
		erase(startpos,pos);
		my_deep_sew(startpos);
		throw;
	}
	return last_leaf;
}

//Implementation of the public insert function.
template <typename T,int L,int M,typename A> template <class InputIterator>
void btree_seq<T,L,M,A>::insert
	(size_type pos,InputIterator first,InputIterator last)
{
	size_type n,startpos=pos;
	if(first==last){
		return;
	}
	n=count_difference(first,last,M/2+1,
		typename std::iterator_traits<InputIterator>::iterator_category());
	if(n<=M/2){
		insert_no_more_half_leaf(pos,first,last,n);
	}else{
		Leaf *last_leaf=start_inserting(pos,first,last);
		last_leaf=insert_whole_leaves(startpos,pos,first,last,last_leaf);
		advanced_sew_together(last_leaf,pos);
	}
}

///The common engine for deletion of elements and visiting them.
///Params: action to perform, node to perform on, interval [start,start+diff) relatively to that node
///depth from the node to the bottom.
template <typename T,int L,int M,typename A> template<typename Action>
bool btree_seq<T,L,M,A>::recursive_action(Action &act,size_type start,size_type diff,size_type depth,Node *node)
{
	while(depth>0){
		if(diff==0){
			return false;
		}
		Branch* b=static_cast<Branch*>(node);
		size_type j=0,k,del;
		while(start>=b->nums[j]){
			start-=b->nums[j];
			j++;
		}
		if((start+diff<=b->nums[j])&&(diff<b->nums[j])){
			act.decrement_value(b->nums[j],diff);
			node=b->children[j];
			depth--;
			continue;
		}
		if(start>0){
			del=diff;
			if(b->nums[j]-start<del){
				del=b->nums[j]-start;
			}
			if(recursive_action(act,start,del,depth-1,b->children[j])){
				return true;
			}
			start+=del;
			diff-=del;
			act.decrement_value(b->nums[j],del);
			j++;
		}
		k=j;
		while((diff>=b->nums[k])&&(diff>0)){
			if(recursive_action(act,0,b->nums[k],depth-1,b->children[k])){
				return true;
			}
			start+=b->nums[k];
			diff-=b->nums[k];
			k++;
		}
		if(diff>0){
			if(recursive_action(act,0,diff,depth-1,b->children[k])){
				return true;
			}
			act.decrement_value(b->nums[k],diff);
		}
		if(act.shift_array()){
			move_children(b,j,b,k,b->fillament-k);
			b->fillament=b->fillament+j-k;
			if(b->fillament==0){
				branch_alloc.deallocate(b,1);
			}
		}
		return false;
	}
	return act.process_leaf(static_cast<Leaf*>(node),start,start+diff);
}

///Processing leaf while deleting elements.
template <typename T,int L,int M,typename A> 
bool btree_seq<T,L,M,A>::erase_helper::process_leaf(Leaf *l,size_type start,size_type end)
{
	leaves++;
	if(l->fillament==end-start){
		aka.burn_elements(l->elements,end-start);
		aka.leaf_alloc.deallocate(l,1);
		last_leaf=0;
	}else{
		aka.burn_elements(l->elements+start,end-start);
		aka.move_elements_inc(l->elements+start,l->elements+end,l->fillament-end);
		l->fillament=l->fillament+start-end;
		last_leaf=l;
	}
	return false;
}

//Implementation of the public erase function.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::erase(size_type first,size_type last)
{
	if(first==last){
		return;
	}
	erase_helper eh(*this);
	recursive_action(eh,first,last-first,depth,root);
	count=count+first-last;
	if((eh.num_leaves()==1)&&(eh.get_last_leaf()!=0)){
		underflow_leaf(eh.get_last_leaf());
	}else{
		my_deep_sew(first);
	}
}

///Processing leaf while visiting elements.
template <typename T,int L,int M,typename A> template<typename V>
bool btree_seq<T,L,M,A>::visitor_helper<V>::
	process_leaf(Leaf *l,size_type start,size_type end)
{
	T *p1=l->elements+start,*p2=l->elements+end;
	while(p1!=p2){
		if(v(*p1)){
			iters+=(p1-l->elements)-start;
			return true;
		}
		p1++;
	}
	iters+=end-start;
	return false;
}

//Implementation of the public visit function.
template <typename T,int L,int M,typename A> template<typename V>
typename btree_seq<T,L,M,A>::size_type
	btree_seq<T,L,M,A>::visit(size_type start,size_type end,V& v)
{
	visitor_helper<V> vh(v);
	recursive_action(vh,start,end-start,depth,root);
	return start+vh.get_iters();
}

///Concateneting that (small) tree to this big one, from the left or right side.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::insert_tree(btree_seq<T,L,M,A> &that,bool last)
{
	Branch *branch_bundle=0,*parent;
	Node *l;
	size_type pos=last?count-1:0;
	find_leaf(l,pos,that.count,that.depth);
	parent=l->parent;
	try{
		branch_bundle=reserve_enough_branches_splitting(l);
	}catch(...){
		find_leaf(l,pos,-that.count,that.depth);
		throw;
	}
	add_child(parent,that.root,that.count,last?parent->fillament-1:0,last?1:0,branch_bundle);
	that.depth=0;
	that.count=0;
}

//Implementation of public function concatenate_right.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::concatenate_right(btree_seq<T,L,M,A> &that)
{
	size_type pos,curdep;
	Branch *b;
	if(count==0){
		swap(that);
		return;
	}
	if(that.count==0){
		return;
	}
	pos=count;
	if(that.depth==depth){
		b=branch_alloc.allocate(1);
		increase_depth(b);
	}
	if(that.depth<depth){
		//we want to attach that (small) tree to this big from the right
		insert_tree(that,true);
	}else {
		//we want to attach this (small) tree to that big from the left
		that.insert_tree(*this,false);
		swap(that);
	}
	my_deep_sew(pos);
}

///Detaching some (smaller) part of the tree to that tree from the left or right.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::detach_some
	(btree_seq<T,L,M,A> &that,Branch *b,size_type dep,bool last)
{
	Node *dummy;
	size_type idx=last?b->fillament-1:0,pos=last?count-1:0;
	that.clear();
	that.root=b->children[idx];
	that.root->parent=0;
	that.depth=dep;
	that.count=b->nums[idx];
	find_leaf(dummy,pos,-that.count,dep);
	delete_children(b,idx,1);
}

//Implementation of the public split_right function.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::split_right
	(btree_seq<T,L,M,A> &that,size_type pos)
{
	Branch *branch_bundle=0;
	if(pos==count){
		that.clear();
		return;
	}	
	if(pos==0){
		that.clear();
		swap(that);
		return;
	}
	//This function splits nodes and leafs from bottom to top
	//until leftmost or rightmost branch represent one of desired parts
	//of splitting operations.
	try{
		Leaf *l;
		size_type found=find_leaf(l,pos-1)+1;
		Branch *parent,*new_branch;
		size_type dep=0,idx,num;
		if(found!=l->fillament){//Splitting leaf first
			Leaf *new_leaf=0;
			prepare_for_splitting(branch_bundle,new_leaf,l,leaf_alloc);
			num=l->fillament;
			move_elements_inc(new_leaf->elements,l->elements+found,num-found);
			new_leaf->fillament=num-found;
			l->fillament=found;
			split(l,new_leaf,new_leaf->fillament,branch_bundle);
		}
		Node *n=l;
		for(;;){
			parent=n->parent;
			idx=find_child(parent,n);
			if((idx==0)&&(parent->nums[0]==pos)){
				//If complete detach left can be performed at this level.
				detach_some(that,parent,dep,false);
				swap(that);
				break;
			}
			if((idx==parent->fillament-2)&&(parent->nums[parent->fillament-1]==count-pos)){
				//If complete detach right can be performaed at this level.
				detach_some(that,parent,dep,true);
				break;
			}
			if(idx!=parent->fillament-1){
				//Split the current branch, if necessary.
				prepare_for_splitting(branch_bundle,new_branch,parent,branch_alloc);
				new_branch->fillament=parent->fillament-idx-1;
				num=move_children(new_branch,0,parent,idx+1,new_branch->fillament);
				parent->fillament-=new_branch->fillament;
				split(parent,new_branch,num,branch_bundle);
			}
			n=parent;
			dep++;
		}
		deep_sew(pos-1);
		that.deep_sew(0);
	}catch(...){
		my_deep_sew(pos);
		throw;
	}
}

//Implementation of the public concatenate_left function.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::concatenate_left(btree_seq<T,L,M,A> &that)
{
	that.concatenate_right(*this);
	swap(that);	
}

//Implementation of the public split_left function.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::split_left(btree_seq<T,L,M,A> &that,size_type pos)
{
	swap(that);	
	that.split_right(*this,pos);
}

//Implementation of the public assign function.
template <typename T,int L,int M,typename A>
	void btree_seq<T,L,M,A>::assign(size_type n,const value_type &val)
{
	clear();
	fill(0,n,val);
}

//Assert that n<count
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::assert_range(size_type n)
{
	if(n>=size()){
		throw std::out_of_range("Index exceeds container size.");
	}
}

//Implementation of the public resize function.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::resize(size_type n,const value_type& val)
{
	if(n<size()){
		erase(n,size());
	}else if(n>size()){
		fill(size(),n-size(),val);
	}
}

//Implementation of the public swap function.
template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::swap(btree_seq<T,L,M,A> &that)
{
	std::swap(root,that.root);
	std::swap(count,that.count);
	std::swap(depth,that.depth);
}

///Iterator rebase function.
///It adjusts iterator's data according to tree and abs_idx.
template <typename T,int L,int M,typename A>
template <typename TT>
typename btree_seq<T,L,M,A>::template iterator_base<TT>::pointer
	btree_seq<T,L,M,A>::iterator_base<TT>::rebase()const
{
	size_type t1;
	Leaf *l;
	//we access element, so iterator must point to correct value
	//we do these asserts only once per leaf, so debug version is not very slow
	//Firstly, we are trying some simple fast cases
	if(tree->depth==0){//tree has only one leaf, we don't call find_leaf
		l=static_cast<Leaf*>(tree->root);
		rel_idx=abs_idx;
		br=0;
	}else{
		t1=rel_idx-avail_ptr;
		if((t1<M/2)&&(br!=0)&&(br->fillament!=idx_in_br+1)){//we want next leaf, it's faster than find_leaf
			rel_idx-=avail_ptr;
			idx_in_br++;
			l=static_cast<Leaf*>(br->children[idx_in_br]);
		}else{
			t1=-rel_idx;
			if((t1<M/2)&&(br!=0)&&(idx_in_br!=0)){//previous leaf
				idx_in_br--;
				l=static_cast<Leaf*>(br->children[idx_in_br]);
				rel_idx+=l->fillament;
			}else{
				rel_idx=tree->find_leaf(l,abs_idx);//general case
				br=l->parent;
				idx_in_br=find_child(br,l);
			}
		}
	}
	avail_ptr=l->fillament;
	elems=&l->elements[0];
	return elems+rel_idx;
}


//==================== Debug functions ==============

template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::my_assert(bool b,const char *msg)
{
	if(!b){
		throw std::runtime_error(msg);
	}
}

template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::check_node(Node* c,size_type sum,bool head,size_type depth,Branch *parent)
{
	size_type j,summ=0;
	if(depth>0){
		Branch *b=static_cast<Branch*>(c);
		if(!head){
			my_assert(b->fillament>=L/2,"Branches must be filled.");
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
		Leaf *l=static_cast<Leaf*>(c);
		my_assert(l->fillament>=1,"Leaf must have at least one element.");
		if(!head){
			my_assert(l->fillament>=M/2,"In multileaf tree, leaves must be at least half-filled.");
		}
		my_assert(l->fillament==sum,"Sum is the number of children in leaf.");
		my_assert(l->parent==parent,"Parent in leaf must be correct.");
	}
}

template <typename T,int L,int M,typename A>
void btree_seq<T,L,M,A>::__check_consistency()
{
	if(count!=0){
		check_node(root,count,true,depth,0);
	}
}

template <typename T,int L,int M,typename A> template <class output_stream>
void btree_seq<T,L,M,A>::
	output_node(output_stream &o,Node* c,size_type tabs,size_type depth)
{
	size_type j;
	for(j=0;j<tabs;j++){
		o<<"  ";
	}
	if(depth){
		Branch *b=static_cast<Branch*>(c);
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
		Leaf *l=static_cast<Leaf*>(c);
		for(j=0;j<l->fillament;j++){
			o<<l->elements[j]<<" ";
		}
		o<<" ("<<c<<")\n";
	}
}

template <typename T,int L,int M,typename A> template <class output_stream>
void btree_seq<T,L,M,A>::
	__output(output_stream &o,const char *comm)
{
	o<<count<<" "<<comm<<"\n";
	if(count!=0){
		output_node(o,root,0,depth);
	}else{
		o<<root<<" "<<depth<<" Empty\n";
	}
}

