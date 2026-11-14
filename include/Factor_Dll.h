#pragma once
#pragma warning(disable : 4251)
#include "stdafx.h"
#include "queue_ex.hpp"
#ifdef _WIN32
#define EXPORT_CLASS __declspec(dllexport)
#define IMPORT_CLASS __declspec(dllimport)
#else
#define EXPORT_CLASS
#define IMPORT_CLASS
#endif

#ifndef DLL_MAP_DEF
#define DLL_MAP_DEF unordered_map
#endif

#ifndef nk_max
#define nk_max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef nk_min
#define nk_min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef nk_ISNAN
#define nk_ISNAN(a) ::isnan(a)
#endif

template class EXPORT_CLASS std::basic_string<char, std::char_traits<char>, std::allocator<char>>;

///
/// @brief 滑动窗口算子基类
///
/// 所有 rolling 算子的公共基类，仅提供虚析构，不含状态。
///
class EXPORT_CLASS RollingHelper
{
public:
	RollingHelper();
	~RollingHelper();
	double nan_to_zero(double value);
	bool nan_greater_equal(const double &a, const double &b);
	bool nan_greater(const double &a, const double &b);
	bool nan_less_equal(const double &a, const double &b);
	bool nan_less(const double &a, const double &b);
	double round_double(double value);
	double inf_nan_to_zero(double value);
};

///
/// @brief 滞后算子
///
/// 返回窗口内第 N 根之前的值（滞后 N 拍），用于构造差分、动量类因子。
///
class EXPORT_CLASS ShiftHelper : public RollingHelper
{
private:
	DLL_MAP_DEF<int, queue_ex<double> *> label_q_map_;
	unsigned int window_;

public:
	ShiftHelper(unsigned int window);
	~ShiftHelper();
	double cal_shift(double value, int label);
};

///
/// @brief 通用一元 rolling 算子
///
/// 对窗口内的值施加自定义一元函数（如 log、sqrt）。
///
class EXPORT_CLASS RollingFuncHelper : public RollingHelper
{
private:
	unsigned int window_;
	DLL_MAP_DEF<int, list<double>> label_l_map_;

public:
	RollingFuncHelper(unsigned int window);
	~RollingFuncHelper();
	list<double> *get_rolling_list(double value, int label);
};

///
/// @brief 通用二元 rolling 算子
///
/// 对两路序列在窗口内施加自定义二元函数（如相关、比值）。
///
class EXPORT_CLASS RollingFuncHelper2 : public RollingHelper
{
private:
	unsigned int window_;
	DLL_MAP_DEF<int, queue_ex<double> *> label_l_map_;
	DLL_MAP_DEF<int, double> max_map_;
	DLL_MAP_DEF<int, double> min_map_;

public:
	RollingFuncHelper2(unsigned int window);
	~RollingFuncHelper2();
	queue_ex<double> *get_rolling_list(double value, int label);
	void get_rolling_max_min(double value, int label, double &max_val, double &min_val);
};

///
/// @brief 滑动求和的内部状态
///
/// 保存窗口内累加值与有效样本数。
///
class EXPORT_CLASS RollingSumData
{
public:
	queue_ex<double> *value_q;
	double sum;
	unsigned int nan_num;
	RollingSumData();
	~RollingSumData();
	RollingSumData(const RollingSumData &);
	RollingSumData &operator=(const RollingSumData &);
};

///
/// @brief 滑动求和
///
/// 窗口内求和，支持 nan 跳过；因子中用得最多的一类基础算子。
///
class EXPORT_CLASS RollingSumHelper : public RollingHelper
{
private:
	DLL_MAP_DEF<int, RollingSumData> label_q_map_;
	unsigned int window_;
	bool nan_sum_;

public:
	RollingSumHelper(unsigned int window, bool nan_sum);
	RollingSumHelper(unsigned int window);
	~RollingSumHelper();
	double cal_rolling_sum(double value, int label);
};

///
/// @brief 滑动标准差内部状态
///
/// 保存窗口内一阶矩与二阶矩，用于 O(1) 增量更新。
///
class EXPORT_CLASS RollingStdData
{
public:
	queue_ex<double> *value_q;
	double square_sum;
	double sum;
	unsigned int nan_num;
	RollingStdData();
	~RollingStdData();
	RollingStdData &operator=(const RollingStdData &);
};

///
/// @brief 滑动标准差
///
/// 窗口内样本标准差；unbiased=true 时为无偏估计（除以 n-1）。
///
class EXPORT_CLASS RollingStdHelper : public RollingHelper
{
private:
	DLL_MAP_DEF<int, RollingStdData> label_q_map_;
	unsigned int window_;
	bool nan_std_;
	bool unbiased_;

public:
	RollingStdHelper(unsigned int window, bool nan_std, bool unbiased);
	RollingStdHelper(unsigned int window, bool nan_std);
	RollingStdHelper(unsigned int window);
	~RollingStdHelper();
	double cal_rolling_std(double value, int label);
};

///
/// @brief 滑动相关内部状态
///
/// 保存两路序列的均值、方差与协方差。
///
class EXPORT_CLASS RollingCorrData
{
public:
	queue_ex<double> *value_q1;
	queue_ex<double> *value_q2;
	double square_sum1;
	double square_sum2;
	double sum1;
	double sum2;
	double sum12;
	unsigned int nan_num;
	RollingCorrData();
	~RollingCorrData();
	RollingCorrData &operator=(const RollingCorrData &);
};

///
/// @brief 滑动相关系数
///
/// 窗口内两路序列的 Pearson 相关系数，常用于价量相关类因子。
///
class EXPORT_CLASS RollingCorrHelper : public RollingHelper
{
private:
	DLL_MAP_DEF<int, RollingCorrData> label_q_map_;
	unsigned int window_;
	bool nan_corr_;

public:
	RollingCorrHelper(unsigned int window, bool nan_corr);
	RollingCorrHelper(unsigned int window);
	~RollingCorrHelper();
	double cal_rolling_corr(double value1, double value2, int label); //(mean12 - mean1 * mean2) / (std1 * std2);
};

///
/// @brief 滑动最大值内部状态
///
/// 单调队列，支持 O(1) 摊销的窗口最大值。
///
class EXPORT_CLASS RollingMaxData
{
public:
	queue_ex<double> *value_l;
	double max_value;
	unsigned int max_value_index; /// ά�����±�
	unsigned int nan_num;
	RollingMaxData();
	~RollingMaxData();
	RollingMaxData &operator=(const RollingMaxData &);
};

///
/// @brief 滑动最大值
///
/// 窗口内最大值，用于构造突破、区间高点类因子。
///
class EXPORT_CLASS RollingMaxHelper : public RollingHelper
{
private:
	DLL_MAP_DEF<int, RollingMaxData> label_l_map_;
	unsigned int window_;
	bool nan_max_;

public:
	RollingMaxHelper(unsigned int window, bool nan_max);
	RollingMaxHelper(unsigned int window);
	~RollingMaxHelper();
	double cal_rolling_max(double value, int label);
};

///
/// @brief 滑动最大值位置
///
/// 返回窗口内最大值所在的位置（距当前 bar 的距离）。
///
class EXPORT_CLASS RollingArgmaxHelper : public RollingHelper
{
private:
	DLL_MAP_DEF<int, RollingMaxData> label_l_map_;
	unsigned int window_;
	bool nan_argmax_;
	bool first_argmax_;

public:
	RollingArgmaxHelper(unsigned int window, bool nan_argmax, bool first_argmax);
	RollingArgmaxHelper(unsigned int window, bool nan_argmax);
	RollingArgmaxHelper(unsigned int window);
	~RollingArgmaxHelper();
	double cal_rolling_argmax(double value, int label);
};

///
/// @brief 滑动最小值内部状态
///
/// 单调队列，支持 O(1) 摊销的窗口最小值。
///
class EXPORT_CLASS RollingMinData
{
public:
	queue_ex<double> *value_l;
	double min_value;
	unsigned int min_value_index;
	unsigned int nan_num;
	RollingMinData();
	~RollingMinData();
	RollingMinData &operator=(const RollingMinData &);
};

///
/// @brief 滑动最小值
///
/// 窗口内最小值，用于构造区间低点类因子。
///
class EXPORT_CLASS RollingMinHelper : public RollingHelper
{
private:
	DLL_MAP_DEF<int, RollingMinData> label_l_map_;
	unsigned int window_;
	bool nan_min_;

public:
	RollingMinHelper(unsigned int window, bool nan_min);
	RollingMinHelper(unsigned int window);
	~RollingMinHelper();
	double cal_rolling_min(double value, int label);
};

///
/// @brief 滑动最小值位置
///
/// 返回窗口内最小值所在位置。
///
class EXPORT_CLASS RollingArgminHelper : public RollingHelper
{
private:
	DLL_MAP_DEF<int, RollingMinData> label_l_map_;
	unsigned int window_;
	bool nan_argmin_;
	bool first_argmin_;

public:
	RollingArgminHelper(unsigned int window, bool nan_argmin, bool first_argmin);
	RollingArgminHelper(unsigned int window, bool nan_argmin);
	RollingArgminHelper(unsigned int window);
	~RollingArgminHelper();
	double cal_rolling_argmin(double value, int label);
};

///
/// @brief 缺失值填充
///
/// 按给定策略填补 NaN（前值/零/均值），保证下游算子有稳定输入。
///
class EXPORT_CLASS FillNANHelper : public RollingHelper
{
private:
	DLL_MAP_DEF<int, double> value_map_;

public:
	FillNANHelper();
	~FillNANHelper();
	double cal_fillnan_value(double value, int label);
};

///
/// @brief 滑动均值
///
/// 窗口内算术平均，最基础的平滑算子。
///
class EXPORT_CLASS RollingMeanHelper : public RollingHelper
{
private:
	unordered_map<int, RollingSumData> label_q_map_;
	unsigned int window_;
	bool nan_mean_;

public:
	RollingMeanHelper(unsigned int window, bool nan_mean);
	RollingMeanHelper(unsigned int window);
	~RollingMeanHelper();
	double cal_rolling_mean(double value, int label);
};

class RollingSumDataPre
{
public:
	queue<double> value_q;
	double sum;
	unsigned int nan_num;
	RollingSumDataPre();
};

///
/// @brief 滑动均值（前缀和版）
///
/// 用前缀和实现，适合需要长期历史且窗口固定的场景。
///
class EXPORT_CLASS RollingMeanHelperPre : public RollingHelper
{
private:
	DLL_MAP_DEF<int, RollingSumDataPre> label_q_map_;
	unsigned int window_;
	bool nan_mean_;

public:
	RollingMeanHelperPre(unsigned int window, bool nan_mean);
	RollingMeanHelperPre(unsigned int window);
	~RollingMeanHelperPre();
	double cal_rolling_mean(double value, int label);
};

///
/// @brief 滑动时序标准化（时序 z-score）
///
/// 把因子值变成 (x - 窗口均值) / 窗口标准差，是因子标准化的核心算子。
///
class EXPORT_CLASS RollingTsstdHelper : public RollingHelper
{
private:
	DLL_MAP_DEF<int, RollingStdData> label_q_map_;
	unsigned int window_;
	bool nan_tsstd_;
	bool unbiased_;

public:
	RollingTsstdHelper(unsigned int window, bool nan_tsstd, bool unbiased);
	RollingTsstdHelper(unsigned int window, bool nan_tsstd);
	RollingTsstdHelper(unsigned int window);
	~RollingTsstdHelper();
	double cal_rolling_tsstd(double value, int label);
};

///
/// @brief 滑动时序标准化（不减均值）
///
/// 只除以窗口标准差，保留因子自身的均值信息。
///
class EXPORT_CLASS RollingTsstdNodmHelper : public RollingHelper
{
private:
	DLL_MAP_DEF<int, RollingStdData> label_q_map_;
	unsigned int window_;
	bool nan_tsstd_;
	bool unbiased_;

public:
	RollingTsstdNodmHelper(unsigned int window, bool nan_tsstd, bool unbiased);
	RollingTsstdNodmHelper(unsigned int window, bool nan_tsstd);
	RollingTsstdNodmHelper(unsigned int window);
	~RollingTsstdNodmHelper();
	double cal_rolling_tsstdnodm(double value, int label);
};

///
/// @brief 多标签滑动时序标准化
///
/// 为多个 label（品种/合约）各自维护一套滚动窗口，一次调用完成分组标准化。
///
class EXPORT_CLASS MultiRollingTsstdHelper
{
private:
	DLL_MAP_DEF<int, shared_ptr<RollingTsstdHelper>> helper_map_;
	unsigned int window_;
	bool nan_tsstd_;
	bool unbiased_;

public:
	MultiRollingTsstdHelper(unsigned int window, bool nan_tsstd, bool unbiased);
	MultiRollingTsstdHelper(unsigned int window, bool nan_tsstd);
	MultiRollingTsstdHelper(unsigned int window);
	~MultiRollingTsstdHelper();
	double cal_multi_rolling_tsstd(double value, int label, int tsstd_label);
};
///// add 20200622 beg

class CumSumData
{
public:
	double sum;
	unsigned int num;
	CumSumData();
};

///
/// @brief 累计求和
///
/// 从序列起点累计，用于构造累计量类因子。
///
class EXPORT_CLASS CumSumHelper : public RollingHelper
{
private:
	unordered_map<int, CumSumData> label_map_;
	unsigned int min_periods_;

public:
	CumSumHelper();
	CumSumHelper(unsigned int min_periods);
	~CumSumHelper();
	double cal_cum_sum(double value, int label);
	int reset(int label);
	void SN_reset();
};

///
/// @brief 累计均值
///
/// 从起点到当前的均值。
///
class EXPORT_CLASS CumMeanHelper : public RollingHelper
{
private:
	unordered_map<int, CumSumData> label_map_;
	unsigned int min_periods_;

public:
	CumMeanHelper();
	CumMeanHelper(unsigned int min_periods);
	~CumMeanHelper();
	double cal_cum_mean(double value, int label);
	int reset(int label);
	void SN_reset();
};

class CumCorrData
{
public:
	double square_sum1;
	double square_sum2;
	double sum1;
	double sum2;
	double sum12;
	unsigned int num;
	CumCorrData();
};

///
/// @brief 累计相关
///
/// 从起点到当前的两路相关系数。
///
class EXPORT_CLASS CumCorrHelper : public RollingHelper
{
private:
	unordered_map<int, CumCorrData> label_map_;
	unsigned int min_periods_;

public:
	CumCorrHelper();
	CumCorrHelper(unsigned int min_periods);
	~CumCorrHelper();
	double cal_cum_corr(double value1, double value2, int label);
	int reset(int label);
	void SN_reset();
};
///// add 20200622 end

class EXPORT_CLASS CumStdData
{
public:
	double square_sum;
	double sum;
	unsigned int nan_num;
	unsigned int num;
	CumStdData();
	~CumStdData();
	CumStdData &operator=(const CumStdData &);
};

///
/// @brief 累计标准差
///
/// 从起点到当前的标准差。
///
class EXPORT_CLASS CumStdHelper : public RollingHelper
{
private:
	DLL_MAP_DEF<int, CumStdData> label_q_map_;
	unsigned int min_periods_;
	bool nan_std_;
	bool unbiased_;

public:
	CumStdHelper(unsigned int min_periods = 1, bool nan_std = false, bool unbiased = true);
	~CumStdHelper();
	double cal_cum_std(double value, int label);
	int reset(int label);
	void SN_reset();
};

/// add 20200701 begin
// Shen Jiazhen 20200701
class RollingEmaData
{
public:
	queue<double> value_q;
	double mean;
	unsigned int nan_num;
	RollingEmaData();
};

///
/// @brief 指数滑动平均 (EMA)
///
/// 按 alpha 递推平滑，等价于无穷窗口的加权平均。
///
class EXPORT_CLASS RollingEmaHelper : public RollingHelper
{
private:
	unordered_map<int, RollingEmaData> label_q_map_;
	unsigned int window_;
	double decay_;
	// window 与 decay 都是构造期定下来的，所以 p = decay/(1+decay) 与 pow(p, window)
	// 这两个每 bar 都要用的常数在构造时算好，热路径上不再调 pow
	double p_;
	double p_pow_window_;

public:
	RollingEmaHelper(unsigned int window, double decay);
	~RollingEmaHelper();
	double cal_rolling_ema(double value, int label);
};
/// add 20200701 end

#ifndef _STU_TagMedianNode
#define _STU_TagMedianNode
typedef struct TagMedianNode
{
	struct TagMedianNode *pre;
	struct TagMedianNode *next;
	double value;
} TMedianNode;
#endif

class RollingMedianData
{
public:
	queue<TMedianNode *> value_q;
	TMedianNode *pMedian;
	unsigned int num;
	RollingMedianData()
	{
		pMedian = NULL;
		num = 0;
	}
};

/// add 20261001 begin
// 顺序统计窗口：fifo + 顺序统计 treap（带子树大小的随机化 BST）。
// insert/erase/rank/kth 期望 O(log n)；节点经 free list 复用，窗口满后每 bar 不再分配。
// NaN 不入树，只缩短有效窗口。实现（RollingSortedImpl）在 Factor_Dll.cpp 内。
// 由 RollingMedianHelper / RollingRankHelper / RollingSortedHelper 共用。
class RollingSortedImpl;

class EXPORT_CLASS RollingSortedData
{
public:
	RollingSortedImpl *impl;
	RollingSortedData();
	~RollingSortedData();
	RollingSortedData(const RollingSortedData &);
	RollingSortedData &operator=(const RollingSortedData &);
};

// 指向某次 cal_rolling_median 时窗口内中位数的游标。
// stamp 是窗口的压入计数，用来拒绝"上一根 bar 的旧游标"（窗口满后 count 不再变化，只靠 count 判不出来）。
struct RollingSortedCursor
{
	int label;
	int median_rank; // 中位数在窗口内的 0 基名次
	int count;       // 窗口内非 NaN 个数
	unsigned long long stamp;
	bool valid;
	RollingSortedCursor() : label(0), median_rank(-1), count(0), stamp(0), valid(false) {}
};
/// add 20261001 end

class EXPORT_CLASS RollingMedianHelper : public RollingHelper
{
private:
	unordered_map<int, RollingSortedData> label_q_map_;
	unsigned int window_;

public:
	RollingMedianHelper(unsigned int window);
	~RollingMedianHelper();
	double cal_rolling_median(double value, int label);
};

class EXPORT_CLASS RollingSortedHelper : public RollingHelper
{
private:
	unordered_map<int, RollingSortedData> label_q_map_;
	unsigned int window_;
	vector<double> sorted_;

public:
	RollingSortedHelper(unsigned int window);
	~RollingSortedHelper();
	// 送入一个值并返回指向窗口中位数的游标（窗口内没有非 NaN 值时 valid=false）。
	RollingSortedCursor cal_rolling_median(const double &value, const int &label);
	// 按游标把窗口从中位数劈成两半：left_v 从大到小（中位数在前），right_v 从小到大
	// （严格大于中位数）。与旧 TMedianNode 的 pre/next 行走口径一致。
	// 游标无效或已过期返回 false。
	bool get_value(const RollingSortedCursor &cursor, vector<double> &left_v, vector<double> &right_v);
	//// ��ȡ��������left_v�ǴӴ�С��right_v��С����
	// bool GetValue(TMedianNode * p
	//	, vector<double> & left_v
	//	, vector<double> & right_v) {
	//	if (!p) {
	//		return false;
	//	}
	//	TMedianNode * pForeachNode = p;
	//	left_v.clear();
	//	while (pForeachNode != NULL)
	//	{
	//		left_v.emplace_back(pForeachNode->value);
	//		pForeachNode = pForeachNode->pre;
	//	}
	//	right_v.clear();
	//	pForeachNode = p->next;
	//	while (pForeachNode) {
	//		right_v.emplace_back(pForeachNode->value);
	//		pForeachNode = pForeachNode->next;
	//	}
	//	return true;
	// }
	///// ��С�����ӡ����
	// for (uint32_t x = 0; x < left_v.size(); ++x) {
	//	std::cout << left_v[left_v.size() - 1 - x] << ",";
	// }
	// for (int y = 0; y < right_v.size(); ++y) {
	//	cout << right_v[y] << ",";
	// }
};

///
/// @brief 滑动分位排名
///
/// 返回当前值在窗口内的分位（0~1），对异常值不敏感。
///
class EXPORT_CLASS RollingRankHelper : public RollingHelper
{
private:
	unordered_map<int, RollingSortedData> label_q_map_;
	unsigned int window_;

public:
	RollingRankHelper(unsigned int window);
	~RollingRankHelper();
	// 窗口内严格小于 value 的非 NaN 个数（0 基）。NaN 输入返回 NAN。
	// 这是标准 Ts_Rank 的分子，也与 CalcTools 的 last_rank 定义一致。
	// 注：旧链表实现对"并列值"的结果不自洽（同一窗口形状会给出不同答案，
	// 属于 median 行走的副产物），这里统一改成严格小于。
	double cal_rolling_rank(double value, int label);
	// 名次 / (窗口内非 NaN 个数 - 1) ∈ [0,1]，名次取"严格小于"的个数；
	// 窗口未满或有效值不足 2 个返回 NAN。
	double cal_rolling_rank_norm(double value, int label);
};

///
/// @brief 通用函数集合
///
/// 数值工具：isnan、截断、安全除法、log/sqrt、归一化等静态函数。
///
class EXPORT_CLASS CommonFuncs
{
public:
	// ����target��values�е���λ��0~1��
	static double GET_RANK(const list<double> &values, double target);
	static double GET_RANK(const vector<double> &values, double target);
	// ��list���淵��arg�Ǳ�ֵ�����Ӷ�ΪO(n)�����Ǳ�Ҫ��������Ҫ�ã�
	static double GET_VALUE_FROM_LIST(const list<double> &values, unsigned int arg);
	static int GET_VALUE_FROM_LIST(list<int> values, unsigned int arg);
	// listתvector
	static vector<double> LIST_TO_VECTOR(list<double> li);
	// Python-like Range
	static vector<double> RANGE(int length);
	static vector<double> RANGE(double length);
	static vector<double> RANGE(double start, double end, double length);
	// stringת��
	static string ITERATE_TO_STRING(const vector<double> &values);
	static string ITERATE_TO_STRING(const list<double> &values);
	static string ITERATE_TO_STRING(vector<int> values);
	static string ITERATE_TO_STRING(list<int> values);
	static string ITERATE_TO_STRING(vector<string> values);
	static string ITERATE_TO_STRING(list<string> values);
	// TSRANK������values���һ��ֵ��values�е���λ��0~1��
	static double LAST_RANK(const list<double> &values);
	static double LAST_RANK(const vector<double> &values);
	// list�ܺ�
	static double SUM(const list<double> &values);
	static double SUM(const vector<double> &values);
	// list�з�nan�ܺ�
	static double NAN_SUM(const list<double> &values);
	static double NAN_SUM(const vector<double> &values);
	// listƽ��ֵ
	static double MEAN(const list<double> &values);
	static double MEAN(const vector<double> &values);
	// list�з�nanƽ��ֵ
	static double NAN_MEAN(const list<double> &values);
	static double NAN_MEAN(const vector<double> &values);
	// list��EMA��decayΪ˥��
	static double EMA(const list<double> &values, double decay);
	static double EMA(const vector<double> &values, double decay);
	static double EMA(const vector<double> &values, vector<double> &weights, double weightsSum);
	// list���
	static double PROD(const list<double> &values);
	static double PROD(const vector<double> &values);
	// list���nan��
	static double NAN_PROD(const list<double> &values);
	static double NAN_PROD(const vector<double> &values);
	// list��std
	static double STD(const list<double> &values);
	static double STD(const vector<double> &values);
	// list���NAN��std
	static double NAN_STD(const list<double> &values);
	static double NAN_STD(const vector<double> &values);
	// list��correlation
	static double CORR(const list<double> &X, const list<double> &Y);
	static double CORR(const vector<double> &X, const vector<double> &Y);
	// list���nanԪ�ص�correlation
	static double NAN_CORR(const list<double> &X, const list<double> &Y);
	static double NAN_CORR(const vector<double> &X, const vector<double> &Y);
	// list�󼸺�ƽ��
	static double GMEAN(const list<double> &values);
	static double GMEAN(const vector<double> &values);
	// list���nan����ƽ��
	static double NAN_GMEAN(const list<double> &values);
	static double NAN_GMEAN(const vector<double> &values);
	// list���ֵ
	static double NK_MAX(const list<double> &values);
	static double NK_MAX(const vector<double> &values);
	// list��Сֵ
	static double NK_MIN(const list<double> &values);
	static double NK_MIN(const vector<double> &values);
#ifdef _WIN32
	// list���ֵ
	static double MAX(const list<double> &values);
	static double MAX(const vector<double> &values);
	// list��Сֵ
	static double MIN(const list<double> &values);
	static double MIN(const vector<double> &values);
#endif
	// list��argmax
	static double ARGMAX(const list<double> &values);
	static double ARGMAX(const vector<double> &values);
	// list��argmin
	static double ARGMIN(const list<double> &values);
	static double ARGMIN(const vector<double> &values);
	static int timet_to_3s_bar_num(const time_t &time);
	// TSRANK������values���һ��ֵ��values�е���λ ����ֵ[0, N]
	// ��xΪNANʱ, ��[x1, x2, x3, ..., NAN] ����ֵ = 0;
	// ��x��ΪNANʱ, [x1, x2, x3, ..., x] ����ֵ[1, N] �������ֵ, ��������, �����������е�NAN
	// ��߾���1e-6
	// Shen Jiazhen 20200611
	static double LAST_RANK_EQ(const list<double> &values);
	static double LAST_RANK_EQ(const vector<double> &values);
	static double get_rank(const queue_ex<double> *queue, const double &value);
	static double get_last_rank(const queue_ex<double> *queue);
	// ��Ʒ��ȡ��Լ��Ϣ����С�۸񡢺�Լ������
	// 20220222
	static bool GetContractInfo(size_t code, int date, double &price_tick, double &multiply, bool isinit = false);
	// ��Ʒ��ȡ��ҵ����
	static int GetIndustryCode(size_t code, bool isinit = false);
	// ��ȡ��ǰxy_value
	static bool GetXYValue(size_t code, double &x_val, double &y_val);
	// ��ȡ�汾��
	static const char *GetVersion();
};

///
/// @brief 截断标准差内部状态
///
/// 保存截断后的一阶矩与二阶矩。
///
class EXPORT_CLASS RollingTruncStdData
{
public:
	queue_ex<double> *value_q;
	double square_sum;
	double sum;
	double upper_val; // ����ֵ
	double down_val;  // ����ֵ
	unsigned int nan_num;
	unsigned int invalid_num; // ��Чֵ����
	bool is_up_down;
	RollingTruncStdData();
	~RollingTruncStdData();
	RollingTruncStdData &operator=(const RollingTruncStdData &);
};

///
/// @brief 截断滑动标准差
///
/// 先按分位数截断极值再求标准差，用于抑制异常值影响。
///
class EXPORT_CLASS RollingTruncStdHelper : public RollingHelper
{
private:
	DLL_MAP_DEF<int, RollingTruncStdData> label_q_map_;
	unsigned int window_;
	bool nan_std_;
	bool unbiased_;

public:
	RollingTruncStdHelper(unsigned int window, bool nan_std, bool unbiased);
	RollingTruncStdHelper(unsigned int window, bool nan_std);
	RollingTruncStdHelper(unsigned int window);
	~RollingTruncStdHelper();
	double cal_rolling_trunc_tsstd(double value, int label, bool calc_upper_down);
};
#ifdef SN_Factor_Dll_DEF
#include "SN_Factor_Dll.h"
#include "SN_half_rolling.h"
#endif