/*
 This file is part of the lvtk_core module for the JUCE Library
 Copyright (c) 2013 - Michael Fisher <mfisher31@gmail.com>.
 
 Permission to use, copy, modify, and/or distribute this software for any purpose with
 or without fee is hereby granted, provided that the above copyright notice and this
 permission notice appear in all copies.
 
 THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef LVTK_PARAMETER_H
#define LVTK_PARAMETER_H

class Parameter
{
public:

    inline Parameter() : name ("Parameter"), symbol("parameter") { }
    inline Parameter (const String& name_, const String& symbol_)
        : name (name_), symbol (symbol_) { }

    inline ~Parameter() { }

    inline static double
    mapLog (double value, double min, double max, double k)
    {
        double y = (value - min) / (max - min);
        return log (1 + y * (exp (k) - 1)) / k;
    }


    inline static double
    mapExp (double value, double min, double max, double k)
    {
        double x = value;
        return min + (max - min) * ((exp (k * x) - 1) / (exp (k) - 1));
    }


    const String& getName()   const { return name; }
    const String& getSymbol() const { return symbol; }

    inline double min() const { return seed.min; }
    inline double max() const { return seed.max; }

    inline void
    set (double value)
    {
        if (value != seed.value && value >= seed.min && value <= seed.max)
        {
            seed.value = value;
        }
    }

    inline void
    set (double min, double max, double value)
    {
        seed.min = min;
        seed.max = max;
        set (value);
    }

    inline const double&
    value() const { return seed.value; }

    inline double
    normal() const
    {
        if (seed.min == 0.0 && seed.max == 1.0)
            return seed.value;

        return (seed.value - seed.min) / (seed.max - seed.min);
    }

    inline void
    setNormal (float val)
    {
        if (val < 0) val = 0;
        if (val > 1.0) val = 1.0;

        if (seed.min == 0.0 && seed.max == 1.0)
            set (val);
        else
            set (val * (seed.max - seed.min) + seed.min);
    }

    inline double
    scaleOf (double min, double max)
    {
        return normal() * (max - min) + min;
    }

    inline double
    logarithmic() const
    {
        return seed.min * pow (seed.max / seed.min, normal());
    }

    inline void
    reset()
    {
        seed.min      = 0;
        seed.max      = 1;
        seed.value    = 1;
    }

protected:

    inline void setName (const String& n) { name = n; }
    inline void setSymbol (const String& s) { symbol = s; }

private:

    String name, symbol;

    struct Seed
    {
        Seed() : min(0.0), max(1.0), value(1.0) { }
        String name, symbol;
        double min, max, value;
    };

    Seed seed;
};


#endif /* LVTK_PARAMETER_H */
