#pragma once
#include "stdafx.h"
#include "DLL_PRE_DEFINE.h"
using namespace std;  // FactorParamDef.h uses ostream/istream/vector/string without std:: prefix
#define ENABLE_1MIN_VWAP
//position_all  就是该品种所有合约 某一个时刻末position的累加  比如IF  就是当天IF4个合约500毫秒position的累计值
//volume_all 就是你计算的500毫秒的volume加总
class DLLEXAM_API FutureData {
public:
	int code;
	int contract_month;
	unsigned long long time;
	float open_price;
	float high_price;
	float close_price;
	float low_price;
	float vwap;//vwap = turnover / volume / multiplier
	float twap;//twap是(close + open + high + low)/4
	int volume;
	double turnover;
	int position;//openInterest
	int tradingdate;
	int up_kc;//有方向的开仓量字段   upkc/downkc = (volume + oi_change) / 2;uppc/downpc = (volume - oi_change) / 2
	int down_kc;//有方向的开仓量字段  如果close_adj大于close_adj.shift(1)，upkc/uppc有值，downkc/downpc填0
	int up_pc;//有方向的平仓量字段    如果close_adj等于close_adj.shift(1)，这四个都是0
	int down_pc;//有方向的平仓量字段  如果close_adj小于close_adj.shift(1)，downkc/downpc有值，upkc/uppc填0
	int oi_change;//oi_change = openInterest - openInterest.shift(1) 
	float s5;
	float s4;
	float s3;
	float s2;
	float s1;
	float b1;
	float b2;
	float b3;
	float b4;
	float b5;
	int sv5;
	int sv4;
	int sv3;
	int sv2;
	int sv1;
	int bv1;
	int bv2;
	int bv3;
	int bv4;
	int bv5;
	short fill_flag;//是否填充
	double price_adj;//price_adj就是换合约时候的复权因子close_adj = closeprice / priceadj
	int volume_all;//turnover_all就是全部合约的turnover的同一时刻总和，volume_all和position_all同理
	int position_all;
	FutureData() {
		memset(this, 0, sizeof(FutureData));
	}
	friend ostream& operator<< (ostream& out, const FutureData& fd)
	{
		out.write((char*)&fd, sizeof(fd));
		return out;
	}
	friend istream& operator>>(istream& is, FutureData& fd)
	{
		is.read((char*)&fd, sizeof(fd));
		return is;
	}
	bool operator< (const FutureData& other) {
		if (time < other.time || (time == other.time && code < other.code) || (time == other.time && code == other.code && contract_month < other.contract_month)) {
			return true;
		}
		else {
			return false;
		}
	}
};
class DLLEXAM_API FutureData3Sec {
public:
	int code;
	int contract_month;
	unsigned long long time;
	float open_price;
	float high_price;
	float close_price;
	float low_price;
	int volume;
	double turnover;
	int position;
	int tradingdate;
	int up_kc;
	int down_kc;
	int up_pc;
	int down_pc;
	int oi_change;
	short fill_flag;
	double price_adj;

	FutureData3Sec() {
	}
	friend ostream& operator<< (ostream& out, const FutureData3Sec& fd)
	{
		out.write((char*)&fd, sizeof(fd));
		return out;
	}
	friend istream& operator>>(istream& is, FutureData3Sec& fd)
	{
		is.read((char*)&fd, sizeof(fd));
		return is;
	}
	bool operator< (const FutureData3Sec& other) {
		if (time < other.time || (time == other.time && code < other.code) || (time == other.time && code == other.code && contract_month < other.contract_month)) {
			return true;
		}
		else {
			return false;
		}
	}
};

class DLLEXAM_API FutureData1Min {
public:
	int code;
	int contract_month;
	unsigned long long time;
	float open_price;
	float high_price;
	float close_price;
	float low_price;
	int volume;
	double turnover;
#ifdef ENABLE_1MIN_VWAP
	double vwap;
#endif
	int position;
	int tradingdate;
	int up_kc;
	int down_kc;
	int up_pc;
	int down_pc;
	int oi_change;
	short fill_flag;
	double price_adj;

	FutureData1Min() {
	}
	friend ostream& operator<< (ostream& out, const FutureData1Min& fd)
	{
		out.write((char*)&fd, sizeof(fd));
		return out;
	}
	friend istream& operator>>(istream& is, FutureData1Min& fd)
	{
		is.read((char*)&fd, sizeof(fd));
		return is;
	}
	bool operator< (const FutureData1Min& other) {
		if (time < other.time || (time == other.time && code < other.code) || (time == other.time && code == other.code && contract_month < other.contract_month)) {
			return true;
		}
		else {
			return false;
		}
	}
};

class DLLEXAM_API Factor {
public:
	int code;  
	int contract_month;
	unsigned long long time; 
	double factor;        /// 因子值
    uint32_t hls_bar_window; /// 涨跌停股票因子值限制窗口（1,2,3...), ０表示不作涨跌停处理
	Factor()
		: code(0)
		, contract_month(0)
		, time(0)
		, factor(0)
        , hls_bar_window(0){
	}
	Factor(const Factor & fv) {
		this->code = fv.code;
		this->contract_month = fv.contract_month;
		this->time = fv.time;
		this->factor = fv.factor;
	}
	Factor & operator = (const Factor & fv) {
		this->code = fv.code;
		this->contract_month = fv.contract_month;
		this->time = fv.time;
		this->factor = fv.factor;
		return *this;
	}
};

struct DLLEXAM_API FactorParamItemSave {
	int code;
	uint64_t time;
	int contract_month;
	FutureData3Sec future_data_3s;        /// 3s行情快照
	FutureData1Min future_data_1min;      /// 1min行情快照
};

struct DLLEXAM_API FactorParamItem {
	int code;
	uint64_t time;
	int contract_month;
	FutureData3Sec future_data_3s;        /// 3s行情快照
	FutureData1Min future_data_1min;      /// 1min行情快照
	vector< FutureData > v_future_datas; /// 0.5s行情快照
	FactorParamItem & operator=(const FactorParamItem & item) {
		code = item.code;
		time = item.time;
		contract_month = item.contract_month;
		future_data_3s = item.future_data_3s;
		future_data_1min = item.future_data_1min;
		v_future_datas.resize(item.v_future_datas.size());
		v_future_datas.assign(item.v_future_datas.begin(), item.v_future_datas.end());
		return *this;
	}
};

class DLLEXAM_API FactorParam {
public:
	uint32_t datafile_index;
	vector< FactorParamItem> v_time_data;
	FactorParam(){
	}

	~FactorParam() {
	}

	void ResetParam() {
	}

	FactorParam(const FactorParam & fp) {
		datafile_index = fp.datafile_index;
		v_time_data = fp.v_time_data;
	}

	FactorParam & operator =(const FactorParam & fp) {
		datafile_index = fp.datafile_index;
		v_time_data = fp.v_time_data;
		return *this;
	}
};
/// 驱动频率
enum DLLEXAM_API CalcFlagType {
	CALC_NAN_FG = 0,
	CALC_0d5s_FG = 0x01,
	CALC_3s_FG = 0x02,
	CALC_1min_FG = 0x04,
};

/// 发送频率
enum SendStepFlag {
	SSF_NAN_FG = 0,
	SSF_1min_FG = 0x01,
	SSF_3sec_FG = 0x02,
	SSF_5sec_FG = 0x05,
};
class DLLEXAM_API FactorCreator
{
public:
	string factor_name;
	uint32_t factor_id;       /// 因子ID 
	int calc_flag;   /// 调用接口标志, CALC_3s_FG调用handle_data_3s(), 
	                 /// CALC_0d5s_FG调用handle_data_0d5()
				     /// CALC_1min_FG调用handle_data_1min()
	int send_step_flag; /// 发送频率,默认与驱动频率一致
	unsigned int max_bar; /// 实盘最大bar数
    uint32_t hls_bar_window;

	uint32_t tsstd_bar_num;///默认值是120*300
	bool    nan_tsstd_flag; /// 默认值是True，只有当tsstd_bar_num!=0时生效，表示序列有nan时，仍计算tsstd
	bool    clip_flag;///只有当tsstd_bar_num!=0时生效，默认为true，表示对与tsstd之后的值不做处理
	double  clip_max; ///默认值为3.0，只有当clip_flag=true时生效，对于tsstd之后的因子值f，
					  ///如果f>clip_max，f的值设置为clip_max（相应地，如果f<clip_min，f的值设置为clip_min）
	double  clip_min; ///默认值为-3.0

public:
	void SetCalcFlag(const int & calc_flag) {
		this->calc_flag	= calc_flag;
	}
	void SetMaxBarNum(const unsigned int max_bar) {
		this->max_bar = max_bar;
	}
	void SetSendStepFlag(const int & send_step_flag) {
		this->send_step_flag = send_step_flag;
	}
	void SetTsstdBarNum(const int &tsstd_bar) {
		this->tsstd_bar_num = tsstd_bar;
	}
	void SetNanTsstdFlag(bool nan_flag) {
		this->nan_tsstd_flag = nan_flag;
	}
	void SetClipFlag(bool flag) {
		this->clip_flag = flag;
	}
	void SetClipMax(const double &max_val) {
		this->clip_max = max_val;
	}
	void SetClipMin(const double &min_val) {
		this->clip_min = min_val;
	}
	virtual void handle_data_3s(const FactorParam & param
		, vector<Factor> & out_val) {};
	virtual void handle_data_0d5(const FactorParam & param
		, vector<Factor> & out_val) {};
	virtual void handle_data_1min(const FactorParam & param
		, vector<Factor> & out_val) {};

	FactorCreator()
		: factor_name("")
		, factor_id(0)
		, calc_flag(0)
		, send_step_flag(0)
		, max_bar(0)
        , hls_bar_window(0)
		, tsstd_bar_num(120*300)
		, nan_tsstd_flag(true)
		, clip_flag(true)
		, clip_max(3.0)
		, clip_min(-3.0)
	{}
	~FactorCreator() {}
};

//#include "CalFactorParam.h"
