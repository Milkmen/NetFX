#pragma once

#include <vector>

template <typename T>
class LCRS_Node
{
public:
	T value;
	LCRS_Node<T>* parent;
	LCRS_Node<T>* sibling;
	LCRS_Node<T>* child;
	LCRS_Node
	(
		T value,
		LCRS_Node<T>* parent,
		LCRS_Node<T>* sibling,
		LCRS_Node<T>* child
	)
	{
		this->value = value;
		this->parent = parent;
		this->sibling = sibling;
		this->child = child;
	}
};

template <typename T>
class LCRS_Tree
{
private:
	std::vector<LCRS_Node<T>*> nodes;
	LCRS_Node<T>* root = nullptr;
public:
	~LCRS_Tree() 
	{
		for (LCRS_Node<T>* n : this->nodes)
			delete n;
	}

	LCRS_Node<T>* createRoot(T value) 
	{
		if (this->root) return 0;
		this->root = new LCRS_Node<T>(value, nullptr, nullptr, nullptr);
		this->nodes.push_back(this->root);
		return this->root;
	}

	LCRS_Node<T>* getRoot() const 
	{
		return this->root;
	}

	LCRS_Node<T>* addChild(LCRS_Node<T>* node, T value) 
	{
		this->nodes.push_back(new LCRS_Node<T>(value, node, 0, 0));

		LCRS_Node<T>* nodeptr = this->nodes.back();

		if (!node->child)
		{
			node->child = nodeptr;
			return nodeptr;
		}

		LCRS_Node<T>* check = node->child;
		while (check->sibling) check = check->sibling;
		check->sibling = nodeptr;
		return nodeptr;
	}
	LCRS_Node<T>* addSibling(LCRS_Node<T>* node, T value)
	{
		this->nodes.push_back(new LCRS_Node<T>(value, node, 0, 0));

		LCRS_Node<T>* nodeptr = this->nodes.back();

		LCRS_Node<T>* check = node;
		while (check->sibling) check = check->sibling;
		check->sibling = nodeptr;
		return nodeptr;
	}
};