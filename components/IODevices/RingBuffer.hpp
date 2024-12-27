#ifndef __RING_BUFFER_HPP__
#define __RING_BUFFER_HPP__

#include <cstddef>
#include <vector>
#include "core/debug/debug.hpp"

// A templated circular ring buffer
template <typename T>
class CircularRingBuffer {
public:
	CircularRingBuffer() : head(0), tail(0), size(0), valid_count(0) {}

	explicit CircularRingBuffer(size_t size)
		: buffer(size), valid(size, false), head(0), tail(0), size(size), valid_count(0) {}

	// Resizes the buffer
	void resize(size_t new_size) {
		buffer.resize(new_size);
		valid.resize(new_size, false);
		head = 0;
		tail = 0;
		size = new_size;
		valid_count = 0;
	}

	size_t getSize () {
		return size;
	}

	void setHead (size_t head) {
		this->head = head;
	}

	void setTail (size_t tail) {
		this->tail = tail;
	}

	// Inserts an element at the position indicated by the tail
	void insertAtTail(const T item) {
		if (isFull()) {
			DBG_(Crit, (<< "Tail Insertion While Full!"));
		}

		buffer[tail] = item;
		valid[tail] = true;
		tail = (tail + 1) % size;
		++valid_count;
	}

	// Removes an element from the position indicated by the head
	T removeFromHead() {
		if (isEmpty()) {
			DBG_(Crit, (<< "Head Deletion While Empty!"));
		}

		T ret = buffer[head];
		valid[head] = false;
		head = (head + 1) % size;
		--valid_count;

		return ret;
	}

	// Returns the number of valid entries in the buffer
	size_t getValidCount() const {
		return valid_count;
	}

	// Checks if the buffer is empty
	bool isEmpty() const {
		return valid_count == 0;
	}

	// Checks if the buffer is full
	bool isFull() const {
		return valid_count == size;
	}

private:
	std::vector<T> buffer;  // The actual buffer
	std::vector<bool> valid; // Tracks the validity of each entry
	size_t head;            // Index of the head
	size_t tail;            // Index of the tail
	size_t size;            // Total size of the buffer
	size_t valid_count;     // Number of valid entries
};

#endif