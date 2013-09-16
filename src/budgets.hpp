#pragma once

struct budgets
{
    static inline bool r_test(double e1, double e2, double e3)
    {
        return (1 - e2) < (e3 - e1*e3);
    }

    static inline double r_val(size_t g, double e1, double e2, double e3)
    {
        double nom, denom;

        if (r_test(e1, e2, e3)) {
            return 1/(e3 - e1*e3);
        } else {
            nom = g - g*e2 - g*e3 + g*e1*e3;
            denom = 1 + e1*e2*e3 - e2 - e1*e3;
            return nom/denom;
        }
    }

    static inline double helper_threshold(size_t g, double e1, double e2, double e3)
    {
        double r = r_val(g, e1, e2, e3);

        return r - r*e1;
    }

    static inline double helper_budget(size_t g, double e1, double e2, double e3)
    {
        double nom, denom, r = r_val(g, e1, e2, e3);

        nom = e3*r - r + g;
        denom = 2 - e2 - e3;

        return nom/denom;
    }

    static inline double helper_credits(size_t g, double e1, double e2, double e3)
    {
        return 1/(1 - e1);
    }

    static inline double source_credits(size_t g, double e1, double e2, double e3)
    {
        return 1/(1 - e3*e1);
    }

    static inline double source_budget(size_t g, double e1, double e2, double e3)
    {
        double nom, denom, r = r_val(g, e1, e2, e3);

        nom = g + r - r*e2;
        denom = 2 - e3 - e2;

        return nom/denom;
    }

    static inline double relay_credits(size_t g, double e1, double e2, double e3)
    {
        return 1;
        return 1/(1 - e3*e1);
    }

    static inline double relay_budget(size_t g, double e1, double e2, double e3, double e4)
    {
        return source_budget(g, e1, e2, e3) - g*(1 - e4);
    }
};  // namespace budgets
