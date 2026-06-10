#include <iostream>
#include <vector>
#include <algorithm>
#include <compare>

// 类声明
class Temperature;
class TemperatureDiff;

/**
 * @brief 表示一个温度点，内部以开尔文存储，默认构造0摄氏度，无字面量时视为摄氏度
 * @note
 *
 * - 从摄氏度、华氏度、开尔文创建的工厂函数
 *
 * - 转换为摄氏度、华氏度、开尔文的成员函数
 *
 * - C++20 三路比较运算符（<=>）与相等运算符（==），自动生成 !=、<、>、<=、>=
 *
 * - 支持用户定义字面量（_C、_F、_K）
 *
 * - 加减法支持：
 *
 *   1. 温度点 - 温度点 = 温差
 *
 *   2. 温度点 + 温差 = 温度点
 *
 *   3. 温度点 - 温差 = 温度点
 */
class Temperature
{
private:
    // 用于区分构造函数的标签
    struct KelvinTag
    {
    };

    // 内部以开尔文存储温度
    double _kelvin;

    // 真的构造函数
    explicit constexpr Temperature(KelvinTag, double k) : _kelvin(k) {}

public:
    // 默认构造为 0 摄氏度（273.15 K）
    constexpr Temperature() : _kelvin(273.15f) {}
    // 摄氏度构造函数，无字面量时视为摄氏度并使用这个函数
    constexpr Temperature(double celsius) : _kelvin(celsius + 273.15f) {}

    /**
     * @brief 以摄氏度获取温度
     */
    constexpr double toCelsius() const
    {
        return _kelvin - 273.15f;
    }
    /**
     * @brief 以华氏度获取温度
     */
    constexpr double toFahrenheit() const
    {
        return (_kelvin - 273.15f) * 9.0f / 5.0f + 32.0f;
    }
    /**
     * @brief 以开尔文获取温度
     */
    constexpr double toKelvin() const
    {
        return _kelvin;
    }

    /**
     * @brief 从摄氏度创建 Temperature 对象
     * @param c 摄氏度数值
     */
    static constexpr Temperature fromCelsius(double c)
    {
        return Temperature(KelvinTag{}, c + 273.15f);
    }
    /**
     * @brief 从华氏度创建 Temperature 对象
     * @param f 华氏度数值
     */
    static constexpr Temperature fromFahrenheit(double f)
    {
        return Temperature(KelvinTag{}, (f - 32.0f) * 5.0f / 9.0f + 273.15f);
    }
    /**
     * @brief 从开尔文创建 Temperature 对象
     * @param k 开尔文度数值
     */
    static constexpr Temperature fromKelvin(double k)
    {
        return Temperature(KelvinTag{}, k);
    }

    // C++20 三路比较运算符与相等运算符
    constexpr std::partial_ordering operator<=>(const Temperature &o) const
    {
        return _kelvin <=> o._kelvin;
    }
    constexpr bool operator==(const Temperature &o) const
    {
        return _kelvin == o._kelvin;
    }

    // 运算符声明
    constexpr TemperatureDiff operator-(const Temperature &other) const;
    constexpr Temperature operator+(const TemperatureDiff &diff) const;
    constexpr Temperature operator-(const TemperatureDiff &diff) const;
    // 友元声明
    friend constexpr Temperature operator""_C(long double c);
    friend constexpr Temperature operator""_F(long double f);
    friend constexpr Temperature operator""_K(long double k);
};

/**
 * @brief 表示一个温差，内部以摄氏度差存储，提供从摄氏度差、华氏度差创建的工厂函数，以及转换函数
 * @note
 * - 从摄氏度差、华氏度差创建的工厂函数
 *
 * - 转换为摄氏度差、华氏度差的成员函数
 *
 * - C++20 三路比较运算符（<=>）与相等运算符（==），自动生成 !=、<、>、<=、>=
 *
 * - 温差之间的加减法，返回新的温差对象
 *
 * - 用户字面量（_C_diff、_F_diff）
 */
class TemperatureDiff
{
private:
    // 内部存储摄氏度温差
    double _celsius_diff;
    // 真的构造函数
    explicit constexpr TemperatureDiff(double d) : _celsius_diff(d) {}

public:
    // 默认构造为 0 摄氏度差
    constexpr TemperatureDiff() : _celsius_diff(0.0f) {}
    /**
     * @brief 以摄氏度差获取温差
     */
    constexpr double toCelsius() const
    {
        return _celsius_diff;
    }
    /**
     * @brief 以开尔文获取温差
     */
    constexpr double toKelvin() const
    {
        return _celsius_diff;
    }
    /**
     * @brief 以华氏度差获取温差
     */
    constexpr double toFahrenheit() const
    {
        return _celsius_diff * 9.0f / 5.0f;
    }
    /**
     * @brief 从摄氏度差创建 TemperatureDiff 对象
     * @param d 摄氏度差数值
     */
    static constexpr TemperatureDiff fromCelsius(double d)
    {
        return TemperatureDiff(d);
    }
    /**
     * @brief 从华氏度差创建 TemperatureDiff 对象
     * @param d 华氏度差数值
     */
    static constexpr TemperatureDiff fromFahrenheit(double d)
    {
        return TemperatureDiff(d * 5.0f / 9.0f);
    }

    // C++20 三路比较运算符与相等运算符
    constexpr std::partial_ordering operator<=>(const TemperatureDiff &o) const
    {
        return _celsius_diff <=> o._celsius_diff;
    }
    constexpr bool operator==(const TemperatureDiff &o) const
    {
        return _celsius_diff == o._celsius_diff;
    }

    /**
     * @brief 温差之间的加减法，返回新的温差对象
     * @param o 另一个温差对象
     */
    constexpr TemperatureDiff operator+(const TemperatureDiff &o) const
    {
        return TemperatureDiff(_celsius_diff + o._celsius_diff);
    }
    /**
     * @brief 温差之间的加减法，返回新的温差对象
     * @param o 另一个温差对象
     */
    constexpr TemperatureDiff operator-(const TemperatureDiff &o) const
    {
        return TemperatureDiff(_celsius_diff - o._celsius_diff);
    }

    // 友元声明
    friend class Temperature;
    friend constexpr TemperatureDiff operator""_C_diff(long double d);
    friend constexpr TemperatureDiff operator""_F_diff(long double d);
};

// 加减法

/**
 * @brief 两温度相减获得温差
 * @param other 另一个温度点
 * @return 两温度点的差值，单位为摄氏度差
 */
inline constexpr TemperatureDiff Temperature::operator-(const Temperature &other) const
{
    return TemperatureDiff::fromCelsius(this->_kelvin - other._kelvin);
}

/**
 * @brief 温度点加温差得到新温度点
 * @param diff 温差
 * @return 结果温度点对象
 */
inline constexpr Temperature Temperature::operator+(const TemperatureDiff &diff) const
{
    return Temperature(Temperature::KelvinTag{}, this->_kelvin + diff.toKelvin());
}

/**
 * @brief 温度点减温差得到新温度点
 * @param diff 温差
 * @return 结果温度点对象
 */
inline constexpr Temperature Temperature::operator-(const TemperatureDiff &diff) const
{
    return Temperature(Temperature::KelvinTag{}, this->_kelvin - diff.toKelvin());
}

// 字面量后缀实现

/**
 * @brief 用户定义字面量，允许直接写 25.0_C 来创建一个 Temperature 对象
 * @param c 摄氏度数值
 */
constexpr Temperature operator""_C(long double c)
{
    return Temperature::fromCelsius(static_cast<double>(c));
}

/**
 * @brief 用户定义字面量，允许直接写 25_C 来创建一个 Temperature 对象
 */
constexpr Temperature operator""_C(unsigned long long c)
{
    return Temperature::fromCelsius(static_cast<double>(c));
}

/**
 * @brief 用户定义字面量，允许直接写 77.0_F 来创建一个 Temperature 对象
 * @param f 华氏度数值
 */
constexpr Temperature operator""_F(long double f)
{
    return Temperature::fromFahrenheit(static_cast<double>(f));
}

/**
 * @brief 用户定义字面量，允许直接写 77_F 来创建一个 Temperature 对象
 */
constexpr Temperature operator""_F(unsigned long long f)
{
    return Temperature::fromFahrenheit(static_cast<double>(f));
}

/**
 * @brief 用户定义字面量，允许直接写 300.0_K 来创建一个 Temperature 对象
 * @param k 开尔文度数值
 */
constexpr Temperature operator""_K(long double k)
{
    return Temperature::fromKelvin(static_cast<double>(k));
}

/**
 * @brief 用户定义字面量，允许直接写 300_K 来创建一个 Temperature 对象
 */
constexpr Temperature operator""_K(unsigned long long k)
{
    return Temperature::fromKelvin(static_cast<double>(k));
}

/**
 * @brief 用户定义字面量，允许直接写 15.0_C_diff 来创建一个 TemperatureDiff 对象
 * @param d 摄氏度温差数值
 */
constexpr TemperatureDiff operator""_C_diff(long double d)
{
    return TemperatureDiff::fromCelsius(static_cast<double>(d));
}

/**
 * @brief 用户定义字面量，允许直接写 15_C_diff 来创建一个 TemperatureDiff 对象
 */
constexpr TemperatureDiff operator""_C_diff(unsigned long long d)
{
    return TemperatureDiff::fromCelsius(static_cast<double>(d));
}

/**
 * @brief 用户定义字面量，允许直接写 27.0_F_diff 来创建一个 TemperatureDiff 对象
 * @param d 华氏度温差数值
 */
constexpr TemperatureDiff operator""_F_diff(long double d)
{
    return TemperatureDiff::fromFahrenheit(static_cast<double>(d));
}

/**
 * @brief 用户定义字面量，允许直接写 27_F_diff 来创建一个 TemperatureDiff 对象
 */
constexpr TemperatureDiff operator""_F_diff(unsigned long long d)
{
    return TemperatureDiff::fromFahrenheit(static_cast<double>(d));
}
