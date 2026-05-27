#pragma once
#include "artgen/algorithms/EscapeTime.h"

namespace artgen {

class BurningShipAlgorithm : public EscapeTimeAlgorithm {
public:
    BurningShipAlgorithm();
    const char* name() const override { return "burning_ship"; }

protected:
    IterResult iterate(double cr, double ci) const override;
};

} // namespace artgen
