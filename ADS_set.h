
//#ifndef ADS_SET_H_ADS_SET_H
//#define ADS_SET_H_ADS_SET_H

#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>

template <typename Key, size_t N = 7>
class ADS_set {
public:
    class Iterator;
    using value_type = Key;
    using key_type = Key;
    using reference = value_type &;
    using const_reference = const value_type &;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using const_iterator = Iterator;
    using iterator = const_iterator;
    using key_equal = std::equal_to<key_type>;                       // Hashing
    using hasher = std::hash<key_type>;                              // Hashing
private:
    struct Element{                 //Element with a key and a pointer that points to the next Element in the list
        key_type key;
        Element* next {nullptr};

        ~Element(){delete next;};
    };
    struct List{
        Element* head{nullptr};    // First Element of the List

        ~List(){delete head;};
    };
    List* table{nullptr};  //table is pointer to a List that in the rehash method becomes a pointer to array of Lists (of Elements).
    size_type table_size{0};
    size_type current_size{0};
    size_type hash(const key_type& key) const {return hasher{}(key) % table_size;};
    float max_lf{0.7};

    void add(const key_type& key){
        if(locate(key))        // if key already in the list -> return.
            return;
        size_type idx{hash(key)};

        Element* new_Element{new Element};
        new_Element->key = key;
        new_Element->next = table[idx].head;          // new Element created with the same key and next pointing to the head of the list
        table[idx].head = new_Element;                // head of the list points to the newly added Element
        ++current_size;
    };

    Element *locate(const key_type& key) const{
        size_type idx{hash(key)};

        if (table[idx].head == nullptr) {
            return nullptr;
        }

        for(Element* elem = table[idx].head; elem != nullptr; elem= elem->next){
            if(key_equal{}(elem->key, key)){ return elem;}
        }
        return nullptr;
    };

    void reserve(size_type n){
        if(table_size * max_lf >= n) return;

        size_type new_table_size{table_size};

        while(new_table_size * max_lf < n) ++(new_table_size *=4);
        rehash(new_table_size);
    };

    void rehash(size_type n){
        size_type new_table_size{std::max(N,std::max(n,size_type(current_size/max_lf)))};
        List* new_table {new List [new_table_size]};
        List* old_table {table};
        size_type old_table_size{table_size};

        current_size = 0;
        table = new_table;
        table_size = new_table_size;

        for(size_type idx{0}; idx < old_table_size; ++idx ){
            Element* elem = old_table[idx].head;
            while(elem){
                add(elem->key);
                elem= elem->next;
            }
            delete elem;
        }

        delete[] old_table;
    };

public:
    ADS_set() { rehash(N); }

    ADS_set(std::initializer_list<key_type> ilist): ADS_set() { insert(ilist);}

    template<typename InputIt> ADS_set(InputIt first, InputIt last): ADS_set() { insert(first,last); }

    ADS_set(const ADS_set &other) {
        rehash(other.table_size);
        for(const auto &k: other)
            add(k);
    }

    ~ADS_set(){ delete[] table; }

    ADS_set &operator=(const ADS_set &other){
        ADS_set tmp{other};
        swap(tmp);
        return *this;
    }

    ADS_set &operator=(std::initializer_list<key_type> ilist){
        ADS_set tmp{ilist};
        swap(tmp);
        return *this;
    }

    size_type size() const { return current_size; }
    bool empty() const { return current_size==0; }

    void insert(std::initializer_list<key_type> ilist){
        for(const auto& l: ilist){
            add(l);
        }
    }

    std::pair<iterator,bool> insert(const key_type &key){
        if(auto e {locate(key)}) return {iterator{e, table, hash(key), table_size},false};

        reserve(current_size+1);
        add(key);
        return {iterator{locate(key),table,hash(key) ,table_size},true};
    }

   template<typename InputIt> void insert(InputIt first, InputIt last){
        for(auto it {first}; it != last; ++it){
            if(!locate(*it)) {
                reserve(current_size+1);
                add(*it);
            }
        }
    }

    void clear(){
        ADS_set temp;
        swap(temp);
    }

    size_type erase(const key_type &key){
        if(count(key)){
            if(key_equal{}(table[hash(key)].head->key, key)){
                Element* head = table[hash(key)].head;

                table[hash(key)].head = head->next;
                head->next = nullptr;
                delete head;

            }else{
                Element* help{nullptr};
                Element* head = table[hash(key)].head;
                while(head){
                    help = head;
                    head = head->next;
                    if(key_equal{}(head->key, key)){
                        help->next = head->next;
                        head->next=nullptr;
                        delete head;
                        help= nullptr;
                        delete help;
                        break;
                    }
                }
            }

            --current_size;
            if(current_size < table_size*0.2) rehash(current_size);
            return 1;
        }
        return 0;
    }

    size_type count(const key_type &key) const { return locate(key)!=nullptr; }

    iterator find(const key_type &key) const{
        if(locate(key)) return iterator {locate(key), table, hash(key),table_size};
        return end();
    }

    void swap(ADS_set &other){
        using std::swap;
        swap(table, other.table);
        swap(table_size, other.table_size);
        swap(current_size, other.current_size);
        swap(max_lf, other.max_lf);
    }

    const_iterator begin() const{
        for(size_type i = 0; i < table_size ; ++i ){
            if(table[i].head) {return iterator(table[i].head ,table ,i ,table_size);}
        }
        return end();
    }

    const_iterator end() const { return const_iterator(); }

    void dump(std::ostream &o = std::cerr) const{
        o<<"table size = "<< table_size<< "\n"<< "current size = "<< current_size<< "\n";
        for(size_type i{0}; i<table_size; ++i){
            o<< i << ":";
            if(table[i].head == nullptr) o<< "-";
            else{
                for(Element* elem = table[i].head; elem != nullptr; elem = elem->next){
                    if(elem == nullptr) break;
                    o<< elem->key;
                    if(elem->next)
                        o<< "->";
                }
            }
            o<<"\n";
        }
    }

    friend bool operator==(const ADS_set &lhs, const ADS_set &rhs) {
        if(lhs.current_size != rhs.current_size) return false;

        for(const auto &k: lhs)
            if(!rhs.count(k)) return false;
        return true;
    }
    friend bool operator!=(const ADS_set &lhs, const ADS_set &rhs) {return !(lhs == rhs);}
};

template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator {
    Element* current;
    List* table;
    size_type position;
    size_type table_size;
public:
    using value_type = Key;
    using difference_type = std::ptrdiff_t;
    using reference = const value_type &;
    using pointer = const value_type *;
    using iterator_category = std::forward_iterator_tag;

    explicit Iterator(Element* current = nullptr, List* table = nullptr, size_type position = 0, size_type size = N): current{current}, table{table}, position{position}, table_size{size} {}

    void skip(){
        while(position < table_size && table[position].head == nullptr) ++position;
    }

    reference operator*() const { return current->key;}
    pointer operator->() const { return &current->key;}

    Iterator &operator++(){
        if( current->next != nullptr) current = current->next;
        else{
            ++position;
            skip();
            if(position == table_size){
                current = nullptr;
                return *this;}
            current = table[position].head;

        }
        return *this;
    }

    Iterator operator++(int) {
        auto rc{*this};
        ++*this;
        return rc;
    }
    friend bool operator==(const Iterator &lhs, const Iterator &rhs) {return lhs.current == rhs.current;}
    friend bool operator!=(const Iterator &lhs, const Iterator &rhs) {return !(lhs==rhs);}
};


template <typename Key, size_t N>
void swap(ADS_set<Key,N> &lhs, ADS_set<Key,N> &rhs) { lhs.swap(rhs); }

#endif // ADS_SET_H

//#endif //ADS_SET_H_ADS_SET_H
