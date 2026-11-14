#include "Zugzw4ngFuncs.h"
// #include "CommTransformer.h"
#include <regex>

namespace Zugzw4ngFuncs
{

	void add_1min_factor(FactorCreator *fc, int max_bar, vector<FactorCreator *> &v_fc)
	{
		fc->SetCalcFlag(CALC_1min_FG);
		fc->SetMaxBarNum(max_bar);
		v_fc.push_back(fc);
		return;
	};

	void add_1min_factor(FactorCreator *fc, vector<FactorCreator *> &v_fc)
	{
		fc->SetCalcFlag(CALC_1min_FG);
		v_fc.push_back(fc);
		return;
	};

	void add_ti_factor(FactorCreator *fc, vector<FactorCreator *> &v_fc)
	{
		fc->SetCalcFlag(CALC_0d5s_FG);
		v_fc.push_back(fc);
		return;
	};

	string replace(string str, string sub1, string sub2)
	{
		std::string test = str;
		test = std::regex_replace(test, std::regex(sub1), sub2);
		return test;
	};

	bool is_in(string s_sub, const string &str)
	{
		return str.find(s_sub) != std::string::npos;
	}

	bool is_in(string s, const vector<string> &s_vec)
	{
		for (auto &cs : s_vec)
		{
			if (cs == s)
				return true;
		}
		return false;
	}

	bool is_in(int s, const vector<int> &s_vec)
	{
		for (auto &cs : s_vec)
		{
			if (cs == s)
				return true;
		}
		return false;
	}

	bool is_in(double s, const vector<double> &s_vec)
	{
		for (auto &cs : s_vec)
		{
			if (abs(cs - s) < 1e-10)
				return true;
		}
		return false;
	}

	int getClockTime(time_t time)
	{
		struct tm tp;
		time_t local_time = time + 28800;
#ifdef _WIN32
		gmtime_s(&tp, &local_time);
#else
		gmtime_r(&local_time, &tp);
#endif
		return tp.tm_hour * 100 + tp.tm_min;
	}

	int getClockHour(time_t time)
	{
		struct tm tp;
		time_t local_time = time + 28800;
#ifdef _WIN32
		gmtime_s(&tp, &local_time);
#else
		gmtime_r(&local_time, &tp);
#endif
		return tp.tm_hour;
	}

	int getClockTime_3s(time_t time)
	{
		struct tm tp;
		time_t local_time = time + 28800;
#ifdef _WIN32
		gmtime_s(&tp, &local_time);
#else
		gmtime_r(&local_time, &tp);
#endif
		return tp.tm_hour * 10000 + tp.tm_min * 100 + tp.tm_sec;
	}

	int getSecTickNum(time_t time)
	{
		struct tm tp;
		time_t local_time = time + 28800;
#ifdef _WIN32
		gmtime_s(&tp, &local_time);
#else
		gmtime_r(&local_time, &tp);
#endif
		return tp.tm_sec / 3;
	}

	int getBarNum(time_t time, int time_period)
	{
		struct tm tp;
		time_t local_time = time + 28800;
#ifdef _WIN32
		gmtime_s(&tp, &local_time);
#else
		gmtime_r(&local_time, &tp);
#endif
		if (time_period == 5)
		{
			return (tp.tm_hour < 12 ? (tp.tm_hour - 9) * 60 + (tp.tm_min - 30) : (tp.tm_hour - 13) * 60 + 120 + tp.tm_min) / 5;
		}
		else if (time_period == 1)
		{
			return tp.tm_hour < 12 ? (tp.tm_hour - 9) * 60 + (tp.tm_min - 30) + 1 : (tp.tm_hour - 13) * 60 + 120 + tp.tm_min + 1;
		}
		else
		{
			return -1;
		}
	};

	double cal_slope(vector<double> &x, vector<double> &y)
	{
		if (x.size() != y.size())
		{
			return NAN;
		}
		else
		{
			double n = x.size() * 1.0;

			double avgX = accumulate(x.begin(), x.end(), 0.0) / n;
			double avgY = accumulate(y.begin(), y.end(), 0.0) / n;

			double numerator = 0.0;
			double denominator = 0.0;

			for (int i = 0; i < n; ++i)
			{
				numerator += (x[i] - avgX) * (y[i] - avgY);
				denominator += (x[i] - avgX) * (x[i] - avgX);
			}

			return (abs(denominator) > 1e-4) ? (numerator / denominator) : NAN;
		}
	}

	double cal_slope(vector<double> &x, double &x_mean, vector<double> &y, double &y_mean)
	{
		if (x.size() != y.size())
		{
			return NAN;
		}
		else
		{
			double n = x.size() * 1.0;

			double numerator = 0.0;
			double denominator = 0.0;

			for (int i = 0; i < n; ++i)
			{
				numerator += (x[i] - x_mean) * (y[i] - y_mean);
				denominator += (x[i] - x_mean) * (x[i] - x_mean);
			}

			return (abs(denominator) > 1e-4) ? (numerator / denominator) : NAN;
		}
	}

	bool is_prime(int a)
	{
		bool isPrime = true;

		// 0 and 1 are not prime numbers
		if (a == 0 || a == 1)
		{
			isPrime = false;
		}
		else
		{
			for (int i = 2; i <= a / 2; ++i)
			{
				if (a % i == 0)
				{
					isPrime = false;
					break;
				}
			}
		}
		return isPrime;
	}

	int getOverPrime(int n, int count)
	{
		int prime_count = 0;

		int detector = n + 1;
		while (prime_count < count)
		{
			// cout << detector << " " << count << " " << prime_count << endl;
			if (Zugzw4ngFuncs::is_prime(detector))
			{
				prime_count++;
			}
			detector++;
		}
		return detector;
	}

	int extract_int_from_string(const string &str)
	{
		size_t i = 0;
		for (; i < str.length(); i++)
		{
			if (isdigit(str[i]))
				break;
		}
		// remove the first chars, which aren't digits
		std::string int_str = str.substr(i, str.length() - i);
		// convert the remaining text to an integer
		return atoi(int_str.c_str());
	}

	bool startswith(const std::string &str, const std::string &start)
	{
		int srclen = (int)str.size();
		int startlen = (int)start.size();
		if (srclen >= startlen)
		{
			string temp = str.substr(0, startlen);
			if (temp == start)
				return true;
		}

		return false;
	}

	bool comp_factor(const Factor &a, const Factor &b)
	{
		if (isnan(a.factor))
		{
			return false;
		}
		if (isnan(b.factor))
		{
			return true;
		}
		if (abs(a.factor - b.factor) > 0.000000001)
		{
			return a.factor > b.factor;
		}

		if (a.code != b.code)
		{
			return a.code > b.code;
		}

		return true;
	}

	double MEDIAN(list<double> &values)
	{
		for (auto v : values)
		{
			if (isnan(v))
			{
				return NAN;
			}
		}
		double median;
		int size = (int)values.size();
		vector<double> n_v;
		for (auto &v : values)
		{
			n_v.push_back(v);
		}

		sort(n_v.begin(), n_v.end());
		if (size % 2 == 1)
		{
			median = n_v[(size - 1) / 2];
		}
		else
		{
			median = (n_v[size / 2] + n_v[size / 2 - 1]) / 2.0;
		}
		return median;
	}

	double quantile(vector<double> vec, double q)
	{
		if (vec.size() == 0)
			return NAN;
		else
		{
			for (auto &X : vec)
			{
				if (isnan(X))
					return NAN;
			}
			sort(vec.begin(), vec.end());
			int N = (int)vec.size();
			double n = (N - 1) * q + 1;
			if (abs(n - 1.0) < 1e-4)
				return vec[0];
			else if (abs(n - N) > 1e-4)
				return vec[N - 1];
			else
			{
				int k = (int)n;
				double d = n - k;
				return vec[k - 1] + d * (vec[k] - vec[k - 1]);
			}
		}
	}

	double quantile_sorted(const vector<double> &vec, double q)
	{
		for (auto &X : vec)
		{
			if (isnan(X))
				return NAN;
		}
		int N = (int)vec.size();
		double n = (N - 1) * q + 1;
		if (abs(n - 1.0) < 1e-4)
			return vec[0];
		else if (abs(n - N) > 1e-4)
			return vec[N - 1];
		else
		{
			int k = (int)n;
			double d = n - k;
			return vec[k - 1] + d * (vec[k] - vec[k - 1]);
		}
	}

	tm getTm(time_t time)
	{
		struct tm tp;
		time_t local_time = time + 28800;
#ifdef _WIN32
		gmtime_s(&tp, &local_time);
#else
		gmtime_r(&local_time, &tp);
#endif
		return tp;
	}

	int get_weekday(time_t time)
	{
		struct tm tp;
		time_t local_time = time + 28800;
#ifdef _WIN32
		gmtime_s(&tp, &local_time);
#else
		gmtime_r(&local_time, &tp);
#endif
		return tp.tm_wday - 1;
	};

	float Q_rsqrt(float number)
	{
		long i;
		float x2, y;
		const float threehalfs = 1.5F;

		x2 = number * 0.5F;
		y = number;
		i = *(long *)&y;		   // evil floating point bit level hacking
		i = 0x5f3759df - (i >> 1); // what the fuck?
		y = *(float *)&i;
		y = y * (threehalfs - (x2 * y * y)); // 1st iteration
											 //	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

		return y;
	}

	// double Q_rsqrt_double(double number_double)
	// {
	// 	float number = (float)number_double;
	// 	long i;
	// 	float x2, y;
	// 	const float threehalfs = 1.5F;
	// 	x2 = number * 0.5F;
	// 	y = number;
	// 	i = *(long *)&y;		   // evil floating point bit level hacking
	// 	i = 0x5f3759df - (i >> 1); // what the fuck?
	// 	y = *(float *)&i;
	// 	y = y * (threehalfs - (x2 * y * y)); // 1st iteration
	// 										 //	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed
	// 	return (double)y;
	// }

	double Q_rsqrt_double(double number_double)
	{
		 return 1.0 / std::sqrt(number_double);
	}

	void filter_factor_list(vector<string> factor_list, vector<FactorCreator *> &v_fc)
	{
		vector<FactorCreator *> filtered_list;
		for (auto &f : v_fc)
		{
			bool del_flag = true;
			for (string &selected_f : factor_list)
			{
				if (f->factor_name == selected_f)
				{
					filtered_list.push_back(f);
					del_flag = false;
					break;
				}
			}

			if (del_flag)
			{
				delete f;
			}
		}
		v_fc = filtered_list;
		return;
	};

	void filter_factor_list_rev(vector<string> factor_list, vector<FactorCreator *> &v_fc)
	{
		vector<FactorCreator *> filtered_list;
		for (auto &f : v_fc)
		{
			bool select = true;
			for (string &selected_f : factor_list)
			{
				if (f->factor_name == selected_f)
				{
					select = false;
					break;
				}
			}

			if (select)
			{
				filtered_list.push_back(f);
			}
			else
			{
				delete f;
			}
		}
		v_fc = filtered_list;
		return;
	};

	Zugzw4ngContractInfo Zugzw4ngGetContractInfo(const int ticker, const int date)
	{
		Zugzw4ngContractInfo info;
		size_t code = (size_t)ticker;
		double price_tick;
		double multiply;
		bool isinit = true;
		CommonFuncs::GetContractInfo(code, date, price_tick, multiply, isinit);

		info.price_tick = price_tick;
		info.multiply = multiply;

		return info;
	};

	map<int, string> transform_map()
	{
		map<int, string> trans_map;
		trans_map[0] = "";

		trans_map[1001] = "Df";
		trans_map[1002] = "Md";
		trans_map[1003] = "MiDf";
		trans_map[1004] = "BaDfSt";
		trans_map[1005] = "MaR";
		trans_map[1006] = "MiR";
		trans_map[1007] = "Wp";
		trans_map[1008] = "CoSh";
		trans_map[1009] = "MaDvStd";
		trans_map[1010] = "StdDvMa";
		trans_map[1011] = "Std2nd";
		trans_map[1012] = "StdMaDf";
		trans_map[1013] = "HlcP10";
		trans_map[1014] = "HlcP25";
		trans_map[1015] = "HlcP50";
		trans_map[1016] = "Ax";
		trans_map[1017] = "An";
		trans_map[1018] = "Ar";
		trans_map[1019] = "RoKu";
		trans_map[1020] = "RoSk";
		trans_map[1021] = "EffWalk";
		trans_map[1022] = "RegSl";
		trans_map[1031] = "ShP25";
		trans_map[1032] = "ShP50";
		trans_map[1033] = "ShP100";
		trans_map[1034] = "ShP200";
		trans_map[1035] = "MaDfP150";
		trans_map[1036] = "MaDfP200";
		trans_map[1037] = "MaDfP300";
		trans_map[1038] = "MaDfP500";
		trans_map[1041] = "AdjInd";
		trans_map[1042] = "CorrInd";
		trans_map[1043] = "CovInd";
		trans_map[1044] = "AvgInd";
		trans_map[1045] = "AdjMarket";
		trans_map[1046] = "CorrMarket";
		trans_map[1047] = "CovMarket";
		trans_map[1048] = "AvgMarket";

		trans_map[1051] = "NewHlcP10";
		trans_map[1052] = "NewHlcP25";
		trans_map[1053] = "NewHlcP50";
		trans_map[1054] = "QtDfP80";
		trans_map[1055] = "QtDfP95";
		trans_map[1056] = "NdMaR";
		trans_map[1057] = "NdMiR";
		trans_map[1058] = "NdWp";
		trans_map[1059] = "TndMaR";
		trans_map[1060] = "TndMiR";
		trans_map[1061] = "TndWp";
		trans_map[1062] = "NdDf";
		trans_map[1063] = "TndDf";
		trans_map[1064] = "LsBrMaR";
		trans_map[1065] = "LsBrMiR";
		trans_map[1066] = "LsBrWp";
		trans_map[1067] = "LsBrDf";

		trans_map[2001] = "FrMean3";
		trans_map[2002] = "FrMean5";
		trans_map[2003] = "FrMean10";
		trans_map[2004] = "FrMean20";
		trans_map[2005] = "FrMaxDelta3";
		trans_map[2006] = "FrMaxDelta5";
		trans_map[2007] = "FrMaxDelta10";
		trans_map[2008] = "FrMaxDelta20";
		trans_map[2009] = "FrMinDelta3";
		trans_map[2010] = "FrMinDelta5";
		trans_map[2011] = "FrMinDelta10";
		trans_map[2012] = "FrMinDelta20";
		trans_map[2013] = "FrSp3";
		trans_map[2014] = "FrSp5";
		trans_map[2015] = "FrSp10";
		trans_map[2016] = "FrSp20";
		trans_map[2017] = "FrAx5";
		trans_map[2018] = "FrAx10";
		trans_map[2019] = "FrAx20";
		trans_map[2020] = "FrAn5";
		trans_map[2021] = "FrAn10";
		trans_map[2022] = "FrAn20";
		trans_map[2023] = "FrAr5";
		trans_map[2024] = "FrAr10";
		trans_map[2025] = "FrAr20";
		trans_map[2026] = "FrMacd5";
		trans_map[2027] = "FrMacd10";
		trans_map[2028] = "FrMacd17";
		trans_map[2029] = "FrMacd23";
		trans_map[2030] = "FwrMean3";
		trans_map[2031] = "FwrMean5";
		trans_map[2032] = "FwrMean10";
		trans_map[2033] = "FwrMean20";

		return trans_map;
	}

	int getInd40(const int code)
	{
		if (code == 110 || code == 130 || code == 107 || code == 108 || code == 109 || code == 134)
			return 1;
		else if (code == 102 || code == 104 || code == 105 || code == 106 || code == 137 || code == 138)
			return 2;
		else if (code == 119 || code == 121 || code == 129 || code == 123 || code == 133)
			return 3;
		else if (code == 124 || code == 132 || code == 141 || code == 113 || code == 112 || code == 111 || code == 139 || code == 114 || code == 127)
			return 4;
		else if (code == 120 || code == 122 || code == 140 || code == 118 || code == 117 || code == 128)
			return 5;
		else if (code == 126 || code == 125 || code == 131 || code == 115)
			return 6;
		else if (code == 103 || code == 101)
			return 7;
		else if (code == 116 || code == 142 || code == 143)
			return 6;
		else
			return 0;
	};

	int getMultiplier40(const int code)
	{
		if (code == 106 || code == 138)
			return 1;
		else if (code == 102 || code == 104 || code == 105 || code == 112 || code == 113 || code == 116 || code == 123 || code == 127 || code == 142 || code == 143)
			return 5;
		else if (code == 108 || code == 111 || code == 114 || code == 115 || code == 117 || code == 118 || code == 119 || code == 120 || code == 121 || code == 122 || code == 124 || code == 125 || code == 128 || code == 129 || code == 132 || code == 133 || code == 134 || code == 140)
			return 10;
		else if (code == 103)
			return 15;
		else if (code == 131 || code == 139 || code == 141)
			return 20;
		else if (code == 107)
			return 60;
		else if (code == 109 || code == 110 || code == 130)
			return 100;
		else if (code == 1)
			return 200;
		else if (code == 2 || code == 3)
			return 300;
		else if (code == 101 || code == 126)
			return 1000;
		else
			return 0;
	};
}