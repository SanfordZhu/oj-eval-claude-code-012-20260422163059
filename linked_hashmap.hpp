/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
    /**
     * In linked_hashmap, iteration ordering is differ from map,
     * which is the order in which keys were inserted into the map.
     * You should maintain a doubly-linked list running through all
     * of its entries to keep the correct iteration order.
     *
     * Note that insertion order is not affected if a key is re-inserted
     * into the map.
     */

	template<
		class Key,
		class T,
		class Hash = std::hash<Key>,
		class Equal = std::equal_to<Key>
	> class linked_hashmap {
	public:
		/**
		 * the internal type of data.
		 * it should have a default constructor, a copy constructor.
		 * You can use sjtu::linked_hashmap as value_type by typedef.
		 */
		typedef pair<const Key, T> value_type;
	private:
		struct Node {
			value_type kv;
			Node *prev, *next;    // insertion-order list
			Node *hnext;          // bucket chain
			Node(const value_type &v) : kv(v), prev(nullptr), next(nullptr), hnext(nullptr) {}
		};
		Node **buckets = nullptr;
		size_t bucket_cnt = 0;
		size_t elem_cnt = 0;
		Node *head = nullptr;
		Node *tail = nullptr;
		Hash hasher;
		Equal equaler;
		static constexpr double load_factor_limit = 0.75;
		static constexpr size_t initial_bucket_cnt = 16;

		size_t bucket_index(const Key &key) const {
			return static_cast<size_t>(hasher(key)) % bucket_cnt;
		}
		void ensure_capacity() {
			if (bucket_cnt == 0) rehash(initial_bucket_cnt);
			if (elem_cnt + 1 > static_cast<size_t>(bucket_cnt * load_factor_limit)) {
				rehash(bucket_cnt * 2);
			}
		}
		void rehash(size_t new_cnt) {
			Node **new_buckets = new Node*[new_cnt];
			for (size_t i = 0; i < new_cnt; ++i) new_buckets[i] = nullptr;
			for (Node *p = head; p != nullptr; p = p->next) {
				p->hnext = nullptr;
				size_t idx = static_cast<size_t>(hasher(p->kv.first)) % new_cnt;
				p->hnext = new_buckets[idx];
				new_buckets[idx] = p;
			}
			delete [] buckets;
			buckets = new_buckets;
			bucket_cnt = new_cnt;
		}
		Node* find_node(const Key &key) const {
			if (bucket_cnt == 0) return nullptr;
			size_t idx = bucket_index(key);
			for (Node *p = buckets[idx]; p != nullptr; p = p->hnext) {
				if (equaler(p->kv.first, key)) return p;
			}
			return nullptr;
		}
		void unlink_from_list(Node *p) {
			if (p->prev) p->prev->next = p->next; else head = p->next;
			if (p->next) p->next->prev = p->prev; else tail = p->prev;
		}
		void unlink_from_bucket(Node *p) {
			size_t idx = bucket_index(p->kv.first);
			Node *cur = buckets[idx], *prev = nullptr;
			while (cur) {
				if (cur == p) {
					if (prev) prev->hnext = cur->hnext; else buckets[idx] = cur->hnext;
					return;
				}
				prev = cur; cur = cur->hnext;
			}
		}
	public:
		class const_iterator;
		class iterator {
		private:
			linked_hashmap *mp = nullptr;
			Node *node = nullptr;
			friend class const_iterator;
			friend class linked_hashmap;
		public:
			using difference_type = std::ptrdiff_t;
			using value_type = typename linked_hashmap::value_type;
			using pointer = value_type*;
			using reference = value_type&;
			using iterator_category = std::bidirectional_iterator_tag;

			iterator() = default;
			iterator(linked_hashmap *m, Node *n) : mp(m), node(n) {}
			iterator(const iterator &other) = default;
			iterator operator++(int) { iterator tmp(*this); ++(*this); return tmp; }
			iterator & operator++() {
				if (node == nullptr) throw invalid_iterator();
				node = node->next;
				return *this;
			}
			iterator operator--(int) { iterator tmp(*this); --(*this); return tmp; }
			iterator & operator--() {
				if (mp == nullptr) throw invalid_iterator();
				if (node == nullptr) { if (mp->tail == nullptr) throw invalid_iterator(); node = mp->tail; return *this; }
				if (node->prev == nullptr) throw invalid_iterator();
				node = node->prev; return *this;
			}
			value_type & operator*() const { if (node == nullptr) throw invalid_iterator(); return node->kv; }
			bool operator==(const iterator &rhs) const { return mp == rhs.mp && node == rhs.node; }
			bool operator==(const const_iterator &rhs) const { return mp == rhs.mp && node == rhs.node; }
			bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
			bool operator!=(const const_iterator &rhs) const { return !(mp == rhs.mp && node == rhs.node); }
			value_type* operator->() const noexcept { return &node->kv; }
		};

		class const_iterator {
		private:
			const linked_hashmap *mp = nullptr;
			const Node *node = nullptr;
			friend class iterator;
			friend class linked_hashmap;
		public:
			using difference_type = std::ptrdiff_t;
			using value_type = typename linked_hashmap::value_type;
			using pointer = const value_type*;
			using reference = const value_type&;
			using iterator_category = std::bidirectional_iterator_tag;
			const_iterator() = default;
			const_iterator(const linked_hashmap *m, const Node *n) : mp(m), node(n) {}
			const_iterator(const const_iterator &other) = default;
			const_iterator(const iterator &other) : mp(other.mp), node(other.node) {}
			const_iterator operator++(int) { const_iterator tmp(*this); ++(*this); return tmp; }
			const_iterator & operator++() { if (node == nullptr) throw invalid_iterator(); node = node->next; return *this; }
			const_iterator operator--(int) { const_iterator tmp(*this); --(*this); return tmp; }
			const_iterator & operator--() {
				if (mp == nullptr) throw invalid_iterator();
				if (node == nullptr) { if (mp->tail == nullptr) throw invalid_iterator(); node = mp->tail; return *this; }
				if (node->prev == nullptr) throw invalid_iterator();
				node = node->prev; return *this;
			}
			reference operator*() const { if (node == nullptr) throw invalid_iterator(); return node->kv; }
			bool operator==(const const_iterator &rhs) const { return mp == rhs.mp && node == rhs.node; }
			bool operator==(const iterator &rhs) const { return rhs == *this; }
			bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
			bool operator!=(const iterator &rhs) const { return !(rhs == *this); }
			pointer operator->() const noexcept { return &node->kv; }
		};

		linked_hashmap() { rehash(initial_bucket_cnt); }
		linked_hashmap(const linked_hashmap &other) : hasher(other.hasher), equaler(other.equaler) {
			rehash(other.bucket_cnt ? other.bucket_cnt : initial_bucket_cnt);
			for (Node *p = other.head; p != nullptr; p = p->next) insert(p->kv);
		}
		linked_hashmap & operator=(const linked_hashmap &other) {
			if (this == &other) return *this;
			clear();
			hasher = other.hasher; equaler = other.equaler;
			rehash(other.bucket_cnt ? other.bucket_cnt : initial_bucket_cnt);
			for (Node *p = other.head; p != nullptr; p = p->next) insert(p->kv);
			return *this;
		}
		~linked_hashmap() { clear(); delete [] buckets; buckets = nullptr; bucket_cnt = 0; }

		T & at(const Key &key) {
			Node *p = find_node(key);
			if (!p) throw index_out_of_bound();
			return p->kv.second;
		}
		const T & at(const Key &key) const {
			Node *p = find_node(key);
			if (!p) throw index_out_of_bound();
			return p->kv.second;
		}

		T & operator[](const Key &key) {
			Node *p = find_node(key);
			if (p) return p->kv.second;
			ensure_capacity();
			value_type v(key, T());
			Node *n = new Node(v);
			if (tail) { tail->next = n; n->prev = tail; tail = n; }
			else { head = tail = n; }
			size_t idx = bucket_index(key);
			n->hnext = buckets[idx]; buckets[idx] = n;
			++elem_cnt;
			return n->kv.second;
		}

		const T & operator[](const Key &key) const {
			Node *p = find_node(key);
			if (!p) throw index_out_of_bound();
			return p->kv.second;
		}

		iterator begin() { return iterator(this, head); }
		const_iterator cbegin() const { return const_iterator(this, head); }
		iterator end() { return iterator(this, nullptr); }
		const_iterator cend() const { return const_iterator(this, nullptr); }

		bool empty() const { return elem_cnt == 0; }
		size_t size() const { return elem_cnt; }

		void clear() {
			Node *p = head; while (p) { Node *n = p->next; delete p; p = n; }
			head = tail = nullptr; elem_cnt = 0;
			if (buckets) { for (size_t i = 0; i < bucket_cnt; ++i) buckets[i] = nullptr; }
		}

		pair<iterator, bool> insert(const value_type &value) {
			Node *ex = find_node(value.first);
			if (ex) return pair<iterator,bool>(iterator(this, ex), false);
			ensure_capacity();
			Node *n = new Node(value);
			if (tail) { tail->next = n; n->prev = tail; tail = n; }
			else { head = tail = n; }
			size_t idx = bucket_index(value.first);
			n->hnext = buckets[idx]; buckets[idx] = n;
			++elem_cnt;
			return pair<iterator,bool>(iterator(this, n), true);
		}

		void erase(iterator pos) {
			if (pos.mp != this || pos.node == nullptr) throw invalid_iterator();
			Node *p = pos.node;
			unlink_from_list(p);
			unlink_from_bucket(p);
			delete p; --elem_cnt;
		}

		size_t count(const Key &key) const { return find_node(key) ? 1 : 0; }

		iterator find(const Key &key) { return iterator(this, find_node(key)); }
		const_iterator find(const Key &key) const { return const_iterator(this, find_node(key)); }
	};

}

#endif
