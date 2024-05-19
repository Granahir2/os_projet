#pragma once
#include "map.hh"
#include "new.hh"

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
    n->color = color::RED;
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
    while (n->parent != nullptr && n->parent->color == color::RED) {
        if (n->parent == n->parent->parent->left) {
            node<T>* y = n->parent->parent->right;
            if (y != nullptr && y->color == color::RED) {
                n->parent->color = color::BLACK;
                y->color = color::BLACK;
                n->parent->parent->color = color::RED;
                n = n->parent->parent;
            } else {
                if (n == n->parent->right) {
                    n = n->parent;
                    rotate_left(n);
                }
                n->parent->color = color::BLACK;
                n->parent->parent->color = color::RED;
                rotate_right(n->parent->parent);
            }
        } else {
            node<T>* y = n->parent->parent->left;
            if (y != nullptr && y->color == color::RED) {
                n->parent->color = color::BLACK;
                y->color = color::BLACK;
                n->parent->parent->color = color::RED;
                n = n->parent->parent;
            } else {
                if (n == n->parent->left) {
                    n = n->parent;
                    rotate_right(n);
                }
                n->parent->color = color::BLACK;
                n->parent->parent->color = color::RED;
                rotate_left(n->parent->parent);
            }
        }
    }
    root->color = color::BLACK;
}

template <class T>
void rb_tree<T>::erase(const T& value) {
    node<T>* n = find_node<T>(value);
    if (n == nullptr) {
        return;
    }

    node<T>* y = n;
    color y_original_color = y->color;
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
        y_original_color = y->color;
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
        y->color = n->color;
    }

    if (y_original_color == color::BLACK) {
        erase_fixup(x);
    }

    delete n;
}

template <class T>
void rb_tree<T>::erase_fixup(node<T>* n) {
    while (n != root && n->color == color::BLACK) {
        if (n == n->parent->left) {
            node<T>* w = n->parent->right;
            if (w->color == color::RED) {
                w->color = color::BLACK;
                n->parent->color = color::RED;
                rotate_left(n->parent);
                w = n->parent->right;
            }
            if (w->left->color == color::BLACK && w->right->color == color::BLACK) {
                w->color = color::RED;
                n = n->parent;
            } else {
                if (w->right->color == color::BLACK) {
                    w->left->color = color::BLACK;
                    w->color = color::RED;
                    rotate_right(w);
                    w = n->parent->right;
                }
                w->color = n->parent->color;
                n->parent->color = color::BLACK;
                w->right->color = color::BLACK;
                rotate_left(n->parent);
                n = root;
            }
        } else {
            node<T>* w = n->parent->left;
            if (w->color == color::RED) {
                w->color = color::BLACK;
                n->parent->color = color::RED;
                rotate_right(n->parent);
                w = n->parent->left;
            }
            if (w->right->color == color::BLACK && w->left->color == color::BLACK) {
                w->color = color::RED;
                n = n->parent;
            } else {
                if (w->left->color == color::BLACK) {
                    w->right->color = color::BLACK;
                    w->color = color::RED;
                    rotate_left(w);
                    w = n->parent->left;
                }
                w->color = n->parent->color;
                n->parent->color = color::BLACK;
                w->left->color = color::BLACK;
                rotate_right(n->parent);
                n = root;
            }
        }
    }
    n->color = color::BLACK;
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
    node<T>* n = tree.find_node<T>(p);
    if (n == nullptr) {
        tree.insert(p);
        n = tree.find_node<T>(p);
    }
    return n->value.value;
}

template <class T_key, class T_value>
T_value& map<T_key, T_value>::at(const T_key& key) {
    pair<T_key, T_value> p;
    p.key = key;
    node<T>* n = tree.find_node<T>(p);
    if (n == nullptr) {
        throw std::out_of_range("Key not found");
    }
    return n->value.value;
}

template <class T_key, class T_value>
const T_value& map<T_key, T_value>::at(const T_key& key) const {
    pair<T_key, T_value> p;
    p.key = key;
    node<T>* n = tree.find_node<T>(p);
    if (n == nullptr) {
        throw std::out_of_range("Key not found");
    }
    return n->value.value;
}

template <class T_key, class T_value>
bool map<T_key, T_value>::contains(const T_key& key) const {
    pair<T_key, T_value> p;
    p.key = key;
    return tree.find_node<T>(p) != nullptr;
}

template <class T_key, class T_value>
bool map<T_key, T_value>::empty() const {
    return tree.is_empty();
}
