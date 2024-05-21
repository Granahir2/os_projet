#pragma once
#include "stdexcept.hh"

/**
 * Implementation of a map using a red-black tree.
 * Not tested, used at your own risk.
 */

enum class color {
    RED,
    BLACK
};

template <class T>
struct node {
    T value;
    color colo;
    node<T>* parent;
    node<T>* left;
    node<T>* right;
};

template <class T>
class rb_tree {
public:
    node<T>* root;

    rb_tree();
    ~rb_tree();
    void insert(const T& value);
    void erase(const T& value);
    void clear();
    bool is_empty() const;
    node<T>* find_node(const T& value) const;

private:
    void insert_fixup(node<T>* n);
    void erase_fixup(node<T>* n);
    void rotate_left(node<T>* n);
    void rotate_right(node<T>* n);
    void clear(node<T>* n);

    void transplant(node<T>* u, node<T>* v);
};
template <class T_key, class T_value>
struct pair {
    T_key key;
    T_value value;

    bool operator<(const pair<T_key, T_value>& other) const {
        return key < other.key;
    }
};

template <class T_key, class T_value>
class map {
public:
    map();
    ~map();
    void insert(const T_key& key, const T_value& value);
    void erase(const T_key& key);
    void clear();
    T_value& operator[](const T_key& key);
    T_value& at(const T_key& key);
    const T_value& at(const T_key& key) const;
    bool contains(const T_key& key) const;
    size_t size() const;
    bool empty() const;

    
    rb_tree<pair<T_key, T_value> > tree;
};


template <class T>
rb_tree<T>::rb_tree() {
    root = nullptr;
}

template <class T>
rb_tree<T>::~rb_tree() {
    clear();
}

template <class T>
void rb_tree<T>::clear() {
    clear(root);
    root = nullptr;
}

template <class T>
bool rb_tree<T>::is_empty() const {
    return root == nullptr;
}

template <class T>
void rb_tree<T>::insert(const T& value) {
    node<T>* n = new node<T>;
    n->value = value;
    n->colo = color::RED;
    n->left = nullptr;
    n->right = nullptr;
    n->parent = nullptr;

    node<T>* y = nullptr;
    node<T>* x = root;
    while (x != nullptr) {
        y = x;
        if (value < x->value) {
            x = x->left;
        } else {
            x = x->right;
        }
    }

    n->parent = y;
    if (y == nullptr) {
        root = n;
    } else if (value < y->value) {
        y->left = n;
    } else {
        y->right = n;
    }

    insert_fixup(n);
}

template <class T>
void rb_tree<T>::insert_fixup(node<T>* n) {
    while (n->parent != nullptr && n->parent->colo == color::RED) {
        if (n->parent == n->parent->parent->left) {
            node<T>* y = n->parent->parent->right;
            if (y != nullptr && y->colo == color::RED) {
                n->parent->colo = color::BLACK;
                y->colo = color::BLACK;
                n->parent->parent->colo = color::RED;
                n = n->parent->parent;
            } else {
                if (n == n->parent->right) {
                    n = n->parent;
                    rotate_left(n);
                }
                n->parent->colo = color::BLACK;
                n->parent->parent->colo = color::RED;
                rotate_right(n->parent->parent);
            }
        } else {
            node<T>* y = n->parent->parent->left;
            if (y != nullptr && y->colo == color::RED) {
                n->parent->colo = color::BLACK;
                y->colo = color::BLACK;
                n->parent->parent->colo = color::RED;
                n = n->parent->parent;
            } else {
                if (n == n->parent->left) {
                    n = n->parent;
                    rotate_right(n);
                }
                n->parent->colo = color::BLACK;
                n->parent->parent->colo = color::RED;
                rotate_left(n->parent->parent);
            }
        }
    }
    root->colo = color::BLACK;
}

template <class T>
void rb_tree<T>::erase(const T& value) {
    node<T>* n = find_node(value);
    if (n == nullptr) {
        return;
    }

    node<T>* y = n;
    color y_original_color = y->colo;
    node<T>* x = nullptr;
    if (n->left == nullptr) {
        x = n->right;
        transplant(n, n->right);
    } else if (n->right == nullptr) {
        x = n->left;
        transplant(n, n->left);
    } else {
        y = n->right;
        while (y->left != nullptr) {
            y = y->left;
        }
        y_original_color = y->colo;
        x = y->right;
        if (y->parent == n) {
            if (x != nullptr) {
                x->parent = y;
            }
        } else {
            transplant(y, y->right);
            y->right = n->right;
            y->right->parent = y;
        }
        transplant(n, y);
        y->left = n->left;
        y->left->parent = y;
        y->colo = n->colo;
    }

    if (y_original_color == color::BLACK) {
        erase_fixup(x);
    }

    delete n;
}

template <class T>
void rb_tree<T>::erase_fixup(node<T>* n) {
    while (n != root && n->colo == color::BLACK) {
        if (n == n->parent->left) {
            node<T>* w = n->parent->right;
            if (w->colo == color::RED) {
                w->colo = color::BLACK;
                n->parent->colo = color::RED;
                rotate_left(n->parent);
                w = n->parent->right;
            }
            if (w->left->colo == color::BLACK && w->right->colo == color::BLACK) {
                w->colo = color::RED;
                n = n->parent;
            } else {
                if (w->right->colo == color::BLACK) {
                    w->left->colo = color::BLACK;
                    w->colo = color::RED;
                    rotate_right(w);
                    w = n->parent->right;
                }
                w->colo = n->parent->colo;
                n->parent->colo = color::BLACK;
                w->right->colo = color::BLACK;
                rotate_left(n->parent);
                n = root;
            }
        } else {
            node<T>* w = n->parent->left;
            if (w->colo == color::RED) {
                w->colo = color::BLACK;
                n->parent->colo = color::RED;
                rotate_right(n->parent);
                w = n->parent->left;
            }
            if (w->right->colo == color::BLACK && w->left->colo == color::BLACK) {
                w->colo = color::RED;
                n = n->parent;
            } else {
                if (w->left->colo == color::BLACK) {
                    w->right->colo = color::BLACK;
                    w->colo = color::RED;
                    rotate_left(w);
                    w = n->parent->left;
                }
                w->colo = n->parent->colo;
                n->parent->colo = color::BLACK;
                w->left->colo = color::BLACK;
                rotate_right(n->parent);
                n = root;
            }
        }
    }
    n->colo = color::BLACK;
}

template <class T>
void rb_tree<T>::rotate_left(node<T>* n) {
    node<T>* y = n->right;
    n->right = y->left;
    if (y->left != nullptr) {
        y->left->parent = n;
    }
    y->parent = n->parent;
    if (n->parent == nullptr) {
        root = y;
    } else if (n == n->parent->left) {
        n->parent->left = y;
    } else {
        n->parent->right = y;
    }
    y->left = n;
    n->parent = y;
}

template <class T>
void rb_tree<T>::rotate_right(node<T>* n) {
    node<T>* y = n->left;
    n->left = y->right;
    if (y->right != nullptr) {
        y->right->parent = n;
    }
    y->parent = n->parent;
    if (n->parent == nullptr) {
        root = y;
    } else if (n == n->parent->right) {
        n->parent->right = y;
    } else {
        n->parent->left = y;
    }
    y->right = n;
    n->parent = y;
}

template <class T>
void rb_tree<T>::clear(node<T>* n) {
    if (n == nullptr) {
        return;
    }
    clear(n->left);
    clear(n->right);
    delete n;
}

template <class T>
void rb_tree<T>::transplant(node<T>* u, node<T>* v) {
    if (u->parent == nullptr) {
        root = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }
    if (v != nullptr) {
        v->parent = u->parent;
    }
}

template <class T>
node<T>* rb_tree<T>::find_node(const T& value) const {
    node<T>* n = root;
    while (n != nullptr) {
        if (value < n->value) {
            n = n->left;
        } else if (n->value < value) {
            n = n->right;
        } else {
            return n;
        }
    }
    return nullptr;
}

template <class T_key, class T_value>
map<T_key, T_value>::map(): tree() {}

template <class T_key, class T_value>
map<T_key, T_value>::~map() {
    tree.clear();
}

template <class T_key, class T_value>
void map<T_key, T_value>::insert(const T_key& key, const T_value& value) {
    pair<T_key, T_value> p;
    p.key = key;
    p.value = value;
    tree.insert(p);
}

template <class T_key, class T_value>
void map<T_key, T_value>::erase(const T_key& key) {
    pair<T_key, T_value> p;
    p.key = key;
    tree.erase(p);
}

template <class T_key, class T_value>
void map<T_key, T_value>::clear() {
    tree.clear();
}

template <class T_key, class T_value>
T_value& map<T_key, T_value>::operator[](const T_key& key) {
    pair<T_key, T_value> p;
    p.key = key;
    node<pair<T_key, T_value>>* n = tree.find_node(p);
    if (n == nullptr) {
        tree.insert(p);
        n = tree.find_node(p);
    }
    return n->value.value;
}

template <class T_key, class T_value>
T_value& map<T_key, T_value>::at(const T_key& key) {
    pair<T_key, T_value> p;
    p.key = key;
    node<decltype(p)>* n = tree.find_node(p);
    if (n == nullptr) {
        throw out_of_range("Key not found");
    }
    return n->value.value;
}

template <class T_key, class T_value>
const T_value& map<T_key, T_value>::at(const T_key& key) const {
    pair<T_key, T_value> p;
    p.key = key;
    node<decltype(p)>* n = tree.find_node(p);
    if (n == nullptr) {
        throw out_of_range("Key not found");
    }
    return n->value.value;
}

template <class T_key, class T_value>
bool map<T_key, T_value>::contains(const T_key& key) const {
    pair<T_key, T_value> p;
    p.key = key;
    return tree.find_node(p) != nullptr;
}

template <class T_key, class T_value>
bool map<T_key, T_value>::empty() const {
    return tree.is_empty();
}
