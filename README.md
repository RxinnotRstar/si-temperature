# si-temperature

基于 C++23 的强类型温度计算库。

新手上路，vibe coding 产物，大佬轻喷。

## 特点

可以区分温度点和温度差。

支持温度点和温度差的加减运算，能够得到正确的温度点/温度差作为结果。

支持用户通过字面量后缀直接指定单位。

带有注释，我尽可能写成 Doxygen 风格了。当然还是离不开 copilot 的行内补全就是了。

乘除还没实现，也暂时没准备捣鼓用于物理化学公式的喜闻乐见的把温度往指数带的行为。

某种意义上这只是个 C++ 新手的面向对象编程练习罢了。

但万一有用呢，我是说万一（）

## 示例

如果你真的决定要用……好吧，确实有这么个示例，也是 AI 的：

```cpp
#include "temperature.hpp"
#include <iostream>

int main()
{
    Temperature ice = 0.0_C;
    Temperature boiling = 100.0_C;

    // 温度点 - 温度点 = 温差
    TemperatureDiff diff = boiling - ice;
    std::cout << "水沸腾和结冰的温差: " << diff.toFahrenheit() << " F_diff\n"; // 正确输出 180

    // 温度点 + 温差 = 温度点
    Temperature room_temp = 20.0_C;
    TemperatureDiff heater = 15.0_C_diff;
    Temperature final_temp = room_temp + heater;
    std::cout << "最终温度: " << final_temp.toCelsius() << " C\n"; // 正确输出 35
}
```

可以拿这个测试一下，或者自己写个测点更有意义的场景，总比这个 AI 的强。

## 计划

实现数乘，比率计算。

然后想想还有什么能做的。
