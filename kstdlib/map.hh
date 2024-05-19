/**
 * Implementation of a map using a red-black tree.
 * Not tested, used at your own risk.
 */

template <class T>
class rb_tree {
public:
    rb_tree();
    ~rb_tree();
    void insert(const T& value);
    void erase(const T& value);
    void clear();
    bool is_empty() const;
private:
    enum class color {
        RED,
        BLACK
    };
    struct node {
        T value;
        color color;
        node* parent;
        node* left;
        node* right;
    };
    node* root;
    void insert_fixup(node* n);
    void erase_fixup(node* n);
    void rotate_left(node* n);
    void rotate_right(node* n);
    void clear(node* n);

    void transplant(node* u, node* v);
    node* find_node(const T& value) const;
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
private:
    struct pair {
        T_key key;
        T_value value;
    };
    rb_tree<pair> tree;
};