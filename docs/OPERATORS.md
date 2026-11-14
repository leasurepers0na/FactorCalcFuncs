# FactorCalcFuncs 算子清单

本文件由源码自动生成，列出库中全部算子及其用途。

算子类共 **35** 个。

## Rolling 算子

| 类名 | 说明 | 语义 |
|---|---|---|
| `RollingHelper` | 滑动窗口算子基类 | 所有 rolling 算子的公共基类，仅提供虚析构，不含状态。 |
| `ShiftHelper` | 滞后算子 | 返回窗口内第 N 根之前的值（滞后 N 拍），用于构造差分、动量类因子。 |
| `RollingFuncHelper` | 通用一元 rolling 算子 | 对窗口内的值施加自定义一元函数（如 log、sqrt）。 |
| `RollingFuncHelper2` | 通用二元 rolling 算子 | 对两路序列在窗口内施加自定义二元函数（如相关、比值）。 |
| `RollingSumData` | 滑动求和的内部状态 | 保存窗口内累加值与有效样本数。 |
| `RollingSumHelper` | 滑动求和 | 窗口内求和，支持 nan 跳过；因子中用得最多的一类基础算子。 |
| `RollingStdData` | 滑动标准差内部状态 | 保存窗口内一阶矩与二阶矩，用于 O(1) 增量更新。 |
| `RollingStdHelper` | 滑动标准差 | 窗口内样本标准差；unbiased=true 时为无偏估计（除以 n-1）。 |
| `RollingCorrData` | 滑动相关内部状态 | 保存两路序列的均值、方差与协方差。 |
| `RollingCorrHelper` | 滑动相关系数 | 窗口内两路序列的 Pearson 相关系数，常用于价量相关类因子。 |
| `RollingMaxData` | 滑动最大值内部状态 | 单调队列，支持 O(1) 摊销的窗口最大值。 |
| `RollingMaxHelper` | 滑动最大值 | 窗口内最大值，用于构造突破、区间高点类因子。 |
| `RollingArgmaxHelper` | 滑动最大值位置 | 返回窗口内最大值所在的位置（距当前 bar 的距离）。 |
| `RollingMinData` | 滑动最小值内部状态 | 单调队列，支持 O(1) 摊销的窗口最小值。 |
| `RollingMinHelper` | 滑动最小值 | 窗口内最小值，用于构造区间低点类因子。 |
| `RollingArgminHelper` | 滑动最小值位置 | 返回窗口内最小值所在位置。 |
| `FillNANHelper` | 缺失值填充 | 按给定策略填补 NaN（前值/零/均值），保证下游算子有稳定输入。 |
| `RollingMeanHelper` | 滑动均值 | 窗口内算术平均，最基础的平滑算子。 |
| `RollingMeanHelperPre` | 滑动均值（前缀和版） | 用前缀和实现，适合需要长期历史且窗口固定的场景。 |
| `RollingTsstdHelper` | 滑动时序标准化（时序 z-score） | 把因子值变成 (x - 窗口均值) / 窗口标准差，是因子标准化的核心算子。 |
| `RollingTsstdNodmHelper` | 滑动时序标准化（不减均值） | 只除以窗口标准差，保留因子自身的均值信息。 |
| `MultiRollingTsstdHelper` | 多标签滑动时序标准化 | 为多个 label（品种/合约）各自维护一套滚动窗口，一次调用完成分组标准化。 |
| `CumSumHelper` | 累计求和 | 从序列起点累计，用于构造累计量类因子。 |
| `CumMeanHelper` | 累计均值 | 从起点到当前的均值。 |
| `CumCorrHelper` | 累计相关 | 从起点到当前的两路相关系数。 |
| `CumStdData` | 累计标准差内部状态 | 保存累计一阶矩与二阶矩。 |
| `CumStdHelper` | 累计标准差 | 从起点到当前的标准差。 |
| `RollingEmaHelper` | 指数滑动平均 (EMA) | 按 alpha 递推平滑，等价于无穷窗口的加权平均。 |
| `RollingSortedData` | — | — |
| `RollingMedianHelper` | — | — |
| `RollingSortedHelper` | — | — |
| `RollingRankHelper` | 滑动分位排名 | 返回当前值在窗口内的分位（0~1），对异常值不敏感。 |
| `CommonFuncs` | 通用函数集合 | 数值工具：isnan、截断、安全除法、log/sqrt、归一化等静态函数。 |
| `RollingTruncStdData` | 截断标准差内部状态 | 保存截断后的一阶矩与二阶矩。 |
| `RollingTruncStdHelper` | 截断滑动标准差 | 先按分位数截断极值再求标准差，用于抑制异常值影响。 |

## 通用函数（Zugzw4ngFuncs）

| 函数 |
|---|
| `is_prime` |
| `getOverPrime` |
| `add_1min_factor` |
| `add_ti_factor` |
| `replace` |
| `is_in` |
| `getClockTime` |
| `getClockHour` |
| `getTm` |
| `get_weekday` |
| `Q_rsqrt` |
| `Q_rsqrt_double` |
| `filter_factor_list` |
| `filter_factor_list_rev` |
| `getBarNum` |
| `getClockTime_3s` |
| `getSecTickNum` |
| `cal_slope` |
| `extract_int_from_string` |
| `startswith` |
| `comp_factor` |
| `MEDIAN` |
| `quantile` |
| `quantile_sorted` |
| `check_key` |
| `Zugzw4ngGetContractInfo` |
| `transform_map` |
| `getInd40` |
| `getMultiplier40` |
