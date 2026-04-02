# 匿名凭证 Demo 非开发者操作指南

这份说明只讲怎么使用，不讲代码实现。

适合的人：

- 需要现场演示这个匿名凭证 demo
- 只想录入信息、选择展示内容、看到验证结果

## 1. 这个系统能做什么

这套 demo 可以完成一条最小流程：

1. 录入一份证件信息
2. 选择想展示的内容
3. 生成匿名证明
4. 验证证明是否成功

当前可录入的字段有：

- `family_name`：姓
- `given_name`：名
- `birth_date`：出生日期
- `issue_date`：签发日期
- `expiry_date`：到期日期
- `issuing_country`：签发国家
- `age_over_18`：是否年满 18 岁

当前可选择展示的内容有：

- `family_name`
- `given_name`
- `birth_date`
- `issue_date`
- `expiry_date`
- `issuing_country`
- `age_over_18`

## 2. 使用前准备

你需要别人先帮你完成一次编译。

如果已经编译好，你只需要确认下面这个程序存在：

```bash
./build-demo-cli/examples/mdoc_anoncred/mdoc_anoncred_console
```

如果这个文件存在，就可以继续。

## 3. 最简单的使用方法

在仓库根目录运行：

```bash
./build-demo-cli/examples/mdoc_anoncred/mdoc_anoncred_console guided \
  --out-root run/mdoc-interactive
```

系统会进入交互式模式。

## 4. 交互式操作步骤

### 第一步：选择模式

屏幕会显示：

```text
Choose issuance mode:
  1. Real sample-based mdoc
  2. Custom interactive mdoc
```

如果你要自己输入信息，请输入：

```text
2
```

然后按回车。

### 第二步：输入证件信息

系统会依次询问：

1. `family_name`
2. `given_name`
3. `birth_date (YYYY-MM-DD)`
4. `issue_date (YYYY-MM-DD)`
5. `expiry_date (YYYY-MM-DD)`
6. `issuing_country (2 letters)`
7. `age_over_18 (true/false)`

输入规则：

- 日期必须写成 `YYYY-MM-DD`
- 国家代码写 2 个英文字母，例如 `US`、`DE`、`CN`
- `age_over_18` 填 `true` 或 `false`
- 如果直接按回车，会使用方括号里的默认值

示例输入：

```text
family_name [Mustermann]: Zhang
given_name [Erika]: San
birth_date (YYYY-MM-DD) [1971-09-01]: 1999-12-31
issue_date (YYYY-MM-DD) [2024-01-01]: 2024-01-01
expiry_date (YYYY-MM-DD) [2035-01-01]: 2030-01-01
issuing_country (2 letters) [DE]: CN
age_over_18 (true/false) [true]: true
```

### 第三步：选择要展示的内容

接下来系统会列出可展示的内容，例如：

```text
Supported claims:
  1. family_name
  2. given_name
  3. birth_date
  4. issue_date
  5. expiry_date
  6. issuing_country
  7. age_over_18
```

然后会提示：

```text
Select 1-2 claims by number, comma separated
```

意思是：

- 最少选 1 项
- 最多选 2 项
- 多项时用逗号分开

示例：

- 只展示姓名：`2`
- 展示姓名和国家：`2,6`
- 展示是否成年：`7`

## 5. 什么叫成功

如果流程成功，最后会看到类似输出：

```text
verification ok: given_name, issuing_country
```

这表示：

- 系统已经成功签发
- 已成功生成匿名证明
- verifier 已成功验证

后面还会看到 3 个目录位置：

- `issue dir`
- `request dir`
- `presentation dir`

它们分别表示：

- 签发结果
- 验证请求
- 证明结果

## 6. 什么叫失败

如果失败，通常会看到下面几类提示。

### 输入格式错误

例如：

- 日期不是 `YYYY-MM-DD`
- 国家代码不是 2 个字母
- `age_over_18` 不是 `true/false`

处理方法：

- 重新运行
- 按要求重新输入

### 选择内容错误

例如：

- 选择了不存在的编号
- 选了超过 2 项

处理方法：

- 重新运行
- 只选 1 到 2 项

### 验证失败

如果出现：

```text
verification failed
```

通常表示这次证明或输入材料不一致。

处理方法：

1. 重新运行整条流程
2. 不要手动修改 `run/mdoc-interactive/` 目录里的文件

## 7. 推荐演示流程

如果你是给别人现场演示，建议这样操作：

1. 运行交互程序
2. 选择 `2`，进入自定义模式
3. 输入一组简单信息
4. 在 claim 里选择 `2,6`
   - `given_name`
   - `issuing_country`
5. 等待结果
6. 看到 `verification ok: given_name, issuing_country`

这样最容易理解。

## 8. 一条可直接复制的命令

```bash
./build-demo-cli/examples/mdoc_anoncred/mdoc_anoncred_console guided \
  --out-root run/mdoc-interactive
```

## 9. 需要开发者介入的情况

如果遇到下面情况，需要找开发者处理：

- `mdoc_anoncred_console` 文件不存在
- 程序无法启动
- 每次运行都在编译电路但最终报错退出
- 目录权限不足
- 想新增新的展示内容
- 想新增新的输入字段
