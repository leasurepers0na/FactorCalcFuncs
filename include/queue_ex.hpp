#pragma once
#include "stdafx.h"

#define renew_cnt 1e9

template<typename type_>
class queue_ex {
public:
	size_t  _usize;
	size_t	_uhead;	//out
	size_t	_utail;	//in,point empty
	type_ * value_q;
public:
	queue_ex(size_t size) {
		_usize = size + 1;
		_uhead = 0;
		_utail = 0;
		value_q = new type_[_usize];
	}
	queue_ex(const queue_ex & q) {
		_usize = q._usize;
		_uhead = q._uhead;
		_utail = q._utail;
		//if (value_q) {
		//	delete value_q;
		//	value_q = nullptr;
		//}
		value_q = new type_[_usize];
        memcpy(value_q, q.value_q, sizeof(type_)*_usize);
	}
	~queue_ex() {
		if (value_q) {
			delete []value_q;
			value_q = nullptr;
		}
	}
	void push(const type_ & val) {
		if (!value_q) {
			return;
		}
		if ((_utail + 1) % _usize == _uhead) { // full
			_uhead = (_uhead + 1) % _usize;
		}
		value_q[_utail] = val;
		_utail = (_utail + 1) % _usize;
	}

    bool push_front(const type_ & val) {
        if (!value_q) {
            return false;
        }
        if ((_utail + 1) % _usize == _uhead) { // full
            return false;
        }
        _uhead = (_uhead + _usize - 1) % _usize;
        value_q[_uhead] = val;
        return true;
    }

	bool pop() {
		if (!value_q || _uhead == _utail)
			return false;
		_uhead = (_uhead + 1) % _usize;
		return true;
	}

	bool front(type_ & val) {
		if (!value_q || _uhead == _utail)
			return false;
		val = value_q[_uhead];
		return true;
	}

	type_ front() {
		if (value_q && _uhead != _utail) {
			return value_q[_uhead];
		}
		return type_();
	}

	bool empty() const noexcept {
		return _uhead == _utail;
	}

	bool full() const noexcept {
		return  (_utail + 1) % _usize == _uhead;
	}

	size_t get_tail_index() {
		return _utail;
	}

	size_t get_back_index() {
		if (_uhead == _utail) {
			return _uhead;
		}
		return  (_utail + _usize - 1) % _usize;
	}

	size_t get_head_index() {
		return _uhead;
	}

	size_t size() noexcept
	{
		size_t uh = _uhead;
		size_t n = 0;
		while (uh != _utail)
		{
			n++;
			uh = (uh + 1) % _usize;
		}
		return n;
	}

	void get_max_value(type_ & max_value, size_t & max_index, std::function<int (const type_ & l_val, const type_  &r_val)> fun_comp) noexcept
	{
		if (!value_q || _uhead == _utail)
			return;
		//auto & vet = *value_q;
		size_t h = _uhead, t = _utail;
		size_t pre = h;
		max_value = value_q[pre];
		max_index = pre;
		while (h != t) {
			if (pre != h) {
				if (fun_comp(value_q[pre], value_q[h]) < 0) {
					max_value = value_q[h];
					max_index = h;
				}
			}
			
			h = (h + 1) % _usize;
		}
	}

	type_ * get_data_vector() {
		return value_q;
	}
};
