# FactorCalcFuncs

CTA 因子计算算子库。把「滑动窗口算子」和「通用数值函数」从因子工程中抽离出来，形成独立的
共享库，供 **FactorReplay 回放平台** 与 **因子 DLL 工程** 共同链接。

- 源码只存在于本仓库
- 使用方只依赖 `dist/include` 与 `dist/lib/libFactorCalcFuncs.so`

## 目录结构

```
FactorCalcFuncs/
├── CMakeLists.txt          构建脚本（生成 libFactorCalcFuncs.so）
├── include/                对外头文件
│   ├── Factor_Dll.h        全部算子类（Rolling* / Cum* / CommonFuncs）
│   ├── Zugzw4ngFuncs.h     通用函数声明
│   ├── FactorParamDef.h    平台与因子之间的 ABI 结构体
│   ├── DLL_PRE_DEFINE.h    导出宏
│   ├── queue_ex.hpp        轻量队列
│   └── stdafx.h
├── src/
│   ├── Factor_Dll.cpp      算子实现
│   └── Zugzw4ngFuncs.cpp   通用函数实现
├── docs/
│   └── OPERATORS.md        算子清单（由源码生成）
└── dist/                   构建产物（不入库）
    ├── include/
    └── lib/libFactorCalcFuncs.so
```

## 构建

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../dist
make -j 8
make install
```

安装后得到：

```
dist/include/*.h
dist/lib/libFactorCalcFuncs.so
```

## 使用

### CMake

```cmake
set(FACTORCALC_ROOT "/path/to/FactorCalcFuncs/dist")
include_directories(${FACTORCALC_ROOT}/include)
target_link_libraries(your_target PRIVATE ${FACTORCALC_ROOT}/lib/libFactorCalcFuncs.so)
```

### 运行时

```bash
export LD_LIBRARY_PATH=$FACTORCALC_ROOT/lib:$LD_LIBRARY_PATH
```

## 算子一览

| 类别 | 算子 |
|---|---|
| 基础 | `RollingSumHelper` `RollingMeanHelper` `RollingStdHelper` `RollingMaxHelper` `RollingMinHelper` |
| 位置 | `RollingArgmaxHelper` `RollingArgminHelper` `RollingRankHelper` |
| 相关 | `RollingCorrHelper` `CumCorrHelper` |
| 累计 | `CumSumHelper` `CumMeanHelper` `CumStdHelper` |
| 平滑 | `RollingEmaHelper` `RollingMeanHelperPre` |
| **标准化** | `RollingTsstdHelper` `RollingTsstdNodmHelper` `MultiRollingTsstdHelper` |
| 稳健 | `RollingTruncStdHelper` `FillNANHelper` |
| 其他 | `ShiftHelper` `RollingFuncHelper` `RollingFuncHelper2` `CommonFuncs` |

完整说明见 [docs/OPERATORS.md](docs/OPERATORS.md)。

## 时序标准化（tsstd）说明

`RollingTsstdHelper` 是因子标准化的核心算子：

```
factor = CLIP((x - mean(x, W)) / std(x, W), clip_min, clip_max)
```

- `W` 为窗口长度（bar 数），由使用方通过 `FactorParam::tsstd_bar_num` 传入
- 每个 `label`（合约代码）各自维护独立的滚动状态，互不干扰
- 常规口径：`clip_min = -3.0`、`clip_max = 3.0`
- 窗口长度与因子参数的关系（与研究员主程序一致）：

| 因子参数 `param` | `tsstd_bar_num` |
|---:|---:|
| ≤ 5 | 100 |
| ≤ 120 | 12000 |
| ≤ 360 | 36000 |
| 其他 | 72000 |

窗口足够长时，因子需要的历史预热长度也相应增加（`max_bar + tsstd_bar`）。

## 版本

- v1.0.0 从因子工程与回放平台中抽离，形成独立共享库
