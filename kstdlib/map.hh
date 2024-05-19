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
    color color;
    node* parent;
    node* left;
    node* right;
};

template <class T>
class rb_tree {
public:
    rb_tree();
    ~rb_tree();
    void insert(const T& value);
    void erase(const T& value);
    void clear();
    bool is_empty() const;

    node<T>* root;
private:
    void insert_fixup(node<T>* n);
    void erase_fixup(node<T>* n);
    void rotate_left(node<T>* n);
    void rotate_right(node<T>* n);
    void clear(node<T>* n);

    void transplant(node<T>* u, node<T>* v);
    node<T>* find_node(const T& value) const;
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