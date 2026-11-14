// Factor_Five_Dll.cpp: 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "Factor_Dll.h"

#define FDLL_VERSION "1.5.5"
//// v1.4.3
//// start to now , std, corr 修改为input少于5个值就赋值nan
//// v1.5.0
//// add sn_shift, sn_sorted, half1_shift, half2_shift
//// v1.5.1
//// add sn_quantile, sn_median
//// v1.5.2
//// 处理NK_MAX和NK_MIN等公共函数在传空值时候异常问题，改成返回NAN
//// v1.5.3
//// 增加GetXYValue函数
//// v1.5.4
//// 增加老的windows因子调用MAX MIN公共函数
//// v1.5.5
//// 修改half1_rank,half2_rank内存泄漏BUG
struct ContractInfoStruDLL {
	int date;
	double price_tick;//最小价格
	double multiply;//合约乘数
	ContractInfoStruDLL *pre;
	ContractInfoStruDLL *next;
	ContractInfoStruDLL() {
		memset(this, 0, sizeof(ContractInfoStruDLL));
	}
};

RollingHelper::RollingHelper()
{
}


RollingHelper::~RollingHelper()
{
}

double RollingHelper::nan_to_zero(double value) {
	if (::isnan(value)) {
		return 0;
	}
	else {
		return value;
	}
}

double RollingHelper::inf_nan_to_zero(double value) {
	if (::isnan(value) || ::isinf(value)) {
		return 0;
	}
	else {
		return value;
	}
}

bool RollingHelper::nan_greater_equal(const double & a, const double & b) {
	// nan >= nan true
	// 3 >= nan true
	// nan >= 3 false
	if (::isnan(b)) {
		return true;
	}
	else if (::isnan(a)) {
		return false;
	}
	else {
		return a - b >= -1e-8;
	}
}

bool RollingHelper::nan_greater(const double & a, const double & b) {
	// nan > nan false
	// 3 > nan true
	// nan > 3 false
	if (::isnan(a)) {
		return false;
	}
	else if (::isnan(b)) {
		return true;
	}
	else {
		return a - b > 1e-8;
	}
}

bool RollingHelper::nan_less_equal(const double & a, const double & b) {
	// nan <= nan true
	// 3 <= nan true
	// nan <= 3 false
	if (::isnan(b)) {
		return true;
	}
	else if (::isnan(a)) {
		return false;
	}
	else {
		return a - b <= 1e-8;
	}
}

bool RollingHelper::nan_less(const double & a, const double & b) {
	// nan < nan false
	// 3 < nan true
	// nan < 3 false
	if (::isnan(a)) {
		return false;
	}
	else if (::isnan(b)) {
		return true;
	}
	else {
		return a - b < -1e-8;
	}
}


double RollingHelper::round_double(double value) {
	return floor(value * (double)1e12 + 0.5) / (double)1e12;
}
ShiftHelper::ShiftHelper(unsigned int window) {
	window_ = window;
}

ShiftHelper::~ShiftHelper() {
	for (auto itor : label_q_map_) {
		if (itor.second) {
			delete itor.second;
			itor.second = nullptr;
		}
	}
	label_q_map_.clear();
}

double ShiftHelper::cal_shift(double value, int label) {
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end()) {
		//label_q_map_[label] = 
		queue_ex<double> * label_q = new queue_ex<double>(window_);
		label_q->push(value);
		label_q_map_[label] = label_q;
	}
	else {
		queue_ex<double> *label_q = label_q_iter->second;
		if (!label_q->full()) {
			label_q->push(value);
			//label_q_map_[label] = label_q;
		}
		else {
			double tmp_pop_value = label_q->front();
			label_q->pop();
			label_q->push(value);
			//label_q_map_[label] = label_q;
			return tmp_pop_value;
		}
	}
	return NAN;
}

RollingFuncHelper::RollingFuncHelper(unsigned int window) {
	window_ = window;
}

RollingFuncHelper::~RollingFuncHelper() {
}

list<double>* RollingFuncHelper::get_rolling_list(double value, int label) {
	auto label_l_iter = label_l_map_.find(label);
	if (label_l_iter == label_l_map_.end()) {
		list<double> & l = label_l_map_[label];
		l.push_back(value);
		return &l;
	}
	else {
		list<double> *label_l = &label_l_iter->second;
		if (label_l->size() < window_) {
			label_l->push_back(value);
			//label_l_map_[label] = label_l;
		}
		else {
			label_l->erase(label_l->begin());
			label_l->push_back(value);
			//label_l_map_[label] = label_l;
		}
		return label_l;
	}
}

RollingFuncHelper2::RollingFuncHelper2(unsigned int window) {
	window_ = window;
}

RollingFuncHelper2::~RollingFuncHelper2() {
	for (auto & itor: label_l_map_) {
		delete itor.second;
	}
	label_l_map_.clear();
}

void RollingFuncHelper2::get_rolling_max_min(double value, int label, double &max_val, double &min_val) {
	auto label_l_iter = label_l_map_.find(label);
	if (label_l_iter == label_l_map_.end()) {
		auto * p_q = new queue_ex<double>(window_);
		p_q->push(value);
		max_map_[label] = value;
		min_map_[label] = value;
		max_val = value;
		min_val = value;
		label_l_map_[label] = p_q;
	}
	else {
		double &min_ = min_map_[label];
		double &max_ = max_map_[label];
		auto * label_l = label_l_iter->second;
		if (!label_l->full()) {
			label_l->push(value);
			//label_l_map_[label] = label_l;
			if (value < min_) {
				min_ = value;
			}
			if (value > max_) {
				max_ = value;
			}
		}
		else {
			double front_val = label_l->front();
			label_l->pop();
			//label_l_map_[label] = label_l;
			if (value < min_) {
				min_ = value;
				if (abs(front_val - max_) < 0.0001) {
					max_ = value;
					size_t head = label_l->_uhead;
					size_t tail = label_l->_utail;
					size_t usize = label_l->_usize;
					double * vet = label_l->value_q;
					while (head != tail) {
						if (vet[head] > max_) {
							max_ = vet[head];
						}
						head = (head + 1) % usize;
					}
				}
			}
			else if (value > max_) {
				max_ = value;
				if (abs(front_val - min_) < 0.0001) {
					min_ = value;
					size_t head = label_l->_uhead;
					size_t tail = label_l->_utail;
					size_t usize = label_l->_usize;
					double * vet = label_l->value_q;
					while (head != tail) {
						if (vet[head] < min_) {
							min_ = vet[head];
						}
						head = (head + 1) % usize;
					}
				}
			}
			else {
				if (abs(front_val - max_) < 0.0001) {
					max_ = value;
					size_t head = label_l->_uhead;
					size_t tail = label_l->_utail;
					size_t usize = label_l->_usize;
					double * vet = label_l->value_q;
					while (head != tail) {
						if (vet[head] > max_) {
							max_ = vet[head];
						}
						head = (head + 1) % usize;
					}
				}
				if (abs(front_val - min_) < 0.0001) {
					min_ = value;
					size_t head = label_l->_uhead;
					size_t tail = label_l->_utail;
					size_t usize = label_l->_usize;
					double * vet = label_l->value_q;
					while (head != tail) {
						if (vet[head] < min_) {
							min_ = vet[head];
						}
						head = (head + 1) % usize;
					}
				}
			}
			label_l->push(value);
		}
		max_val = max_;
		min_val = min_;
	}
}

queue_ex<double>* RollingFuncHelper2::get_rolling_list(double value, int label) {
	auto label_l_iter = label_l_map_.find(label);
	if (label_l_iter == label_l_map_.end()) {
		auto * p_q = new queue_ex<double>(window_);
		p_q->push(value);
		label_l_map_[label] = p_q;
		return p_q;
	}
	else {
		auto * label_l = label_l_iter->second;
		if (!label_l->full()) {
			label_l->push(value);
			//label_l_map_[label] = label_l;
		}
		else {
			label_l->pop();
			label_l->push(value);
			//label_l_map_[label] = label_l;
		}
		return label_l;
	}
}

RollingSumData::RollingSumData() {
	sum = 0;
	nan_num = 0;
	value_q = nullptr;
}

RollingSumData::~RollingSumData() {
	if (value_q) {
		delete value_q;
		value_q = nullptr;
	}
}

RollingSumData::RollingSumData(const RollingSumData & rsd) {
	this->nan_num = rsd.nan_num;
	this->sum = rsd.sum;
	if (value_q) {
		delete value_q;
		value_q = nullptr;
	}

	if (rsd.value_q) {
		value_q = new queue_ex<double>(*rsd.value_q);
	}
}

RollingSumData & RollingSumData::operator=(const RollingSumData & rsd) {
	this->nan_num = rsd.nan_num;
	this->sum = rsd.sum;
	if (value_q) {
		delete value_q;
		value_q = nullptr;
	}
	if (rsd.value_q) {
		value_q = new queue_ex<double>(*rsd.value_q);
	}
	return *this;
}

RollingSumHelper::RollingSumHelper(unsigned int window, bool nan_sum)
{
	window_ = window;
	nan_sum_ = nan_sum;
}

RollingSumHelper::RollingSumHelper(unsigned int window)
{
	window_ = window;
	nan_sum_ = false;
}

RollingSumHelper::~RollingSumHelper()
{
	
}

double RollingSumHelper::cal_rolling_sum(double value, int label)
{
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end()) {
		RollingSumData & t_rsd = label_q_map_[label];
		t_rsd.value_q = new queue_ex<double>(window_);
		t_rsd.value_q->push(value);
		t_rsd.nan_num += ::isnan(value) ? 1 : 0;
		t_rsd.sum += nan_to_zero(value);
	}
	else {
		RollingSumData & t_rsd = label_q_iter->second;
		if (!t_rsd.value_q->full()) {
			t_rsd.value_q->push(value);
			t_rsd.nan_num += ::isnan(value) ? 1 : 0;
			t_rsd.sum += nan_to_zero(value);
			if (t_rsd.value_q->full()) {
				if (nan_sum_ == true) {
					return round_double(t_rsd.sum);
				}
				else {
					return t_rsd.nan_num == 0 ? round_double(t_rsd.sum) : NAN;
				}
			}
		}
		else {
			double tmp_pop_value = t_rsd.value_q->front();
			t_rsd.value_q->pop();
			t_rsd.value_q->push(value);
			t_rsd.nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rsd.nan_num += ::isnan(value) ? 1 : 0;
			t_rsd.sum = t_rsd.sum - nan_to_zero(tmp_pop_value) + nan_to_zero(value);
			//label_q_map_[label] = t_rsd;
			if (nan_sum_ == true) {
				return round_double(t_rsd.sum);
			}
			else {
				return t_rsd.nan_num == 0 ? round_double(t_rsd.sum) : NAN;
			}
		}
	}
	return NAN;
}

RollingStdData::RollingStdData()
{
	square_sum = 0;
	sum = 0;
	nan_num = 0;
	value_q = nullptr;
}

RollingStdData::~RollingStdData()
{
	if (value_q) {
		delete value_q;
		value_q = nullptr;
	}
}

RollingStdData & RollingStdData::operator=(const RollingStdData & rsd) {
	square_sum = rsd.square_sum;
	sum = rsd.sum;
	nan_num = rsd.nan_num;
	if (value_q) {
		delete value_q;
		value_q = nullptr;
	}
	if (rsd.value_q) {
		value_q = new queue_ex<double>(*rsd.value_q);
	}
	return *this;
}

RollingStdHelper::RollingStdHelper(unsigned int window, bool nan_std, bool unbiased)
{
	window_ = window;
	nan_std_ = nan_std;
	unbiased_ = unbiased;
}

RollingStdHelper::RollingStdHelper(unsigned int window, bool nan_std)
{
	window_ = window;
	nan_std_ = nan_std;
	unbiased_ = true;
}

RollingStdHelper::RollingStdHelper(unsigned int window)
{
	window_ = window;
	nan_std_ = false;
	unbiased_ = true;
}

RollingStdHelper::~RollingStdHelper()
{
}

double RollingStdHelper::cal_rolling_std(double value, int label)
{
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end()) {
		RollingStdData & t_rsd = label_q_map_[label];
		queue_ex<double> * t_value_q = new queue_ex<double>(window_);
		t_value_q->push(value);
		t_rsd.value_q = t_value_q;
		t_rsd.nan_num += ::isnan(value) ? 1 : 0;
		t_rsd.sum += nan_to_zero(value);
		t_rsd.square_sum = t_rsd.square_sum + nan_to_zero(value) * nan_to_zero(value);
	}
	else {
		RollingStdData *t_rsd = &label_q_iter->second;
		if (!t_rsd->value_q->full()) {
			t_rsd->value_q->push(value);
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum += nan_to_zero(value);
			t_rsd->square_sum = t_rsd->square_sum + nan_to_zero(value) * nan_to_zero(value);
			//label_q_map_[label] = t_rsd;
			if (t_rsd->value_q->full() ) {
                if (window_ == t_rsd->nan_num) {
                    return NAN;
                }
				double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num);
                if (variance < 0) {
                    variance = 0;
                }
				if (nan_std_ == true) {
					if (unbiased_ == true) {
                        if (window_ == t_rsd->nan_num + 1) {
                            return NAN;
                        }
						return sqrt(round_double(variance / (window_ - t_rsd->nan_num - 1)));
					}
					else {
                        if (window_ == t_rsd->nan_num) {
                            return NAN;
                        }
						return sqrt(round_double(variance / (window_ - t_rsd->nan_num)));
					}
				}
				else {
					if (t_rsd->nan_num == 0) {
						if (unbiased_ == true) {
                            if (window_ == 1) {
                                return NAN;
                            }
							return sqrt(round_double(variance / (window_ - 1)));
						}
						else {
							return sqrt(round_double(variance / window_));
						}
					}
					else {
						return NAN;
					}
				}
			}
		}
		else {
			double tmp_pop_value = t_rsd->value_q->front();
			t_rsd->value_q->pop();
			t_rsd->value_q->push(value);
			t_rsd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum = t_rsd->sum - nan_to_zero(tmp_pop_value) + nan_to_zero(value);
			t_rsd->square_sum = t_rsd->square_sum - nan_to_zero(tmp_pop_value) * nan_to_zero(tmp_pop_value) + nan_to_zero(value) * nan_to_zero(value);
			//label_q_map_[label] = t_rsd;
            if (window_ == t_rsd->nan_num) {
                return NAN;
            }
			double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num);
            if (variance < 0) {
                variance = 0;
            }
			if (nan_std_ == true) {
				if (unbiased_ == true) {
                    if (window_ == t_rsd->nan_num + 1) {
                        return NAN;
                    }
					return sqrt(round_double(variance / (window_ - t_rsd->nan_num - 1)));
				}
				else {
                    if (window_ == t_rsd->nan_num) {
                        return NAN;
                    }
					return sqrt(round_double(variance / (window_ - t_rsd->nan_num)));
				}
			}
			else {
				if (t_rsd->nan_num == 0) {
					if (unbiased_ == true) {
                        if (window_ == 1) {
                            return NAN;
                        }
						return sqrt(round_double(variance / (window_ - 1)));
					}
					else {
						return sqrt(round_double(variance / window_));
					}
				}
				else {
					return NAN;
				}
			}
		}
	}
	return NAN;
}

RollingCorrData::RollingCorrData()
{
	square_sum1 = 0;
	square_sum2 = 0;
	sum1 = 0;
	sum2 = 0;
	sum12 = 0;
	nan_num = 0;
	value_q1 = nullptr;
	value_q2 = nullptr;
}

RollingCorrData::~RollingCorrData() {
	if (value_q1) {
		delete value_q1;
		value_q1 = nullptr;
	}

	if (value_q2) {
		delete value_q2;
		value_q2 = nullptr;
	}
}
RollingCorrData & RollingCorrData::operator=(const RollingCorrData & rcd) {
	square_sum1 = rcd.square_sum1;
	square_sum2 = rcd.square_sum2;
	sum1 = rcd.sum1;
	sum2 = rcd.sum2;
	sum12 = rcd.sum12;
	nan_num = rcd.nan_num;
	if (value_q1) {
		delete value_q1;
		value_q1 = nullptr;
	}

	if (value_q2) {
		delete value_q2;
		value_q2 = nullptr;
	}
	if(rcd.value_q1)
		value_q1 = new queue_ex<double>(*rcd.value_q1);
	if(rcd.value_q2)
		value_q2 = new queue_ex<double>(*rcd.value_q2);
	return *this;
}

RollingCorrHelper::RollingCorrHelper(unsigned int window, bool nan_corr)
{
	window_ = window;
	nan_corr_ = nan_corr;
}

RollingCorrHelper::RollingCorrHelper(unsigned int window)
{
	window_ = window;
	nan_corr_ = false;
}


RollingCorrHelper::~RollingCorrHelper()
{
}

double RollingCorrHelper::cal_rolling_corr(double value1, double value2, int label)
{
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end()) {
		RollingCorrData t_rcd;
		queue_ex<double> *t_value_q1 = new queue_ex<double>(window_);
		queue_ex<double> *t_value_q2 = new queue_ex<double>(window_);
		t_value_q1->push(value1);
		t_value_q2->push(value2);
		t_rcd.value_q1 = t_value_q1;
		t_rcd.value_q2 = t_value_q2;
		bool tmp_nan_flag = (::isnan(value1) || ::isnan(value2));
		t_rcd.nan_num += tmp_nan_flag ? 1 : 0;
		if (tmp_nan_flag == false) {
			t_rcd.sum1 += value1;
			t_rcd.sum2 += value2;
			t_rcd.sum12 += value1 * value2;
			t_rcd.square_sum1 += value1 * value1;
			t_rcd.square_sum2 += value2 * value2;
		}
		label_q_map_[label] = t_rcd;
	}
	else {
		RollingCorrData *t_rcd = &(label_q_iter->second);
		if (!t_rcd->value_q1->full()) {
			t_rcd->value_q1->push(value1);
			t_rcd->value_q2->push(value2);
			bool tmp_nan_flag = (::isnan(value1) || ::isnan(value2));
			t_rcd->nan_num += tmp_nan_flag ? 1 : 0;
			if (tmp_nan_flag == false) {
				t_rcd->sum1 += value1;
				t_rcd->sum2 += value2;
				t_rcd->sum12 += value1 * value2;
				t_rcd->square_sum1 += value1 * value1;
				t_rcd->square_sum2 += value2 * value2;
			}
			if (t_rcd->value_q1->full()) {
				unsigned int value_num = window_ - t_rcd->nan_num;
				if (value_num == 0) {
					return NAN;
				}
				else {
					if ((nan_corr_ == true) || (t_rcd->nan_num == 0)) {
						double variance1 = round_double((t_rcd->square_sum1 - t_rcd->sum1 * t_rcd->sum1 / value_num) / value_num);
						double variance2 = round_double((t_rcd->square_sum2 - t_rcd->sum2 * t_rcd->sum2 / value_num) / value_num);
						if ((variance1 < 1e-12) || (variance2 < 1e-12)) {
							return NAN;
						}
						else {
							double std1 = sqrt(variance1);
							double std2 = sqrt(variance2);
							double mean1 = t_rcd->sum1 / value_num;
							double mean2 = t_rcd->sum2 / value_num;
							double mean12 = t_rcd->sum12 / value_num;
							double result = round_double((mean12 - mean1 * mean2) / round_double((std1 * std2)));
							if (result >= 1.0001 || result <= -1.0001) {
								return NAN;
							}
							return result;
						}
					}
					else {
						return NAN;
					}
				}
			}
		}
		else {
			double tmp_pop_value1 = t_rcd->value_q1->front();
			double tmp_pop_value2 = t_rcd->value_q2->front();
			t_rcd->value_q1->pop();
			t_rcd->value_q2->pop();
			t_rcd->value_q1->push(value1);
			t_rcd->value_q2->push(value2);
			bool tmp_pop_nan_flag = (::isnan(tmp_pop_value1) || ::isnan(tmp_pop_value2));
			bool tmp_nan_flag = (::isnan(value1) || ::isnan(value2));
			t_rcd->nan_num -= tmp_pop_nan_flag ? 1 : 0;
			t_rcd->nan_num += tmp_nan_flag ? 1 : 0;
			if (tmp_pop_nan_flag == false) {
				t_rcd->sum1 -= tmp_pop_value1;
				t_rcd->sum2 -= tmp_pop_value2;
				t_rcd->sum12 -= tmp_pop_value1 * tmp_pop_value2;
				t_rcd->square_sum1 -= tmp_pop_value1 * tmp_pop_value1;
				t_rcd->square_sum2 -= tmp_pop_value2 * tmp_pop_value2;
			}
			if (tmp_nan_flag == false) {
				t_rcd->sum1 += value1;
				t_rcd->sum2 += value2;
				t_rcd->sum12 += value1 * value2;
				t_rcd->square_sum1 += value1 * value1;
				t_rcd->square_sum2 += value2 * value2;
			}
			unsigned int value_num = window_ - t_rcd->nan_num;
			if (value_num == 0) {
				return NAN;
			}
			else {
				if ((nan_corr_ == true) || (t_rcd->nan_num == 0)) {
					double variance1 = round_double((t_rcd->square_sum1 - t_rcd->sum1 * t_rcd->sum1 / value_num) / value_num);
					double variance2 = round_double((t_rcd->square_sum2 - t_rcd->sum2 * t_rcd->sum2 / value_num) / value_num);
					if ((variance1 < 1e-12) || (variance2 < 1e-12)) {
						return NAN;
					}
					else {
						double std1 = sqrt(variance1);
						double std2 = sqrt(variance2);
						double mean1 = t_rcd->sum1 / value_num;
						double mean2 = t_rcd->sum2 / value_num;
						double mean12 = t_rcd->sum12 / value_num;
						double result = round_double((mean12 - mean1 * mean2) / round_double((std1 * std2)));
						if (result >= 1.0001 || result <= -1.0001) {
							return NAN;
						}
						return result;
					}
				}
				else {
					return NAN;
				}
			}
		}
	}
	return NAN;
}

RollingMaxHelper::RollingMaxHelper(unsigned int window, bool nan_max)
{
	window_ = window;
	nan_max_ = nan_max;
}

RollingMaxHelper::RollingMaxHelper(unsigned int window)
{
	window_ = window;
	nan_max_ = false;
}

RollingMaxHelper::~RollingMaxHelper()
{
}

double RollingMaxHelper::cal_rolling_max(double value, int label) {
	auto label_l_iter = label_l_map_.find(label);
	if (label_l_iter == label_l_map_.end()) {
		RollingMaxData & t_rmd = label_l_map_[label];
		queue_ex<double> * t_value_l = new queue_ex<double>(window_);
		t_value_l->push(value);
		t_rmd.value_l = t_value_l;
		t_rmd.max_value_index = window_;
		t_rmd.max_value = value;
		t_rmd.nan_num += ::isnan(value) ? 1 : 0;
	}
	else {
		RollingMaxData *t_rmd = &(label_l_iter->second);
		if (!t_rmd->value_l->full()) {
			t_rmd->value_l->push(value);
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			if (nan_greater_equal(value, t_rmd->max_value)) {
				t_rmd->max_value = value;
				t_rmd->max_value_index = window_;
			}
			else {
				t_rmd->max_value_index -= 1;
			}
			if (t_rmd->value_l->full()) {
				if ((nan_max_ == true) || (t_rmd->nan_num == 0)) {
					return t_rmd->max_value;
				}
				else {
					return NAN;
				}
			}
		}
		else {
			double tmp_pop_value = t_rmd->value_l->front();
			t_rmd->value_l->pop();
			t_rmd->value_l->push(value);
			t_rmd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			if (nan_greater_equal(value, t_rmd->max_value)) {
				t_rmd->max_value = value;
				t_rmd->max_value_index = window_;
			}
			else {
				t_rmd->max_value_index -= 1;
				if (t_rmd->max_value_index == 0) {
					double tmp_max_value;
					unsigned int tmp_max_index;
					unsigned int tmp_iter_index = 1;				
					size_t uh = t_rmd->value_l->get_head_index();
					size_t tail = t_rmd->value_l->get_tail_index();
					double * vet = t_rmd->value_l->get_data_vector();
					size_t _usize = window_ + 1;
					size_t pre_uh = uh;
					while (uh != tail)
					{
						auto & val = vet[uh];
						if (tmp_iter_index == 1) {
							tmp_max_value = val;
							tmp_max_index = tmp_iter_index;
						}
						else {
							if (nan_greater_equal(val, tmp_max_value)) {
								tmp_max_value = val;
								tmp_max_index = tmp_iter_index;
							}
						}
						pre_uh = uh;
						++tmp_iter_index;
						uh = (uh + 1) % _usize;
					}

					t_rmd->max_value = tmp_max_value;
					t_rmd->max_value_index = tmp_max_index;
				}
			}
			if ((nan_max_ == true) || (t_rmd->nan_num == 0)) {
				return t_rmd->max_value;
			}
			else {
				return NAN;
			}
		}
	}
	return NAN;
}

RollingMaxData::RollingMaxData() {
	max_value = NAN;
	max_value_index = 0;
	nan_num = 0;
	value_l = nullptr;
}

RollingMaxData::~RollingMaxData() {
	if (value_l) {
		delete value_l;
		value_l = nullptr;
	}
}

RollingMaxData & RollingMaxData::operator=(const RollingMaxData & rmd) {
	max_value = rmd.max_value;
	max_value_index = rmd.max_value_index;
	nan_num = rmd.nan_num;
	if (value_l) {
		delete value_l;
		value_l = nullptr;
	}

	if (rmd.value_l) {
		value_l = new queue_ex<double>(*rmd.value_l);
	}
	return *this;
}


RollingArgmaxHelper::RollingArgmaxHelper(unsigned int window, bool nan_argmax, bool first_argmax)
{
	window_ = window;
	nan_argmax_ = nan_argmax;
	first_argmax_ = first_argmax;

}

RollingArgmaxHelper::RollingArgmaxHelper(unsigned int window, bool nan_argmax)
{
	window_ = window;
	nan_argmax_ = nan_argmax;
	first_argmax_ = true;
}

RollingArgmaxHelper::RollingArgmaxHelper(unsigned int window)
{
	window_ = window;
	nan_argmax_ = false;
	first_argmax_ = true;
}

RollingArgmaxHelper::~RollingArgmaxHelper()
{
}

double RollingArgmaxHelper::cal_rolling_argmax(double value, int label) {
	auto label_l_iter = label_l_map_.find(label);
	if (label_l_iter == label_l_map_.end()) {
		RollingMaxData & t_rmd = label_l_map_[label];
		t_rmd.value_l = new queue_ex<double>(window_);
		t_rmd.value_l->push(value);
		t_rmd.max_value_index = window_;
		t_rmd.max_value = value;
		t_rmd.nan_num += ::isnan(value) ? 1 : 0;
	}
	else {
		RollingMaxData *t_rmd = &(label_l_iter->second);
		if (!t_rmd->value_l->full()) {
			t_rmd->value_l->push(value);
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			bool greater_flag = first_argmax_ ? nan_greater(value, t_rmd->max_value) : nan_greater_equal(value, t_rmd->max_value);
			if (greater_flag) {
				t_rmd->max_value = value;
				t_rmd->max_value_index = window_;
			}
			else {
				t_rmd->max_value_index -= 1;
			}
			if (t_rmd->value_l->full()) {
				if ((nan_argmax_ == true) || (t_rmd->nan_num == 0)) {
					return t_rmd->max_value_index - 1;
				}
				else {
					return NAN;
				}
			}
		}
		else {
			double tmp_pop_value = t_rmd->value_l->front();
			t_rmd->value_l->pop();
			t_rmd->value_l->push(value);
			t_rmd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			bool greater_flag = first_argmax_ ? nan_greater(value, t_rmd->max_value) : nan_greater_equal(value, t_rmd->max_value);
			if (greater_flag) {
				t_rmd->max_value = value;
				t_rmd->max_value_index = window_;
			}
			else {
				t_rmd->max_value_index -= 1;
				if (t_rmd->max_value_index == 0) {
					double tmp_max_value;
					unsigned int tmp_max_index;
					unsigned int tmp_iter_index = 1;
					size_t uh = t_rmd->value_l->get_head_index();
					size_t tail = t_rmd->value_l->get_tail_index();
					double * vet = t_rmd->value_l->get_data_vector();
					size_t _usize = window_ + 1;
					size_t pre_uh = uh;
					while (uh != tail)
					{
						auto & val = vet[uh];
						if (tmp_iter_index == 1) {
							tmp_max_value = val;
							tmp_max_index = tmp_iter_index;
						}
						else {
							bool greater_flag = first_argmax_ ? nan_greater(val, tmp_max_value) : nan_greater_equal(val, tmp_max_value);
							if (greater_flag) {
								tmp_max_value = val;
								tmp_max_index = tmp_iter_index;
							}
						}
						pre_uh = uh;
						++tmp_iter_index;
						uh = (uh + 1) % _usize;
					}

					t_rmd->max_value = tmp_max_value;
					t_rmd->max_value_index = tmp_max_index;
				}
			}
			if ((nan_argmax_ == true) || (t_rmd->nan_num == 0)) {
				return t_rmd->max_value_index - 1;
			}
			else {
				return NAN;
			}
		}
	}
	return NAN;
}

RollingMinData::RollingMinData() {
	min_value = NAN;
	min_value_index = 0;
	nan_num = 0;
	value_l = nullptr;
}

RollingMinData::~RollingMinData() {
	if (value_l)
		delete value_l;
	value_l = nullptr;
}

RollingMinData & RollingMinData::operator=(const RollingMinData & rmd) {
	min_value = rmd.min_value;
	min_value_index = rmd.min_value_index;
	nan_num = rmd.nan_num;
	if (value_l) {
		delete value_l;
		value_l = nullptr;
	}
	if (rmd.value_l)
		value_l = new queue_ex<double>(*rmd.value_l);
	return *this;
}

RollingMinHelper::RollingMinHelper(unsigned int window, bool nan_min)
{
	window_ = window;
	nan_min_ = nan_min;
}

RollingMinHelper::RollingMinHelper(unsigned int window)
{
	window_ = window;
	nan_min_ = false;
}

RollingMinHelper::~RollingMinHelper()
{
}

double RollingMinHelper::cal_rolling_min(double value, int label) {
	auto label_l_iter = label_l_map_.find(label);
	if (label_l_iter == label_l_map_.end()) {
		RollingMinData & t_rmd = label_l_map_[label];
		t_rmd.value_l = new queue_ex<double>(window_);
		t_rmd.value_l->push(value);
		t_rmd.min_value_index = window_;
		t_rmd.min_value = value;
		t_rmd.nan_num += ::isnan(value) ? 1 : 0;
		
	}
	else {
		RollingMinData *t_rmd = &(label_l_iter->second);
		if (!t_rmd->value_l->full()) {
			t_rmd->value_l->push(value);
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			if (nan_less_equal(value, t_rmd->min_value)) {
				t_rmd->min_value = value;
				t_rmd->min_value_index = window_;
			}
			else {
				t_rmd->min_value_index -= 1;
			}
			if (t_rmd->value_l->full()) {
				if ((nan_min_ == true) || (t_rmd->nan_num == 0)) {
					return t_rmd->min_value;
				}
				else {
					return NAN;
				}
			}
		}
		else {
			double tmp_pop_value = t_rmd->value_l->front();
			t_rmd->value_l->pop();
			t_rmd->value_l->push(value);
			t_rmd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			if (nan_less_equal(value, t_rmd->min_value)) {
				t_rmd->min_value = value;
				t_rmd->min_value_index = window_;
			}
			else {
				t_rmd->min_value_index -= 1;
				if (t_rmd->min_value_index == 0) {
					double tmp_min_value;
					unsigned int tmp_min_index;
					unsigned int tmp_iter_index = 1;
					size_t uh = t_rmd->value_l->get_head_index();
					size_t tail = t_rmd->value_l->get_tail_index();
					double * vet = t_rmd->value_l->get_data_vector();
					size_t _usize = window_ + 1;
					size_t pre_uh = uh;
					while (uh != tail)
					{
						auto & val = vet[uh];
						if (tmp_iter_index == 1) {
							tmp_min_value = val;
							tmp_min_index = tmp_iter_index;
						}
						else {
							if (nan_less_equal(val, tmp_min_value)) {
								tmp_min_value = val;
								tmp_min_index = tmp_iter_index;
							}
						}
						pre_uh = uh;
						++tmp_iter_index;
						uh = (uh + 1) % _usize;
					}

					t_rmd->min_value = tmp_min_value;
					t_rmd->min_value_index = tmp_min_index;
				}
			}
			if ((nan_min_ == true) || (t_rmd->nan_num == 0)) {
				return t_rmd->min_value;
			}
			else {
				return NAN;
			}
		}
	}
	return NAN;
}

RollingArgminHelper::RollingArgminHelper(unsigned int window, bool nan_argmin, bool first_argmin)
{
	window_ = window;
	nan_argmin_ = nan_argmin;
	first_argmin_ = first_argmin;
}

RollingArgminHelper::RollingArgminHelper(unsigned int window, bool nan_argmin)
{
	window_ = window;
	nan_argmin_ = nan_argmin;
	first_argmin_ = true;
}

RollingArgminHelper::RollingArgminHelper(unsigned int window)
{
	window_ = window;
	nan_argmin_ = false;
	first_argmin_ = true;
}

RollingArgminHelper::~RollingArgminHelper()
{
}

double RollingArgminHelper::cal_rolling_argmin(double value, int label) {
	auto label_l_iter = label_l_map_.find(label);
	if (label_l_iter == label_l_map_.end()) {
		RollingMinData & t_rmd = label_l_map_[label];
		t_rmd.value_l = new queue_ex<double>(window_);
		t_rmd.value_l->push(value);
		t_rmd.min_value_index = window_;
		t_rmd.min_value = value;
		t_rmd.nan_num += ::isnan(value) ? 1 : 0;
		
	}
	else {
		RollingMinData *t_rmd = &(label_l_iter->second);
		if (!t_rmd->value_l->full()) {
			t_rmd->value_l->push(value);
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			bool less_flag = first_argmin_ ? nan_less(value, t_rmd->min_value) : nan_less_equal(value, t_rmd->min_value);
			if (less_flag) {
				t_rmd->min_value = value;
				t_rmd->min_value_index = window_;
			}
			else {
				t_rmd->min_value_index -= 1;
			}
			if (t_rmd->value_l->full()) {
				if ((nan_argmin_ == true) || (t_rmd->nan_num == 0)) {
					return t_rmd->min_value_index - 1;
				}
				else {
					return NAN;
				}
			}
		}
		else {
			double tmp_pop_value = t_rmd->value_l->front();
			t_rmd->value_l->pop();
			t_rmd->value_l->push(value);
			t_rmd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			bool less_flag = first_argmin_ ? nan_less(value, t_rmd->min_value) : nan_less_equal(value, t_rmd->min_value);
			if (less_flag) {
				t_rmd->min_value = value;
				t_rmd->min_value_index = window_;
			}
			else {
				t_rmd->min_value_index -= 1;
				if (t_rmd->min_value_index == 0) {
					double tmp_min_value;
					unsigned int tmp_min_index;
					unsigned int tmp_iter_index = 1;
					size_t uh = t_rmd->value_l->get_head_index();
					size_t tail = t_rmd->value_l->get_tail_index();
					double * vet = t_rmd->value_l->get_data_vector();
					size_t _usize = window_ + 1;
					size_t pre_uh = uh;
					while (uh != tail)
					{
						auto & val = vet[uh];
						if (tmp_iter_index == 1) {
							tmp_min_value = val;
							tmp_min_index = tmp_iter_index;
						}
						else {
							bool less_flag = first_argmin_ ? nan_less(val, tmp_min_value) : nan_less_equal(val, tmp_min_value);
							if (less_flag) {
								tmp_min_value = val;
								tmp_min_index = tmp_iter_index;
							}
						}
						pre_uh = uh;
						++tmp_iter_index;
						uh = (uh + 1) % _usize;
					}
					t_rmd->min_value = tmp_min_value;
					t_rmd->min_value_index = tmp_min_index;
				}
			}
			if ((nan_argmin_ == true) || (t_rmd->nan_num == 0)) {
				return t_rmd->min_value_index - 1;
			}
			else {
				return NAN;
			}
		}
	}
	return NAN;
}

FillNANHelper::FillNANHelper() {

}

FillNANHelper::~FillNANHelper() {

}

double FillNANHelper::cal_fillnan_value(double value, int label) {
	if (value_map_.find(label) == value_map_.end()) {
		if (::isnan(value)) {
			return NAN;
		}
		else {
			value_map_[label] = value;
			return value;
		}
	}
	else {
		if (::isnan(value)) {
			return value_map_[label];
		}
		else {
			value_map_[label] = value;
			return value;
		}
	}
}

RollingMeanHelper::RollingMeanHelper(unsigned int window, bool nan_mean)
{
	window_ = window;
	nan_mean_ = nan_mean;
}

RollingMeanHelper::RollingMeanHelper(unsigned int window)
{
	window_ = window;
	nan_mean_ = false;
}

RollingMeanHelper::~RollingMeanHelper()
{
}

double RollingMeanHelper::cal_rolling_mean(double value, int label)
{
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end()) {
		RollingSumData & t_rsd = label_q_map_[label];
		t_rsd.value_q = new queue_ex<double>(window_);
		t_rsd.value_q->push(value);
		t_rsd.nan_num += ::isnan(value) ? 1 : 0;
		t_rsd.sum += nan_to_zero(value);
	}
	else {
		RollingSumData & t_rsd = label_q_iter->second;
		if (!t_rsd.value_q->full()) {
			t_rsd.value_q->push(value);
			t_rsd.nan_num += ::isnan(value) ? 1 : 0;
			t_rsd.sum += nan_to_zero(value);
			//label_q_map_[label] = t_rsd;
			if (t_rsd.value_q->full()) {
				if (nan_mean_ == true) {
					return window_ > t_rsd.nan_num ? 
                        round_double(t_rsd.sum / (window_ - t_rsd.nan_num))
                        : NAN;
				}
				else {
					return t_rsd.nan_num == 0 ? round_double(t_rsd.sum / window_) : NAN;
				}
			}
		}
		else {
			double tmp_pop_value = t_rsd.value_q->front();
			t_rsd.value_q->pop();
			t_rsd.value_q->push(value);
			t_rsd.nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rsd.nan_num += ::isnan(value) ? 1 : 0;
			t_rsd.sum = t_rsd.sum - nan_to_zero(tmp_pop_value) + nan_to_zero(value);
			//label_q_map_[label] = t_rsd;
			if (nan_mean_ == true) {
				return window_ > t_rsd.nan_num ?
                    round_double(t_rsd.sum / (window_ - t_rsd.nan_num))
                    : NAN;
			}
			else {
				return t_rsd.nan_num == 0 ? round_double(t_rsd.sum / window_) : NAN;
			}
		}
	}
	return NAN;
}

RollingMeanHelperPre::RollingMeanHelperPre(unsigned int window, bool nan_mean)
{
	window_ = window;
	nan_mean_ = nan_mean;
}

RollingMeanHelperPre::RollingMeanHelperPre(unsigned int window)
{
	window_ = window;
	nan_mean_ = false;
}

RollingMeanHelperPre::~RollingMeanHelperPre()
{
}
RollingSumDataPre::RollingSumDataPre() {
	sum = 0;
	nan_num = 0;
}
double RollingMeanHelperPre::cal_rolling_mean(double value, int label)
{
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end()) {
		RollingSumDataPre & t_rsd = label_q_map_[label];
		queue<double> t_value_q;
		t_value_q.push(value);
		t_rsd.value_q = t_value_q;
		t_rsd.nan_num += ::isnan(value) ? 1 : 0;
		t_rsd.sum += nan_to_zero(value);
	}
	else {
		RollingSumDataPre *t_rsd = &(label_q_map_[label]);
		if (t_rsd->value_q.size() < window_) {
			t_rsd->value_q.push(value);
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum += nan_to_zero(value);
			//label_q_map_[label] = t_rsd;
			if (t_rsd->value_q.size() == window_) {
				if (nan_mean_ == true) {
					return round_double(t_rsd->sum / (window_ - t_rsd->nan_num));
				}
				else {
					return t_rsd->nan_num == 0 ? round_double(t_rsd->sum / window_) : NAN;
				}
			}
		}
		else {
			double tmp_pop_value = t_rsd->value_q.front();
			t_rsd->value_q.pop();
			t_rsd->value_q.push(value);
			t_rsd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum = t_rsd->sum - nan_to_zero(tmp_pop_value) + nan_to_zero(value);
			//label_q_map_[label] = t_rsd;
			if (nan_mean_ == true) {
				return round_double(t_rsd->sum / (window_ - t_rsd->nan_num));
			}
			else {
				return t_rsd->nan_num == 0 ? round_double(t_rsd->sum / window_) : NAN;
			}
		}
	}
	return NAN;
}

//占内存少，但是效率变低了，低两倍速
//RollingMeanHelperEx::RollingMeanHelperEx(unsigned int total_bar_num, unsigned int window, bool nan_mean)
//{
//	window_ = window;
//	nan_mean_ = nan_mean;
//	total_bar_num_ = total_bar_num;
//}
//
//RollingMeanHelperEx::RollingMeanHelperEx(unsigned int total_bar_num, unsigned int window)
//{
//	RollingMeanHelperEx(total_bar_num, window, false);
//}
//
//RollingMeanHelperEx::~RollingMeanHelperEx()
//{
//	label_q_map_.clear();
//}
//
//double RollingMeanHelperEx::cal_rolling_mean(const unsigned int & bar_num, const double & value, const int & label)
//{
//	auto label_q_iter = label_q_map_.find(label);
//	if (label_q_iter == label_q_map_.end()) {
//		vector<RollingSumData> & t_rsd_vet = label_q_map_[label];
//		t_rsd_vet.resize(total_bar_num_);
//		for (size_t i = 0; i < total_bar_num_; ++i) {
//			RollingSumData & t_rsd = t_rsd_vet[i];
//			t_rsd.value_q = new queue_ex<double>(window_);
//			if (bar_num == i) {
//				t_rsd.value_q->push(value);
//				t_rsd.nan_num += ::isnan(value) ? 1 : 0;
//				t_rsd.sum += nan_to_zero(value);
//			}
//		}
//	}
//	else {
//		vector<RollingSumData> & t_rsd_vet = label_q_iter->second;
//		if (bar_num > t_rsd_vet.size()) {
//			return NAN;
//		}
//		RollingSumData & t_rsd = t_rsd_vet[bar_num];
//		if (!t_rsd.value_q->full()) {
//			t_rsd.value_q->push(value);
//			t_rsd.nan_num += ::isnan(value) ? 1 : 0;
//			t_rsd.sum += nan_to_zero(value);
//			if (t_rsd.value_q->full()) {
//				if (nan_mean_ == true) {
//					return round_double(t_rsd.sum / (window_ - t_rsd.nan_num));
//				}
//				else {
//					return t_rsd.nan_num == 0 ? round_double(t_rsd.sum / window_) : NAN;
//				}
//			}
//		}
//		else {
//			double tmp_pop_value = t_rsd.value_q->front();
//			t_rsd.value_q->pop();
//			t_rsd.value_q->push(value);
//			t_rsd.nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
//			t_rsd.nan_num += ::isnan(value) ? 1 : 0;
//			t_rsd.sum = t_rsd.sum - nan_to_zero(tmp_pop_value) + nan_to_zero(value);
//			if (nan_mean_ == true) {
//				return round_double(t_rsd.sum / (window_ - t_rsd.nan_num));
//			}
//			else {
//				return t_rsd.nan_num == 0 ? round_double(t_rsd.sum / window_) : NAN;
//			}
//		}
//	}
//	return NAN;
//}

RollingTsstdHelper::RollingTsstdHelper(unsigned int window, bool nan_tsstd, bool unbiased) {
	window_ = window;
	nan_tsstd_ = nan_tsstd;
	unbiased_ = unbiased;
}

RollingTsstdHelper::RollingTsstdHelper(unsigned int window, bool nan_tsstd) {
	window_ = window;
	nan_tsstd_ = nan_tsstd;
	unbiased_ = true;
}

RollingTsstdHelper::RollingTsstdHelper(unsigned int window) {
	window_ = window;
	nan_tsstd_ = false;
	unbiased_ = true;
}

RollingTsstdHelper::~RollingTsstdHelper() {

}

double RollingTsstdHelper::cal_rolling_tsstd(double value, int label) {
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end()) {
		RollingStdData & t_rsd = label_q_map_[label];
		queue_ex<double> * t_value_q = new queue_ex<double>(window_);
		t_value_q->push(value);
		t_rsd.value_q = t_value_q;
		t_rsd.nan_num += ::isnan(value) ? 1 : 0;
		t_rsd.sum += nan_to_zero(value);
		t_rsd.square_sum = t_rsd.square_sum + nan_to_zero(value) * nan_to_zero(value);
	}
	else {
		RollingStdData *t_rsd = &label_q_iter->second;
		if (!t_rsd->value_q->full()) {
			t_rsd->value_q->push(value);
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum += nan_to_zero(value);
			t_rsd->square_sum = t_rsd->square_sum + nan_to_zero(value) * nan_to_zero(value);
			//label_q_map_[label] = t_rsd;
			if (t_rsd->value_q->full()) {
                if (window_ == t_rsd->nan_num) {
                    return NAN;
                }
				double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num);
                if (variance < 0) {
                    variance = 0;
                }
				double t_mean = NAN;
				double t_std = NAN;
				if (nan_tsstd_ == true) {
					if (unbiased_ == true) {
                        if (window_ == t_rsd->nan_num + 1) {
                            return NAN;
                        }
						t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - 1)));
					}
					else {
                        if (window_ == t_rsd->nan_num) {
                            return NAN;
                        }
						t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num)));
					}
                    if (window_ == t_rsd->nan_num) {
                        return NAN;
                    }
					t_mean = round_double(t_rsd->sum / (window_ - t_rsd->nan_num));
				}
				else {
					if (t_rsd->nan_num == 0) {
						if (unbiased_ == true) {
                            if (window_ == 1) {
                                return NAN;
                            }
							t_std = sqrt(round_double(variance / (window_ - 1)));
						}
						else {
							t_std = sqrt(round_double(variance / window_));
						}
						t_mean = round_double(t_rsd->sum / window_);
					}
					else {
						return NAN;
					}
				}
                if (t_std > -1e-12 && t_std < 1e-12) {
                    return NAN;
                }
				return (value - t_mean) / t_std;
			}
		}
		else {
			double tmp_pop_value = t_rsd->value_q->front();
			t_rsd->value_q->pop();
			t_rsd->value_q->push(value);
			t_rsd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum = t_rsd->sum - nan_to_zero(tmp_pop_value) + nan_to_zero(value);
			t_rsd->square_sum = t_rsd->square_sum - nan_to_zero(tmp_pop_value) * nan_to_zero(tmp_pop_value) + nan_to_zero(value) * nan_to_zero(value);
			//label_q_map_[label] = t_rsd;
            if (window_ == t_rsd->nan_num) {
                return NAN;
            }
			double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num);
			double t_mean = NAN;
			double t_std = NAN;
			if (nan_tsstd_ == true) {
				if (unbiased_ == true) {
                    if (window_ == t_rsd->nan_num + 1) {
                        return NAN;
                    }
					t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - 1)));
				}
				else {
                    if (window_ == t_rsd->nan_num) {
                        return NAN;
                    }
					t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num)));
				}
                if (window_ == t_rsd->nan_num) {
                    return NAN;
                }
				t_mean = round_double(t_rsd->sum / (window_ - t_rsd->nan_num));
			}
			else {
				if (t_rsd->nan_num == 0) {
					if (unbiased_ == true) {
                        if (window_ == 1) {
                            return NAN;
                        }
						t_std = sqrt(round_double(variance / (window_ - 1)));
					}
					else {
						t_std = sqrt(round_double(variance / window_));
					}
					t_mean = round_double(t_rsd->sum / window_);
				}
				else {
					return NAN;
				}
			}
            if (t_std > -1e-12 && t_std < 1e-12) {
                return NAN;
            }
			return (value - t_mean) / t_std;
		}
	}
	return NAN;
}

MultiRollingTsstdHelper::MultiRollingTsstdHelper(unsigned int window, bool nan_tsstd, bool unbiased) {
	window_ = window;
	nan_tsstd_ = nan_tsstd;
	unbiased_ = unbiased;
}

MultiRollingTsstdHelper::MultiRollingTsstdHelper(unsigned int window, bool nan_tsstd) {
	window_ = window;
	nan_tsstd_ = nan_tsstd;
	unbiased_ = true;
}

MultiRollingTsstdHelper::MultiRollingTsstdHelper(unsigned int window) {
	window_ = window;
	nan_tsstd_ = false;
	unbiased_ = true;
}

MultiRollingTsstdHelper::~MultiRollingTsstdHelper() {
	for (auto helper_iter = helper_map_.begin(); helper_iter != helper_map_.end(); helper_iter++) {
		if (helper_iter->second) {
			delete helper_iter->second;
			helper_iter->second = nullptr;
		}
	}
	helper_map_.clear();
}

double MultiRollingTsstdHelper::cal_multi_rolling_tsstd(double value, int label, int tsstd_label) {
	auto helper_iter = helper_map_.find(tsstd_label);
	if ( helper_iter == helper_map_.end()) {
		auto * p = new RollingTsstdHelper(window_, nan_tsstd_, unbiased_);
		helper_map_[tsstd_label] = p;
		return p->cal_rolling_tsstd(value, label);
	}
	else {
		return helper_iter->second->cal_rolling_tsstd(value, label);
	}
}

double CommonFuncs::GET_RANK(const list<double> & values, double target) {
	double count = 0.0;
	for (auto x : values) {
		if (target - x > 1e-8) {
			count++;
		}
	}
	return count / ((double)(values.size() - 1));
}
double CommonFuncs::GET_RANK(const vector<double> & values, double target) {
	double count = 0.0;
	for (auto x : values) {
		if (target - x > 1e-8) {
			count++;
		}
	}
	return count / ((double)(values.size() - 1));
}
	
double CommonFuncs::GET_VALUE_FROM_LIST(const list<double> & values, unsigned int arg) {
	auto values_front = values.begin();
	std::advance(values_front, arg);
	return *values_front;
}
int CommonFuncs::GET_VALUE_FROM_LIST(list<int> values, unsigned int arg) {
	auto values_front = values.begin();
	std::advance(values_front, arg);
	return *values_front;
}

vector<double> CommonFuncs::LIST_TO_VECTOR(list<double> li) {
	vector<double> vi;
	copy(li.begin(), li.end(), back_inserter(vi));
	return vi;
}

vector<double> CommonFuncs::RANGE(int length) {
	vector<double> range;
	if (length > 0) {
		for (int i = 0; i < length; i++) {
			range.push_back(i);
		}
	}
	return range;
}
vector<double> CommonFuncs::RANGE(double length) {
	vector<double> range;
	if (length > 0) {
		for (unsigned int i = 0; i < length; i++) {
			range.push_back(i);
		}
	}
	return range;
}
vector<double> CommonFuncs::RANGE(double start, double end, double length) {
	vector<double> range;
	if (!((start > end && length > 0) || (start < end && length < 0) || (start == end) || length == 0)) {
		if (length > 0) {
			double rollingValue = start;
			while (rollingValue < end) {
				range.push_back(rollingValue);
				rollingValue += length;
			}
		}
		else
		{
			double rollingValue = start;
			while (rollingValue > end) {
				range.push_back(rollingValue);
				rollingValue += length;
			}
		}
	}
	return range;
}

string CommonFuncs::ITERATE_TO_STRING(const vector<double> & values) {
	string s = "(";
	for (auto v : values) {
		s = s.append(to_string(v)).append("_");
	}
	return s.substr(0, s.size() - 1).append(")");
}

string CommonFuncs::ITERATE_TO_STRING(const list<double> & values) {
	string s = "(";
	for (auto v : values) {
		s = s.append(to_string(v)).append("_");
	}
	return s.substr(0, s.size() - 1).append(")");
}
string CommonFuncs::ITERATE_TO_STRING(vector<int> values) {
	string s = "(";
	for (auto v : values) {
		s = s.append(to_string(v)).append("_");
	}
	return s.substr(0, s.size() - 1).append(")");
}
string CommonFuncs::ITERATE_TO_STRING(list<int> values) {
	string s = "(";
	for (auto v : values) {
		s = s.append(to_string(v)).append("_");
	}
	return s.substr(0, s.size() - 1).append(")");
}
string CommonFuncs::ITERATE_TO_STRING(vector<string> values) {
	string s = "(";
	for (auto v : values) {
		s = s.append(v).append("_");
	}
	return s.substr(0, s.size() - 1).append(")");
}
string CommonFuncs::ITERATE_TO_STRING(list<string> values) {
	string s = "(";
	for (auto v : values) {
		s = s.append(v).append("_");
	}
	return s.substr(0, s.size() - 1).append(")");
}

double CommonFuncs::LAST_RANK(const list<double> & values) {
	return GET_RANK(values, values.back());
}

double CommonFuncs::LAST_RANK(const vector<double> & values) {
	return GET_RANK(values, values.back());
}

double CommonFuncs::SUM(const list<double> & values) {
	double sum = 0;
	for (auto x : values) {
		sum += x;
	}
	return sum;
}

double CommonFuncs::SUM(const vector<double> & values) {
	double sum = 0;
	for (auto x : values) {
		sum += x;
	}
	return sum;
}

double CommonFuncs::NAN_SUM(const list<double> & values) {
	double sum = 0;
	for (auto x : values) {
		if (!(::isnan(x))) {
			sum += x;
		}
	}
	return sum;
}

double CommonFuncs::NAN_SUM(const vector<double> & values) {
	double sum = 0;
	for (auto x : values) {
		if (!(::isnan(x))) {
			sum += x;
		}
	}
	return sum;
}

double CommonFuncs::MEAN(const list<double> & values) {
	if (values.size() == 0)
		return NAN;
	return SUM(values) / ((double)values.size());
}

double CommonFuncs::MEAN(const vector<double> & values) {
	if (values.size() == 0)
		return NAN;
	return SUM(values) / ((double)values.size());
}

double CommonFuncs::NAN_MEAN(const list<double> & values) {
	if (values.size() == 0)
		return NAN;
	double count = 0;
	double sum = 0;
	for (auto x : values) {
		if (!(::isnan(x))) {
			count++;
			sum += x;
		}
	}
	return sum / count;
}

double CommonFuncs::NAN_MEAN(const vector<double> & values) {
	if (values.size() == 0) {
		return NAN;
	}
	double count = 0;
	double sum = 0;
	for (auto x : values) {
		if (!(::isnan(x))) {
			count++;
			sum += x;
		}
	}
	return sum / count;
}

double CommonFuncs::EMA(const list<double> & values, double decay) {
	double alpha = 1.0 - 1.0 / (1.0 + decay);
	unsigned int size = (unsigned int)values.size();
	vector<double> weights(size);
	double weightsSum = 0;
	for (unsigned int i = 0; i < size; i++) {
		double currentWeight = pow(alpha, (size - i - 1));
		weights[i] = currentWeight;
		weightsSum += currentWeight;
	}
	double emaSum = 0;
	unsigned i = 0;
	for (auto x : values) {
		emaSum += (x * weights[i]);
		i++;
	}
	return emaSum / weightsSum;
}

double CommonFuncs::EMA(const vector<double> & values, double decay) {
	double alpha = 1.0 - 1.0 / (1.0 + decay);
	unsigned int size = (unsigned int)values.size();
	vector<double> weights(size);
	double weightsSum = 0;
	for (unsigned int i = 0; i < size; i++) {
		double currentWeight = pow(alpha, (size - i - 1));
		weights[i] = currentWeight;
		weightsSum += currentWeight;
	}
	double emaSum = 0;
	unsigned i = 0;
	for (auto x : values) {
		emaSum += (x * weights[i]);
		i++;
	}
	return emaSum / weightsSum;
}

double CommonFuncs::EMA(const vector<double> & values, vector<double> & weights, double weightsSum)
{
    if (values.size() != weights.size())
    {
        return NAN;
    }
    double emaSum = 0;
    unsigned i = 0;
    for (auto x : values) {
        emaSum += (x * weights[i]);
        i++;
    }
    return emaSum / weightsSum;
}

double CommonFuncs::PROD(const list<double> & values) {
	double prod = 1.0;
	for (auto x : values) {
		prod *= x;
	}
	return prod;
}

double CommonFuncs::PROD(const vector<double> & values) {
	double prod = 1.0;
	for (auto x : values) {
		prod *= x;
	}
	return prod;
}

double CommonFuncs::NAN_PROD(const list<double> & values) {
	double prod = 1.0;
	for (auto x : values) {
		if (!(::isnan(x))) {
			prod *= x;
		}
	}
	return prod;
}

double CommonFuncs::NAN_PROD(const vector<double> & values) {
	double prod = 1.0;
	for (auto x : values) {
		if (!(::isnan(x))) {
			prod *= x;
		}
	}
	return prod;
}

double CommonFuncs::STD(const list<double> & values) {
	if (values.size() == 0)
		return NAN;
	double mean = MEAN(values);
	double squareSum = 0;
	for (auto x : values) {
		squareSum += pow((x - mean), 2);
	}
	return pow(squareSum / ((double)(values.size() - 1)), 0.5);
}

double CommonFuncs::STD(const vector<double> & values) {
	if (values.size() == 0)
		return NAN;
	double mean = MEAN(values);
	double squareSum = 0;
	for (auto x : values) {
		squareSum += pow((x - mean), 2);
	}
	return pow(squareSum / ((double)(values.size() - 1)), 0.5);
}

double CommonFuncs::NAN_STD(const list<double> & values) {
	if (values.size() == 0)
		return NAN;
	double mean = NAN_MEAN(values);
	double squareSum = 0;
	double notNanCount = 0;
	for (auto x : values) {
		if (!(::isnan(x))) {
			squareSum += pow((x - mean), 2);
			notNanCount++;
		}
	}
	return pow(squareSum / (notNanCount - 1), 0.5);
}

double CommonFuncs::NAN_STD(const vector<double> & values) {
	if (values.size() == 0)
		return NAN;
	double mean = NAN_MEAN(values);
	double squareSum = 0;
	double notNanCount = 0;
	for (auto x : values) {
		if (!(::isnan(x))) {
			squareSum += pow((x - mean), 2);
			notNanCount++;
		}
	}
	return pow(squareSum / (notNanCount - 1), 0.5);
}

double CommonFuncs::CORR(const list<double> & X, const list<double> & Y)
{
	if (X.size() == 0 || Y.size() == 0) {
		return NAN;
	}
	if (X.size() != Y.size()) {
		throw "list size not equal with each other!";
	}

	double size = (double)X.size();

	vector<double> X_vector = LIST_TO_VECTOR(X);
	vector<double> Y_vector = LIST_TO_VECTOR(Y);
	double sum_XY = 0;
	double squareSum_X = 0, squareSum_Y = 0;

	double sum_X = SUM(X);
	double sum_Y = SUM(Y);

	for (unsigned long long int i = 0; i < size; i++)
	{
		// sum of elements of array X. 
		// sum of elements of array Y.
		// sum of X[i] * Y[i]. 
		sum_XY = sum_XY + X_vector[i] * Y_vector[i];
		// sum of square of array elements. 
		squareSum_X = squareSum_X + X_vector[i] * X_vector[i];
		squareSum_Y = squareSum_Y + Y_vector[i] * Y_vector[i];
	}
	// use formula for calculating correlation coefficient. 
	double corr = (double)(size * sum_XY - sum_X * sum_Y);
	double divide = sqrt((size * squareSum_X - sum_X * sum_X)
		* (size * squareSum_Y - sum_Y * sum_Y));
	if (divide == 0) {
		corr = NAN;
	}
	else {
		corr /= divide;
	}
	return corr;
}
double CommonFuncs::CORR(const vector<double> & X, const vector<double> & Y)
{
	if (X.size() == 0 || Y.size() == 0)
		return NAN;
	if (X.size() != Y.size()) {
		throw "list size not equal with each other!";
	}

	double size = (double)X.size();
	double sum_XY = 0;
	double squareSum_X = 0, squareSum_Y = 0;

	double sum_X = SUM(X);
	double sum_Y = SUM(Y);

	for (unsigned long long int i = 0; i < size; i++)
	{
		// sum of elements of array X. 
		// sum of elements of array Y.
		// sum of X[i] * Y[i]. 
		sum_XY = sum_XY + X[i] * Y[i];
		// sum of square of array elements. 
		squareSum_X = squareSum_X + X[i] * X[i];
		squareSum_Y = squareSum_Y + Y[i] * Y[i];
	}
	// use formula for calculating correlation coefficient. 
	double corr = (double)(size * sum_XY - sum_X * sum_Y);
	double divide = sqrt((size * squareSum_X - sum_X * sum_X)
		* (size * squareSum_Y - sum_Y * sum_Y));
	if (divide == 0) {
		corr = NAN;
	}
	else {
		corr /= divide;
	}
	return corr;
}

double CommonFuncs::NAN_CORR(const list<double> & X, const list<double> & Y)
{
	if (X.size() == 0 || Y.size() == 0)
		return NAN;
	if (X.size() != Y.size()) {
		throw "list size not equal with each other!";
	}

	double size = (double)X.size();
	double listSize = size;

	vector<double> X_vector = LIST_TO_VECTOR(X);
	vector<double> Y_vector = LIST_TO_VECTOR(Y);
	double sum_X = 0, sum_Y = 0, sum_XY = 0;
	double squareSum_X = 0, squareSum_Y = 0;

	for (unsigned long long int i = 0; i < listSize; i++)
	{
		if ((::isnan(X_vector[i]) || (::isnan(Y_vector[i])))) {
			size--;
			continue;
		}
		// sum of elements of array X.
		sum_X += X_vector[i];
		// sum of elements of array Y.
		sum_Y += Y_vector[i];
		// sum of X[i] * Y[i]. 
		sum_XY = sum_XY + X_vector[i] * Y_vector[i];
		// sum of square of array elements. 
		squareSum_X = squareSum_X + X_vector[i] * X_vector[i];
		squareSum_Y = squareSum_Y + Y_vector[i] * Y_vector[i];
	}
	// use formula for calculating correlation coefficient. 
	double corr = (double)(size * sum_XY - sum_X * sum_Y);
	double divide = sqrt((size * squareSum_X - sum_X * sum_X)
		* (size * squareSum_Y - sum_Y * sum_Y));
	if (divide == 0) {
		corr = NAN;
	}
	else {
		corr /= divide;
	}
	return corr;
}

double CommonFuncs::NAN_CORR(const vector<double> & X, const vector<double> & Y)
{
	if (X.size() == 0 || Y.size() == 0)
		return NAN;
	if (X.size() != Y.size()) {
		throw "list size not equal with each other!";
	}

	double size = (double)X.size();
	double listSize = size;
	double sum_X = 0, sum_Y = 0, sum_XY = 0;
	double squareSum_X = 0, squareSum_Y = 0;

	for (unsigned long long int i = 0; i < listSize; i++)
	{
		if ((::isnan(X[i]) || (::isnan(Y[i])))) {
			size--;
			continue;
		}
		// sum of elements of array X. 
		sum_X += X[i];
		// sum of elements of array Y.
		sum_Y += Y[i];
		// sum of X[i] * Y[i]. 
		sum_XY = sum_XY + X[i] * Y[i];
		// sum of square of array elements. 
		squareSum_X = squareSum_X + X[i] * X[i];
		squareSum_Y = squareSum_Y + Y[i] * Y[i];
	}
	// use formula for calculating correlation coefficient. 
	double corr = (double)(size * sum_XY - sum_X * sum_Y);
	double divide = sqrt((size * squareSum_X - sum_X * sum_X)
		* (size * squareSum_Y - sum_Y * sum_Y));
    //cout << size << " -- " << sum_XY<<" -- "<< size * sum_XY 
    //    <<" -- "<< sum_X * sum_Y
    //    << " -- "<< size * sum_XY - sum_X * sum_Y << endl;
    //cout << corr << " -- " << divide << endl;
	if (fabs(divide) < 0.000001) {
		corr = NAN;
	}
	else {
		corr /= divide;
	}
	return corr;
}

double CommonFuncs::GMEAN(const list<double> & values) {
	if (values.size() == 0)
		return NAN;
	return pow(PROD(values), (1.0 / (double)values.size()));
}

double CommonFuncs::GMEAN(const vector<double> & values) {
	if (values.size() == 0)
		return NAN;
	return pow(PROD(values), (1.0 / (double)values.size()));
}

double CommonFuncs::NAN_GMEAN(const list<double> & values) {
	if (values.size() == 0)
		return NAN;
	double prod = 1.0;
	double count = 0.0;
	for (auto x : values) {
		if (!(::isnan(x))) {
			count++;
			prod *= x;
		}
	}
	return pow(prod, (1.0 / count));
}

double CommonFuncs::NAN_GMEAN(const vector<double> & values) {
	if (values.size() == 0)
		return NAN;
	double prod = 1.0;
	double count = 0.0;
	for (auto x : values) {
		if (!(::isnan(x))) {
			count++;
			prod *= x;
		}
	}
	return pow(prod, (1.0 / count));
}

double CommonFuncs::NK_MAX(const list<double> & values) {
	if (values.size() == 0)
		return NAN;
	double max = values.front();
	for (auto x : values) {
		if (x > max) {
			max = x;
		}
	}
	return max;
}
double CommonFuncs::NK_MAX(const vector<double> & values) {
	if (values.size() == 0)
		return NAN;
	double max = values.front();
	for (auto x : values) {
		if (x > max) {
			max = x;
		}
	}
	return max;
}

double CommonFuncs::NK_MIN(const list<double> & values) {
	if (values.size() == 0)
		return NAN;
	double min = values.front();
	for (auto x : values) {
		if (x < min) {
			min = x;
		}
	}
	return min;
}

double CommonFuncs::NK_MIN(const vector<double> & values) {
	if (values.size() == 0)
		return NAN;
	double min = values.front();
	for (auto x : values) {
		if (x < min) {
			min = x;
		}
	}
	return min;
}

#ifdef _WIN32
double CommonFuncs::MAX(const list<double> & values) {
	if (values.size() == 0)
		return NAN;
	double max = values.front();
	for (auto x : values) {
		if (x > max) {
			max = x;
		}
	}
	return max;
}
double CommonFuncs::MAX(const vector<double> & values) {
	if (values.size() == 0)
		return NAN;
	double max = values.front();
	for (auto x : values) {
		if (x > max) {
			max = x;
		}
	}
	return max;
}

double CommonFuncs::MIN(const list<double> & values) {
	if (values.size() == 0)
		return NAN;
	double min = values.front();
	for (auto x : values) {
		if (x < min) {
			min = x;
		}
	}
	return min;
}

double CommonFuncs::MIN(const vector<double> & values) {
	if (values.size() == 0)
		return NAN;
	double min = values.front();
	for (auto x : values) {
		if (x < min) {
			min = x;
		}
	}
	return min;
}
#endif

double CommonFuncs::ARGMAX(const list<double> & values) {
	if (values.size() == 0)
		return NAN;
	unsigned int argmax = 0;
	int count = -1;
	double max = values.front();
	for (auto cursor : values) {
		count++;
		if (cursor > max) {
			max = cursor;
			argmax = count;
		}
	}
	return argmax;
}

double CommonFuncs::ARGMAX(const vector<double> & values) {
	if (values.size() == 0)
		return NAN;
	unsigned int argmax = 0;
	int count = -1;
	double max = values.front();
	for (auto cursor : values) {
		count++;
		if (cursor > max) {
			max = cursor;
			argmax = count;
		}
	}
	return argmax;
}

double CommonFuncs::ARGMIN(const list<double> & values) {
	if (values.size() == 0)
		return NAN;
	unsigned int argmin = 0;
	int count = -1;
	double min = values.front();
	for (auto cursor : values) {
		count++;
		if (cursor < min) {
			min = cursor;
			argmin = count;
		}
	}
	return argmin;
}

double CommonFuncs::ARGMIN(const vector<double> & values) {
	if (values.size() == 0)
		return NAN;
	unsigned int argmin = 0;
	int count = -1;
	double min = values.front();
	for (auto cursor : values) {
		count++;
		if (cursor < min) {
			min = cursor;
			argmin = count;
		}
	}
	return argmin;
}

int CommonFuncs::timet_to_3s_bar_num(const time_t & time) {
#ifdef _WIN32
	struct tm tp;
	time_t local_time = time + 28800;
	gmtime_s(&tp, &local_time);
	return tp.tm_hour < 12 ? (tp.tm_hour - 9) * 1200 + (tp.tm_min - 30) * 20 + tp.tm_sec / 3 + 1 : 2401 + (tp.tm_hour - 13) * 1200 + tp.tm_min * 20 + tp.tm_sec / 3;
#else
        time_t local_time = time + 28800;
        struct tm * tp = gmtime(&local_time);
	return tp->tm_hour < 12 ? (tp->tm_hour - 9) * 1200 + (tp->tm_min - 30) * 20 + tp->tm_sec / 3 + 1 : 2401 + (tp->tm_hour - 13) * 1200 + tp->tm_min * 20 + tp->tm_sec / 3;
#endif
}

double CommonFuncs::LAST_RANK_EQ(const list<double> & values) {
	return GET_RANK(values, values.back() + 1e-7)*(values.size() - 1);
}

double CommonFuncs::LAST_RANK_EQ(const vector<double> & values) {
	return GET_RANK(values, values.back() + 1e-7)*(values.size() - 1);
}

double CommonFuncs::get_rank(const queue_ex<double> * queue, const double & target) {
	if (queue->empty()) {
		return false;
	}
	double count = 0.0;
	double v_size = 0.0;
	size_t head = queue->_uhead;
	size_t tail = queue->_utail;
	size_t usize = queue->_usize;
	double * vet = queue->value_q;
	while (head != tail) {
		const double & val = vet[head];
		if (target - val > 1e-8) {
			count += 1.0;
		}
		v_size += 1.0;
		head = (head + 1) % usize;
	}
	return count / (v_size - 1);
}

double CommonFuncs::get_last_rank(const queue_ex<double> * queue) {
	if (queue->empty()) {
		return false;
	}
	double count = 0.0;
	double v_size = 0.0;
	size_t head = queue->_uhead;
	size_t tail = queue->_utail;
	size_t usize = queue->_usize;	
	double * vet = queue->value_q;
	const double & target = vet[(tail + usize - 1) % usize];
	while (head != tail) {
		const double & val = vet[head];
		if (target - val > 1e-8) {
			count += 1.0;
		}
		v_size += 1.0;
		head = (head + 1) % usize;
	}
	return count / (v_size - 1);
}

//获取版本号
const char * CommonFuncs::GetVersion() {
	return FDLL_VERSION;
}

/// add 20200622 beg

CumSumData::CumSumData() {
	sum = 0;
	num = 0;
}

CumSumHelper::CumSumHelper() {
	min_periods_ = 1;
}

CumSumHelper::CumSumHelper(unsigned int min_periods) {
	min_periods_ = min_periods;
}

CumSumHelper::~CumSumHelper() {

}

double CumSumHelper::cal_cum_sum(double value, int label) {
	unordered_map<int, CumSumData>::const_iterator label_map_iter = label_map_.find(label);
	if (label_map_iter == label_map_.end()) {
		CumSumData & t_csd = label_map_[label];
		t_csd.num += ::isnan(value) ? 0 : 1;
		t_csd.sum += nan_to_zero(value);
		if (t_csd.num >= min_periods_) {
			return t_csd.sum;
		}
	}
	else {
		CumSumData *t_csd = &(label_map_[label]);
		t_csd->num += ::isnan(value) ? 0 : 1;
		t_csd->sum += nan_to_zero(value);
		if (t_csd->num >= min_periods_) {
			return t_csd->sum;
		}
	}
	return NAN;
}

int CumSumHelper::reset(int label) {
	unordered_map<int, CumSumData>::const_iterator label_map_iter = label_map_.find(label);
	if (label_map_iter != label_map_.end()) {
		CumSumData *t_csd = &(label_map_[label]);
		t_csd->num = 0;
		t_csd->sum = 0;
	}
	return 0;
}

void CumSumHelper::SN_reset()
{
	unordered_map<int, CumSumData>::iterator label_map_iter = label_map_.begin();
	for (; label_map_iter != label_map_.end(); label_map_iter++)
	{
		CumSumData *t_csd = &(label_map_iter->second);
		t_csd->num = 0;
		t_csd->sum = 0;
	}
}

CumMeanHelper::CumMeanHelper() {
	min_periods_ = 1;
}

CumMeanHelper::CumMeanHelper(unsigned int min_periods) {
	min_periods_ = min_periods;
}

CumMeanHelper::~CumMeanHelper() {

}

double CumMeanHelper::cal_cum_mean(double value, int label) {
	unordered_map<int, CumSumData>::const_iterator label_map_iter = label_map_.find(label);
	if (label_map_iter == label_map_.end()) {
		CumSumData & t_csd = label_map_[label];
		t_csd.num += ::isnan(value) ? 0 : 1;
		t_csd.sum += nan_to_zero(value);
		if (t_csd.num >= min_periods_) {
			return round_double(t_csd.sum / t_csd.num);
		}
	}
	else {
		CumSumData *t_csd = &(label_map_[label]);
		t_csd->num += ::isnan(value) ? 0 : 1;
		t_csd->sum += nan_to_zero(value);
		if (t_csd->num >= min_periods_) {
			return round_double(t_csd->sum / t_csd->num);
		}
	}
	return NAN;
}

int CumMeanHelper::reset(int label) {
	unordered_map<int, CumSumData>::const_iterator label_map_iter = label_map_.find(label);
	if (label_map_iter != label_map_.end()) {
		CumSumData *t_csd = &(label_map_[label]);
		t_csd->num = 0;
		t_csd->sum = 0;
	}
	return 0;
}

void CumMeanHelper::SN_reset() {
	unordered_map<int, CumSumData>::iterator label_map_iter = label_map_.begin();
	for (; label_map_iter != label_map_.end(); label_map_iter++)
	{
		CumSumData *t_csd = &(label_map_iter->second);
		t_csd->num = 0;
		t_csd->sum = 0;
	}
}

CumCorrData::CumCorrData() {
	square_sum1 = 0;
	square_sum2 = 0;
	sum1 = 0;
	sum2 = 0;
	sum12 = 0;
	num = 0;
}

CumCorrHelper::CumCorrHelper() {
	min_periods_ = 1;
}

CumCorrHelper::CumCorrHelper(unsigned int min_periods) {
	min_periods_ = min_periods;
}

CumCorrHelper::~CumCorrHelper() {

}

double CumCorrHelper::cal_cum_corr(double value1, double value2, int label) {
	unordered_map<int, CumCorrData>::const_iterator label_map_iter = label_map_.find(label);
	if (label_map_iter == label_map_.end()) {
		CumCorrData & t_ccd = label_map_[label];
		bool tmp_nan_flag = (::isnan(value1) || ::isnan(value2));
		t_ccd.num += tmp_nan_flag ? 0 : 1;
		if (tmp_nan_flag == false) {
			t_ccd.sum1 += value1;
			t_ccd.sum2 += value2;
			t_ccd.sum12 += value1 * value2;
			t_ccd.square_sum1 += value1 * value1;
			t_ccd.square_sum2 += value2 * value2;
		}
		if (t_ccd.num >= min_periods_) {
			double variance1 = round_double((t_ccd.square_sum1 - t_ccd.sum1 * t_ccd.sum1 / t_ccd.num) / t_ccd.num);
			double variance2 = round_double((t_ccd.square_sum2 - t_ccd.sum2 * t_ccd.sum2 / t_ccd.num) / t_ccd.num);
			if ((variance1 < 1e-12) || (variance2 < 1e-12)) {
				return NAN;
			}
			else {
				double std1 = sqrt(variance1);
				double std2 = sqrt(variance2);
				double mean1 = t_ccd.sum1 / t_ccd.num;
				double mean2 = t_ccd.sum2 / t_ccd.num;
				double mean12 = t_ccd.sum12 / t_ccd.num;
				return round_double((mean12 - mean1 * mean2) / round_double((std1 * std2)));
			}
		}
	}
	else {
		CumCorrData *t_ccd = &(label_map_[label]);
		bool tmp_nan_flag = (::isnan(value1) || ::isnan(value2));
		t_ccd->num += tmp_nan_flag ? 0 : 1;
		if (tmp_nan_flag == false) {
			t_ccd->sum1 += value1;
			t_ccd->sum2 += value2;
			t_ccd->sum12 += value1 * value2;
			t_ccd->square_sum1 += value1 * value1;
			t_ccd->square_sum2 += value2 * value2;
		}
		if (t_ccd->num >= min_periods_) {
			double variance1 = round_double((t_ccd->square_sum1 - t_ccd->sum1 * t_ccd->sum1 / t_ccd->num) / t_ccd->num);
			double variance2 = round_double((t_ccd->square_sum2 - t_ccd->sum2 * t_ccd->sum2 / t_ccd->num) / t_ccd->num);
			if ((variance1 < 1e-12) || (variance2 < 1e-12)) {
				return NAN;
			}
			else {
				double std1 = sqrt(variance1);
				double std2 = sqrt(variance2);
				double mean1 = t_ccd->sum1 / t_ccd->num;
				double mean2 = t_ccd->sum2 / t_ccd->num;
				double mean12 = t_ccd->sum12 / t_ccd->num;
				return round_double((mean12 - mean1 * mean2) / round_double((std1 * std2)));
			}
		}
	}
	return NAN;
}

int CumCorrHelper::reset(int label) {
	unordered_map<int, CumCorrData>::const_iterator label_map_iter = label_map_.find(label);
	if (label_map_iter != label_map_.end()) {
		CumCorrData *t_ccd = &(label_map_[label]);
		t_ccd->square_sum1 = 0;
		t_ccd->square_sum2 = 0;
		t_ccd->sum1 = 0;
		t_ccd->sum2 = 0;
		t_ccd->sum12 = 0;
		t_ccd->num = 0;
	}
	return 0;
}
//// add 20200622 end
void  CumCorrHelper::SN_reset() {
	unordered_map<int, CumCorrData>::iterator label_map_iter = label_map_.begin();
	for (; label_map_iter != label_map_.end(); label_map_iter++)
	{
		CumCorrData *t_ccd = &(label_map_iter->second);
		t_ccd->square_sum1 = 0;
		t_ccd->square_sum2 = 0;
		t_ccd->sum1 = 0;
		t_ccd->sum2 = 0;
		t_ccd->sum12 = 0;
		t_ccd->num = 0;
	}
}

CumStdData::CumStdData(){
	square_sum = 0;
	sum = 0;
	nan_num = 0;
	num = 0;
}

CumStdData::~CumStdData(){
}

CumStdData & CumStdData::operator=(const CumStdData & rsd) {
	square_sum = rsd.square_sum;
	sum = rsd.sum;
	nan_num = rsd.nan_num;
	num = rsd.num;
	return *this;
}

CumStdHelper::CumStdHelper(unsigned int min_periods, bool nan_std, bool unbiased){
	min_periods_ = min_periods;
	nan_std_ = nan_std;
	unbiased_ = unbiased;
}

CumStdHelper::~CumStdHelper(){
}

double CumStdHelper::cal_cum_std(double value, int label){
	CumStdData & t_rsd = label_q_map_[label];
	t_rsd.nan_num += ::isnan(value) ? 1 : 0;
	t_rsd.num += 1;
	t_rsd.sum += nan_to_zero(value);
	t_rsd.square_sum = t_rsd.square_sum + nan_to_zero(value) * nan_to_zero(value);
	if (t_rsd.num == 1 || t_rsd.num < min_periods_) {
		return NAN;
	}
	if (t_rsd.num == t_rsd.nan_num) {
		return NAN;
	}
	double variance = t_rsd.square_sum - t_rsd.sum * t_rsd.sum / (t_rsd.num - t_rsd.nan_num);
	if (variance < 0) {
		variance = 0;
	}
	if (nan_std_ == true) {
		if (unbiased_ == true) {
			if (t_rsd.num == t_rsd.nan_num + 1) {
				return NAN;
			}
			return sqrt(round_double(variance / (t_rsd.num - t_rsd.nan_num - 1)));
		}
		else {
			if (t_rsd.num == t_rsd.nan_num) {
				return NAN;
			}
			return sqrt(round_double(variance / (t_rsd.num - t_rsd.nan_num)));
		}
	}
	else {
		if (t_rsd.nan_num == 0) {
			if (unbiased_ == true) {
				if (t_rsd.num == 1) {
					return NAN;
				}
				return sqrt(round_double(variance / (t_rsd.num - 1)));
			}
			else {
				return sqrt(round_double(variance / t_rsd.num));
			}
		}
		else {
			return NAN;
		}
	}	
	return NAN;
}

int CumStdHelper::reset(int label) {
	auto itor = label_q_map_.find(label);
	if (itor != label_q_map_.end()) {
		CumStdData & csd = itor->second;
		csd.nan_num = 0;
		csd.num = 0;
		csd.square_sum = 0;
		csd.sum = 0;
	}
	return 1;
}
void CumStdHelper::SN_reset() {
	for (auto & itor : label_q_map_) {
		CumStdData & csd = itor.second;
		csd.nan_num = 0;
		csd.num = 0;
		csd.square_sum = 0;
		csd.sum = 0;
	}
}

/// add 20200701 begin
RollingEmaData::RollingEmaData() {
	mean = 0;
	nan_num = 0;
}


RollingEmaHelper::RollingEmaHelper(unsigned int window, double decay)
{
	window_ = window;
	decay_ = decay;
}

RollingEmaHelper::~RollingEmaHelper()
{
}

double RollingEmaHelper::cal_rolling_ema(double value, int label)
{
	unordered_map<int, RollingEmaData>::const_iterator label_q_iter = label_q_map_.find(label);
	double p = decay_ / (1.0 + decay_);
	if (label_q_iter == label_q_map_.end()) {
		RollingEmaData & t_red = label_q_map_[label];
		t_red.value_q.push(value);
		t_red.nan_num += ::isnan(value) ? 1 : 0;
		t_red.mean += nan_to_zero(value);
	}
	else {
		RollingEmaData *t_red = &(label_q_map_[label]);
		if (t_red->value_q.size() < window_) {
			t_red->value_q.push(value);
			t_red->nan_num += ::isnan(value) ? 1 : 0;
			int n = t_red->value_q.size();
			double oldsum = (1 - pow(p, n - 1)) / (1 - p);
			double newsum = (1 - pow(p, n)) / (1 - p);
			t_red->mean = (t_red->mean*oldsum*p + nan_to_zero(value)) / newsum;
			//label_q_map_[label] = t_rsd;
			if (t_red->value_q.size() == window_) {
				if (false) {
					return round_double(t_red->mean);
				}
				else {
					return t_red->nan_num == 0 ? round_double(t_red->mean) : NAN;
				}
			}
		}
		else {
			double tmp_pop_value = t_red->value_q.front();
			t_red->value_q.pop();
			t_red->value_q.push(value);
			t_red->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_red->nan_num += ::isnan(value) ? 1 : 0;
			int n = t_red->value_q.size();

			double oldsum = (1 - pow(p, n)) / (1 - p);
			double newsum = oldsum;
			t_red->mean = (t_red->mean*oldsum*p - nan_to_zero(tmp_pop_value)*pow(p, n) + nan_to_zero(value)) / newsum;
			//label_q_map_[label] = t_rsd;
			if (false) {
				return round_double(t_red->mean);
			}
			else {
				return t_red->nan_num == 0 ? round_double(t_red->mean) : NAN;
			}
		}
	}
	return NAN;
}
/// add 20200701 end
RollingMedianHelper::RollingMedianHelper(unsigned int window)
{
	window_ = window;
}
RollingMedianHelper::~RollingMedianHelper()
{

}

double RollingMedianHelper::cal_rolling_median(double value, int label)
{
	RollingMedianData &t_red = label_q_map_[label];
	TMedianNode *pTNewMedianNode = NULL;
	if (t_red.value_q.size() < window_)
	{
		pTNewMedianNode = new TMedianNode;
		if (pTNewMedianNode == NULL)
		{
			return NAN;
		}
		pTNewMedianNode->next = NULL;
		pTNewMedianNode->pre = NULL;
		pTNewMedianNode->value = value;
	}
	else
	{
		TMedianNode *pTop = t_red.value_q.front();
		t_red.value_q.pop();
		if(pTop == NULL){
			cout<<"000 pTop = NULL..."<<endl;
		}
		//std::cout << "pop " << pTop->value << endl;
		if (!::isnan(pTop->value))
		{
			t_red.num -= 1;
			if (pTop == t_red.pMedian)
			{
				if (t_red.num == 0)
				{
					t_red.pMedian = NULL;
				}
				else
				{
					if (t_red.num % 2 == 0)
					{
						if (t_red.pMedian->pre != NULL)
						{
							t_red.pMedian->pre->next = t_red.pMedian->next;
						}
						if (t_red.pMedian->next != NULL)
						{
							t_red.pMedian->next->pre = t_red.pMedian->pre;
						}
						t_red.pMedian = t_red.pMedian->pre;
					}
					else
					{
						if (t_red.pMedian->pre != NULL)
						{
							t_red.pMedian->pre->next = t_red.pMedian->next;
						}
						if (t_red.pMedian->next != NULL)
						{
							t_red.pMedian->next->pre = t_red.pMedian->pre;
						}
						t_red.pMedian = t_red.pMedian->next;
					}
				}
			}
			else
			{
				double zero_val = 1e-13;
			//判断当前数据是在median的左边还是右边
				TMedianNode *pForeach = pTop;
				if (fabs(pTop->value - t_red.pMedian->value) < zero_val)
				{
					pForeach = pTop->next;
					while (pForeach && fabs(pForeach->value - t_red.pMedian->value) < zero_val)
					{
						if (pForeach == t_red.pMedian)
						{
							if (t_red.num % 2 == 1)
							{
								t_red.pMedian = t_red.pMedian->next;
							}
							break;
						}
						pForeach = pForeach->next;
					}
					if (pForeach == NULL || fabs(pForeach->value - t_red.pMedian->value) > zero_val) //表示不等
					{
						if (t_red.num % 2 == 0)
						{
							t_red.pMedian = t_red.pMedian->pre;
						}
					}
				}
				else if (pTop->value < t_red.pMedian->value)
				{
					if (t_red.num % 2 == 1)
					{
						t_red.pMedian = t_red.pMedian->next;
					}
				}
				else
				{
					if (t_red.num % 2 == 0)
					{
						t_red.pMedian = t_red.pMedian->pre;
					}
				}
				if (pTop->next == NULL)
				{
					pTop->pre->next = NULL;
				}
				else if (pTop->pre == NULL)
				{
					pTop->next->pre = NULL;
				}
				else
				{
					pTop->pre->next = pTop->next;
					pTop->next->pre = pTop->pre;
				}
			}
		}
		pTNewMedianNode = pTop;
		pTNewMedianNode->next = NULL;
		pTNewMedianNode->pre = NULL;
		pTNewMedianNode->value = value;
	}

	t_red.value_q.push(pTNewMedianNode);
	if (::isnan(value))
	{
		if (t_red.pMedian == NULL)
		{
			return NAN;
		}
		if (t_red.num % 2 == 1)
		{
		   return t_red.pMedian->value;
		}
		else
		{
		   return (t_red.pMedian->value + t_red.pMedian->next->value) / 2.0;
		}
	}
	t_red.num += 1;
	if (t_red.pMedian == NULL)
	{
		t_red.pMedian = pTNewMedianNode;
		return value;
	}
	TMedianNode *pForeachNode = t_red.pMedian;
	if (value >= pForeachNode->value)
	{
		while (pForeachNode)
		{
			if (value < pForeachNode->value)
			{
				pTNewMedianNode->next = pForeachNode;
				pTNewMedianNode->pre = pForeachNode->pre;
				if(pForeachNode->pre == NULL){
					cout<<"001 pre = NULL..."<<endl;
				}
				pForeachNode->pre->next = pTNewMedianNode;
				pForeachNode->pre = pTNewMedianNode;

				if (t_red.num % 2 == 1) //如果从偶数变成奇数，Media移动一个
				{
					t_red.pMedian = t_red.pMedian->next;
				}
				break;
			}
			else
			{
				if (pForeachNode->next == NULL)
				{
					pForeachNode->next = pTNewMedianNode;
					pTNewMedianNode->pre = pForeachNode;
					if (t_red.num % 2 == 1) //如果从偶数变成奇数，Media移动一个
					{
						if(t_red.pMedian == NULL){
							cout<<"002 pMedian = NULL..."<<endl;
						}
						t_red.pMedian = t_red.pMedian->next;
					}
					break;
				}
				else
				{
					pForeachNode = pForeachNode->next;
				}
			}
		}
	}
	else
	{
		while (pForeachNode)
		{
			if (value > pForeachNode->value)
			{
				pTNewMedianNode->next = pForeachNode->next;
				pTNewMedianNode->pre = pForeachNode;
				if(pForeachNode->next == NULL){
					cout<<"003 next = NULL..."<<endl;
				}
				pForeachNode->next->pre = pTNewMedianNode;
				pForeachNode->next = pTNewMedianNode;
				/*pTNewMedianNode->next = pForeachNode;
				pTNewMedianNode->pre = pForeachNode->pre;
				pForeachNode->pre->next = pTNewMedianNode;
				pForeachNode->pre = pTNewMedianNode;*/
				if (t_red.num % 2 == 0) //如果从偶数变成奇数，Media移动一个
				{
					t_red.pMedian = t_red.pMedian->pre;
				}
				break;
			}
			else
			{
				if (pForeachNode->pre == NULL)
				{
					pTNewMedianNode->next = pForeachNode;
					pForeachNode->pre = pTNewMedianNode;
					if (t_red.num % 2 == 0) //如果从偶数变成奇数，Media移动一个
					{
						t_red.pMedian = t_red.pMedian->pre;
					}
					break;
				}
				else
				{
					pForeachNode = pForeachNode->pre;
				}
			}
		}
	}
	//print
	/*std::cout << "list is ";
	pForeachNode = t_red.pMedian;
	while (pForeachNode->pre != NULL)
	{
		//std::cout << pForeachNode->value << " ";
		pForeachNode = pForeachNode->pre;
	}
	//std::cout << "  TRT ";
	//pForeachNode = t_red.pMedian;
	while (pForeachNode != NULL)
	{
		std::cout << pForeachNode->value << " ";
		pForeachNode = pForeachNode->next;
	}
	std::cout << endl;

	pForeachNode = t_red.pMedian;
	while (pForeachNode->next != NULL)
	{
		//std::cout << pForeachNode->value << " ";
		pForeachNode = pForeachNode->next;
	}
	//std::cout << "  TRT ";
	//pForeachNode = t_red.pMedian;
	while (pForeachNode != NULL)
	{
		std::cout << pForeachNode->value << " ";
		pForeachNode = pForeachNode->pre;
	}
	std::cout << endl;*/
	if (t_red.pMedian == NULL)
	{
		return NAN;
	}
	if (t_red.num % 2 == 1)
	{
		return t_red.pMedian->value;
	}
	else
	{
		if(t_red.pMedian->next == NULL){
			cout<<"004 next = NULL..."<<endl;
		}
		return (t_red.pMedian->value + t_red.pMedian->next->value) / 2.0;
	}

}

//// add 20220809_beg

RollingSortedHelper::RollingSortedHelper(unsigned int window)
{
	window_ = window;
}
RollingSortedHelper::~RollingSortedHelper()
{

}

TMedianNode * RollingSortedHelper::cal_rolling_median(
	const double & value, const int & label)
{
	RollingMedianData &t_red = label_q_map_[label];
	TMedianNode *pTNewMedianNode = NULL;
	if (t_red.value_q.size() < window_)
	{
		pTNewMedianNode = new TMedianNode;
		if (pTNewMedianNode == NULL)
		{
			return NULL;
		}
		pTNewMedianNode->next = NULL;
		pTNewMedianNode->pre = NULL;
		pTNewMedianNode->value = value;
	}
	else
	{
		TMedianNode *pTop = t_red.value_q.front();
		t_red.value_q.pop();
		//std::cout << "pop " << pTop->value << endl;
		if (!::isnan(pTop->value))
		{
			t_red.num -= 1;
			if (pTop == t_red.pMedian)
			{
				if (t_red.num == 0)
				{
					t_red.pMedian = NULL;
				}
				else
				{
					if (t_red.num % 2 == 0)
					{
						if (t_red.pMedian->pre != NULL)
						{
							t_red.pMedian->pre->next = t_red.pMedian->next;
						}
						if (t_red.pMedian->next != NULL)
						{
							t_red.pMedian->next->pre = t_red.pMedian->pre;
						}
						t_red.pMedian = t_red.pMedian->pre;
					}
					else
					{
						if (t_red.pMedian->pre != NULL)
						{
							t_red.pMedian->pre->next = t_red.pMedian->next;
						}
						if (t_red.pMedian->next != NULL)
						{
							t_red.pMedian->next->pre = t_red.pMedian->pre;
						}
						t_red.pMedian = t_red.pMedian->next;
					}
				}
			}
			else
			{
				double zero_val = 1e-13;
				//判断当前数据是在median的左边还是右边
				TMedianNode *pForeach = pTop;
				if (fabs(pTop->value - t_red.pMedian->value) < zero_val)
				{
					pForeach = pTop->next;
					while (pForeach && fabs(pForeach->value - t_red.pMedian->value) < zero_val)
					{
						if (pForeach == t_red.pMedian)
						{
							if (t_red.num % 2 == 1)
							{
								t_red.pMedian = t_red.pMedian->next;
							}
							break;
						}
						pForeach = pForeach->next;
					}
					if (pForeach == NULL || fabs(pForeach->value - t_red.pMedian->value) > zero_val) //表示不等
					{
						if (t_red.num % 2 == 0)
						{
							t_red.pMedian = t_red.pMedian->pre;
						}
					}
				}
				else if (pTop->value < t_red.pMedian->value)
				{
					if (t_red.num % 2 == 1)
					{
						t_red.pMedian = t_red.pMedian->next;
					}
				}
				else
				{
					if (t_red.num % 2 == 0)
					{
						t_red.pMedian = t_red.pMedian->pre;
					}
				}
				if (pTop->next == NULL)
				{
					pTop->pre->next = NULL;
				}
				else if (pTop->pre == NULL)
				{
					pTop->next->pre = NULL;
				}
				else
				{
					pTop->pre->next = pTop->next;
					pTop->next->pre = pTop->pre;
				}
			}
		}
		pTNewMedianNode = pTop;
		pTNewMedianNode->next = NULL;
		pTNewMedianNode->pre = NULL;
		pTNewMedianNode->value = value;
	}

	t_red.value_q.push(pTNewMedianNode);
	if (::isnan(value))
	{
		if (t_red.pMedian == NULL)
		{
			return NULL;
		}
		return t_red.pMedian;
	}
	t_red.num += 1;
	if (t_red.pMedian == NULL)
	{
		t_red.pMedian = pTNewMedianNode;
		return t_red.pMedian;
	}
	TMedianNode *pForeachNode = t_red.pMedian;
	if (value >= pForeachNode->value)
	{
		while (pForeachNode)
		{
			if (value < pForeachNode->value)
			{
				pTNewMedianNode->next = pForeachNode;
				pTNewMedianNode->pre = pForeachNode->pre;
				pForeachNode->pre->next = pTNewMedianNode;
				pForeachNode->pre = pTNewMedianNode;

				if (t_red.num % 2 == 1) //如果从偶数变成奇数，Media移动一个
				{
					t_red.pMedian = t_red.pMedian->next;
				}
				break;
			}
			else
			{
				if (pForeachNode->next == NULL)
				{
					pForeachNode->next = pTNewMedianNode;
					pTNewMedianNode->pre = pForeachNode;
					if (t_red.num % 2 == 1) //如果从偶数变成奇数，Media移动一个
					{
						t_red.pMedian = t_red.pMedian->next;
					}
					break;
				}
				else
				{
					pForeachNode = pForeachNode->next;
				}
			}
		}
	}
	else
	{
		while (pForeachNode)
		{
			if (value > pForeachNode->value)
			{
				pTNewMedianNode->next = pForeachNode->next;
				pTNewMedianNode->pre = pForeachNode;
				pForeachNode->next->pre = pTNewMedianNode;
				pForeachNode->next = pTNewMedianNode;
				/*pTNewMedianNode->next = pForeachNode;
				pTNewMedianNode->pre = pForeachNode->pre;
				pForeachNode->pre->next = pTNewMedianNode;
				pForeachNode->pre = pTNewMedianNode;*/
				if (t_red.num % 2 == 0) //如果从偶数变成奇数，Media移动一个
				{
					t_red.pMedian = t_red.pMedian->pre;
				}
				break;
			}
			else
			{
				if (pForeachNode->pre == NULL)
				{
					pTNewMedianNode->next = pForeachNode;
					pForeachNode->pre = pTNewMedianNode;
					if (t_red.num % 2 == 0) //如果从偶数变成奇数，Media移动一个
					{
						t_red.pMedian = t_red.pMedian->pre;
					}
					break;
				}
				else
				{
					pForeachNode = pForeachNode->pre;
				}
			}
		}
	}
	//print
	/*std::cout << "list is ";
	pForeachNode = t_red.pMedian;
	while (pForeachNode->pre != NULL)
	{
		//std::cout << pForeachNode->value << " ";
		pForeachNode = pForeachNode->pre;
	}
	//std::cout << "  TRT ";
	//pForeachNode = t_red.pMedian;
	while (pForeachNode != NULL)
	{
		std::cout << pForeachNode->value << " ";
		pForeachNode = pForeachNode->next;
	}
	std::cout << endl;

	pForeachNode = t_red.pMedian;
	while (pForeachNode->next != NULL)
	{
		//std::cout << pForeachNode->value << " ";
		pForeachNode = pForeachNode->next;
	}
	//std::cout << "  TRT ";
	//pForeachNode = t_red.pMedian;
	while (pForeachNode != NULL)
	{
		std::cout << pForeachNode->value << " ";
		pForeachNode = pForeachNode->pre;
	}
	std::cout << endl;*/
	if (t_red.pMedian == NULL)
	{
		return NULL;
	}
	return t_red.pMedian;
}
//// add 20220809_end

RollingRankHelper::RollingRankHelper(unsigned int window)
{
	window_ = window;
}
RollingRankHelper::~RollingRankHelper()
{

}

double RollingRankHelper::cal_rolling_rank(double value, int label)
{
	RollingMedianData &t_red = label_q_map_[label];
	TMedianNode *pTNewMedianNode = NULL;
	if (t_red.value_q.size() < window_)
	{
		pTNewMedianNode = new TMedianNode;
		if (pTNewMedianNode == NULL)
		{
			return NAN;
		}
		pTNewMedianNode->next = NULL;
		pTNewMedianNode->pre = NULL;
		pTNewMedianNode->value = value;
	}
	else
	{
		TMedianNode *pTop = t_red.value_q.front();
		t_red.value_q.pop();
		//std::cout << "pop " << pTop->value << endl;
		if (!::isnan(pTop->value))
		{
			t_red.num -= 1;
			if (pTop == t_red.pMedian)
			{
				if (t_red.num == 0)
				{
					t_red.pMedian = NULL;
				}
				else
				{
					if (t_red.num % 2 == 0)
					{
						if (t_red.pMedian->pre != NULL)
						{
							t_red.pMedian->pre->next = t_red.pMedian->next;
						}
						if (t_red.pMedian->next != NULL)
						{
							t_red.pMedian->next->pre = t_red.pMedian->pre;
						}
						t_red.pMedian = t_red.pMedian->pre;
					}
					else
					{
						if (t_red.pMedian->pre != NULL)
						{
							t_red.pMedian->pre->next = t_red.pMedian->next;
						}
						if (t_red.pMedian->next != NULL)
						{
							t_red.pMedian->next->pre = t_red.pMedian->pre;
						}
						t_red.pMedian = t_red.pMedian->next;
					}
				}
			}
			else
			{
				double zero_val=1e-13;
			//判断当前数据是在median的左边还是右边
				TMedianNode *pForeach = pTop;
				if (fabs(pTop->value - t_red.pMedian->value) < zero_val)
				{
					pForeach = pTop->next;
					while (pForeach && fabs(pForeach->value - t_red.pMedian->value) < zero_val)
					{
						if (pForeach == t_red.pMedian)
						{
							if (t_red.num % 2 == 1)
							{
								t_red.pMedian = t_red.pMedian->next;
							}
							break;
						}
						pForeach = pForeach->next;
					}
					if (pForeach == NULL || fabs(pForeach->value - t_red.pMedian->value) > zero_val) //表示不等
					{
						if (t_red.num % 2 == 0)
						{
							t_red.pMedian = t_red.pMedian->pre;
						}
					}
				}
				else if (pTop->value < t_red.pMedian->value)
				{
					if (t_red.num % 2 == 1)
					{
						t_red.pMedian = t_red.pMedian->next;
					}
				}
				else
				{
					if (t_red.num % 2 == 0)
					{
						t_red.pMedian = t_red.pMedian->pre;
					}
				}
				if (pTop->next == NULL)
				{
					pTop->pre->next = NULL;
				}
				else if (pTop->pre == NULL)
				{
					pTop->next->pre = NULL;
				}
				else
				{
					pTop->pre->next = pTop->next;
					pTop->next->pre = pTop->pre;
				}
			}
		}
		pTNewMedianNode = pTop;
		pTNewMedianNode->next = NULL;
		pTNewMedianNode->pre = NULL;
		pTNewMedianNode->value = value;
	}

	t_red.value_q.push(pTNewMedianNode);
	if (::isnan(value))
	{
		return NAN;
	}
	t_red.num += 1;
	if (t_red.pMedian == NULL)
	{
		t_red.pMedian = pTNewMedianNode;
		return 0;
	}
	int count = 0;
	TMedianNode *pForeachNode = t_red.pMedian;
	if (value >= pForeachNode->value)
	{
		while (pForeachNode)
		{
			count += 1;
			if (value < pForeachNode->value)
			{
				pTNewMedianNode->next = pForeachNode;
				pTNewMedianNode->pre = pForeachNode->pre;
				pForeachNode->pre->next = pTNewMedianNode;
				pForeachNode->pre = pTNewMedianNode;

				if (t_red.num % 2 == 1) //如果从偶数变成奇数，Media移动一个
				{
					t_red.pMedian = t_red.pMedian->next;
				}
				break;
			}
			else
			{
				if (pForeachNode->next == NULL)
				{
					pForeachNode->next = pTNewMedianNode;
					pTNewMedianNode->pre = pForeachNode;
					if (t_red.num % 2 == 1) //如果从偶数变成奇数，Media移动一个
					{
						t_red.pMedian = t_red.pMedian->next;
					}
					break;
				}
				else
				{
					pForeachNode = pForeachNode->next;
				}
			}
		}
		//print
		/*std::cout << "list is ";
		pForeachNode = t_red.pMedian;
		while (pForeachNode->pre != NULL)
		{
			//std::cout << pForeachNode->value << " ";
			pForeachNode = pForeachNode->pre;
		}
		//std::cout << "  TRT ";
		//pForeachNode = t_red.pMedian;
		while (pForeachNode != NULL)
		{
			std::cout << pForeachNode->value << " ";
			pForeachNode = pForeachNode->next;
		}
		std::cout << endl;*/
		if (pTNewMedianNode->pre == NULL)
		{
			return 0;
		}

		else if (pTNewMedianNode->next == NULL)
		{
			return t_red.num - 1;
		}
		return (t_red.num) / 2 - 1 + count - 1;
	}
	else
	{
		while (pForeachNode)
		{
			count++;
			if (value > pForeachNode->value)
			{
				pTNewMedianNode->next = pForeachNode->next;
				pTNewMedianNode->pre = pForeachNode;
				pForeachNode->next->pre = pTNewMedianNode;
				pForeachNode->next = pTNewMedianNode;
				/*pTNewMedianNode->next = pForeachNode;
				pTNewMedianNode->pre = pForeachNode->pre;
				pForeachNode->pre->next = pTNewMedianNode;
				pForeachNode->pre = pTNewMedianNode;*/
				if (t_red.num % 2 == 0) //如果从偶数变成奇数，Media移动一个
				{
					t_red.pMedian = t_red.pMedian->pre;
				}
				break;
			}
			else
			{
				if (pForeachNode->pre == NULL)
				{
					count += 1;
					pTNewMedianNode->next = pForeachNode;
					pForeachNode->pre = pTNewMedianNode;
					if (t_red.num % 2 == 0) //如果从偶数变成奇数，Media移动一个
					{
						t_red.pMedian = t_red.pMedian->pre;
					}
					break;
				}
				else
				{
					pForeachNode = pForeachNode->pre;
				}
			}
		}

		//print
		/*std::cout << "list is ";
		pForeachNode = t_red.pMedian;
		while (pForeachNode->pre != NULL)
		{
			//std::cout << pForeachNode->value << " ";
			pForeachNode = pForeachNode->pre;
		}
		//std::cout << "  TRT ";
		//pForeachNode = t_red.pMedian;
		while (pForeachNode != NULL)
		{
			std::cout << pForeachNode->value << " ";
			pForeachNode = pForeachNode->next;
		}
		std::cout << endl;*/
		if (pTNewMedianNode->pre == NULL)
		{
			return 0;
		}
		return (t_red.num) / 2 - count + 1;
	}
}


///add 20220222
bool CommonFuncs::GetContractInfo(size_t code, int date, double &price_tick, double &multiply, bool isinit) {
	static char *p = NULL;
	static int cnt = 0;
	if (!isinit && NULL == p) {
		return false;
	}
	if (NULL == p) {
		p = (char*)code;
		cnt = *(int*)p;
		return true;
	}
	if (code > cnt) {
		return false;
	}
	size_t *pp = (size_t*)(p + 4);
	ContractInfoStruDLL *p1 = (ContractInfoStruDLL*)pp[code];
	ContractInfoStruDLL *pre = p1;
	while (NULL != p1) {
		if (p1->date > date) {
			price_tick = pre->price_tick;
			multiply = pre->multiply;
			return true;
		}
		else if (NULL == p1->next) {
			price_tick = p1->price_tick;
			multiply = p1->multiply;
			return true;
		}
		pre = p1;
		p1 = p1->next;
	}
	return false;
}

int CommonFuncs::GetIndustryCode(size_t code, bool isinit) {
	static char *p = NULL;
	static int cnt = 0;
	if (!isinit && NULL == p) {
		return -2;
	}
	if (NULL == p) {
		p = (char*)code;
		cnt = *(int*)p;
		return 0;
	}
	if (code > cnt) {
		return -1;
	}
	int *pp = (int*)(p + 4);
	return pp[code];
}

bool CommonFuncs::GetXYValue(size_t code, double &x_val, double &y_val) {
	static double *px = NULL;
	static double *py = NULL;
	if (x_val == 9527 && y_val == 9527) {
		char *p = (char*)code;
		px = *(double**)(p);
		py = *(double**)(p + 8);
		return true;
	}
	if (px == NULL || py == NULL || code > 800000) {
		return false;
	}
	x_val = *(px + code);
	y_val = *(py + code);
	return true;
}

///add 20220222 end
RollingTsstdNodmHelper::RollingTsstdNodmHelper(unsigned int window, bool nan_tsstd, bool unbiased) {
	window_ = window;
	nan_tsstd_ = nan_tsstd;
	unbiased_ = unbiased;
}

RollingTsstdNodmHelper::RollingTsstdNodmHelper(unsigned int window, bool nan_tsstd) {
	window_ = window;
	nan_tsstd_ = nan_tsstd;
	unbiased_ = true;
}

RollingTsstdNodmHelper::RollingTsstdNodmHelper(unsigned int window) {
	window_ = window;
	nan_tsstd_ = false;
	unbiased_ = true;
}

RollingTsstdNodmHelper::~RollingTsstdNodmHelper() {

}

double RollingTsstdNodmHelper::cal_rolling_tsstdnodm(double value, int label) {
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end()) {
		RollingStdData & t_rsd = label_q_map_[label];
		queue_ex<double> * t_value_q = new queue_ex<double>(window_);
		t_value_q->push(value);
		t_rsd.value_q = t_value_q;
		t_rsd.nan_num += ::isnan(value) ? 1 : 0;
		t_rsd.sum += nan_to_zero(value);
		t_rsd.square_sum = t_rsd.square_sum + nan_to_zero(value) * nan_to_zero(value);
	}
	else {
		RollingStdData *t_rsd = &label_q_iter->second;
		if (!t_rsd->value_q->full()) {
			t_rsd->value_q->push(value);
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum += nan_to_zero(value);
			t_rsd->square_sum = t_rsd->square_sum + nan_to_zero(value) * nan_to_zero(value);
			//label_q_map_[label] = t_rsd;
			if (t_rsd->value_q->full()) {
				if (window_ == t_rsd->nan_num) {
					return NAN;
				}
				double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num);
				if (variance < 0) {
					variance = 0;
				}
				//double t_mean = NAN;
				double t_std = NAN;
				if (nan_tsstd_ == true) {
					if (unbiased_ == true) {
						if (window_ == t_rsd->nan_num + 1) {
							return NAN;
						}
						t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - 1)));
					}
					else {
						if (window_ == t_rsd->nan_num) {
							return NAN;
						}
						t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num)));
					}
					if (window_ == t_rsd->nan_num) {
						return NAN;
					}
					//t_mean = round_double(t_rsd->sum / (window_ - t_rsd->nan_num));
				}
				else {
					if (t_rsd->nan_num == 0) {
						if (unbiased_ == true) {
							if (window_ == 1) {
								return NAN;
							}
							t_std = sqrt(round_double(variance / (window_ - 1)));
						}
						else {
							t_std = sqrt(round_double(variance / window_));
						}
						//t_mean = round_double(t_rsd->sum / window_);
					}
					else {
						return NAN;
					}
				}
				if (t_std > -1e-12 && t_std < 1e-12) {
					return NAN;
				}
				//return (value - t_mean) / t_std;
				return (value) / t_std;
			}
		}
		else {
			double tmp_pop_value = t_rsd->value_q->front();
			t_rsd->value_q->pop();
			t_rsd->value_q->push(value);
			t_rsd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum = t_rsd->sum - nan_to_zero(tmp_pop_value) + nan_to_zero(value);
			t_rsd->square_sum = t_rsd->square_sum - nan_to_zero(tmp_pop_value) * nan_to_zero(tmp_pop_value) + nan_to_zero(value) * nan_to_zero(value);
			//label_q_map_[label] = t_rsd;
			if (window_ == t_rsd->nan_num) {
				return NAN;
			}
			double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num);
			//double t_mean = NAN;
			double t_std = NAN;
			if (nan_tsstd_ == true) {
				if (unbiased_ == true) {
					if (window_ == t_rsd->nan_num + 1) {
						return NAN;
					}
					t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - 1)));
				}
				else {
					if (window_ == t_rsd->nan_num) {
						return NAN;
					}
					t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num)));
				}
				if (window_ == t_rsd->nan_num) {
					return NAN;
				}
				//t_mean = round_double(t_rsd->sum / (window_ - t_rsd->nan_num));
			}
			else {
				if (t_rsd->nan_num == 0) {
					if (unbiased_ == true) {
						if (window_ == 1) {
							return NAN;
						}
						t_std = sqrt(round_double(variance / (window_ - 1)));
					}
					else {
						t_std = sqrt(round_double(variance / window_));
					}
					//t_mean = round_double(t_rsd->sum / window_);
				}
				else {
					return NAN;
				}
			}
			if (t_std > -1e-12 && t_std < 1e-12) {
				return NAN;
			}
			//return (value - t_mean) / t_std;
			return value / t_std;
		}
	}
	return NAN;
}

RollingTruncStdData::RollingTruncStdData()
{
	square_sum = 0;
	sum = 0;
	nan_num = 0;
	value_q = nullptr;
	is_up_down = false;
}

RollingTruncStdData::~RollingTruncStdData()
{
	if (value_q) {
		delete value_q;
		value_q = nullptr;
	}
}

RollingTruncStdData & RollingTruncStdData::operator=(const RollingTruncStdData & rsd) {
	square_sum = rsd.square_sum;
	sum = rsd.sum;
	nan_num = rsd.nan_num;
	if (value_q) {
		delete value_q;
		value_q = nullptr;
	}
	if (rsd.value_q) {
		value_q = new queue_ex<double>(*rsd.value_q);
	}
	return *this;
}

RollingTruncStdHelper::RollingTruncStdHelper(unsigned int window, bool nan_std, bool unbiased)
{
	window_ = window;
	nan_std_ = nan_std;
	unbiased_ = unbiased;
}

RollingTruncStdHelper::RollingTruncStdHelper(unsigned int window, bool nan_std)
{
	window_ = window;
	nan_std_ = nan_std;
	unbiased_ = true;
}

RollingTruncStdHelper::RollingTruncStdHelper(unsigned int window)
{
	window_ = window;
	nan_std_ = true;
	unbiased_ = true;
}

RollingTruncStdHelper::~RollingTruncStdHelper()
{
}

double RollingTruncStdHelper::cal_rolling_trunc_tsstd(double value, int label, bool calc_upper_down)
{
	if (window_ < 10) {
		return NAN;
	}
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end()) {
		RollingTruncStdData & t_rsd = label_q_map_[label];
		queue_ex<double> * t_value_q = new queue_ex<double>(window_);
		t_value_q->push(value);
		t_rsd.value_q = t_value_q;
		return NAN;
	}
	else {
		RollingTruncStdData *t_rsd = &label_q_iter->second;
		if ((!calc_upper_down) && (!t_rsd->is_up_down)) {//还未产生upper_val和down_val 且当前bar不需要计算这两值
			if (t_rsd->value_q->full())
				t_rsd->value_q->pop();
			t_rsd->value_q->push(value);
			return NAN;
		}
		
		double v_std = 0;
		if (calc_upper_down) {//每天15:00截面重新计算upper_val和down_val
			if (t_rsd->value_q->full()) {
				t_rsd->value_q->pop();
			}
			t_rsd->value_q->push(value);

			if (!t_rsd->value_q->full()) {
				return NAN;
			}
			t_rsd->nan_num = 0;
			t_rsd->invalid_num = 0;
			t_rsd->square_sum = 0;
			t_rsd->sum = 0;
			size_t uh = t_rsd->value_q->get_head_index();
			size_t tail = t_rsd->value_q->get_tail_index();
			double * vet = t_rsd->value_q->get_data_vector();
			vector<double> tmpvec;
			tmpvec.reserve(window_);
			size_t _usize = window_ + 1;
			size_t pre_uh = uh;
			while (uh != tail)
			{
				auto & val = vet[uh];
				if (!(::isnan(val) || ::isinf(val))) {
					tmpvec.push_back(val);
				}
				uh = (uh + 1) % _usize;
			}
			std::sort(tmpvec.begin(), tmpvec.end());
			uint32_t tail_size = tmpvec.size()*0.01;//去掉首尾1%
			if (tail_size > 0) {
				t_rsd->down_val = tmpvec[tail_size - 1];//更新下限值
				t_rsd->upper_val = tmpvec[tmpvec.size() - tail_size];//更新上限值
			}
			else {
				t_rsd->down_val = *tmpvec.begin();
				t_rsd->upper_val = *tmpvec.rbegin();
			}
			t_rsd->is_up_down = true;
			uh = t_rsd->value_q->get_head_index();
			while (uh != tail)
			{
				auto & val = vet[uh];
				if (::isnan(val) || ::isinf(val)) {
					t_rsd->nan_num++;
				}
				else if (val <= t_rsd->down_val || val >= t_rsd->upper_val) {
					t_rsd->invalid_num++;
				}
				else {
					t_rsd->sum += val;
					t_rsd->square_sum += val * val;
				}
				uh = (uh + 1) % _usize;
			}
			double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num - t_rsd->invalid_num);
			if (variance < 0) {
				variance = 0;
			}
			if (nan_std_ == true) {
				if (unbiased_ == true) {
					if (window_ == t_rsd->nan_num + 1) {
						return NAN;
					}
					v_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - t_rsd->invalid_num - 1)));
				}
				else {
					if (window_ == t_rsd->nan_num) {
						return NAN;
					}
					v_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - t_rsd->invalid_num)));
				}
			}
			else {
				if (t_rsd->nan_num == 0) {
					if (unbiased_ == true) {
						if (window_ == 1) {
							return NAN;
						}
						v_std = sqrt(round_double(variance / (window_ - t_rsd->invalid_num - 1)));
					}
					else {
						v_std = sqrt(round_double(variance / window_ - t_rsd->invalid_num));
					}
				}
				else {
					return NAN;
				}
			}
			if (isinf(value) || ::isnan(value)) {
				return NAN;
			}
			else {
				return value / v_std;
			}
		}

		if (!t_rsd->is_up_down) {//还未产生upper_val和down_val 直接返回nan
			t_rsd->value_q->push(value);
			return NAN;
		}

		if (!t_rsd->value_q->full()) {//队列还未满， 则上限和下限一定还未产生
			t_rsd->value_q->push(value);
			return NAN;
		}
		else {
			double tmp_pop_value = t_rsd->value_q->front();
			if (::isnan(tmp_pop_value) || ::isinf(tmp_pop_value)) {
				t_rsd->nan_num -= 1;
			}
			else if (tmp_pop_value <= t_rsd->down_val || tmp_pop_value >= t_rsd->upper_val) {
				t_rsd->invalid_num -= 1;
			}
			else {
				t_rsd->sum -= tmp_pop_value;
				t_rsd->square_sum -= tmp_pop_value * tmp_pop_value;
			}
			t_rsd->value_q->pop();
			t_rsd->value_q->push(value);
			if (::isnan(value) || ::isinf(value)) {
				t_rsd->nan_num += 1;
				return NAN;
			}
			if (value <= t_rsd->down_val || value >= t_rsd->upper_val) {
				t_rsd->invalid_num += 1;
			}
			else {
				t_rsd->sum += value;
				t_rsd->square_sum += value * value;
			}
	
			if (window_ == t_rsd->nan_num) {
				return NAN;
			}
			double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num - t_rsd->invalid_num);
			if (variance < 0) {
				variance = 0;
			}
			if (nan_std_ == true) {
				if (unbiased_ == true) {
					if (window_ == t_rsd->nan_num + 1) {
						return NAN;
					}
					v_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - t_rsd->invalid_num - 1)));
				}
				else {
					if (window_ == t_rsd->nan_num) {
						return NAN;
					}
					v_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - t_rsd->invalid_num)));
				}
			}
			else {
				if (t_rsd->nan_num == 0) {
					if (unbiased_ == true) {
						if (window_ == 1) {
							return NAN;
						}
						v_std = sqrt(round_double(variance / (window_ - t_rsd->invalid_num - 1)));
					}
					else {
						v_std = sqrt(round_double(variance / (window_- t_rsd->invalid_num)));
					}
				}
				else {
					return NAN;
				}
			}
			return value / v_std;
		}
	}
	return NAN;
}