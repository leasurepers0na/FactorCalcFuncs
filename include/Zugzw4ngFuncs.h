#pragma once
#include "Factor_Dll.h"
#include "FactorParamDef.h"

#include <algorithm>

namespace Zugzw4ngFuncs
{
	bool is_prime(int a);
	int getOverPrime(int n, int count);
	void add_1min_factor(FactorCreator *fc, int max_bar, vector<FactorCreator *> &v_fc);
	void add_1min_factor(FactorCreator *fc, vector<FactorCreator *> &v_fc);
	void add_ti_factor(FactorCreator *fc, vector<FactorCreator *> &v_fc);
	string replace(string str, string sub1, string sub2);
	bool is_in(string s_sub, const string &str);
	bool is_in(string s, const vector<string> &s_vec);
	bool is_in(int s, const vector<int> &s_vec);
	bool is_in(double s, const vector<double> &s_vec);
	int getClockTime(time_t time);
	int getClockHour(time_t time);
	tm getTm(time_t time);
	int get_weekday(time_t time);
	float Q_rsqrt(float number);
	double Q_rsqrt_double(double number);
	void filter_factor_list(vector<string> factor_list, vector<FactorCreator *> &v_fc);
	void filter_factor_list_rev(vector<string> factor_list, vector<FactorCreator *> &v_fc);
	int getBarNum(time_t time, int time_period);
	int getClockTime_3s(time_t time);
	int getSecTickNum(time_t time);
	double cal_slope(vector<double> &x, vector<double> &y);
	double cal_slope(vector<double> &x, double &x_mean, vector<double> &y, double &y_mean);
	int extract_int_from_string(const string &str);
	bool startswith(const std::string &str, const std::string &start);
	bool comp_factor(const Factor &a, const Factor &b);
	double MEDIAN(list<double> &values);
	double quantile(vector<double> vec, double q);
	double quantile_sorted(const vector<double> &vec, double q);
	// void filter_factor_list(vector<string> factor_list, vector<FactorCreator*>& v_fc);

	template <typename T>
	inline bool check_key(const std::map<int, T> &m, int key)
	{
		return m.find(key) != m.end();
	};

	struct Zugzw4ngContractInfo
	{
		double price_tick;
		double multiply;
	};

	Zugzw4ngContractInfo Zugzw4ngGetContractInfo(const int ticker, const int date);

	map<int, string> transform_map();

	int getInd40(const int code);

	int getMultiplier40(const int code);
}

// class YzyFactor : public Zugzw4ngFactor {
//	bool is_nd, is_3s;
// };