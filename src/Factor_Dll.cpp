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
struct ContractInfoStruDLL
{
	int date;
	double price_tick; // 最小价格
	double multiply;   // 合约乘数
	ContractInfoStruDLL *pre;
	ContractInfoStruDLL *next;
	ContractInfoStruDLL()
	{
		memset(this, 0, sizeof(ContractInfoStruDLL));
	}
};

RollingHelper::RollingHelper() {};

RollingHelper::~RollingHelper() {};

double RollingHelper::nan_to_zero(double value)
{
	if (::isnan(value))
	{
		return 0;
	}
	else
	{
		return value;
	}
}

double RollingHelper::inf_nan_to_zero(double value)
{
	if (::isnan(value) || ::isinf(value))
	{
		return 0;
	}
	else
	{
		return value;
	}
}

bool RollingHelper::nan_greater_equal(const double &a, const double &b)
{
	// nan >= nan true
	// 3 >= nan true
	// nan >= 3 false
	if (::isnan(b))
	{
		return true;
	}
	else if (::isnan(a))
	{
		return false;
	}
	else
	{
		return a - b >= -1e-8;
	}
}

bool RollingHelper::nan_greater(const double &a, const double &b)
{
	// nan > nan false
	// 3 > nan true
	// nan > 3 false
	if (::isnan(a))
	{
		return false;
	}
	else if (::isnan(b))
	{
		return true;
	}
	else
	{
		return a - b > 1e-8;
	}
}

bool RollingHelper::nan_less_equal(const double &a, const double &b)
{
	// nan <= nan true
	// 3 <= nan true
	// nan <= 3 false
	if (::isnan(b))
	{
		return true;
	}
	else if (::isnan(a))
	{
		return false;
	}
	else
	{
		return a - b <= 1e-8;
	}
}

bool RollingHelper::nan_less(const double &a, const double &b)
{
	// nan < nan false
	// 3 < nan true
	// nan < 3 false
	if (::isnan(a))
	{
		return false;
	}
	else if (::isnan(b))
	{
		return true;
	}
	else
	{
		return a - b < -1e-8;
	}
}

double RollingHelper::round_double(double value)
{
	return floor(value * (double)1e12 + 0.5) / (double)1e12;
}
ShiftHelper::ShiftHelper(unsigned int window)
{
	window_ = window;
}

ShiftHelper::~ShiftHelper()
{
	for (auto itor : label_q_map_)
	{
		if (itor.second)
		{
			delete itor.second;
			itor.second = nullptr;
		}
	}
	label_q_map_.clear();
}

double ShiftHelper::cal_shift(double value, int label)
{
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end())
	{
		// label_q_map_[label] =
		queue_ex<double> *label_q = new queue_ex<double>(window_);
		label_q->push(value);
		label_q_map_[label] = label_q;
	}
	else
	{
		queue_ex<double> *label_q = label_q_iter->second;
		if (!label_q->full())
		{
			label_q->push(value);
			// label_q_map_[label] = label_q;
		}
		else
		{
			double tmp_pop_value = label_q->front();
			label_q->pop();
			label_q->push(value);
			// label_q_map_[label] = label_q;
			return tmp_pop_value;
		}
	}
	return NAN;
}

RollingFuncHelper::RollingFuncHelper(unsigned int window)
{
	window_ = window;
}

RollingFuncHelper::~RollingFuncHelper() {};

list<double> *RollingFuncHelper::get_rolling_list(double value, int label)
{
	auto label_l_iter = label_l_map_.find(label);
	if (label_l_iter == label_l_map_.end())
	{
		list<double> &l = label_l_map_[label];
		l.push_back(value);
		return &l;
	}
	else
	{
		list<double> *label_l = &label_l_iter->second;
		if (label_l->size() < window_)
		{
			label_l->push_back(value);
			// label_l_map_[label] = label_l;
		}
		else
		{
			label_l->erase(label_l->begin());
			label_l->push_back(value);
			// label_l_map_[label] = label_l;
		}
		return label_l;
	}
}

RollingFuncHelper2::RollingFuncHelper2(unsigned int window)
{
	window_ = window;
}

RollingFuncHelper2::~RollingFuncHelper2()
{
	for (auto &itor : label_l_map_)
	{
		delete itor.second;
	}
	label_l_map_.clear();
}

void RollingFuncHelper2::get_rolling_max_min(double value, int label, double &max_val, double &min_val)
{
	auto label_l_iter = label_l_map_.find(label);
	if (label_l_iter == label_l_map_.end())
	{
		auto *p_q = new queue_ex<double>(window_);
		p_q->push(value);
		max_map_[label] = value;
		min_map_[label] = value;
		max_val = value;
		min_val = value;
		label_l_map_[label] = p_q;
	}
	else
	{
		double &min_ = min_map_[label];
		double &max_ = max_map_[label];
		auto *label_l = label_l_iter->second;
		if (!label_l->full())
		{
			label_l->push(value);
			// label_l_map_[label] = label_l;
			if (value < min_)
			{
				min_ = value;
			}
			if (value > max_)
			{
				max_ = value;
			}
		}
		else
		{
			double front_val = label_l->front();
			label_l->pop();
			// label_l_map_[label] = label_l;
			if (value < min_)
			{
				min_ = value;
				if (abs(front_val - max_) < 0.0001)
				{
					max_ = value;
					size_t head = label_l->_uhead;
					size_t tail = label_l->_utail;
					size_t usize = label_l->_usize;
					double *vet = label_l->value_q;
					while (head != tail)
					{
						if (vet[head] > max_)
						{
							max_ = vet[head];
						}
						head = (head + 1) % usize;
					}
				}
			}
			else if (value > max_)
			{
				max_ = value;
				if (abs(front_val - min_) < 0.0001)
				{
					min_ = value;
					size_t head = label_l->_uhead;
					size_t tail = label_l->_utail;
					size_t usize = label_l->_usize;
					double *vet = label_l->value_q;
					while (head != tail)
					{
						if (vet[head] < min_)
						{
							min_ = vet[head];
						}
						head = (head + 1) % usize;
					}
				}
			}
			else
			{
				if (abs(front_val - max_) < 0.0001)
				{
					max_ = value;
					size_t head = label_l->_uhead;
					size_t tail = label_l->_utail;
					size_t usize = label_l->_usize;
					double *vet = label_l->value_q;
					while (head != tail)
					{
						if (vet[head] > max_)
						{
							max_ = vet[head];
						}
						head = (head + 1) % usize;
					}
				}
				if (abs(front_val - min_) < 0.0001)
				{
					min_ = value;
					size_t head = label_l->_uhead;
					size_t tail = label_l->_utail;
					size_t usize = label_l->_usize;
					double *vet = label_l->value_q;
					while (head != tail)
					{
						if (vet[head] < min_)
						{
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

queue_ex<double> *RollingFuncHelper2::get_rolling_list(double value, int label)
{
	auto label_l_iter = label_l_map_.find(label);
	if (label_l_iter == label_l_map_.end())
	{
		auto *p_q = new queue_ex<double>(window_);
		p_q->push(value);
		label_l_map_[label] = p_q;
		return p_q;
	}
	else
	{
		auto *label_l = label_l_iter->second;
		if (!label_l->full())
		{
			label_l->push(value);
			// label_l_map_[label] = label_l;
		}
		else
		{
			label_l->pop();
			label_l->push(value);
			// label_l_map_[label] = label_l;
		}
		return label_l;
	}
}

RollingSumData::RollingSumData()
{
	sum = 0;
	nan_num = 0;
	value_q = nullptr;
}

RollingSumData::~RollingSumData()
{
	if (value_q)
	{
		delete value_q;
		value_q = nullptr;
	}
}

RollingSumData::RollingSumData(const RollingSumData &rsd)
{
	this->nan_num = rsd.nan_num;
	this->sum = rsd.sum;
	if (value_q)
	{
		delete value_q;
		value_q = nullptr;
	}

	if (rsd.value_q)
	{
		value_q = new queue_ex<double>(*rsd.value_q);
	}
}

RollingSumData &RollingSumData::operator=(const RollingSumData &rsd)
{
	this->nan_num = rsd.nan_num;
	this->sum = rsd.sum;
	if (value_q)
	{
		delete value_q;
		value_q = nullptr;
	}
	if (rsd.value_q)
	{
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
	if (label_q_iter == label_q_map_.end())
	{
		RollingSumData &t_rsd = label_q_map_[label];
		t_rsd.value_q = new queue_ex<double>(window_);
		t_rsd.value_q->push(value);
		t_rsd.nan_num += ::isnan(value) ? 1 : 0;
		t_rsd.sum += nan_to_zero(value);
	}
	else
	{
		RollingSumData &t_rsd = label_q_iter->second;
		if (!t_rsd.value_q->full())
		{
			t_rsd.value_q->push(value);
			t_rsd.nan_num += ::isnan(value) ? 1 : 0;
			t_rsd.sum += nan_to_zero(value);
			if (t_rsd.value_q->full())
			{
				if (nan_sum_ == true)
				{
					return round_double(t_rsd.sum);
				}
				else
				{
					return t_rsd.nan_num == 0 ? round_double(t_rsd.sum) : NAN;
				}
			}
		}
		else
		{
			double tmp_pop_value = t_rsd.value_q->front();
			t_rsd.value_q->pop();
			t_rsd.value_q->push(value);
			t_rsd.nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rsd.nan_num += ::isnan(value) ? 1 : 0;
			t_rsd.sum = t_rsd.sum - nan_to_zero(tmp_pop_value) + nan_to_zero(value);
			// label_q_map_[label] = t_rsd;
			if (nan_sum_ == true)
			{
				return round_double(t_rsd.sum);
			}
			else
			{
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
	if (value_q)
	{
		delete value_q;
		value_q = nullptr;
	}
}

RollingStdData &RollingStdData::operator=(const RollingStdData &rsd)
{
	square_sum = rsd.square_sum;
	sum = rsd.sum;
	nan_num = rsd.nan_num;
	if (value_q)
	{
		delete value_q;
		value_q = nullptr;
	}
	if (rsd.value_q)
	{
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

RollingStdHelper::~RollingStdHelper() {};

double RollingStdHelper::cal_rolling_std(double value, int label)
{
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end())
	{
		RollingStdData &t_rsd = label_q_map_[label];
		queue_ex<double> *t_value_q = new queue_ex<double>(window_);
		t_value_q->push(value);
		t_rsd.value_q = t_value_q;
		t_rsd.nan_num += ::isnan(value) ? 1 : 0;
		t_rsd.sum += nan_to_zero(value);
		t_rsd.square_sum = t_rsd.square_sum + nan_to_zero(value) * nan_to_zero(value);
	}
	else
	{
		RollingStdData *t_rsd = &label_q_iter->second;
		if (!t_rsd->value_q->full())
		{
			t_rsd->value_q->push(value);
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum += nan_to_zero(value);
			t_rsd->square_sum = t_rsd->square_sum + nan_to_zero(value) * nan_to_zero(value);
			// label_q_map_[label] = t_rsd;
			if (t_rsd->value_q->full())
			{
				if (window_ == t_rsd->nan_num)
				{
					return NAN;
				}
				double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num);
				if (variance < 0)
				{
					variance = 0;
				}
				if (nan_std_ == true)
				{
					if (unbiased_ == true)
					{
						if (window_ == t_rsd->nan_num + 1)
						{
							return NAN;
						}
						return sqrt(round_double(variance / (window_ - t_rsd->nan_num - 1)));
					}
					else
					{
						if (window_ == t_rsd->nan_num)
						{
							return NAN;
						}
						return sqrt(round_double(variance / (window_ - t_rsd->nan_num)));
					}
				}
				else
				{
					if (t_rsd->nan_num == 0)
					{
						if (unbiased_ == true)
						{
							if (window_ == 1)
							{
								return NAN;
							}
							return sqrt(round_double(variance / (window_ - 1)));
						}
						else
						{
							return sqrt(round_double(variance / window_));
						}
					}
					else
					{
						return NAN;
					}
				}
			}
		}
		else
		{
			double tmp_pop_value = t_rsd->value_q->front();
			t_rsd->value_q->pop();
			t_rsd->value_q->push(value);
			t_rsd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum = t_rsd->sum - nan_to_zero(tmp_pop_value) + nan_to_zero(value);
			t_rsd->square_sum = t_rsd->square_sum - nan_to_zero(tmp_pop_value) * nan_to_zero(tmp_pop_value) + nan_to_zero(value) * nan_to_zero(value);
			// label_q_map_[label] = t_rsd;
			if (window_ == t_rsd->nan_num)
			{
				return NAN;
			}
			double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num);
			if (variance < 0)
			{
				variance = 0;
			}
			if (nan_std_ == true)
			{
				if (unbiased_ == true)
				{
					if (window_ == t_rsd->nan_num + 1)
					{
						return NAN;
					}
					return sqrt(round_double(variance / (window_ - t_rsd->nan_num - 1)));
				}
				else
				{
					if (window_ == t_rsd->nan_num)
					{
						return NAN;
					}
					return sqrt(round_double(variance / (window_ - t_rsd->nan_num)));
				}
			}
			else
			{
				if (t_rsd->nan_num == 0)
				{
					if (unbiased_ == true)
					{
						if (window_ == 1)
						{
							return NAN;
						}
						return sqrt(round_double(variance / (window_ - 1)));
					}
					else
					{
						return sqrt(round_double(variance / window_));
					}
				}
				else
				{
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

RollingCorrData::~RollingCorrData()
{
	if (value_q1)
	{
		delete value_q1;
		value_q1 = nullptr;
	}

	if (value_q2)
	{
		delete value_q2;
		value_q2 = nullptr;
	}
}
RollingCorrData &RollingCorrData::operator=(const RollingCorrData &rcd)
{
	square_sum1 = rcd.square_sum1;
	square_sum2 = rcd.square_sum2;
	sum1 = rcd.sum1;
	sum2 = rcd.sum2;
	sum12 = rcd.sum12;
	nan_num = rcd.nan_num;
	if (value_q1)
	{
		delete value_q1;
		value_q1 = nullptr;
	}

	if (value_q2)
	{
		delete value_q2;
		value_q2 = nullptr;
	}
	if (rcd.value_q1)
		value_q1 = new queue_ex<double>(*rcd.value_q1);
	if (rcd.value_q2)
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

RollingCorrHelper::~RollingCorrHelper() {};

double RollingCorrHelper::cal_rolling_corr(double value1, double value2, int label)
{
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end())
	{
		RollingCorrData t_rcd;
		queue_ex<double> *t_value_q1 = new queue_ex<double>(window_);
		queue_ex<double> *t_value_q2 = new queue_ex<double>(window_);
		t_value_q1->push(value1);
		t_value_q2->push(value2);
		t_rcd.value_q1 = t_value_q1;
		t_rcd.value_q2 = t_value_q2;
		bool tmp_nan_flag = (::isnan(value1) || ::isnan(value2));
		t_rcd.nan_num += tmp_nan_flag ? 1 : 0;
		if (tmp_nan_flag == false)
		{
			t_rcd.sum1 += value1;
			t_rcd.sum2 += value2;
			t_rcd.sum12 += value1 * value2;
			t_rcd.square_sum1 += value1 * value1;
			t_rcd.square_sum2 += value2 * value2;
		}
		label_q_map_[label] = t_rcd;
	}
	else
	{
		RollingCorrData *t_rcd = &(label_q_iter->second);
		if (!t_rcd->value_q1->full())
		{
			t_rcd->value_q1->push(value1);
			t_rcd->value_q2->push(value2);
			bool tmp_nan_flag = (::isnan(value1) || ::isnan(value2));
			t_rcd->nan_num += tmp_nan_flag ? 1 : 0;
			if (tmp_nan_flag == false)
			{
				t_rcd->sum1 += value1;
				t_rcd->sum2 += value2;
				t_rcd->sum12 += value1 * value2;
				t_rcd->square_sum1 += value1 * value1;
				t_rcd->square_sum2 += value2 * value2;
			}
			if (t_rcd->value_q1->full())
			{
				unsigned int value_num = window_ - t_rcd->nan_num;
				if (value_num == 0)
				{
					return NAN;
				}
				else
				{
					if ((nan_corr_ == true) || (t_rcd->nan_num == 0))
					{
						double variance1 = round_double((t_rcd->square_sum1 - t_rcd->sum1 * t_rcd->sum1 / value_num) / value_num);
						double variance2 = round_double((t_rcd->square_sum2 - t_rcd->sum2 * t_rcd->sum2 / value_num) / value_num);
						if ((variance1 < 1e-12) || (variance2 < 1e-12))
						{
							return NAN;
						}
						else
						{
							double std1 = sqrt(variance1);
							double std2 = sqrt(variance2);
							double mean1 = t_rcd->sum1 / value_num;
							double mean2 = t_rcd->sum2 / value_num;
							double mean12 = t_rcd->sum12 / value_num;
							double result = round_double((mean12 - mean1 * mean2) / round_double((std1 * std2)));
							if (result >= 1.0001 || result <= -1.0001)
							{
								return NAN;
							}
							return result;
						}
					}
					else
					{
						return NAN;
					}
				}
			}
		}
		else
		{
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
			if (tmp_pop_nan_flag == false)
			{
				t_rcd->sum1 -= tmp_pop_value1;
				t_rcd->sum2 -= tmp_pop_value2;
				t_rcd->sum12 -= tmp_pop_value1 * tmp_pop_value2;
				t_rcd->square_sum1 -= tmp_pop_value1 * tmp_pop_value1;
				t_rcd->square_sum2 -= tmp_pop_value2 * tmp_pop_value2;
			}
			if (tmp_nan_flag == false)
			{
				t_rcd->sum1 += value1;
				t_rcd->sum2 += value2;
				t_rcd->sum12 += value1 * value2;
				t_rcd->square_sum1 += value1 * value1;
				t_rcd->square_sum2 += value2 * value2;
			}
			unsigned int value_num = window_ - t_rcd->nan_num;
			if (value_num == 0)
			{
				return NAN;
			}
			else
			{
				if ((nan_corr_ == true) || (t_rcd->nan_num == 0))
				{
					double variance1 = round_double((t_rcd->square_sum1 - t_rcd->sum1 * t_rcd->sum1 / value_num) / value_num);
					double variance2 = round_double((t_rcd->square_sum2 - t_rcd->sum2 * t_rcd->sum2 / value_num) / value_num);
					if ((variance1 < 1e-12) || (variance2 < 1e-12))
					{
						return NAN;
					}
					else
					{
						double std1 = sqrt(variance1);
						double std2 = sqrt(variance2);
						double mean1 = t_rcd->sum1 / value_num;
						double mean2 = t_rcd->sum2 / value_num;
						double mean12 = t_rcd->sum12 / value_num;
						double result = round_double((mean12 - mean1 * mean2) / round_double((std1 * std2)));
						if (result >= 1.0001 || result <= -1.0001)
						{
							return NAN;
						}
						return result;
					}
				}
				else
				{
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

RollingMaxHelper::~RollingMaxHelper() {};

double RollingMaxHelper::cal_rolling_max(double value, int label)
{
	auto label_l_iter = label_l_map_.find(label);
	if (label_l_iter == label_l_map_.end())
	{
		RollingMaxData &t_rmd = label_l_map_[label];
		queue_ex<double> *t_value_l = new queue_ex<double>(window_);
		t_value_l->push(value);
		t_rmd.value_l = t_value_l;
		t_rmd.max_value_index = window_;
		t_rmd.max_value = value;
		t_rmd.nan_num += ::isnan(value) ? 1 : 0;
	}
	else
	{
		RollingMaxData *t_rmd = &(label_l_iter->second);
		if (!t_rmd->value_l->full())
		{
			t_rmd->value_l->push(value);
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			if (nan_greater_equal(value, t_rmd->max_value))
			{
				t_rmd->max_value = value;
				t_rmd->max_value_index = window_;
			}
			else
			{
				t_rmd->max_value_index -= 1;
			}
			if (t_rmd->value_l->full())
			{
				if ((nan_max_ == true) || (t_rmd->nan_num == 0))
				{
					return t_rmd->max_value;
				}
				else
				{
					return NAN;
				}
			}
		}
		else
		{
			double tmp_pop_value = t_rmd->value_l->front();
			t_rmd->value_l->pop();
			t_rmd->value_l->push(value);
			t_rmd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			if (nan_greater_equal(value, t_rmd->max_value))
			{
				t_rmd->max_value = value;
				t_rmd->max_value_index = window_;
			}
			else
			{
				t_rmd->max_value_index -= 1;
				if (t_rmd->max_value_index == 0)
				{
					double tmp_max_value;
					unsigned int tmp_max_index;
					unsigned int tmp_iter_index = 1;
					size_t uh = t_rmd->value_l->get_head_index();
					size_t tail = t_rmd->value_l->get_tail_index();
					double *vet = t_rmd->value_l->get_data_vector();
					size_t _usize = window_ + 1;
					size_t pre_uh = uh;
					while (uh != tail)
					{
						auto &val = vet[uh];
						if (tmp_iter_index == 1)
						{
							tmp_max_value = val;
							tmp_max_index = tmp_iter_index;
						}
						else
						{
							if (nan_greater_equal(val, tmp_max_value))
							{
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
			if ((nan_max_ == true) || (t_rmd->nan_num == 0))
			{
				return t_rmd->max_value;
			}
			else
			{
				return NAN;
			}
		}
	}
	return NAN;
}

RollingMaxData::RollingMaxData()
{
	max_value = NAN;
	max_value_index = 0;
	nan_num = 0;
	value_l = nullptr;
}

RollingMaxData::~RollingMaxData()
{
	if (value_l)
	{
		delete value_l;
		value_l = nullptr;
	}
}

RollingMaxData &RollingMaxData::operator=(const RollingMaxData &rmd)
{
	max_value = rmd.max_value;
	max_value_index = rmd.max_value_index;
	nan_num = rmd.nan_num;
	if (value_l)
	{
		delete value_l;
		value_l = nullptr;
	}

	if (rmd.value_l)
	{
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

RollingArgmaxHelper::~RollingArgmaxHelper() {};

double RollingArgmaxHelper::cal_rolling_argmax(double value, int label)
{
	auto label_l_iter = label_l_map_.find(label);
	if (label_l_iter == label_l_map_.end())
	{
		RollingMaxData &t_rmd = label_l_map_[label];
		t_rmd.value_l = new queue_ex<double>(window_);
		t_rmd.value_l->push(value);
		t_rmd.max_value_index = window_;
		t_rmd.max_value = value;
		t_rmd.nan_num += ::isnan(value) ? 1 : 0;
	}
	else
	{
		RollingMaxData *t_rmd = &(label_l_iter->second);
		if (!t_rmd->value_l->full())
		{
			t_rmd->value_l->push(value);
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			bool greater_flag = first_argmax_ ? nan_greater(value, t_rmd->max_value) : nan_greater_equal(value, t_rmd->max_value);
			if (greater_flag)
			{
				t_rmd->max_value = value;
				t_rmd->max_value_index = window_;
			}
			else
			{
				t_rmd->max_value_index -= 1;
			}
			if (t_rmd->value_l->full())
			{
				if ((nan_argmax_ == true) || (t_rmd->nan_num == 0))
				{
					return t_rmd->max_value_index - 1;
				}
				else
				{
					return NAN;
				}
			}
		}
		else
		{
			double tmp_pop_value = t_rmd->value_l->front();
			t_rmd->value_l->pop();
			t_rmd->value_l->push(value);
			t_rmd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			bool greater_flag = first_argmax_ ? nan_greater(value, t_rmd->max_value) : nan_greater_equal(value, t_rmd->max_value);
			if (greater_flag)
			{
				t_rmd->max_value = value;
				t_rmd->max_value_index = window_;
			}
			else
			{
				t_rmd->max_value_index -= 1;
				if (t_rmd->max_value_index == 0)
				{
					double tmp_max_value;
					unsigned int tmp_max_index;
					unsigned int tmp_iter_index = 1;
					size_t uh = t_rmd->value_l->get_head_index();
					size_t tail = t_rmd->value_l->get_tail_index();
					double *vet = t_rmd->value_l->get_data_vector();
					size_t _usize = window_ + 1;
					size_t pre_uh = uh;
					while (uh != tail)
					{
						auto &val = vet[uh];
						if (tmp_iter_index == 1)
						{
							tmp_max_value = val;
							tmp_max_index = tmp_iter_index;
						}
						else
						{
							bool greater_flag = first_argmax_ ? nan_greater(val, tmp_max_value) : nan_greater_equal(val, tmp_max_value);
							if (greater_flag)
							{
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
			if ((nan_argmax_ == true) || (t_rmd->nan_num == 0))
			{
				return t_rmd->max_value_index - 1;
			}
			else
			{
				return NAN;
			}
		}
	}
	return NAN;
}

RollingMinData::RollingMinData()
{
	min_value = NAN;
	min_value_index = 0;
	nan_num = 0;
	value_l = nullptr;
}

RollingMinData::~RollingMinData()
{
	if (value_l)
		delete value_l;
	value_l = nullptr;
}

RollingMinData &RollingMinData::operator=(const RollingMinData &rmd)
{
	min_value = rmd.min_value;
	min_value_index = rmd.min_value_index;
	nan_num = rmd.nan_num;
	if (value_l)
	{
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

RollingMinHelper::~RollingMinHelper() {};

double RollingMinHelper::cal_rolling_min(double value, int label)
{
	auto label_l_iter = label_l_map_.find(label);
	if (label_l_iter == label_l_map_.end())
	{
		RollingMinData &t_rmd = label_l_map_[label];
		t_rmd.value_l = new queue_ex<double>(window_);
		t_rmd.value_l->push(value);
		t_rmd.min_value_index = window_;
		t_rmd.min_value = value;
		t_rmd.nan_num += ::isnan(value) ? 1 : 0;
	}
	else
	{
		RollingMinData *t_rmd = &(label_l_iter->second);
		if (!t_rmd->value_l->full())
		{
			t_rmd->value_l->push(value);
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			if (nan_less_equal(value, t_rmd->min_value))
			{
				t_rmd->min_value = value;
				t_rmd->min_value_index = window_;
			}
			else
			{
				t_rmd->min_value_index -= 1;
			}
			if (t_rmd->value_l->full())
			{
				if ((nan_min_ == true) || (t_rmd->nan_num == 0))
				{
					return t_rmd->min_value;
				}
				else
				{
					return NAN;
				}
			}
		}
		else
		{
			double tmp_pop_value = t_rmd->value_l->front();
			t_rmd->value_l->pop();
			t_rmd->value_l->push(value);
			t_rmd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			if (nan_less_equal(value, t_rmd->min_value))
			{
				t_rmd->min_value = value;
				t_rmd->min_value_index = window_;
			}
			else
			{
				t_rmd->min_value_index -= 1;
				if (t_rmd->min_value_index == 0)
				{
					double tmp_min_value;
					unsigned int tmp_min_index;
					unsigned int tmp_iter_index = 1;
					size_t uh = t_rmd->value_l->get_head_index();
					size_t tail = t_rmd->value_l->get_tail_index();
					double *vet = t_rmd->value_l->get_data_vector();
					size_t _usize = window_ + 1;
					size_t pre_uh = uh;
					while (uh != tail)
					{
						auto &val = vet[uh];
						if (tmp_iter_index == 1)
						{
							tmp_min_value = val;
							tmp_min_index = tmp_iter_index;
						}
						else
						{
							if (nan_less_equal(val, tmp_min_value))
							{
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
			if ((nan_min_ == true) || (t_rmd->nan_num == 0))
			{
				return t_rmd->min_value;
			}
			else
			{
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

RollingArgminHelper::~RollingArgminHelper() {};

double RollingArgminHelper::cal_rolling_argmin(double value, int label)
{
	auto label_l_iter = label_l_map_.find(label);
	if (label_l_iter == label_l_map_.end())
	{
		RollingMinData &t_rmd = label_l_map_[label];
		t_rmd.value_l = new queue_ex<double>(window_);
		t_rmd.value_l->push(value);
		t_rmd.min_value_index = window_;
		t_rmd.min_value = value;
		t_rmd.nan_num += ::isnan(value) ? 1 : 0;
	}
	else
	{
		RollingMinData *t_rmd = &(label_l_iter->second);
		if (!t_rmd->value_l->full())
		{
			t_rmd->value_l->push(value);
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			bool less_flag = first_argmin_ ? nan_less(value, t_rmd->min_value) : nan_less_equal(value, t_rmd->min_value);
			if (less_flag)
			{
				t_rmd->min_value = value;
				t_rmd->min_value_index = window_;
			}
			else
			{
				t_rmd->min_value_index -= 1;
			}
			if (t_rmd->value_l->full())
			{
				if ((nan_argmin_ == true) || (t_rmd->nan_num == 0))
				{
					return t_rmd->min_value_index - 1;
				}
				else
				{
					return NAN;
				}
			}
		}
		else
		{
			double tmp_pop_value = t_rmd->value_l->front();
			t_rmd->value_l->pop();
			t_rmd->value_l->push(value);
			t_rmd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rmd->nan_num += ::isnan(value) ? 1 : 0;
			bool less_flag = first_argmin_ ? nan_less(value, t_rmd->min_value) : nan_less_equal(value, t_rmd->min_value);
			if (less_flag)
			{
				t_rmd->min_value = value;
				t_rmd->min_value_index = window_;
			}
			else
			{
				t_rmd->min_value_index -= 1;
				if (t_rmd->min_value_index == 0)
				{
					double tmp_min_value;
					unsigned int tmp_min_index;
					unsigned int tmp_iter_index = 1;
					size_t uh = t_rmd->value_l->get_head_index();
					size_t tail = t_rmd->value_l->get_tail_index();
					double *vet = t_rmd->value_l->get_data_vector();
					size_t _usize = window_ + 1;
					size_t pre_uh = uh;
					while (uh != tail)
					{
						auto &val = vet[uh];
						if (tmp_iter_index == 1)
						{
							tmp_min_value = val;
							tmp_min_index = tmp_iter_index;
						}
						else
						{
							bool less_flag = first_argmin_ ? nan_less(val, tmp_min_value) : nan_less_equal(val, tmp_min_value);
							if (less_flag)
							{
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
			if ((nan_argmin_ == true) || (t_rmd->nan_num == 0))
			{
				return t_rmd->min_value_index - 1;
			}
			else
			{
				return NAN;
			}
		}
	}
	return NAN;
}

FillNANHelper::FillNANHelper()
{
}

FillNANHelper::~FillNANHelper()
{
}

double FillNANHelper::cal_fillnan_value(double value, int label)
{
	if (value_map_.find(label) == value_map_.end())
	{
		if (::isnan(value))
		{
			return NAN;
		}
		else
		{
			value_map_[label] = value;
			return value;
		}
	}
	else
	{
		if (::isnan(value))
		{
			return value_map_[label];
		}
		else
		{
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

RollingMeanHelper::~RollingMeanHelper() {};

double RollingMeanHelper::cal_rolling_mean(double value, int label)
{
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end())
	{
		RollingSumData &t_rsd = label_q_map_[label];
		t_rsd.value_q = new queue_ex<double>(window_);
		t_rsd.value_q->push(value);
		t_rsd.nan_num += ::isnan(value) ? 1 : 0;
		t_rsd.sum += nan_to_zero(value);
	}
	else
	{
		RollingSumData &t_rsd = label_q_iter->second;
		if (!t_rsd.value_q->full())
		{
			t_rsd.value_q->push(value);
			t_rsd.nan_num += ::isnan(value) ? 1 : 0;
			t_rsd.sum += nan_to_zero(value);
			// label_q_map_[label] = t_rsd;
			if (t_rsd.value_q->full())
			{
				if (nan_mean_ == true)
				{
					return window_ > t_rsd.nan_num ? round_double(t_rsd.sum / (window_ - t_rsd.nan_num))
												   : NAN;
				}
				else
				{
					return t_rsd.nan_num == 0 ? round_double(t_rsd.sum / window_) : NAN;
				}
			}
		}
		else
		{
			double tmp_pop_value = t_rsd.value_q->front();
			t_rsd.value_q->pop();
			t_rsd.value_q->push(value);
			t_rsd.nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rsd.nan_num += ::isnan(value) ? 1 : 0;
			t_rsd.sum = t_rsd.sum - nan_to_zero(tmp_pop_value) + nan_to_zero(value);
			// label_q_map_[label] = t_rsd;
			if (nan_mean_ == true)
			{
				return window_ > t_rsd.nan_num ? round_double(t_rsd.sum / (window_ - t_rsd.nan_num))
											   : NAN;
			}
			else
			{
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

RollingMeanHelperPre::~RollingMeanHelperPre() {};
RollingSumDataPre::RollingSumDataPre()
{
	sum = 0;
	nan_num = 0;
}
double RollingMeanHelperPre::cal_rolling_mean(double value, int label)
{
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end())
	{
		RollingSumDataPre &t_rsd = label_q_map_[label];
		queue<double> t_value_q;
		t_value_q.push(value);
		t_rsd.value_q = t_value_q;
		t_rsd.nan_num += ::isnan(value) ? 1 : 0;
		t_rsd.sum += nan_to_zero(value);
	}
	else
	{
		RollingSumDataPre *t_rsd = &(label_q_map_[label]);
		if (t_rsd->value_q.size() < window_)
		{
			t_rsd->value_q.push(value);
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum += nan_to_zero(value);
			// label_q_map_[label] = t_rsd;
			if (t_rsd->value_q.size() == window_)
			{
				if (nan_mean_ == true)
				{
					return round_double(t_rsd->sum / (window_ - t_rsd->nan_num));
				}
				else
				{
					return t_rsd->nan_num == 0 ? round_double(t_rsd->sum / window_) : NAN;
				}
			}
		}
		else
		{
			double tmp_pop_value = t_rsd->value_q.front();
			t_rsd->value_q.pop();
			t_rsd->value_q.push(value);
			t_rsd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum = t_rsd->sum - nan_to_zero(tmp_pop_value) + nan_to_zero(value);
			// label_q_map_[label] = t_rsd;
			if (nan_mean_ == true)
			{
				return round_double(t_rsd->sum / (window_ - t_rsd->nan_num));
			}
			else
			{
				return t_rsd->nan_num == 0 ? round_double(t_rsd->sum / window_) : NAN;
			}
		}
	}
	return NAN;
}

// 占内存少，但是效率变低了，低两倍速
// RollingMeanHelperEx::RollingMeanHelperEx(unsigned int total_bar_num, unsigned int window, bool nan_mean)
//{
//	window_ = window;
//	nan_mean_ = nan_mean;
//	total_bar_num_ = total_bar_num;
// }
//
// RollingMeanHelperEx::RollingMeanHelperEx(unsigned int total_bar_num, unsigned int window)
//{
//	RollingMeanHelperEx(total_bar_num, window, false);
// }
//
// RollingMeanHelperEx::~RollingMeanHelperEx()
//{
//	label_q_map_.clear();
// }
//
// double RollingMeanHelperEx::cal_rolling_mean(const unsigned int & bar_num, const double & value, const int & label)
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
// }

RollingTsstdHelper::RollingTsstdHelper(unsigned int window, bool nan_tsstd, bool unbiased)
{
	window_ = window;
	nan_tsstd_ = nan_tsstd;
	unbiased_ = unbiased;
}

RollingTsstdHelper::RollingTsstdHelper(unsigned int window, bool nan_tsstd)
{
	window_ = window;
	nan_tsstd_ = nan_tsstd;
	unbiased_ = true;
}

RollingTsstdHelper::RollingTsstdHelper(unsigned int window)
{
	window_ = window;
	nan_tsstd_ = false;
	unbiased_ = true;
}

RollingTsstdHelper::~RollingTsstdHelper()
{
}

double RollingTsstdHelper::cal_rolling_tsstd(double value, int label)
{
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end())
	{
		RollingStdData &t_rsd = label_q_map_[label];
		queue_ex<double> *t_value_q = new queue_ex<double>(window_);
		t_value_q->push(value);
		t_rsd.value_q = t_value_q;
		t_rsd.nan_num += ::isnan(value) ? 1 : 0;
		t_rsd.sum += nan_to_zero(value);
		t_rsd.square_sum = t_rsd.square_sum + nan_to_zero(value) * nan_to_zero(value);
	}
	else
	{
		RollingStdData *t_rsd = &label_q_iter->second;
		if (!t_rsd->value_q->full())
		{
			t_rsd->value_q->push(value);
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum += nan_to_zero(value);
			t_rsd->square_sum = t_rsd->square_sum + nan_to_zero(value) * nan_to_zero(value);
			// label_q_map_[label] = t_rsd;
			if (t_rsd->value_q->full())
			{
				if (window_ == t_rsd->nan_num)
				{
					return NAN;
				}
				double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num);
				if (variance < 0)
				{
					variance = 0;
				}
				double t_mean = NAN;
				double t_std = NAN;
				if (nan_tsstd_ == true)
				{
					if (unbiased_ == true)
					{
						if (window_ == t_rsd->nan_num + 1)
						{
							return NAN;
						}
						t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - 1)));
					}
					else
					{
						if (window_ == t_rsd->nan_num)
						{
							return NAN;
						}
						t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num)));
					}
					if (window_ == t_rsd->nan_num)
					{
						return NAN;
					}
					t_mean = round_double(t_rsd->sum / (window_ - t_rsd->nan_num));
				}
				else
				{
					if (t_rsd->nan_num == 0)
					{
						if (unbiased_ == true)
						{
							if (window_ == 1)
							{
								return NAN;
							}
							t_std = sqrt(round_double(variance / (window_ - 1)));
						}
						else
						{
							t_std = sqrt(round_double(variance / window_));
						}
						t_mean = round_double(t_rsd->sum / window_);
					}
					else
					{
						return NAN;
					}
				}
				if (t_std > -1e-12 && t_std < 1e-12)
				{
					return NAN;
				}
				return (value - t_mean) / t_std;
			}
		}
		else
		{
			double tmp_pop_value = t_rsd->value_q->front();
			t_rsd->value_q->pop();
			t_rsd->value_q->push(value);
			t_rsd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum = t_rsd->sum - nan_to_zero(tmp_pop_value) + nan_to_zero(value);
			t_rsd->square_sum = t_rsd->square_sum - nan_to_zero(tmp_pop_value) * nan_to_zero(tmp_pop_value) + nan_to_zero(value) * nan_to_zero(value);
			// label_q_map_[label] = t_rsd;
			if (window_ == t_rsd->nan_num)
			{
				return NAN;
			}
			double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num);
			double t_mean = NAN;
			double t_std = NAN;
			if (nan_tsstd_ == true)
			{
				if (unbiased_ == true)
				{
					if (window_ == t_rsd->nan_num + 1)
					{
						return NAN;
					}
					t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - 1)));
				}
				else
				{
					if (window_ == t_rsd->nan_num)
					{
						return NAN;
					}
					t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num)));
				}
				if (window_ == t_rsd->nan_num)
				{
					return NAN;
				}
				t_mean = round_double(t_rsd->sum / (window_ - t_rsd->nan_num));
			}
			else
			{
				if (t_rsd->nan_num == 0)
				{
					if (unbiased_ == true)
					{
						if (window_ == 1)
						{
							return NAN;
						}
						t_std = sqrt(round_double(variance / (window_ - 1)));
					}
					else
					{
						t_std = sqrt(round_double(variance / window_));
					}
					t_mean = round_double(t_rsd->sum / window_);
				}
				else
				{
					return NAN;
				}
			}
			if (t_std > -1e-12 && t_std < 1e-12)
			{
				return NAN;
			}
			return (value - t_mean) / t_std;
		}
	}
	return NAN;
}

MultiRollingTsstdHelper::MultiRollingTsstdHelper(unsigned int window, bool nan_tsstd, bool unbiased)
{
	window_ = window;
	nan_tsstd_ = nan_tsstd;
	unbiased_ = unbiased;
}

MultiRollingTsstdHelper::MultiRollingTsstdHelper(unsigned int window, bool nan_tsstd)
{
	window_ = window;
	nan_tsstd_ = nan_tsstd;
	unbiased_ = true;
}

MultiRollingTsstdHelper::MultiRollingTsstdHelper(unsigned int window)
{
	window_ = window;
	nan_tsstd_ = false;
	unbiased_ = true;
}

MultiRollingTsstdHelper::~MultiRollingTsstdHelper()
{
	// 头文件里是 shared_ptr<RollingTsstdHelper>，直接清空即可（不能 delete）
	helper_map_.clear();
}

double MultiRollingTsstdHelper::cal_multi_rolling_tsstd(double value, int label, int tsstd_label)
{
	auto helper_iter = helper_map_.find(tsstd_label);
	if (helper_iter == helper_map_.end())
	{
		shared_ptr<RollingTsstdHelper> p = make_shared<RollingTsstdHelper>(window_, nan_tsstd_, unbiased_);
		helper_map_[tsstd_label] = p;
		return p->cal_rolling_tsstd(value, label);
	}
	else
	{
		return helper_iter->second->cal_rolling_tsstd(value, label);
	}
}

double CommonFuncs::GET_RANK(const list<double> &values, double target)
{
	double count = 0.0;
	for (auto x : values)
	{
		if (target - x > 1e-8)
		{
			count++;
		}
	}
	return count / ((double)(values.size() - 1));
}
double CommonFuncs::GET_RANK(const vector<double> &values, double target)
{
	double count = 0.0;
	for (auto x : values)
	{
		if (target - x > 1e-8)
		{
			count++;
		}
	}
	return count / ((double)(values.size() - 1));
}

double CommonFuncs::GET_VALUE_FROM_LIST(const list<double> &values, unsigned int arg)
{
	auto values_front = values.begin();
	std::advance(values_front, arg);
	return *values_front;
}
int CommonFuncs::GET_VALUE_FROM_LIST(list<int> values, unsigned int arg)
{
	auto values_front = values.begin();
	std::advance(values_front, arg);
	return *values_front;
}

vector<double> CommonFuncs::LIST_TO_VECTOR(list<double> li)
{
	vector<double> vi;
	copy(li.begin(), li.end(), back_inserter(vi));
	return vi;
}

vector<double> CommonFuncs::RANGE(int length)
{
	vector<double> range;
	if (length > 0)
	{
		for (int i = 0; i < length; i++)
		{
			range.push_back(i);
		}
	}
	return range;
}
vector<double> CommonFuncs::RANGE(double length)
{
	vector<double> range;
	if (length > 0)
	{
		for (unsigned int i = 0; i < length; i++)
		{
			range.push_back(i);
		}
	}
	return range;
}
vector<double> CommonFuncs::RANGE(double start, double end, double length)
{
	vector<double> range;
	if (!((start > end && length > 0) || (start < end && length < 0) || (start == end) || length == 0))
	{
		if (length > 0)
		{
			double rollingValue = start;
			while (rollingValue < end)
			{
				range.push_back(rollingValue);
				rollingValue += length;
			}
		}
		else
		{
			double rollingValue = start;
			while (rollingValue > end)
			{
				range.push_back(rollingValue);
				rollingValue += length;
			}
		}
	}
	return range;
}

string CommonFuncs::ITERATE_TO_STRING(const vector<double> &values)
{
	string s = "(";
	for (auto v : values)
	{
		s = s.append(to_string(v)).append("_");
	}
	return s.substr(0, s.size() - 1).append(")");
}

string CommonFuncs::ITERATE_TO_STRING(const list<double> &values)
{
	string s = "(";
	for (auto v : values)
	{
		s = s.append(to_string(v)).append("_");
	}
	return s.substr(0, s.size() - 1).append(")");
}
string CommonFuncs::ITERATE_TO_STRING(vector<int> values)
{
	string s = "(";
	for (auto v : values)
	{
		s = s.append(to_string(v)).append("_");
	}
	return s.substr(0, s.size() - 1).append(")");
}
string CommonFuncs::ITERATE_TO_STRING(list<int> values)
{
	string s = "(";
	for (auto v : values)
	{
		s = s.append(to_string(v)).append("_");
	}
	return s.substr(0, s.size() - 1).append(")");
}
string CommonFuncs::ITERATE_TO_STRING(vector<string> values)
{
	string s = "(";
	for (auto v : values)
	{
		s = s.append(v).append("_");
	}
	return s.substr(0, s.size() - 1).append(")");
}
string CommonFuncs::ITERATE_TO_STRING(list<string> values)
{
	string s = "(";
	for (auto v : values)
	{
		s = s.append(v).append("_");
	}
	return s.substr(0, s.size() - 1).append(")");
}

double CommonFuncs::LAST_RANK(const list<double> &values)
{
	return GET_RANK(values, values.back());
}

double CommonFuncs::LAST_RANK(const vector<double> &values)
{
	return GET_RANK(values, values.back());
}

double CommonFuncs::SUM(const list<double> &values)
{
	double sum = 0;
	for (auto x : values)
	{
		sum += x;
	}
	return sum;
}

double CommonFuncs::SUM(const vector<double> &values)
{
	double sum = 0;
	for (auto x : values)
	{
		sum += x;
	}
	return sum;
}

double CommonFuncs::NAN_SUM(const list<double> &values)
{
	double sum = 0;
	for (auto x : values)
	{
		if (!(::isnan(x)))
		{
			sum += x;
		}
	}
	return sum;
}

double CommonFuncs::NAN_SUM(const vector<double> &values)
{
	double sum = 0;
	for (auto x : values)
	{
		if (!(::isnan(x)))
		{
			sum += x;
		}
	}
	return sum;
}

double CommonFuncs::MEAN(const list<double> &values)
{
	if (values.size() == 0)
		return NAN;
	return SUM(values) / ((double)values.size());
}

double CommonFuncs::MEAN(const vector<double> &values)
{
	if (values.size() == 0)
		return NAN;
	return SUM(values) / ((double)values.size());
}

double CommonFuncs::NAN_MEAN(const list<double> &values)
{
	if (values.size() == 0)
		return NAN;
	double count = 0;
	double sum = 0;
	for (auto x : values)
	{
		if (!(::isnan(x)))
		{
			count++;
			sum += x;
		}
	}
	return sum / count;
}

double CommonFuncs::NAN_MEAN(const vector<double> &values)
{
	if (values.size() == 0)
	{
		return NAN;
	}
	double count = 0;
	double sum = 0;
	for (auto x : values)
	{
		if (!(::isnan(x)))
		{
			count++;
			sum += x;
		}
	}
	return sum / count;
}

double CommonFuncs::EMA(const list<double> &values, double decay)
{
	double alpha = 1.0 - 1.0 / (1.0 + decay);
	unsigned int size = (unsigned int)values.size();
	vector<double> weights(size);
	double weightsSum = 0;
	for (unsigned int i = 0; i < size; i++)
	{
		double currentWeight = pow(alpha, (size - i - 1));
		weights[i] = currentWeight;
		weightsSum += currentWeight;
	}
	double emaSum = 0;
	unsigned i = 0;
	for (auto x : values)
	{
		emaSum += (x * weights[i]);
		i++;
	}
	return emaSum / weightsSum;
}

double CommonFuncs::EMA(const vector<double> &values, double decay)
{
	double alpha = 1.0 - 1.0 / (1.0 + decay);
	unsigned int size = (unsigned int)values.size();
	vector<double> weights(size);
	double weightsSum = 0;
	for (unsigned int i = 0; i < size; i++)
	{
		double currentWeight = pow(alpha, (size - i - 1));
		weights[i] = currentWeight;
		weightsSum += currentWeight;
	}
	double emaSum = 0;
	unsigned i = 0;
	for (auto x : values)
	{
		emaSum += (x * weights[i]);
		i++;
	}
	return emaSum / weightsSum;
}

double CommonFuncs::EMA(const vector<double> &values, vector<double> &weights, double weightsSum)
{
	if (values.size() != weights.size())
	{
		return NAN;
	}
	double emaSum = 0;
	unsigned i = 0;
	for (auto x : values)
	{
		emaSum += (x * weights[i]);
		i++;
	}
	return emaSum / weightsSum;
}

double CommonFuncs::PROD(const list<double> &values)
{
	double prod = 1.0;
	for (auto x : values)
	{
		prod *= x;
	}
	return prod;
}

double CommonFuncs::PROD(const vector<double> &values)
{
	double prod = 1.0;
	for (auto x : values)
	{
		prod *= x;
	}
	return prod;
}

double CommonFuncs::NAN_PROD(const list<double> &values)
{
	double prod = 1.0;
	for (auto x : values)
	{
		if (!(::isnan(x)))
		{
			prod *= x;
		}
	}
	return prod;
}

double CommonFuncs::NAN_PROD(const vector<double> &values)
{
	double prod = 1.0;
	for (auto x : values)
	{
		if (!(::isnan(x)))
		{
			prod *= x;
		}
	}
	return prod;
}

double CommonFuncs::STD(const list<double> &values)
{
	if (values.size() == 0)
		return NAN;
	double mean = MEAN(values);
	double squareSum = 0;
	for (auto x : values)
	{
		squareSum += (x - mean) * (x - mean);
	}
	return sqrt(squareSum / ((double)(values.size() - 1)));
}

double CommonFuncs::STD(const vector<double> &values)
{
	if (values.size() == 0)
		return NAN;
	double mean = MEAN(values);
	double squareSum = 0;
	for (auto x : values)
	{
		squareSum += (x - mean) * (x - mean);
	}
	return sqrt(squareSum / ((double)(values.size() - 1)));
}

double CommonFuncs::NAN_STD(const list<double> &values)
{
	if (values.size() == 0)
		return NAN;
	double mean = NAN_MEAN(values);
	double squareSum = 0;
	double notNanCount = 0;
	for (auto x : values)
	{
		if (!(::isnan(x)))
		{
			squareSum += (x - mean) * (x - mean);
			notNanCount++;
		}
	}
	return sqrt(squareSum / (notNanCount - 1));
}

double CommonFuncs::NAN_STD(const vector<double> &values)
{
	if (values.size() == 0)
		return NAN;
	double mean = NAN_MEAN(values);
	double squareSum = 0;
	double notNanCount = 0;
	for (auto x : values)
	{
		if (!(::isnan(x)))
		{
			squareSum += (x - mean) * (x - mean);
			notNanCount++;
		}
	}
	return sqrt(squareSum / (notNanCount - 1));
}

double CommonFuncs::CORR(const list<double> &X, const list<double> &Y)
{
	if (X.size() == 0 || Y.size() == 0)
	{
		return NAN;
	}
	if (X.size() != Y.size())
	{
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
	double divide = sqrt((size * squareSum_X - sum_X * sum_X) * (size * squareSum_Y - sum_Y * sum_Y));
	if (divide == 0)
	{
		corr = NAN;
	}
	else
	{
		corr /= divide;
	}
	return corr;
}
double CommonFuncs::CORR(const vector<double> &X, const vector<double> &Y)
{
	if (X.size() == 0 || Y.size() == 0)
		return NAN;
	if (X.size() != Y.size())
	{
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
	double divide = sqrt((size * squareSum_X - sum_X * sum_X) * (size * squareSum_Y - sum_Y * sum_Y));
	if (divide == 0)
	{
		corr = NAN;
	}
	else
	{
		corr /= divide;
	}
	return corr;
}

double CommonFuncs::NAN_CORR(const list<double> &X, const list<double> &Y)
{
	if (X.size() == 0 || Y.size() == 0)
		return NAN;
	if (X.size() != Y.size())
	{
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
		if ((::isnan(X_vector[i]) || (::isnan(Y_vector[i]))))
		{
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
	double divide = sqrt((size * squareSum_X - sum_X * sum_X) * (size * squareSum_Y - sum_Y * sum_Y));
	if (divide == 0)
	{
		corr = NAN;
	}
	else
	{
		corr /= divide;
	}
	return corr;
}

double CommonFuncs::NAN_CORR(const vector<double> &X, const vector<double> &Y)
{
	if (X.size() == 0 || Y.size() == 0)
		return NAN;
	if (X.size() != Y.size())
	{
		throw "list size not equal with each other!";
	}

	double size = (double)X.size();
	double listSize = size;
	double sum_X = 0, sum_Y = 0, sum_XY = 0;
	double squareSum_X = 0, squareSum_Y = 0;

	for (unsigned long long int i = 0; i < listSize; i++)
	{
		if ((::isnan(X[i]) || (::isnan(Y[i]))))
		{
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
	double divide = sqrt((size * squareSum_X - sum_X * sum_X) * (size * squareSum_Y - sum_Y * sum_Y));
	// cout << size << " -- " << sum_XY<<" -- "<< size * sum_XY
	//     <<" -- "<< sum_X * sum_Y
	//     << " -- "<< size * sum_XY - sum_X * sum_Y << endl;
	// cout << corr << " -- " << divide << endl;
	if (fabs(divide) < 0.000001)
	{
		corr = NAN;
	}
	else
	{
		corr /= divide;
	}
	return corr;
}

double CommonFuncs::GMEAN(const list<double> &values)
{
	if (values.size() == 0)
		return NAN;
	return pow(PROD(values), (1.0 / (double)values.size()));
}

double CommonFuncs::GMEAN(const vector<double> &values)
{
	if (values.size() == 0)
		return NAN;
	return pow(PROD(values), (1.0 / (double)values.size()));
}

double CommonFuncs::NAN_GMEAN(const list<double> &values)
{
	if (values.size() == 0)
		return NAN;
	double prod = 1.0;
	double count = 0.0;
	for (auto x : values)
	{
		if (!(::isnan(x)))
		{
			count++;
			prod *= x;
		}
	}
	return pow(prod, (1.0 / count));
}

double CommonFuncs::NAN_GMEAN(const vector<double> &values)
{
	if (values.size() == 0)
		return NAN;
	double prod = 1.0;
	double count = 0.0;
	for (auto x : values)
	{
		if (!(::isnan(x)))
		{
			count++;
			prod *= x;
		}
	}
	return pow(prod, (1.0 / count));
}

double CommonFuncs::NK_MAX(const list<double> &values)
{
	if (values.size() == 0)
		return NAN;
	double max = values.front();
	for (auto x : values)
	{
		if (x > max)
		{
			max = x;
		}
	}
	return max;
}
double CommonFuncs::NK_MAX(const vector<double> &values)
{
	if (values.size() == 0)
		return NAN;
	double max = values.front();
	for (auto x : values)
	{
		if (x > max)
		{
			max = x;
		}
	}
	return max;
}

double CommonFuncs::NK_MIN(const list<double> &values)
{
	if (values.size() == 0)
		return NAN;
	double min = values.front();
	for (auto x : values)
	{
		if (x < min)
		{
			min = x;
		}
	}
	return min;
}

double CommonFuncs::NK_MIN(const vector<double> &values)
{
	if (values.size() == 0)
		return NAN;
	double min = values.front();
	for (auto x : values)
	{
		if (x < min)
		{
			min = x;
		}
	}
	return min;
}

#ifdef _WIN32
double CommonFuncs::MAX(const list<double> &values)
{
	if (values.size() == 0)
		return NAN;
	double max = values.front();
	for (auto x : values)
	{
		if (x > max)
		{
			max = x;
		}
	}
	return max;
}
double CommonFuncs::MAX(const vector<double> &values)
{
	if (values.size() == 0)
		return NAN;
	double max = values.front();
	for (auto x : values)
	{
		if (x > max)
		{
			max = x;
		}
	}
	return max;
}

double CommonFuncs::MIN(const list<double> &values)
{
	if (values.size() == 0)
		return NAN;
	double min = values.front();
	for (auto x : values)
	{
		if (x < min)
		{
			min = x;
		}
	}
	return min;
}

double CommonFuncs::MIN(const vector<double> &values)
{
	if (values.size() == 0)
		return NAN;
	double min = values.front();
	for (auto x : values)
	{
		if (x < min)
		{
			min = x;
		}
	}
	return min;
}
#endif

double CommonFuncs::ARGMAX(const list<double> &values)
{
	if (values.size() == 0)
		return NAN;
	unsigned int argmax = 0;
	int count = -1;
	double max = values.front();
	for (auto cursor : values)
	{
		count++;
		if (cursor > max)
		{
			max = cursor;
			argmax = count;
		}
	}
	return argmax;
}

double CommonFuncs::ARGMAX(const vector<double> &values)
{
	if (values.size() == 0)
		return NAN;
	unsigned int argmax = 0;
	int count = -1;
	double max = values.front();
	for (auto cursor : values)
	{
		count++;
		if (cursor > max)
		{
			max = cursor;
			argmax = count;
		}
	}
	return argmax;
}

double CommonFuncs::ARGMIN(const list<double> &values)
{
	if (values.size() == 0)
		return NAN;
	unsigned int argmin = 0;
	int count = -1;
	double min = values.front();
	for (auto cursor : values)
	{
		count++;
		if (cursor < min)
		{
			min = cursor;
			argmin = count;
		}
	}
	return argmin;
}

double CommonFuncs::ARGMIN(const vector<double> &values)
{
	if (values.size() == 0)
		return NAN;
	unsigned int argmin = 0;
	int count = -1;
	double min = values.front();
	for (auto cursor : values)
	{
		count++;
		if (cursor < min)
		{
			min = cursor;
			argmin = count;
		}
	}
	return argmin;
}

int CommonFuncs::timet_to_3s_bar_num(const time_t &time)
{
#ifdef _WIN32
	struct tm tp;
	time_t local_time = time + 28800;
	gmtime_s(&tp, &local_time);
	return tp.tm_hour < 12 ? (tp.tm_hour - 9) * 1200 + (tp.tm_min - 30) * 20 + tp.tm_sec / 3 + 1 : 2401 + (tp.tm_hour - 13) * 1200 + tp.tm_min * 20 + tp.tm_sec / 3;
#else
	time_t local_time = time + 28800;
	struct tm *tp = gmtime(&local_time);
	return tp->tm_hour < 12 ? (tp->tm_hour - 9) * 1200 + (tp->tm_min - 30) * 20 + tp->tm_sec / 3 + 1 : 2401 + (tp->tm_hour - 13) * 1200 + tp->tm_min * 20 + tp->tm_sec / 3;
#endif
}

double CommonFuncs::LAST_RANK_EQ(const list<double> &values)
{
	return GET_RANK(values, values.back() + 1e-7) * (values.size() - 1);
}

double CommonFuncs::LAST_RANK_EQ(const vector<double> &values)
{
	return GET_RANK(values, values.back() + 1e-7) * (values.size() - 1);
}

double CommonFuncs::get_rank(const queue_ex<double> *queue, const double &target)
{
	if (queue->empty())
	{
		return false;
	}
	double count = 0.0;
	double v_size = 0.0;
	size_t head = queue->_uhead;
	size_t tail = queue->_utail;
	size_t usize = queue->_usize;
	double *vet = queue->value_q;
	while (head != tail)
	{
		const double &val = vet[head];
		if (target - val > 1e-8)
		{
			count += 1.0;
		}
		v_size += 1.0;
		head = (head + 1) % usize;
	}
	return count / (v_size - 1);
}

double CommonFuncs::get_last_rank(const queue_ex<double> *queue)
{
	if (queue->empty())
	{
		return false;
	}
	double count = 0.0;
	double v_size = 0.0;
	size_t head = queue->_uhead;
	size_t tail = queue->_utail;
	size_t usize = queue->_usize;
	double *vet = queue->value_q;
	const double &target = vet[(tail + usize - 1) % usize];
	while (head != tail)
	{
		const double &val = vet[head];
		if (target - val > 1e-8)
		{
			count += 1.0;
		}
		v_size += 1.0;
		head = (head + 1) % usize;
	}
	return count / (v_size - 1);
}

// 获取版本号
const char *CommonFuncs::GetVersion()
{
	return FDLL_VERSION;
}

/// add 20200622 beg

CumSumData::CumSumData()
{
	sum = 0;
	num = 0;
}

CumSumHelper::CumSumHelper()
{
	min_periods_ = 1;
}

CumSumHelper::CumSumHelper(unsigned int min_periods)
{
	min_periods_ = min_periods;
}

CumSumHelper::~CumSumHelper()
{
}

double CumSumHelper::cal_cum_sum(double value, int label)
{
	unordered_map<int, CumSumData>::const_iterator label_map_iter = label_map_.find(label);
	if (label_map_iter == label_map_.end())
	{
		CumSumData &t_csd = label_map_[label];
		t_csd.num += ::isnan(value) ? 0 : 1;
		t_csd.sum += nan_to_zero(value);
		if (t_csd.num >= min_periods_)
		{
			return t_csd.sum;
		}
	}
	else
	{
		CumSumData *t_csd = &(label_map_[label]);
		t_csd->num += ::isnan(value) ? 0 : 1;
		t_csd->sum += nan_to_zero(value);
		if (t_csd->num >= min_periods_)
		{
			return t_csd->sum;
		}
	}
	return NAN;
}

int CumSumHelper::reset(int label)
{
	unordered_map<int, CumSumData>::const_iterator label_map_iter = label_map_.find(label);
	if (label_map_iter != label_map_.end())
	{
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

CumMeanHelper::CumMeanHelper()
{
	min_periods_ = 1;
}

CumMeanHelper::CumMeanHelper(unsigned int min_periods)
{
	min_periods_ = min_periods;
}

CumMeanHelper::~CumMeanHelper()
{
}

double CumMeanHelper::cal_cum_mean(double value, int label)
{
	unordered_map<int, CumSumData>::const_iterator label_map_iter = label_map_.find(label);
	if (label_map_iter == label_map_.end())
	{
		CumSumData &t_csd = label_map_[label];
		t_csd.num += ::isnan(value) ? 0 : 1;
		t_csd.sum += nan_to_zero(value);
		if (t_csd.num >= min_periods_)
		{
			return round_double(t_csd.sum / t_csd.num);
		}
	}
	else
	{
		CumSumData *t_csd = &(label_map_[label]);
		t_csd->num += ::isnan(value) ? 0 : 1;
		t_csd->sum += nan_to_zero(value);
		if (t_csd->num >= min_periods_)
		{
			return round_double(t_csd->sum / t_csd->num);
		}
	}
	return NAN;
}

int CumMeanHelper::reset(int label)
{
	unordered_map<int, CumSumData>::const_iterator label_map_iter = label_map_.find(label);
	if (label_map_iter != label_map_.end())
	{
		CumSumData *t_csd = &(label_map_[label]);
		t_csd->num = 0;
		t_csd->sum = 0;
	}
	return 0;
}

void CumMeanHelper::SN_reset()
{
	unordered_map<int, CumSumData>::iterator label_map_iter = label_map_.begin();
	for (; label_map_iter != label_map_.end(); label_map_iter++)
	{
		CumSumData *t_csd = &(label_map_iter->second);
		t_csd->num = 0;
		t_csd->sum = 0;
	}
}

CumCorrData::CumCorrData()
{
	square_sum1 = 0;
	square_sum2 = 0;
	sum1 = 0;
	sum2 = 0;
	sum12 = 0;
	num = 0;
}

CumCorrHelper::CumCorrHelper()
{
	min_periods_ = 1;
}

CumCorrHelper::CumCorrHelper(unsigned int min_periods)
{
	min_periods_ = min_periods;
}

CumCorrHelper::~CumCorrHelper()
{
}

double CumCorrHelper::cal_cum_corr(double value1, double value2, int label)
{
	unordered_map<int, CumCorrData>::const_iterator label_map_iter = label_map_.find(label);
	if (label_map_iter == label_map_.end())
	{
		CumCorrData &t_ccd = label_map_[label];
		bool tmp_nan_flag = (::isnan(value1) || ::isnan(value2));
		t_ccd.num += tmp_nan_flag ? 0 : 1;
		if (tmp_nan_flag == false)
		{
			t_ccd.sum1 += value1;
			t_ccd.sum2 += value2;
			t_ccd.sum12 += value1 * value2;
			t_ccd.square_sum1 += value1 * value1;
			t_ccd.square_sum2 += value2 * value2;
		}
		if (t_ccd.num >= min_periods_)
		{
			double variance1 = round_double((t_ccd.square_sum1 - t_ccd.sum1 * t_ccd.sum1 / t_ccd.num) / t_ccd.num);
			double variance2 = round_double((t_ccd.square_sum2 - t_ccd.sum2 * t_ccd.sum2 / t_ccd.num) / t_ccd.num);
			if ((variance1 < 1e-12) || (variance2 < 1e-12))
			{
				return NAN;
			}
			else
			{
				double std1 = sqrt(variance1);
				double std2 = sqrt(variance2);
				double mean1 = t_ccd.sum1 / t_ccd.num;
				double mean2 = t_ccd.sum2 / t_ccd.num;
				double mean12 = t_ccd.sum12 / t_ccd.num;
				return round_double((mean12 - mean1 * mean2) / round_double((std1 * std2)));
			}
		}
	}
	else
	{
		CumCorrData *t_ccd = &(label_map_[label]);
		bool tmp_nan_flag = (::isnan(value1) || ::isnan(value2));
		t_ccd->num += tmp_nan_flag ? 0 : 1;
		if (tmp_nan_flag == false)
		{
			t_ccd->sum1 += value1;
			t_ccd->sum2 += value2;
			t_ccd->sum12 += value1 * value2;
			t_ccd->square_sum1 += value1 * value1;
			t_ccd->square_sum2 += value2 * value2;
		}
		if (t_ccd->num >= min_periods_)
		{
			double variance1 = round_double((t_ccd->square_sum1 - t_ccd->sum1 * t_ccd->sum1 / t_ccd->num) / t_ccd->num);
			double variance2 = round_double((t_ccd->square_sum2 - t_ccd->sum2 * t_ccd->sum2 / t_ccd->num) / t_ccd->num);
			if ((variance1 < 1e-12) || (variance2 < 1e-12))
			{
				return NAN;
			}
			else
			{
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

int CumCorrHelper::reset(int label)
{
	unordered_map<int, CumCorrData>::const_iterator label_map_iter = label_map_.find(label);
	if (label_map_iter != label_map_.end())
	{
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
void CumCorrHelper::SN_reset()
{
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

CumStdData::CumStdData()
{
	square_sum = 0;
	sum = 0;
	nan_num = 0;
	num = 0;
}

CumStdData::~CumStdData() {};

CumStdData &CumStdData::operator=(const CumStdData &rsd)
{
	square_sum = rsd.square_sum;
	sum = rsd.sum;
	nan_num = rsd.nan_num;
	num = rsd.num;
	return *this;
}

CumStdHelper::CumStdHelper(unsigned int min_periods, bool nan_std, bool unbiased)
{
	min_periods_ = min_periods;
	nan_std_ = nan_std;
	unbiased_ = unbiased;
}

CumStdHelper::~CumStdHelper() {};

double CumStdHelper::cal_cum_std(double value, int label)
{
	CumStdData &t_rsd = label_q_map_[label];
	t_rsd.nan_num += ::isnan(value) ? 1 : 0;
	t_rsd.num += 1;
	t_rsd.sum += nan_to_zero(value);
	t_rsd.square_sum = t_rsd.square_sum + nan_to_zero(value) * nan_to_zero(value);
	if (t_rsd.num == 1 || t_rsd.num < min_periods_)
	{
		return NAN;
	}
	if (t_rsd.num == t_rsd.nan_num)
	{
		return NAN;
	}
	double variance = t_rsd.square_sum - t_rsd.sum * t_rsd.sum / (t_rsd.num - t_rsd.nan_num);
	if (variance < 0)
	{
		variance = 0;
	}
	if (nan_std_ == true)
	{
		if (unbiased_ == true)
		{
			if (t_rsd.num == t_rsd.nan_num + 1)
			{
				return NAN;
			}
			return sqrt(round_double(variance / (t_rsd.num - t_rsd.nan_num - 1)));
		}
		else
		{
			if (t_rsd.num == t_rsd.nan_num)
			{
				return NAN;
			}
			return sqrt(round_double(variance / (t_rsd.num - t_rsd.nan_num)));
		}
	}
	else
	{
		if (t_rsd.nan_num == 0)
		{
			if (unbiased_ == true)
			{
				if (t_rsd.num == 1)
				{
					return NAN;
				}
				return sqrt(round_double(variance / (t_rsd.num - 1)));
			}
			else
			{
				return sqrt(round_double(variance / t_rsd.num));
			}
		}
		else
		{
			return NAN;
		}
	}
	return NAN;
}

int CumStdHelper::reset(int label)
{
	auto itor = label_q_map_.find(label);
	if (itor != label_q_map_.end())
	{
		CumStdData &csd = itor->second;
		csd.nan_num = 0;
		csd.num = 0;
		csd.square_sum = 0;
		csd.sum = 0;
	}
	return 1;
}
void CumStdHelper::SN_reset()
{
	for (auto &itor : label_q_map_)
	{
		CumStdData &csd = itor.second;
		csd.nan_num = 0;
		csd.num = 0;
		csd.square_sum = 0;
		csd.sum = 0;
	}
}

/// add 20200701 begin
RollingEmaData::RollingEmaData()
{
	mean = 0;
	nan_num = 0;
}

RollingEmaHelper::RollingEmaHelper(unsigned int window, double decay)
{
	window_ = (window == 0) ? 1 : window;
	decay_ = decay;
	// 每 bar 都要用的两个常数，构造期算好，热路径上不再调 pow
	p_ = decay_ / (1.0 + decay_);
	p_pow_window_ = std::pow(p_, (double)window_);
}

RollingEmaHelper::~RollingEmaHelper() {};

double RollingEmaHelper::cal_rolling_ema(double value, int label)
{
	unordered_map<int, RollingEmaData>::const_iterator label_q_iter = label_q_map_.find(label);
	const double p = p_;
	if (label_q_iter == label_q_map_.end())
	{
		RollingEmaData &t_red = label_q_map_[label];
		t_red.value_q.push(value);
		t_red.nan_num += ::isnan(value) ? 1 : 0;
		t_red.mean += nan_to_zero(value);
	}
	else
	{
		RollingEmaData *t_red = &(label_q_map_[label]);
		if (t_red->value_q.size() < window_)
		{
			t_red->value_q.push(value);
			t_red->nan_num += ::isnan(value) ? 1 : 0;
			int n = t_red->value_q.size();
			double oldsum = (1 - pow(p, n - 1)) / (1 - p);
			double newsum = (1 - pow(p, n)) / (1 - p);
			t_red->mean = (t_red->mean * oldsum * p + nan_to_zero(value)) / newsum;
			// label_q_map_[label] = t_rsd;
			if (t_red->value_q.size() == window_)
			{
				if (false)
				{
					return round_double(t_red->mean);
				}
				else
				{
					return t_red->nan_num == 0 ? round_double(t_red->mean) : NAN;
				}
			}
		}
		else
		{
			double tmp_pop_value = t_red->value_q.front();
			t_red->value_q.pop();
			t_red->value_q.push(value);
			t_red->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_red->nan_num += ::isnan(value) ? 1 : 0;
			// 窗口已满：n 恒为 window_，权重和是常数，热路径不再调 pow
			double oldsum = (1 - p_pow_window_) / (1 - p);
			t_red->mean = (t_red->mean * oldsum * p - nan_to_zero(tmp_pop_value) * p_pow_window_ + nan_to_zero(value)) / oldsum;
			// label_q_map_[label] = t_rsd;
			if (false)
			{
				return round_double(t_red->mean);
			}
			else
			{
				return t_red->nan_num == 0 ? round_double(t_red->mean) : NAN;
			}
		}
	}
	return NAN;
}
/// add 20200701 end
// ---------------------------------------------------------------------------
// 以下 RollingMedianHelper / RollingSortedHelper / RollingRankHelper 的旧实现，
// 是「有序双向链表 + 每 bar 从 pMedian 走过去」的 O(n) 算法，而且
// RollingSortedHelper 里对 pForeachNode->pre / ->next 缺少空指针保护，
// 在极端数据下会崩。现已被 RollingSortedImpl（顺序统计 treap，期望 O(log n)）
// 取代，新实现在本段之后。旧代码留在此处仅作口径对照，不参与编译。
// ---------------------------------------------------------------------------
#if 0
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
		if (pTop == NULL)
		{
			cout << "000 pTop = NULL..." << endl;
		}
		// std::cout << "pop " << pTop->value << endl;
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
				// 判断当前数据是在median的左边还是右边
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
					if (pForeach == NULL || fabs(pForeach->value - t_red.pMedian->value) > zero_val) // 表示不等
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
				if (pForeachNode->pre == NULL)
				{
					cout << "001 pre = NULL..." << endl;
				}
				pForeachNode->pre->next = pTNewMedianNode;
				pForeachNode->pre = pTNewMedianNode;

				if (t_red.num % 2 == 1) // 如果从偶数变成奇数，Media移动一个
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
					if (t_red.num % 2 == 1) // 如果从偶数变成奇数，Media移动一个
					{
						if (t_red.pMedian == NULL)
						{
							cout << "002 pMedian = NULL..." << endl;
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
				if (pForeachNode->next == NULL)
				{
					cout << "003 next = NULL..." << endl;
				}
				pForeachNode->next->pre = pTNewMedianNode;
				pForeachNode->next = pTNewMedianNode;
				/*pTNewMedianNode->next = pForeachNode;
				pTNewMedianNode->pre = pForeachNode->pre;
				pForeachNode->pre->next = pTNewMedianNode;
				pForeachNode->pre = pTNewMedianNode;*/
				if (t_red.num % 2 == 0) // 如果从偶数变成奇数，Media移动一个
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
					if (t_red.num % 2 == 0) // 如果从偶数变成奇数，Media移动一个
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
	// print
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
		if (t_red.pMedian->next == NULL)
		{
			cout << "004 next = NULL..." << endl;
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

TMedianNode *RollingSortedHelper::cal_rolling_median(
	const double &value, const int &label)
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
		// std::cout << "pop " << pTop->value << endl;
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
				// 判断当前数据是在median的左边还是右边
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
					if (pForeach == NULL || fabs(pForeach->value - t_red.pMedian->value) > zero_val) // 表示不等
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

				if (t_red.num % 2 == 1) // 如果从偶数变成奇数，Media移动一个
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
					if (t_red.num % 2 == 1) // 如果从偶数变成奇数，Media移动一个
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
				if (t_red.num % 2 == 0) // 如果从偶数变成奇数，Media移动一个
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
					if (t_red.num % 2 == 0) // 如果从偶数变成奇数，Media移动一个
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
	// print
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
		// std::cout << "pop " << pTop->value << endl;
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
				// 判断当前数据是在median的左边还是右边
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
					if (pForeach == NULL || fabs(pForeach->value - t_red.pMedian->value) > zero_val) // 表示不等
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

				if (t_red.num % 2 == 1) // 如果从偶数变成奇数，Media移动一个
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
					if (t_red.num % 2 == 1) // 如果从偶数变成奇数，Media移动一个
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
		// print
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
				if (t_red.num % 2 == 0) // 如果从偶数变成奇数，Media移动一个
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
					if (t_red.num % 2 == 0) // 如果从偶数变成奇数，Media移动一个
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

		// print
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

#endif // 旧 O(n) 链表实现（RollingMedianHelper / RollingSortedHelper / RollingRankHelper）

/// add 20261001 begin
// ---------------------------------------------------------------------------
// RollingSortedImpl：顺序统计 treap（带子树大小的随机化 BST）
//   * 窗口 = fifo(queue_ex) + 只装非 NaN 值的 treap，push 时同步淘汰队首
//   * insert / erase / 数小于个数 / 取第 k 小，均摊 O(log n)
//   * 节点经 free list 复用，窗口填满后每 bar 不再分配
//   * 随机数种子固定，树形与结果可复现
//   * NaN 不入树，只缩短有效窗口（与旧链表版口径一致）
// ---------------------------------------------------------------------------
class RollingSortedImpl
{
public:
	struct RankNode
	{
		double value;
		unsigned int prio;
		int size;
		int left;
		int right;
	};

	RollingSortedImpl(unsigned int window);
	~RollingSortedImpl();
	RollingSortedImpl *clone() const;

	// 送入一个值（同时按窗口长度淘汰队首）
	void push(double value);
	// fifo 已经攒满一个完整窗口
	bool window_full() const { return value_q_ != nullptr && value_q_->full(); }
	// 当前窗口里的非 NaN 个数
	int count() const { return count_; }
	// 窗口内严格小于"最后压入值"的个数
	double last_rank() const { return last_rank_; }
	// 最后压入的是 NaN
	bool last_is_nan() const { return last_is_nan_; }
	// 窗口内第 k 小（0 基），期望 O(log n)
	double kth(int k) const;
	// 升序输出窗口内容，O(n)
	void values_ascending(vector<double> &out);
	// 累计压入次数，供调用方识别过期游标
	unsigned long long stamp() const { return pushes_; }

private:
	RollingSortedImpl(const RollingSortedImpl &);
	RollingSortedImpl &operator=(const RollingSortedImpl &);

	int new_node(double value);
	void free_node(int i);
	void update(int i);
	int size_of(int i) const;
	unsigned int next_rand();
	void split(int t, double value, int &a, int &b);
	int merge(int a, int b);
	int insert(int t, int node);
	int erase(int t, double value, bool &removed);
	int count_less(int t, double value) const;
	double kth_of(int t, int k) const;

	unsigned int window_;
	queue_ex<double> *value_q_;
	vector<RankNode> nodes_;
	vector<int> free_;
	int root_;
	int count_; // 树里当前的值的个数（NaN 不计）
	double last_rank_;
	double last_value_;
	bool last_is_nan_;
	unsigned int seed_;
	unsigned long long pushes_;
	vector<int> stack_; // 中序遍历的暂存栈
};

RollingSortedImpl::RollingSortedImpl(unsigned int window)
{
	window_ = (window == 0) ? 1 : window;
	value_q_ = new queue_ex<double>(window_);
	root_ = -1;
	count_ = 0;
	last_rank_ = NAN;
	last_value_ = NAN;
	last_is_nan_ = true;
	pushes_ = 0;
	// 固定种子，保证树形与结果可复现
	seed_ = 0x9E3779B9u + window_;
}

RollingSortedImpl::~RollingSortedImpl()
{
	if (value_q_)
	{
		delete value_q_;
		value_q_ = nullptr;
	}
}

RollingSortedImpl *RollingSortedImpl::clone() const
{
	RollingSortedImpl *copy = new RollingSortedImpl(window_);
	delete copy->value_q_;
	copy->value_q_ = value_q_ ? new queue_ex<double>(*value_q_) : nullptr;
	copy->nodes_ = nodes_;
	copy->free_ = free_;
	copy->root_ = root_;
	copy->count_ = count_;
	copy->last_rank_ = last_rank_;
	copy->last_value_ = last_value_;
	copy->last_is_nan_ = last_is_nan_;
	copy->seed_ = seed_;
	copy->pushes_ = pushes_;
	return copy;
}

unsigned int RollingSortedImpl::next_rand()
{
	seed_ ^= seed_ << 13;
	seed_ ^= seed_ >> 17;
	seed_ ^= seed_ << 5;
	return seed_;
}

int RollingSortedImpl::size_of(int i) const
{
	return i < 0 ? 0 : nodes_[i].size;
}

void RollingSortedImpl::update(int i)
{
	if (i < 0)
	{
		return;
	}
	nodes_[i].size = 1 + size_of(nodes_[i].left) + size_of(nodes_[i].right);
}

int RollingSortedImpl::new_node(double value)
{
	int i = -1;
	if (!free_.empty())
	{
		i = free_.back();
		free_.pop_back();
	}
	else
	{
		nodes_.push_back(RankNode());
		i = (int)nodes_.size() - 1;
	}
	nodes_[i].value = value;
	nodes_[i].prio = next_rand();
	nodes_[i].size = 1;
	nodes_[i].left = -1;
	nodes_[i].right = -1;
	return i;
}

void RollingSortedImpl::free_node(int i)
{
	free_.push_back(i);
}

// a 拿到小于 value 的部分，b 拿到其余
void RollingSortedImpl::split(int t, double value, int &a, int &b)
{
	if (t < 0)
	{
		a = -1;
		b = -1;
		return;
	}
	if (nodes_[t].value < value)
	{
		split(nodes_[t].right, value, nodes_[t].right, b);
		a = t;
		update(a);
	}
	else
	{
		split(nodes_[t].left, value, a, nodes_[t].left);
		b = t;
		update(b);
	}
}

// a 里的值全部小于 b 里的值
int RollingSortedImpl::merge(int a, int b)
{
	if (a < 0)
	{
		return b;
	}
	if (b < 0)
	{
		return a;
	}
	if (nodes_[a].prio > nodes_[b].prio)
	{
		nodes_[a].right = merge(nodes_[a].right, b);
		update(a);
		return a;
	}
	nodes_[b].left = merge(a, nodes_[b].left);
	update(b);
	return b;
}

int RollingSortedImpl::insert(int t, int node)
{
	if (t < 0)
	{
		return node;
	}
	if (nodes_[node].prio > nodes_[t].prio)
	{
		split(t, nodes_[node].value, nodes_[node].left, nodes_[node].right);
		update(node);
		return node;
	}
	if (nodes_[node].value < nodes_[t].value)
	{
		nodes_[t].left = insert(nodes_[t].left, node);
	}
	else
	{
		nodes_[t].right = insert(nodes_[t].right, node);
	}
	update(t);
	return t;
}

int RollingSortedImpl::erase(int t, double value, bool &removed)
{
	if (t < 0)
	{
		removed = false;
		return -1;
	}
	if (nodes_[t].value == value)
	{
		removed = true;
		int merged = merge(nodes_[t].left, nodes_[t].right);
		free_node(t);
		return merged;
	}
	if (value < nodes_[t].value)
	{
		nodes_[t].left = erase(nodes_[t].left, value, removed);
	}
	else
	{
		nodes_[t].right = erase(nodes_[t].right, value, removed);
	}
	update(t);
	return t;
}

int RollingSortedImpl::count_less(int t, double value) const
{
	int less = 0;
	while (t >= 0)
	{
		if (nodes_[t].value < value)
		{
			less += 1 + size_of(nodes_[t].left);
			t = nodes_[t].right;
		}
		else
		{
			t = nodes_[t].left;
		}
	}
	return less;
}

// 第 k 小（0 基），按子树大小迭代下降
double RollingSortedImpl::kth_of(int t, int k) const
{
	while (t >= 0)
	{
		int left_size = size_of(nodes_[t].left);
		if (k < left_size)
		{
			t = nodes_[t].left;
		}
		else if (k == left_size)
		{
			return nodes_[t].value;
		}
		else
		{
			k -= left_size + 1;
			t = nodes_[t].right;
		}
	}
	return NAN;
}

double RollingSortedImpl::kth(int k) const
{
	if (k < 0 || k >= count_)
	{
		return NAN;
	}
	return kth_of(root_, k);
}

// 升序输出窗口内容，迭代式中序，窗口很深也不会爆栈
void RollingSortedImpl::values_ascending(vector<double> &out)
{
	out.clear();
	if (root_ < 0)
	{
		return;
	}
	out.reserve((size_t)count_);
	stack_.clear();
	int t = root_;
	while (t >= 0 || !stack_.empty())
	{
		while (t >= 0)
		{
			stack_.push_back(t);
			t = nodes_[t].left;
		}
		t = stack_.back();
		stack_.pop_back();
		out.push_back(nodes_[t].value);
		t = nodes_[t].right;
	}
}

void RollingSortedImpl::push(double value)
{
	pushes_ += 1;
	if (value_q_->full())
	{
		double pop_value = value_q_->front();
		value_q_->pop();
		if (!::isnan(pop_value))
		{
			bool removed = false;
			root_ = erase(root_, pop_value, removed);
			if (removed)
			{
				count_ -= 1;
			}
		}
	}

	value_q_->push(value);
	last_value_ = value;
	last_is_nan_ = ::isnan(value);
	if (last_is_nan_)
	{
		last_rank_ = NAN;
		return;
	}

	int node = new_node(value);
	root_ = insert(root_, node);
	count_ += 1;
	last_rank_ = (double)count_less(root_, value);
}

RollingSortedData::RollingSortedData()
{
	impl = nullptr;
}

RollingSortedData::~RollingSortedData()
{
	if (impl)
	{
		delete impl;
		impl = nullptr;
	}
}

RollingSortedData::RollingSortedData(const RollingSortedData &rsd)
{
	impl = rsd.impl ? rsd.impl->clone() : nullptr;
}

RollingSortedData &RollingSortedData::operator=(const RollingSortedData &rsd)
{
	if (this == &rsd)
	{
		return *this;
	}
	if (impl)
	{
		delete impl;
		impl = nullptr;
	}
	impl = rsd.impl ? rsd.impl->clone() : nullptr;
	return *this;
}

RollingMedianHelper::RollingMedianHelper(unsigned int window)
{
	window_ = (window == 0) ? 1 : window;
}

RollingMedianHelper::~RollingMedianHelper()
{
	label_q_map_.clear();
}

// 窗口内非 NaN 值的中位数。口径与旧链表版完全一致：
//   * 窗口是"最近 window_ 个压入值"，起始阶段允许未满
//   * 奇数个取中间那个；偶数个取中间两个的平均
//   * 没有任何有效值时返回 NAN
double RollingMedianHelper::cal_rolling_median(double value, int label)
{
	RollingSortedData &data = label_q_map_[label];
	if (data.impl == nullptr)
	{
		data.impl = new RollingSortedImpl(window_);
	}
	data.impl->push(value);

	const int num = data.impl->count();
	if (num == 0)
	{
		return NAN;
	}
	const int lower_index = (num - 1) / 2;
	const double lower = data.impl->kth(lower_index);
	if (num % 2 == 1)
	{
		return lower;
	}
	return (lower + data.impl->kth(lower_index + 1)) / 2.0;
}

RollingRankHelper::RollingRankHelper(unsigned int window)
{
	window_ = (window == 0) ? 1 : window;
}

RollingRankHelper::~RollingRankHelper()
{
	label_q_map_.clear();
}

// 口径：窗口内严格小于 value 的非 NaN 个数（0 基）。窗口未满也照常给值，
// NaN 输入返回 NAN。并列值不再像旧链表实现那样被算进去（旧行为不自洽）。
double RollingRankHelper::cal_rolling_rank(double value, int label)
{
	RollingSortedData &data = label_q_map_[label];
	if (data.impl == nullptr)
	{
		data.impl = new RollingSortedImpl(window_);
	}
	data.impl->push(value);
	if (data.impl->last_is_nan())
	{
		return NAN;
	}
	return data.impl->last_rank();
}

// 标准 Ts_Rank 口径：严格小于的个数 / (有效值个数 - 1) ∈ [0,1]。
// 窗口未满或有效值不足 2 个时无名次可言，返回 NAN。
double RollingRankHelper::cal_rolling_rank_norm(double value, int label)
{
	RollingSortedData &data = label_q_map_[label];
	if (data.impl == nullptr)
	{
		data.impl = new RollingSortedImpl(window_);
	}
	data.impl->push(value);
	if (!data.impl->window_full() || data.impl->last_is_nan())
	{
		return NAN;
	}
	const double num = (double)data.impl->count();
	if (num < 2.0)
	{
		return NAN;
	}
	return data.impl->last_rank() / (num - 1.0);
}

RollingSortedHelper::RollingSortedHelper(unsigned int window)
{
	window_ = (window == 0) ? 1 : window;
}

RollingSortedHelper::~RollingSortedHelper()
{
	label_q_map_.clear();
}

RollingSortedCursor RollingSortedHelper::cal_rolling_median(const double &value, const int &label)
{
	RollingSortedCursor cursor;
	RollingSortedData &data = label_q_map_[label];
	if (data.impl == nullptr)
	{
		data.impl = new RollingSortedImpl(window_);
	}
	data.impl->push(value);

	const int count = data.impl->count();
	if (count <= 0)
	{
		return cursor; // 窗口里还没有可用值
	}
	cursor.label = label;
	cursor.count = count;
	cursor.median_rank = (count - 1) / 2;
	cursor.stamp = data.impl->stamp();
	cursor.valid = true;
	return cursor;
}

bool RollingSortedHelper::get_value(const RollingSortedCursor &cursor
	, vector<double> &left_v
	, vector<double> &right_v)
{
	left_v.clear();
	right_v.clear();
	if (!cursor.valid)
	{
		return false;
	}
	unordered_map<int, RollingSortedData>::iterator it = label_q_map_.find(cursor.label);
	if (it == label_q_map_.end() || it->second.impl == nullptr)
	{
		return false;
	}
	RollingSortedImpl *impl = it->second.impl;
	// 窗口每 bar 都在动，所以旧游标要拒绝掉，不能默默换成另一个窗口的内容。
	// 窗口填满后 count 不再变化，光靠 count 判不出来，所以还要比 stamp。
	if (impl->stamp() != cursor.stamp || impl->count() != cursor.count)
	{
		return false;
	}

	impl->values_ascending(sorted_);
	const int count = (int)sorted_.size();
	if (count <= 0 || count != cursor.count)
	{
		return false;
	}
	const int median_rank = (count - 1) / 2;
	if (median_rank != cursor.median_rank)
	{
		return false;
	}

	left_v.reserve((size_t)count);
	right_v.reserve((size_t)count);
	// 从中位数往下，降序，中位数在前
	for (int k = median_rank; k >= 0; --k)
	{
		left_v.push_back(sorted_[(size_t)k]);
	}
	// 严格大于中位数，升序
	for (int k = median_rank + 1; k < count; ++k)
	{
		right_v.push_back(sorted_[(size_t)k]);
	}
	return true;
}
/// add 20261001 end

/// add 20220222
bool CommonFuncs::GetContractInfo(size_t code, int date, double &price_tick, double &multiply, bool isinit)
{
	static char *p = NULL;
	static int cnt = 0;
	if (!isinit && NULL == p)
	{
		return false;
	}
	if (NULL == p)
	{
		p = (char *)code;
		cnt = *(int *)p;
		return true;
	}
	if (code > cnt)
	{
		return false;
	}
	size_t *pp = (size_t *)(p + 4);
	ContractInfoStruDLL *p1 = (ContractInfoStruDLL *)pp[code];
	ContractInfoStruDLL *pre = p1;
	while (NULL != p1)
	{
		if (p1->date > date)
		{
			price_tick = pre->price_tick;
			multiply = pre->multiply;
			return true;
		}
		else if (NULL == p1->next)
		{
			price_tick = p1->price_tick;
			multiply = p1->multiply;
			return true;
		}
		pre = p1;
		p1 = p1->next;
	}
	return false;
}

int CommonFuncs::GetIndustryCode(size_t code, bool isinit)
{
	static char *p = NULL;
	static int cnt = 0;
	if (!isinit && NULL == p)
	{
		return -2;
	}
	if (NULL == p)
	{
		p = (char *)code;
		cnt = *(int *)p;
		return 0;
	}
	if (code > cnt)
	{
		return -1;
	}
	int *pp = (int *)(p + 4);
	return pp[code];
}

bool CommonFuncs::GetXYValue(size_t code, double &x_val, double &y_val)
{
	static double *px = NULL;
	static double *py = NULL;
	if (x_val == 9527 && y_val == 9527)
	{
		char *p = (char *)code;
		px = *(double **)(p);
		py = *(double **)(p + 8);
		return true;
	}
	if (px == NULL || py == NULL || code > 800000)
	{
		return false;
	}
	x_val = *(px + code);
	y_val = *(py + code);
	return true;
}

/// add 20220222 end
RollingTsstdNodmHelper::RollingTsstdNodmHelper(unsigned int window, bool nan_tsstd, bool unbiased)
{
	window_ = window;
	nan_tsstd_ = nan_tsstd;
	unbiased_ = unbiased;
}

RollingTsstdNodmHelper::RollingTsstdNodmHelper(unsigned int window, bool nan_tsstd)
{
	window_ = window;
	nan_tsstd_ = nan_tsstd;
	unbiased_ = true;
}

RollingTsstdNodmHelper::RollingTsstdNodmHelper(unsigned int window)
{
	window_ = window;
	nan_tsstd_ = false;
	unbiased_ = true;
}

RollingTsstdNodmHelper::~RollingTsstdNodmHelper()
{
}

double RollingTsstdNodmHelper::cal_rolling_tsstdnodm(double value, int label)
{
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end())
	{
		RollingStdData &t_rsd = label_q_map_[label];
		queue_ex<double> *t_value_q = new queue_ex<double>(window_);
		t_value_q->push(value);
		t_rsd.value_q = t_value_q;
		t_rsd.nan_num += ::isnan(value) ? 1 : 0;
		t_rsd.sum += nan_to_zero(value);
		t_rsd.square_sum = t_rsd.square_sum + nan_to_zero(value) * nan_to_zero(value);
	}
	else
	{
		RollingStdData *t_rsd = &label_q_iter->second;
		if (!t_rsd->value_q->full())
		{
			t_rsd->value_q->push(value);
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum += nan_to_zero(value);
			t_rsd->square_sum = t_rsd->square_sum + nan_to_zero(value) * nan_to_zero(value);
			// label_q_map_[label] = t_rsd;
			if (t_rsd->value_q->full())
			{
				if (window_ == t_rsd->nan_num)
				{
					return NAN;
				}
				double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num);
				if (variance < 0)
				{
					variance = 0;
				}
				// double t_mean = NAN;
				double t_std = NAN;
				if (nan_tsstd_ == true)
				{
					if (unbiased_ == true)
					{
						if (window_ == t_rsd->nan_num + 1)
						{
							return NAN;
						}
						t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - 1)));
					}
					else
					{
						if (window_ == t_rsd->nan_num)
						{
							return NAN;
						}
						t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num)));
					}
					if (window_ == t_rsd->nan_num)
					{
						return NAN;
					}
					// t_mean = round_double(t_rsd->sum / (window_ - t_rsd->nan_num));
				}
				else
				{
					if (t_rsd->nan_num == 0)
					{
						if (unbiased_ == true)
						{
							if (window_ == 1)
							{
								return NAN;
							}
							t_std = sqrt(round_double(variance / (window_ - 1)));
						}
						else
						{
							t_std = sqrt(round_double(variance / window_));
						}
						// t_mean = round_double(t_rsd->sum / window_);
					}
					else
					{
						return NAN;
					}
				}
				if (t_std > -1e-12 && t_std < 1e-12)
				{
					return NAN;
				}
				// return (value - t_mean) / t_std;
				return (value) / t_std;
			}
		}
		else
		{
			double tmp_pop_value = t_rsd->value_q->front();
			t_rsd->value_q->pop();
			t_rsd->value_q->push(value);
			t_rsd->nan_num -= ::isnan(tmp_pop_value) ? 1 : 0;
			t_rsd->nan_num += ::isnan(value) ? 1 : 0;
			t_rsd->sum = t_rsd->sum - nan_to_zero(tmp_pop_value) + nan_to_zero(value);
			t_rsd->square_sum = t_rsd->square_sum - nan_to_zero(tmp_pop_value) * nan_to_zero(tmp_pop_value) + nan_to_zero(value) * nan_to_zero(value);
			// label_q_map_[label] = t_rsd;
			if (window_ == t_rsd->nan_num)
			{
				return NAN;
			}
			double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num);
			// double t_mean = NAN;
			double t_std = NAN;
			if (nan_tsstd_ == true)
			{
				if (unbiased_ == true)
				{
					if (window_ == t_rsd->nan_num + 1)
					{
						return NAN;
					}
					t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - 1)));
				}
				else
				{
					if (window_ == t_rsd->nan_num)
					{
						return NAN;
					}
					t_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num)));
				}
				if (window_ == t_rsd->nan_num)
				{
					return NAN;
				}
				// t_mean = round_double(t_rsd->sum / (window_ - t_rsd->nan_num));
			}
			else
			{
				if (t_rsd->nan_num == 0)
				{
					if (unbiased_ == true)
					{
						if (window_ == 1)
						{
							return NAN;
						}
						t_std = sqrt(round_double(variance / (window_ - 1)));
					}
					else
					{
						t_std = sqrt(round_double(variance / window_));
					}
					// t_mean = round_double(t_rsd->sum / window_);
				}
				else
				{
					return NAN;
				}
			}
			if (t_std > -1e-12 && t_std < 1e-12)
			{
				return NAN;
			}
			// return (value - t_mean) / t_std;
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
	if (value_q)
	{
		delete value_q;
		value_q = nullptr;
	}
}

RollingTruncStdData &RollingTruncStdData::operator=(const RollingTruncStdData &rsd)
{
	square_sum = rsd.square_sum;
	sum = rsd.sum;
	nan_num = rsd.nan_num;
	if (value_q)
	{
		delete value_q;
		value_q = nullptr;
	}
	if (rsd.value_q)
	{
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

RollingTruncStdHelper::~RollingTruncStdHelper() {};

double RollingTruncStdHelper::cal_rolling_trunc_tsstd(double value, int label, bool calc_upper_down)
{
	if (window_ < 10)
	{
		return NAN;
	}
	auto label_q_iter = label_q_map_.find(label);
	if (label_q_iter == label_q_map_.end())
	{
		RollingTruncStdData &t_rsd = label_q_map_[label];
		queue_ex<double> *t_value_q = new queue_ex<double>(window_);
		t_value_q->push(value);
		t_rsd.value_q = t_value_q;
		return NAN;
	}
	else
	{
		RollingTruncStdData *t_rsd = &label_q_iter->second;
		if ((!calc_upper_down) && (!t_rsd->is_up_down))
		{ // 还未产生upper_val和down_val 且当前bar不需要计算这两值
			if (t_rsd->value_q->full())
				t_rsd->value_q->pop();
			t_rsd->value_q->push(value);
			return NAN;
		}

		double v_std = 0;
		if (calc_upper_down)
		{ // 每天15:00截面重新计算upper_val和down_val
			if (t_rsd->value_q->full())
			{
				t_rsd->value_q->pop();
			}
			t_rsd->value_q->push(value);

			if (!t_rsd->value_q->full())
			{
				return NAN;
			}
			t_rsd->nan_num = 0;
			t_rsd->invalid_num = 0;
			t_rsd->square_sum = 0;
			t_rsd->sum = 0;
			size_t uh = t_rsd->value_q->get_head_index();
			size_t tail = t_rsd->value_q->get_tail_index();
			double *vet = t_rsd->value_q->get_data_vector();
			vector<double> tmpvec;
			tmpvec.reserve(window_);
			size_t _usize = window_ + 1;
			size_t pre_uh = uh;
			while (uh != tail)
			{
				auto &val = vet[uh];
				if (!(::isnan(val) || ::isinf(val)))
				{
					tmpvec.push_back(val);
				}
				uh = (uh + 1) % _usize;
			}
			std::sort(tmpvec.begin(), tmpvec.end());
			uint32_t tail_size = tmpvec.size() * 0.01; // 去掉首尾1%
			if (tail_size > 0)
			{
				t_rsd->down_val = tmpvec[tail_size - 1];			  // 更新下限值
				t_rsd->upper_val = tmpvec[tmpvec.size() - tail_size]; // 更新上限值
			}
			else
			{
				t_rsd->down_val = *tmpvec.begin();
				t_rsd->upper_val = *tmpvec.rbegin();
			}
			t_rsd->is_up_down = true;
			uh = t_rsd->value_q->get_head_index();
			while (uh != tail)
			{
				auto &val = vet[uh];
				if (::isnan(val) || ::isinf(val))
				{
					t_rsd->nan_num++;
				}
				else if (val <= t_rsd->down_val || val >= t_rsd->upper_val)
				{
					t_rsd->invalid_num++;
				}
				else
				{
					t_rsd->sum += val;
					t_rsd->square_sum += val * val;
				}
				uh = (uh + 1) % _usize;
			}
			double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num - t_rsd->invalid_num);
			if (variance < 0)
			{
				variance = 0;
			}
			if (nan_std_ == true)
			{
				if (unbiased_ == true)
				{
					if (window_ == t_rsd->nan_num + 1)
					{
						return NAN;
					}
					v_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - t_rsd->invalid_num - 1)));
				}
				else
				{
					if (window_ == t_rsd->nan_num)
					{
						return NAN;
					}
					v_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - t_rsd->invalid_num)));
				}
			}
			else
			{
				if (t_rsd->nan_num == 0)
				{
					if (unbiased_ == true)
					{
						if (window_ == 1)
						{
							return NAN;
						}
						v_std = sqrt(round_double(variance / (window_ - t_rsd->invalid_num - 1)));
					}
					else
					{
						v_std = sqrt(round_double(variance / window_ - t_rsd->invalid_num));
					}
				}
				else
				{
					return NAN;
				}
			}
			if (isinf(value) || ::isnan(value))
			{
				return NAN;
			}
			else
			{
				return value / v_std;
			}
		}

		if (!t_rsd->is_up_down)
		{ // 还未产生upper_val和down_val 直接返回nan
			t_rsd->value_q->push(value);
			return NAN;
		}

		if (!t_rsd->value_q->full())
		{ // 队列还未满， 则上限和下限一定还未产生
			t_rsd->value_q->push(value);
			return NAN;
		}
		else
		{
			double tmp_pop_value = t_rsd->value_q->front();
			if (::isnan(tmp_pop_value) || ::isinf(tmp_pop_value))
			{
				t_rsd->nan_num -= 1;
			}
			else if (tmp_pop_value <= t_rsd->down_val || tmp_pop_value >= t_rsd->upper_val)
			{
				t_rsd->invalid_num -= 1;
			}
			else
			{
				t_rsd->sum -= tmp_pop_value;
				t_rsd->square_sum -= tmp_pop_value * tmp_pop_value;
			}
			t_rsd->value_q->pop();
			t_rsd->value_q->push(value);
			if (::isnan(value) || ::isinf(value))
			{
				t_rsd->nan_num += 1;
				return NAN;
			}
			if (value <= t_rsd->down_val || value >= t_rsd->upper_val)
			{
				t_rsd->invalid_num += 1;
			}
			else
			{
				t_rsd->sum += value;
				t_rsd->square_sum += value * value;
			}

			if (window_ == t_rsd->nan_num)
			{
				return NAN;
			}
			double variance = t_rsd->square_sum - t_rsd->sum * t_rsd->sum / (window_ - t_rsd->nan_num - t_rsd->invalid_num);
			if (variance < 0)
			{
				variance = 0;
			}
			if (nan_std_ == true)
			{
				if (unbiased_ == true)
				{
					if (window_ == t_rsd->nan_num + 1)
					{
						return NAN;
					}
					v_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - t_rsd->invalid_num - 1)));
				}
				else
				{
					if (window_ == t_rsd->nan_num)
					{
						return NAN;
					}
					v_std = sqrt(round_double(variance / (window_ - t_rsd->nan_num - t_rsd->invalid_num)));
				}
			}
			else
			{
				if (t_rsd->nan_num == 0)
				{
					if (unbiased_ == true)
					{
						if (window_ == 1)
						{
							return NAN;
						}
						v_std = sqrt(round_double(variance / (window_ - t_rsd->invalid_num - 1)));
					}
					else
					{
						v_std = sqrt(round_double(variance / (window_ - t_rsd->invalid_num)));
					}
				}
				else
				{
					return NAN;
				}
			}
			return value / v_std;
		}
	}
	return NAN;
}