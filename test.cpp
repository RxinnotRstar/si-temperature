#include "temperature.hpp"
#include <iostream>

int main()
{
    // 基本赋值测试
    Temperature ice = 0;
    Temperature boiling = 212.0_F;
    Temperature absolute_zero = 0_K;
    Temperature so_cold = -5;
    // Temperature invalid = -5_C; // 温度点不该重载负号，所以没有重载负号。请使用工厂函数 Temperature::fromCelsius(-5) 来创建负温度。或者如果你确定是摄氏度，可以省略单位。
    std::cout << "水的冰点是" << ice.toCelsius() << " C = " << ice.toKelvin() << " K = " << ice.toFahrenheit() << " F\n";                               // 正确输出 0 C, 273.15 K, 32 F
    std::cout << "水的沸点是" << boiling.toCelsius() << " C = " << boiling.toKelvin() << " K = " << boiling.toFahrenheit() << " F\n";                   // 正确输出 100 C, 373.15 K, 212 F
    std::cout << "绝对零度是" << absolute_zero.toCelsius() << " C = " << absolute_zero.toKelvin() << " K = " << absolute_zero.toFahrenheit() << " F\n"; // 正确输出 -273.15 C, 0 K, -459.67 F
    std::cout << "一个很冷的温度是" << so_cold.toCelsius() << " C = " << so_cold.toKelvin() << " K = " << so_cold.toFahrenheit() << " F\n";             // 正确输出 -5 C, 268.15 K, 23 F

    // 温度点 - 温度点 = 温差
    TemperatureDiff diff = boiling - ice;
    std::cout << "水沸腾和结冰的温差: " << diff.toFahrenheit() << " F_diff\n"; // 正确输出 180

    // 温度点 + 温差 = 温度点
    Temperature room_temp = 20.0_C;
    TemperatureDiff heater = 15.0_C_diff;
    Temperature final_temp = room_temp + heater;
    std::cout << "温度: " << final_temp.toCelsius() << " C\n"; // 正确输出 35
    final_temp += heater;
    std::cout << "温度: " << final_temp.toCelsius() << " C\n"; // 正确输出 50

    // 温度点 - 温差 = 温度点
    room_temp = 35.0_C;
    TemperatureDiff cooler = 9_F_diff; // 整数
    final_temp = room_temp - cooler;
    std::cout << "温度: " << final_temp.toCelsius() << " C\n"; // 正确输出 30
    final_temp -= cooler;
    std::cout << "温度: " << final_temp.toCelsius() << " C\n"; // 正确输出 25

    // 数乘测试 (diff * scalar)
    TemperatureDiff big_diff = diff * 2.0;
    std::cout << "温差的2倍: " << big_diff.toFahrenheit() << " F_diff\n"; // 正确输出 360

    // 数乘测试 (scalar * diff)
    TemperatureDiff another_diff = 3.0 * diff;
    std::cout << "温差的3倍: " << another_diff.toFahrenheit() << " F_diff\n"; // 正确输出 540

    // 数除测试
    TemperatureDiff half_diff = diff / 2.0;
    std::cout << "温差的一半: " << half_diff.toFahrenheit() << " F_diff\n"; // 正确输出 90

    // *= 测试
    TemperatureDiff multiply_test = 10.0_C_diff;
    multiply_test *= 5.0;
    std::cout << "10_C_diff *= 5.0 = " << multiply_test.toCelsius() << " C_diff\n"; // 正确输出 50

    // /= 测试
    TemperatureDiff divide_test = 100.0_C_diff;
    divide_test /= 4.0;
    std::cout << "100_C_diff /= 4.0 = " << divide_test.toCelsius() << " C_diff\n"; // 正确输出 25

    // 温差取负测试
    TemperatureDiff negative_diff = -diff;
    std::cout << "温差的负值: " << negative_diff.toFahrenheit() << " F_diff\n"; // 正确输出 -180

    // Temperature += double（视为摄氏度温差）
    Temperature t1 = 32_F;
    t1 += 5.0;
    std::cout << "32_F += 5.0 = " << t1.toCelsius() << " C\n"; // 正确输出 5 摄氏度

    // Temperature -= double（视为摄氏度温差）
    t1 -= 10.0;
    std::cout << "25_C -= 10.0 = " << t1.toCelsius() << " C\n"; // 正确输出 15

    // Temperature += int（视为摄氏度温差）
    Temperature t2 = 0.0_C;
    t2 += 3;
    std::cout << "0_C += 3 = " << t2.toCelsius() << " C\n"; // 正确输出 3

    // Temperature -= int（视为摄氏度温差）
    t2 -= 8;
    std::cout << "3_C -= 8 = " << t2.toCelsius() << " C\n"; // 正确输出 -5
}