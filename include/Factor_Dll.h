#pragma once
#pragma warning(disable:4251)
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
#define nk_max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef nk_min
#define nk_min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef nk_ISNAN
#define nk_ISNAN(a) ::isnan(a)
#endif

template class EXPORT_CLASS std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
/**
 * @brief Rolling 计算的基础辅助类
 *
 * 提供 NaN / INF 处理、比较、数值修正等通用工具函数，
 * 被所有 Rolling / Cum 类继承使用。
 */
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
	/// NaN 转 0
	double nan_to_zero(double value);
	/// NaN 参与的 >= 比较（自定义规则）
	bool nan_greater_equal(const double & a, const double & b);
	/// NaN 参与的 > 比较
	bool nan_greater(const double & a, const double & b);
	/// NaN 参与的 <= 比较
	bool nan_less_equal(const double & a, const double & b);
	/// NaN 参与的 < 比较
	bool nan_less(const double & a, const double & b);
	/// double 四舍五入（具体精度在 cpp 中定义）
	double round_double(double value);
	/// INF / NaN 转 0
	double inf_nan_to_zero(double value);
};

/**
 * @brief 时间序列 shift（滞后）计算
 *
 * 功能等价于：
 *   pandas.Series.shift(window)
 *
 * 每个 label 对应一个 queue，保存历史值
 */
///
/// @brief 滞后算子
///
/// 返回窗口内第 N 根之前的值（滞后 N 拍），用于构造差分、动量类因子。
///
class EXPORT_CLASS ShiftHelper : public RollingHelper {
private:
	DLL_MAP_DEF<int, queue_ex<double>*> label_q_map_;///< label -> 历史队列
	unsigned int window_;///< 滞后窗口大小
public:
	ShiftHelper(unsigned int window);
	~ShiftHelper();
	/**
	 * @param value 当前值
	 * @param label 序列标识
	 * @return window 步之前的值（不足 window 时通常返回 NaN 或 0）
	 */
	double cal_shift(double value, int label);
};

/**
 * @brief 维护 Rolling window 的原始值列表（list）
 *
 * 不直接计算统计量，仅负责维护窗口内数据
 */
///
/// @brief 通用一元 rolling 算子
///
/// 对窗口内的值施加自定义一元函数（如 log、sqrt）。
///
class EXPORT_CLASS RollingFuncHelper : public RollingHelper {
private:
	unsigned int window_;
	DLL_MAP_DEF<int, list<double>> label_l_map_;
public:
	RollingFuncHelper(unsigned int window);
	~RollingFuncHelper();
	/// 更新 rolling list，并返回当前窗口指针
	list<double>* get_rolling_list(double value, int label);
};

/**
 * @brief Rolling 队列版本（queue_ex）
 *
 * 额外维护 max / min，加速 max/min 计算
 */
///
/// @brief 通用二元 rolling 算子
///
/// 对两路序列在窗口内施加自定义二元函数（如相关、比值）。
///
class EXPORT_CLASS RollingFuncHelper2 : public RollingHelper {
private:
	unsigned int window_;
	DLL_MAP_DEF<int, queue_ex<double> *> label_l_map_;
	DLL_MAP_DEF<int,double> max_map_;
	DLL_MAP_DEF<int,double> min_map_;
public:
	RollingFuncHelper2(unsigned int window);
	~RollingFuncHelper2();
	/// 更新并返回 rolling 队列
	queue_ex<double> * get_rolling_list(double value, int label);
	/// 获取当前 rolling 窗口内最大/最小值
	void get_rolling_max_min(double value, int label, double &max_val, double &min_val);
};

/**
 * @brief Rolling Sum 内部数据结构
 */
class EXPORT_CLASS RollingSumData {
public:
	queue_ex<double> * value_q;///< 窗口内值
	double sum;///< 当前窗口求和
	unsigned int nan_num;///< NaN 个数
	RollingSumData();
	~RollingSumData();
	RollingSumData(const RollingSumData &);
	RollingSumData & operator=(const RollingSumData &);
};

/**
 * @brief Rolling Sum 计算
 *
 * 支持：
 * - 忽略 NaN
 * - NaN 直接传播
 */
///
/// @brief 滑动求和
///
/// 窗口内求和，支持 nan 跳过；因子中用得最多的一类基础算子。
///
class EXPORT_CLASS RollingSumHelper : public RollingHelper {
private:
	DLL_MAP_DEF<int, RollingSumData> label_q_map_;
	unsigned int window_;
	bool nan_sum_;///< 是否忽略 NaN
public:
	RollingSumHelper(unsigned int window, bool nan_sum);
	RollingSumHelper(unsigned int window);
	~RollingSumHelper();
	/// 计算 rolling sum
	double cal_rolling_sum(double value, int label);
};

/**
 * @brief Rolling Std 内部数据
 *
 * 使用 sum / square_sum 在线计算
 */
///
/// @brief 滑动标准差内部状态
///
/// 保存窗口内一阶矩与二阶矩，用于 O(1) 增量更新。
///
class EXPORT_CLASS RollingStdData {
public:
	queue_ex<double>* value_q;
	double square_sum;
	double sum;
	unsigned int nan_num;
	RollingStdData();
	~RollingStdData();
	RollingStdData & operator=(const RollingStdData &);
};

/**
 * @brief Rolling 标准差
 *
 * @param nan_std 是否忽略 NaN
 * @param unbiased 是否使用无偏估计 (n-1)
 */
///
/// @brief 滑动标准差
///
/// 窗口内样本标准差；unbiased=true 时为无偏估计（除以 n-1）。
///
class EXPORT_CLASS RollingStdHelper : public RollingHelper {
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

/**
 * @brief Rolling Correlation 内部数据
 */
class EXPORT_CLASS RollingCorrData {
public:
	queue_ex<double>* value_q1;
	queue_ex<double>* value_q2;
	double square_sum1;
	double square_sum2;
	double sum1;
	double sum2;
	double sum12;
	unsigned int nan_num;
	RollingCorrData();
	~RollingCorrData();
	RollingCorrData & operator=(const RollingCorrData &);
};

/**
 * @brief Rolling Pearson 相关系数
 *
 * corr = (E[XY] - E[X]E[Y]) / (std(X) * std(Y))
 */
///
/// @brief 滑动相关系数
///
/// 窗口内两路序列的 Pearson 相关系数，常用于价量相关类因子。
///
class EXPORT_CLASS RollingCorrHelper : public RollingHelper {
private:
	DLL_MAP_DEF<int, RollingCorrData> label_q_map_;
	unsigned int window_;
	bool nan_corr_;
public:
	RollingCorrHelper(unsigned int window, bool nan_corr);
	RollingCorrHelper(unsigned int window);
	~RollingCorrHelper();
	double cal_rolling_corr(double value1, double value2, int label);//(mean12 - mean1 * mean2) / (std1 * std2);
};


/**
 * @brief Rolling 最大值内部数据
 */
class EXPORT_CLASS RollingMaxData {
public:
	queue_ex<double> * value_l;
	double max_value;
	unsigned int max_value_index; /// 维护的下标
	unsigned int nan_num;
	RollingMaxData();
	~RollingMaxData();
	RollingMaxData & operator=(const RollingMaxData &);
};

/**
 * @brief Rolling 最大值
 */
class EXPORT_CLASS RollingMaxHelper : public RollingHelper {
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

/**
 * @brief Rolling Argmax（最大值所在位置）
 *
 * @param first_argmax 若有多个最大值，是否取第一个
 */
///
/// @brief 滑动最大值位置
///
/// 返回窗口内最大值所在的位置（距当前 bar 的距离）。
///
class EXPORT_CLASS RollingArgmaxHelper : public RollingHelper {
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
class EXPORT_CLASS RollingMinData {
public:
	queue_ex<double> *value_l;
	double min_value;
	unsigned int min_value_index;
	unsigned int nan_num;
	RollingMinData();
	~RollingMinData();
	RollingMinData & operator=(const RollingMinData &);
};

///
/// @brief 滑动最小值
///
/// 窗口内最小值，用于构造区间低点类因子。
///
class EXPORT_CLASS RollingMinHelper : public RollingHelper {
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
class EXPORT_CLASS RollingArgminHelper : public RollingHelper {
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
class EXPORT_CLASS FillNANHelper : public RollingHelper {
private:
	DLL_MAP_DEF<int, double> value_map_;
public:
	FillNANHelper();
	~FillNANHelper();
	double cal_fillnan_value(double value, int label);
};


/**
 * @brief Rolling Mean（基于 RollingSum）
 */
class EXPORT_CLASS RollingMeanHelper : public RollingHelper {
private:
	unordered_map<int, RollingSumData> label_q_map_;
	unsigned int window_;
	bool nan_mean_;
public:
/**
 * @brief 构造函数
 *
 * @param window rolling 窗口长度
 * @param nan_mean 是否忽略 NaN 参与均值计算
 */
	RollingMeanHelper(unsigned int window, bool nan_mean);
	/**
 * @brief 构造函数（默认不忽略 NaN）
 *
 * 当窗口中存在 NaN 时，rolling mean 结果直接为 NaN
 */
	RollingMeanHelper(unsigned int window);
	~RollingMeanHelper();
	/// rolling mean = rolling sum / 有效样本数
	/**
 * @brief 计算 rolling mean
 *
 * 对每一个 label（独立时间序列）：
 * - 维护一个固定长度的滑动窗口
 * - 同时维护窗口内的 sum 与 NaN 数量
 *
 * 计算规则：
 * - nan_mean_ == true :
 *      忽略 NaN，均值 = sum / (window - nan_num)
 * - nan_mean_ == false:
 *      只要窗口中存在 NaN，结果为 NaN
 *
 * @param value 新进入窗口的数值
 * @param label 时间序列标识（如股票、合约、因子实例）
 * @return 当前 rolling mean，窗口未满或非法时返回 NAN
 */
	double cal_rolling_mean(double value, int label);
};

class RollingSumDataPre {
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
class EXPORT_CLASS RollingMeanHelperPre : public RollingHelper {
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


/**
 * @brief Rolling 时间序列标准差（tsstd）
 *
 * 语义上通常用于量化因子标准化
 */
///
/// @brief 滑动时序标准化（时序 z-score）
///
/// 把因子值变成 (x - 窗口均值) / 窗口标准差，是因子标准化的核心算子。
///
class EXPORT_CLASS RollingTsstdHelper : public RollingHelper {
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
class EXPORT_CLASS RollingTsstdNodmHelper : public RollingHelper {
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
class EXPORT_CLASS MultiRollingTsstdHelper {
private:
	DLL_MAP_DEF<int, RollingTsstdHelper*> helper_map_;
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

class CumSumData {
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
class EXPORT_CLASS CumSumHelper : public RollingHelper {
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
class EXPORT_CLASS CumMeanHelper : public RollingHelper {
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

class CumCorrData {
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
class EXPORT_CLASS CumCorrHelper : public RollingHelper {
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

class EXPORT_CLASS CumStdData {
public:
	double square_sum;
	double sum;
	unsigned int nan_num;
	unsigned int num;
	CumStdData();
	~CumStdData();
	CumStdData & operator=(const CumStdData &);
};

///
/// @brief 累计标准差
///
/// 从起点到当前的标准差。
///
class EXPORT_CLASS CumStdHelper : public RollingHelper {
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
//Shen Jiazhen 20200701
class RollingEmaData {
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
class EXPORT_CLASS RollingEmaHelper : public RollingHelper {
private:
	unordered_map<int, RollingEmaData> label_q_map_;
	unsigned int window_;
	double decay_;
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
}TMedianNode;
#endif

class RollingMedianData {
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

class  EXPORT_CLASS RollingMedianHelper : public RollingHelper {
private:
	unordered_map<int, RollingMedianData> label_q_map_;
	unsigned int window_;
public:
	RollingMedianHelper(unsigned int window);
	~RollingMedianHelper();
	double cal_rolling_median(double value, int label);
};

class  EXPORT_CLASS RollingSortedHelper : public RollingHelper {
private:
	unordered_map<int, RollingMedianData> label_q_map_;
	unsigned int window_;
public:
	RollingSortedHelper(unsigned int window);
	~RollingSortedHelper();
	TMedianNode * cal_rolling_median(const double & value
		, const int & label);
};

class EXPORT_CLASS  RollingRankHelper : public RollingHelper {
private:
	unordered_map<int, RollingMedianData> label_q_map_;
	unsigned int window_;
public:
	RollingRankHelper(unsigned int window);
	~RollingRankHelper();
	double cal_rolling_rank(double value, int label);
};


/**
 * @brief 常用统计 / 工具函数集合
 *
 * 主要用于：
 * - list / vector 的统计
 * - Python 风格工具函数
 * - 因子计算辅助
 */
///
/// @brief 通用函数集合
///
/// 数值工具：isnan、截断、安全除法、log/sqrt、归一化等静态函数。
///
class EXPORT_CLASS CommonFuncs
{
public:
	/// 计算 target 在 values 中的分位数 [0,1]
	static double GET_RANK(const list<double> & values, double target);
	static double GET_RANK(const vector<double> & values, double target);
	//从list里面返回arg角标值（复杂度为O(n)，除非必要否则尽量不要用）
	static double GET_VALUE_FROM_LIST(const list<double> & values, unsigned int arg);
	static int GET_VALUE_FROM_LIST(list<int> values, unsigned int arg);
	/// list -> vector
	static vector<double> LIST_TO_VECTOR(list<double> li);
	/// Python-like range
	static vector<double> RANGE(int length);
	static vector<double> RANGE(double length);
	static vector<double> RANGE(double start, double end, double length);
	/// 容器转字符串
	static string ITERATE_TO_STRING(const vector<double> & values);
	static string ITERATE_TO_STRING(const list<double> & values);
	static string ITERATE_TO_STRING(vector<int> values);
	static string ITERATE_TO_STRING(list<int> values);
	static string ITERATE_TO_STRING(vector<string> values);
	static string ITERATE_TO_STRING(list<string> values);
	//TSRANK：返回values最后一个值在values中的排位（0~1）
	static double LAST_RANK(const list<double> & values);
	static double LAST_RANK(const vector<double> & values);
	//list总和
	static double SUM(const list<double> & values);
	static double SUM(const vector<double> & values);
	//list中非nan总和
	static double NAN_SUM(const list<double> & values);
	static double NAN_SUM(const vector<double> & values);
	//list平均值
	static double MEAN(const list<double> & values);
	static double MEAN(const vector<double> & values);
	//list中非nan平均值
	static double NAN_MEAN(const list<double> & values);
	static double NAN_MEAN(const vector<double> & values);
	//list求EMA，decay为衰减
	static double EMA(const list<double> & values, double decay);
	static double EMA(const vector<double> & values, double decay);
    static double EMA(const vector<double> & values, vector<double> & weights, double weightsSum);
	//list求积
	static double PROD(const list<double> & values);
	static double PROD(const vector<double> & values);
	//list求非nan积
	static double NAN_PROD(const list<double> & values);
	static double NAN_PROD(const vector<double> & values);
	//list求std
	static double STD(const list<double> & values);
	static double STD(const vector<double> & values);
	//list求非NAN的std
	static double NAN_STD(const list<double> & values);
	static double NAN_STD(const vector<double> & values);
	//list求correlation
	static double CORR(const list<double> & X, const list<double> & Y);
	static double CORR(const vector<double> & X, const vector<double> & Y);
	//list求非nan元素的correlation
	static double NAN_CORR(const list<double> & X, const list<double> & Y);
	static double NAN_CORR(const vector<double> & X, const vector<double> & Y);
	//list求几何平均
	static double GMEAN(const list<double> & values);
	static double GMEAN(const vector<double> & values);
	//list求非nan几何平均
	static double NAN_GMEAN(const list<double> & values);
	static double NAN_GMEAN(const vector<double> & values);
	//list最大值
	static double NK_MAX(const list<double> & values);
	static double NK_MAX(const vector<double> & values);
	//list最小值
	static double NK_MIN(const list<double> & values);
	static double NK_MIN(const vector<double> & values);
#ifdef _WIN32
	//list最大值
	static double MAX(const list<double> & values);
	static double MAX(const vector<double> & values);
	//list最小值
	static double MIN(const list<double> & values);
	static double MIN(const vector<double> & values);
#endif
	//list求argmax
	static double ARGMAX(const list<double> & values);
	static double ARGMAX(const vector<double> & values);
	//list求argmin
	static double ARGMIN(const list<double> & values);
	static double ARGMIN(const vector<double> & values);
	static int timet_to_3s_bar_num(const time_t & time);
	//TSRANK：返回values最后一个值在values中的排位 返回值[0, N]
	//当x为NAN时, 即[x1, x2, x3, ..., NAN] 返回值 = 0;
	//当x不为NAN时, [x1, x2, x3, ..., x] 返回值[1, N] 包含相等值, 包含自身, 不包含序列中的NAN
	//最高精度1e-6
	//Shen Jiazhen 20200611
	static double LAST_RANK_EQ(const list<double> & values);
	static double LAST_RANK_EQ(const vector<double> & values);
	static double get_rank(const queue_ex<double> * queue, const double & value);
	static double get_last_rank(const queue_ex<double> * queue);
	//商品获取合约信息（最小价格、合约乘数）
	//20220222
	static bool GetContractInfo(size_t code, int date, double &price_tick, double &multiply, bool isinit = false);
	//商品获取行业代码
	static int GetIndustryCode(size_t code, bool isinit = false);
	//获取盘前xy_value
	static bool GetXYValue(size_t code, double &x_val, double &y_val);
	//获取版本号
	static const char * GetVersion();
};


/**
 * @brief Rolling 截断标准差（Truncated Std）计算所需的中间数据结构
 *
 * 该结构用于在 rolling 窗口内维护截断标准差计算的状态，
 * 通常由 RollingTruncStdHelper 按 label 进行管理。
 *
 * 核心思想：
 * - 维护一个滑动窗口（value_q）
 * - 记录窗口内的累积统计量（sum / square_sum）
 * - 在计算 std 前，对异常值进行上下截断（upper_val / down_val）
 */
///
/// @brief 截断标准差内部状态
///
/// 保存截断后的一阶矩与二阶矩。
///
class EXPORT_CLASS RollingTruncStdData {
public:
	queue_ex<double>* value_q;/// rolling 窗口内的原始数据队列（未截断）
	double square_sum;/// 截断后有效数据的平方和 用于 std 计算：E[x^2]
	/// 截断后有效数据的和
	/// 用于 std 计算：E[x]
	double sum;
	/// 上截断阈值（upper bound）
	/// 大于该值的数据在计算 std 时会被截断或视为无效
	double upper_val;
	/// 上截断阈值（upper bound）
	/// 大于该值的数据在计算 std 时会被截断或视为无效
	double down_val;
	/// 窗口内 NaN 的数量
	/// 用于在 nan_std_ 模式下调整有效样本数
	unsigned int nan_num;
	/// 无效值数量（invalid values）
	/// 通常指被截断（超出 upper / down 范围）的样本数
	unsigned int invalid_num;
	/// 是否已经计算并初始化过上下截断阈值
	/// true  : upper_val / down_val 已有效
	/// false : 尚未计算，可能需要重新估计
	bool is_up_down;
	RollingTruncStdData();
	~RollingTruncStdData();
	RollingTruncStdData & operator=(const RollingTruncStdData &);
};


/**
 * @brief Rolling 截断标准差（Truncated Rolling Std / TsStd）
 *
 * 用于在 rolling 窗口内计算“截断后的”时间序列标准差，
 * 即在计算 std 之前，先对极端值进行上下截断（winsorize / truncate）。
 *
 * 常见用途：
 * - 因子去极值（防止极端 tick / 错单 放大波动）
 * - 高频因子标准化前的稳健波动率估计
 *
 * 计算流程（逻辑说明，具体以 cpp 实现为准）：
 * 1. 维护长度为 window_ 的 rolling 队列
 * 2. 统计窗口内数据的上下截断阈值（upper / down）
 * 3. 对超过阈值的数据进行截断
 * 4. 基于截断后的数据计算标准差
 *
 * 与 RollingTsstdHelper 的区别：
 * - RollingTsstd：直接使用原始值计算 std
 * - RollingTruncStd：先做上下截断，再计算 std（对异常值更鲁棒）
 */
///
/// @brief 截断滑动标准差
///
/// 先按分位数截断极值再求标准差，用于抑制异常值影响。
///
class EXPORT_CLASS RollingTruncStdHelper : public RollingHelper {
private:
	DLL_MAP_DEF<int, RollingTruncStdData> label_q_map_;/// label -> rolling 截断标准差计算所需的中间数据
	unsigned int window_;
	/// 是否忽略 NaN 值
	/// true  : NaN 不参与 std 计算
	/// false : 窗口内存在 NaN 时结果可能为 NaN
	bool nan_std_;
	/// 是否使用无偏估计
	/// true  : 使用 (n - 1) 作为分母
	/// false : 使用 n 作为分母
	bool unbiased_;
public:
	/**
	 * @brief 构造函数（完整参数）
	 *
	 * @param window       rolling 窗口长度
	 * @param nan_std      是否忽略 NaN
	 * @param unbiased     是否使用无偏标准差
	 */
	RollingTruncStdHelper(unsigned int window, bool nan_std, bool unbiased);
	/**
	 * @brief 构造函数（默认有偏 / 或 cpp 中定义的默认行为）
	 *
	 * @param window       rolling 窗口长度
	 * @param nan_std      是否忽略 NaN
	 */
	RollingTruncStdHelper(unsigned int window, bool nan_std);
	/**
	 * @brief 构造函数（仅指定窗口，其余使用默认参数）
	 *
	 * @param window rolling 窗口长度
	 */
	RollingTruncStdHelper(unsigned int window);
	~RollingTruncStdHelper();
	/**
	 * @brief 计算 rolling 截断时间序列标准差
	 *
	 * @param value              当前时刻输入值
	 * @param label              序列标识（symbol / 因子实例）
	 * @param calc_upper_down    是否在本次调用中重新计算上下截断阈值
	 *
	 * @return 当前窗口对应的截断标准差
	 *
	 * 说明：
	 * - 每次调用会将 value 推入 rolling 窗口
	 * - 当窗口未满时，通常返回 NaN 或不完整结果（依 cpp 实现）
	 * - calc_upper_down 一般用于控制：
	 *     - 是否动态更新截断边界
	 *     - 或在特定时刻（如窗口刚满）才重新计算
	 */
	double cal_rolling_trunc_tsstd(double value, int label, bool calc_upper_down);
};
#ifdef SN_Factor_Dll_DEF
#include "SN_Factor_Dll.h"
#include "SN_half_rolling.h"
#endif